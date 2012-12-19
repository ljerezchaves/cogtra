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

/* COGTRA Agressivness Self-Adjustment (ASA) */
static inline int
cogtra_asa (unsigned int last_mean, unsigned int curr_mean, u32 last_thp, 
		u32 curr_thp, unsigned int stdev)
{
	/* Check for more than ASA_DELTA thp variation */
	s32 delta = (s32)(last_thp / COGTRA_ASA_DELTA);
	s32 diff = (s32)(curr_thp - last_thp);

	if (abs (diff) > delta)
		return min (stdev + 10, (unsigned int) COGTRA_MAX_STDEV);
	else
		return max (stdev - 10, (unsigned int) COGTRA_MIN_STDEV);
} 

/* COGTRA Normal random number generator. 
 * stdev parameter has to be stedv * 100 (to avoid FP operations) */
static int
rc80211_cogtra_normal_generator (int mean, int stdev_times100)
{
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
	case 20:
		round_fac = 40;
		break;
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


/* Converting mac80211 rate index into local array index */
static inline int
rix_to_ndx (struct cogtra_sta_info *ci, int rix)
{
	int i;
	for (i = ci->n_rates - 1; i >= 0; i--)
		if (ci->r[i].rix == rix)
			break;
	return i;
}


/* Sort rates by bitrates. Is called once by cogtra_rate_init */
static void
sort_bitrates (struct cogtra_sta_info *ci, int n_rates)
{
	int i, j;
	struct cogtra_rate cr_key;

	/* Insertion sort */
	for (j = 1; j < n_rates; j++) {
		cr_key = ci->r[j];
		i = j - 1;
		while ((i >= 0) && (ci->r[i].bitrate > cr_key.bitrate)) {
			ci->r[i+1] = ci->r[i];
			i--;
		}
		ci->r[i+1] = cr_key;
	}
}


/* cogtra_mrr_populate fill in the multirate retry chain in acordance with random
 * and best data rates*/
static void
cogtra_mrr_populate (struct cogtra_sta_info *ci)
{
	struct chain_table *ct = ci->t;
	memset (ct, 0, 4 * sizeof (*ct));
	
	/* Random COGTRA rate */
	ct[0].type = 0;	
	ct[0].rix = ci->r[ci->random_rate_ndx].rix;
	ct[0].bitrate = ci->r[ci->random_rate_ndx].bitrate;
	ct[0].count = 2U;

	/* Best throughput */ 
	ct[1].type = 1;	
	ct[1].rix = ci->r[ci->max_tp_rate_ndx].rix;
	ct[1].bitrate = ci->r[ci->max_tp_rate_ndx].bitrate;
	ct[1].count = 2U;

	/* Best probability */
	ct[2].type = 2;
	ct[2].rix = ci->r[ci->max_prob_rate_ndx].rix;
	ct[2].bitrate = ci->r[ci->max_prob_rate_ndx].bitrate;
	ct[2].count = 2U; 

	/* Lowest rate */
	ct[3].type = 3;
	ct[3].rix = ci->lowest_rix;
	ct[3].bitrate = ci->r[0].bitrate;
	ct[3].count = 2U;
}


/* cogtra_update_stats is called by cogtra_get_rate when the update_interval
 * expires. It sumarizes statistics information and use cogtra core algorithm to
 * select the rate to be used during next interval. */
static void
cogtra_update_stats (struct cogtra_priv *cp, struct cogtra_sta_info *ci, 
		struct ieee80211_sta *sta)
{
	u32 usecs;
	u32 max_tp = 0, max_prob = 0;
	unsigned int i, max_tp_ndx, max_prob_ndx;
	unsigned int old_stdev, old_mean;
	u32 old_thp, new_thp;
	int random = 0;
	unsigned long j, diff = 0;

	ci->up_stats_counter++;

	old_stdev = ci->cur_stdev;
	old_mean = ci->max_tp_rate_ndx;
	old_thp = ci->r[ci->random_rate_ndx].avg_tp;

	/* For each supported rate... */
	for (i = 0; i < ci->n_rates; i++) {
		struct cogtra_rate *cr = &ci->r[i]; 

		/* To avoid rounding issues, probabilities scale from 0 (0%)
		 * to 1800 (100%) */
		if (cr->attempts) {
			
			usecs = cr->perfect_tx_time;
			if (!usecs)
				usecs = 1000000;

			/* Update thp and prob for last interval */
			cr->cur_prob = (cr->success * 1800) / cr->attempts;
			cr->cur_tp = cr->cur_prob * (1000000 / usecs);

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

	new_thp = ci->r[ci->random_rate_ndx].avg_tp;

	/* Look for the rate with highest throughput and probability */
	for (i = 0; i < ci->n_rates; i++) {
		struct cogtra_rate *cr = &ci->r[i];
		if (max_tp < cr->avg_tp) {
			max_tp_ndx = i;
			max_tp = cr->avg_tp;
		}
		if (max_prob < cr->avg_prob) {
			max_prob_ndx = i;
			max_prob = cr->avg_prob;
		}
	}
	ci->max_tp_rate_ndx = max_tp_ndx;
	ci->max_prob_rate_ndx = max_prob_ndx;
	ci->update_counter = 0UL;

#ifdef COGTRA_USE_ASA
	/* Adjusting stdev with CogTRA AAA */
	ci->cur_stdev = cogtra_asa (old_mean, ci->max_tp_rate_ndx,
			old_thp, new_thp, old_stdev);
#endif

	/* Get a new random rate for next interval (using a normal distribution) */
	random = rc80211_cogtra_normal_generator ((int)ci->max_tp_rate_ndx,
			(int)ci->cur_stdev);
	ci->random_rate_ndx = (unsigned int)(max (0, min (random,
					(int)((int)(ci->n_rates) - 1))));
	ci->r[ci->random_rate_ndx].times_called++;

	cogtra_mrr_populate (ci);

#ifdef COGTRA_USE_ISA
	/* Adjust update_interval dependending on the random rate */
	/* RANDOM < BEST || RANDOM.PROB < 10% */
	if ((ci->r[ci->random_rate_ndx].perfect_tx_time >
				ci->r[ci->max_tp_rate_ndx].perfect_tx_time) ||
			(ci->r[ci->random_rate_ndx].avg_prob < 180)) 
		ci->update_interval = COGTRA_RECOVERY_INTERVAL;
	else
		ci->update_interval = COGTRA_UPDATE_INTERVAL;
#endif

#ifdef CONFIG_MAC80211_DEBUGFS
	/* History table information */
	if (ci->dbg_idx < COGTRA_DEBUGFS_HIST_SIZE) {
		struct cogtra_hist_info *t = &ci->hi[ci->dbg_idx];
		struct sta_info *si = container_of (sta, struct sta_info, sta);

		j = jiffies;
		diff = (long)j - (long)ci->last_time;
		ci->last_time = j;
		
		t->rrate = ci->r[ci->random_rate_ndx].bitrate;
		t->brate = ci->r[ci->max_tp_rate_ndx].bitrate;
		t->prate = ci->r[ci->max_tp_rate_ndx].bitrate;
		t->currstdev = ci->cur_stdev;
		t->pktinterval = ci->update_interval;
		t->lastsignal = si->last_signal;
		t->msec = diff * 1000 / HZ;
		ci->dbg_idx++;
	}
#endif
}


/* cogtra_tx_status is called just after frame tx and it is used to update
 * statistics information for the used rate */
static void
cogtra_tx_status (void *priv, struct ieee80211_supported_band *sband, 
		struct ieee80211_sta *sta, void *priv_sta, struct sk_buff *skb)
{
	struct cogtra_sta_info *ci = priv_sta;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_tx_rate *ar = info->status.rates;
	struct chain_table *ct = ci->t;
	int i, ndx;
	int success;
 
 	/* Checking for a success in frame transmission */
	success = !!(info->flags & IEEE80211_TX_STAT_ACK);

	/* Updating information for each used rate */
	for (i = 0; i < IEEE80211_TX_MAX_RATES; i++) {
	
		/* A rate idx -1 means that the following rates weren't used in tx */
		if (ar[i].idx < 0)
			break;

		/* Getting the local idx for this ieee80211_tx_rate */
		ndx = rix_to_ndx (ci, ar[i].idx);
		if (ndx < 0)
			continue;
	
		/* Increasing attempts counter */
		ci->r[ndx].attempts += ar[i].count;
		ci->update_counter += ar[i].count;
		ct[i].att += ar[i].count;
	
		/* If it is the last used rate and resultesd in tx success, also
		 * increse the success counter */
		if ((i != IEEE80211_TX_MAX_RATES - 1) && (ar[i + 1].idx < 0)) {
			ci->r[ndx].success += success;
			ct[i].suc += success;
		}
	}
}


/* cogtra_get_rate is called just before each frame tx and sets the appropriate
 * data rate to be used */
static void
cogtra_get_rate (void *priv, struct ieee80211_sta *sta, void *priv_sta,
		struct ieee80211_tx_rate_control *txrc)
{
	struct sk_buff *skb = txrc->skb;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct cogtra_sta_info *ci = priv_sta;
	struct cogtra_priv *cp = priv;
	struct ieee80211_tx_rate *ar = info->control.rates;
	bool mrr;
	int i;

	/* Check for management or control packet, which should be transmitted
	 * unsing lower rate */
	if (rate_control_send_low (sta, priv_sta, txrc))
		return;

	/* Check MRR hardware support */
	mrr = cp->has_mrr && !txrc->rts && !txrc->bss_conf->use_cts_prot;

	/* Check the need of an update_stats based on update_interval */
	if (ci->update_counter >= ci->update_interval)
		cogtra_update_stats (cp, ci, sta);

	/* Setting up tx rate information. 
	 * Be careful to convert ndx indexes into ieee80211_tx_rate indexes */

#ifdef COGTRA_USE_MRR
	if (mrr) {
		/* MRR setup */
		for (i = 0; i < 4; i++) {
			ar[i].idx = ci->t[i].rix;
			ar[i].count = ci->t[i].count;
		}
		return;	
	}
#endif

	/* Executed when no MRR support or COGTRA_USE_MRR disable */	
	ar[0].idx = ci->r[ci->random_rate_ndx].rix;
	ar[0].count = cp->max_retry;
	ar[1].idx = -1;
	ar[1].count = 0;
	return;
}


/* calc_rate_durations estimates the tx time for a single data frame of 1200
 * bytes and for an ack frame of standard size for a specific rate */
static void
calc_rate_durations (struct ieee80211_local *local, struct cogtra_rate *cr,
		struct ieee80211_rate *rate)
{
	int erp = !!(rate->flags & IEEE80211_RATE_ERP_G);

	cr->perfect_tx_time = ieee80211_frame_duration (local, 1200, 
			rate->bitrate, erp, 1);
	cr->ack_time = ieee80211_frame_duration (local, 10, 
			rate->bitrate, erp, 1);
}


/* cogtra_rate_init is called after cogtra_alloc_sta to check and populate
 * information for supported rates */
static void
cogtra_rate_init (void *priv, struct ieee80211_supported_band *sband,
		struct ieee80211_sta *sta, void *priv_sta)
{
	struct cogtra_sta_info *ci = priv_sta;
	struct cogtra_priv *cp = priv;
	struct ieee80211_local *local = hw_to_local(cp->hw);
	struct ieee80211_rate *ctl_rate;
	unsigned int i, n = 0;

	/* Get the lowest index rate and calculate the duration of a ack tx */
	ci->lowest_rix = rate_lowest_index (sband, sta);
	ctl_rate = &sband->bitrates[ci->lowest_rix];

	/* Populating information for each supported rate */
	for (i = 0; i < sband->n_bitrates; i++) {
		struct cogtra_rate *cr = &ci->r[n];

		/* Check rate support */
		if (!rate_supported (sta, sband->band, i))
			continue;

		n++;
		memset (cr, 0, sizeof (*cr));

		/* Set index, bitrate and tx duration */
		cr->rix = i;
		cr->bitrate = sband->bitrates[i].bitrate / 5;
		calc_rate_durations (local, cr, &sband->bitrates[i]);
	}

	/* Sort rates based on bitrate */
	sort_bitrates (ci, n);

	/* Mark unsupported rates with rix = -1 */
	for (i = n; i < sband->n_bitrates; i++)
		ci->r[i].rix = -1;

	ci->cur_stdev = COGTRA_MAX_STDEV;
	ci->n_rates = n;
	ci->update_counter = 0UL;
}


/* cogtra_alloc_sta is called every time a new station joins the networks */
static void *
cogtra_alloc_sta (void *priv, struct ieee80211_sta *sta, gfp_t gfp)
{
	struct ieee80211_supported_band *sband;
	struct cogtra_sta_info *ci;
	struct cogtra_priv *cp = priv;
	struct ieee80211_hw *hw = cp->hw;
	int max_rates = 0;
	int i;

	ci = kzalloc (sizeof (struct cogtra_sta_info), gfp);
	if (!ci)
		return NULL;

	/* Get the maximum number of rates based on available bands */
	for (i = 0; i < IEEE80211_NUM_BANDS; i++) {
		sband = hw->wiphy->bands[i];
		if (sband && sband->n_bitrates > max_rates)
			max_rates = sband->n_bitrates;
	}

	/* Memory allocation for rates that can be used by this station */
	ci->r = kzalloc (sizeof (struct cogtra_rate) * (max_rates), gfp);
	if (!ci->r) {
		kfree(ci);
		return NULL;
	}

	/* Memory allocation for mrr chain table */
	ci->t = kzalloc (sizeof (struct chain_table) * 4, gfp);
	if (!ci->t) {
		kfree(ci->r);
		kfree(ci);
		return NULL;
	}

#ifdef CONFIG_MAC80211_DEBUGFS
   	ci->hi = kzalloc (sizeof (struct cogtra_hist_info) * COGTRA_DEBUGFS_HIST_SIZE, gfp);
	if (!ci->hi) {
		kfree (ci->t);
		kfree (ci->r);
		kfree (ci);
		return NULL;
	}
#endif

	ci->update_interval = COGTRA_UPDATE_INTERVAL;
	ci->update_counter = 0UL;
	ci->last_time = jiffies;

	return ci;
}


/* cogtra_free_sta used to release memory when a station leaves the network */
static void
cogtra_free_sta (void *priv, struct ieee80211_sta *sta, void *priv_sta)
{
	struct cogtra_sta_info *ci = priv_sta;

	kfree (ci->t);
	kfree (ci->r);
	kfree (ci);
}


/* cogtra_alloc called once before turning on the wireless interface */
static void *
cogtra_alloc (struct ieee80211_hw *hw, struct dentry *debugfsdir)
{
	struct cogtra_priv *cp;

	cp = kzalloc (sizeof (struct cogtra_priv), GFP_ATOMIC);
	if (!cp)
		return NULL;

	/* Default cogtra values */
	cp->ewma_level = COGTRA_EWMA_LEVEL;

	/* Max number of retries and MRR support */
	cp->max_retry = hw->max_rate_tries > 0 ? hw->max_rate_tries : 7;
	cp->has_mrr = hw->max_rates >= 4 ? true : false;

	cp->hw = hw;
	return cp;
}


/* cogtra_free called once before turning off the wireless interface */
static void
cogtra_free (void *priv)
{
	kfree (priv);
}

struct rate_control_ops mac80211_cogtra = {
	.name = "cogtra",
	.tx_status = cogtra_tx_status,
	.get_rate = cogtra_get_rate,
	.rate_init = cogtra_rate_init,
	.alloc = cogtra_alloc,
	.free = cogtra_free,
	.alloc_sta = cogtra_alloc_sta,
	.free_sta = cogtra_free_sta,
#ifdef CONFIG_MAC80211_DEBUGFS
	.add_sta_debugfs = cogtra_add_sta_debugfs,
	.remove_sta_debugfs = cogtra_remove_sta_debugfs,
#endif
};

int __init
rc80211_cogtra_init(void)
{
	printk ("LJC CogTRA algorithm.\n");
	return ieee80211_rate_control_register (&mac80211_cogtra);
}

void
rc80211_cogtra_exit(void)
{
	ieee80211_rate_control_unregister (&mac80211_cogtra);
}

