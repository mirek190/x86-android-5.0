/*
 * Support for BYD BF3905 0.3M camera sensor.
 *
 * Copyright (c) 2013 Intel Corporation. All Rights Reserved.
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
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <linux/io.h>

#include "bf3905.h"


/* i2c read/write stuff */
static int bf3905_read_reg(struct i2c_client *client,
			   u16 data_length, u8 reg, u8 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[6];

	if (!client->adapter) {
		dev_err(&client->dev, "%s error, no client->adapter\n",
			__func__);
		return -ENODEV;
	}

	if (data_length != BF3905_8BIT && data_length != BF3905_16BIT
					&& data_length != BF3905_32BIT) {
		dev_err(&client->dev, "%s error, invalid data length\n",
			__func__);
		return -EINVAL;
	}

	memset(msg, 0 , sizeof(msg));

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = I2C_MSG_LENGTH;
	msg[0].buf = data;

	/* One byte register */
	data[0] = reg;

	msg[1].addr = client->addr;
	msg[1].len = data_length;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = data;

	err = i2c_transfer(client->adapter, msg, 2);
	if (err != 2) {
		if (err >= 0)
			err = -EIO;
		dev_err(&client->dev,
			"read from offset 0x%x error %d", reg, err);
		return err;
	}

	*val = 0;
	/* one byte data */
	if (data_length == BF3905_8BIT)
		*val = (u8)data[0];

	return 0;
}

static int bf3905_i2c_write(struct i2c_client *client, u16 len, u8 *data)
{
	struct i2c_msg msg;
	const int num_msg = 1;
	int ret;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = len;
	msg.buf = data;
	ret = i2c_transfer(client->adapter, &msg, 1);

	return ret == num_msg ? 0 : -EIO;
}

static int bf3905_write_reg(struct i2c_client *client, u16 data_length,
							u8 reg, u8 val)
{
	int ret;
	unsigned char data[4] = {0};
	const u16 len = data_length + sizeof(u8); /* 8-bit address + data */

	if (data_length != BF3905_8BIT && data_length != BF3905_16BIT) {
		dev_err(&client->dev,
			"%s error, invalid data_length\n", __func__);
		return -EINVAL;
	}

	/* One byte data format */
	data[0] = reg;


	if (data_length == BF3905_8BIT) {
		data[1] = val;
	}

	ret = bf3905_i2c_write(client, len, data);
	if (ret)
		dev_err(&client->dev,
			"write error: wrote 0x%x to offset 0x%x error %d",
			val, reg, ret);

	return ret;
}

static int bf3905_MIPI_read_cmos_sensor(struct v4l2_subdev *sd, unsigned char add, unsigned char * data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	bf3905_read_reg(client, BF3905_8BIT,
					add, data);
	printk("***read add 0x%x, data is 0x%x.\n", add, *data);
	return 0;
}

static int bf3905_MIPI_write_cmos_sensor(struct v4l2_subdev *sd, unsigned char add, unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	/*Try write times*/	
	int i = 5;
	unsigned char data = 0;
	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	bf3905_write_reg(client, 1, add, value);
	/*Wait for the slave write data*/
	msleep(20);
	
	/*0x12 is for reset, the read data is not the same as write data*/
	if (add != 0x12) {
		bf3905_MIPI_read_cmos_sensor(sd, add, &data);
		while ((data != value) && i){
			bf3905_write_reg(client, 1, add, value);
			/*Wait for the slave write data*/
			msleep(20);
			bf3905_MIPI_read_cmos_sensor(sd, add, &data);
			i--;
		}
		if (i == 0) {
			printk("****write add 0x%x, value 0x%x. error\n.", add, value);	
		}
	}
	return 0;
}

/*
 * bf3905_write_reg_array - Initializes a list of BF3905 registers
 * @client: i2c driver client structure
 * @reglist: list of registers to be written
 *
 * This function initializes a list of registers. When consecutive addresses
 * are found in a row on the list, this function creates a buffer and sends
 * consecutive data in a single i2c_transfer().
 *
 * __bf3905_flush_reg_array, __bf3905_buf_reg_array() and
 * __bf3905_write_reg_is_consecutive() are internal functions to
 * bf3905_write_reg_array_fast() and should be not used anywhere else.
 *
 */

static int __bf3905_flush_reg_array(struct i2c_client *client,
				    struct bf3905_write_ctrl *ctrl)
{
	u16 size;

	if (ctrl->index == 0)
		return 0;

	size = sizeof(u16) + ctrl->index; /* 16-bit address + data */
	ctrl->buffer.addr = cpu_to_be16(ctrl->buffer.addr);
	ctrl->index = 0;

	return bf3905_i2c_write(client, size, (u8 *)&ctrl->buffer);
}

static int __bf3905_buf_reg_array(struct i2c_client *client,
				  struct bf3905_write_ctrl *ctrl,
				  const struct bf3905_reg *next)
{

	int size;
	u16 *data16;

	switch (next->type) {
	case BF3905_8BIT:
		size = 1;
		ctrl->buffer.data[ctrl->index] = (u8)next->val;
		break;
	case BF3905_16BIT:
		size = 2;
		data16 = (u16 *)&ctrl->buffer.data[ctrl->index];
		*data16 = cpu_to_be16((u16)next->val);
		break;
	default:
		return -EINVAL;
	}

	/* When first item is added, we need to store its starting address */
	if (ctrl->index == 0)
		ctrl->buffer.addr = next->reg;

	ctrl->index += size;

	/*
	 * Buffer cannot guarantee free space for u32? Better flush it to avoid
	 * possible lack of memory for next item.
	 */
	if (ctrl->index + sizeof(u16) >= BF3905_MAX_WRITE_BUF_SIZE)
		return __bf3905_flush_reg_array(client, ctrl);

	return 0;
}

static int __bf3905_write_reg_is_consecutive(struct i2c_client *client,
					     struct bf3905_write_ctrl *ctrl,
					     const struct bf3905_reg *next)
{
	if (ctrl->index == 0)
		return 1;

	return ctrl->buffer.addr + ctrl->index == next->reg;
}

static int bf3905_write_reg_array(struct i2c_client *client,
				  const struct bf3905_reg *reglist)
{
	const struct bf3905_reg *next = reglist;
	struct bf3905_write_ctrl ctrl;
	int err;

	ctrl.index = 0;
	for (; next->type != BF3905_TOK_TERM; next++) {
		switch (next->type & BF3905_TOK_MASK) {
		case BF3905_TOK_DELAY:
			err = __bf3905_flush_reg_array(client, &ctrl);
			if (err)
				return err;
			msleep(next->val);
			break;
		default:
			/*
			 * If next address is not consecutive, data needs to be
			 * flushed before proceed.
			 */
			if (!__bf3905_write_reg_is_consecutive(client, &ctrl,
								next)) {
				err = __bf3905_flush_reg_array(client, &ctrl);
				if (err)
					return err;
			}
			err = __bf3905_buf_reg_array(client, &ctrl, next);
			if (err) {
				dev_err(&client->dev, "%s: write error, aborted\n",
					 __func__);
				return err;
			}
			break;
		}
	}

	return __bf3905_flush_reg_array(client, &ctrl);
}
static int bf3905_g_focal(struct v4l2_subdev *sd, s32 *val)
{
	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);

	*val = (BF3905_FOCAL_LENGTH_NUM << 16) | BF3905_FOCAL_LENGTH_DEM;
	return 0;
}

static int bf3905_g_fnumber(struct v4l2_subdev *sd, s32 *val)
{
	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);

	*val = (BF3905_F_NUMBER_DEFAULT_NUM << 16) | BF3905_F_NUMBER_DEM;
	return 0;
}

static int bf3905_g_fnumber_range(struct v4l2_subdev *sd, s32 *val)
{
	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);

	*val = (BF3905_F_NUMBER_DEFAULT_NUM << 24) |
	(BF3905_F_NUMBER_DEM << 16) |
	(BF3905_F_NUMBER_DEFAULT_NUM << 8) | BF3905_F_NUMBER_DEM;
	return 0;
}


static int bf3905_get_intg_factor(struct i2c_client *client,
				struct camera_mipi_info *info,
				const struct bf3905_resolution *res)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	struct atomisp_sensor_mode_data *buf = &info->data;
	const unsigned int ext_clk_freq_hz = 19200000;
	const unsigned int pll_invariant_div = 10;
	unsigned int pix_clk_freq_hz;
	u8 pre_pll_clk_div;
	u8 pll_multiplier;
	u8 op_pix_clk_div;
	u8 reg_val;
	int ret;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	if (info == NULL)
		return -EINVAL;

	/* pixel clock calculattion */

	pre_pll_clk_div = 1;

	pll_multiplier = 1;
	op_pix_clk_div = 1;
	pix_clk_freq_hz = ext_clk_freq_hz / pre_pll_clk_div * pll_multiplier
				* op_pix_clk_div/pll_invariant_div;

    pix_clk_freq_hz = res->pix_clk_freq;
	dev->vt_pix_clk_freq_mhz = pix_clk_freq_hz;
	buf->vt_pix_clk_freq_mhz = pix_clk_freq_hz;


#if 1	
	/* get integration time *///ji fen shi jian

	buf->coarse_integration_time_min = BF3905_COARSE_INTG_TIME_MIN;
	buf->coarse_integration_time_max_margin =
					BF3905_COARSE_INTG_TIME_MAX_MARGIN;

	buf->fine_integration_time_min = BF3905_FINE_INTG_TIME_MIN;
	buf->fine_integration_time_max_margin =
					BF3905_FINE_INTG_TIME_MAX_MARGIN;

	buf->fine_integration_time_def = BF3905_FINE_INTG_TIME_MIN;
	buf->frame_length_lines = res->lines_per_frame;
	buf->line_length_pck = res->pixels_per_line;
	buf->read_mode = res->bin_mode;
#endif

	/* get the cropping and output resolution to ISP for this mode. */
	buf->crop_horizontal_start = 0;

	buf->crop_vertical_start = 0;

	buf->crop_horizontal_end = res->width;

	buf->crop_vertical_end = res->height;

	buf->output_width = res->width;

	buf->output_height = res->height;

	buf->binning_factor_x = res->bin_factor_x ?
					res->bin_factor_x : 1;
	buf->binning_factor_y = res->bin_factor_y ?
					res->bin_factor_y : 1;
	return 0;
}

static long __bf3905_set_exposure(struct v4l2_subdev *sd, int coarse_itg,
				 int gain, int digitgain)

{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;
    u8 value = 0;
	printk("%s coarse_itg:%d,gain:%d,digitgain:%d\n",__func__,coarse_itg,gain,digitgain);

	ret = bf3905_write_reg(client, BF3905_8BIT,
					0x8c, (coarse_itg >> 8) & 0xff);
	ret = bf3905_write_reg(client, BF3905_8BIT,
					0x8d, (coarse_itg) & 0xff);

	/* set analog gain */
	if (gain > 0x3f)
		gain = 0x3f;
	else if (gain < 0x00)
		gain = 0x00;

    bf3905_write_reg(client,BF3905_8BIT, 0x87, gain);
    printk("%s, write global gain, value = 0x%x",__func__, gain);

    bf3905_read_reg(client,BF3905_8BIT, 0x87, &value);
    printk("%s, read global gain, value = 0x%x",__func__, value);

	return 0;
}

static int bf3905_set_exposure(struct v4l2_subdev *sd, int exposure,int gain, int digitgain)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	int ret;

	printk("%s\n",__func__);
	mutex_lock(&dev->input_lock);
	ret = __bf3905_set_exposure(sd, exposure, gain, digitgain);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static long bf3905_s_exposure(struct v4l2_subdev *sd,struct atomisp_exposure *exposure)
{
	int exp = exposure->integration_time[0];
	int gain = exposure->gain[0];
	int digitgain = exposure->gain[1];

	printk("%s\n",__func__);
	return bf3905_set_exposure(sd, exp, gain, digitgain);
}

static long bf3905_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	switch (cmd) {
	case ATOMISP_IOC_S_EXPOSURE:
		return bf3905_s_exposure(sd, arg);
	default:
		return -EINVAL;
	}
	return 0;
}

/* This returns the exposure time being used. This should only be used
   for filling in EXIF data, not for actual image processing. */
static int bf3905_q_exposure(struct v4l2_subdev *sd, s32 *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 high, low, exposure_value;
	int ret;

	printk("%s\n",__func__);
	/* get exposure */
	ret = bf3905_read_reg(client, BF3905_8BIT, BF3905_COARSE_INTG_TIME_MSB, &high);
	if (ret < 0) {
		return ret;
	}

	ret = bf3905_read_reg(client, BF3905_8BIT, BF3905_COARSE_INTG_TIME_LSB, &low);
	if (ret < 0) {
		return ret;
	}

	exposure_value = ((((u16) high) << 8) | (u16) low);
	*value = exposure_value;

	return 0;
}


struct bf3905_control bf3905_controls[] = {
	{
		.qc = {
			.id = V4L2_CID_EXPOSURE_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "exposure",
			.minimum = 0x0,
			.maximum = 0xffff,
			.step = 0x01,
			.default_value = 0x00,
			.flags = 0,
		},
		.query = bf3905_q_exposure,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCAL_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focal length",
			.minimum = BF3905_FOCAL_LENGTH_DEFAULT,
			.maximum = BF3905_FOCAL_LENGTH_DEFAULT,
			.step = 0x01,
			.default_value = BF3905_FOCAL_LENGTH_DEFAULT,
			.flags = 0,
		},
		.query = bf3905_g_focal,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number",
			.minimum = BF3905_F_NUMBER_DEFAULT,
			.maximum = BF3905_F_NUMBER_DEFAULT,
			.step = 0x01,
			.default_value = BF3905_F_NUMBER_DEFAULT,
			.flags = 0,
		},
		.query = bf3905_g_fnumber,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_RANGE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number range",
			.minimum = BF3905_F_NUMBER_RANGE,
			.maximum =  BF3905_F_NUMBER_RANGE,
			.step = 0x01,
			.default_value = BF3905_F_NUMBER_RANGE,
			.flags = 0,
		},
		.query = bf3905_g_fnumber_range,
	},
};
#define N_CONTROLS (ARRAY_SIZE(bf3905_controls))

static struct bf3905_control *bf3905_find_control(u32 id)
{
	int i;

	printk("%s\n",__func__);
	for (i = 0; i < N_CONTROLS; i++)
		if (bf3905_controls[i].qc.id == id)
			return &bf3905_controls[i];
	return NULL;
}

static int bf3905_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	struct bf3905_control *ctrl = bf3905_find_control(qc->id);
	struct bf3905_device *dev = to_bf3905_sensor(sd);

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	if (ctrl == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	*qc = ctrl->qc;
	mutex_unlock(&dev->input_lock);

	return 0;
}

/* imx control set/get */
static int bf3905_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct bf3905_control *s_ctrl;
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	int ret;

	if (!ctrl)
		return -EINVAL;
	printk("###fun:%s, line:%d,ctrl id:%d.\n", __FUNCTION__, __LINE__,ctrl->id);

	s_ctrl = bf3905_find_control(ctrl->id);
	if ((s_ctrl == NULL) || (s_ctrl->query == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = s_ctrl->query(sd, &ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int bf3905_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct bf3905_control *octrl = bf3905_find_control(ctrl->id);
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	int ret;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	if ((octrl == NULL) || (octrl->tweak == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = octrl->tweak(sd, ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int bf3905_init(struct v4l2_subdev *sd)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	unsigned char data = 0;

	mutex_lock(&dev->input_lock);

	/* restore settings */
	bf3905_res = bf3905_res_preview;
	N_RES = N_RES_PREVIEW;

	printk("^^^^^^debug configure!\n");
	mutex_unlock(&dev->input_lock);

    bf3905_MIPI_write_cmos_sensor(sd, 0x12, 0x80);
    bf3905_MIPI_write_cmos_sensor(sd, 0x15, 0x12);
    bf3905_MIPI_write_cmos_sensor(sd, 0x09, 0x01);
    bf3905_MIPI_write_cmos_sensor(sd, 0x12, 0x01);//output raw data
    bf3905_MIPI_write_cmos_sensor(sd, 0xf3, 0x2a);//raw8
    bf3905_MIPI_write_cmos_sensor(sd, 0x3a, 0x20);

    bf3905_MIPI_write_cmos_sensor(sd, 0x1b, 0x0a);//25.6M MCLK,19.2M Xclk
    bf3905_MIPI_write_cmos_sensor(sd, 0x1e, 0x40);
    bf3905_MIPI_write_cmos_sensor(sd, 0x2a, 0x00);
    bf3905_MIPI_write_cmos_sensor(sd, 0x2b, 0x20);//dummy pixel
    bf3905_MIPI_write_cmos_sensor(sd, 0x8a, 0xa0);
    bf3905_MIPI_write_cmos_sensor(sd, 0x8b, 0x85);

    bf3905_MIPI_write_cmos_sensor(sd, 0x5d, 0xb3);
    bf3905_MIPI_write_cmos_sensor(sd, 0xbf, 0x08);
    bf3905_MIPI_write_cmos_sensor(sd, 0xc3, 0x08);
    bf3905_MIPI_write_cmos_sensor(sd, 0xca, 0x10);
    bf3905_MIPI_write_cmos_sensor(sd, 0x62, 0x00);
    bf3905_MIPI_write_cmos_sensor(sd, 0x63, 0x00);
    bf3905_MIPI_write_cmos_sensor(sd, 0xb9, 0x00);
    bf3905_MIPI_write_cmos_sensor(sd, 0x64, 0x00);
    bf3905_MIPI_write_cmos_sensor(sd, 0xbb, 0x10);
    bf3905_MIPI_write_cmos_sensor(sd, 0x08, 0x02);
    bf3905_MIPI_write_cmos_sensor(sd, 0x20, 0x09);
    bf3905_MIPI_write_cmos_sensor(sd, 0x21, 0x4f);
    bf3905_MIPI_write_cmos_sensor(sd, 0x3e, 0x83);
    bf3905_MIPI_write_cmos_sensor(sd, 0x2f, 0x04);
    bf3905_MIPI_write_cmos_sensor(sd, 0x16, 0xa3);
    bf3905_MIPI_write_cmos_sensor(sd, 0x6c, 0xc2);
    bf3905_MIPI_write_cmos_sensor(sd, 0xd9, 0x25);
    bf3905_MIPI_write_cmos_sensor(sd, 0xdf, 0x26);
    bf3905_MIPI_write_cmos_sensor(sd, 0x13, 0x08);

	return 0;
}


static int power_up(struct v4l2_subdev *sd)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	if (NULL == dev->platform_data) {
		dev_err(&client->dev,
			"no camera_sensor_platform_data");
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
	if (ret) {
		ret = dev->platform_data->gpio_ctrl(sd, 1);
		if (ret)
			goto fail_power;
	}

	/* flis clock control */
	ret = dev->platform_data->flisclk_ctrl(sd, 1);
	if (ret)
		goto fail_clk;

	/* according to DS, 20ms is needed between PWDN and i2c access */
	msleep(20);
	printk("***power up return 0!");
	return 0;

fail_clk:
	dev->platform_data->gpio_ctrl(sd, 0);
fail_power:
	dev->platform_data->power_ctrl(sd, 0);
	dev_err(&client->dev, "sensor power-up failed\n");

	return ret;
}

static int power_down(struct v4l2_subdev *sd)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	if (NULL == dev->platform_data) {
		dev_err(&client->dev,
			"no camera_sensor_platform_data");
		return -ENODEV;
	}

	ret = dev->platform_data->flisclk_ctrl(sd, 0);
	if (ret)
		dev_err(&client->dev, "flisclk failed\n");

	/* gpio ctrl */
	ret = dev->platform_data->gpio_ctrl(sd, 0);
	if (ret) {
		ret = dev->platform_data->gpio_ctrl(sd, 0);
		if (ret)
			dev_err(&client->dev, "gpio failed 2\n");
	}

	/* power control */
	ret = dev->platform_data->power_ctrl(sd, 0);
	if (ret)
		dev_err(&client->dev, "vprog failed.\n");
	return ret;
}

static int bf3905_s_power(struct v4l2_subdev *sd, int on)
{
	int ret;
	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	if (on == 0)
		return power_down(sd);
	else {
		ret = power_up(sd);
		if (!ret)
			return bf3905_init(sd);
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
static int distance(struct bf3905_resolution *res, u32 w, u32 h)
{
	unsigned int w_ratio = ((res->width << 13)/w);
	unsigned int h_ratio;
	int match;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
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
	struct bf3905_resolution *tmp_res = NULL;

	printk("###fun:%s, line:%d, w:%d, h:%d.\n", __FUNCTION__, __LINE__, w, h);
	for (i = 0; i < N_RES; i++) {
		tmp_res = &bf3905_res[i];
		dist = distance(tmp_res, w, h);
		if (dist == -1)
			continue;
		if (dist < min_dist) {
			min_dist = dist;
			idx = i;
		}
	}
	printk("###return idex is %d.\n", idx);
	return idx;
}

static int get_resolution_index(int w, int h)
{
	int i;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	printk("####w is %d, h is %d.\n", w, h);
	for (i = 0; i < N_RES; i++) {
		if (w != bf3905_res[i].width)
			continue;
		if (h != bf3905_res[i].height)
			continue;

		return i;
	}

	return -1;
}

static int bf3905_try_mbus_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *fmt)
{
	int idx;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	if (!fmt)
		return -EINVAL;
	idx = nearest_resolution_index(fmt->width,
					fmt->height);
	if (idx == -1) {
		/* return the largest resolution */
		fmt->width = bf3905_res[N_RES - 1].width;
		fmt->height = bf3905_res[N_RES - 1].height;
	} else {
		fmt->width = bf3905_res[idx].width;
		fmt->height = bf3905_res[idx].height;
	}
	fmt->code = V4L2_MBUS_FMT_SBGGR8_1X8;

	return 0;
}

/* TODO: remove it. */
static int startup(struct v4l2_subdev *sd)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);

	ret = bf3905_write_reg_array(client, bf3905_res[dev->fmt_idx].regs);
	if (ret) {
		dev_err(&client->dev, "bf3905 write register err.\n");
		return ret;
	}

	return ret;
}

static int bf3905_s_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_mipi_info *bf3905_info = NULL;
	int ret = 0;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	bf3905_info = v4l2_get_subdev_hostdata(sd);
	if (bf3905_info == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = bf3905_try_mbus_fmt(sd, fmt);
	if (ret == -1) {
		dev_err(&client->dev, "try fmt fail\n");
		goto err;
	}

	dev->fmt_idx = get_resolution_index(fmt->width,
					      fmt->height);
	if (dev->fmt_idx == -1) {
		dev_err(&client->dev, "get resolution fail\n");
		mutex_unlock(&dev->input_lock);
		return -EINVAL;
	}

	ret = bf3905_get_intg_factor(client, bf3905_info,
					&bf3905_res[dev->fmt_idx]);
	if (ret) {
		dev_err(&client->dev, "failed to get integration_factor\n");
		goto err;
	}

	ret = startup(sd);
	if (ret)
		dev_err(&client->dev, "bf3905 startup err\n");

err:
	mutex_unlock(&dev->input_lock);
	return ret;
}
static int bf3905_g_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);

	if (!fmt)
		return -EINVAL;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	fmt->width = bf3905_res[dev->fmt_idx].width;
	fmt->height = bf3905_res[dev->fmt_idx].height;
	fmt->code = V4L2_MBUS_FMT_SBGGR8_1X8;

	return 0;
}

static int bf3905_detect(struct i2c_client *client)
{
	struct i2c_adapter *adapter = client->adapter;
	u8 high, low;
	int ret;
	u16 id;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -ENODEV;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	ret = bf3905_read_reg(client, BF3905_8BIT,
					BF3905_SC_CMMN_CHIP_ID_H, &high);
	if (ret) {
		dev_err(&client->dev, "sensor_id_high = 0x%x\n", high);
		return -ENODEV;
	}
	ret = bf3905_read_reg(client, BF3905_8BIT,
					BF3905_SC_CMMN_CHIP_ID_L, &low);
	id = ((((u16) high) << 8) | (u16) low);

	//printk("####The id is 0x%x.", id);
	//while (1) {};

	if (id != BF3905_ID) {
		dev_err(&client->dev, "sensor ID error, id is 0x%x\n", id);
		return -ENODEV;
	}

	dev_dbg(&client->dev, "detect bf3905 success\n");
	return 0;
}

static int bf3905_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	mutex_lock(&dev->input_lock);

	
	printk("###fun:%s, line:%d, enable:%d.\n", __FUNCTION__, __LINE__, enable);
	ret = bf3905_write_reg(client, BF3905_8BIT, BF3905_SW_STREAM,
				enable ? BF3905_START_STREAMING :
				BF3905_STOP_STREAMING);
	/* restore settings */
	bf3905_res = bf3905_res_preview;
	N_RES = N_RES_PREVIEW;

	mutex_unlock(&dev->input_lock);
	return ret;
}

/* bf3905 enum frame size, frame intervals */
static int bf3905_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	unsigned int index = fsize->index;

	if (index >= N_RES)
		return -EINVAL;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = bf3905_res[index].width;
	fsize->discrete.height = bf3905_res[index].height;
	fsize->reserved[0] = bf3905_res[index].used;

	return 0;
}

static int bf3905_enum_frameintervals(struct v4l2_subdev *sd,
				      struct v4l2_frmivalenum *fival)
{
	unsigned int index = fival->index;

	if (index >= N_RES)
		return -EINVAL;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->width = bf3905_res[index].width;
	fival->height = bf3905_res[index].height;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = bf3905_res[index].fps;

	return 0;
}

static int bf3905_enum_mbus_fmt(struct v4l2_subdev *sd,
				unsigned int index,
				enum v4l2_mbus_pixelcode *code)
{
	*code = V4L2_MBUS_FMT_SBGGR8_1X8;

	return 0;
}

static int bf3905_s_config(struct v4l2_subdev *sd,
			   int irq, void *platform_data)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	if (platform_data == NULL)
		return -ENODEV;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	dev->platform_data =
		(struct camera_sensor_platform_data *)platform_data;

	mutex_lock(&dev->input_lock);
	/* power off the module, then power on it in future
	 * as first power on by board may not fulfill the
	 * power on sequqence needed by the module
	 */

	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "bf3905 power-off err.\n");
		goto fail_power_off;
	}

	ret = power_up(sd);
	if (ret) {
		dev_err(&client->dev, "bf3905 power-up err.\n");
		goto fail_power_on;
	}

	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret)
		goto fail_csi_cfg;


	/* config & detect sensor */
	ret = bf3905_detect(client);
	if (ret) {
		dev_err(&client->dev, "bf3905_detect err s_config.\n");
		goto fail_csi_cfg;
	}

	/* turn off sensor, after probed */
	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "bf3905 power-off err.\n");
		goto fail_csi_cfg;
	}

	mutex_unlock(&dev->input_lock);

	return 0;

fail_csi_cfg:
	dev->platform_data->csi_cfg(sd, 0);
fail_power_on:
	power_down(sd);
	dev_err(&client->dev, "sensor power-gating failed\n");
fail_power_off:
	mutex_unlock(&dev->input_lock);
	return ret;
}

static int bf3905_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!param)
		return -EINVAL;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	if (param->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		dev_err(&client->dev,  "unsupported buffer type.\n");
		return -EINVAL;
	}

	memset(param, 0, sizeof(*param));
	param->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (dev->fmt_idx >= 0 && dev->fmt_idx < N_RES) {
		param->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
		param->parm.capture.timeperframe.numerator = 1;
		param->parm.capture.capturemode = dev->run_mode;
		param->parm.capture.timeperframe.denominator =
			bf3905_res[dev->fmt_idx].fps;
	}
	return 0;
}

static int bf3905_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	dev->run_mode = param->parm.capture.capturemode;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	mutex_lock(&dev->input_lock);
	switch (dev->run_mode) {
	case CI_MODE_VIDEO:
		bf3905_res = bf3905_res_video;
		N_RES = N_RES_VIDEO;
		break;
	case CI_MODE_STILL_CAPTURE:
		bf3905_res = bf3905_res_still;
		N_RES = N_RES_STILL;
		break;
	default:
		bf3905_res = bf3905_res_preview;
		N_RES = N_RES_PREVIEW;
	}
	mutex_unlock(&dev->input_lock);
	return 0;
}

static int bf3905_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	interval->interval.numerator = 1;
	interval->interval.denominator = bf3905_res[dev->fmt_idx].fps;

	return 0;
}

static int bf3905_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= MAX_FMTS)
		return -EINVAL;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	code->code = V4L2_MBUS_FMT_SBGGR8_1X8;//V4L2_MBUS_FMT_SBGGR10_1X10;  //ryx
	return 0;
}

static int bf3905_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_frame_size_enum *fse)
{
	int index = fse->index;

	if (index >= N_RES)
		return -EINVAL;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	fse->min_width = bf3905_res[index].width;
	fse->min_height = bf3905_res[index].height;
	fse->max_width = bf3905_res[index].width;
	fse->max_height = bf3905_res[index].height;

	return 0;

}

static struct v4l2_mbus_framefmt *
__bf3905_get_pad_format(struct bf3905_device *sensor,
			struct v4l2_subdev_fh *fh, unsigned int pad,
			enum v4l2_subdev_format_whence which)
{
	struct i2c_client *client = v4l2_get_subdevdata(&sensor->sd);

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	if (pad != 0) {
		dev_err(&client->dev,
			"__bf3905_get_pad_format err. pad %x\n", pad);
		return NULL;
	}

	switch (which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		return v4l2_subdev_get_try_format(fh, pad);
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		return &sensor->format;
	default:
		return NULL;
	}
}

static int bf3905_get_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	struct bf3905_device *snr = to_bf3905_sensor(sd);
	struct v4l2_mbus_framefmt *format =
			__bf3905_get_pad_format(snr, fh, fmt->pad, fmt->which);
	if (!format)
		return -EINVAL;

	fmt->format = *format;
	return 0;
}

static int bf3905_set_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct bf3905_device *snr = to_bf3905_sensor(sd);

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		snr->format = fmt->format;

	return 0;
}

static int bf3905_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
	struct bf3905_device *dev = to_bf3905_sensor(sd);

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	mutex_lock(&dev->input_lock);
	*frames = bf3905_res[dev->fmt_idx].skip_frames;
	mutex_unlock(&dev->input_lock);

	return 0;
}

static const struct v4l2_subdev_sensor_ops bf3905_sensor_ops = {
	.g_skip_frames	= bf3905_g_skip_frames,//bf3905_device
};

static const struct v4l2_subdev_video_ops bf3905_video_ops = {
	.s_stream = bf3905_s_stream,//BF3905_SW_STREAM//BF3905_STOP_STREAMING//bf3905_res_preview
	.g_parm = bf3905_g_parm,
	.s_parm = bf3905_s_parm,
	.enum_framesizes = bf3905_enum_framesizes,///sizes
	.enum_frameintervals = bf3905_enum_frameintervals,//interval
	.enum_mbus_fmt = bf3905_enum_mbus_fmt,//format//V4L2_MBUS_FMT_SBGGR10_1X10
	.try_mbus_fmt = bf3905_try_mbus_fmt,//nearest_resolution_index//V4L2_MBUS_FMT_SBGGR8_1X8
	.g_mbus_fmt = bf3905_g_mbus_fmt,//V4L2_MBUS_FMT_SBGGR10_1X10
	.s_mbus_fmt = bf3905_s_mbus_fmt,//bf3905_try_mbus_fmt//get_resolution_index//bf3905_get_intg_factor//startup//BF3905_SW_RESET
	.g_frame_interval = bf3905_g_frame_interval,
};

static const struct v4l2_subdev_core_ops bf3905_core_ops = {
	.s_power = bf3905_s_power,//power_up//bf3905_init//if (on == 0)power_down
	.queryctrl = bf3905_queryctrl,///ryx//bf3905_controls
	.g_ctrl = bf3905_g_ctrl,
	.s_ctrl = bf3905_s_ctrl,
	.ioctl = bf3905_ioctl,//ryx//bf3905_s_exposure//bf3905_set_exposure//__bf3905_set_exposure
};

static const struct v4l2_subdev_pad_ops bf3905_pad_ops = {
	.enum_mbus_code = bf3905_enum_mbus_code,//V4L2_MBUS_FMT_SBGGR10_1X10
	.enum_frame_size = bf3905_enum_frame_size,
	.get_fmt = bf3905_get_pad_format,//__bf3905_get_pad_format
	.set_fmt = bf3905_set_pad_format,//bf3905_device
};

static const struct v4l2_subdev_ops bf3905_ops = {
	.core = &bf3905_core_ops,
	.video = &bf3905_video_ops,
	.pad = &bf3905_pad_ops,
	.sensor = &bf3905_sensor_ops,
};

static int bf3905_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct bf3905_device *dev = to_bf3905_sensor(sd);
	dev_dbg(&client->dev, "bf3905_remove...\n");

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	dev->platform_data->csi_cfg(sd, 0);

	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	kfree(dev);

	return 0;
}

static int bf3905_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct bf3905_device *dev;
	int ret;

	printk("###fun:%s, line:%d.\n", __FUNCTION__, __LINE__);
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&client->dev, "out of memory\n");
		return -ENOMEM;
	}

	mutex_init(&dev->input_lock);

	dev->fmt_idx = 0;
	v4l2_i2c_subdev_init(&(dev->sd), client, &bf3905_ops);

	if (client->dev.platform_data) {
		ret = bf3905_s_config(&dev->sd, client->irq,
				       client->dev.platform_data);
		if (ret)
			goto out_free;
	}

	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dev->pad.flags = MEDIA_PAD_FL_SOURCE;
	dev->format.code = V4L2_MBUS_FMT_SBGGR8_1X8;
	dev->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;

	ret = media_entity_init(&dev->sd.entity, 1, &dev->pad, 0);
	if (ret)
		bf3905_remove(client);

	return ret;
out_free:
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);
	return ret;
}

MODULE_DEVICE_TABLE(i2c, bf3905_id);
static struct i2c_driver bf3905_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = BF3905_NAME,
	},
	.probe = bf3905_probe,
	.remove = bf3905_remove,
	.id_table = bf3905_id,
};

static int init_bf3905(void)
{
	return i2c_add_driver(&bf3905_driver);
}

static void exit_bf3905(void)
{

	i2c_del_driver(&bf3905_driver);
}

module_init(init_bf3905);
module_exit(exit_bf3905);

MODULE_AUTHOR("Wei Liu <wei.liu@intel.com>");
MODULE_DESCRIPTION("A low-level driver for BYD 3905 sensors");
MODULE_LICENSE("GPL");
