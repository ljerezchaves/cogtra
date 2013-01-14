/*
 * Copyright (C) 2012 Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Based on rc80211_minstrel_debugfs.c:
 *   Copyright (C) 2008 Felix Fietkau <nbd@openwrt.org>
 *   Copyright (C) 2010 Tiaho Chedraiou Silva <tsilva@lrc.ic.unicamp.br>
 *
 * Based on minstrel.c:
 *   Copyright (C) 2005-2007 Derek Smithies <derek@indranet.co.nz>
 *   Sponsored by Indranet Technologies Ltd
 *
 * Based on sample.c:
 *   Copyright (c) 2005 John Bicket
 *   All rights reserved.
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
#include <linux/ieee80211.h>
#include <linux/slab.h>
#include <net/mac80211.h>
#include "rc80211_cogtra.h"

/* Information for rc_stats file */
int
cogtra_stats_open (struct inode *inode, struct file *file)
{
	struct cogtra_sta_info *ci = inode->i_private;
	struct cogtra_debugfs_info *cs;
	unsigned int i, avg_tp, avg_prob, cur_tp, cur_prob;
	char *p;

	cs = kmalloc (sizeof (*cs) + 4096, GFP_KERNEL);
	if (!cs)
		return -ENOMEM;

	file->private_data = cs;
	p = cs->buf;

	/* Table header */
	p += sprintf(p, "\n Rate Table:\n");
	p += sprintf(p, "    | rate | avg_thp | avg_pro | cur_thp | cur_pro | "
			"succ ( atte ) | success | attempts | #used \n");

	/* Table lines */
	for (i = 0; i < ci->n_rates; i++) {
		struct cogtra_rate *cr = &ci->r[i];

		/* Print T for the rate with highest throughput (the mean of normal
		 * curve), P for the rate with hisgest delivery probability and print *
		 * for the rate been used now */
		*(p++) = (i == ci->random_rate_ndx)		? '*' : ' ';
		*(p++) = (i == ci->max_tp_rate_ndx) 	? 'T' : ' ';   
		*(p++) = (i == ci->max_prob_rate_ndx) 	? 'P' : ' ';   

		p += sprintf(p, " |%3u%s ", cr->bitrate / 2,
				(cr->bitrate & 1 ? ".5" : "  "));

		/* Converting the internal thp and prob format */
		avg_tp = cr->avg_tp / ((1800 << 10) / 96);
		cur_tp = cr->cur_tp / ((1800 << 10) / 96);
		avg_prob = cr->avg_prob;
		cur_prob = cr->cur_prob;

		p += sprintf (
				p, 
				"| %5u.%1u | %7u | %5u.%1u | %7u "
				"| %4u ( %4u ) | %7llu | %8llu | %5u\n",
				avg_tp / 10, avg_tp % 10,
				avg_prob / 18,
				cur_tp / 10, cur_tp % 10,
				cur_prob / 18,
				cr->last_success, cr->last_attempts,
				(unsigned long long) cr->succ_hist,
				(unsigned long long) cr->att_hist,
				cr->times_called
			);
	}

	/* Chain table  */
	p += sprintf(p, "\n MRR Chain Table:\n");
	p += sprintf(p, " type |  rate | count | success | attempts \n");

	for (i = 0; i < 4; i++) {
		struct chain_table *ct = &ci->t[i];

		p += sprintf (
			p, 
			" %s | %3u%s | %5u | %7llu | %8llu\n",
			ct->type == 0 ? "rand": ct->type == 1 ? "best" : ct->type == 2 ? "prob" : "lowr", 
			ct->bitrate / 2, (ct->bitrate & 1 ? ".5" : "  "),
			ct->count,
			(unsigned long long) ct->suc,
			(unsigned long long) ct->att
		);
	}

	/* Table footer */
	p += sprintf(p, "\n Cognitive Transmission Rate Adaptation (CogTRA):\n"
			"   Number of rates:      %u\n"
			"   Current pkt interval: %u\n"
			"   Current Normal Mean:  %u\n"
		   	"   Current Normal Stdev: %u.%2u\n",
			ci->n_rates,
			ci->update_interval,
			ci->max_tp_rate_ndx,
			ci->cur_stdev / 100, ci->cur_stdev % 100
		);

	cs->len = p - cs->buf;
	return 0;
}
	
/* Information for rc_history file */
int
cogtra_hist_open (struct inode *inode, struct file *file)
{
	struct cogtra_sta_info *ci = inode->i_private;
	struct cogtra_debugfs_info *ch;
	unsigned int i;
	unsigned int time = 0;
	char *p;

	ch = kmalloc (sizeof (*ch) 
			    + sizeof (struct cogtra_hist_info) * COGTRA_DEBUGFS_HIST_SIZE 
				+ 1024, GFP_KERNEL);
	if (!ch)
		return -ENOMEM;

	file->private_data = ch;
	p = ch->buf;

	/* Table header */
	p += sprintf(p, "Cognitive Transmission Rate Adaptation (CogTRA)\n");
	p += sprintf(p, "History Information Table\n"); 
	p += sprintf(p, "Rate adaptations: %u (max of %u)\n\n", ci->dbg_idx, COGTRA_DEBUGFS_HIST_SIZE);
	p += sprintf(p, "Idx | Start time | Duration | AvgSig | Random | BestThp | BestPro | Stdev | Pktint | MRR usage\n");

	/* Table lines */
	for (i = 0; i < ci->dbg_idx && i < COGTRA_DEBUGFS_HIST_SIZE; i++) {
		struct cogtra_hist_info	*t = &ci->hi[i];

		time += t->msec;	
		p += sprintf(p, "%3u | %10d | %8d | %6d | %4u%s | %5u%s | %5u%s | %2u.%2u | %6u | %3d,%3d,%3d,%3d,%3d\n", 
				i,
				t->start_ms,
				t->duration_ms,
				t->avg_signal,
				t->rand_rate / 2, (t->rand_rate & 1 ? ".5" : "  "),
				t->best_rate / 2, (t->best_rate & 1 ? ".5" : "  "),
				t->prob_rate / 2, (t->prob_rate & 1 ? ".5" : "  "),
				t->currstdev / 100, t->currstdev % 100,
				t->pktinterval,
				t->rand_pct, t->best_pct, t->prob_pct, t->lowr_pct, 
				(100 - t->rand_pct - t->best_pct - t->prob_pct - t->lowr_pct)
			);
	}

	ch->len = p - ch->buf;
	return 0;
}

ssize_t
cogtra_stats_read (struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	struct cogtra_debugfs_info *cs;

	cs = file->private_data;
	return simple_read_from_buffer (buf, len, ppos, cs->buf, cs->len);
}

ssize_t
cogtra_hist_read (struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	struct cogtra_debugfs_info *ch;

	ch = file->private_data;
	return simple_read_from_buffer (buf, len, ppos, ch->buf, ch->len);
}

int
cogtra_stats_release (struct inode *inode, struct file *file)
{
	kfree (file->private_data);
	return 0;
}

int
cogtra_hist_release (struct inode *inode, struct file *file)
{
	kfree (file->private_data);
	return 0;
}

static const struct file_operations cogtra_stat_fops = {
	.owner = THIS_MODULE,
	.open = cogtra_stats_open,
	.read = cogtra_stats_read,
	.release = cogtra_stats_release,
};

static const struct file_operations cogtra_hist_fops = {
	.owner = THIS_MODULE,
	.open = cogtra_hist_open,
	.read = cogtra_hist_read,
	.release = cogtra_hist_release,
};

void
cogtra_add_sta_debugfs (void *priv, void *priv_sta, struct dentry *dir)
{
	struct cogtra_sta_info *ci = priv_sta;

	ci->dbg_stats = debugfs_create_file ("rc_stats", S_IRUGO, dir,
			ci, &cogtra_stat_fops); 
	ci->dbg_hist = debugfs_create_file ("rc_history", S_IRUGO, dir,
			ci, &cogtra_hist_fops);
}

void
cogtra_remove_sta_debugfs(void *priv, void *priv_sta)
{
	struct cogtra_sta_info *ci = priv_sta;
	
	debugfs_remove (ci->dbg_stats);
	debugfs_remove (ci->dbg_hist);
}
