/*
 *  byt_common.h - Common driver for codecs on Baytrail Platfrom
 *
 *  Copyright (C) 2013 Intel Corp
 *  Author: Zhang Ning <ning.a.zhang@intel.com>
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#ifndef _BYT_COMMON_H_
#define _BYT_COMMON_H_

#define BYT_PLAT_CLK_3_HZ	25000000
/* Data struct for byt_common */
struct byt_comms_mc_private {
	bool ssp_bt_sco_master_mode;
	bool ssp_modem_master_mode;
};

struct byt_common_private {
	struct byt_machine_ops *ops;
	struct byt_comms_mc_private comms_ctl;
	struct snd_soc_jack jack;
/* TODO */
};
/* functions for Machine driver */
void byt_jack_report(int status);
int snd_byt_init(struct snd_soc_pcm_runtime *runtime);

/* machine driver to implement */
struct byt_machine_ops {
	void (*card_init)(struct snd_soc_card *card);
	int  (*byt_init)(struct snd_soc_pcm_runtime *runtime);
	int  (*jack_detection)(void);
};
#ifdef CONFIG_SND_BYT_RT5645
extern struct byt_machine_ops byt_bl_alc5645_ops;
#endif
#ifdef CONFIG_SND_BYT_RT5651
extern struct byt_machine_ops byt_cr_rt5651_ops;
#endif
#ifdef CONFIG_SND_BYT_ES8396
extern struct byt_machine_ops byt_bl_es8396_ops;
#endif


/* data for drivers */
extern unsigned int rates_8000_16000[];

extern struct snd_pcm_hw_constraint_list constraints_8000_16000;

extern unsigned int rates_48000[];

extern struct snd_pcm_hw_constraint_list constraints_48000;

extern unsigned int rates_16000[];
extern struct snd_pcm_hw_constraint_list constraints_16000;

#endif
