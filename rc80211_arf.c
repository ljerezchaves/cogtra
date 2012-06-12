/*
 * Copyright (C) 2011 Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Based on rc80211_cora.c:
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
#include "rc80211_arf.h"

/* Converting mac80211 rate index into local array index */
static inline int
rix_to_ndx (struct arf_sta_info *ci, int rix)
{
	int i;
	for (i = ci->n_rates - 1; i >= 0; i--)
		if (ci->r[i].rix == rix)
			break;
	return i;
}


/* Sort rates by bitrates. Is called once by arf_rate_init */
static void
sort_bitrates (struct arf_sta_info *ci, int n_rates)
{
	int i, j;
	struct arf_rate cr_key;

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


static void arf_success (struct arf_sta_info *ci)
{
	ci->timer++;
	ci->success++;
	ci->failures = 0;
	ci->recovery = false;
	if (((ci->success >= ci->success_thrs) || (ci->timer >= ci->timeout)) &&
			(ci->current_rate_ndx < (ci->n_rates - 1)))
	{
		ci->current_rate_ndx++;
		ci->timer = 0;
		ci->success = 0;
		ci->recovery = true;
	}
}


static void arf_failure (struct arf_sta_info *ci)
{
	ci->timer++;
	ci->failures++;
	ci->success = 0;
	if (((ci->recovery == true) || (ci->failures >= 2)) &&
            (ci->current_rate_ndx != 0))
	{
		ci->current_rate_ndx--;
		ci->timer = 0;
		ci->failures = 0;
	}
}


/* arf_tx_status is called just after frame tx and it is used to update
 * statistics information for the used rate */
static void
arf_tx_status (void *priv, struct ieee80211_supported_band *sband, 
		struct ieee80211_sta *sta, void *priv_sta, struct sk_buff *skb)
{
	struct arf_sta_info *ci = priv_sta;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_tx_rate *ar = info->status.rates;
	int i = 0;
   	int success = 0;
 
	if (ar[i].idx < 0)
		return;

	/* Checking for a success in frame transmission */
	success = !!(info->flags & IEEE80211_TX_STAT_ACK);
	if (success)
		arf_success (ci);
	else 
		arf_failure (ci);
}


/* arf_get_rate is called just before each frame tx and sets the appropriate
 * data rate to be used */
static void
arf_get_rate (void *priv, struct ieee80211_sta *sta, void *priv_sta,
		struct ieee80211_tx_rate_control *txrc)
{
	struct sk_buff *skb = txrc->skb;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct arf_sta_info *ci = priv_sta;
	struct arf_priv *cp = priv;
	struct ieee80211_tx_rate *ar = info->control.rates;

	/* Check for management or control packet, which should be transmitted
	 * unsing lower rate */
	if (rate_control_send_low (sta, priv_sta, txrc))
		return;

	/* Setting up tx rate information. */
	ar[0].idx = ci->r[ci->current_rate_ndx].rix;
	ar[0].count = cp->max_retry;
	ar[1].idx = -1;
	ar[1].count = 0;
}


/* arf_rate_init is called after arf_alloc_sta to check and populate
 * information for supported rates */
static void
arf_rate_init (void *priv, struct ieee80211_supported_band *sband,
		struct ieee80211_sta *sta, void *priv_sta)
{
	struct arf_sta_info *ci = priv_sta;
	unsigned int i, n = 0;

	/* Populating information for each supported rate */
	for (i = 0; i < sband->n_bitrates; i++) {
		struct arf_rate *cr = &ci->r[n];

		/* Check rate support */
		if (!rate_supported (sta, sband->band, i))
			continue;
		n++;

		/* Set index, bitrate and tx duration */
		cr->rix = i;
		cr->bitrate = sband->bitrates[i].bitrate / 5;
	}

	/* Sort rates based on bitrate */
	sort_bitrates (ci, n);

	/* Mark unsupported rates with rix = -1 */
	for (i = n; i < sband->n_bitrates; i++) {
		struct arf_rate *cr = &ci->r[i];
		cr->rix = -1;
	}
	
	ci->n_rates = n;
}


/* arf_alloc_sta is called every time a new station joins the networks */
static void *
arf_alloc_sta (void *priv, struct ieee80211_sta *sta, gfp_t gfp)
{
	struct ieee80211_supported_band *sband;
	struct arf_sta_info *ci;
	struct arf_priv *cp = priv;
	struct ieee80211_hw *hw = cp->hw;
	int max_rates = 0;
	int i;

	ci = kzalloc (sizeof (struct arf_sta_info), gfp);
	if (!ci)
		return NULL;

	/* Get the maximum number of rates based on available bands */
	for (i = 0; i < IEEE80211_NUM_BANDS; i++) {
		sband = hw->wiphy->bands[i];
		if (sband && sband->n_bitrates > max_rates)
			max_rates = sband->n_bitrates;
	}

	/* Memory allocation for rates that can be used by this station */
	ci->r = kzalloc (sizeof (struct arf_rate) * (max_rates), gfp);
	if (!ci->r) {
		kfree(ci);
		return NULL;
	}

	ci->current_rate_ndx = 0;
	ci->success = 0;
	ci->failures = 0;
	ci->timer = 0;
	ci->recovery = false;
	ci->success_thrs = ARF_SUCC_THRS;
	ci->timeout = ARF_TIMEOUT;

	return ci;
}


/* arf_free_sta used to release memory when a station leaves the network */
static void
arf_free_sta (void *priv, struct ieee80211_sta *sta, void *priv_sta)
{
	struct arf_sta_info *ci = priv_sta;

	kfree (ci->r);
	kfree (ci);
}


/* arf_alloc called once before turning on the wireless interface */
static void *
arf_alloc (struct ieee80211_hw *hw, struct dentry *debugfsdir)
{
	struct arf_priv *cp;

	cp = kzalloc (sizeof (struct arf_priv), GFP_ATOMIC);
	if (!cp)
		return NULL;
	
	/* Max number of retries */
	if (hw->max_rate_tries > 0)
		cp->max_retry = hw->max_rate_tries;
	else
		cp->max_retry = 7;

	cp->hw = hw;
	return cp;
}


/* arf_free called once before turning off the wireless interface */
static void
arf_free (void *priv)
{
	kfree (priv);
}

struct rate_control_ops mac80211_arf = {
	.name = "arf",
	.tx_status = arf_tx_status,
	.get_rate = arf_get_rate,
	.rate_init = arf_rate_init,
	.alloc = arf_alloc,
	.free = arf_free,
	.alloc_sta = arf_alloc_sta,
	.free_sta = arf_free_sta,
#ifdef CONFIG_MAC80211_DEBUGFS
	.add_sta_debugfs = arf_add_sta_debugfs,
	.remove_sta_debugfs = arf_remove_sta_debugfs,
#endif
};

int __init
rc80211_arf_init(void)
{
	printk ("LJC ARF algorithm.\n");
	return ieee80211_rate_control_register(&mac80211_arf);
}

void
rc80211_arf_exit(void)
{
	ieee80211_rate_control_unregister(&mac80211_arf);
}

