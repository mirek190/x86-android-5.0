/*
 * platform_bf3a20.c: bf3a20 platform data initilization file
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
#include "platform_bf3a20.h"

/* pin defined for byd */
#define CAMERA_1_RESET 120
#define CAMERA_1_PWDN 124
#define CAM28_EN 119

#ifdef CONFIG_INTEL_SOC_PMC
#define OSC_CAM1_CLK 0x1
#define CLK_19P2MHz 0x1
#endif

#ifdef CONFIG_CRYSTAL_COVE
#define VPROG_2P8V 0x66
#define VPROG_1P8V 0x5D
#define VPROG_ENABLE 0x3
#define VPROG_DISABLE 0x2

#endif

static int byd_camera_power_down;
static int byd_camera_reset;
static int camera_vprog1_on;
static int camera_p28_en;

/*
 * BF3A20 platform data
 */

/*
 * BF3A20 Power up sequences
 */
static int bf3a20_gpio_ctrl(struct v4l2_subdev *sd, int flag)
{
	int ret;
	int pin = CAMERA_1_RESET;

	if (byd_camera_reset < 0) {
		ret = gpio_request(pin, "camera_0_reset");
		if (ret) {
			pr_err("%s: failed to request gpio(pin %d)\n",
					__func__, pin);
			return ret;
		}
	}
	byd_camera_reset = pin;
	ret = gpio_direction_output(pin, 1);
	if (ret) {
		pr_err("%s: failed to set gpio(pin %d) direction\n",
				__func__, pin);
		gpio_free(pin);
		return ret;
	}

	pin = CAMERA_1_PWDN;
	if (byd_camera_power_down < 0) {
		ret = gpio_request(pin, "camera_0_power");
		if (ret) {
			pr_err("%s: failed to request gpio(pin %d)\n",
					__func__, pin);
			return ret;
		}
	}
	byd_camera_power_down = pin;
	ret = gpio_direction_output(pin, 1);
	if (ret) {
		pr_err("%s: failed to set gpio(pin %d) direction\n",
				__func__, pin);
		gpio_free(pin);
		return ret;
	}

	if (flag) {
		gpio_set_value(byd_camera_reset, 1);
		msleep(10);
		gpio_set_value(byd_camera_power_down, 0);
		msleep(25);
	} else {
		/* power off sequence */
		gpio_set_value(byd_camera_reset, 0);
		gpio_set_value(byd_camera_power_down, 1);
		gpio_free(byd_camera_reset);
		gpio_free(byd_camera_power_down);
		byd_camera_reset = -1;
		byd_camera_power_down = -1;
		msleep(20);
	}

	return 0;
}

static int bf3a20_flisclk_ctrl(struct v4l2_subdev *sd, int flag)
{
	static const unsigned int clock_khz = 19200;
#ifdef CONFIG_INTEL_SOC_PMC
	int ret = 0;
	if (flag) {
		ret = pmc_pc_set_freq(OSC_CAM1_CLK, CLK_19P2MHz);
		if (ret)
			return ret;
	}
	return pmc_pc_configure(OSC_CAM1_CLK, flag);
#endif
	
	return 0;
}

/*
 * The power_down gpio pin is to control BF3A20's
 * internal power state.
 */
static int bf3a20_power_ctrl(struct v4l2_subdev *sd, int flag)
{
	int ret = 0;

	if (flag) {
		if (!camera_vprog1_on) {
#ifdef CONFIG_CRYSTAL_COVE
			/*
			 * This should call VRF APIs.
			 *
			 * VRF not implemented for BTY, so call this
			 * as WAs
			 */
			ret = camera_set_pmic_power(CAMERA_1P8V, true);
			if (ret)
				return ret;
			ret = camera_set_pmic_power(CAMERA_2P8V, true);
#endif
			if (!ret)
				camera_vprog1_on = 1;
			msleep(10);
			return ret;
		}
	} else {
		if (camera_vprog1_on) {
#ifdef CONFIG_CRYSTAL_COVE
			ret = camera_set_pmic_power(CAMERA_2P8V, false);
			if (ret)
				return ret;
			ret = camera_set_pmic_power(CAMERA_1P8V, false);
#endif
			if (!ret)
				camera_vprog1_on = 0;
			return ret;
		}
	}
	return ret;
}

static int bf3a20_csi_configure(struct v4l2_subdev *sd, int flag)
{
	return camera_sensor_csi(sd, ATOMISP_CAMERA_PORT_SECONDARY, 1,
		ATOMISP_INPUT_FORMAT_RAW_8, atomisp_bayer_order_gbrg, flag);
}

static struct camera_sensor_platform_data bf3a20_sensor_platform_data = {
	.gpio_ctrl	= bf3a20_gpio_ctrl,
	.flisclk_ctrl	= bf3a20_flisclk_ctrl,
	.power_ctrl	= bf3a20_power_ctrl,
	.csi_cfg	= bf3a20_csi_configure,
};

void *bf3a20_platform_data(void *info)
{
	camera_vprog1_on = 0;
	byd_camera_power_down = -1;
	byd_camera_reset = -1;
	return &bf3a20_sensor_platform_data;
}
