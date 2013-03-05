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
#include "rc80211_cogtra_ht.h"

int
cogtra_ht_stats_open (struct inode *inode, struct file *file)
{
	struct cogtra_ht_sta_priv *csp = inode->i_private;
	struct cogtra_ht_sta *ci = &csp->ht;
	struct cogtra_debugfs_info *cs;
	unsigned int i, j, avg_tp, avg_prob, cur_tp, cur_prob;
	char *p;
	int ret;
	
	if (!csp->is_ht) {
		inode->i_private = &csp->legacy;
		ret = cogtra_stats_open(inode, file);
		inode->i_private = csp;
		return ret;
	}
	
	cs = kmalloc (sizeof (*cs) + 4096, GFP_KERNEL);
	if (!cs)
		return -ENOMEM;

	file->private_data = cs;
	p = cs->buf;

	/* Table header */
	p += sprintf(p, "\n Rate Table:\n");
	p += sprintf(p, "             MCS  | avg_thp | avg_pro | cur_thp | cur_pro | "
			"succ ( atte ) | success | attempts | #used \n");

	for (i = 0; i < MINSTREL_MAX_STREAMS * MINSTREL_STREAM_GROUPS; i++) {
		char htmode = '2';
		char gimode = 'L';

		if (!ci->groups[i].supported)
			continue;

		if (minstrel_mcs_groups[i].flags & IEEE80211_TX_RC_40_MHZ_WIDTH)
			htmode = '4';
		if (minstrel_mcs_groups[i].flags & IEEE80211_TX_RC_SHORT_GI)
			gimode = 'S';

		for (j = 0; j < MCS_GROUP_RATES; j++) {
			struct minstrel_rate_stats *cr = &ci->groups[i].rates[j];
			int idx = i * MCS_GROUP_RATES + j;

			if (!(ci->groups[i].supported & BIT(j)))
				continue;

			p += sprintf(p, "HT%c0/%cGI ", htmode, gimode);

			*(p++) = (idx == ci->random_rate_mcs)		? '*' : ' ';
			*(p++) = (idx == ci->max_tp_rate_mcs) 	? 'T' : ' ';   
			*(p++) = (idx == ci->max_prob_rate_mcs) 	? 'P' : ' ';  

			p += sprintf(p, " MCS%-2u", (minstrel_mcs_groups[i].streams - 1) *
					MCS_GROUP_RATES + j);
		
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
	}
	

	/* Chain table  */
	p += sprintf(p, "\n MRR Tx_Rate Table:\n");
	p += sprintf(p, " idx \n");

	for (i = 0; i < 4; i++) {
		struct ieee80211_tx_rate *tx = &ci->tx_rates[i];
		p += sprintf (p, " %u\n",tx->idx);
	}

	/* Table footer */
	p += sprintf(p, "\n Cognitive Transmission Rate Adaptation High Throughput(CogTRA_HT):\n"
			"   Number of rates:      %u\n"
			"   Current pkt interval: %u\n"
			"   Current Normal Mean:  %u\n",
		   	//"   Current Normal Stdev: %u.%2u\n",
			ci->n_rates,
			ci->update_interval,
			ci->max_tp_rate_mcs//,
			//ci->cur_stdev / 100, ci->cur_stdev % 100
		);

	cs->len = p - cs->buf;
	return 0;
}

/*
ssize_t
cogtra_ht_stats_read (struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	struct cogtra_ht_debugfs_info *cs;

	cs = file->private_data;
	return simple_read_from_buffer (buf, len, ppos, cs->buf, cs->len);
}
*/

/*
int
cogtra_ht_stats_release (struct inode *inode, struct file *file)
{
	kfree (file->private_data);
	return 0;
}
*/

static const struct file_operations cogtra_ht_stat_fops = {
	.owner = THIS_MODULE,
	.open = cogtra_ht_stats_open,
	.read = cogtra_stats_read,
	.release = cogtra_stats_release,
	//.llseek = no_llseek,

};

void
cogtra_ht_add_sta_debugfs (void *priv, void *priv_sta, struct dentry *dir)
{
	struct cogtra_ht_sta_priv *csp = priv_sta;

	csp->dbg_stats = debugfs_create_file ("rc_stats", S_IRUGO, dir,
			csp, &cogtra_ht_stat_fops); 
}

void
cogtra_ht_remove_sta_debugfs(void *priv, void *priv_sta)
{
	struct cogtra_ht_sta_priv *csp = priv_sta;
	
	debugfs_remove (csp->dbg_stats);
}
