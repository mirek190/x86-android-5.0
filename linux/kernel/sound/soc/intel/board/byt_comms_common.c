/*
 *  byt_comms_common.c - Common driver for codecs on Baytrail Platfrom
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
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include "../ssp/mid_ssp.h"
#include "byt_comms_common.h"
#include "byt_common.h"
#include <asm/platform_byt_audio.h>

struct snd_pcm_hardware byt_comms_common_bt_hw_param = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_DOUBLE |
		SNDRV_PCM_INFO_PAUSE |
		SNDRV_PCM_INFO_RESUME |
		SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_BATCH |
		SNDRV_PCM_INFO_SYNC_START),
	.formats = (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE),
	.rates = (SNDRV_PCM_RATE_8000),
	.rate_min = 8000,
	.rate_max = 8000,
	.channels_min = 1,
	.channels_max = 1,
	.buffer_bytes_max = (320*1024),
	.period_bytes_min = 32,
	.period_bytes_max = (320*1024),
	.periods_min = 2,
	.periods_max = (1024*2),
	.fifo_size = 0,
};

/* Data path functionalities */
struct snd_pcm_hardware byt_comms_common_modem_hw_param = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_DOUBLE |
		SNDRV_PCM_INFO_PAUSE |
		SNDRV_PCM_INFO_RESUME |
		SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_BATCH |
		SNDRV_PCM_INFO_SYNC_START),
	.formats = (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE),
	.rates = (SNDRV_PCM_RATE_48000),
	.rate_min = 48000,
	.rate_max = 48000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = (640*1024),
	.period_bytes_min = 64,
	.period_bytes_max = (640*1024),
	.periods_min = 2,
	.periods_max = (1024*2),
	.fifo_size = 0,
};


static int byt_comms_dai_link_startup(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *str_runtime;

	str_runtime = substream->runtime;

	WARN(!substream->pcm, "BYT Comms Machine: ERROR NULL substream->pcm\n");

	if (!substream->pcm)
		return -EINVAL;

    /* set the runtime hw parameter with local snd_pcm_hardware struct */
	switch (substream->pcm->device) {
	case BYT_COMMS_BT:
		str_runtime->hw = byt_comms_common_bt_hw_param;
	break;

	case BYT_COMMS_MODEM:
		str_runtime->hw = byt_comms_common_modem_hw_param;
	break;
	default:
		pr_err("BYT Comms Machine: bad PCM Device = %d\n",
			substream->pcm->device);
	}
	return snd_pcm_hw_constraint_integer(str_runtime,
					 SNDRV_PCM_HW_PARAM_PERIODS);
}

static int byt_comms_dai_link_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_card *soc_card = rtd->card;
	struct byt_common_private *ctx = snd_soc_card_get_drvdata(soc_card);
	struct byt_comms_mc_private *ctl = &(ctx->comms_ctl);

	int ret = 0;
	unsigned int tx_mask, rx_mask;
	unsigned int nb_slot = 0;
	unsigned int slot_width = 0;
	unsigned int tristate_offset = 0;
	unsigned int device = substream->pcm->device;


	pr_debug("ssp_bt_sco_master_mode %d\n", ctl->ssp_bt_sco_master_mode);
	pr_debug("ssp_modem_master_mode %d\n", ctl->ssp_modem_master_mode);

	switch (device) {
	case BYT_COMMS_BT:
	/*
	 * set cpu DAI configuration
	 * frame_format = PSP_FORMAT
	 * ssp_serial_clk_mode = SSP_CLK_MODE_1
	 * ssp_frmsync_pol_bit = SSP_FRMS_ACTIVE_HIGH
	 */
	ret = snd_soc_dai_set_fmt(cpu_dai,
				  SND_SOC_DAIFMT_I2S |
				  SSP_DAI_SCMODE_1 |
				  SND_SOC_DAIFMT_NB_NF |
				  (ctl->ssp_bt_sco_master_mode ?
				   SND_SOC_DAIFMT_CBM_CFM :
				   SND_SOC_DAIFMT_CBS_CFS));

	if (ret < 0) {
		pr_err("BYT Comms Machine: Set FMT Fails %d\n",
			ret);
		return -EINVAL;
	}

	/*
	 * BT SCO SSP Config
	 * ssp_active_tx_slots_map = 0x01
	 * ssp_active_rx_slots_map = 0x01
	 * frame_rate_divider_control = 1
	 * data_size = 16
	 * tristate = 1
	 * ssp_frmsync_timing_bit = 0
	 * (NEXT_FRMS_ASS_AFTER_END_OF_T4)
	 * ssp_frmsync_timing_bit = 1
	 * (NEXT_FRMS_ASS_WITH_LSB_PREVIOUS_FRM)
	 * ssp_psp_T2 = 1
	 * (Dummy start offset = 1 bit clock period)
	 */
	nb_slot = BYT_SSP_BT_SLOT_NB_SLOT;
	slot_width = BYT_SSP_BT_SLOT_WIDTH;
	tx_mask = BYT_SSP_BT_SLOT_TX_MASK;
	rx_mask = BYT_SSP_BT_SLOT_RX_MASK;

	if (ctl->ssp_bt_sco_master_mode)
		tristate_offset = BIT(TRISTATE_BIT);
	else
		tristate_offset = BIT(FRAME_SYNC_RELATIVE_TIMING_BIT);
	break;

	case BYT_COMMS_MODEM:
	/*
	 * set cpu DAI configuration
	 * frame_format = PSP_FORMAT
	 * ssp_serial_clk_mode = SSP_CLK_MODE_0
	 * ssp_frmsync_pol_bit = SSP_FRMS_ACTIVE_HIGH
	 */
	ret = snd_soc_dai_set_fmt(cpu_dai,
					SND_SOC_DAIFMT_I2S |
					SSP_DAI_SCMODE_0 |
					SND_SOC_DAIFMT_NB_NF |
					(ctl->ssp_modem_master_mode ?
					SND_SOC_DAIFMT_CBM_CFM :
					SND_SOC_DAIFMT_CBS_CFS));
	if (ret < 0) {
		pr_err("BYT Comms Machine:  Set FMT Fails %d\n", ret);
		return -EINVAL;
	}

	/*
	 * Modem Mixing SSP Config
	 * ssp_active_tx_slots_map = 0x01
	 * ssp_active_rx_slots_map = 0x01
	 * frame_rate_divider_control = 1
	 * data_size = 32
	 * Master:
	 *	tristate = 3
	 *	ssp_frmsync_timing_bit = 1, for MASTER
	 *	(NEXT_FRMS_ASS_WITH_LSB_PREVIOUS_FRM)
	 * Slave:
	 *	tristate = 1
	 *	ssp_frmsync_timing_bit = 0, for SLAVE
	 *	(NEXT_FRMS_ASS_AFTER_END_OF_T4)
	 *
	 */
	nb_slot = BYT_SSP_MIXING_SLOT_NB_SLOT;
	slot_width = BYT_SSP_MIXING_SLOT_WIDTH;
	tx_mask = BYT_SSP_MIXING_SLOT_TX_MASK;
	rx_mask = BYT_SSP_MIXING_SLOT_RX_MASK;

	tristate_offset = BIT(TRISTATE_BIT) |
		BIT(FRAME_SYNC_RELATIVE_TIMING_BIT);

	break;
	default:
	pr_err("BYT Comms Machine: bad PCM Device ID = %d\n", device);
	return -EINVAL;
	}

	ret = snd_soc_dai_set_tdm_slot(cpu_dai, tx_mask,
				   rx_mask, nb_slot, slot_width);

	if (ret < 0) {
		pr_err("BYT Comms Machine:  Set TDM Slot Fails %d\n", ret);
		return -EINVAL;
	}

	ret = snd_soc_dai_set_tristate(cpu_dai, tristate_offset);
	if (ret < 0) {
		pr_err("BYT Comms Machine: Set Tristate Fails %d\n", ret);
	return -EINVAL;
	}

	pr_debug("BYT Comms Machine: slot_width = %d\n",
	     slot_width);
	pr_debug("BYT Comms Machine: tx_mask = %d\n",
	     tx_mask);
	pr_debug("BYT Comms Machine: rx_mask = %d\n",
	     rx_mask);
	pr_debug("BYT Comms Machine: tristate_offset = %d\n",
	     tristate_offset);

	return 0;
}

static int byt_comms_dai_link_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct byt_common_private *ctx = snd_soc_card_get_drvdata(rtd->card);
	struct byt_comms_mc_private *ctl = &(ctx->comms_ctl);

	unsigned int device = substream->pcm->device;

	pr_debug("%s substream->runtime->rate %d\n",
		__func__,
		substream->runtime->rate);

	/* select clock source (if master) */
	/* BT SCO: CPU DAI is master */
	/* FM: CPU DAI is master */
	/* BT_VOIP: CPU DAI is master */
	if ((device == BYT_COMMS_BT && ctl->ssp_bt_sco_master_mode) ||
	    (device == BYT_COMMS_MODEM && ctl->ssp_modem_master_mode)) {

		snd_soc_dai_set_sysclk(cpu_dai, SSP_CLK_ONCHIP,
				substream->runtime->rate, 0);

	}

	return 0;
}

struct snd_soc_ops byt_comms_common_dai_link_ops = {
	.startup = byt_comms_dai_link_startup,
	.hw_params = byt_comms_dai_link_hw_params,
	.prepare = byt_comms_dai_link_prepare,
};
