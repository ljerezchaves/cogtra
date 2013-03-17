/*
 * Copyright (C) 2012 Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Based on rc80211_minstrel.c:
 *   Copyright (C) 2008 Felix Fietkau <nbd@openwrt.org>
 *   Copyright (C) 2010 Tiago Chedraoui Silva <tsilva@lrc.ic.unicamp.br>
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer,
 *      without modification.
 *   2. Redistributions in binary form must reproduce at minimum a disclaimer
 *      similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
 *      redistribution must be conditioned upon including a substantially
 *      similar Disclaimer requirement for further binary redistribution.
 *   3. Neither the names of the above-listed copyright holders nor the names
 *      of any contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *   Alternatively, this software may be distributed under the terms of the
 *   GNU General Public License ("GPL") version 2 as published by the Free
 *   Software Foundation.
 *
 *   NO WARRANTY
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
 *   AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 *   THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
 *   OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *   IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *   THE POSSIBILITY OF SUCH DAMAGES.
 */

#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/debugfs.h>
#include <linux/random.h>
#include <linux/ieee80211.h>
#include <linux/slab.h>
#include <net/mac80211.h>
#include "rate.h"
#include "rc80211_cogtra.h"
#include "rc80211_cogtra_ht.h"



#define AVG_PKT_SIZE	1200

/* Number of bits for an average sized packet */
#define MCS_NBITS (AVG_PKT_SIZE << 3)

/* Number of symbols for a packet with (bps) bits per symbol */
#define MCS_NSYMS(bps) ((MCS_NBITS + (bps) - 1) / (bps))

/* Transmission time for a packet containing (syms) symbols */
#define MCS_SYMBOL_TIME(sgi, syms)                                      \
        (sgi ?                                                          \
          ((syms) * 18 + 4) / 5 :       /* syms * 3.6 us */             \
          (syms) << 2                   /* syms * 4 us */               \
        )

/* Transmit duration for the raw data part of an average sized packet */
#define MCS_DURATION(streams, sgi, bps) MCS_SYMBOL_TIME(sgi, MCS_NSYMS((streams) * (bps)))

/* MCS rate information for an MCS group */
#define MCS_GROUP(_streams, _sgi, _ht40) {                              \
        .streams = _streams,                                            \
        .flags =                                                        \
                (_sgi ? IEEE80211_TX_RC_SHORT_GI : 0) |                 \
                (_ht40 ? IEEE80211_TX_RC_40_MHZ_WIDTH : 0),             \
        .duration = {                                                   \
                MCS_DURATION(_streams, _sgi, _ht40 ? 54 : 26),          \
                MCS_DURATION(_streams, _sgi, _ht40 ? 108 : 52),         \
                MCS_DURATION(_streams, _sgi, _ht40 ? 162 : 78),         \
                MCS_DURATION(_streams, _sgi, _ht40 ? 216 : 104),        \
                MCS_DURATION(_streams, _sgi, _ht40 ? 324 : 156),        \
                MCS_DURATION(_streams, _sgi, _ht40 ? 432 : 208),        \
                MCS_DURATION(_streams, _sgi, _ht40 ? 486 : 234),        \
                MCS_DURATION(_streams, _sgi, _ht40 ? 540 : 260)         \
        }                                                               \
}


/*
 * To enable sufficiently targeted rate sampling, MCS rates are divided into
 * groups, based on the number of streams and flags (HT40, SGI) that they
 * use.
 */
const struct mcs_group minstrel_mcs_groups[] = {
	MCS_GROUP(1, 0, 0),
	MCS_GROUP(2, 0, 0),
#if MINSTREL_MAX_STREAMS >= 3
	MCS_GROUP(3, 0, 0),
#endif

	MCS_GROUP(1, 1, 0),
	MCS_GROUP(2, 1, 0),
#if MINSTREL_MAX_STREAMS >= 3
	MCS_GROUP(3, 1, 0),
#endif

	MCS_GROUP(1, 0, 1),
	MCS_GROUP(2, 0, 1),
#if MINSTREL_MAX_STREAMS >= 3
	MCS_GROUP(3, 0, 1),
#endif

	MCS_GROUP(1, 1, 1),
	MCS_GROUP(2, 1, 1),
#if MINSTREL_MAX_STREAMS >= 3
	MCS_GROUP(3, 1, 1),
#endif
};

int groupFlag = 0; 

static inline int cogtra_ht_aaa (unsigned int last_mean, unsigned int curr_mean, u32 last_thp, u32 curr_thp, unsigned int stdev) {
	/* Check for more than 10% thp variation */
	s32 delta = (s32)(last_thp / 10);
	s32 diff = (s32)(curr_thp - last_thp);
	
	if (abs (diff) > delta)
		return min (stdev + 10, (unsigned int) COGTRA_HT_MAX_STDEV);
	else
		return max (stdev - 10, (unsigned int) COGTRA_HT_MIN_STDEV);
} 

static int rc80211_cogtra_ht_normal_generator (int mean, int stdev_times100) {
	u32 rand = 0;
	int value = 0;
	int round_fac = 0;
	unsigned int i = 0;

	/* Convolution method to generate random normal numbers
	 * Refer to "Raj Jain", "The art of computer systems performance analysis",
	 * 1 edition, page 494.
	 *
	 * In LaTeX Syntax:
	 * \mathcal{N}(\mu, \sigma) = \mu + \sigma
	 * \dfrac{(\sum_{i=1}^{n}{\mathcal{U}_{i}}) -
	 * \frac{n}{2}}{\sqrt{\frac{n}{12}}}
	 *
	 * Since we are using twelve integer uniform numbers of one byte (n in
	 * [0,255]) instead of uniform numbers from 0 to 1, this code returns a
	 * variable from a normal curve with mean of ((12 * 255)-12)/2 = 1524 and
	 * stdev of 256 (1 byte) */
	for (i = 0; i < 12; i++) {
		get_random_bytes(&rand, 4);
		value += (int)(rand & 0x000000FF);
	}

	/* Calculating a rouding factor used to avoid problems during integer
	 * rounding. The following values were obtained from simulations. I'm still
	 * not sure about this, but is working well... */
	switch (stdev_times100) {
	case 25:
		round_fac = 45;
		break;
	case 30:
		round_fac = 50;
		break;
	case 35:
		round_fac = 55;
		break;
	case 40:
		round_fac = 57;
		break;
	case 45:
		round_fac = 60;
		break;
	case 50:
		round_fac = 63;
		break;
	case 55: case 60:
		round_fac = 66;
		break;
	case 65: case 70:
		round_fac = 68;
		break;
	case 75:
		round_fac = 70;
		break;
	case 80:
		round_fac = 72;
		break;
	case 85: case 90:
		round_fac = 73;
		break;
	case 95:
		round_fac = 74;
		break;
	case 100: case 105:
		round_fac = 75;
		break;
	case 110: case 115:
		round_fac = 76;
		break;
	case 120:
		round_fac = 77;
		break;
	case 125:
		round_fac = 78;
		break;
	case 130: case 135:
		round_fac = 79;
		break;
	case 140: case 145:
		round_fac = 80;
		break;
	case 150: case 155:
		round_fac = 81;
		break;
	case 160: case 165: case 170:
		round_fac = 82;
		break;
	case 175: case 180: case 185:
		round_fac = 83;
		break;
	case 190: case 195:
		round_fac = 84;
		break;
	case 200:
		round_fac = 85;
		break;
	default:
		if (stdev_times100 < 25)
			round_fac = 40;
		else
			round_fac = 87;	
	}

	/* Converting the above normal random variable to the appropriate curve
	 * from parameter mean and stdev (considering stdev_times100 = stdev * 100)
	 * return mean + ((value - 1524) * stdev) / 256)) */
	return (int)(mean + ((value - 1524) * stdev_times100)/(round_fac * 256));
}

/*
static inline int minstrel_get_duration(int index) {
	const struct mcs_group *group = &minstrel_mcs_groups[index / MCS_GROUP_RATES];
	return group->duration[index % MCS_GROUP_RATES];
}*/


/*
 * Calculate throughput based on the average A-MPDU length, taking into account
 * the expected number of retransmissions and their expected length
 */
static void minstrel_ht_calc_tp(struct cogtra_priv *cp, struct cogtra_ht_sta *ci, int group, int rate) {
	struct minstrel_rate_stats *cr;
	unsigned int usecs;

	cr = &ci->groups[group].rates[rate];

	//if (mr->probability < MINSTREL_FRAC(1, 10)) {
		//mr->cur_tp = 0;
		//return;
	// }

	usecs = ci->overhead / MINSTREL_TRUNC(ci->avg_ampdu_len);
	usecs += minstrel_mcs_groups[group].duration[rate];
	cr->cur_tp = MINSTREL_TRUNC((1000000 / usecs) * cr->cur_prob); //USar a média ou a curr_prob??
}

static void cogtra_ht_set_rate(struct ieee80211_tx_rate *rate, int index){
    const struct mcs_group *group = &minstrel_mcs_groups[index / MCS_GROUP_RATES];
    //rate->idx = index % MCS_GROUP_RATES + (group->streams - 1) * MCS_GROUP_RATES;
	rate->idx = index;
	rate->flags = IEEE80211_TX_RC_MCS | group->flags;
	rate->count = 2U;
}

static void cogtra_ht_tx_rate_populate(struct cogtra_ht_sta *ci) {
	cogtra_ht_set_rate(&ci->tx_rates[0], ci->random_rate_mcs);
	cogtra_ht_set_rate(&ci->tx_rates[1], ci->max_tp_rate_mcs);
	cogtra_ht_set_rate(&ci->tx_rates[2], ci->max_prob_rate_mcs);
	cogtra_ht_set_rate(&ci->tx_rates[3], 0);
}

static inline struct minstrel_rate_stats * minstrel_get_ratestats(struct cogtra_ht_sta *ci, int index){
        return &ci->groups[index / MCS_GROUP_RATES].rates[index % MCS_GROUP_RATES];
}

static void cogtra_ht_update_stats (struct cogtra_priv *cp, struct cogtra_ht_sta *ci) {
	struct minstrel_mcs_group_data *cg;
    struct minstrel_rate_stats *cr;
	
	unsigned int random_rate_gix, random_rt;
	unsigned int max_tp_rate_gix, max_tp_rate;
	unsigned int max_prob_rate_gix, max_prob_rate ;
	
	int i,j;
	
	ci->up_stats_counter++;
	
	if (ci->ampdu_packets > 0) {
		ci->avg_ampdu_len = ((MINSTREL_FRAC(ci->ampdu_len, ci->ampdu_packets) * (100 - cp->ewma_level)) + (ci->avg_ampdu_len * cp->ewma_level)) / 100;
		ci->ampdu_len = 0;
		ci->ampdu_packets = 0;
	}

	/* For each supported rate... */
	for (i = 0; i < ARRAY_SIZE(minstrel_mcs_groups); i++) {
		unsigned int old_stdev, old_mean;
		u32 old_thp, new_thp;
		
		int random_gix = 0;
		u32 usecs;
		
		unsigned int max_tp_gix = 0, max_prob_gix = 0;
		u32 max_tp_value = 0, max_prob_value = 0;		
		
		cg = &ci->groups[i];
		if (!cg->supported)
			continue;
		
		old_stdev 	= cg->cur_stdev;
		old_mean 	= cg->max_tp_rate_gix;
		old_thp 	= cg->rates[cg->random_rate_gix].avg_tp;
		
		cg->random_rate_gix = 0;
		cg->max_tp_rate_gix = 0;
		cg->max_prob_rate_gix = 0;

		for (j = 0; j < MCS_GROUP_RATES; j++) {
			if (!(cg->supported & BIT(j)))
			continue;
			
			cr = &cg->rates[j];
			
			/* To avoid rounding issues, probabilities scale from 0 (0%)
			 * to 1800 (100%) */
			if (cr->attempts) {
				usecs = minstrel_mcs_groups[i].duration[j];
				if (!usecs)
					usecs = 1000000;

				/* Update thp and prob for last interval */
				cr->cur_prob 	= (cr->success * 1800) / cr->attempts;
				minstrel_ht_calc_tp(cp, ci, i, j);

				/* Update average thp and prob with EWMA */
				cr->avg_prob = cr->avg_prob ? ((cr->cur_prob * (100 -
						cp->ewma_level)) + (cr->avg_prob * cp->ewma_level)) / 100 :
						cr->cur_prob;
				cr->avg_tp = cr->avg_tp ? ((cr->cur_tp * (100 - cp->ewma_level)) +
						(cr->avg_tp * cp->ewma_level)) / 100 : cr->cur_tp;

				/* Update success and attempt counters */
				cr->succ_hist += cr->success;
				cr->att_hist += cr->attempts;
			
			}

			/* Update success and attempt counters */
			cr->last_success = cr->success;
			cr->last_attempts = cr->attempts;
			cr->success = 0;
			cr->attempts = 0;
		}
		new_thp = cg->rates[cg->random_rate_gix].avg_tp;

		/* Look for the rate with highest throughput and probability */
		for (j = 0; j < MCS_GROUP_RATES; j++) {
			struct minstrel_rate_stats *cr = &cg->rates[j];
			if (max_tp_value < cr->avg_tp) {
				max_tp_gix = j;
				max_tp_value = cr->avg_tp;
			}
			if (max_prob_value < cr->avg_prob) {
				max_prob_gix = j;
				max_prob_value = cr->avg_prob;
			}
		}
		
		cg->max_tp_rate_gix = max_tp_gix;
		cg->max_prob_rate_gix = max_prob_gix;
			
		/* Adjusting stdev with CogTRA_HT AAA */
		cg->cur_stdev = cogtra_ht_aaa (old_mean, cg->max_tp_rate_gix,
			old_thp, new_thp, old_stdev);

		/* Get a new random rate for next interval (using a normal distribution) */
		random_gix = rc80211_cogtra_ht_normal_generator ((int)cg->max_tp_rate_gix,
				(int)cg->cur_stdev);
		cg->random_rate_gix = (unsigned int)(max (0, min (random_gix,
						(int)((int)(MCS_GROUP_RATES) - 1))));
		cg->rates[cg->random_rate_gix].times_called++;
		
	}
	
	random_rt = 0;
	max_tp_rate = 0;
	max_prob_rate = 0;
	
	
	//Qual dos dois grupos?
		if(groupFlag == 0){
			random_rate_gix = 0;
			random_rt = ci->groups[0].random_rate_gix;
			groupFlag = 1;
		}else{
			random_rate_gix = 1;
			random_rt = ci->groups[1].random_rate_gix;
			groupFlag = 0;
		}
			
	/* For each supported group... */
	for (i = 0; i < ARRAY_SIZE(minstrel_mcs_groups); i++) {
		cg = &ci->groups[i];
		if (!cg->supported)
			continue;
			
		if (max_tp_rate < cg->rates[max_tp_rate_gix].avg_tp) {
			max_tp_rate_gix = i;
			max_tp_rate = cg->max_tp_rate_gix;
		}
		if (max_prob_rate < cg->rates[max_prob_rate_gix].avg_prob) {
			max_prob_rate_gix = i;
			max_prob_rate = cg->max_prob_rate_gix;
		}	
	}
		//ERRO
		ci->random_rate_mcs = (random_rate_gix * MCS_GROUP_RATES) + random_rt;
		ci->max_tp_rate_mcs = (max_tp_rate_gix * MCS_GROUP_RATES) + max_tp_rate;
		ci->max_prob_rate_mcs = (max_prob_rate_gix * MCS_GROUP_RATES) + max_prob_rate;
		

		cogtra_ht_tx_rate_populate (ci);

		/* Adjust update_interval dependending on the random rate */
		/* RANDOM < BEST || RANDOM.PROB < 10% */
		/*if ((ci->rates[ci->random_rate_ndx].perfect_tx_time >
					ci->rates[ci->max_tp_rate_mcs].perfect_tx_time) ||
				(ci->rates[ci->random_rate_ndx].avg_prob < 180)) 
			ci->update_interval = COGTRA_HT_RECOVERY_INTERVAL;
		else
			ci->update_interval = COGTRA_HT_UPDATE_INTERVAL;
		}*/
}

static int minstrel_ht_get_group_idx(struct ieee80211_tx_rate *rate) {
	int streams = (rate->idx / MCS_GROUP_RATES) + 1;
	u32 flags = IEEE80211_TX_RC_SHORT_GI | IEEE80211_TX_RC_40_MHZ_WIDTH;
	int i;

	for (i = 0; i < ARRAY_SIZE(minstrel_mcs_groups); i++) {
		if (minstrel_mcs_groups[i].streams != streams)
			continue;
		if (minstrel_mcs_groups[i].flags != (rate->flags & flags))
			continue;

		return i;
	}

	WARN_ON(1);
	return 0;
}

static bool minstrel_ht_txstat_valid(struct ieee80211_tx_rate *rate) {
	if (!rate->count)
		return false;

	if (rate->idx < 0)
		return false;

	return !!(rate->flags & IEEE80211_TX_RC_MCS);
}

static void cogtra_ht_tx_status (void *priv, struct ieee80211_supported_band *sband, struct ieee80211_sta *sta, void *priv_sta, struct sk_buff *skb){
	struct cogtra_ht_sta_priv *csp = priv_sta;
	struct cogtra_ht_sta *ci = &csp->ht;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_tx_rate *ar = info->status.rates;
	struct minstrel_rate_stats *rate;
	int i, last = 0,group;
 
	if(!csp->is_ht){
		return mac80211_cogtra.tx_status(priv,sband, sta, &csp->legacy,skb);
	}

	/* This packet was aggregated but doesn't carry status info */
	if ((info->flags & IEEE80211_TX_CTL_AMPDU) &&
	    !(info->flags & IEEE80211_TX_STAT_AMPDU))
		return;

	if (!(info->flags & IEEE80211_TX_STAT_AMPDU)) {
		info->status.ampdu_ack_len = (info->flags & IEEE80211_TX_STAT_ACK ? 1 : 0);
		info->status.ampdu_len = 1;
	}
	
	ci->ampdu_packets++;
	ci->ampdu_len += info->status.ampdu_len;
	
	/*for (i = 0; !last; i++) {
		last = (i == IEEE80211_TX_MAX_RATES - 1) ||
		       !minstrel_ht_txstat_valid(&ar[i + 1]);

		if (!minstrel_ht_txstat_valid(&ar[i]))
			break;

		group = minstrel_ht_get_group_idx(&ar[i]);
		rate = &ci->groups[group].rates[ar[i].idx % 8];

		if (last)
			rate->success += info->status.ampdu_ack_len;

		rate->attempts += ar[i].count * info->status.ampdu_len;
		ci->update_counter += ar[i].count * info->status.ampdu_len;
	}*/
	
	for (i = 0; i < IEEE80211_TX_MAX_RATES; i++) {
	
	
		/* A rate idx -1 means that the following rates weren't used in tx */
		if (ar[i].idx < 0)
			break;
			
		group = minstrel_ht_get_group_idx(&ar[i]);
		rate = &ci->groups[group].rates[ar[i].idx % 8];
				
		/* Increasing attempts counter */
		rate->attempts += ar[i].count * info->status.ampdu_len;
		ci->update_counter += ar[i].count * info->status.ampdu_len;
		
	
		/* If it is the last used rate and resultesd in tx success, also
		 * increse the success counter */
		if ((i != IEEE80211_TX_MAX_RATES - 1) && (ar[i + 1].idx < 0)) {
			rate->success += info->status.ampdu_ack_len;
		}
		
		printk("ar[%d] : idx %d | g:%d r:%d | count %u ampdu_len %u\n",i,ar[i].idx,group,ar[i].idx % 8,ar[i].count,info->status.ampdu_len );
	}
	printk("\n");

}


/* cogtra_ht_get_rate is called just before each frame tx and sets the appropriate
 * data rate to be used */
static void
cogtra_ht_get_rate (void *priv, struct ieee80211_sta *sta, void *priv_sta, struct ieee80211_tx_rate_control *txrc) {
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(txrc->skb);
	struct ieee80211_tx_rate *ar = info->control.rates;
	struct cogtra_ht_sta_priv *csp = priv_sta;
	struct cogtra_ht_sta *ci = &csp->ht;
	struct cogtra_priv *cp = priv;
	
	bool mrr;
	int i;	
	
	
	/* Check for management or control packet, which should be transmitted
	 * unsing lower rate */
	if (rate_control_send_low (sta, priv_sta, txrc))
		return;

	if(!csp->is_ht){
		return mac80211_cogtra.get_rate(priv, sta, &csp->legacy, txrc);
	}
	
	info->flags |= ci->tx_flags;
	
	/* Check MRR hardware support */
	mrr = cp->has_mrr && !txrc->rts && !txrc->bss_conf->use_cts_prot;

	
	/* Check the need of an update_stats based on update_interval */
	printk("Update Counter: %lu\n Update Interval: %u\n",ci->update_counter,ci->update_interval);
	if (ci->update_counter >= ci->update_interval)
		cogtra_ht_update_stats (cp, ci);
	
	
	/* Setting up tx rate information.
	 * Be careful to convert ndx indexes into ieee80211_tx_rate indexes */

		
	if (!mrr) {
		ar[0] = ci->tx_rates[0];
		ar[1].idx = -1;
		ar[1].count = 0;
		return;
	}

	/* MRR setup */
	for (i = 0; i < 4; i++) {
		ar[i] = ci->tx_rates[i];
	}
}


/* cogtra_ht_rate_init is called after cogtra_ht_alloc_sta to check and populate
 * information for supported rates */
static void
cogtra_ht_update_caps (void *priv, struct ieee80211_supported_band *sband, struct ieee80211_sta *sta, void *priv_sta, enum nl80211_channel_type oper_chan_type) {
	struct cogtra_ht_sta_priv *csp = priv_sta;
	struct cogtra_priv *cp = priv;
	struct cogtra_ht_sta *ci = &csp->ht;
	struct ieee80211_mcs_info *mcs = &sta->ht_cap.mcs;
	struct ieee80211_local *local = hw_to_local(cp->hw);
	u16 sta_cap = sta->ht_cap.cap;
	unsigned int i = 0;
	int n_supported = 0;
	int ack_dur;
	int stbc;

	/* fall back to the old cogtra for legacy stations */
	if(!sta->ht_cap.ht_supported){
		csp->is_ht = false;
		memset(&csp->legacy, 0, sizeof(csp->legacy));
		csp->legacy.r = csp->r;
		csp->legacy.t = csp->t;
		return mac80211_cogtra.rate_init(priv,sband,sta,&csp->legacy);
	}

	csp->is_ht = true;
	memset(ci,0,sizeof(*ci));

	ack_dur = ieee80211_frame_duration(local, 10, 60, 1, 1);
	ci->overhead = ieee80211_frame_duration(local, 0, 60, 1, 1) + ack_dur;
	ci->overhead_rtscts = ci->overhead + 2 * ack_dur;

	ci->avg_ampdu_len = MINSTREL_FRAC(1, 1);

	stbc = (sta_cap & IEEE80211_HT_CAP_RX_STBC) >>
		IEEE80211_HT_CAP_RX_STBC_SHIFT;
	ci->tx_flags |= stbc << IEEE80211_TX_CTL_STBC_SHIFT;

	if (sta_cap & IEEE80211_HT_CAP_LDPC_CODING)
		ci->tx_flags |= IEEE80211_TX_CTL_LDPC;

	if (oper_chan_type != NL80211_CHAN_HT40MINUS &&
	    oper_chan_type != NL80211_CHAN_HT40PLUS)
		sta_cap &= ~IEEE80211_HT_CAP_SUP_WIDTH_20_40;

	for (i = 0; i < ARRAY_SIZE(ci->groups); i++) {
		u16 req = 0;

		ci->groups[i].supported = 0;
		if (minstrel_mcs_groups[i].flags & IEEE80211_TX_RC_SHORT_GI) {
			if (minstrel_mcs_groups[i].flags & IEEE80211_TX_RC_40_MHZ_WIDTH)
				req |= IEEE80211_HT_CAP_SGI_40;
			else
				req |= IEEE80211_HT_CAP_SGI_20;
		}

		if (minstrel_mcs_groups[i].flags & IEEE80211_TX_RC_40_MHZ_WIDTH)
			req |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;

		if ((sta_cap & req) != req)
			continue;

		ci->groups[i].supported =
			mcs->rx_mask[minstrel_mcs_groups[i].streams - 1];

		if (ci->groups[i].supported){
			n_supported++;
			ci->groups[i].cur_stdev = COGTRA_HT_MAX_STDEV;
		}
	}

	if (!n_supported){
		csp->is_ht = false;
		memset(&csp->legacy, 0, sizeof(csp->legacy));
		csp->legacy.r = csp->r;
		csp->legacy.t = csp->t;
		return mac80211_cogtra.rate_init(priv,sband,sta,&csp->legacy);
	}
	
	ci->n_groups = n_supported;
	ci->n_rates = ci->n_groups * MCS_GROUP_RATES;

	/*Antes no alloc_sta()*/
	ci->update_interval = COGTRA_HT_UPDATE_INTERVAL;
	ci->update_counter = 0UL;

	return;
}

static void
cogtra_ht_rate_init (void *priv, struct ieee80211_supported_band *sband, struct ieee80211_sta *sta, void *priv_sta) {
	struct cogtra_priv *cp = priv;
	cogtra_ht_update_caps(priv, sband, sta, priv_sta, cp->hw->conf.channel_type);
}

static void
cogtra_ht_rate_update (void *priv, struct ieee80211_supported_band *sband, struct ieee80211_sta *sta, void *priv_sta, u32 changed, enum nl80211_channel_type oper_chan_type) {
	cogtra_ht_update_caps(priv, sband, sta, priv_sta, oper_chan_type);
}

/* cogtra_ht_alloc_sta is called every time a new station joins the networks */
static void *
cogtra_ht_alloc_sta (void *priv, struct ieee80211_sta *sta, gfp_t gfp) {
	struct ieee80211_supported_band *sband;
	struct cogtra_ht_sta_priv *csp;
	struct cogtra_priv *cp = priv;
	struct ieee80211_hw *hw = cp->hw;
	int max_rates = 0;
	int i;


	csp = kzalloc (sizeof (struct cogtra_ht_sta_priv), gfp);
	if (!csp)
		return NULL;

	/* Get the maximum number of rates based on available bands */
	for (i = 0; i < IEEE80211_NUM_BANDS; i++) {
		sband = hw->wiphy->bands[i];
		if (sband && sband->n_bitrates > max_rates)
			max_rates = sband->n_bitrates;
	}

	/* Memory allocation for rates that can be used by this station */
	csp->r = kzalloc (sizeof (struct cogtra_rate) * (max_rates), gfp);
	if (!csp->r) {
		kfree(csp);
		return NULL;
	}

	/* Memory allocation for mrr chain table */
	csp->t = kzalloc (sizeof (struct chain_table) * 4, gfp);
	if (!csp->t) {
		kfree(csp->r);
		kfree(csp);
		return NULL;
	}

	return csp;
}


/* cogtra_ht_free_sta used to release memory when a station leaves the network */
static void
cogtra_ht_free_sta (void *priv, struct ieee80211_sta *sta, void *priv_sta) {
	struct cogtra_ht_sta_priv *csp = priv_sta;

	kfree (csp->t);
	kfree (csp->r);
	kfree (csp);
}


/* chama o alloc do cogtra */
static void *
cogtra_ht_alloc (struct ieee80211_hw *hw, struct dentry *debugfsdir) {
	return mac80211_cogtra.alloc(hw, debugfsdir);
}


/* cogtra_ht_free called once before turning off the wireless interface */
static void
cogtra_ht_free (void *priv){
	kfree (priv);
}

struct rate_control_ops mac80211_cogtra_ht = {
	.name = "cogtra_ht",
	.tx_status = cogtra_ht_tx_status,
	.get_rate = cogtra_ht_get_rate,
	.rate_init = cogtra_ht_rate_init,
	.rate_update = cogtra_ht_rate_update,
	.alloc = cogtra_ht_alloc,
	.free = cogtra_ht_free,
	.alloc_sta = cogtra_ht_alloc_sta,
	.free_sta = cogtra_ht_free_sta,
#ifdef CONFIG_MAC80211_DEBUGFS
	.add_sta_debugfs = cogtra_ht_add_sta_debugfs,
	.remove_sta_debugfs = cogtra_ht_remove_sta_debugfs,
#endif
};

int __init
rc80211_cogtra_ht_init(void)
{
	printk ("LJC CogTRA_HT algorithm.\n");
	return ieee80211_rate_control_register (&mac80211_cogtra_ht);
}

void
rc80211_cogtra_ht_exit(void)
{
	ieee80211_rate_control_unregister (&mac80211_cogtra_ht);
}
