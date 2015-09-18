/*
 * platform_gc2155.c: gc2155 platform data initilization file
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
#include <linux/i2c.h>
#include <linux/atomisp_platform.h>
#include <linux/regulator/consumer.h>
#include <asm/intel_scu_ipcutil.h>
#include <asm/intel-mid.h>
#include <media/v4l2-subdev.h>
#include <linux/mfd/intel_mid_pmic.h>
#include <asm/intel_soc_pmc.h>
#include "platform_camera.h"
#include "platform_gc2155.h"

/* workround - pin defined for byt */
#define CAMERA_0_RESET 119
#define CAMERA_0_PWDN 123
#define AFVCC28_EN 118
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


#ifdef CONFIG_CRYSTAL_COVE
static struct regulator *v1p8_reg;
static struct regulator *v2p8_reg;

/* PMIC HID */
#define PMIC_HID_ROHM	"INT33FD:00"
#define PMIC_HID_XPOWER	"INT33F4:00"
#define PMIC_HID_TI	"INT33F5:00"

enum pmic_ids {
	PMIC_ROHM = 0,
	PMIC_XPOWER,
	PMIC_TI,
	PMIC_MAX
};

static enum pmic_ids pmic_id;
#endif



#ifdef CONFIG_CRYSTAL_COVE
static int match_name(struct device *dev, void *data)
{
	const char *name = data;
	struct i2c_client *client = to_i2c_client(dev);
	return !strncmp(client->name, name, strlen(client->name));
}

static struct i2c_client *i2c_find_client_by_name(char *name)
{
	struct device *dev = bus_find_device(&i2c_bus_type, NULL,
						name, match_name);
	return dev ? to_i2c_client(dev) : NULL;
}

static enum pmic_ids camera_pmic_probe()
{
	/* search by client name */
	struct i2c_client *client;
	if (spid.hardware_id != BYT_TABLET_BLK_CRV2 ||
		i2c_find_client_by_name(PMIC_HID_ROHM))
		return PMIC_ROHM;

	client = i2c_find_client_by_name(PMIC_HID_XPOWER);
	if (client)
		return PMIC_XPOWER;

	client = i2c_find_client_by_name(PMIC_HID_TI);
	if (client)
		return PMIC_TI;

	return PMIC_MAX;
}

#if 0
static int camera_pmic_set(bool flag)
{
	int val;
	int ret = 0;
	if (pmic_id == PMIC_MAX) {
		pmic_id = camera_pmic_probe();

		if (pmic_id == PMIC_MAX)
			return -EINVAL;
	}

	if (flag) {
		switch (pmic_id) {
		case PMIC_ROHM:
			ret = regulator_enable(v2p8_reg);
			if (ret)
				return ret;

			ret = regulator_enable(v1p8_reg);

			if (ret)
				regulator_disable(v2p8_reg);
			break;
		case PMIC_XPOWER:

			/* ALDO1 */
			ret = intel_mid_pmic_writeb(0x28, 0x16);
			if (ret)
				return ret;

			/* PMIC Output CTRL 3 for ALDO1 */
			val = intel_mid_pmic_readb(0x13);
			val |= (1 << 5);
			ret = intel_mid_pmic_writeb(0x13, val);
			if (ret)
				return ret;

			/* ELDO2 */
			ret = intel_mid_pmic_writeb(0x1A, 0x16);
			if (ret)
				return ret;

			/* PMIC Output CTRL 2 for ELDO2 */
			val = intel_mid_pmic_readb(0x12);
			val |= (1 << 1);
			ret = intel_mid_pmic_writeb(0x12, val);
			break;
		case PMIC_TI:
			/* LDO9 */
			ret = intel_mid_pmic_writeb(0x49, 0x2F);
			if (ret)
				return ret;

			/* LDO10 */
			ret = intel_mid_pmic_writeb(0x4A, 0x59);
			if (ret)
				return ret;

			/* LDO11 */
			ret = intel_mid_pmic_writeb(0x4B, 0x59);
			if (ret)
				return ret;
			break;
		default:
			return -EINVAL;
		}

	} else {
		switch (pmic_id) {
		case PMIC_ROHM:
			ret = regulator_disable(v2p8_reg);
			ret += regulator_disable(v1p8_reg);
			break;
		case PMIC_XPOWER:

			val = intel_mid_pmic_readb(0x13);
			val &= ~(1 << 5);
			ret = intel_mid_pmic_writeb(0x13, val);
			if (ret)
				return ret;

			val = intel_mid_pmic_readb(0x12);
			val &= ~(1 << 1);
			ret = intel_mid_pmic_writeb(0x12, val);
			break;

		case PMIC_TI:
			/* LDO9 */
			ret = intel_mid_pmic_writeb(0x49, 0x2E);
			if (ret)
				return ret;

			/* LDO10 */
			ret = intel_mid_pmic_writeb(0x4A, 0x58);
			if (ret)
				return ret;

			/* LDO11 */
			ret = intel_mid_pmic_writeb(0x4B, 0x58);
			if (ret)
				return ret;
			break;
		default:
			return -EINVAL;
		}
	}
	return ret;
}
#endif
#endif


/*
 * camera sensor - gc2155 platform data
 */

static int gc2155_gpio_ctrl(struct v4l2_subdev *sd, int flag)
{
	int ret;
	int pin;
		/*
		 * FIXME: WA using hardcoded GPIO value here.
		 * The GPIO value would be provided by ACPI table, which is
		 * not implemented currently.
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
		gpio_set_value(camera_power_down, 1);

		gpio_set_value(camera_power_down, 0);

		msleep(20);

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

static int gc2155_flisclk_ctrl(struct v4l2_subdev *sd, int flag)
{
	static const unsigned int clock_khz = 19200;
	int ret = 0;
#ifdef CONFIG_INTEL_SOC_PMC
	if (flag) {
		ret = pmc_pc_set_freq(OSC_CAM0_CLK, 0x1 /*CLK_19P2MHz*/);
		if (ret)
			return ret;
	}
	ret = pmc_pc_configure(OSC_CAM0_CLK, flag);
#endif
	return ret;
}

/*
 * The power_down gpio pin is to control GC2155's
 * internal power state.
 */
static int gc2155_power_ctrl(struct v4l2_subdev *sd, int flag)
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

			msleep(30);
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

			msleep(30);
			return ret;
		}
	}
	return ret;
}

static int gc2155_csi_configure(struct v4l2_subdev *sd, int flag)
{
	static const int LANES = 2;
	return camera_sensor_csi(sd, ATOMISP_CAMERA_PORT_PRIMARY, LANES,
		ATOMISP_INPUT_FORMAT_YUV422_8, atomisp_bayer_order_rggb, flag);
}


#ifdef CONFIG_CRYSTAL_COVE
static int gc2155_platform_init(struct i2c_client *client)
{
	pmic_id = camera_pmic_probe();
	if (pmic_id != PMIC_ROHM)
		return 0;


	v1p8_reg = regulator_get(&client->dev, "v1p8sx");
	if (IS_ERR(v1p8_reg)) {
		dev_err(&client->dev, "v1p8s regulator_get failed\n");
		return PTR_ERR(v1p8_reg);
	}

	v2p8_reg = regulator_get(&client->dev, "v2p85sx");
	if (IS_ERR(v2p8_reg)) {
		regulator_put(v1p8_reg);
		dev_err(&client->dev, "v2p85sx regulator_get failed\n");
		return PTR_ERR(v2p8_reg);
	}

	return 0;
}

static int gc2155_platform_deinit(void)
{
	if (pmic_id != PMIC_ROHM)
		return 0;

	regulator_put(v1p8_reg);
	regulator_put(v2p8_reg);

	return 0;
}
#endif

static struct camera_sensor_platform_data gc2155_sensor_platform_data = {
	.gpio_ctrl	= gc2155_gpio_ctrl,
	.flisclk_ctrl	= gc2155_flisclk_ctrl,
	.power_ctrl	= gc2155_power_ctrl,
	.csi_cfg	= gc2155_csi_configure,
#ifdef CONFIG_CRYSTAL_COVE
	.platform_init = gc2155_platform_init,
	.platform_deinit = gc2155_platform_deinit,
#endif
};

void *gc2155_platform_data(void *info)
{
	camera_reset = -1;
	camera_power_down = -1;
	camera_vprog1_on = 0;

	pmic_id = camera_pmic_probe();

	return &gc2155_sensor_platform_data;
}
