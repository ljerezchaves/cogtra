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

#ifndef __RC_CORA_H
#define __RC_CORA_H

/* Cora custom code optimization */
#define CORA_STDEV			    80
#define CORA_EWMA_LEVEL			30
#define CORA_UPDATE_INTERVAL	100

/* cora_rate is allocated once for each available rate at each cora_sta_info.
 * Information in this struct is private to this rate at this station */ 
struct cora_rate {
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
	u32 last_attempts;				// before last cora_update_stats
	u32 last_success;				// before last cora_update_stats
	
	/* Number of times this rate was used by cora */
	u32 times_called;
};



/* cora_sta_info is allocated once per station. Information in this strcut
 * allows independed rate adaptation for each station */
struct cora_sta_info {
	unsigned int cur_stdev;			// current normal stdev
	unsigned int max_tp_rate_ndx;	// index of rate with highest thp (current normal mean)
	unsigned int random_rate_ndx;	// random rate index (will be used in the next interval) 
	unsigned int lowest_rix;		// lowest rate index 
	unsigned int n_rates;			// number o supported rates 
	unsigned long update_counter;	// last update time
    unsigned int update_interval; 	// time between cora_update_stats
	unsigned long up_stats_counter;	// update stats counter
	
	struct cora_rate *r;			// rate pointer for each station

#ifdef CONFIG_MAC80211_DEBUGFS
	struct dentry *dbg_stats;		// debug file pointer 
#endif
};


/* cora_priv is allocated once. Information in this struct is shared among all
 * cora_sta_info. */
struct cora_priv {
	struct ieee80211_hw *hw;	  	// hardware properties 
	unsigned int cw_min;		  	// congestion window base
	unsigned int cw_max;		  	// congestion window roof
	unsigned int max_retry;		  	// default max number o retries before frame discard
	unsigned int ewma_level;	  	// ewma alpha for ammortize throughput.
	unsigned int segment_time;	  	// maximum time allowed at the same mrr segment
};


/* Common functions */
extern struct rate_control_ops mac80211_cora;
void cora_add_sta_debugfs (void *priv, void *priv_sta, struct dentry *dir);
void cora_remove_sta_debugfs (void *priv, void *priv_sta);

/* Debugfs */
struct cora_debugfs_info {
	size_t len;
	char buf[];
};

int cora_stats_open (struct inode *inode, struct file *file);
ssize_t cora_stats_read (struct file *file, char __user *buf, size_t len, 
		loff_t *ppos);
int cora_stats_release (struct inode *inode, struct file *file);

#endif

