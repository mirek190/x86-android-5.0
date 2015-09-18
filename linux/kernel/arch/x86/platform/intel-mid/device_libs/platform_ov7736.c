/*
 * platform_ov7736.c: ov7736 platform data initilization file
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
#include "platform_ov7736.h"

/* workround - pin defined for byt */
#define CAMERA_1_RESET 120 //meng 0924 :MCSI_GPIO[09]
#define CAMERA_1_PWDN 124 //MCSI_GPIO[06]-active low to power down in P1
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
static int camera_power_down;
static int camera_reset;
static int camera_p28_en;

/*
 * camera sensor - ov7736 platform data
 */

static int ov7736_gpio_ctrl(struct v4l2_subdev *sd, int flag)
{
	int ret;
	int pin;

    /*
     * FIXME: WA using hardcoded GPIO value here.
     * The GPIO value would be provided by ACPI table, which is
     * not implemented currently
     */
    pin = CAMERA_1_RESET; 
    if (camera_reset < 0) {
        ret = gpio_request(CAMERA_1_RESET, "camera_1_reset");
        if (ret) {
            pr_err("%s: failed to request gpio(pin %d)\n",
                    __func__, CAMERA_1_RESET);
            return -EINVAL;
        }
    }
    camera_reset = pin;
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


	if (flag) {
		gpio_set_value(camera_power_down, 0);
		usleep_range(5000, 6000);
		gpio_set_value(camera_reset, 1);
	} else {
		gpio_set_value(camera_reset, 0);
		gpio_set_value(camera_power_down, 1);
		gpio_free(camera_reset);
		gpio_free(camera_power_down);
		camera_reset = -1;
		camera_power_down = -1;
	}

	return 0;
}

static int ov7736_flisclk_ctrl(struct v4l2_subdev *sd, int flag)
{
	static const unsigned int clock_khz = 19200;
#ifdef CONFIG_INTEL_SOC_PMC
	if (flag) {
		int ret;
		ret = pmc_pc_set_freq(OSC_CAM1_CLK, CLK_19P2MHz);
		if (ret)
			return ret;
	}
	return pmc_pc_configure(OSC_CAM1_CLK, flag);
#endif
	return 0;
}

/*
 * The power_down gpio pin is to control OV7736's
 * internal power state.
 */
static int ov7736_power_ctrl(struct v4l2_subdev *sd, int flag)
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

static int ov7736_csi_configure(struct v4l2_subdev *sd, int flag)
{
    static const int LANES = 1;
	return camera_sensor_csi(sd, ATOMISP_CAMERA_PORT_SECONDARY, LANES,
		ATOMISP_INPUT_FORMAT_YUV422_8, -1, flag);
}

static struct camera_sensor_platform_data ov7736_sensor_platform_data = {
	.gpio_ctrl	= ov7736_gpio_ctrl,
	.flisclk_ctrl	= ov7736_flisclk_ctrl,
	.power_ctrl	= ov7736_power_ctrl,
	.csi_cfg	= ov7736_csi_configure,
};

void *ov7736_platform_data(void *info)
{
	camera_reset = -1;
	camera_power_down = -1;
	camera_vprog1_on = 0;

	return &ov7736_sensor_platform_data;
}

