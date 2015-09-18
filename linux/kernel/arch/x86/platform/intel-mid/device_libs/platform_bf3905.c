/*
 * platform_bf3905.c: bf3905 platform data initilization file
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
#include "platform_bf3905.h"

/* workround - pin defined for byt */
#define CAMERA_1_RESET 120
#define CAMERA_1_PWDN 124
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
static int camera_vprog1_on;
static int gp_camera1_power_down;
static int gp_camera1_reset;

/*
 * BF3905 platform data
 */

static int bf3905_gpio_ctrl(struct v4l2_subdev *sd, int flag)
{
		int ret;
		int pin;

		pin = CAMERA_1_RESET;
		if (gp_camera1_reset < 0) {
				ret = gpio_request(pin, "camera_0_reset");
				if (ret) {
						pr_err("%s: failed to request gpio(pin %d)\n",
										__func__, pin);
						return ret;
				}
		}
		gp_camera1_reset = pin;
		printk("fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
		ret = gpio_direction_output(pin, 1);
		if (ret) {
				pr_err("%s: failed to set gpio(pin %d) direction\n",
								__func__, pin);
				gpio_free(pin);
				return ret;
		}

		/*
		 * FIXME: WA using hardcoded GPIO value here.
		 * The GPIO value would be provided by ACPI table, which is
		 * not implemented currently.
		 */
		pin = CAMERA_1_PWDN;
		if (gp_camera1_power_down < 0) {
				ret = gpio_request(pin, "camera_0_power");
				if (ret) {
						pr_err("%s: failed to request gpio(pin %d)\n",
										__func__, pin);
						return ret;
				}
		}
		gp_camera1_power_down = pin;

		ret = gpio_direction_output(pin, 1);

		if (ret) {
				pr_err("%s: failed to set gpio(pin %d) direction\n",
								__func__, pin);
				gpio_free(pin);
				return ret;
		}

		if (flag) {
				/*		printk("fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
						gpio_set_value(gp_camera1_power_down, 1);
						msleep(20);
						printk("fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
						gpio_set_value(gp_camera1_reset, 0);
						msleep(20);
						printk("fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
						gpio_set_value(gp_camera1_reset, 1);	
						printk("fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
				 */
				gpio_set_value(gp_camera1_reset, 1);
				msleep(10);
				gpio_set_value(gp_camera1_power_down, 0);
				msleep(25);
		} else {
				/*
				   printk("fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
				   gpio_set_value(gp_camera1_reset, 1);
				   printk("fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
				   gpio_set_value(gp_camera1_power_down, 0);
				 */
				gpio_set_value(gp_camera1_reset, 0);
				gpio_set_value(gp_camera1_power_down, 1);
				gpio_free(gp_camera1_reset);
				gpio_free(gp_camera1_power_down);
				gp_camera1_reset = -1;
				gp_camera1_power_down = -1;
				msleep(20);//
		}

		return 0;
}

static int bf3905_flisclk_ctrl(struct v4l2_subdev *sd, int flag)
{
		static const unsigned int clock_khz = 19200;
		int ret = 0;
#ifdef CONFIG_INTEL_SOC_PMC
		if (flag) {
				printk("fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
				ret = pmc_pc_set_freq(OSC_CAM1_CLK, CLK_19P2MHz);
				if (ret)
						return ret;
		}
		printk("fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
		ret = pmc_pc_configure(OSC_CAM1_CLK, flag);
#endif
		return ret;
}

/*
 * The power_down gpio pin is to control BF3905's
 * internal power state.
 */
static int bf3905_power_ctrl(struct v4l2_subdev *sd, int flag)
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

		return 0;
}

static int bf3905_csi_configure(struct v4l2_subdev *sd, int flag)
{
		return camera_sensor_csi(sd, ATOMISP_CAMERA_PORT_SECONDARY, 1,
						ATOMISP_INPUT_FORMAT_RAW_8, atomisp_bayer_order_bggr, flag);
}

static struct camera_sensor_platform_data bf3905_sensor_platform_data = {
		.gpio_ctrl	= bf3905_gpio_ctrl,
		.flisclk_ctrl	= bf3905_flisclk_ctrl,
		.power_ctrl	= bf3905_power_ctrl,
		.csi_cfg	= bf3905_csi_configure,
};

void *bf3905_platform_data(void *info)
{
		camera_vprog1_on = 0;
		gp_camera1_power_down = -1;
		gp_camera1_reset = -1;
		return &bf3905_sensor_platform_data;
}
