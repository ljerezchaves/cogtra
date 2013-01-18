/*
 * Copyright (C) 2011 Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
 * Based on rc80211_cora_debugfs.c:
 *   Copyright (C) 2010 Tiago Chedraoui Silva <tsilva@lrc.ic.unicamp.br>
 *
 * Based on rc80211_minstrel_debugfs.c:
 *   Copyright (C) 2008 Felix Fietkau <nbd@openwrt.org>
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
#include "rc80211_arf.h"

int
arf_stats_open (struct inode *inode, struct file *file)
{
	struct arf_sta_info *ci = inode->i_private;
	struct arf_rate *cr = &ci->r[ci->current_rate_ndx];
	struct arf_debugfs_info *cs;
	char *p;

	cs = kmalloc (sizeof (*cs) + 4096, GFP_KERNEL);
	if (!cs)
		return -ENOMEM;

	file->private_data = cs;
	p = cs->buf;

	p += sprintf (p,
            "Auto Rate Fallback (ARF):\n"
			"   Current rate: %3u%s\n"
			"   Consec success: %2u\n"
			"   Consec failures: %2u\n"
			"   Success threshold: %2u\n"
			"   Timer: %2u\n"
			"   Timeout: %2u\n"
			"   Recovery mode: %s\n\n"
		    "   Number of rates: %2u\n"
			"   Minimum timeout: %2u\n"
			"   Minimum success threshold: %2u\n"
			"   Maximum success threshold: %2u\n"
			"   Success increase factor: %2u\n"
			"   Timeout increase factor: %2u\n",
		    cr->bitrate / 2, (cr->bitrate & 1 ? ".5" : "  "),
			ci->success,
			ci->failures,
			ci->success_thrs,
			ci->timer,
			ci->timeout,
            ((ci->recovery) ? "true" : "false"),
		    
            ci->n_rates,
			ci->min_timeout,
			ci->min_succ_thrs,
			ci->max_succ_thrs,
			ci->success_k,
			ci->timeout_k
	);

	cs->len = p - cs->buf;
	return 0;
}

/* Information for rc_history file */
int
arf_hist_open (struct inode *inode, struct file *file)
{
	struct arf_sta_info *ci = inode->i_private;
	struct arf_debugfs_info *ch;
	unsigned int i;
	char *p;

	ch = kmalloc (sizeof (*ch) 
			    + sizeof (struct arf_hist_info) * ARF_DEBUGFS_HIST_SIZE 
				+ 1024, GFP_KERNEL);
	if (!ch)
		return -ENOMEM;

	file->private_data = ch;
	p = ch->buf;

	/* Table header */
	p += sprintf (p, "Auto Rate Fallback (ARF)\n");
	p += sprintf (p, "History Information Table\n"); 
	p += sprintf (p, "Rate adaptations: %u (max of %u)\n\n", ci->dbg_idx, ARF_DEBUGFS_HIST_SIZE);
	p += sprintf (p, "Idx | Start time |  Event  | Rate -> Rate | Timer | Succ | Fail | Recover | succ_ths | timeout \n");

	/* Table lines */
	for (i = 0; i < ci->dbg_idx && i < ARF_DEBUGFS_HIST_SIZE; i++) {
		struct arf_hist_info	*t = &ci->hi[i];

		p += sprintf (p, "%3u | %10d | %s | %2u%s%s%2u%s | %5d | %4d | %4d | %s | %8d | %7d \n", 
				i,
				t->start_ms,
				(t->event == ARF_LOG_SUCCESS ? "Success" : (t->event == ARF_LOG_FAILURE ? "Failure": (t->event == ARF_LOG_RECOVER ? "Recover" : "Timeout"))),				
				t->last / 2, (t->last & 1 ? ".5" : "  "), " -> ",
				t->rate / 2, (t->rate & 1 ? ".5" : "  "),
				t->timer,
				t->success,
				t->failures,
				((t->recovery) ? "  true " : " false "),
				t->success_thrs,
				t->timeout				
			);
	}

	if (i == ARF_DEBUGFS_HIST_SIZE)
		p += sprintf (p, "\n   *** Table Full... ***\n");

	ch->len = p - ch->buf;
	return 0;
}



ssize_t
arf_stats_read (struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	struct arf_debugfs_info *cs;

	cs = file->private_data;
	return simple_read_from_buffer (buf, len, ppos, cs->buf, cs->len);
}

ssize_t
arf_hist_read (struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	struct arf_debugfs_info *cs;

	cs = file->private_data;
	return simple_read_from_buffer (buf, len, ppos, cs->buf, cs->len);
}

int
arf_stats_release (struct inode *inode, struct file *file)
{
	kfree (file->private_data);
	return 0;
}

int
arf_hist_release (struct inode *inode, struct file *file)
{
	kfree (file->private_data);
	return 0;
}

static const struct file_operations arf_stat_fops = {
	.owner = THIS_MODULE,
	.open = arf_stats_open,
	.read = arf_stats_read,
	.release = arf_stats_release,
};

static const struct file_operations arf_hist_fops = {
	.owner = THIS_MODULE,
	.open = arf_hist_open,
	.read = arf_hist_read,
	.release = arf_hist_release,
};

void
arf_add_sta_debugfs (void *priv, void *priv_sta, struct dentry *dir)
{
	struct arf_sta_info *ci = priv_sta;

	ci->dbg_stats = debugfs_create_file ("rc_stats", S_IRUGO, dir,
			ci, &arf_stat_fops); 
	ci->dbg_hist = debugfs_create_file ("rc_history", S_IRUGO, dir,
			ci, &arf_hist_fops); 
}

void
arf_remove_sta_debugfs(void *priv, void *priv_sta)
{
	struct arf_sta_info *ci = priv_sta;
	
	debugfs_remove (ci->dbg_stats);
	debugfs_remove (ci->dbg_hist);
}
