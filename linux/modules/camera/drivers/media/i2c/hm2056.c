/*
 * Support for Himax HM2056 camera sensor.
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

#include "hm2056.h"

static int h_flag = 0;
static int v_flag = 0;

static enum atomisp_bayer_order hm2056_bayer_order_mapping[] = {
        atomisp_bayer_order_grbg,
        atomisp_bayer_order_bggr,
        atomisp_bayer_order_rggb,
        atomisp_bayer_order_gbrg,
};

static enum v4l2_mbus_pixelcode hm2056_translate_bayer_order(enum atomisp_bayer_order code)
{
	switch (code) {
                case atomisp_bayer_order_gbrg: // 3
                        return V4L2_MBUS_FMT_SGBRG8_1X8;

                case atomisp_bayer_order_bggr: // 1
                        return V4L2_MBUS_FMT_SBGGR8_1X8;

                case atomisp_bayer_order_grbg: // 0
                        return V4L2_MBUS_FMT_SGRBG8_1X8;

                case atomisp_bayer_order_rggb: // 2
                        return V4L2_MBUS_FMT_SRGGB8_1X8;

                default:
                        break;
	}

	return 0;
}

/* i2c read/write stuff */
static int hm2056_read_reg(struct i2c_client *client,
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

	if (data_length != HM2056_8BIT && data_length != HM2056_16BIT
					&& data_length != HM2056_32BIT) {
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
	if (data_length == HM2056_8BIT)
		*val = (u8)data[0];
	else if (data_length == HM2056_16BIT)
		*val = be16_to_cpu(*(u16 *)&data[0]);
	else
		*val = be32_to_cpu(*(u32 *)&data[0]);
	return 0;
}

static int hm2056_i2c_write(struct i2c_client *client, u16 len, u8 *data)
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

static int hm2056_write_reg(struct i2c_client *client, u16 data_length,
							u16 reg, u16 val)
{
	int ret;
	unsigned char data[4] = {0};
	u16 *wreg = (u16 *)data;
	const u16 len = data_length + sizeof(u16); /* 16-bit address + data */

	if (data_length != HM2056_8BIT && data_length != HM2056_16BIT) {
		dev_err(&client->dev,
			"%s error, invalid data_length\n", __func__);
		return -EINVAL;
	}

	/* high byte goes out first */
	*wreg = cpu_to_be16(reg);

	if (data_length == HM2056_8BIT) {
		data[2] = (u8)(val);
	} else {
		/* HM2056_16BIT */
		u16 *wdata = (u16 *)&data[2];
		*wdata = cpu_to_be16(val);
	}

	ret = hm2056_i2c_write(client, len, data);
	if (ret)
		dev_err(&client->dev,
			"write error: wrote 0x%x to offset 0x%x error %d",
			val, reg, ret);

	return ret;
}

#if 0
/*
 * hm2056_write_reg_array - Initializes a list of HM2056 registers
 * @client: i2c driver client structure
 * @reglist: list of registers to be written
 *
 * This function initializes a list of registers. When consecutive addresses
 * are found in a row on the list, this function creates a buffer and sends
 * consecutive data in a single i2c_transfer().
 *
 * __hm2056_flush_reg_array, __hm2056_buf_reg_array() and
 * __hm2056_write_reg_is_consecutive() are internal functions to
 * hm2056_write_reg_array_fast() and should be not used anywhere else.
 *
 */

static int __hm2056_flush_reg_array(struct i2c_client *client,
				    struct hm2056_write_ctrl *ctrl)
{
	u16 size;

	if (ctrl->index == 0)
		return 0;

	size = sizeof(u16) + ctrl->index; /* 16-bit address + data */
	ctrl->buffer.addr = cpu_to_be16(ctrl->buffer.addr);
	ctrl->index = 0;

	return hm2056_i2c_write(client, size, (u8 *)&ctrl->buffer);
}

static int __hm2056_buf_reg_array(struct i2c_client *client,
				  struct hm2056_write_ctrl *ctrl,
				  const struct hm2056_reg *next)
{
	int size;
	u16 *data16;

	switch (next->type) {
	case HM2056_8BIT:
		size = 1;
		ctrl->buffer.data[ctrl->index] = (u8)next->val;
		break;
	case HM2056_16BIT:
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
	if (ctrl->index + sizeof(u16) >= HM2056_MAX_WRITE_BUF_SIZE)
		return __hm2056_flush_reg_array(client, ctrl);

	return 0;
}

static int __hm2056_write_reg_is_consecutive(struct i2c_client *client,
					     struct hm2056_write_ctrl *ctrl,
					     const struct hm2056_reg *next)
{
	if (ctrl->index == 0)
		return 1;

	return ctrl->buffer.addr + ctrl->index == next->reg;
}
#endif

static int hm2056_write_reg_array(struct i2c_client *client,
				  const struct hm2056_reg *reglist)
{
	const struct hm2056_reg *next = reglist;
	int err;

	for (; next->type != HM2056_TOK_TERM; next++) {
		switch (next->type & HM2056_TOK_MASK) {
		case HM2056_TOK_DELAY:
			msleep(next->val);
			break;
		default:
			err = hm2056_write_reg(client, next->type, next->reg, next->val);
			if (err)
				return err;
		}
	}

	return 0;
}

static int hm2056_g_focal(struct v4l2_subdev *sd, s32 *val)
{

	*val = (HM2056_FOCAL_LENGTH_NUM << 16) | HM2056_FOCAL_LENGTH_DEM;
	return 0;
}

static int hm2056_g_fnumber(struct v4l2_subdev *sd, s32 *val)
{
	/*const f number for hm2056*/

	*val = (HM2056_F_NUMBER_DEFAULT_NUM << 16) | HM2056_F_NUMBER_DEM;
	return 0;
}

static int hm2056_g_fnumber_range(struct v4l2_subdev *sd, s32 *val)
{
	*val = (HM2056_F_NUMBER_DEFAULT_NUM << 24) |
		(HM2056_F_NUMBER_DEM << 16) |
		(HM2056_F_NUMBER_DEFAULT_NUM << 8) | HM2056_F_NUMBER_DEM;
	return 0;
}

static int hm2056_g_bin_factor_x(struct v4l2_subdev *sd, s32 *val)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	*val = hm2056_res[dev->fmt_idx].bin_factor_x;

	return 0;
}

static int hm2056_g_bin_factor_y(struct v4l2_subdev *sd, s32 *val)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);

	*val = hm2056_res[dev->fmt_idx].bin_factor_y;
	return 0;
}


static int hm2056_get_intg_factor(struct i2c_client *client,
				struct camera_mipi_info *info,
				const struct hm2056_resolution *res)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	struct atomisp_sensor_mode_data *buf = &info->data;

	if (info == NULL)
		return -EINVAL;

	dev->vt_pix_clk_freq_mhz = res->vt_pix_clk_freq_mhz;
	buf->vt_pix_clk_freq_mhz = res->vt_pix_clk_freq_mhz;

	/* get integration time */
	buf->coarse_integration_time_min = HM2056_COARSE_INTG_TIME_MIN;
	buf->coarse_integration_time_max_margin =
					HM2056_COARSE_INTG_TIME_MAX_MARGIN;

	buf->fine_integration_time_min = HM2056_FINE_INTG_TIME_MIN;
	buf->fine_integration_time_max_margin =
					HM2056_FINE_INTG_TIME_MAX_MARGIN;

	buf->fine_integration_time_def = HM2056_FINE_INTG_TIME_MIN;
	buf->frame_length_lines = res->lines_per_frame;
	buf->line_length_pck = res->pixels_per_line;
	buf->read_mode = res->bin_mode;

	buf->crop_horizontal_start = 0;
	buf->crop_vertical_start = 0;
	buf->crop_horizontal_end = res->width - 1;
	buf->crop_vertical_end = res->height - 1;
	buf->output_width = res->width;
	buf->output_height = res->height;

	buf->binning_factor_x = res->bin_factor_x;
	buf->binning_factor_y = res->bin_factor_y;

	return 0;
}

static int hm2056_g_vflip(struct v4l2_subdev *sd, s32* value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	u16 flip;

	ret = hm2056_read_reg(client, HM2056_8BIT, 0x0006, &flip);
	if (ret) {
		dev_err(&client->dev, "%s: read 0x0006 error, aborted\n",
			__func__);
		return ret;
	}

	if (flip & 0x1) {
		*value = 1;
	} else {
		*value = 0;
	}

	return 0;
}

static int hm2056_g_hflip(struct v4l2_subdev *sd, s32* value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	u16 flip;

	ret = hm2056_read_reg(client, HM2056_8BIT, 0x0006, &flip);
	if (ret) {
		dev_err(&client->dev, "%s: read 0x0006 error, aborted\n",
			__func__);
		return ret;
	}

	if (flip & 0x2) {
		*value = 1;
	} else {
		*value = 0;
	}

	return 0;
}

static int hm2056_s_vflip(struct v4l2_subdev *sd, s32 value)
{
        u16 index = 0;
        u16 flip = 0;
        int ret = 0;
        struct i2c_client *client = v4l2_get_subdevdata(sd);
        struct hm2056_device *dev = to_hm2056_sensor(sd);
        struct camera_mipi_info *hm2056_info = NULL;

	ret = hm2056_read_reg(client, HM2056_8BIT, 0x0006, &flip);
	if (ret) {
		dev_err(&client->dev, "%s: read 0x0006 error, aborted\n",
			__func__);
		return ret;
	}

	if (1 == value) {
		ret = hm2056_write_reg(client, HM2056_8BIT, 0x0006, flip | 0x1);
	} else {
		ret = hm2056_write_reg(client, HM2056_8BIT, 0x0006, (flip & (~0x1)));
	}

	if (ret) {
		dev_err(&client->dev, "%s: write 0x0006 error, aborted\n",
			__func__);
	}

        index = ((v_flag) ? HM2056_FLIP_BIT : 0) | ((h_flag) ? HM2056_MIRROR_BIT : 0);
        hm2056_info = v4l2_get_subdev_hostdata(sd);
        if (hm2056_info) {
                hm2056_info->raw_bayer_order = hm2056_bayer_order_mapping[index];
                dev->format.code = hm2056_translate_bayer_order(hm2056_info->raw_bayer_order);
        }

	return ret;
}

static int hm2056_s_hflip(struct v4l2_subdev *sd, s32 value)
{
        u16 index = 0;
        u16 flip = 0;
        int ret = 0;
        struct i2c_client *client = v4l2_get_subdevdata(sd);
        struct hm2056_device *dev = to_hm2056_sensor(sd);
        struct camera_mipi_info *hm2056_info = NULL;

	ret = hm2056_read_reg(client, HM2056_8BIT, 0x0006, &flip);
	if (ret) {
		dev_err(&client->dev, "%s: read 0x0006 error, aborted\n",
			__func__);
		return ret;
	}

	if (1 == value) {
		ret = hm2056_write_reg(client, HM2056_8BIT, 0x0006, flip | 0x2);
	} else {
		ret = hm2056_write_reg(client, HM2056_8BIT, 0x0006, (flip & (~0x2)));
	}

	if (ret) {
		dev_err(&client->dev, "%s: write 0x0006 error, aborted\n",
			__func__);
	}

        index = ((v_flag) ? HM2056_FLIP_BIT : 0) | ((h_flag) ? HM2056_MIRROR_BIT : 0);
        hm2056_info = v4l2_get_subdev_hostdata(sd);
        if (hm2056_info) {
                hm2056_info->raw_bayer_order = hm2056_bayer_order_mapping[index];
                dev->format.code = hm2056_translate_bayer_order(hm2056_info->raw_bayer_order);
        }

	return ret;
}

static long __hm2056_set_exposure(struct v4l2_subdev *sd, int coarse_itg,
				 int gain, int digitgain)

{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 Again,Dgain;
	int ret;

	/* set exposure */
	ret = hm2056_write_reg(client, HM2056_8BIT,
			       HM2056_EXPOSURE_H, (coarse_itg & 0xFF00)>>8);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM2056_EXPOSURE_H);
		return ret;
	}

	ret = hm2056_write_reg(client, HM2056_8BIT,
			       HM2056_EXPOSURE_L, coarse_itg&0xff);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, HM2056_EXPOSURE_H);
		return ret;
	}

	if (16 <= gain && gain < 32) {
		Again = 0x0;
		Dgain = gain * 4;
	} else if (32 <= gain && gain < 64) {
		Again = 0x1;
		Dgain = gain * 2 * 11 / 10;
	} else if (64 <= gain && gain < 128) {
		Again = 0x2;
		Dgain = gain * 11 / 10;
	} else if (128 <= gain && gain <= 256) {
		Again = 0x3;
		Dgain = gain/2 * 11 / 10;
	} else {
		dev_err(&client->dev, "%s: unsupported gain value(%d)\n", __func__, gain);
		return -1;
	}

	ret = hm2056_write_reg(client, HM2056_8BIT, HM2056_AGAIN_REG, Again);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, 0x0018);
		return ret;
	}

	ret = hm2056_write_reg(client, HM2056_8BIT, HM2056_DGAIN_REG, Dgain);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, 0x001D);
		return ret;
	}

	ret = hm2056_write_reg(client, HM2056_8BIT, 0x0000, 0x01);
	ret = hm2056_write_reg(client, HM2056_8BIT, 0x0100, 0x01);
	ret = hm2056_write_reg(client, HM2056_8BIT, 0x0101, 0x01);

	return ret;
}

static int hm2056_set_exposure(struct v4l2_subdev *sd, int exposure,
	int gain, int digitgain)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	int ret;

	mutex_lock(&dev->input_lock);
	ret = __hm2056_set_exposure(sd, exposure, gain, digitgain);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static long hm2056_s_exposure(struct v4l2_subdev *sd,
			       struct atomisp_exposure *exposure)
{
	u16 coarse_itg = exposure->integration_time[0];
	u16 analog_gain = exposure->gain[0];
	u16 digital_gain = exposure->gain[1];

	/* we should not accept the invalid value below */
	if (analog_gain == 0) {
		struct i2c_client *client = v4l2_get_subdevdata(sd);
		v4l2_err(client, "%s: invalid value\n", __func__);
		return -EINVAL;
	}
	return hm2056_set_exposure(sd, coarse_itg, analog_gain, digital_gain);
}

static long hm2056_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	switch (cmd) {
	case ATOMISP_IOC_S_EXPOSURE:
		return hm2056_s_exposure(sd, arg);

	default:
		return -EINVAL;
	}
	return 0;
}

/* This returns the exposure time being used. This should only be used
   for filling in EXIF data, not for actual image processing. */
static int hm2056_q_exposure(struct v4l2_subdev *sd, s32 *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 h, l;
	int ret;

	ret = hm2056_read_reg(client, HM2056_8BIT,
					HM2056_EXPOSURE_H,
					&h);
	if (ret)
		goto err;

	/* get exposure */
	ret = hm2056_read_reg(client, HM2056_8BIT,
					HM2056_EXPOSURE_L,
					&l);
	if (ret)
		goto err;

	*value = ((h << 8) | l);
err:
	return ret;
}


struct hm2056_control hm2056_controls[] = {
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
		.query = hm2056_q_exposure,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCAL_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focal length",
			.minimum = HM2056_FOCAL_LENGTH_DEFAULT,
			.maximum = HM2056_FOCAL_LENGTH_DEFAULT,
			.step = 0x01,
			.default_value = HM2056_FOCAL_LENGTH_DEFAULT,
			.flags = 0,
		},
		.query = hm2056_g_focal,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number",
			.minimum = HM2056_F_NUMBER_DEFAULT,
			.maximum = HM2056_F_NUMBER_DEFAULT,
			.step = 0x01,
			.default_value = HM2056_F_NUMBER_DEFAULT,
			.flags = 0,
		},
		.query = hm2056_g_fnumber,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_RANGE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number range",
			.minimum = HM2056_F_NUMBER_RANGE,
			.maximum =  HM2056_F_NUMBER_RANGE,
			.step = 0x01,
			.default_value = HM2056_F_NUMBER_RANGE,
			.flags = 0,
		},
		.query = hm2056_g_fnumber_range,
	},
	{
		.qc = {
			.id = V4L2_CID_BIN_FACTOR_HORZ,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "horizontal binning factor",
			.minimum = 0,
			.maximum = HM2056_BIN_FACTOR_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = hm2056_g_bin_factor_x,
	},
	{
		.qc = {
			.id = V4L2_CID_BIN_FACTOR_VERT,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "vertical binning factor",
			.minimum = 0,
			.maximum = HM2056_BIN_FACTOR_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = hm2056_g_bin_factor_y,
	},
	{
		.qc = {
			.id = V4L2_CID_VFLIP,
			.type = V4L2_CTRL_TYPE_BOOLEAN,
			.name = "Flip",
			.minimum = 0,
			.maximum = 1,
			.step = 1,
			.default_value = 0,
		},
		.tweak = hm2056_s_vflip,
		.query = hm2056_g_vflip,
	},
	{
		.qc = {
			.id = V4L2_CID_HFLIP,
			.type = V4L2_CTRL_TYPE_BOOLEAN,
			.name = "Mirror",
			.minimum = 0,
			.maximum = 1,
			.step = 1,
			.default_value = 0,
		},
		.tweak = hm2056_s_hflip,
		.query = hm2056_g_hflip,
	},
};
#define N_CONTROLS (ARRAY_SIZE(hm2056_controls))

static struct hm2056_control *hm2056_find_control(u32 id)
{
	int i;

	for (i = 0; i < N_CONTROLS; i++)
		if (hm2056_controls[i].qc.id == id)
			return &hm2056_controls[i];
	return NULL;
}

static int hm2056_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	struct hm2056_control *ctrl = hm2056_find_control(qc->id);
	struct hm2056_device *dev = to_hm2056_sensor(sd);

	if (ctrl == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	*qc = ctrl->qc;
	mutex_unlock(&dev->input_lock);

	return 0;
}

/* hm2056 control set/get */
static int hm2056_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct hm2056_control *s_ctrl;
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	int ret;

	if (!ctrl)
		return -EINVAL;

	s_ctrl = hm2056_find_control(ctrl->id);
	if ((s_ctrl == NULL) || (s_ctrl->query == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = s_ctrl->query(sd, &ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int hm2056_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct hm2056_control *octrl = hm2056_find_control(ctrl->id);
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	int ret;

	if ((octrl == NULL) || (octrl->tweak == NULL))
		return -EINVAL;

	switch (ctrl->id)
	{
		case V4L2_CID_VFLIP:
                        v_flag = (ctrl->value) ? 1 : 0;
                        break;

		case V4L2_CID_HFLIP:
                        h_flag = (ctrl->value) ? 1 : 0;
                        break;

		default:
                        break;
	};

	mutex_lock(&dev->input_lock);
	ret = octrl->tweak(sd, ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int hm2056_init(struct v4l2_subdev *sd)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	int ret;

	mutex_lock(&dev->input_lock);

	/* restore settings */
	hm2056_res = hm2056_res_preview;
	N_RES = N_RES_PREVIEW;

	ret = hm2056_write_reg_array(client, hm2056_global_setting);

	mutex_unlock(&dev->input_lock);

	return ret;
}

static int power_up(struct v4l2_subdev *sd)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

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
		goto fail_power;
	}

	usleep_range(5000, 6000);

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
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

        h_flag = 0;
        v_flag = 0;

	if (NULL == dev->platform_data) {
		dev_err(&client->dev,
			"no camera_sensor_platform_data");
		return -ENODEV;
	}

    /* flis clock control */
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

static int hm2056_s_power(struct v4l2_subdev *sd, int on)
{
	int ret;

	if (on == 0){
		ret = power_down(sd);
	} else {
		ret = power_up(sd);
		if (!ret)
			return hm2056_init(sd);
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
#define LARGEST_ALLOWED_RATIO_MISMATCH 600
static int distance(struct hm2056_resolution *res, u32 w, u32 h)
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
	struct hm2056_resolution *tmp_res = NULL;

	for (i = 0; i < N_RES; i++) {
		tmp_res = &hm2056_res[i];
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
		if (w != hm2056_res[i].width)
			continue;
		if (h != hm2056_res[i].height)
			continue;

		return i;
	}

	return -1;
}

static int hm2056_try_mbus_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *fmt)
{
	int idx = 0;

	if (!fmt)
		return -EINVAL;
	idx = nearest_resolution_index(fmt->width, fmt->height);
	printk("%s, idx(%d), W(%d), H(%d)\n", __func__, idx, fmt->width, fmt->height);

	if (idx == -1) {
		/* return the largest resolution */
		fmt->width = hm2056_res[N_RES - 1].width;
		fmt->height = hm2056_res[N_RES - 1].height;
	} else {
		fmt->width = hm2056_res[idx].width;
		fmt->height = hm2056_res[idx].height;
	}
	fmt->code = V4L2_MBUS_FMT_SGRBG8_1X8;

	return 0;
}

static int hm2056_s_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_mipi_info *hm2056_info = NULL;
	s32 vFlip, hFlip;
	int ret = 0;

	hm2056_info = v4l2_get_subdev_hostdata(sd);
	if (hm2056_info == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = hm2056_try_mbus_fmt(sd, fmt);
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

	vFlip = hFlip = -1;
	hm2056_g_vflip(sd, &vFlip);
	hm2056_g_hflip(sd, &hFlip);

	v4l2_info(client, "%s i=%d, w=%d, h=%d\n",__func__, dev->fmt_idx,
		  fmt->width, fmt->height);

	ret = hm2056_write_reg_array(client, hm2056_res[dev->fmt_idx].regs);
	if (ret)
		dev_err(&client->dev, "hm2056 write resolution register err\n");

	hm2056_s_vflip(sd, vFlip);
	hm2056_s_hflip(sd, hFlip);

	ret = hm2056_get_intg_factor(client, hm2056_info,
					&hm2056_res[dev->fmt_idx]);
	if (ret) {
		dev_err(&client->dev, "failed to get integration_factor\n");
		goto err;
	}

err:
	mutex_unlock(&dev->input_lock);
	return ret;
}

static int hm2056_g_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);

	if (!fmt)
		return -EINVAL;

	fmt->width = hm2056_res[dev->fmt_idx].width;
	fmt->height = hm2056_res[dev->fmt_idx].height;
	fmt->code = V4L2_MBUS_FMT_SGRBG8_1X8;

	return 0;
}

static int hm2056_detect(struct i2c_client *client)
{
	struct i2c_adapter *adapter = client->adapter;
	u16 high, low;
	int ret;
	u16 id;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -ENODEV;

	ret = hm2056_read_reg(client, HM2056_8BIT,
					HM2056_CHIP_ID_H, &high);
	if (ret) {
		dev_err(&client->dev, "sensor_id_high = 0x%x\n", high);
		return -ENODEV;
	}
	ret = hm2056_read_reg(client, HM2056_8BIT,
					HM2056_CHIP_ID_L, &low);
	id = ((((u16) high) << 8) | (u16) low);

	if (id != HM2056_ID) {
		dev_err(&client->dev, "sensor ID error 0x%x\n", id);
		return -ENODEV;
	}

	return 0;
}

static int hm2056_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	mutex_lock(&dev->input_lock);

	ret = hm2056_write_reg_array(client, enable ? hm2056_stream_on : hm2056_stream_off);

	mutex_unlock(&dev->input_lock);

	return ret;
}

/* hm2056 enum frame size, frame intervals */
static int hm2056_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	unsigned int index = fsize->index;

	if (index >= N_RES)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = hm2056_res[index].width;
	fsize->discrete.height = hm2056_res[index].height;
	fsize->reserved[0] = hm2056_res[index].used;

	return 0;
}

static int hm2056_enum_frameintervals(struct v4l2_subdev *sd,
				      struct v4l2_frmivalenum *fival)
{
	unsigned int index = fival->index;

	if (index >= N_RES)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->width = hm2056_res[index].width;
	fival->height = hm2056_res[index].height;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = hm2056_res[index].fps;

	return 0;
}

static int hm2056_enum_mbus_fmt(struct v4l2_subdev *sd,
				unsigned int index,
				enum v4l2_mbus_pixelcode *code)
{
	*code = V4L2_MBUS_FMT_SGRBG8_1X8;

	return 0;
}

static int hm2056_s_config(struct v4l2_subdev *sd,
			   int irq, void *platform_data)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);
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
	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "hm2056 power-off err.\n");
		goto fail_power_off;
	}

	msleep(20);

	ret = power_up(sd);
	if (ret) {
		dev_err(&client->dev, "hm2056 power-up err.\n");
		goto fail_power_on;
	}

	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret)
		goto fail_csi_cfg;

	/* config & detect sensor */
	ret = hm2056_detect(client);
	if (ret) {
		dev_err(&client->dev, "hm2056_detect err s_config.\n");
		goto fail_csi_cfg;
	}

	/* turn off sensor, after probed */
	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "hm2056 power-off err.\n");
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

static int hm2056_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);
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
			hm2056_res[dev->fmt_idx].fps;
	}
	return 0;
}

static int hm2056_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	dev->run_mode = param->parm.capture.capturemode;

	mutex_lock(&dev->input_lock);
	switch (dev->run_mode) {
	case CI_MODE_VIDEO:
		hm2056_res = hm2056_res_video;
		N_RES = N_RES_VIDEO;
		break;
	case CI_MODE_STILL_CAPTURE:
		hm2056_res = hm2056_res_still;
		N_RES = N_RES_STILL;
		break;
	default:
		hm2056_res = hm2056_res_preview;
		N_RES = N_RES_PREVIEW;
	}
	mutex_unlock(&dev->input_lock);
	return 0;
}

static int hm2056_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);

	interval->interval.numerator = 1;
	interval->interval.denominator = hm2056_res[dev->fmt_idx].fps;

	return 0;
}

static int hm2056_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= MAX_FMTS)
		return -EINVAL;

	code->code = V4L2_MBUS_FMT_SGRBG8_1X8;
	return 0;
}

static int hm2056_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_frame_size_enum *fse)
{
	int index = fse->index;

	if (index >= N_RES)
		return -EINVAL;

	fse->min_width = hm2056_res[index].width;
	fse->min_height = hm2056_res[index].height;
	fse->max_width = hm2056_res[index].width;
	fse->max_height = hm2056_res[index].height;

	return 0;

}

static struct v4l2_mbus_framefmt *__hm2056_get_pad_format(struct hm2056_device
							  *sensor,
							  struct v4l2_subdev_fh
							  *fh, unsigned int pad,
							  enum
							  v4l2_subdev_format_whence
							  which)
{
	struct i2c_client *client = v4l2_get_subdevdata(&sensor->sd);

	if (pad != 0) {
		dev_err(&client->dev,
			"__hm2056_get_pad_format err. pad %x\n", pad);
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

static int hm2056_get_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct hm2056_device *snr = to_hm2056_sensor(sd);
	struct v4l2_mbus_framefmt *format =
			__hm2056_get_pad_format(snr, fh, fmt->pad, fmt->which);
	if (!format)
		return -EINVAL;

	fmt->format = *format;
	return 0;
}

static int hm2056_set_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct hm2056_device *snr = to_hm2056_sensor(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		snr->format = fmt->format;

	return 0;
}

static int hm2056_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
	struct hm2056_device *dev = to_hm2056_sensor(sd);

	mutex_lock(&dev->input_lock);
	*frames = hm2056_res[dev->fmt_idx].skip_frames;
	mutex_unlock(&dev->input_lock);

	return 0;
}

static const struct v4l2_subdev_video_ops hm2056_video_ops = {
	.s_stream = hm2056_s_stream,
	.g_parm = hm2056_g_parm,
	.s_parm = hm2056_s_parm,
	.enum_framesizes = hm2056_enum_framesizes,
	.enum_frameintervals = hm2056_enum_frameintervals,
	.enum_mbus_fmt = hm2056_enum_mbus_fmt,
	.try_mbus_fmt = hm2056_try_mbus_fmt,
	.g_mbus_fmt = hm2056_g_mbus_fmt,
	.s_mbus_fmt = hm2056_s_mbus_fmt,
	.g_frame_interval = hm2056_g_frame_interval,
};

static const struct v4l2_subdev_sensor_ops hm2056_sensor_ops = {
		.g_skip_frames	= hm2056_g_skip_frames,
};

static const struct v4l2_subdev_core_ops hm2056_core_ops = {
	.s_power = hm2056_s_power,
	.queryctrl = hm2056_queryctrl,
	.g_ctrl = hm2056_g_ctrl,
	.s_ctrl = hm2056_s_ctrl,
	.ioctl = hm2056_ioctl,
};

static const struct v4l2_subdev_pad_ops hm2056_pad_ops = {
	.enum_mbus_code = hm2056_enum_mbus_code,
	.enum_frame_size = hm2056_enum_frame_size,
	.get_fmt = hm2056_get_pad_format,
	.set_fmt = hm2056_set_pad_format,
};

static const struct v4l2_subdev_ops hm2056_ops = {
	.core = &hm2056_core_ops,
	.video = &hm2056_video_ops,
	.pad = &hm2056_pad_ops,
	.sensor = &hm2056_sensor_ops,
};

static int hm2056_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct hm2056_device *dev = to_hm2056_sensor(sd);
	dev_dbg(&client->dev, "hm2056_remove...\n");

	dev->platform_data->csi_cfg(sd, 0);

	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	kfree(dev);

	return 0;
}

static int hm2056_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct hm2056_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&client->dev, "out of memory\n");
		return -ENOMEM;
	}

	mutex_init(&dev->input_lock);

	dev->fmt_idx = 0;
	v4l2_i2c_subdev_init(&(dev->sd), client, &hm2056_ops);

	if (client->dev.platform_data) {
		ret = hm2056_s_config(&dev->sd, client->irq,
				       client->dev.platform_data);
		if (ret)
			goto out_free;
	}

	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dev->pad.flags = MEDIA_PAD_FL_SOURCE;
	dev->format.code = V4L2_MBUS_FMT_SGRBG8_1X8;
	dev->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;

	ret = media_entity_init(&dev->sd.entity, 1, &dev->pad, 0);
	if (ret)
	{
		hm2056_remove(client);
	}
	return ret;

out_free:
	dev_err(&client->dev, "+++ out free \n");
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);
	return ret;
}

MODULE_DEVICE_TABLE(i2c, hm2056_id);
static struct i2c_driver hm2056_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = HM2056_NAME,
	},
	.probe = hm2056_probe,
	.remove = hm2056_remove,
	.id_table = hm2056_id,
};

static int init_hm2056(void)
{
	return i2c_add_driver(&hm2056_driver);
}

static void exit_hm2056(void)
{

	i2c_del_driver(&hm2056_driver);
}

module_init(init_hm2056);
module_exit(exit_hm2056);

MODULE_AUTHOR("Guanghui Li <guanghuix.li@intel.com>");
MODULE_DESCRIPTION("A low-level driver for himax hm2056 sensors");
MODULE_LICENSE("GPL");

