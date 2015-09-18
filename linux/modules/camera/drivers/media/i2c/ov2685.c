/*
 * Support for ov2685 Camera Sensor.
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

#include "ov2685.h"

#define to_ov2685_sensor(sd) container_of(sd, struct ov2685_device, sd)

static int  v_flag=0;
static int  h_flag=0;
static int  test_num=7777;

static int
ov2685_read_reg(struct i2c_client *client, u16 data_length, u16 reg, u32 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[4];

	if (!client->adapter) {
		dev_err(&client->dev, "%s error, no client->adapter\n",
			__func__);
		return -ENODEV;
	}

	if (data_length != MISENSOR_8BIT && data_length != MISENSOR_16BIT
					 && data_length != MISENSOR_32BIT) {
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
	if (data_length == MISENSOR_8BIT)
		*val = (u8)data[0];
	else if (data_length == MISENSOR_16BIT)
		*val = be16_to_cpu(*(u16 *)&data[0]);
	else
		*val = be32_to_cpu(*(u32 *)&data[0]);

	return 0;
}

static int
ov2685_write_reg(struct i2c_client *client, u16 data_length, u16 reg, u32 val)
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

	if (data_length != MISENSOR_8BIT && data_length != MISENSOR_16BIT
					 && data_length != MISENSOR_32BIT) {
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

	if (data_length == MISENSOR_8BIT) {
		data[2] = (u8)(val);
	} else if (data_length == MISENSOR_16BIT) {
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
 * ov2685_write_reg_array - Initializes a list of MT9T111 registers
 * @client: i2c driver client structure
 * @reglist: list of registers to be written
 *
 * Initializes a list of MT9T111 registers. The list of registers is
 * terminated by MISENSOR_TOK_TERM.
 */
static int ov2685_write_reg_array(struct i2c_client *client,
			    const struct misensor_reg *reglist)
{
	const struct misensor_reg *next = reglist;
	int err;

	for (; next->length != MISENSOR_TOK_TERM; next++) {
		if (next->length == MISENSOR_TOK_DELAY) {
			msleep(next->val);
		} else {
			err = ov2685_write_reg(client, next->length, next->reg,
						next->val);
			/* REVISIT: Do we need this delay? */
			udelay(10);
			if (err) {
				dev_err(&client->dev, "%s err. aborted\n",
					__func__);
				return err;
			}
		}
	}

	return 0;
}

/* Horizontal flip the image. */
static int ov2685_g_hflip(struct v4l2_subdev *sd, s32 * val)
{
	return 0;
}

static int ov2685_g_vflip(struct v4l2_subdev *sd, s32 * val)
{
	return 0;
}

/* Horizontal flip the image. */
static int ov2685_t_hflip(struct v4l2_subdev *sd, int value)
{
	return 0;
}

/* Vertically flip the image */
static int ov2685_t_vflip(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int ov2685_s_freq(struct v4l2_subdev *sd, int value)
{
	switch (value) {
		case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED :
		case V4L2_CID_POWER_LINE_FREQUENCY_50HZ :
		case V4L2_CID_POWER_LINE_FREQUENCY_60HZ :
		case V4L2_CID_POWER_LINE_FREQUENCY_AUTO :
		default:
			printk("ov2685_s_freq: %d\n", value);
		break;
	}
	return 0;
}

static int ov2685_g_scene(struct v4l2_subdev *sd, s32 *value)
{
	return 0;
}

static int ov2685_s_scene(struct v4l2_subdev *sd, int value)
{
	switch (value) {
		case V4L2_SCENE_MODE_NONE		  :
		case V4L2_SCENE_MODE_BACKLIGHT    :
		case V4L2_SCENE_MODE_BEACH_SNOW   :
		case V4L2_SCENE_MODE_CANDLE_LIGHT :
		case V4L2_SCENE_MODE_DAWN_DUSK    :
		case V4L2_SCENE_MODE_FALL_COLORS  :
		case V4L2_SCENE_MODE_FIREWORKS    :
		case V4L2_SCENE_MODE_LANDSCAPE    :
		case V4L2_SCENE_MODE_NIGHT        :
		case V4L2_SCENE_MODE_PARTY_INDOOR :
		case V4L2_SCENE_MODE_PORTRAIT     :
		case V4L2_SCENE_MODE_SPORTS       :
		case V4L2_SCENE_MODE_SUNSET       :
		case V4L2_SCENE_MODE_TEXT         :
		default:
			printk("ov2685_s_scene: %d\n", value);
			break;
	}
	return 0;
}

static int ov2685_g_wb(struct v4l2_subdev *sd, s32 *value)
{
	return 0;
}

static int ov2685_s_wb(struct v4l2_subdev *sd, int value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	printk("%s: %d\n",__func__, value);
	switch (value) {
		case V4L2_WHITE_BALANCE_AUTO	        :
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0x00);//start group 1
			ov2685_write_reg(client, MISENSOR_8BIT,0x5180,0xf4);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0x10);//end group 1
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0xa0);//launch group 1
			break;

		case V4L2_WHITE_BALANCE_MANUAL			:
		case V4L2_WHITE_BALANCE_INCANDESCENT	:
			//	Sunny
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0x00);// start group 1
			ov2685_write_reg(client, MISENSOR_8BIT,0x5180,0xf6);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5195,0x07);//R Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x5196,0x9c);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5197,0x04);//G Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x5198,0x00);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5199,0x05);//B Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x519a,0xf3);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0x10);// end group 1
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0xa0);// launch group 1
			break;
		case V4L2_WHITE_BALANCE_FLUORESCENT		:
			//		Home
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0x00);// start group 1
			ov2685_write_reg(client, MISENSOR_8BIT,0x5180,0xf6);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5195,0x04);//R Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x5196,0x90);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5197,0x04);//G Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x5198,0x00);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5199,0x09);//B Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x519a,0x20);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0x10);// end group 1
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0xa0);// launch group 1
			break;
		case V4L2_WHITE_BALANCE_FLUORESCENT_H :
		case V4L2_WHITE_BALANCE_HORIZON:
		case V4L2_WHITE_BALANCE_DAYLIGHT:
			//Office
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0x00);// start group 1
			ov2685_write_reg(client, MISENSOR_8BIT,0x5180,0xf6);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5195,0x06);//R Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x5196,0xb8);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5197,0x04);//G Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x5198,0x00);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5199,0x06);//B Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x519a,0x5f);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0x10);// end group 1
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0xa0);// launch group 1
			break;
		case V4L2_WHITE_BALANCE_FLASH:
		case V4L2_WHITE_BALANCE_CLOUDY:
			//Cloudy
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0x00);// start group 1
			ov2685_write_reg(client, MISENSOR_8BIT,0x5180,0xf6);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5195,0x07);//R Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x5196,0xdc);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5197,0x04);//G Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x5198,0x00);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x5199,0x05);//B Gain
			ov2685_write_reg(client, MISENSOR_8BIT,0x519a,0xd3);//
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0x10);// end group 1
			ov2685_write_reg(client, MISENSOR_8BIT,0x3208,0xa0);// launch group 1
			break;
		case V4L2_WHITE_BALANCE_SHADE:
		default:
			printk("ov2685_s_wb: %d\n", value);
		break;
	}
	return 0;
}

static int ov2685_g_exposure(struct v4l2_subdev *sd, s32 *value)
{
	return 0;
}

static int ov2685_s_exposure(struct v4l2_subdev *sd, int value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	switch (value) {
		case -2:
			ov2685_write_reg(client, MISENSOR_8BIT,0x3a03, 0x3a);
			ov2685_write_reg(client, MISENSOR_8BIT,0x3a04, 0x30);
			break;
		case -1:
			ov2685_write_reg(client, MISENSOR_8BIT,0x3a03, 0x42);
			ov2685_write_reg(client, MISENSOR_8BIT,0x3a04, 0x38);
			break;
		case 0:
			ov2685_write_reg(client, MISENSOR_8BIT,0x3a03, 0x4e);
			ov2685_write_reg(client, MISENSOR_8BIT,0x3a04, 0x40);
			break;
		case 1:
			ov2685_write_reg(client, MISENSOR_8BIT,0x3a03, 0x52);
			ov2685_write_reg(client, MISENSOR_8BIT,0x3a04, 0x48);
			break;

		case 2:
			ov2685_write_reg(client, MISENSOR_8BIT,0x3a03, 0x5a);
			ov2685_write_reg(client, MISENSOR_8BIT,0x3a04, 0x50);
			break;
	}
	return 0;
}

static int __ov2685_init(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov2685_device *dev = to_ov2685_sensor(sd);
	int ret;

	ret = ov2685_write_reg_array(client, ov2685_2M_init);
	dev->res = OV2685_RES_2M;
	if (ret)
		return ret;

	/*
	 * delay 5ms to wait for sensor initialization finish.
	 */
	usleep_range(5000, 6000);
	return 0;
}

static int power_up(struct v4l2_subdev *sd)
{
	struct ov2685_device *dev = to_ov2685_sensor(sd);
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

	/* flis clock control */
	ret = dev->platform_data->flisclk_ctrl(sd, 1);
	if (ret)
		goto fail_clk;

	/* gpio ctrl */
	ret = dev->platform_data->gpio_ctrl(sd, 1);
	if (ret)
		dev_err(&client->dev, "gpio failed\n");

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
	struct ov2685_device *dev = to_ov2685_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
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

static int ov2685_s_power(struct v4l2_subdev *sd, int power)
{
	if (power == 0)
		return power_down(sd);

	if (power_up(sd))
		return -EINVAL;

	return __ov2685_init(sd);
}

/* ov2685 resolutions supports below 2 aspect ratio */
#define OV2685_4_3 1333 /* 4:3*//* 1600x1200*/
#define OV2685_3_2 1500 /* 3:2*//* 720x480*/
#define OV2685_16_9 1777 /* 16:9*//* 1280x720*/

static int ov2685_try_res(u32 *w, u32 *h)
{
	int i;
	unsigned int aspec_ratio;
	printk("ov2685 is asked for w=%d h=%d\n", *w, *h);

	aspec_ratio = *w * 1000 / *h;
	if (aspec_ratio < OV2685_3_2) {
		*w = OV2685_RES_2M_SIZE_H;
		*h = OV2685_RES_2M_SIZE_V;
	} else {
		*w = OV2685_RES_720P_SIZE_H;
		*h = OV2685_RES_720P_SIZE_V;
	}
	printk("ov2685 aspec_ratio is %d\n", aspec_ratio);
	return 0;
}

static struct ov2685_res_struct *ov2685_to_res(u32 w, u32 h)
{
	int  index;

	for (index = 0; index < N_RES; index++) {
		if (ov2685_res[index].width == w &&
		    ov2685_res[index].height == h)
			break;
	}

	/* No mode found */
	if (index >= N_RES)
		return NULL;

	return &ov2685_res[index];
}

static int ov2685_try_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	return ov2685_try_res(&fmt->width, &fmt->height);
}

static int ov2685_res2size(unsigned int res, int *h_size, int *v_size)
{
	unsigned short hsize;
	unsigned short vsize;

	switch (res) {
		/*
	case OV2685_RES_VGA:
		hsize = OV2685_RES_VGA_SIZE_H;
		vsize = OV2685_RES_VGA_SIZE_V;
		break;*/
	case OV2685_RES_720P:
		hsize = OV2685_RES_720P_SIZE_H;
		vsize = OV2685_RES_720P_SIZE_V;
		break;

	case OV2685_RES_2M:
		hsize = OV2685_RES_2M_SIZE_H;
		vsize = OV2685_RES_2M_SIZE_V;
		break;
	default:
		/* QVGA mode is still unsupported */
		WARN(1, "%s: Resolution 0x%08x unknown\n", __func__, res);
		return -EINVAL;
	}

	if (h_size != NULL)
		*h_size = hsize;
	if (v_size != NULL)
		*v_size = vsize;

	return 0;
}

static int ov2685_g_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct ov2685_device *dev = to_ov2685_sensor(sd);
	int width, height;
	int ret;
	fmt->code = V4L2_MBUS_FMT_UYVY8_1X16;
	ret = ov2685_res2size(dev->res, &width, &height);
	if (ret)
		return ret;
	fmt->width = width;
	fmt->height = height;

	return 0;
}


static struct misensor_reg const  Normal_full[]={
	{MISENSOR_8BIT, 0x3820, 0xc0},
	{MISENSOR_8BIT, 0x3821, 0x00},
	{MISENSOR_TOK_TERM, 0, 0},
};
static struct misensor_reg const   Mirror_on_full[]={
	{MISENSOR_8BIT, 0x3820, 0xc0},
	{MISENSOR_8BIT, 0x3821, 0x04},
	{MISENSOR_TOK_TERM, 0, 0},
};
static struct misensor_reg const    Flip_On_full[]={
	//Normal_full
	{MISENSOR_8BIT, 0x3820, 0xc4},
	{MISENSOR_8BIT, 0x3821, 0x00},
	{MISENSOR_TOK_TERM, 0, 0},
};
static struct misensor_reg const    Mirror_Flip_on_full[]={
	//Flip_On_full
	{MISENSOR_8BIT, 0x3820, 0xc4},
	{MISENSOR_8BIT, 0x3821, 0x04},
	{MISENSOR_TOK_TERM, 0, 0},
};


static struct misensor_reg const    Normal_quarter_size[]={
	//Mirror_on_quarter_size
	{MISENSOR_8BIT, 0x3820, 0xc2},
	{MISENSOR_8BIT, 0x3821, 0x01},
	{MISENSOR_TOK_TERM, 0, 0},
};
static struct misensor_reg const    Mirror_on_quarter_size[]={
	//Mirror_Flip_on_quarter_size
	{MISENSOR_8BIT, 0x3820, 0x02},
	{MISENSOR_8BIT, 0x3821, 0x05},
	{MISENSOR_TOK_TERM, 0, 0},
};
static struct misensor_reg const    Flip_On_quarter_size[]={
	//Normal_quarter_size
	{MISENSOR_8BIT, 0x3820, 0xc6},
	{MISENSOR_8BIT, 0x3821, 0x01},
	{MISENSOR_TOK_TERM, 0, 0},
};
static struct misensor_reg const    Mirror_Flip_on_quarter_size[]={
	//Flip_On_quarter_size
	{MISENSOR_8BIT, 0x3820, 0x06},
	{MISENSOR_8BIT, 0x3821, 0x05},
	{MISENSOR_TOK_TERM, 0, 0},
};

static int ov2685_s_mbus_fmt(struct v4l2_subdev *sd,
			      struct v4l2_mbus_framefmt *fmt)
{
	struct i2c_client *c = v4l2_get_subdevdata(sd);
	struct ov2685_device *dev = to_ov2685_sensor(sd);
	struct ov2685_res_struct *res_index;
	struct camera_mipi_info *mipi_info = NULL;
	u32 width = fmt->width;
	u32 height = fmt->height;
	int ret;
	u8 sum_flag=0;
	mipi_info = v4l2_get_subdev_hostdata(sd);
	if (mipi_info == NULL) {
		dev_err(&c->dev, "%s: can not find mipi info!!!\n", __func__);
		return -EINVAL;
	}

	ov2685_try_res(&width, &height);
	res_index = ov2685_to_res(width, height);

	/* Sanity check */
	if (unlikely(!res_index)) {
		WARN_ON(1);
		return -EINVAL;
	}

	printk("ov2685_s_mbus_fmt res %d, w%d, h%d\n",
		res_index->res, res_index->width, res_index->height);
	switch(res_index->res) {
		/*
		case OV2685_RES_VGA:
			ret = ov2685_write_reg_array(c, ov2685_vga_init);
			break;*/
		case OV2685_RES_720P:
			if (dev->res != res_index->res)
				ret = ov2685_write_reg_array(c, ov2685_720p_init);
			mipi_info->num_lanes = 1;
			break;

		case OV2685_RES_2M:
			if (dev->res != res_index->res)
				ret = ov2685_write_reg_array(c, ov2685_2M_init);
			mipi_info->num_lanes = 2;
			break;
		default:
			dev_err(&c->dev, "%s: can not support the resolution!!!\n", __func__);
	}
	sum_flag = (v_flag & 0x03) | (h_flag & 0x03);
//if(fmt->width <= 1296)
#if  1
	switch(sum_flag)
	{
		case 1:ov2685_write_reg_array(c, Mirror_on_full);break;
		case 2:ov2685_write_reg_array(c, Flip_On_full);break;
		case 3:ov2685_write_reg_array(c, Mirror_Flip_on_full);break;
		default: ov2685_write_reg_array(c, Normal_full);
	}
#else
		switch(sum_flag)
	{
		case 1:ov2685_write_reg_array(c, Mirror_on_quarter_size);break;
		case 2:ov2685_write_reg_array(c, Flip_On_quarter_size);break;
		case 3:ov2685_write_reg_array(c, Mirror_Flip_on_quarter_size);break;
		default: ov2685_write_reg_array(c, Normal_quarter_size);
	}
#endif

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
			if (width == ov2685_res[index].width &&
			    height == ov2685_res[index].height) {
				ov2685_res[index].used = 1;
				continue;
			}
			ov2685_res[index].used = 0;
		}
	}

	/*
	 * ov2685 - we don't poll for context switch
	 * because it does not happen with streaming disabled.
	 */
	dev->res = res_index->res;
	fmt->width = width;
	fmt->height = height;
	return 0;
}

static int ov2685_detect(struct i2c_client *client,  u16 *id, u8 *revision)
{
	struct i2c_adapter *adapter = client->adapter;
	u32 retvalue = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "%s: i2c error", __func__);
		return -ENODEV;
	}

	if (ov2685_read_reg(client, MISENSOR_16BIT,
		OV2685_REG_PID, &retvalue)) {
		dev_err(&client->dev, "sensor_id_high = 0x%x\n", retvalue);
		return -ENODEV;
	}

	dev_info(&client->dev, "sensor_id = 0x%x\n", retvalue);
	if (retvalue != OV2685_MOD_ID) {
		dev_err(&client->dev, "%s: failed: client->addr = %x\n",
			__func__, client->addr);
		return -ENODEV;
	}

	/* REVISIT: HACK: Driver is currently forcing revision to 0 */
	*revision = 0;

	return 0;
}

static int
ov2685_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct ov2685_device *dev = to_ov2685_sensor(sd);
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

	ret = ov2685_s_power(sd, 1);
	if (ret) {
		dev_err(&client->dev, "power_ctrl failed");
		goto fail_detect;
	}

	/* config & detect sensor */
	ret = ov2685_detect(client, &sensor_id, &sensor_revision);
	if (ret) {
		dev_err(&client->dev, "ov2685_detect err s_config.\n");
		goto fail_detect;
	}

	dev->sensor_id = sensor_id;
	dev->sensor_revision = sensor_revision;

	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret)
		goto fail_csi_cfg;

	ret = ov2685_s_power(sd, 0);
	if (ret)
		dev_err(&client->dev, "sensor power-gating failed\n");

	return ret;

fail_csi_cfg:
	dev->platform_data->csi_cfg(sd, 0);
fail_detect:
	ov2685_s_power(sd, 0);
	dev_err(&client->dev, "sensor power-gating failed\n");
	return ret;
}

static struct ov2685_control ov2685_controls[] = {
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
		.query = ov2685_g_exposure,
		.tweak = ov2685_s_exposure,
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
		.query = ov2685_g_scene,
		.tweak = ov2685_s_scene,
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
		.tweak = ov2685_s_freq,
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
		.query = ov2685_g_wb,
		.tweak = ov2685_s_wb,
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
		.query = ov2685_g_vflip,
		.tweak = ov2685_t_vflip,
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
		.query = ov2685_g_hflip,
		.tweak = ov2685_t_hflip,
	},
};
#define N_CONTROLS (ARRAY_SIZE(ov2685_controls))

static struct ov2685_control *ov2685_find_control(__u32 id)
{
	int i;

	for (i = 0; i < N_CONTROLS; i++) {
		if (ov2685_controls[i].qc.id == id)
			return &ov2685_controls[i];
	}
	return NULL;
}

static int ov2685_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	struct ov2685_control *ctrl = ov2685_find_control(qc->id);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (ctrl == NULL) {
		dev_err(&client->dev,  "%s unsupported control id!\n", __func__);
		return 0;
	}
	*qc = ctrl->qc;
	return 0;
}

static int ov2685_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ov2685_control *octrl = ov2685_find_control(ctrl->id);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	if (!octrl || !octrl->query) {
		dev_err(&client->dev,  "%s unsupported control id or no query func!\n",
			__func__);
		return 0;
	}
	ret = octrl->query(sd, &ctrl->value);
	return 0;
}

static int ov2685_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ov2685_control *octrl = ov2685_find_control(ctrl->id);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	printk("%s: %d\n", __func__,ctrl->id);

	if (!octrl || !octrl->tweak) {
		dev_err(&client->dev,  "%s unsupported control id or no tweak func!\n",
			__func__);
		return 0;
	}
	switch(ctrl->id)
	{
		case V4L2_CID_VFLIP:if(ctrl->value) v_flag=2;else  v_flag=0;
			break;
		case V4L2_CID_HFLIP:if(ctrl->value) h_flag=1;else  h_flag=0;
			break;
		default:break;
	};

	ret = octrl->tweak(sd, ctrl->value);
	return 0;
}

static int ov2685_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov2685_device *dev = to_ov2685_sensor(sd);
	dev->streaming = enable;
	return ov2685_write_reg(client, MISENSOR_8BIT,
			0x301c,
			enable ? 0xf0 : 0xf4);
}

static int
ov2685_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
	unsigned int index = fsize->index;

	if (index >= N_RES)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = ov2685_res[index].width;
	fsize->discrete.height = ov2685_res[index].height;

	fsize->reserved[0] = ov2685_res[index].used;

	return 0;
}

static int ov2685_enum_frameintervals(struct v4l2_subdev *sd,
				       struct v4l2_frmivalenum *fival)
{
	unsigned int index = fival->index;
	int i;

	if (index >= N_RES)
		return -EINVAL;

	/* find out the first equal or bigger size */
	for (i = 0; i < N_RES; i++) {
		if (ov2685_res[i].width >= fival->width &&
		    ov2685_res[i].height >= fival->height)
			break;
	}
	if (i == N_RES)
		i--;

	index = i;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = ov2685_res[index].fps;

	return 0;
}

static int
ov2685_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_OV2685, 0);
}

static int ov2685_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_fh *fh,
				  struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index)
		return -EINVAL;
	code->code = V4L2_MBUS_FMT_UYVY8_1X16;

	return 0;
}

static int ov2685_enum_frame_size(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh,
	struct v4l2_subdev_frame_size_enum *fse)
{
	unsigned int index = fse->index;

	if (index >= N_RES)
		return -EINVAL;

	fse->min_width = ov2685_res[index].width;
	fse->min_height = ov2685_res[index].height;
	fse->max_width = ov2685_res[index].width;
	fse->max_height = ov2685_res[index].height;

	return 0;
}

static int
ov2685_get_pad_format(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		       struct v4l2_subdev_format *fmt)
{
	struct ov2685_device *snr = to_ov2685_sensor(sd);

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
ov2685_set_pad_format(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		       struct v4l2_subdev_format *fmt)
{
	struct ov2685_device *snr = to_ov2685_sensor(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		snr->format = fmt->format;

	return 0;
}

/* set focus zone */
static int
ov2685_set_selection(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			     struct v4l2_subdev_selection *sel)
{
	return 0;
}

static int ov2685_g_parm(struct v4l2_subdev *sd,
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

static int ov2685_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct ov2685_device *dev = to_ov2685_sensor(sd);
	dev->run_mode = param->parm.capture.capturemode;
	return 0;
}

static int
ov2685_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
	int index;
	struct ov2685_device *snr = to_ov2685_sensor(sd);

	for (index = 0; index < N_RES; index++) {
		if (ov2685_res[index].res == snr->res) {
			*frames = ov2685_res[index].skip_frames;
			return 0;
		}
	}
	return -EINVAL;
}

static const struct v4l2_subdev_video_ops ov2685_video_ops = {
	.try_mbus_fmt = ov2685_try_mbus_fmt,
	.g_mbus_fmt = ov2685_g_mbus_fmt,
	.s_mbus_fmt = ov2685_s_mbus_fmt,
	.s_parm = ov2685_s_parm,
	.g_parm = ov2685_g_parm,
	.s_stream = ov2685_s_stream,
	.enum_framesizes = ov2685_enum_framesizes,
	.enum_frameintervals = ov2685_enum_frameintervals,
};

static const struct v4l2_subdev_sensor_ops ov2685_sensor_ops = {
	.g_skip_frames	= ov2685_g_skip_frames,
};

static const struct v4l2_subdev_core_ops ov2685_core_ops = {
	.g_chip_ident = ov2685_g_chip_ident,
	.queryctrl = ov2685_queryctrl,
	.g_ctrl = ov2685_g_ctrl,
	.s_ctrl = ov2685_s_ctrl,
	.s_power = ov2685_s_power,
};

static const struct v4l2_subdev_pad_ops ov2685_pad_ops = {
	.enum_mbus_code = ov2685_enum_mbus_code,
	.enum_frame_size = ov2685_enum_frame_size,
	.get_fmt = ov2685_get_pad_format,
	.set_fmt = ov2685_set_pad_format,
	.set_selection = ov2685_set_selection,
};

static const struct v4l2_subdev_ops ov2685_ops = {
	.core = &ov2685_core_ops,
	.video = &ov2685_video_ops,
	.sensor = &ov2685_sensor_ops,
	.pad = &ov2685_pad_ops,
};

static const struct media_entity_operations ov2685_entity_ops;

static int ov2685_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov2685_device *dev = container_of(sd,
					struct ov2685_device, sd);

	dev->platform_data->csi_cfg(sd, 0);

	release_firmware(dev->firmware);
	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	kfree(dev);

	return 0;
}

static int ov2685_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct ov2685_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&client->dev, "out of memory\n");
		return -ENOMEM;
	}

	v4l2_i2c_subdev_init(&(dev->sd), client, &ov2685_ops);
	if (client->dev.platform_data) {
		ret = ov2685_s_config(&dev->sd, client->irq,
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
		ov2685_remove(client);

	return ret;
out_free:
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);
	return ret;
}

MODULE_DEVICE_TABLE(i2c, ov2685_id);
static struct i2c_driver ov2685_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = OV2685_NAME,
	},
	.probe = ov2685_probe,
	.remove = __exit_p(ov2685_remove),
	.id_table = ov2685_id,
};

static __init int ov2685_init_mod(void)
{
	return i2c_add_driver(&ov2685_driver);
}

static __exit void ov2685_exit_mod(void)
{
	i2c_del_driver(&ov2685_driver);
}

module_init(ov2685_init_mod);
module_exit(ov2685_exit_mod);

MODULE_DESCRIPTION("A low-level driver for Omnivision OV2685 sensors");
MODULE_LICENSE("GPL");
