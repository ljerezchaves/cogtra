/*
 * Copyright (C) 2011 Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 * 
 * Based on rc80211_cora.h:
 * Copyright (C) 2010 Tiago Chedraoui Silva <tsilva@lrc.ic.unicamp.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __RC_AARF_H
#define __RC_AARF_H

#define AARF_MIN_SUCC_THRS		10
#define AARF_MAX_SUCC_THRS		50
#define AARF_TIMEOUT		    15
#define AARF_TIMER_K			2
#define AARF_SUCCESS_K			2

#define AARF_DEBUGFS_HIST_SIZE	1000U

#define AARF_LOG_SUCCESS	1
#define AARF_LOG_FAILURE	2
#define AARF_LOG_RECOVER	3



/* aarf_rate is allocated once for each available rate at each aarf_sta_info.
 * Information in this struct is private to this rate at this station */ 
struct aarf_rate {
	int bitrate;
	int rix;
};


/* aarf_sta_info is allocated once per station and holds information that allows
 * independed rate adaptation for each station */
struct aarf_sta_info {
	unsigned int n_rates;			// number o supported rates 
	unsigned int current_rate_ndx;	// index of current rate
	
	u32 timer;						// packet timer
	u32 success;					// consecutive tx success
	u32 failures;					// consecutive tx failures
	bool recovery;					// recovery mode
	unsigned int success_thrs;		// success threshold
	unsigned int timeout;			// packet timer timeout
	unsigned int min_timeout;       // minimum packet timer timeout
	unsigned int min_succ_thrs;     // minimum success threshold
	unsigned int max_succ_thrs;     // maximun success threshold
	unsigned int success_k;         // success threshold increase factor
	unsigned int timeout_k;         // timeout increase factor
	unsigned int first_time;		//jiffies for the first rate adaptation

	/* Rate pointer for each station (created in aarf_alloc_sta) */
	struct aarf_rate *r;

#ifdef CONFIG_MAC80211_DEBUGFS
	struct aarf_hist_info *hi;		// history table (for the first AARF_DEBUGFS_HIST_SIZE rate adaptations)
	unsigned int dbg_idx;			// history table index
	struct dentry *dbg_stats;		// debug rc_stats file pointer
	struct dentry *dbg_hist;		// debug rc_history file pointer
#endif
};


/* aarf_priv is allocated once and holds information shared among all
 * aarf_sta_info. */
struct aarf_priv {
	struct ieee80211_hw *hw; 		// hardware properties
	unsigned int max_retry;		  	// default max number o retries before frame discard
};


/* Common functions */
extern struct rate_control_ops mac80211_aarf;
void aarf_add_sta_debugfs (void *priv, void *priv_sta, struct dentry *dir);
void aarf_remove_sta_debugfs (void *priv, void *priv_sta);

/* Debugfs */
struct aarf_debugfs_info {
	size_t len;
	char buf[];
};

/* Debugfs history table entry */
struct aarf_hist_info {
	int start_ms;
	int rate;
	int event;
	int timer;
	int success;
	int failures;
	bool recovery;
	int success_thrs;
	int timeout;
};


int aarf_stats_open (struct inode *inode, struct file *file);
ssize_t aarf_stats_read (struct file *file, char __user *buf, size_t len, 
		loff_t *ppos);
int aarf_stats_release (struct inode *inode, struct file *file);

#endif

