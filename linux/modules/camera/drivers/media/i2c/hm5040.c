/*
 * Support for Himax HM5040 1080p HD camera sensor.
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
#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-device.h>
#include <asm/intel-mid.h>

#include "hm5040.h"

/* i2c read/write stuff */
static int hm5040_read_reg(struct i2c_client *client,
			   u16 data_length, u16 reg, u16 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[6];

	if (!client->adapter) {
		dev_err(&client->dev, "%s error, no client->adapter\n",
			__func__);
		return -ENODEV;
	}

	if (data_length != HM5040_8BIT && data_length != HM5040_16BIT
					&& data_length != HM5040_32BIT) {
		dev_err(&client->dev, "%s error, invalid data length\n",
			__func__);
		return -EINVAL;
	}

	memset(msg, 0 , sizeof(msg));

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = I2C_MSG_LENGTH;
	msg[0].buf = data;

	/* high byte goes out first */
	data[0] = (u8)(reg >> 8);
	data[1] = (u8)(reg & 0xff);

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
	/* high byte comes first */
	if (data_length == HM5040_8BIT)
		*val = (u8)data[0];
	else if (data_length == HM5040_16BIT)
		*val = be16_to_cpu(*(u16 *)&data[0]);
	else
		*val = be32_to_cpu(*(u32 *)&data[0]);

	return 0;
}

static int hm5040_i2c_write(struct i2c_client *client, u16 len, u8 *data)
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

static int hm5040_write_reg(struct i2c_client *client, u16 data_length,
							u16 reg, u16 val)
{
	int ret;
	unsigned char data[4] = {0};
	u16 *wreg = (u16 *)data;
	const u16 len = data_length + sizeof(u16); /* 16-bit address + data */

	if (data_length != HM5040_8BIT && data_length != HM5040_16BIT) {
		dev_err(&client->dev,
			"%s error, invalid data_length\n", __func__);
		return -EINVAL;
	}

	/* high byte goes out first */
	*wreg = cpu_to_be16(reg);

	if (data_length == HM5040_8BIT) {
		data[2] = (u8)(val);
	} else {
		/* HM5040_16BIT */
		u16 *wdata = (u16 *)&data[2];
		*wdata = cpu_to_be16(val);
	}

	ret = hm5040_i2c_write(client, len, data);
	if (ret)
		dev_err(&client->dev,
			"write error: wrote 0x%x to offset 0x%x error %d",
			val, reg, ret);

	return ret;
}

/*
 * hm5040_write_reg_array - Initializes a list of HM5040 registers
 * @client: i2c driver client structure
 * @reglist: list of registers to be written
 *
 * This function initializes a list of registers. When consecutive addresses
 * are found in a row on the list, this function creates a buffer and sends
 * consecutive data in a single i2c_transfer().
 *
 * __hm5040_flush_reg_array, __hm5040_buf_reg_array() and
 * __hm5040_write_reg_is_consecutive() are internal functions to
 * hm5040_write_reg_array_fast() and should be not used anywhere else.
 *
 */

static int __hm5040_flush_reg_array(struct i2c_client *client,
				    struct hm5040_write_ctrl *ctrl)
{
	u16 size;

	if (ctrl->index == 0)
		return 0;

	size = sizeof(u16) + ctrl->index; /* 16-bit address + data */
	ctrl->buffer.addr = cpu_to_be16(ctrl->buffer.addr);
	ctrl->index = 0;

	return hm5040_i2c_write(client, size, (u8 *)&ctrl->buffer);
}

static int __hm5040_buf_reg_array(struct i2c_client *client,
				  struct hm5040_write_ctrl *ctrl,
				  const struct hm5040_reg *next)
{
	int size;
	u16 *data16;

	switch (next->type) {
	case HM5040_8BIT:
		size = 1;
		ctrl->buffer.data[ctrl->index] = (u8)next->val;
		break;
	case HM5040_16BIT:
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
	if (ctrl->index + sizeof(u16) >= HM5040_MAX_WRITE_BUF_SIZE)
		return __hm5040_flush_reg_array(client, ctrl);

	return 0;
}

static int __hm5040_write_reg_is_consecutive(struct i2c_client *client,
					     struct hm5040_write_ctrl *ctrl,
					     const struct hm5040_reg *next)
{
	if (ctrl->index == 0)
		return 1;

	return ctrl->buffer.addr + ctrl->index == next->reg;
}

static int hm5040_write_reg_array(struct i2c_client *client,
				  const struct hm5040_reg *reglist)
{
	const struct hm5040_reg *next = reglist;
	struct hm5040_write_ctrl ctrl;
	int err;

	ctrl.index = 0;
	for (; next->type != HM5040_TOK_TERM; next++) {
		switch (next->type & HM5040_TOK_MASK) {
		case HM5040_TOK_DELAY:
			err = __hm5040_flush_reg_array(client, &ctrl);
			if (err)
				return err;
			msleep(next->val);
			break;
		default:
			/*
			 * If next address is not consecutive, data needs to be
			 * flushed before proceed.
			 */
			if (!__hm5040_write_reg_is_consecutive(client, &ctrl,
								next)) {
				err = __hm5040_flush_reg_array(client, &ctrl);
			if (err)
				return err;
			}
			err = __hm5040_buf_reg_array(client, &ctrl, next);
			if (err) {
				dev_err(&client->dev, "%s: write error, aborted\n",
					 __func__);
				return err;
			}
			break;
		}
	}

	return __hm5040_flush_reg_array(client, &ctrl);
}
static int hm5040_g_focal(struct v4l2_subdev *sd, s32 *val)
{
	*val = (HM5040_FOCAL_LENGTH_NUM << 16) | HM5040_FOCAL_LENGTH_DEM;
	return 0;
}

static int hm5040_g_fnumber(struct v4l2_subdev *sd, s32 *val)
{
	/*const f number for hm5040*/
	*val = (HM5040_F_NUMBER_DEFAULT_NUM << 16) | HM5040_F_NUMBER_DEM;
	return 0;
}

static int hm5040_g_fnumber_range(struct v4l2_subdev *sd, s32 *val)
{
	*val = (HM5040_F_NUMBER_DEFAULT_NUM << 24) |
		(HM5040_F_NUMBER_DEM << 16) |
		(HM5040_F_NUMBER_DEFAULT_NUM << 8) | HM5040_F_NUMBER_DEM;
	return 0;
}

static int hm5040_g_bin_factor_x(struct v4l2_subdev *sd, s32 *val)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);

	*val = hm5040_res[dev->fmt_idx].bin_factor_x;

	return 0;
}

static int hm5040_g_bin_factor_y(struct v4l2_subdev *sd, s32 *val)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);

	*val = hm5040_res[dev->fmt_idx].bin_factor_y;

	return 0;
}

static int hm5040_get_intg_factor(struct i2c_client *client,
				struct camera_mipi_info *info,
				const struct hm5040_resolution *res)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	struct atomisp_sensor_mode_data *buf = &info->data;
	const unsigned int ext_clk_freq_hz = 19200000;
	u16 vt_pix_clk_div;
	u16 vt_sys_clk_div;
	u16 pre_pll_clk_div;
	u16 pll_multiplier;
	u32 vt_pix_clk_freq_mhz;
	u32 div;
	u16 coarse_integration_time_min;
	u16 coarse_integration_time_max_margin;
	u16 fine_integration_time_min;
	u16 fine_integration_time_max_margin;
	u16 reg_val;
	int ret;

	if (info == NULL)
		return -EINVAL;

	/* pixel clock calculattion */
	ret =  hm5040_read_reg(client, HM5040_8BIT,
				HM5040_VT_PIX_CLK_DIV, &vt_pix_clk_div);
	if (ret)
		return ret;

	ret =  hm5040_read_reg(client, HM5040_8BIT,
				HM5040_VT_SYS_CLK_DIV, &vt_sys_clk_div);
	if (ret)
		return ret;

	ret =  hm5040_read_reg(client, HM5040_8BIT,
				HM5040_PRE_PLL_CLK_DIV, &pre_pll_clk_div);
	if (ret)
		return ret;

	ret =  hm5040_read_reg(client, HM5040_8BIT,
				HM5040_PLL_MULTIPLIER, &pll_multiplier);
	if (ret)
		return ret;

	div = pre_pll_clk_div * vt_sys_clk_div * vt_pix_clk_div;
	if (div == 0) {
		dev_err(&client->dev, "%s: div gets zero value.\n",
			__func__);
		return -EINVAL;
	}
	vt_pix_clk_freq_mhz = ext_clk_freq_hz / div;
	vt_pix_clk_freq_mhz *= pll_multiplier;

	pr_info("%s: pre_pll_clk_div %d vt_sys_clk_div %d vt_pix_clk_div %d. vt_pix_clk_freq_mhz %d\n",
		__func__, pre_pll_clk_div, vt_sys_clk_div, vt_pix_clk_div, vt_pix_clk_freq_mhz);

	dev->vt_pix_clk_freq_mhz = vt_pix_clk_freq_mhz;
	buf->vt_pix_clk_freq_mhz = vt_pix_clk_freq_mhz;

	/* get integration time */
	ret =  hm5040_read_reg(client, HM5040_16BIT,
				HM5040_COARSE_INTG_TIME_MIN, &coarse_integration_time_min);
	if (ret)
		return ret;

	ret =  hm5040_read_reg(client, HM5040_16BIT,
				HM5040_COARSE_INTG_TIME_MARGIN, &coarse_integration_time_max_margin);
	if (ret)
		return ret;

	buf->coarse_integration_time_min = coarse_integration_time_min;
	buf->coarse_integration_time_max_margin =
				coarse_integration_time_max_margin;

	ret =  hm5040_read_reg(client, HM5040_16BIT,
				HM5040_FINE_INTG_TIME_MIN, &fine_integration_time_min);
	if (ret)
		return ret;

	ret =  hm5040_read_reg(client, HM5040_16BIT,
				HM5040_FINE_INTG_TIME_MARGIN, &fine_integration_time_max_margin);
	if (ret)
		return ret;

	buf->fine_integration_time_min = fine_integration_time_min;
	buf->fine_integration_time_max_margin =
					fine_integration_time_max_margin;
	buf->fine_integration_time_def = fine_integration_time_min;

	pr_info("%s: coarse_integration_time_min %d coarse_integration_time_max_margin %d fine_integration_time_min %d. fine_integration_time_max_margin %d\n",
		__func__, coarse_integration_time_min, coarse_integration_time_max_margin, fine_integration_time_min, fine_integration_time_max_margin);

	buf->frame_length_lines = res->lines_per_frame;
	buf->line_length_pck = res->pixels_per_line;
	buf->read_mode = res->bin_mode;

	/* get the cropping and output resolution to ISP for this mode. */
	ret =  hm5040_read_reg(client, HM5040_16BIT,
					HM5040_HORIZONTAL_START, &reg_val);
	if (ret)
		return ret;
	buf->crop_horizontal_start = reg_val;

	ret =  hm5040_read_reg(client, HM5040_16BIT,
					HM5040_VERTICAL_START, &reg_val);
	if (ret)
		return ret;
	buf->crop_vertical_start = reg_val;

	ret = hm5040_read_reg(client, HM5040_16BIT,
					HM5040_HORIZONTAL_END, &reg_val);
	if (ret)
		return ret;
	buf->crop_horizontal_end = reg_val;

	ret = hm5040_read_reg(client, HM5040_16BIT,
					HM5040_VERTICAL_END, &reg_val);
	if (ret)
		return ret;
	buf->crop_vertical_end = reg_val;

	ret = hm5040_read_reg(client, HM5040_16BIT,
					HM5040_HORIZONTAL_OUTPUT_SIZE, &reg_val);
	if (ret)
		return ret;
	buf->output_width = reg_val;

	ret = hm5040_read_reg(client, HM5040_16BIT,
					HM5040_VERTICAL_OUTPUT_SIZE, &reg_val);
	if (ret)
		return ret;
	buf->output_height = reg_val;

	pr_info("%s: crop_horizontal_start %d crop_vertical_start %d crop_horizontal_end %d crop_vertical_end %d\n",
		__func__, buf->crop_horizontal_start, buf->crop_vertical_start, buf->crop_horizontal_end, buf->crop_vertical_end);

	pr_info("%s: output_width %d output_height %d\n",
		__func__, buf->output_width, buf->output_height);

	buf->binning_factor_x = res->bin_factor_x ?
					res->bin_factor_x : 1;
	buf->binning_factor_y = res->bin_factor_y ?
					res->bin_factor_y : 1;
	return 0;
}

static long __hm5040_set_exposure(struct v4l2_subdev *sd, int coarse_itg,
				 int gain, int digitgain)

{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	u16 vts, hts;
	int ret;
	int again,dgain;
	
	hts = hm5040_res[dev->fmt_idx].pixels_per_line;
	vts = hm5040_res[dev->fmt_idx].lines_per_frame;

	/* group hold */
	ret = hm5040_write_reg(client, HM5040_8BIT,
                                       HM5040_GROUP_ACCESS, 0x01);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_GROUP_ACCESS);
		return ret;
	}

	/* Increase the VTS to match exposure + MARGIN */
	if (coarse_itg > vts - HM5040_INTEGRATION_TIME_MARGIN)
		vts = (u16) coarse_itg + HM5040_INTEGRATION_TIME_MARGIN;

	ret = hm5040_write_reg(client, HM5040_8BIT, HM5040_TIMING_VTS_H, (vts >> 8) & 0xFF);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_TIMING_VTS_H);
		return ret;
	}

	ret = hm5040_write_reg(client, HM5040_8BIT, HM5040_TIMING_VTS_L, vts & 0xFF);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_TIMING_VTS_L);
		return ret;
	}

	/* set exposure */
	ret = hm5040_write_reg(client, HM5040_8BIT,
			       HM5040_EXPOSURE_L, coarse_itg & 0xFF);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_EXPOSURE_L);
		return ret;
	}

	ret = hm5040_write_reg(client, HM5040_8BIT,
			       HM5040_EXPOSURE_H, (coarse_itg >> 8) & 0xFF);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_EXPOSURE_H);
		return ret;
	}
#if 0
	/* Analog gain */
	ret = hm5040_write_reg(client, HM5040_8BIT, HM5040_AGC, gain & 0xff);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_AGC);
		return ret;
	}

	/* Digital gain */
	if (digitgain) {
		ret = hm5040_write_reg(client, HM5040_16BIT,
				HM5040_DIGITAL_GAIN_GR, digitgain);
		if (ret) {
			dev_err(&client->dev, "%s: write %x error, aborted\n",
				__func__, HM5040_DIGITAL_GAIN_GR);
			return ret;
		}

		ret = hm5040_write_reg(client, HM5040_16BIT,
				HM5040_DIGITAL_GAIN_R, digitgain);
		if (ret) {
			dev_err(&client->dev, "%s: write %x error, aborted\n",
				__func__, HM5040_DIGITAL_GAIN_R);
			return ret;
		}

		ret = hm5040_write_reg(client, HM5040_16BIT,
				HM5040_DIGITAL_GAIN_B, digitgain);
		if (ret) {
			dev_err(&client->dev, "%s: write %x error, aborted\n",
				__func__, HM5040_DIGITAL_GAIN_B);
			return ret;
		}

		ret = hm5040_write_reg(client, HM5040_16BIT,
				HM5040_DIGITAL_GAIN_GB, digitgain);
		if (ret) {
			dev_err(&client->dev, "%s: write %x error, aborted\n",
				__func__, HM5040_DIGITAL_GAIN_GB);
			return ret;
		}
	}
#else
	if(gain > 511)
		gain = 511;
	if(gain < 64){
		again = 0x0;
		dgain = gain*8;
	} else if (gain < 128) {
		again = 0x80;
		dgain = gain*4;
	} else if (gain < 256) {
		again = 0xC0;
		dgain = gain*2;
	} else if (gain < 512) {
		again = 0xE0;
		dgain = gain;
	}else {
		again = 0xF0;
		dgain = gain/2;
	}
	ret = hm5040_write_reg(client, HM5040_8BIT, HM5040_AGC, again & 0xff);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_AGC);
		return ret;
	}

if (dgain) {
	ret = hm5040_write_reg(client, HM5040_16BIT,
			HM5040_DIGITAL_GAIN_GR, dgain);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_DIGITAL_GAIN_GR);
		return ret;
	}

	ret = hm5040_write_reg(client, HM5040_16BIT,
			HM5040_DIGITAL_GAIN_R, dgain);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_DIGITAL_GAIN_R);
		return ret;
	}

	ret = hm5040_write_reg(client, HM5040_16BIT,
			HM5040_DIGITAL_GAIN_B, dgain);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_DIGITAL_GAIN_B);
		return ret;
	}

	ret = hm5040_write_reg(client, HM5040_16BIT,
			HM5040_DIGITAL_GAIN_GB, dgain);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_DIGITAL_GAIN_GB);
		return ret;
	}
}

#endif
	/* End group */
	ret = hm5040_write_reg(client, HM5040_8BIT,
			       HM5040_GROUP_ACCESS, 0x00);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM5040_GROUP_ACCESS);
		return ret;
	}

	return ret;
}

static int hm5040_set_exposure(struct v4l2_subdev *sd, int exposure,
	int gain, int digitgain)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	int ret;

	mutex_lock(&dev->input_lock);
	ret = __hm5040_set_exposure(sd, exposure, gain, digitgain);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static long hm5040_s_exposure(struct v4l2_subdev *sd,
			       struct atomisp_exposure *exposure)
{
	u16 coarse_itg = exposure->integration_time[0];
	u16 analog_gain = exposure->gain[0];
	u16 digital_gain = exposure->gain[1];

#if 0
	/* we should not accept the invalid value below */
	if (analog_gain == 0) {
		struct i2c_client *client = v4l2_get_subdevdata(sd);
		v4l2_err(client, "%s: invalid value\n", __func__);
		return -EINVAL;
	}
#endif
	return hm5040_set_exposure(sd, coarse_itg, analog_gain, digital_gain);
}

static long hm5040_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{

	switch (cmd) {
	case ATOMISP_IOC_S_EXPOSURE:
		return hm5040_s_exposure(sd, arg);
	default:
		return -EINVAL;
	}
	return 0;
}

/* This returns the exposure time being used. This should only be used
   for filling in EXIF data, not for actual image processing. */
static int hm5040_q_exposure(struct v4l2_subdev *sd, s32 *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 reg_v, reg_v2;
	int ret;

	/* get exposure */
	ret = hm5040_read_reg(client, HM5040_8BIT,
					HM5040_EXPOSURE_L,
					&reg_v);
	if (ret)
		goto err;

	ret = hm5040_read_reg(client, HM5040_8BIT,
					HM5040_EXPOSURE_H,
					&reg_v2);
	if (ret)
		goto err;

	reg_v += reg_v2 << 8;

	*value = reg_v + ((u32)reg_v2 << 8);
err:
	return ret;
}

int hm5040_vcm_power_up(struct v4l2_subdev *sd)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->power_up)
		return dev->vcm_driver->power_up(sd);
	return 0;
}

int hm5040_vcm_power_down(struct v4l2_subdev *sd)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->power_down)
		return dev->vcm_driver->power_down(sd);
	return 0;
}

int hm5040_vcm_init(struct v4l2_subdev *sd)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->init)
		return dev->vcm_driver->init(sd);
	return 0;
}

int hm5040_t_focus_vcm(struct v4l2_subdev *sd, u16 val)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_focus_vcm)
		return dev->vcm_driver->t_focus_vcm(sd, val);
	return 0;
}

int hm5040_t_focus_abs(struct v4l2_subdev *sd, s32 value)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_focus_abs)
		return dev->vcm_driver->t_focus_abs(sd, value);
	return 0;
}
int hm5040_t_focus_rel(struct v4l2_subdev *sd, s32 value)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_focus_rel)
		return dev->vcm_driver->t_focus_rel(sd, value);
	return 0;
}

int hm5040_q_focus_status(struct v4l2_subdev *sd, s32 *value)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->q_focus_status)
		return dev->vcm_driver->q_focus_status(sd, value);
	return 0;
}

int hm5040_q_focus_abs(struct v4l2_subdev *sd, s32 *value)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->q_focus_abs)
		return dev->vcm_driver->q_focus_abs(sd, value);
	return 0;
}

int hm5040_t_vcm_slew(struct v4l2_subdev *sd, s32 value)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_vcm_slew)
		return dev->vcm_driver->t_vcm_slew(sd, value);
	return 0;
}

int hm5040_t_vcm_timing(struct v4l2_subdev *sd, s32 value)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_vcm_timing)
		return dev->vcm_driver->t_vcm_timing(sd, value);
	return 0;
}

struct hm5040_control hm5040_controls[] = {
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
		.query = hm5040_q_exposure,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCUS_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focus move absolute",
			.minimum = 0,
			.maximum = HM5040_MAX_FOCUS_POS,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = hm5040_t_focus_abs,
		.query = hm5040_q_focus_abs,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCUS_RELATIVE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focus move relative",
			.minimum = HM5040_MAX_FOCUS_NEG,
			.maximum = HM5040_MAX_FOCUS_POS,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = hm5040_t_focus_rel,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCUS_STATUS,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focus status",
			.minimum = 0,
			.maximum = 100, /* allow enum to grow in the future */
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = hm5040_q_focus_status,
	},
#if 0	
	{
		.qc = {
			.id = V4L2_CID_VCM_SLEW,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "vcm slew",
			.minimum = 0,
			.maximum = HM5040_VCM_SLEW_STEP_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = hm5040_t_vcm_slew,
	},
	{
		.qc = {
			.id = V4L2_CID_VCM_TIMEING,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "vcm step time",
			.minimum = 0,
			.maximum = HM5040_VCM_SLEW_TIME_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = hm5040_t_vcm_timing,
	},
#endif	
	{
		.qc = {
			.id = V4L2_CID_FOCAL_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focal length",
			.minimum = HM5040_FOCAL_LENGTH_DEFAULT,
			.maximum = HM5040_FOCAL_LENGTH_DEFAULT,
			.step = 0x01,
			.default_value = HM5040_FOCAL_LENGTH_DEFAULT,
			.flags = 0,
		},
		.query = hm5040_g_focal,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number",
			.minimum = HM5040_F_NUMBER_DEFAULT,
			.maximum = HM5040_F_NUMBER_DEFAULT,
			.step = 0x01,
			.default_value = HM5040_F_NUMBER_DEFAULT,
			.flags = 0,
		},
		.query = hm5040_g_fnumber,
	},	
	{
		.qc = {
			.id = V4L2_CID_FOCAL_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focal length",
			.minimum = HM5040_FOCAL_LENGTH_DEFAULT,
			.maximum = HM5040_FOCAL_LENGTH_DEFAULT,
			.step = 0x01,
			.default_value = HM5040_FOCAL_LENGTH_DEFAULT,
			.flags = 0,
		},
		.query = hm5040_g_focal,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number",
			.minimum = HM5040_F_NUMBER_DEFAULT,
			.maximum = HM5040_F_NUMBER_DEFAULT,
			.step = 0x01,
			.default_value = HM5040_F_NUMBER_DEFAULT,
			.flags = 0,
		},
		.query = hm5040_g_fnumber,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_RANGE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number range",
			.minimum = HM5040_F_NUMBER_RANGE,
			.maximum =  HM5040_F_NUMBER_RANGE,
			.step = 0x01,
			.default_value = HM5040_F_NUMBER_RANGE,
			.flags = 0,
		},
		.query = hm5040_g_fnumber_range,
	},
	{
		.qc = {
			.id = V4L2_CID_BIN_FACTOR_HORZ,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "horizontal binning factor",
			.minimum = 0,
			.maximum = HM5040_BIN_FACTOR_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = hm5040_g_bin_factor_x,
	},
	{
		.qc = {
			.id = V4L2_CID_BIN_FACTOR_VERT,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "vertical binning factor",
			.minimum = 0,
			.maximum = HM5040_BIN_FACTOR_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = hm5040_g_bin_factor_y,
	},
};
#define N_CONTROLS (ARRAY_SIZE(hm5040_controls))

static struct hm5040_control *hm5040_find_control(u32 id)
{
	int i;

	for (i = 0; i < N_CONTROLS; i++)
		if (hm5040_controls[i].qc.id == id)
			return &hm5040_controls[i];
	return NULL;
}

static int hm5040_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	struct hm5040_control *ctrl = hm5040_find_control(qc->id);
	struct hm5040_device *dev = to_hm5040_sensor(sd);

	if (ctrl == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	*qc = ctrl->qc;
	mutex_unlock(&dev->input_lock);

	return 0;
}

/* hm5040 control set/get */
static int hm5040_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct hm5040_control *s_ctrl;
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	int ret;

	if (!ctrl)
		return -EINVAL;

	s_ctrl = hm5040_find_control(ctrl->id);
	if ((s_ctrl == NULL) || (s_ctrl->query == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = s_ctrl->query(sd, &ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int hm5040_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct hm5040_control *octrl = hm5040_find_control(ctrl->id);
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	int ret;

	if ((octrl == NULL) || (octrl->tweak == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = octrl->tweak(sd, ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int hm5040_init(struct v4l2_subdev *sd)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);

	pr_info("%s\n", __func__);
	mutex_lock(&dev->input_lock);

	/* restore settings */
	hm5040_res = hm5040_res_preview;
	N_RES = N_RES_PREVIEW;

	mutex_unlock(&dev->input_lock);

	return 0;
}


static int power_up(struct v4l2_subdev *sd)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	if (NULL == dev->platform_data) {
		dev_err(&client->dev,
			"no camera_sensor_platform_data");
		return -ENODEV;
	}

	pr_info("%s\n", __func__);
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
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	if (NULL == dev->platform_data) {
		dev_err(&client->dev,
			"no camera_sensor_platform_data");
		return -ENODEV;
	}

	pr_info("%s\n", __func__);
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

static int hm5040_s_power(struct v4l2_subdev *sd, int on)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	int ret;

	pr_info("%s: on %d\n", __func__, on);
	if (on == 0){
		if (dev->vcm_driver && dev->vcm_driver->power_down)
			ret = dev->vcm_driver->power_down(sd);
		return power_down(sd);
	}else {
		if (dev->vcm_driver && dev->vcm_driver->power_up)
			ret = dev->vcm_driver->power_up(sd);		
		ret = power_up(sd);
		if (!ret)
			return hm5040_init(sd);
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
static int distance(struct hm5040_resolution *res, u32 w, u32 h)
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
	struct hm5040_resolution *tmp_res = NULL;

	for (i = 0; i < N_RES; i++) {
		tmp_res = &hm5040_res[i];
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
		if (w != hm5040_res[i].width)
			continue;
		if (h != hm5040_res[i].height)
			continue;

		return i;
	}

	return -1;
}

static int hm5040_try_mbus_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *fmt)
{
	int idx;

	if (!fmt)
		return -EINVAL;
	idx = nearest_resolution_index(fmt->width,
					fmt->height);

	if (idx == -1) {
		/* return the largest resolution */
		fmt->width = hm5040_res[0].width;
		fmt->height = hm5040_res[0].height;
	} else {
		fmt->width = hm5040_res[idx].width;
		fmt->height = hm5040_res[idx].height;
	}

	fmt->code = V4L2_MBUS_FMT_SGRBG10_1X10;

	return 0;
}

/* TODO: remove it. */
static int startup(struct v4l2_subdev *sd)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	pr_info("%s\n", __func__);
	ret = hm5040_write_reg(client, HM5040_8BIT,
					HM5040_SW_RESET, 0x01);
	if (ret) {
		dev_err(&client->dev, "hm5040 reset err.\n");
		return ret;
	}

	ret = hm5040_write_reg_array(client, hm5040_global_setting);
	if (ret) {
		dev_err(&client->dev, "hm5040 write register err.\n");
		return ret;
	}

	ret = hm5040_write_reg_array(client, hm5040_res[dev->fmt_idx].regs);
	if (ret) {
		dev_err(&client->dev, "hm5040 write register err.\n");
		return ret;
	}

	return ret;
}

static int hm5040_s_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_mipi_info *hm5040_info = NULL;
	int ret = 0;

	hm5040_info = v4l2_get_subdev_hostdata(sd);
	if (hm5040_info == NULL)
		return -EINVAL;

	pr_info("%s\n", __func__);
	mutex_lock(&dev->input_lock);
	ret = hm5040_try_mbus_fmt(sd, fmt);
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

	ret = startup(sd);
	if (ret)
		dev_err(&client->dev, "hm5040 startup err\n");

	ret = hm5040_get_intg_factor(client, hm5040_info,
		&hm5040_res[dev->fmt_idx]);
	if (ret) {
		dev_err(&client->dev, "failed to get integration_factor\n");
		goto err;
	}

err:
	mutex_unlock(&dev->input_lock);
	return ret;
}
static int hm5040_g_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);

	if (!fmt)
		return -EINVAL;

	fmt->width = hm5040_res[dev->fmt_idx].width;
	fmt->height = hm5040_res[dev->fmt_idx].height;
	fmt->code = V4L2_MBUS_FMT_SGRBG10_1X10;

	return 0;
}

static int hm5040_detect(struct i2c_client *client)
{
	struct i2c_adapter *adapter = client->adapter;
	u16 high, low;
	int ret;
	u16 id;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -ENODEV;

	ret = hm5040_read_reg(client, HM5040_8BIT,
					HM5040_SC_CMMN_CHIP_ID_H, &high);
	if (ret) {
		dev_err(&client->dev, "sensor_id_high = 0x%x\n", high);
		return -ENODEV;
	}
	ret = hm5040_read_reg(client, HM5040_8BIT,
					HM5040_SC_CMMN_CHIP_ID_L, &low);
	id = ((((u16) high) << 8) | (u16) low);

	if (id != HM5040_ID) {
		dev_err(&client->dev, "sensor ID error 0x%x\n", id);
		return -ENODEV;
	}

	dev_dbg(&client->dev, "detect hm5040 success\n");

    //Rich
    printk("detect hm5040 success\n");

	return 0;
}

static int hm5040_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	mutex_lock(&dev->input_lock);

	ret = hm5040_write_reg(client, HM5040_8BIT, HM5040_SW_STREAM,
				enable ? HM5040_START_STREAMING :
				HM5040_STOP_STREAMING);

	mutex_unlock(&dev->input_lock);

	return ret;
}

/* hm5040 enum frame size, frame intervals */
static int hm5040_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	unsigned int index = fsize->index;

	if (index >= N_RES)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = hm5040_res[index].width;
	fsize->discrete.height = hm5040_res[index].height;
	fsize->reserved[0] = hm5040_res[index].used;

	return 0;
}

static int hm5040_enum_frameintervals(struct v4l2_subdev *sd,
				      struct v4l2_frmivalenum *fival)
{
	unsigned int index = fival->index;

	if (index >= N_RES)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->width = hm5040_res[index].width;
	fival->height = hm5040_res[index].height;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = hm5040_res[index].fps;

	return 0;
}

static int hm5040_enum_mbus_fmt(struct v4l2_subdev *sd,
				unsigned int index,
				enum v4l2_mbus_pixelcode *code)
{
	*code = V4L2_MBUS_FMT_SGRBG10_1X10;

	return 0;
}

static int __update_hm5040_device_settings(struct hm5040_device *dev)
{
	dev->vcm_driver = &hm5040_vcm_ops;
	return dev->vcm_driver->init(&dev->sd);
}


static int hm5040_s_config(struct v4l2_subdev *sd,
			   int irq, void *platform_data)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	if (platform_data == NULL)
		return -ENODEV;

	dev->platform_data =
		(struct camera_sensor_platform_data *)platform_data;

	mutex_lock(&dev->input_lock);
	/* power off the module, then power on it in future
	 * as first power on by board may not fulfill the
	 * power on sequqence needed by the module
	 */	
	if (dev->platform_data->platform_init) {
		ret = dev->platform_data->platform_init(client);
		if (ret) {
			mutex_unlock(&dev->input_lock);
			dev_err(&client->dev, "hm5040 platform init err\n");
			return ret;
		}
	}

	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "hm5040 power-off err.\n");
		goto fail_power_off;
	}

	ret = power_up(sd);
	if (ret) {
		dev_err(&client->dev, "hm5040 power-up err.\n");
		goto fail_power_on;
	}

	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret)
		goto fail_csi_cfg;

	/* config & detect sensor */
	ret = hm5040_detect(client);
	if (ret) {
		dev_err(&client->dev, "hm5040_detect err s_config.\n");
		goto fail_csi_cfg;
	}

	ret = __update_hm5040_device_settings(dev);

	/* turn off sensor, after probed */
	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "hm5040 power-off err.\n");
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

static int hm5040_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!param)
		return -EINVAL;

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
			hm5040_res[dev->fmt_idx].fps;
	}
	return 0;
}

static int hm5040_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	dev->run_mode = param->parm.capture.capturemode;

	mutex_lock(&dev->input_lock);
	switch (dev->run_mode) {
	case CI_MODE_VIDEO:
		hm5040_res = hm5040_res_video;
		N_RES = N_RES_VIDEO;
		break;
	case CI_MODE_STILL_CAPTURE:
		hm5040_res = hm5040_res_still;
		N_RES = N_RES_STILL;
		break;
	default:
		hm5040_res = hm5040_res_preview;
		N_RES = N_RES_PREVIEW;
	}
	mutex_unlock(&dev->input_lock);
	return 0;
}

static int hm5040_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct hm5040_device *dev = to_hm5040_sensor(sd);

	interval->interval.numerator = 1;
	interval->interval.denominator = hm5040_res[dev->fmt_idx].fps;

	return 0;
}

static int hm5040_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= MAX_FMTS)
		return -EINVAL;

	code->code = V4L2_MBUS_FMT_SGRBG10_1X10;
	return 0;
}

static int hm5040_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_frame_size_enum *fse)
{
	int index = fse->index;

	if (index >= N_RES)
		return -EINVAL;

	fse->min_width = hm5040_res[index].width;
	fse->min_height = hm5040_res[index].height;
	fse->max_width = hm5040_res[index].width;
	fse->max_height = hm5040_res[index].height;

	return 0;

}

static struct v4l2_mbus_framefmt *
__hm5040_get_pad_format(struct hm5040_device *sensor,
			struct v4l2_subdev_fh *fh, unsigned int pad,
			enum v4l2_subdev_format_whence which)
{
	struct i2c_client *client = v4l2_get_subdevdata(&sensor->sd);

	if (pad != 0) {
		dev_err(&client->dev,
			"__hm5040_get_pad_format err. pad %x\n", pad);
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

static int hm5040_get_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct hm5040_device *snr = to_hm5040_sensor(sd);
	struct v4l2_mbus_framefmt *format =
			__hm5040_get_pad_format(snr, fh, fmt->pad, fmt->which);
	if (!format)
		return -EINVAL;

	fmt->format = *format;
	return 0;
}

static int hm5040_set_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct hm5040_device *snr = to_hm5040_sensor(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		snr->format = fmt->format;

	return 0;
}

static const struct v4l2_subdev_video_ops hm5040_video_ops = {
	.s_stream = hm5040_s_stream,
	.g_parm = hm5040_g_parm,
	.s_parm = hm5040_s_parm,
	.enum_framesizes = hm5040_enum_framesizes,
	.enum_frameintervals = hm5040_enum_frameintervals,
	.enum_mbus_fmt = hm5040_enum_mbus_fmt,
	.try_mbus_fmt = hm5040_try_mbus_fmt,
	.g_mbus_fmt = hm5040_g_mbus_fmt,
	.s_mbus_fmt = hm5040_s_mbus_fmt,
	.g_frame_interval = hm5040_g_frame_interval,
};

static const struct v4l2_subdev_core_ops hm5040_core_ops = {
	.s_power = hm5040_s_power,
	.queryctrl = hm5040_queryctrl,
	.g_ctrl = hm5040_g_ctrl,
	.s_ctrl = hm5040_s_ctrl,
	.ioctl = hm5040_ioctl,
};

static const struct v4l2_subdev_pad_ops hm5040_pad_ops = {
	.enum_mbus_code = hm5040_enum_mbus_code,
	.enum_frame_size = hm5040_enum_frame_size,
	.get_fmt = hm5040_get_pad_format,
	.set_fmt = hm5040_set_pad_format,
};

static const struct v4l2_subdev_ops hm5040_ops = {
	.core = &hm5040_core_ops,
	.video = &hm5040_video_ops,
	.pad = &hm5040_pad_ops,
};

static int hm5040_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct hm5040_device *dev = to_hm5040_sensor(sd);
	dev_dbg(&client->dev, "hm5040_remove...\n");

	dev->platform_data->csi_cfg(sd, 0);

	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	kfree(dev);

	return 0;
}

static int hm5040_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct hm5040_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&client->dev, "out of memory\n");
		return -ENOMEM;
	}

	pr_info("%s\n", __func__);
	mutex_init(&dev->input_lock);

	dev->fmt_idx = 0;
	v4l2_i2c_subdev_init(&(dev->sd), client, &hm5040_ops);

	if (client->dev.platform_data) {
		ret = hm5040_s_config(&dev->sd, client->irq,
				       client->dev.platform_data);
		if (ret)
			goto out_free;
	}

	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dev->pad.flags = MEDIA_PAD_FL_SOURCE;
	dev->format.code = V4L2_MBUS_FMT_SGRBG10_1X10;
	dev->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	dev->vcm_driver = &hm5040_vcm_ops;

	ret = media_entity_init(&dev->sd.entity, 1, &dev->pad, 0);
	if (ret)
		hm5040_remove(client);

	return ret;
out_free:
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);
	return ret;
}

MODULE_DEVICE_TABLE(i2c, hm5040_id);
static struct i2c_driver hm5040_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = HM5040_NAME,
	},
	.probe = hm5040_probe,
	.remove = hm5040_remove,
	.id_table = hm5040_id,
};

static int init_hm5040(void)
{
	return i2c_add_driver(&hm5040_driver);
}

static void exit_hm5040(void)
{

	i2c_del_driver(&hm5040_driver);
}

module_init(init_hm5040);
module_exit(exit_hm5040);
MODULE_AUTHOR("Jun CHEN <junx.chen@intel.com>");
MODULE_DESCRIPTION("A low-level driver for Himax hm5040 sensors");
MODULE_LICENSE("GPL");
