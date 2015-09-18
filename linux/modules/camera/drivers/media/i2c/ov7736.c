/*
 * Support for ov7736 Camera Sensor.
 *
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/firmware.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>

#include "ov7736.h"

#define ov7736_debug dev_dbg

static int awb_index = V4L2_WHITE_BALANCE_AUTO;
static int scene_index = V4L2_SCENE_MODE_NONE;
static int vflag = 0;
static int hflag = 0;
static int exposure_index = 0;
static bool mbus_fmt_called = true;
static bool need_init = false;

static int
ov7736_read_reg(struct i2c_client *client, u16 data_length, u16 reg, u32 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[4];

	if (!client->adapter) {
		dev_err(&client->dev, "%s error, no client->adapter\n",
			__func__);
		return -ENODEV;
	}

	if (data_length != OV7736_8BIT && data_length != OV7736_16BIT
					 && data_length != OV7736_32BIT) {
		dev_err(&client->dev, "%s error, invalid data length\n",
			__func__);
		return -EINVAL;
	}

	memset(msg, 0 , sizeof(msg));

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = MSG_LEN_OFFSET;
	msg[0].buf = data;

	/* high byte goes out first */
	data[0] = (u8)(reg >> 8);
	data[1] = (u8)(reg & 0xff);

	msg[1].addr = client->addr;
	msg[1].len = data_length;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = data;

	err = i2c_transfer(client->adapter, msg, 2);
	if (err < 0) {
		dev_err(&client->dev,
			"read from offset 0x%x error %d", reg, err);
		return err;
	}

	*val = 0;
	/* high byte comes first */
	if (data_length == OV7736_8BIT)
		*val = (u8)data[0];
	else if (data_length == OV7736_16BIT)
		*val = be16_to_cpu(*(u16 *)&data[0]);
	else
		*val = be32_to_cpu(*(u32 *)&data[0]);

	return 0;
}

static int
ov7736_write_reg(struct i2c_client *client, u16 data_length, u16 reg, u32 val)
{
	int num_msg;
	struct i2c_msg msg;
	unsigned char data[6] = {0};
	u16 *wreg;
	int retry = 0;

	if (!client->adapter) {
		dev_err(&client->dev, "%s error, no client->adapter\n",
			__func__);
		return -ENODEV;
	}

	if (data_length != OV7736_8BIT && data_length != OV7736_16BIT
					 && data_length != OV7736_32BIT) {
		dev_err(&client->dev, "%s error, invalid data_length\n",
			__func__);
		return -EINVAL;
	}

	memset(&msg, 0, sizeof(msg));

again:
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 2 + data_length;
	msg.buf = data;

	/* high byte goes out first */
	wreg = (u16 *)data;
	*wreg = cpu_to_be16(reg);

	if (data_length == OV7736_8BIT) {
		data[2] = (u8)(val);
	} else if (data_length == OV7736_16BIT) {
		u16 *wdata = (u16 *)&data[2];
		*wdata = cpu_to_be16((u16)val);
	} else {
		u32 *wdata = (u32 *)&data[2];
		*wdata = cpu_to_be32(val);
	}

	num_msg = i2c_transfer(client->adapter, &msg, 1);

	/*
	 * It is said that Rev 2 sensor needs some delay here otherwise
	 * registers do not seem to load correctly. But tests show that
	 * removing the delay would not cause any in-stablility issue and the
	 * delay will cause serious performance down, so, removed previous
	 * mdelay(1) here.
	 */

	if (num_msg >= 0)
		return 0;

	dev_err(&client->dev, "write error: wrote 0x%x to offset 0x%x error %d",
		val, reg, num_msg);
	if (retry <= I2C_RETRY_COUNT) {
		dev_err(&client->dev, "retrying... %d", retry);
		retry++;
		msleep(20);
		goto again;
	}

	return num_msg;
}

/*
 * ov7736_write_reg_array - Initializes a list of MT9T111 registers
 * @client: i2c driver client structure
 * @reglist: list of registers to be written
 *
 * Initializes a list of MT9T111 registers. The list of registers is
 * terminated by OV7736_TOK_TERM.
 */
static int ov7736_write_reg_array(struct i2c_client *client,
			    const struct misensor_reg *reglist)
{
	const struct misensor_reg *next = reglist;
	int err;
	u32 tmp_val;
	u32 targetVal;
	u16 bitLen = OV7736_8BIT;

	for (; next->length != OV7736_TOK_TERM; next++) {
		if (next->length == OV7736_TOK_DELAY) {
			msleep(next->val);
		} else {
			targetVal = next->val;
			bitLen = (next->length & OV7736_8BIT) ? (OV7736_8BIT) :
						((next->length & OV7736_16BIT) ? OV7736_16BIT : OV7736_32BIT);

			if (next->length & OV7736_RMW_AND) {
				err = ov7736_read_reg(client, bitLen, next->reg, &tmp_val);
				if (err) {
					dev_err(&client->dev, "%s err. read %0x aborted\n", __func__, next->reg);
					continue;
				}

				targetVal = tmp_val & next->val;
			}

			if (next->length & OV7736_RMW_OR) {
				err = ov7736_read_reg(client, bitLen, next->reg, &tmp_val);
				if (err) {
					dev_err(&client->dev, "%s err. read %0x aborted\n", __func__, next->reg);
					continue;
				}

				targetVal = tmp_val | next->val;
			}

			err = ov7736_write_reg(client, bitLen, next->reg, targetVal);

			/* REVISIT: Do we need this delay? */
			udelay(10);

			if (err) {
				dev_err(&client->dev, "%s err. write %0x aborted\n", __func__, next->reg);
				continue;
			}
		}
	}

	return 0;
}

/* *****Horizontal mirror the image.***** */
static int ov7736_g_hflip(struct v4l2_subdev *sd, s32 * val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	int err;
	u32 tmp;

	err = ov7736_read_reg(client, OV7736_8BIT, 0x3818, &tmp);
	if (!err) {
		if (tmp & 0x40)
			*val = 1;
		else
			*val = 0;
	}

	return err;
}

/* *****Original image.***** */
static int ov7736_g_vflip(struct v4l2_subdev *sd, s32 * val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	int err;
	u32 tmp;

	err = ov7736_read_reg(client, OV7736_8BIT, 0x3818, &tmp);
	if (!err) {
		if (tmp & 0x20)
			*val = 1;
		else
			*val = 0;
	}

	return err;
}

/* *****Vertical flip the image.***** */
static int ov7736_t_hflip(struct v4l2_subdev *sd, int value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	hflag = value;
	if (value)
		ov7736_write_reg_array(client, ov7736_hflip_on_table);
	else
		ov7736_write_reg_array(client, ov7736_hflip_off_table);

	return 0;
}

/* *****Vertically flip and Horizontal mirror the image.**** */
static int ov7736_t_vflip(struct v4l2_subdev *sd, int value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	vflag = value;
	if (value)
		ov7736_write_reg_array(client, ov7736_vflip_on_table);
	else
		ov7736_write_reg_array(client, ov7736_vflip_off_table);

	return 0;
}

static int ov7736_s_freq(struct v4l2_subdev *sd, int value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	
	switch (value) {
		case V4L2_CID_POWER_LINE_FREQUENCY_AUTO:
				ov7736_write_reg(client, OV7736_8BIT,0x3c01,0x32);
				break;
		case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
				ov7736_write_reg(client, OV7736_8BIT,0x3c01,0xb2);
				ov7736_write_reg(client, OV7736_8BIT,0x3c00,0x04);// manual 50hz
				break;
		case V4L2_CID_POWER_LINE_FREQUENCY_60HZ:
				ov7736_write_reg(client, OV7736_8BIT,0x3c01,0xb2);
				ov7736_write_reg(client, OV7736_8BIT,0x3c00,0x00);// manual 60hz
				break;
		default:
			printk("ov7736_s_freq: %d\n", value);
		break;
	}
	return 0;
}

static int ov7736_g_scene(struct v4l2_subdev *sd, s32 *value)
{
	return 0;
}

static int ov7736_s_scene(struct v4l2_subdev *sd, int value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	int ret=0;
	int old_index = scene_index;
	ov7736_debug(&client->dev, "DEBUG@%s:val:%d\n", __func__, value);
	switch (value) {
		case V4L2_SCENE_MODE_NONE:
			scene_index = V4L2_SCENE_MODE_NONE;
			if (old_index != scene_index)
				need_init = true;
			ret = ov7736_write_reg_array(client, ov7736_night_mode_off);
			//ret |= ov7736_write_reg_array(client, ov7736_vga_init);
			break;
		case V4L2_SCENE_MODE_SUNSET:
			//ret = ov7736_write_reg_array(client, ov7736_sunny_scene);
			break;
		case V4L2_SCENE_MODE_CANDLE_LIGHT:
			//ret = ov7736_write_reg_array(client, ov7736_home_scene);
			break;
		case V4L2_SCENE_MODE_DAWN_DUSK:
			//ret = ov7736_write_reg_array(client, ov7736_cloudy_scene);
			break;
		case V4L2_SCENE_MODE_PARTY_INDOOR:
			//ret = ov7736_write_reg_array(client, ov7736_office_scene);
			break;
		case V4L2_SCENE_MODE_FALL_COLORS:		
		case V4L2_SCENE_MODE_FIREWORKS:
		case V4L2_SCENE_MODE_LANDSCAPE:
			break;
		case V4L2_SCENE_MODE_NIGHT:
			scene_index = V4L2_SCENE_MODE_NIGHT;
			ret = ov7736_write_reg_array(client, ov7736_night_mode_on);
			break;
		case V4L2_SCENE_MODE_PORTRAIT:
		case V4L2_SCENE_MODE_SPORTS:
		case V4L2_SCENE_MODE_BACKLIGHT:
		case V4L2_SCENE_MODE_BEACH_SNOW:
		case V4L2_SCENE_MODE_TEXT:

    	default:
			printk("ov7736_s_scene: %d\n", value);
			break;
	}
	return 0;
}

int ov7736_g_focal_absolute(struct v4l2_subdev *sd, s32 *val)
{
	struct ov7736_device *dev = to_ov7736_sensor(sd);

	*val = (OV7736_FOCAL_LENGTH_NUM << 16) | OV7736_FOCAL_LENGTH_DEM;

	return 0;
}

int ov7736_g_fnumber(struct v4l2_subdev *sd, s32 *val)
{
	struct ov7736_device *dev = to_ov7736_sensor(sd);

	*val = (OV7736_F_NUMBER_DEFAULT_NUM << 16) | OV7736_F_NUMBER_DEM;
	return 0;
}

int ov7736_g_fnumber_range(struct v4l2_subdev *sd, s32 *val)
{
	struct ov7736_device *dev = to_ov7736_sensor(sd);

	*val = (OV7736_F_NUMBER_DEFAULT_NUM << 24) |
		(OV7736_F_NUMBER_DEM << 16) |
		(OV7736_F_NUMBER_DEFAULT_NUM << 8) | OV7736_F_NUMBER_DEM;
	return 0;
}

static int ov7736_g_wb(struct v4l2_subdev *sd, s32 *value)
{
	*value = awb_index;
	return 0;
}

static int ov7736_s_wb(struct v4l2_subdev *sd, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	int ret=0;
	ov7736_debug(&client->dev, "DEBUG@%s:val:%d\n", __func__, value);

	switch (value) {
		case V4L2_WHITE_BALANCE_MANUAL:
			awb_index = V4L2_WHITE_BALANCE_MANUAL;
			break;
		case V4L2_WHITE_BALANCE_AUTO:
			awb_index = V4L2_WHITE_BALANCE_AUTO;
			ov7736_write_reg_array(client, ov7736_auto_wb);
			break;
		case V4L2_WHITE_BALANCE_INCANDESCENT:
			awb_index = V4L2_WHITE_BALANCE_INCANDESCENT;
			ov7736_write_reg_array(client, ov7736_home_wb);
			break;
		case V4L2_WHITE_BALANCE_FLUORESCENT:
			awb_index = V4L2_WHITE_BALANCE_FLUORESCENT;
			ov7736_write_reg_array(client, ov7736_office_wb);
			break;
		case V4L2_WHITE_BALANCE_FLUORESCENT_H:
			awb_index = V4L2_WHITE_BALANCE_FLUORESCENT_H;
			break;
		case V4L2_WHITE_BALANCE_HORIZON:
			awb_index = V4L2_WHITE_BALANCE_HORIZON;
			break;
		case V4L2_WHITE_BALANCE_DAYLIGHT:
			awb_index = V4L2_WHITE_BALANCE_DAYLIGHT;
			ov7736_write_reg_array(client, ov7736_sunny_wb);
			break;
		case V4L2_WHITE_BALANCE_FLASH:
			awb_index = V4L2_WHITE_BALANCE_FLASH;
			break;
		case V4L2_WHITE_BALANCE_CLOUDY:
			awb_index = V4L2_WHITE_BALANCE_CLOUDY;
			ov7736_write_reg_array(client, ov7736_cloudy_wb);
			break;
		case V4L2_WHITE_BALANCE_SHADE:
			awb_index = V4L2_WHITE_BALANCE_SHADE;
			break;
		default:
			printk("ov7736_s_wb: %d\n", value);
		break;
	}
	return 0;
}


static int ov7736_g_effect(struct v4l2_subdev *sd, s32 *value)
{
	return 0;
}

static int ov7736_s_effect(struct v4l2_subdev *sd, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	int ret=0;

	printk("\n%s",__func__);
	
	switch (value) {
		case V4L2_COLORFX_NONE:
				ret = ov7736_write_reg_array(client, ov7736_normal_effect);
		        break;
		case V4L2_COLORFX_BW:
				ret = ov7736_write_reg_array(client, ov7736_BW_effect);
		        break;
		case V4L2_COLORFX_SKY_BLUE:
				ret = ov7736_write_reg_array(client, ov7736_Bluish_effect);
		        break;
		case V4L2_COLORFX_SEPIA:
				ret = ov7736_write_reg_array(client, ov7736_Sepia_effect);
		        break;
		case V4L2_COLORFX_ANTIQUE:
				ret = ov7736_write_reg_array(client, ov7736_Redish_effect);
		        break;
		case V4L2_COLORFX_GRASS_GREEN:
				ret = ov7736_write_reg_array(client, ov7736_Greenish_effect);
		        break;
		case V4L2_COLORFX_NEGATIVE:
				ret = ov7736_write_reg_array(client, ov7736_Negative_effect);
		        break;		
		default:
			printk("ov7736_s_wb: %d\n", value);
		break;
	}
	return 0;
}


static int ov7736_g_exposure(struct v4l2_subdev *sd, s32 *value)
{
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u32 coarse;
	u32 reg_val_h, reg_val_m, reg_val_l;
	int ret;

	ret = ov7736_read_reg(client, OV7736_8BIT, OV7736_REG_EXPOSURE_0, &reg_val_h);
	if (ret)
		return ret;
	coarse = ((u32)(reg_val_h & 0x0000000f)) << 16;

	ret = ov7736_read_reg(client, OV7736_8BIT, OV7736_REG_EXPOSURE_1, &reg_val_m);
	if (ret)
		return ret;
	coarse |= ((u32)(reg_val_m & 0x000000ff)) << 8;

	ret = ov7736_read_reg(client, OV7736_8BIT, OV7736_REG_EXPOSURE_2, &reg_val_l);
	if (ret)
		return ret;
	coarse |= ((u32)(reg_val_l & 0x000000ff));
	/*WA: max exposure time is 1/30s for 30fps, max coarse line is 7200.
	match expression: 7200 * 10 / 72 /3 / 10000(denominator in hal) = 1/30s*/
	coarse = coarse * 10 / 72 / 3;
	ov7736_debug(&client->dev, "@%s: exp_h:0x%x, exp_m:0x%x, exp_l:0x%x, coarse:%d\n",
		__func__, reg_val_h, reg_val_m, reg_val_l, coarse);
	*value = coarse;
	return 0;
}

static int ov7736_s_exposure(struct v4l2_subdev *sd, int value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	int ret;

	exposure_index = value;
	switch (value) {
		case -2:
			ret = ov7736_write_reg_array(client, ov7736_exposure_negative2);
			break;
		case -1:
			ret = ov7736_write_reg_array(client, ov7736_exposure_negative1);
			break;
		case 0:
			ret = ov7736_write_reg_array(client, ov7736_exposure_negative0);
			break;
		case 1:
			ret = ov7736_write_reg_array(client, ov7736_exposure_positive1);
			break;
		case 2:
			ret = ov7736_write_reg_array(client, ov7736_exposure_positive2);
			break;
	}
	return 0;
}

static int ov7736_standby(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return ov7736_write_reg_array(client, ov7736_standby_reg);
}

static int ov7736_wakeup(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return ov7736_write_reg_array(client, ov7736_wakeup_reg);
}

static int ov7736_reset_ctrls(struct v4l2_subdev *sd)
{
	int ret = 0;
	ret = ov7736_t_vflip(sd, vflag);
	ret |= ov7736_t_hflip(sd, hflag);
	ret |= ov7736_s_scene(sd, scene_index);
	ret |= ov7736_s_wb(sd, awb_index);
	ret |= ov7736_s_exposure(sd, exposure_index);

	return ret;
}
static int ov7736_init(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	int ret;

	
	//ret = ov7736_write_reg_array(client, ov7736_vga_init);
	ret = ov7736_write_reg_array(client, ov7736_basic_init);
	//dev->res = OV7736_RES_VGA;
	
	if (ret)
		return ret;

	/*
	 * delay 5ms to wait for sensor initialization finish.
	 */
	usleep_range(5000, 6000);

	return 0;
	
	//return ov7736_standby(sd);;
}

static int power_up(struct v4l2_subdev *sd)
{
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;


	if (NULL == dev->platform_data) {
		dev_err(&client->dev, "no camera_sensor_platform_data");
		return -ENODEV;
	}
	/* power control */
	ret = dev->platform_data->power_ctrl(sd, 1);
	if (ret)
		goto fail_power;

	/* according to DS, at least 5ms is needed between DOVDD and PWDN */
	usleep_range(5000, 6000);

	/* gpio ctrl */
	ret = dev->platform_data->gpio_ctrl(sd, 1);
	if (ret)
		goto fail_power;

	/* flis clock control */
	ret = dev->platform_data->flisclk_ctrl(sd, 1);
	if (ret)
		goto fail_clk;


	/*
	 * according to DS, 20ms is needed between power up and first i2c
	 * commend
	 */
	msleep(20);
	return 0;

fail_clk:
	dev->platform_data->flisclk_ctrl(sd, 0);
fail_power:
	dev->platform_data->power_ctrl(sd, 0);
	dev_err(&client->dev, "sensor power-up failed\n");

	return ret;
}

static int power_down(struct v4l2_subdev *sd)
{
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;


	mbus_fmt_called = false;
	if (NULL == dev->platform_data) {
		dev_err(&client->dev, "no camera_sensor_platform_data");
		return -ENODEV;
	}

	ret = dev->platform_data->flisclk_ctrl(sd, 0);
	if (ret)
		dev_err(&client->dev, "flisclk failed\n");

	/* gpio ctrl */
	ret = dev->platform_data->gpio_ctrl(sd, 0);
	if (ret)
		dev_err(&client->dev, "gpio failed\n");

	/* power control */
	ret = dev->platform_data->power_ctrl(sd, 0);
	if (ret)
		dev_err(&client->dev, "vprog failed.\n");

	/*according to DS, 20ms is needed after power down*/
	msleep(20);
	return ret;
}

static int ov7736_s_power(struct v4l2_subdev *sd, int on)
{
    int ret;
	if (on == 0) {
		ret = power_down(sd);
	} else {
		ret = power_up(sd);
		if (!ret)
			return ov7736_init(sd);
	}
	return ret;
}


/*
 * distance - calculate the distance
 * @res: resolution
 * @w: width
 * @h: height
 *
 * Get the gap between resolution and w/h.
 * res->width/height smaller than w/h wouldn't be considered.
 * Returns the value of gap or -1 if fail.
 */
#define LARGEST_ALLOWED_RATIO_MISMATCH 800
static int distance(struct ov7736_res_struct *res, u32 w, u32 h)
{
	unsigned int w_ratio = ((res->width << 13)/w);
	unsigned int h_ratio;
	int match;

	if (h == 0)
		return -1;
	h_ratio = ((res->height << 13) / h);
	if (h_ratio == 0)
		return -1;
	match   = abs(((w_ratio << 13) / h_ratio) - ((int)8192));

	if ((w_ratio < (int)8192) || (h_ratio < (int)8192)  ||
		(match > LARGEST_ALLOWED_RATIO_MISMATCH))
		return -1;

	return w_ratio + h_ratio;
}

/* Return the nearest higher resolution index */
static int nearest_resolution_index(int w, int h)
{
	int i;
	int idx = -1;
	int dist;
	int min_dist = INT_MAX;
	struct ov7736_res_struct *tmp_res = NULL;

	for (i = 0; i < N_RES; i++) {
		tmp_res = &ov7736_res[i];
		dist = distance(tmp_res, w, h);
		if (dist == -1)
			continue;
		if (dist < min_dist) {
			min_dist = dist;
			idx = i;
		}
	}

	return idx;
}

static int get_resolution_index(int w, int h)
{
	int i;

	for (i = 0; i < N_RES; i++) {
		if (w != ov7736_res[i].width)
			continue;
		if (h != ov7736_res[i].height)
			continue;

		return i;
	}

	return -1;
}

static int ov7736_try_res(u32 *w, u32 *h)
{
	int i;

	/*
	 * The mode list is in ascending order. We're done as soon as
	 * we have found the first equal or bigger size.
	 */
	for (i = 0; i < N_RES; i++) {
		if ((ov7736_res[i].width >= *w) &&
		    (ov7736_res[i].height >= *h))
			break;
	}

	/*
	 * If no mode was found, it means we can provide only a smaller size.
	 * Returning the biggest one available in this case.
	 */
	if (i == N_RES)
		i--;

	*w = ov7736_res[i].width;
	*h = ov7736_res[i].height;

	return 0;
}

static struct ov7736_res_struct *ov7736_to_res(u32 w, u32 h)
{
	int  index;

	for (index = 0; index < N_RES; index++) {
		if (ov7736_res[index].width == w &&
		    ov7736_res[index].height == h)
			break;
	}

	/* No mode found */
	if (index >= N_RES)
		return NULL;

	return &ov7736_res[index];
}

static int ov7736_try_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	return ov7736_try_res(&fmt->width, &fmt->height);
}

static int ov7736_res2size(unsigned int res, int *h_size, int *v_size)
{
	unsigned short hsize;
	unsigned short vsize;

	switch (res) {
	case OV7736_RES_VGA:
		hsize = OV7736_RES_VGA_SIZE_H;
		vsize = OV7736_RES_VGA_SIZE_V;
		break;
	case OV7736_RES_QVGA:
		hsize = OV7736_RES_QVGA_SIZE_H;
		vsize = OV7736_RES_QVGA_SIZE_V;
		break;
	default:
		WARN(1, "%s: Resolution 0x%08x unknown\n", __func__, res);
		return -EINVAL;
	}

	if (h_size != NULL)
		*h_size = hsize;
	if (v_size != NULL)
		*v_size = vsize;

	return 0;
}

static int ov7736_g_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	int width, height;
	int ret;
	fmt->code = V4L2_MBUS_FMT_UYVY8_1X16;
	ret = ov7736_res2size(dev->res, &width, &height);
	if (ret)
		return ret;
	fmt->width = width;
	fmt->height = height;

	return 0;
}

static int ov7736_s_mbus_fmt(struct v4l2_subdev *sd,
			      struct v4l2_mbus_framefmt *fmt)
{
	struct i2c_client *c = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	struct ov7736_res_struct *res_index;
	struct camera_mipi_info *mipi_info = NULL;
	u32 width = fmt->width;
	u32 height = fmt->height;
	int ret;
	ov7736_debug(&c->dev, "DEBUG@%s:\n", __func__);
	mipi_info = v4l2_get_subdev_hostdata(sd);
	if (mipi_info == NULL) {
		dev_err(&c->dev, "%s: can not find mipi info!!!\n", __func__);
		return -EINVAL;
	}

	ov7736_try_res(&width, &height);
	res_index = ov7736_to_res(width, height);

	/* Sanity check */
	if (unlikely(!res_index)) {
		WARN_ON(1);
		return -EINVAL;
	}

	printk("ov7736_s_mbus_fmt res %d, w%d, h%d\n",
		res_index->res, res_index->width, res_index->height);
	switch(res_index->res) {
		case OV7736_RES_VGA:
			if (need_init) {
				ret = ov7736_write_reg_array(c, ov7736_basic_init);
				need_init = false;
			}
			//break;
		case OV7736_RES_QVGA:
			//if (dev->res != res_index->res)
				ret = ov7736_write_reg_array(c, ov7736_vga_init);
			break;
		default:
			dev_err(&c->dev, "%s: can not support the resolution!!!\n", __func__);
	}


	if (dev->res != res_index->res) {
		int index;
		/*
		 * Marked current sensor res as being "used"
		 *
		 * REVISIT: We don't need to use an "used" field on each mode
		 * list entry to know which mode is selected. If this
		 * information is really necessary, how about to use a single
		 * variable on sensor dev struct?
		 */
		for (index = 0; index < N_RES; index++) {
			if (width == ov7736_res[index].width &&
			    height == ov7736_res[index].height) {
				ov7736_res[index].used = 1;
				continue;
			}
			ov7736_res[index].used = 0;
		}
	}

	/*
	 * ov7736 - we don't poll for context switch
	 * because it does not happen with streaming disabled.
	 */
	dev->res = res_index->res;
	fmt->width = width;
	fmt->height = height;
	ov7736_reset_ctrls(sd);
	mbus_fmt_called = true;
	return 0;
	
	//return ov7736_wakeup(sd);
	
}

static int ov7736_detect(struct i2c_client *client,  u16 *id, u8 *revision)
{
	struct i2c_adapter *adapter = client->adapter;
	u32 high, low;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -ENODEV;

	ret = ov7736_read_reg(client, OV7736_8BIT, OV7736_SC_CMMN_CHIP_ID_H, &high);
	if (ret) {
		dev_err(&client->dev, "sensor_id_high = 0x%x\n", high);
		return -ENODEV;
	}
	ret = ov7736_read_reg(client, OV7736_8BIT, OV7736_SC_CMMN_CHIP_ID_L, &low);
	*id = ((((u16) high) << 8) | (u16) low);

	if (*id != OV7736_ID)  {
		dev_err(&client->dev, "sensor ID error: high:0x%x low:0x%x\n",high,low);
		return -ENODEV;
	}

	ret = ov7736_read_reg(client, OV7736_8BIT, OV7736_SC_CMMN_SUB_ID, &high);
	*revision = (u8) high & 0x0f;

    ov7736_debug(&client->dev, "sensor_revision id  = 0x%x\n", *id);
	ov7736_debug(&client->dev, "detect ov7736 success\n");
	ov7736_debug(&client->dev, "################6##########\n");

	return 0;
}

static int
ov7736_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 sensor_revision;
	u16 sensor_id = 0;
	int ret;

	if (NULL == platform_data)
		return -ENODEV;

	dev->platform_data =
	    (struct camera_sensor_platform_data *)platform_data;

	if (dev->platform_data->platform_init) {
		ret = dev->platform_data->platform_init(client);
		if (ret) {
			dev_err(&client->dev, "platform init err\n");
			return -ENODEV;
		}
	}
	
	ret = power_up(sd);
	if (ret) {
		dev_err(&client->dev, "power_ctrl failed");
		goto fail_detect;
	}

	/* config & detect sensor */
	ret = ov7736_detect(client, &sensor_id, &sensor_revision);
	if (ret) {
		dev_err(&client->dev, "ov7736_detect err s_config.\n");
		goto fail_detect;
	}

	dev->sensor_id = sensor_id;
	dev->sensor_revision = sensor_revision;

	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret)
		goto fail_csi_cfg;

	ret = power_down(sd);
	if (ret)
		dev_err(&client->dev, "sensor power-gating failed\n");

	return ret;

fail_csi_cfg:
	dev->platform_data->csi_cfg(sd, 0);
fail_detect:
	power_down(sd);
	dev_err(&client->dev, "sensor power-gating failed\n");
	return ret;
}

static struct ov7736_control ov7736_controls[] = {
	{
		.qc = {
			.id = V4L2_CID_EXPOSURE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "exposure",
			.minimum = 0x0,
			.maximum = 0xffff,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = ov7736_g_exposure,
		.tweak = ov7736_s_exposure,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCAL_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focal length",
			.minimum = OV7736_FOCAL_LENGTH_DEFAULT,
			.maximum = OV7736_FOCAL_LENGTH_DEFAULT,
			.step = 0x01,
			.default_value = OV7736_FOCAL_LENGTH_DEFAULT,
			.flags = 0,
		},
		.query = ov7736_g_focal_absolute,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number",
			.minimum = OV7736_F_NUMBER_DEFAULT,
			.maximum = OV7736_F_NUMBER_DEFAULT,
			.step = 0x01,
			.default_value = OV7736_F_NUMBER_DEFAULT,
			.flags = 0,
		},
		.query = ov7736_g_fnumber,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_RANGE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number range",
			.minimum = OV7736_F_NUMBER_RANGE,
			.maximum =  OV7736_F_NUMBER_RANGE,
			.step = 0x01,
			.default_value = OV7736_F_NUMBER_RANGE,
			.flags = 0,
		},
		.query = ov7736_g_fnumber_range,
	},
	{
		.qc = {
			.id = V4L2_CID_EXPOSURE_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "exposure",
			.minimum = -2,
			.maximum = 2,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = ov7736_g_exposure,
		.tweak = ov7736_s_exposure,
	},
	{
		.qc = {
			.id = V4L2_CID_SCENE_MODE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Scene Mode",
			.minimum = V4L2_SCENE_MODE_NONE,
			.maximum = V4L2_SCENE_MODE_TEXT,
			.step = 1,
			.default_value = V4L2_SCENE_MODE_NONE,
			.flags = 0,
		},
		.query = ov7736_g_scene,
		.tweak = ov7736_s_scene,
	},
	{
		.qc = {
			.id = V4L2_CID_POWER_LINE_FREQUENCY,
			.type = V4L2_CTRL_TYPE_MENU,
			.name = "Light frequency filter",
			.minimum = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED,
			.maximum =	V4L2_CID_POWER_LINE_FREQUENCY_AUTO, /* 1: 50Hz, 2:60Hz */
			.step = 1,
			.default_value = V4L2_CID_POWER_LINE_FREQUENCY_AUTO,
			.flags = 0,
		},
		.tweak = ov7736_s_freq,
	},
	{
		.qc = {
			.id = V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "White Balance",
			.minimum = V4L2_WHITE_BALANCE_MANUAL,
			.maximum = V4L2_WHITE_BALANCE_SHADE,
			.step = 1,
			.default_value = V4L2_WHITE_BALANCE_AUTO,
			.flags = 0,
		},
		.query = ov7736_g_wb,
		.tweak = ov7736_s_wb,
	},
	{
		.qc = {
			.id = V4L2_CID_VFLIP,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Image v-Flip",
			.minimum = 0,
			.maximum = 1,
			.step = 1,
			.default_value = 0,
		},
		.query = ov7736_g_vflip,
		.tweak = ov7736_t_vflip,
	},
	{
		.qc = {
			.id = V4L2_CID_HFLIP,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Image h-Flip",
			.minimum = 0,
			.maximum = 1,
			.step = 1,
			.default_value = 0,
		},
		.query = ov7736_g_hflip,
		.tweak = ov7736_t_hflip,
	},
};
#define N_CONTROLS (ARRAY_SIZE(ov7736_controls))

static struct ov7736_control *ov7736_find_control(__u32 id)
{
	int i;

	for (i = 0; i < N_CONTROLS; i++) {
		if (ov7736_controls[i].qc.id == id)
			return &ov7736_controls[i];
	}
	return NULL;
}

static int ov7736_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	struct ov7736_control *ctrl = ov7736_find_control(qc->id);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (ctrl == NULL) {
		dev_err(&client->dev,  "%s unsupported control id!\n", __func__);
		return 0;
	}
	*qc = ctrl->qc;
	return 0;
}

static int ov7736_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ov7736_control *octrl = ov7736_find_control(ctrl->id);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	if (!octrl || !octrl->query) {
		dev_err(&client->dev,  "%s unsupported control id or no query func!\n",
			__func__);
		return -EINVAL;
	}
	ret = octrl->query(sd, &ctrl->value);
	return ret;
}

static int ov7736_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ov7736_control *octrl = ov7736_find_control(ctrl->id);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	if (!octrl || !octrl->tweak) {
		dev_err(&client->dev,  "%s unsupported control id or no tweak func!\n",
			__func__);
		return 0;
	}

	ret = octrl->tweak(sd, ctrl->value);
	return 0;
}

static int ov7736_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	int ret;

	ov7736_debug(&client->dev, "DEBUG@%s: enable:%d", __func__, enable);
	dev->streaming = enable;	

#if 0
	ret = ov7736_write_reg(client, OV7736_8BIT, OV7736_SW_STREAM,
				enable ? OV7736_START_STREAMING :
				OV7736_STOP_STREAMING);
#else
#if 1
	if (enable && !mbus_fmt_called) {
		ov7736_debug(&client->dev, "DEBUG@%s: RESET", __func__);
		ov7736_reset_ctrls(sd);
	}
#endif
	msleep(20);
	ret = ov7736_write_reg(client, OV7736_8BIT, 0x3003,enable ? 0x00 : 0x02);
	
	//ret = ov7736_write_reg(client, OV7736_8BIT, 0x3103,enable ? 0x02 : 0x06);

	//ret = ov7736_write_reg(client, OV7736_8BIT,OV7736_REG_FRAME_CTRL,
			//enable ? OV7736_FRAME_START : OV7736_FRAME_STOP);    

	//if(!enable)
		//ret = ov7736_standby(sd);

#endif

	return ret;
}


static int
ov7736_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
	unsigned int index = fsize->index;

	if (index >= N_RES)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = ov7736_res[index].width;
	fsize->discrete.height = ov7736_res[index].height;
	fsize->reserved[0] = ov7736_res[index].used;

	return 0;
}

static int ov7736_enum_frameintervals(struct v4l2_subdev *sd,
				       struct v4l2_frmivalenum *fival)
{
	unsigned int index = fival->index;
	int i;

	if (index >= N_RES)
		return -EINVAL;

	/* find out the first equal or bigger size */
	for (i = 0; i < N_RES; i++) {
		if (ov7736_res[i].width >= fival->width &&
		    ov7736_res[i].height >= fival->height)
			break;
	}
	if (i == N_RES)
		i--;

	index = i;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = ov7736_res[index].fps;

	return 0;
}

static int
ov7736_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_OV7736, 0);
}

static int ov7736_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_fh *fh,
				  struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index)
		return -EINVAL;
	code->code = V4L2_MBUS_FMT_UYVY8_1X16;

	return 0;
}

static int ov7736_enum_frame_size(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh,
	struct v4l2_subdev_frame_size_enum *fse)
{
	unsigned int index = fse->index;

	if (index >= N_RES)
		return -EINVAL;

	fse->min_width = ov7736_res[index].width;
	fse->min_height = ov7736_res[index].height;
	fse->max_width = ov7736_res[index].width;
	fse->max_height = ov7736_res[index].height;

	return 0;
}

static int
ov7736_get_pad_format(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		       struct v4l2_subdev_format *fmt)
{
	struct ov7736_device *snr = to_ov7736_sensor(sd);

	switch (fmt->which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		fmt->format = *v4l2_subdev_get_try_format(fh, fmt->pad);
		break;
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		fmt->format = snr->format;
	}

	return 0;
}

static int
ov7736_set_pad_format(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		       struct v4l2_subdev_format *fmt)
{
	struct ov7736_device *snr = to_ov7736_sensor(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		snr->format = fmt->format;

	return 0;
}

/* set focus zone */
static int
ov7736_set_selection(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			     struct v4l2_subdev_selection *sel)
{
	return 0;
}

static int ov7736_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!param)
		return -EINVAL;

	if (param->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		dev_err(&client->dev,  "unsupported buffer type.\n");
		return -EINVAL;
	}

	memset(param, 0, sizeof(*param));
	param->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	return 0;
}

static int ov7736_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct ov7736_device *dev = to_ov7736_sensor(sd);
	dev->run_mode = param->parm.capture.capturemode;

	printk("\n%s:run_mode :%x\n", __func__, dev->run_mode);
	
	return 0;
}

static int
ov7736_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
	int index;
	struct ov7736_device *snr = to_ov7736_sensor(sd);

	for (index = 0; index < N_RES; index++) {
		if (ov7736_res[index].res == snr->res) {
			*frames = ov7736_res[index].skip_frames;
			return 0;
		}
	}
	return -EINVAL;
}

static const struct v4l2_subdev_video_ops ov7736_video_ops = {
	.try_mbus_fmt = ov7736_try_mbus_fmt,
	.g_mbus_fmt = ov7736_g_mbus_fmt,
	.s_mbus_fmt = ov7736_s_mbus_fmt,
	.s_parm = ov7736_s_parm,
	.g_parm = ov7736_g_parm,
	.s_stream = ov7736_s_stream,
	.enum_framesizes = ov7736_enum_framesizes,
	.enum_frameintervals = ov7736_enum_frameintervals,
};

static const struct v4l2_subdev_sensor_ops ov7736_sensor_ops = {
	.g_skip_frames	= ov7736_g_skip_frames,
};

static const struct v4l2_subdev_core_ops ov7736_core_ops = {
	.g_chip_ident = ov7736_g_chip_ident,
	.queryctrl = ov7736_queryctrl,
	.g_ctrl = ov7736_g_ctrl,
	.s_ctrl = ov7736_s_ctrl,
	.s_power = ov7736_s_power,
};

static const struct v4l2_subdev_pad_ops ov7736_pad_ops = {
	.enum_mbus_code = ov7736_enum_mbus_code,
	.enum_frame_size = ov7736_enum_frame_size,
	.get_fmt = ov7736_get_pad_format,
	.set_fmt = ov7736_set_pad_format,
	.set_selection = ov7736_set_selection,
};

static const struct v4l2_subdev_ops ov7736_ops = {
	.core = &ov7736_core_ops,
	.video = &ov7736_video_ops,
	.sensor = &ov7736_sensor_ops,
	.pad = &ov7736_pad_ops,
};

static const struct media_entity_operations ov7736_entity_ops;

static int ov7736_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov7736_device *dev = container_of(sd,
					struct ov7736_device, sd);

	dev->platform_data->csi_cfg(sd, 0);

	release_firmware(dev->firmware);
	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	kfree(dev);

	return 0;
}

static int ov7736_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct ov7736_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&client->dev, "out of memory\n");
		return -ENOMEM;
	}

	v4l2_i2c_subdev_init(&(dev->sd), client, &ov7736_ops);
	if (client->dev.platform_data) {
		ret = ov7736_s_config(&dev->sd, client->irq,
				       client->dev.platform_data);
		if (ret)
			goto out_free;
	}

	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dev->pad.flags = MEDIA_PAD_FL_SOURCE;
	dev->format.code = V4L2_MBUS_FMT_UYVY8_1X16;
	dev->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;

	ret = media_entity_init(&dev->sd.entity, 1, &dev->pad, 0);
	if (ret)
		ov7736_remove(client);

	return ret;
out_free:
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);
	return ret;
}

MODULE_DEVICE_TABLE(i2c, ov7736_id);
static struct i2c_driver ov7736_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = OV7736_NAME,
	},
	.probe = ov7736_probe,
	.remove = __exit_p(ov7736_remove),
	.id_table = ov7736_id,
};

static __init int ov7736_init_mod(void)
{
	return i2c_add_driver(&ov7736_driver);
}

static __exit void ov7736_exit_mod(void)
{
	i2c_del_driver(&ov7736_driver);
}

module_init(ov7736_init_mod);
module_exit(ov7736_exit_mod);

MODULE_DESCRIPTION("A low-level driver for Omnivision OV7736 sensors");
MODULE_LICENSE("GPL");
