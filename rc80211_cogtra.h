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

#ifndef __RC_COGTRA_H
#define __RC_COGTRA_H

/* Cogtra custom code optimization */
#define COGTRA_MAX_STDEV			150
#define COGTRA_MIN_STDEV			40
#define COGTRA_EWMA_LEVEL			30
#define COGTRA_UPDATE_INTERVAL	    150
#define COGTRA_RECOVERY_INTERVAL	20

struct chain_table {
	unsigned int type;
	unsigned int count;
	int bitrate;
	int rix;
	u32 att;
	u32 suc;
};


/* cogtra_rate is allocated once for each available rate at each cogtra_sta_info.
 * Information in this struct is private to this rate at this station */ 
struct cogtra_rate {
	int bitrate;
	int rix;

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
};



/* cogtra_sta_info is allocated once per station. Information in this strcut
 * allows independed rate adaptation for each station */
struct cogtra_sta_info {
	unsigned int cur_stdev;			// current normal stdev
	unsigned int max_tp_rate_ndx;	// index of rate with highest thp (current normal mean)
	unsigned int max_prob_rate_ndx;	// index of rate with highest probability
	unsigned int random_rate_ndx;	// random rate index (will be used in the next interval) 
	unsigned int lowest_rix;		// lowest rate index 
	unsigned int n_rates;			// number o supported rates 
	unsigned long update_counter;	// last update time (time based) or pkt counter (pkt based)
    unsigned int update_interval; 	// time (or pkts) between cogtra_update_stats
	unsigned long up_stats_counter;	// update stats counter
	
	struct cogtra_rate *r;			// rate pointer for each station
	struct chain_table *t;			// chain table pointer for mrr

#ifdef CONFIG_MAC80211_DEBUGFS
	struct dentry *dbg_stats;		// debug file pointer 
#endif
};


/* cogtra_priv is allocated once. Information in this struct is shared among all
 * cogtra_sta_info. */
struct cogtra_priv {
	struct ieee80211_hw *hw;	  	// hardware properties 
	bool has_mrr;				  	// mrr support
	unsigned int cw_min;		  	// congestion window base
	unsigned int cw_max;		  	// congestion window roof
	unsigned int max_retry;		  	// default max number o retries before frame discard
	unsigned int ewma_level;	  	// ewma alpha for ammortize throughput.
	unsigned int segment_time;	  	// maximum time allowed at the same mrr segment
};


/* Common functions */
extern struct rate_control_ops mac80211_cogtra;
void cogtra_add_sta_debugfs (void *priv, void *priv_sta, struct dentry *dir);
void cogtra_remove_sta_debugfs (void *priv, void *priv_sta);

/* Debugfs */
struct cogtra_debugfs_info {
	size_t len;
	char buf[];
};

int cogtra_stats_open (struct inode *inode, struct file *file);
ssize_t cogtra_stats_read (struct file *file, char __user *buf, size_t len, 
		loff_t *ppos);
int cogtra_stats_release (struct inode *inode, struct file *file);

#endif

