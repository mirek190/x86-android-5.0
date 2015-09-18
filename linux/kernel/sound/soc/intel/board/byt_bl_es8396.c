/*
 *  byt_bl_es8396.c - ASoc Machine driver for Intel Baytrail Baylake MID platform
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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
/* #define DEBUG 1 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>

#include <linux/acpi_gpio.h>
#include <asm/platform_byt_audio.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include "../../codecs/es8396.h"
#include "../ssp/mid_ssp.h"
#include "byt_common.h"
#include "byt_comms_common.h"

#include <linux/delay.h>

#include <linux/rtc.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <asm/intel_soc_pmc.h>


#define VLV2_PLAT_CLK_AUDIO	3
#define PLAT_CLK_FORCE_ON	1
#define PLAT_CLK_FORCE_OFF	2

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define ES8396_NO_JACK		BIT(0)
#define ES8396_HEADSET_DET	BIT(1)
#define ES8396_HEADPHO_DET	BIT(2)

#define BYT_HS_DET_RETRY_COUNT 3
#define BYT_HS_DET_POLL_INTRVL 200
#define BYT_HS_BTN_POLL_INTRVL 200


#define BYT_HS_DET_WORKQUEUE
#define BYT_HS_BUTTON_WORKQUEUE

struct es8396_private {
	struct snd_soc_codec *codec;
	struct mutex mutex;
	bool jack_status;
	bool hs_status;


	int report;
	struct delayed_work headset_remove_poll_func;
	struct delayed_work headset_insert_poll_func;
	struct delayed_work hs_button_poll_func;

	int hs_det_retry;

	int hs_button_poll_intrvl;

	int button_state;

} es8396;

extern struct snd_soc_jack_gpio hs_gpio_common;

/* Machine Audio Map */
static const struct snd_soc_dapm_route byt_es8396_audio_map[] = {
		{"Headphone", NULL, "HPL"},
		{"Headphone", NULL, "HPR"},
		{"Ext Spk", NULL, "SPKOUTL"},
		{"Ext Spk", NULL, "SPKOUTR"},
		/* AMIC */
	/*
		Don't use RVP resource
		for HP detection issue on RVP
	*/
		{"Headset AMIC", NULL, "MIC Bias"},
		{"MONOINP", NULL, "Headset AMIC"},
		{"MONOINN", NULL, "Headset AMIC"},

		{"AMIC", NULL, "MIC Bias"},
		{"MIC", NULL, "AMIC"},


	/* DMIC */
/*
	RVP use V1P8 as DMIC
else
	use DBVDD
	{"DMIC", NULL, "MICBIAS3"},
*/
/*
	{"DMIC L1", NULL, "DMIC"},
	{"DMIC R1", NULL, "DMIC"},
*/

/*	MICVDD is supplied by external
else
	{"micbias1", NULL, "LDO2"},
*/
/*	DCVDD, DBVDD, SPKVDD, etc.
	is supply by external
else
	add widget to connect it
*/
};

static const struct snd_kcontrol_new byt_mc_controls_es8396[] = {
	SOC_DAPM_PIN_SWITCH("Headphone"),
	SOC_DAPM_PIN_SWITCH("Headset AMIC"),
	SOC_DAPM_PIN_SWITCH("AMIC"),
	SOC_DAPM_PIN_SWITCH("Ext Spk"),
};

/* One note for PLL and sysclk
1, BYT has 50fs issue
2, use pll to genarate 512fs clock for sysclk
3, still need to enable Realtek ASRC for resovle 50fs issue
4, using pll is to faster ASRC clock lock
*/
static int byt_aif_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	unsigned int fmt;
	int ret;

	pr_debug("Enter:%s", __func__);
	/* I2S Slave Mode`*/
	fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBS_CFS;

	/* Set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, fmt);
	if (ret < 0) {
		pr_err("can't set codec DAI configuration %d\n", ret);
		return ret;
	}


	ret = snd_soc_dai_set_pll(codec_dai, 0, ES8396_PLL_SRC_FRM_MCLK,
				BYT_PLAT_CLK_3_HZ, params_rate(params) * 256);
	if (ret < 0) {
		pr_err("can't set codec pll***********: %d\n", ret);
		return ret;
	}



	if (codec_dai->driver && codec_dai->driver->ops->hw_params)
		codec_dai->driver->ops->hw_params(substream, params, codec_dai);

	snd_soc_dai_set_sysclk(codec_dai, ES8396_CLKID_PLLO, BYT_PLAT_CLK_3_HZ, 0);


	return 0;
}

static int byt_compr_set_params(struct snd_compr_stream *cstream)
{
	struct snd_soc_pcm_runtime *rtd = cstream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	unsigned int fmt;
	int ret;

	pr_debug("Enter:%s", __func__);
	/* I2S Slave Mode`*/
	fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBS_CFS;

	/* Set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, fmt);
	if (ret < 0) {
		pr_err("can't set codec DAI configuration %d\n", ret);
		return ret;
	}

	return 0;
}

static int byt_es8396_init(struct snd_soc_pcm_runtime *runtime)
{
		struct snd_soc_codec *codec = runtime->codec;
		struct snd_soc_dapm_context *dapm = &codec->dapm;
		struct snd_soc_card *card = runtime->card;
		int ret;

		es8396.codec = codec;
		es8396.report = 0;
		mutex_init(&es8396.mutex);
		es8396.jack_status = false;
		pr_debug("Enter:%s", __func__);
		/* Set codec bias level */
		card->dapm.idle_bias_off = true;

	/* codec init registers */
	/* TODO */
		/* Keep the voice call paths active during
		suspend. Mark the end points ignore_suspend */
		/*TODO: CHECK this */

		snd_soc_dapm_ignore_suspend(dapm, "HPL");
		snd_soc_dapm_ignore_suspend(dapm, "HPR");

		snd_soc_dapm_ignore_suspend(dapm, "SPKOUTL");
		snd_soc_dapm_ignore_suspend(dapm, "SPKOUTR");
		snd_soc_dapm_ignore_suspend(dapm, "SDP1 Playback");
		snd_soc_dapm_ignore_suspend(dapm, "SDP1 Capture");

		snd_soc_dapm_ignore_suspend(dapm, "SDP2 Playback");
		snd_soc_dapm_ignore_suspend(dapm, "SDP2 Capture");

		snd_soc_dapm_ignore_suspend(&card->dapm, "Headphone");
		snd_soc_dapm_ignore_suspend(&card->dapm, "Headset AMIC");
		snd_soc_dapm_ignore_suspend(&card->dapm, "Ext Spk");
		snd_soc_dapm_ignore_suspend(&card->dapm, "AMIC");
		snd_soc_dapm_ignore_suspend(&card->dapm, "MIC");

		snd_soc_dapm_ignore_suspend(&card->dapm, "MONOINP");
		snd_soc_dapm_ignore_suspend(&card->dapm, "MONOINN");

		snd_soc_dapm_enable_pin(dapm, "Headphone");
		snd_soc_dapm_enable_pin(dapm, "Ext Spk");
		snd_soc_dapm_enable_pin(dapm, "Ext Spk");
		snd_soc_dapm_enable_pin(dapm, "Headset AMIC");
		snd_soc_dapm_enable_pin(dapm, "MIC Bias");
		snd_soc_dapm_enable_pin(dapm, "AMIC");
		snd_soc_dapm_enable_pin(dapm, "MIC");
		snd_soc_dapm_enable_pin(dapm, "MONOINP");
		snd_soc_dapm_enable_pin(dapm, "MONOINN");

		snd_soc_dapm_sync(dapm);

		ret = snd_soc_add_card_controls(card, byt_mc_controls_es8396,
						ARRAY_SIZE(byt_mc_controls_es8396));
		if (ret) {
			pr_err("unable to add card controls\n");

			return ret;
		}

	return 0;
}


static int es8396_headset_detect(struct snd_soc_codec *codec, int jack_insert)
{
	int value;
	int status;

	if (jack_insert) {

		value = snd_soc_read(codec, ES8396_GPIO_STA_REG17);

		switch (value & 0x03) {
		case 0x00:
			status = DET_HEADPHONE;
			break;
		case 0x01:
			status = DET_HEADSET;
			break;
		default:
			status = 0;
			break;
		}
	} else {
		status = 0;
	}

	return status;
}

static void headset_insert_poll(struct work_struct *work)
{
	struct snd_soc_jack_gpio *gpio = &hs_gpio_common;
	struct snd_soc_jack *jack = gpio->jack;
	struct es8396_private *ctx =
		 container_of(work, struct es8396_private, headset_insert_poll_func.work);
	struct snd_soc_codec *codec = ctx->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	int jack_type;

	jack_type = es8396_headset_detect(codec, 1);
	if (jack_type == DET_HEADSET) {
		jack_type = SND_JACK_HEADSET;
		dev_err(codec->dev, "%s %d:  Jack type: 0x%x\n", __func__, __LINE__, jack_type);
		ctx->hs_status = true;
		ctx->jack_status = true;
		snd_soc_jack_report(jack, jack_type, gpio->report);
		schedule_delayed_work(&ctx->hs_button_poll_func, 1 * HZ);
		return;
	} else {
		if (ctx->hs_det_retry++ >= BYT_HS_DET_RETRY_COUNT) {
			jack_type = SND_JACK_HEADPHONE;
			dev_dbg(codec->dev, "%s %d:  Jack type: 0x%x\n", __func__, __LINE__, jack_type);
			snd_soc_dapm_disable_pin(dapm, "MIC Bias");
			snd_soc_dapm_sync(dapm);
			ctx->hs_status = true;
			snd_soc_jack_report(jack, jack_type, gpio->report);
			return;
		}
	}
	schedule_delayed_work(&ctx->headset_insert_poll_func, 200);

}

static void headset_remove_poll(struct work_struct *work)
{
	struct snd_soc_jack_gpio *gpio = &hs_gpio_common;
	struct snd_soc_jack *jack = gpio->jack;
	struct es8396_private *ctx =
		 container_of(work, struct es8396_private, headset_remove_poll_func.work);
	struct snd_soc_codec *codec = ctx->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_dapm_disable_pin(dapm, "MIC Bias");
	snd_soc_dapm_sync(dapm);

	ctx->hs_status = false;
	ctx->jack_status = false;
	ctx->hs_det_retry = 0;
	snd_soc_jack_report(jack, 0, gpio->report);

}

static int byt_es8396_button_press_func(struct snd_soc_codec *codec)
{
	int btnstate = -1;
	u32 value;
	bool status;

	status = gpio_get_value(hs_gpio_common.gpio) ? 0 : 1;
	value = snd_soc_read(codec, ES8396_GPIO_STA_REG17);

	if (status) {
		switch (value & 0x3) {
		case 0x0:
				btnstate = 1;
				break;
		case 0x1:
				btnstate = 0;
				break;
		default:
				btnstate = -1;
				break;
		}
	}
	return btnstate;
}

static void hs_button_poll(struct work_struct *work)
{
	struct snd_soc_jack_gpio *gpio = &hs_gpio_common;
	struct snd_soc_jack *jack = gpio->jack;
	struct es8396_private *ctx =
		 container_of(work, struct es8396_private, hs_button_poll_func.work);
	int jack_type;
	int button;

	button = byt_es8396_button_press_func(ctx->codec);
	if ((button != ctx->button_state) && (button >= 0) && ctx->jack_status) {
		if (button)
			jack_type = SND_JACK_HEADSET | SND_JACK_BTN_0;
		else
			jack_type = SND_JACK_HEADSET;
		ctx->button_state = button;
		pr_debug("HS button event report: jack type: 0x%x, gpio: 0x%x\n",
					jack_type, gpio->report);
		snd_soc_jack_report(jack, jack_type, gpio->report);
	}

	if (ctx->hs_status && ctx->jack_status) {
		schedule_delayed_work(&ctx->hs_button_poll_func,
				msecs_to_jiffies(ctx->hs_button_poll_intrvl));
	}

}



void cancel_all_work(struct es8396_private *ctx)
{
	cancel_delayed_work_sync(&ctx->headset_insert_poll_func);
	cancel_delayed_work_sync(&ctx->headset_remove_poll_func);
	cancel_delayed_work_sync(&ctx->hs_button_poll_func);

}

static int byt_es8396_jack_detection(void)
{

	struct snd_soc_jack_gpio *gpio = &hs_gpio_common;
	struct snd_soc_jack *jack = gpio->jack;
	struct snd_soc_codec *codec = es8396.codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int enable;

	mutex_lock(&es8396.mutex);

	cancel_all_work(&es8396);
	enable = gpio_get_value(gpio->gpio);

	if (!enable) {
		snd_soc_dapm_force_enable_pin(dapm, "MIC Bias");
		snd_soc_dapm_sync(dapm);

		schedule_delayed_work(&es8396.headset_insert_poll_func, 50);
	} else {
		if (es8396.jack_status) {
			pr_debug("%s %d, enable = %d\n", __func__, __LINE__, enable);
			schedule_delayed_work(&es8396.headset_remove_poll_func, 0);
		}
	}

	mutex_unlock(&es8396.mutex);	/* Report old status */

	return jack->status;
}


static int byt_aif1_startup(struct snd_pcm_substream *substream)
{
	return snd_pcm_hw_constraint_list(substream->runtime, 0,
			SNDRV_PCM_HW_PARAM_RATE,
			&constraints_48000);
}

static struct snd_soc_ops byt_aif1_ops = {
	.startup = byt_aif1_startup,
	.hw_params = byt_aif_hw_params,
};

static struct snd_soc_ops byt_aif2_ops = {
	.hw_params = byt_aif_hw_params,
};

static struct snd_soc_compr_ops byt_compr_ops = {
	.set_params = byt_compr_set_params,
};

static struct snd_soc_dai_link byt_es8396_dailink[] = {

	[BYT_AUD_AIF1] = {
		.name = "Baytrail Audio",
		.stream_name = "Audio",
		.cpu_dai_name = "Headset-cpu-dai",
		.codec_dai_name = "es8396-sdp2",
		.codec_name = "es8396.2-0011",
		.platform_name = "sst-platform",
		.init = snd_byt_init,
		.ignore_suspend = 1,
		.ops = &byt_aif1_ops,
		.playback_count = 2,
	},
	[BYT_AUD_AIF2] = {
		.name = "Baytrail Voice",
		.stream_name = "Voice",
		.cpu_dai_name = "Voice-cpu-dai",
		.codec_dai_name = "es8396-sdp1",
		.codec_name = "es8396.2-0011",
		.platform_name = "sst-platform",
		.init = NULL,
		.ignore_suspend = 1,
		.ops = &byt_aif2_ops,
	},
	[BYT_AUD_COMPR_DEV] = {
		.name = "Baytrail Compressed Audio",
		.stream_name = "Compress",
		.cpu_dai_name = "Compress-cpu-dai",
		.codec_dai_name = "es8396-sdp2",
		.codec_name = "es8396.2-0011",
		.platform_name = "sst-platform",
		.init = NULL,
		.ignore_suspend = 1,
		.compr_ops = &byt_compr_ops,
	},

#ifdef CONFIG_SND_SOC_COMMS_SSP
	COMMS_DAI_LINK
#endif
};

static void byt_es8396_card_init(struct snd_soc_card *card)
{
	pmc_pc_configure(VLV2_PLAT_CLK_AUDIO,
		PLAT_CLK_FORCE_ON);

	card->dai_link = byt_es8396_dailink;
	card->num_links = ARRAY_SIZE(byt_es8396_dailink);
	card->dapm_routes = byt_es8396_audio_map;
	card->num_dapm_routes = ARRAY_SIZE(byt_es8396_audio_map);

#ifdef BYT_HS_BUTTON_WORKQUEUE
	INIT_DELAYED_WORK(&es8396.hs_button_poll_func, hs_button_poll);
	es8396.hs_button_poll_intrvl = BYT_HS_BTN_POLL_INTRVL;
#endif

#ifdef BYT_HS_DET_WORKQUEUE
	INIT_DELAYED_WORK(&es8396.headset_insert_poll_func, headset_insert_poll);
	INIT_DELAYED_WORK(&es8396.headset_remove_poll_func, headset_remove_poll);

	es8396.hs_det_retry = 0;
#endif
}


struct byt_machine_ops byt_bl_es8396_ops = {
	.card_init	= byt_es8396_card_init,
	.byt_init	= byt_es8396_init,
	.jack_detection	= byt_es8396_jack_detection,
};

MODULE_DESCRIPTION("ASoC Intel(R) Baytrail Machine driver");
MODULE_AUTHOR("Zhang Ning <ning.a.zhang@intel.com");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:bytes8396-audio");
