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

struct cogtra_ht_sta{
	unsigned int cur_stdev;			// current normal stdev
	unsigned int max_tp_rate_ndx;		// index of rate with highest thp (current normal mean)
	unsigned int max_prob_rate_ndx;		// index of rate with highest probability
	unsigned int random_rate_ndx;		// random rate index (will be used in the next interval) 
	unsigned int lowest_rix;		// lowest rate index 
	unsigned int n_rates;			// number o supported rates 
	unsigned long update_counter;		// last update time (time based) or pkt counter (pkt based)
    	unsigned int update_interval; 		// time (or pkts) between cogtra_ht_update_stats
	unsigned long up_stats_counter;		// update stats counter
	
	struct cogtra_rate *r;				// rate pointer for each station
	struct chain_table *t;				// chain table pointer for mrr

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

