/*
 *  byt_common.c - Common driver for codecs on Baytrail Platfrom
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/acpi_gpio.h>
#include <acpi/acpi_bus.h>
#include <asm/platform_byt_audio.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <asm/intel_soc_pmc.h>

#include "byt_common.h"

#define MAX_CONTROLS	2
/* extern from byt_bl_rt5642.c */
extern struct snd_kcontrol_new byt_ssp_comms_controls[MAX_CONTROLS];

struct snd_soc_jack_gpio hs_gpio_common = {
		.name			= "byt-codec-int",
		.report			= SND_JACK_HEADSET |
					  SND_JACK_HEADPHONE |
					  SND_JACK_BTN_0,
		.debounce_time		= 100,
};

unsigned int rates_8000_16000[] = {
	8000,
	16000,
};

struct snd_pcm_hw_constraint_list constraints_8000_16000 = {
	.count = ARRAY_SIZE(rates_8000_16000),
	.list = rates_8000_16000,
};

unsigned int rates_48000[] = {
	48000,
};

struct snd_pcm_hw_constraint_list constraints_48000 = {
	.count  = ARRAY_SIZE(rates_48000),
	.list   = rates_48000,
};

unsigned int rates_16000[] = {
	16000,
};

struct snd_pcm_hw_constraint_list constraints_16000 = {
	.count  = ARRAY_SIZE(rates_16000),
	.list   = rates_16000,
};

#define VLV2_PLAT_CLK_AUDIO	3
#define PLAT_CLK_FORCE_ON	1
#define PLAT_CLK_FORCE_OFF	2
/* Board specific codec bias level control */
int byt_set_bias_level(struct snd_soc_card *card,
		struct snd_soc_dapm_context *dapm,
		enum snd_soc_bias_level level)
{
	struct snd_soc_codec *codec;

	/* Clock management is done only if there is an associated codec
	 * to dapm context and if this not the dummy codec
	 */
	if (dapm->codec) {
		codec = dapm->codec;
		if (!strcmp(codec->name, "snd-soc-dummy"))
			return 0;
	} else {
		pr_debug("In %s dapm context has no associated codec or it is dummy codec.", __func__);
		return 0;
	}

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
	case SND_SOC_BIAS_STANDBY:
		if (card->dapm.bias_level == SND_SOC_BIAS_OFF) {
			pmc_pc_configure(VLV2_PLAT_CLK_AUDIO,
				PLAT_CLK_FORCE_ON);
		/*
			vlv2_plat_configure_clock(VLV2_PLAT_CLK_AUDIO,
					PLAT_CLK_FORCE_ON);
		*/
			pr_debug("Platform clk turned ON\n");
		}
		card->dapm.bias_level = level;
		break;
	case SND_SOC_BIAS_OFF:
		/* OSC clk will be turned OFF after processing
		 * codec->dapm.bias_level = SND_SOC_BIAS_OFF.
		 */
		break;
	default:
		pr_err("%s: Invalid bias level=%d\n", __func__, level);
		return -EINVAL;
		break;
	}
	pr_debug("%s: card(%s)->bias_level %u\n", __func__, card->name,
			card->dapm.bias_level);

	return 0;
}

int byt_set_bias_level_post(struct snd_soc_card *card,
		struct snd_soc_dapm_context *dapm,
		enum snd_soc_bias_level level)
{
	struct snd_soc_codec *codec;

	/* Clock management is done only if there is an associated codec
	 * to dapm context and if this not the dummy codec
	 */
	if (dapm->codec) {
		codec = dapm->codec;
		if (!strcmp(codec->name, "snd-soc-dummy"))
			return 0;
	} else {
		pr_debug("In %s dapm context has no associated codec or it is dummy codec.", __func__);
		return 0;
	}

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
	case SND_SOC_BIAS_STANDBY:
		/* Processing already done during set_bias_level()
		 * callback. No action required here.
		 */
		break;
	case SND_SOC_BIAS_OFF:
		if (codec->dapm.bias_level != SND_SOC_BIAS_OFF)
			break;
		pmc_pc_configure(VLV2_PLAT_CLK_AUDIO,
						PLAT_CLK_FORCE_OFF);
		/* vlv2_plat_configure_clock(VLV2_PLAT_CLK_AUDIO,
					PLAT_CLK_FORCE_OFF);
		*/
		pr_debug("Platform clk turned OFF\n");
		card->dapm.bias_level = level;
		break;
	default:
		pr_err("%s: Invalid bias level=%d\n", __func__, level);
		return -EINVAL;
		break;
	}
	dapm->bias_level = level;
	pr_debug("%s:card(%s)->bias_level %u\n", __func__, card->name,
			card->dapm.bias_level);
	return 0;
}

/*Machine  widgets */
static const struct snd_soc_dapm_widget byt_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone", NULL),
	SND_SOC_DAPM_MIC("Headset AMIC", NULL),
	SND_SOC_DAPM_MIC("AMIC", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
	SND_SOC_DAPM_MIC("DMIC", NULL),
};

struct snd_soc_card snd_soc_card_byt = {
	.name		= "baytrailaudio",
	.dapm_widgets = byt_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(byt_dapm_widgets),
	.set_bias_level = byt_set_bias_level,
	.set_bias_level_post = byt_set_bias_level_post,
};

int snd_byt_jack_init(struct snd_soc_pcm_runtime *runtime)
{
	int ret = 0;
	struct byt_common_private *ctx = snd_soc_card_get_drvdata(runtime->card);
	struct snd_soc_codec *codec = runtime->codec;

/* if driver doesn't implement the function,
   that means driver has its own jack detection
*/
	if (ctx->ops->jack_detection) {
		hs_gpio_common.gpio = acpi_get_gpio("\\_SB.GPO2", 4); /* GPIO_SUS4 */
		if (hs_gpio_common.gpio <= 0)
			return hs_gpio_common.gpio;
		hs_gpio_common.jack_status_check = ctx->ops->jack_detection;
		ret = snd_soc_jack_new(codec, "Intel MID Audio Jack",
			SND_JACK_HEADSET | SND_JACK_HEADPHONE | SND_JACK_BTN_0,
			       &ctx->jack);
		if (ret < 0)
			return ret;
		ret = snd_soc_jack_add_gpios(&ctx->jack, 1, &hs_gpio_common);
		if (ret < 0)
			return ret;
	}
	return ret;
}


int snd_byt_init(struct snd_soc_pcm_runtime *runtime)
{
	int ret;
	struct byt_common_private *ctx = snd_soc_card_get_drvdata(runtime->card);
	struct snd_soc_card *card = runtime->card;
	ret = ctx->ops->byt_init(runtime);
	if (ret) {
		pr_err("byt init returned failure\n");
		return ret;
	}
	/* Add Comms specific controls */
	ctx->comms_ctl.ssp_bt_sco_master_mode = true;
	ctx->comms_ctl.ssp_modem_master_mode = false;
#ifdef CONFIG_SND_SOC_COMMS_SSP
	snd_soc_add_card_controls(card, byt_ssp_comms_controls,
					ARRAY_SIZE(byt_ssp_comms_controls));
#endif

	return snd_byt_jack_init(runtime);
}
struct byt_machine_ops *byt_get_acpi_driver_data(const char *hid);
static int snd_byt_common_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct byt_common_private *ctx;

	struct device *dev = &pdev->dev;
	acpi_handle handle = ACPI_HANDLE(dev);
	struct acpi_device *device;
	const char *hid;

	pr_info("Entry %s\n", __func__);

	ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_ATOMIC);
	if (!ctx) {
		pr_err("allocation failed\n");
		return -ENOMEM;
	}

	ret = acpi_bus_get_device(handle, &device);
	if (ret) {
		pr_err("%s: could not get acpi device - %d\n", __func__, ret);
		return -ENODEV;
	}

	if (acpi_bus_get_status(device) || !device->status.present) {
		pr_err("%s: device has invalid status", __func__);
		return -ENODEV;
	}

	hid = acpi_device_hid(device);

	ctx->ops = byt_get_acpi_driver_data(hid);

	if (ctx->ops == NULL) {
		pr_err("No device added!!\n");
		return -ENODEV;
	}

	ctx->ops->card_init(&snd_soc_card_byt); /* fill snd_soc_card_byt */
	/* register the soc card */
	snd_soc_card_byt.dev = &pdev->dev;
	snd_soc_card_set_drvdata(&snd_soc_card_byt, ctx);
	ret = snd_soc_register_card(&snd_soc_card_byt);
	if (ret) {
		pr_err("snd_soc_register_card failed %d\n", ret);
		return ret;
	}


	platform_set_drvdata(pdev, &snd_soc_card_byt);
	pr_info("%s successful\n", __func__);
	return ret;
}
static int snd_byt_common_remove(struct platform_device *pdev)
{
	struct snd_soc_card *soc_card = platform_get_drvdata(pdev);
	struct byt_common_private *drv = snd_soc_card_get_drvdata(soc_card);

	pr_debug("In %s\n", __func__);
/* TODO */
	snd_soc_jack_free_gpios(&drv->jack, 1, &hs_gpio_common);
	snd_soc_card_set_drvdata(soc_card, NULL);
	snd_soc_unregister_card(soc_card);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int snd_byt_prepare(struct device *dev)
{
	pr_debug("In %s device name\n", __func__);
	return snd_soc_suspend(dev);
}

static void snd_byt_complete(struct device *dev)
{
	pr_debug("In %s\n", __func__);
	snd_soc_resume(dev);
}

static int snd_byt_poweroff(struct device *dev)
{
	pr_debug("In %s\n", __func__);
	return snd_soc_poweroff(dev);
}
#else
#define snd_byt_prepare NULL
#define snd_byt_complete NULL
#define snd_byt_poweroff NULL
#endif

const struct dev_pm_ops snd_byt_common_pm_ops = {
	.prepare = snd_byt_prepare,
	.complete = snd_byt_complete,
	.poweroff = snd_byt_poweroff,
};



static const struct acpi_device_id byt_common_acpi_ids[] = {
/* TODO
for ACPI FW engineer to provide new APCI match id
Use "TIMC0F28" to test No FW support
*/
#ifdef CONFIG_SND_BYT_RT5645
	{"AMCR0F29", (kernel_ulong_t) &byt_bl_alc5645_ops},
#endif
#ifdef CONFIG_SND_BYT_RT5651
	{"5651x", (kernel_ulong_t) &byt_cr_rt5651_ops},
#endif
#ifdef CONFIG_SND_BYT_ES8396
	{"8396x", (kernel_ulong_t) &byt_bl_es8396_ops},
#endif

	{},
};

MODULE_DEVICE_TABLE(acpi, byt_common_acpi_ids);


struct byt_machine_ops *byt_get_acpi_driver_data(const char *hid)
{
	const struct acpi_device_id *id;

	pr_debug("%s", __func__);
	for (id = byt_common_acpi_ids; id->id[0]; id++)
		if (!strncmp(id->id, hid, 16))
			return (struct byt_machine_ops *)id->driver_data;
	return NULL;
}


static struct platform_driver snd_byt_common_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "byt_catalog_audio",
		.pm = &snd_byt_common_pm_ops,
		.acpi_match_table = ACPI_PTR(byt_common_acpi_ids),
	},
	.probe = snd_byt_common_probe,
	.remove = snd_byt_common_remove,
};

static int __init snd_byt_driver_init(void)
{
	pr_info("Baytrail Machine Driver byt_catalog_audio registerd [%s]\n", __func__);
	return platform_driver_register(&snd_byt_common_driver);
}
late_initcall(snd_byt_driver_init);

static void __exit snd_byt_driver_exit(void)
{
	pr_debug("In %s\n", __func__);
	platform_driver_unregister(&snd_byt_common_driver);
}
module_exit(snd_byt_driver_exit);

MODULE_DESCRIPTION("ASoC Intel(R) Baytrail Machine driver");
MODULE_AUTHOR("Zhang Ning <ning.a.zhang@intel.com");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:byt-audio");
