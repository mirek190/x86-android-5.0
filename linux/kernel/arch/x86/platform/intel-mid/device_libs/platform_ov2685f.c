/*
 * platform_ov2685f.c: ov2685f platform data initilization file
 *
 * (C) Copyright 2013 Intel Corporation
 * Author:
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/atomisp_platform.h>
#include <asm/intel_scu_ipcutil.h>
#include <asm/intel-mid.h>
#include <media/v4l2-subdev.h>
#include <linux/mfd/intel_mid_pmic.h>

#ifdef CONFIG_INTEL_SOC_PMC
#include <asm/intel_soc_pmc.h>
#endif

#include "platform_camera.h"
#include "platform_ov2685f.h"

/* workround - pin defined for byt */
/* #ifdef CONFIG_MRD8 */
#define CAMERA_1_RESET 120
#define CAMERA_1_PWDN 124
/*
#define CAM28_EN 119
#define AFVCC28_EN 118
#else
#define CAMERA_0_RESET	126
#endif
*/

#define CAMERA_1P8_EN	128
#ifdef CONFIG_INTEL_SOC_PMC
#define OSC_CAM1_CLK 0x0
#define CLK_19P2MHz 0x1
#endif
#ifdef CONFIG_CRYSTAL_COVE
#define VPROG_2P8V 0x66
#define VPROG_1P8V 0x5D
#endif
/* #ifdef CONFIG_MRD8 */
static int camera_power_down;
/* #endif */
static int camera_vprog1_on;
static int camera_reset;

/*
 * OV2685 platform data
 */

static int ov2685f_gpio_ctrl(struct v4l2_subdev *sd, int flag)
{
	int ret;
	int pin;

	/*
	 * FIXME: WA using hardcoded GPIO value here.
	 * The GPIO value would be provided by ACPI table, which is
	 * not implemented currently.
	 */
	pin = CAMERA_1_RESET;
	if (camera_reset < 0) {
		ret = gpio_request(pin, "camera_1_reset");
		if (ret) {
			pr_err("%s: failed to request gpio(pin %d)\n",
				__func__, pin);
			return ret;
		}
		camera_reset = pin;
		ret = gpio_direction_output(camera_reset, 1);
		if (ret) {
			pr_err("%s: failed to set gpio(pin %d) direction\n",
				__func__, camera_reset);
			gpio_free(camera_reset);
			return ret;
		}
	}

/* #ifdef CONFIG_MRD8 */
	/*
	 * FIXME: WA using hardcoded GPIO value here.
	 * The GPIO value would be provided by ACPI table, which is
	 * not implemented currently.
	 */
	pin = CAMERA_1_PWDN;
	if (camera_power_down < 0) {
		ret = gpio_request(pin, "camera_1_power");
		if (ret) {
			pr_err("%s: failed to request gpio(pin %d)\n",
				__func__, pin);
			return ret;
		}
	}
	camera_power_down = pin;
	ret = gpio_direction_output(pin, 1);

	if (ret) {
		pr_err("%s: failed to set gpio(pin %d) direction\n",
			__func__, pin);
		gpio_free(pin);
		return ret;
	}
/* #endif */

	if (flag) {
		/* #ifdef CONFIG_MRD8 */
		gpio_set_value(camera_power_down, 0);

		gpio_set_value(camera_reset, 0);
		msleep(20);
		/* #endif */
		gpio_set_value(camera_reset, 1);
	} else {
		gpio_set_value(camera_reset, 0);
		/* #ifdef CONFIG_MRD8 */
		gpio_set_value(camera_power_down, 1);
		gpio_free(camera_reset);
		gpio_free(camera_power_down);
		camera_reset = -1;
		camera_power_down = -1;
		/* #endif */
	}

	return 0;
}

/*
 * WORKAROUND:
 * This func will return 0 since MCLK is enabled by BIOS
 * and will be always on event if set MCLK failed here.
 * TODO: REMOVE WORKAROUND, err should be returned when
 * set MCLK failed.
 */
static int ov2685f_flisclk_ctrl(struct v4l2_subdev *sd, int flag)
{
	static const unsigned int clock_khz = 19200;
#ifdef CONFIG_INTEL_SOC_PMC
	int ret = 0;
	if (flag) {
		ret = pmc_pc_set_freq(OSC_CAM1_CLK, CLK_19P2MHz);
		if (ret)
			pr_err("ov2685f clock set failed.\n");
	}
	pmc_pc_configure(OSC_CAM1_CLK, flag);
	return 0;
#elif defined(CONFIG_INTEL_SCU_IPC_UTIL)
	return intel_scu_ipc_osc_clk(OSC_CLK_CAM1,
				     flag ? clock_khz : 0);
#else
	pr_err("ov2685f clock is not set.\n");
	return 0;
#endif
}

/*
 * The camera_v1p8_en gpio pin is to enable 1.8v power.
 */
static int ov2685f_power_ctrl(struct v4l2_subdev *sd, int flag)
{
	int ret = 0;
	/* int pin = CAMERA_1P8_EN; */
	int val;

#if 0
	ret = gpio_request(pin, "camera_v1p8_en");
	if (ret) {
		pr_err("Request camera_v1p8_en failed.\n");
		gpio_free(pin);
		ret = gpio_request(pin, "camera_v1p8_en");
		if (ret) {
			pr_err("Request camera_v1p8_en still failed.\n");
			return ret;
		}
	}
	gpio_direction_output(pin, 0);
#endif

	if (flag) {
		if (!camera_vprog1_on) {
#ifdef CONFIG_CRYSTAL_COVE
			/*
			 * This should call VRF APIs.
			 *
			 * VRF not implemented for BTY, so call this
			 * as WAs
			 */
			camera_set_pmic_power(CAMERA_2P8V, true);
			camera_set_pmic_power(CAMERA_1P8V, true);
#elif defined(CONFIG_INTEL_SCU_IPC_UTIL)
			ret = intel_scu_ipc_msic_vprog1(1);
#else
			pr_err("ov2685f power is not set.\n");
#endif

#if 0
			/* enable 1.8v power */
			gpio_set_value(pin, 1);
#endif
			camera_vprog1_on = 1;
			usleep_range(10000, 11000);
		}
	} else {
		if (camera_vprog1_on) {
#ifdef CONFIG_CRYSTAL_COVE
			camera_set_pmic_power(CAMERA_2P8V, false);
			camera_set_pmic_power(CAMERA_1P8V, false);
#elif defined(CONFIG_INTEL_SCU_IPC_UTIL)
			ret = intel_scu_ipc_msic_vprog1(0);
#else
			pr_err("ov2685f power is not set.\n");
#endif
#if 0
			/* disable 1.8v power */
			gpio_set_value(pin, 0);

#endif
			camera_vprog1_on = 0;
		}
	}

#if 0
	gpio_free(pin);
#endif
	return 0;
}

static int ov2685f_csi_configure(struct v4l2_subdev *sd, int flag)
{
	const u32 csi_lane = 1;
	return camera_sensor_csi(sd, ATOMISP_CAMERA_PORT_SECONDARY, csi_lane,
		ATOMISP_INPUT_FORMAT_YUV422_8, atomisp_bayer_order_bggr, flag);
}

static struct camera_sensor_platform_data ov2685f_sensor_platform_data = {
	.gpio_ctrl	= ov2685f_gpio_ctrl,
	.flisclk_ctrl	= ov2685f_flisclk_ctrl,
	.power_ctrl	= ov2685f_power_ctrl,
	.csi_cfg	= ov2685f_csi_configure,
};

void *ov2685f_platform_data(void *info)
{
	camera_reset = -1;
/* #ifdef CONFIG_MRD8 */
	camera_power_down = -1;
	camera_vprog1_on = 0;
/* #endif */
	return &ov2685f_sensor_platform_data;
}

