/*
 * platform_gc5004.c: gc5004 platform data initilization file
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
#include <asm/intel_soc_pmc.h>
#include "platform_camera.h"
#include "platform_gc5004.h"

/* workround - pin defined for byt */
#define CAMERA_0_RESET 119 //meng 0924 :MCSI_GPIO[09]
#define CAMERA_0_PWDN 123 //MCSI_GPIO[06]-active low to power down in P1
#define AFVCC28_EN 118 //ECS-BYT: MCSI_GPIO[01], camera VCM2.8 control, active high.
#ifdef CONFIG_INTEL_SOC_PMC 
#define OSC_CAM0_CLK 0x0
#define CLK_19P2MHz 0x1
#endif
#ifdef CONFIG_CRYSTAL_COVE
#define VPROG_2P8V 0x66
#define VPROG_1P8V 0x5D
#define VPROG_ENABLE 0x3
#define VPROG_DISABLE 0x2
#endif
static int camera_reset;
static int camera_power_down;
static int camera_vprog1_on;
static int camera_vcm28_en;



/*
 * camera sensor - gc5004 platform data
 */

static int gc5004_gpio_ctrl(struct v4l2_subdev *sd, int flag)
{
	int ret;
	int pin;
	printk("%s,flag:%d\n",__func__,flag);
		/*
		 * FIXME: WA using hardcoded GPIO value here.
		 * The GPIO value would be provided by ACPI table, which is
		 * not implemented currently
		 */
		if (camera_reset < 0) {
			ret = gpio_request(CAMERA_0_RESET, "camera_0_reset");
			if (ret) {
				pr_err("%s: failed to request gpio(pin %d)\n",
				__func__, CAMERA_0_RESET);
				return -EINVAL;
			}
		}
		camera_reset = CAMERA_0_RESET;
		ret = gpio_direction_output(camera_reset, 1);
		if (ret) {
			pr_err("%s: failed to set gpio(pin %d) direction\n",
				__func__, camera_reset);
			gpio_free(camera_reset);
		}

		/*
		 * FIXME: WA using hardcoded GPIO value here.
		 * The GPIO value would be provided by ACPI table, which is
		 * not implemented currently.
		 */
		pin = CAMERA_0_PWDN;
		if (camera_power_down < 0) {
			ret = gpio_request(pin, "camera_0_power");
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

		
	if (flag) {
		gpio_set_value(camera_power_down, 0);
		
		gpio_set_value(camera_reset, 1);

	} else {
		gpio_set_value(camera_reset, 0);
		gpio_set_value(camera_power_down, 1);
		gpio_free(camera_reset);
		gpio_free(camera_power_down);
		camera_power_down = -1;
		camera_reset = -1;
	}

	return 0;
}

static int gc5004_flisclk_ctrl(struct v4l2_subdev *sd, int flag)
{
	static const unsigned int clock_khz = 19200;
	int ret = 0;
#ifdef CONFIG_INTEL_SOC_PMC
	if (flag) {
		ret = pmc_pc_set_freq(OSC_CAM0_CLK, CLK_19P2MHz);
		if (ret)
			return ret;
	}
	ret = pmc_pc_configure(OSC_CAM0_CLK, flag);
#endif
	return ret;
}

/*
 * The power_down gpio pin is to control OV5693's
 * internal power state.
 */
static int gc5004_power_ctrl(struct v4l2_subdev *sd, int flag)
{
	int ret = 0;
	printk("%s,flag:%d\n",__func__,flag);

	if (flag) {		
		if (!camera_vprog1_on) {
#ifdef CONFIG_CRYSTAL_COVE
			/*
			 * This should call VRF APIs.
			 *
			 * VRF not implemented for BTY, so call this
			 * as WAs
			 */
			ret = camera_set_pmic_power(CAMERA_2P8V, true);
			if (ret)
				return ret;
			ret = camera_set_pmic_power(CAMERA_1P8V, true);
#endif
			if (!ret)
				camera_vprog1_on = 1;
//			gpio_set_value(camera_vcm28_en, 1);
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
//			gpio_set_value(camera_vcm28_en, 0);
//			gpio_free(camera_vcm28_en);
//			camera_vcm28_en = -1;
			return ret;
		}
	}
	return ret;
}

static int gc5004_csi_configure(struct v4l2_subdev *sd, int flag)
{
    static const int LANES = 2;
	return camera_sensor_csi(sd, ATOMISP_CAMERA_PORT_PRIMARY, LANES,
		ATOMISP_INPUT_FORMAT_RAW_10, atomisp_bayer_order_rggb, flag);
}

static struct camera_sensor_platform_data gc5004_sensor_platform_data = {
	.gpio_ctrl	= gc5004_gpio_ctrl,
	.flisclk_ctrl	= gc5004_flisclk_ctrl,
	.power_ctrl	= gc5004_power_ctrl,
	.csi_cfg	= gc5004_csi_configure,
};

void *gc5004_platform_data(void *info)
{
	camera_reset = -1;
	camera_power_down = -1;
	camera_vprog1_on = 0;
	camera_vcm28_en = -1;

	return &gc5004_sensor_platform_data;
}
