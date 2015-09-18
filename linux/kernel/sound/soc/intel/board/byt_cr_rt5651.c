/*
 *  byt_bl_rt5651.c - ASoc Machine driver for Intel Baytrail Baylake MID platform
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
#define DEBUG 1
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
#include "../../codecs/rt5651/rt5651.h"
#include "../ssp/mid_ssp.h"
#include "byt_common.h"
#include "byt_comms_common.h"

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

struct rt5651_private {
	struct snd_soc_codec *codec;
	struct mutex mutex;
	bool jack_status;
	int report;
} rt5651;

extern struct snd_soc_jack_gpio hs_gpio_common;


/* Machine Audio Map */
static const struct snd_soc_dapm_route byt_rt5651_audio_map[] = {
	{"Headphone", NULL, "HPOL"},
	{"Headphone", NULL, "HPOR"},
	{"Ext Spk", NULL, "LOUTL"},
	{"Ext Spk", NULL, "LOUTR"},
	/* AMIC */
/*
	Don't use RVP resource
	for HP detection issue on RVP
*/
	{"Headset AMIC", NULL, "micbias1"},
	{"IN3P", NULL, "Headset AMIC"},

	{"AMIC", NULL, "micbias1"},
	{"IN2P", NULL, "AMIC"},
	{"IN2N", NULL, "AMIC"},


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

static const struct snd_kcontrol_new byt_controls[] = {
	SOC_DAPM_PIN_SWITCH("Ext Spk"),
	SOC_DAPM_PIN_SWITCH("Headphone"),
	SOC_DAPM_PIN_SWITCH("AMIC"),
	SOC_DAPM_PIN_SWITCH("Headset AMIC"),
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

	ret = snd_soc_dai_set_pll(codec_dai, 0, RT5651_PLL1_S_MCLK,
				  BYT_PLAT_CLK_3_HZ, params_rate(params) * 512);
	if (ret < 0) {
		pr_err("can't set codec pll: %d\n", ret);
		return ret;
	}
	ret = snd_soc_dai_set_sysclk(codec_dai, RT5651_SCLK_S_PLL1,
				  params_rate(params) * 512, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		pr_err("can't set codec sysclk: %d\n", ret);
		return ret;
	}
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

	ret = snd_soc_dai_set_pll(codec_dai, 0, RT5651_PLL1_S_MCLK,
				  BYT_PLAT_CLK_3_HZ, 48000 * 512);
	if (ret < 0) {
		pr_err("can't set codec pll: %d\n", ret);
		return ret;
	}
	ret = snd_soc_dai_set_sysclk(codec_dai, RT5651_SCLK_S_PLL1,
				  48000 * 512, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		pr_err("can't set codec sysclk: %d\n", ret);
		return ret;
	}
	return 0;
}

static int byt_rt5651_init(struct snd_soc_pcm_runtime *runtime)
{
	struct snd_soc_codec *codec = runtime->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = runtime->card;

	rt5651.codec = codec;
	mutex_init(&rt5651.mutex);
	rt5651.jack_status = false;
	pr_debug("Enter:%s", __func__);
	/* Set codec bias level */
	card->dapm.idle_bias_off = true;

/* codec init registers */
/* TODO */
	/* Keep the voice call paths active during
	suspend. Mark the end points ignore_suspend */
	/*TODO: CHECK this */
	snd_soc_dapm_ignore_suspend(dapm, "HPOL");
	snd_soc_dapm_ignore_suspend(dapm, "HPOR");

	snd_soc_dapm_ignore_suspend(dapm, "LOUTL");
	snd_soc_dapm_ignore_suspend(dapm, "LOUTR");

	snd_soc_dapm_ignore_suspend(dapm, "IN2P");
	snd_soc_dapm_ignore_suspend(dapm, "IN2N");

	snd_soc_dapm_ignore_suspend(dapm, "IN3P");

	snd_soc_dapm_enable_pin(dapm, "Headphone");
	snd_soc_dapm_enable_pin(dapm, "Ext Spk");
	snd_soc_dapm_enable_pin(dapm, "Headset AMIC");
	snd_soc_dapm_enable_pin(dapm, "AMIC");
	snd_soc_dapm_enable_pin(dapm, "IN2P");
	snd_soc_dapm_enable_pin(dapm, "IN2N");
	snd_soc_dapm_enable_pin(dapm, "IN3P");

	snd_soc_dapm_disable_pin(dapm, "PDMR");
	snd_soc_dapm_disable_pin(dapm, "PDML");
	snd_soc_dapm_disable_pin(dapm, "IN1P");


	snd_soc_dapm_disable_pin(dapm, "DMIC");
/*	snd_soc_dapm_disable_pin(dapm, "DMIC L2");
	snd_soc_dapm_disable_pin(dapm, "DMIC R2");
	snd_soc_dapm_disable_pin(dapm, "DMIC L1");
	snd_soc_dapm_disable_pin(dapm, "DMIC R1");
	snd_soc_dapm_disable_pin(dapm, "LOUTL");
	snd_soc_dapm_disable_pin(dapm, "LOUTR");
*/
	/*mutex_lock(&codec->mutex);*/
	snd_soc_dapm_sync(dapm);
	return 0;
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
static struct snd_soc_dai_link byt_rt5651_dailink[] = {
	[BYT_AUD_AIF1] = {
		.name = "Baytrail Audio",
		.stream_name = "Audio",
		.cpu_dai_name = "Headset-cpu-dai",
		.codec_dai_name = "rt5651-aif1",
		.codec_name = "rt5651.2-001a",
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
		.codec_dai_name = "rt5651-aif2",
		.codec_name = "rt5651.2-001a",
		.platform_name = "sst-platform",
		.init = NULL,
		.ignore_suspend = 1,
		.ops = &byt_aif2_ops,
	},
	[BYT_AUD_COMPR_DEV] = {
		.name = "Baytrail Compressed Audio",
		.stream_name = "Compress",
		.cpu_dai_name = "Compress-cpu-dai",
		.codec_dai_name = "rt5651-aif1",
		.codec_name = "rt5651.2-001a",
		.platform_name = "sst-platform",
		.init = NULL,
		.ignore_suspend = 1,
		.compr_ops = &byt_compr_ops,
	},
#ifdef CONFIG_SND_SOC_COMMS_SSP
	COMMS_DAI_LINK
#endif
};

static void byt_rt5651_card_init(struct snd_soc_card *card)
{
	card->dai_link = byt_rt5651_dailink;
	card->num_links = ARRAY_SIZE(byt_rt5651_dailink);
	card->dapm_routes = byt_rt5651_audio_map;
	card->num_dapm_routes = ARRAY_SIZE(byt_rt5651_audio_map);
	card->controls = byt_controls;
	card->num_controls = ARRAY_SIZE(byt_controls);
}

#define JD_EVENT 0
#define BOT_EVENT 1
static int rt5651_check_event(struct snd_soc_codec *codec)
{

#if 0
	int status;
	status = snd_soc_read(codec, RT5645_INT_IRQ_ST);
	if (status & 0x1000)
		return JD_EVENT;
	else if (status & 0x4)
		return BOT_EVENT;
	else if (alc5645.jack_status)
		return JD_EVENT;
	return -1;
#else
	int status, reg;
	status = gpio_get_value(hs_gpio_common.gpio);
	reg = snd_soc_read(codec, RT5651_INT_IRQ_ST);

	if (status == 0) {
		if (!(reg & 0x1000) && !rt5651.jack_status)
			return JD_EVENT;
		else
			return BOT_EVENT;
	} else {
		if ((reg & 0x1000) && rt5651.jack_status)
			return JD_EVENT;
		else
			return BOT_EVENT;
	}
	return -1;
#endif

}
/* int rt5645_button_detect(struct snd_soc_codec *codec); */
static int byt_rt5651_jack_detection(void)
{
	int jack_type/*, bottun_type*/;
	int event, status;
	struct snd_soc_codec *codec = rt5651.codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	mutex_lock(&rt5651.mutex);
	event = rt5651_check_event(codec);
	pr_debug("event = %d\n", event);
	switch (event) {
	case JD_EVENT:
		status = gpio_get_value(hs_gpio_common.gpio) ? 0 : 1;
		pr_debug("gpio status is %d\n", status);
		if (status == 1) {
			snd_soc_dapm_force_enable_pin(dapm, "micbias1");
			snd_soc_dapm_sync(dapm);
			jack_type = rt5651_headset_detect(codec, status);
			switch (jack_type) {
			case RT5651_HEADSET_DET:
				rt5651.jack_status = true;
				rt5651.report = SND_JACK_HEADSET;
				mutex_unlock(&rt5651.mutex);
				return SND_JACK_HEADSET;
				break;
			case RT5651_HEADPHO_DET:
				snd_soc_dapm_disable_pin(dapm, "micbias1");
				snd_soc_dapm_sync(dapm);
				rt5651.jack_status = true;
				rt5651.report = SND_JACK_HEADPHONE;
				mutex_unlock(&rt5651.mutex);
				return SND_JACK_HEADPHONE;
				break;
			default:
				pr_err("No such jack_type %d\n", jack_type);
				mutex_unlock(&rt5651.mutex);
				return rt5651.report;
				break;
			}

		} else {
			snd_soc_dapm_disable_pin(dapm, "micbias1");
			snd_soc_dapm_sync(dapm);
			rt5651.jack_status = false;
			rt5651.report = 0;
			mutex_unlock(&rt5651.mutex);
			return 0;
		}
		break;
	case BOT_EVENT:
/*		bottun_type = rt5645_button_detect(codec);
		pr_debug("TODO: BOT_event: %d\n", bottun_type);
		pr_debug("TODO: all bottom events report as BTN_0\n");
		if (bottun_type)
			alc5645.report |= SND_JACK_BTN_0;
		else
			alc5645.report &= ~SND_JACK_BTN_0;
		mutex_unlock(&alc5645.mutex);
		return alc5645.report;

		break;
*/
	default:
		mutex_unlock(&rt5651.mutex);
		return rt5651.report;
		break;
	}
}

struct byt_machine_ops byt_cr_rt5651_ops = {
	.card_init	= byt_rt5651_card_init,
	.byt_init	= byt_rt5651_init,
	.jack_detection	= byt_rt5651_jack_detection,
};

MODULE_DESCRIPTION("ASoC Intel(R) Baytrail Machine driver");
MODULE_AUTHOR("Zhang Ning <ning.a.zhang@intel.com");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:bytrt5651-audio");
