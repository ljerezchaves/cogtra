/*
 * Copyright (C) 2012 Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 * 
 * Based on rc80211_minstrel.h:
 * Copyright (C) 2008 Felix Fietkau <nbd@openwrt.org>
 * Copyright (C) 2010 Tiago Chedraoui Silva <tsilva@lrc.ic.unicamp.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __RC_COGTRA_HT_H
#define __RC_COGTRA_HT_H

/*
 * The number of streams can be changed to 2 to reduce code
 * size and memory footprint.
 */
#define MINSTREL_MAX_STREAMS	3
#define MINSTREL_STREAM_GROUPS	4
#define MCS_GROUP_RATES	8
/* scaled fraction values */
#define MINSTREL_SCALE	16
#define MINSTREL_FRAC(val, div) (((val) << MINSTREL_SCALE) / div)
#define MINSTREL_TRUNC(val) ((val) >> MINSTREL_SCALE)

/* Cogtra_HT custom code optimization */
#define COGTRA_HT_MAX_STDEV			150
#define COGTRA_HT_MIN_STDEV			40
#define COGTRA_HT_EWMA_LEVEL			30
#define COGTRA_HT_UPDATE_INTERVAL	    150
#define COGTRA_HT_RECOVERY_INTERVAL	20

extern struct chain_table;
extern struct cogtra_rate;

struct mcs_group {
	u32 flags;
	int streams;
	unsigned int duration[MCS_GROUP_RATES];
};

extern const struct mcs_group minstrel_mcs_groups[];

struct minstrel_rate_stats {
	/* Devilery probability */
	u32 cur_prob;					// prob for last interval (parts per thousand)
	u32 avg_prob;					// avg prob (using ewma -- parts per thousand)

	/* Throughput information */
	u32 cur_tp;						// thp for the last interval
	u32 avg_tp;						// avg thp (using ewma)

	/* Transmission times for this rate */
	unsigned int perfect_tx_time;	// tx time for 1200-byte data packet
	unsigned int ack_time;			// tx time for ack packet

	/* Personalized retry count to avoid stall in the same packet */
	unsigned int retry_count;

	/* Tx success and attempts counters */
	u32 success;					// during last interval
	u32 attempts;					// during last interval
	u64 succ_hist;					// since ever 
	u64 att_hist;					// since ever
	u32 last_attempts;				// before last cogtra_update_stats
	u32 last_success;				// before last cogtra_update_stats
	
	/* Number of times this rate was used by cogtra */
	u32 times_called;


	/* packet delivery probabilities */
	//unsigned int probability;

	/* maximum retry counts */
	//unsigned int retry_count_rtscts;

	//bool retry_updated;
	//u8 sample_skipped;
};

struct minstrel_mcs_group_data {
	/* bitfield of supported MCS rates of this group */
	u8 supported;

	/* selected primary rates */
	unsigned int cur_stdev;
	
	unsigned int random_rate_gix; //0 a 7
	unsigned int max_tp_rate_gix;
	unsigned int max_prob_rate_gix;

	/* MCS rate statistics */
	struct minstrel_rate_stats rates[MCS_GROUP_RATES];
};

struct cogtra_ht_sta{

	struct ieee80211_tx_rate tx_rates[4];

	unsigned int overhead;
	unsigned int overhead_rtscts;
	unsigned int avg_ampdu_len;
	unsigned int ampdu_packets;

	//unsigned int cur_stdev;			// current normal stdev
	unsigned int random_rate_mcs;		// random mcs index (will be used in the next interval)  //0 a 
	unsigned int max_tp_rate_mcs;		// index mcs with highest thp (current normal mean)
	unsigned int max_prob_rate_mcs;		// index mcs with highest probability
	
	unsigned int n_groups;				// number of actives MCS GROUPS
	unsigned int n_rates;				// number o supported rates
	
	unsigned long update_counter;		// last update time (time based) or pkt counter (pkt based)
    unsigned int update_interval; 		// time (or pkts) between cogtra_ht_update_stats
	unsigned long up_stats_counter;		// update stats counter

	u32 tx_flags;
	struct minstrel_mcs_group_data groups[MINSTREL_MAX_STREAMS * MINSTREL_STREAM_GROUPS];
#ifdef CONFIG_MAC80211_DEBUGFS
	struct dentry *dbg_stats;		// debug file pointer 
#endif
};

struct cogtra_ht_sta_priv {
	union {
		struct cogtra_ht_sta ht;
		struct cogtra_sta_info legacy;
	};
#ifdef CONFIG_MAC80211_DEBUGFS
	struct dentry *dbg_stats;		// debug file pointer 
#endif
	struct cogtra_rate *r;	
	struct chain_table *t;
	bool is_ht;

};

/* Common functions */
extern struct rate_control_ops mac80211_cogtra_ht;
void cogtra_ht_add_sta_debugfs (void *priv, void *priv_sta, struct dentry *dir);
void cogtra_ht_remove_sta_debugfs (void *priv, void *priv_sta);


int cogtra_ht_stats_open (struct inode *inode, struct file *file);

#endif

