/*
 * Support for BYD BF3A20 2MP camera sensor.
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

#include <linux/mfd/intel_mid_pmic.h>
#include "bf3a20.h"

static int register_reset = 1;

/* i2c read/write stuff */
static int bf3a20_read_reg(struct i2c_client *client,
			   u16 data_length, u16 reg, u16 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[6];
	int retry_count = 0;

	if (!client->adapter) {
		dev_err(&client->dev, "%s error, no client->adapter\n",
			__func__);
		return -ENODEV;
	}

	if (data_length != BF3A20_8BIT && data_length != BF3A20_16BIT
					&& data_length != BF3A20_32BIT) {
		dev_err(&client->dev, "%s error, invalid data length\n",
			__func__);
		return -EINVAL;
	}

	memset(msg, 0 , sizeof(msg));

	msg[0].addr = client->addr;
	msg[0].flags =  0;
	msg[0].len = I2C_MSG_LENGTH;
	msg[0].buf = data;

	data[0] = (u8)(reg & 0xFF);

	msg[1].addr = client->addr;
	msg[1].len = I2C_MSG_LENGTH;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = data;

	for (retry_count=0; retry_count<BF3A20_I2C_RETRY_COUNT; retry_count++) {
		err = i2c_transfer(client->adapter, msg, 2);
		if (err != 2) {
			if (err >= 0)
				err = -EIO;
			dev_err(&client->dev,"read from offset 0x%x error %d", reg, err);
		}else
       		break;
	}

	*val = 0;
	/* high byte comes first */
	if (data_length == BF3A20_8BIT)
		*val = (u8)data[0];
	else if (data_length == BF3A20_16BIT)
		*val = be16_to_cpu(*(u16 *)&data[0]);
	else
		*val = be32_to_cpu(*(u32 *)&data[0]);

	return 0;
}

static int bf3a20_i2c_write(struct i2c_client *client, u16 len, u8 *data)
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

static int bf3a20_write_reg(struct i2c_client *client, u16 data_length,
							u16 reg, u16 val)
{
	int ret;
	unsigned char data[2] = {0};
	const u16 len = 2;
	int retry_count = 0;

	if (data_length != BF3A20_8BIT && data_length != BF3A20_16BIT) {
		dev_err(&client->dev,
			"%s error, invalid data_length\n", __func__);
		return -EINVAL;
	}

	/* One byte data format */
	data[0] = reg;
	data[1] = val;

	for (retry_count=0; retry_count<BF3A20_I2C_RETRY_COUNT; retry_count++) {
		ret = bf3a20_i2c_write(client, len, data);
		if (!ret)
			break;
		if (ret)	
			dev_err(&client->dev,\
					"write error: wrote 0x%x to offset 0x%x error %d retry now\n",\
					val, reg, ret);
	}
	return ret;
}


static int bf3a20_write(struct v4l2_subdev *sd, unsigned char reg,
                   unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	ret = bf3a20_write_reg(client, 1, reg, value);
	if (ret < 0)
		return ret; 
	return 0;
}

static int bf3a20_write_array(struct v4l2_subdev *sd, const struct regval_list *vals)
{

	int ret;

	while (vals->reg_num != BF3A20_REG_END)
	{
		ret = bf3a20_write(sd, vals->reg_num, vals->value);
		/* The below sensor registers needs delay */
		if(vals->reg_num == 0x11 ||  vals->reg_num == 0x09 ||  vals->reg_num == 0xf9)
			mdelay(20);

			if (ret < 0)
				return ret;
		vals++;
	}
	return 0;
}

static int bf3a20_g_focal(struct v4l2_subdev *sd, s32 *val)
{
	*val = (BF3A20_FOCAL_LENGTH_NUM << 16) | BF3A20_FOCAL_LENGTH_DEM;
	return 0;
}

static int bf3a20_g_fnumber(struct v4l2_subdev *sd, s32 *val)
{
	/*const f number */
	*val = (BF3A20_F_NUMBER_DEFAULT_NUM << 16) | BF3A20_F_NUMBER_DEM;
	return 0;
}

static int bf3a20_g_fnumber_range(struct v4l2_subdev *sd, s32 *val)
{
	*val = (BF3A20_F_NUMBER_DEFAULT_NUM << 24) |
		(BF3A20_F_NUMBER_DEM << 16) |
		(BF3A20_F_NUMBER_DEFAULT_NUM << 8) | BF3A20_F_NUMBER_DEM;
	return 0;
}

static int bf3a20_get_intg_factor(struct i2c_client *client,
				struct camera_mipi_info *info,
				const struct bf3a20_resolution *res)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
	struct atomisp_sensor_mode_data *buf = &info->data;
	const unsigned int ext_clk_freq_hz = 19200000;
	const unsigned int pll_invariant_div = 10;
	unsigned int pix_clk_freq_hz;
	u16 pre_pll_clk_div;
	u16 pll_multiplier;
	u16 op_pix_clk_div;

	if (info == NULL)
		return -EINVAL;

	/* pixel clock calculation */
	pre_pll_clk_div = 1;

	pll_multiplier = 1;
	op_pix_clk_div = 1;
	pix_clk_freq_hz = ext_clk_freq_hz / pre_pll_clk_div * pll_multiplier
				* op_pix_clk_div/pll_invariant_div;

	dev->vt_pix_clk_freq_mhz = res->pix_clk_freq;
	buf->vt_pix_clk_freq_mhz = res->pix_clk_freq;

	/* get integration time */
	buf->coarse_integration_time_min = BF3A20_COARSE_INTG_TIME_MIN;
	buf->coarse_integration_time_max_margin =
					BF3A20_COARSE_INTG_TIME_MAX_MARGIN;

	buf->fine_integration_time_min = BF3A20_FINE_INTG_TIME_MIN;
	buf->fine_integration_time_max_margin =
					BF3A20_FINE_INTG_TIME_MAX_MARGIN;

	buf->fine_integration_time_def = BF3A20_FINE_INTG_TIME_MIN;
	buf->frame_length_lines = res->lines_per_frame;
	buf->line_length_pck = res->pixels_per_line;
	buf->read_mode = res->bin_mode;

	/* get the cropping and output resolution to ISP for this mode. */
	buf->crop_horizontal_start = res->crop_h_start;
	buf->crop_vertical_start = res->crop_v_start;
	buf->crop_horizontal_end = res->crop_h_end;
	buf->crop_vertical_end = res->crop_v_end;
	buf->output_width = res->output_width;
	buf->output_height = res->output_height;
	buf->binning_factor_x = res->bin_factor_x ?
					res->bin_factor_x : 1;
	buf->binning_factor_y = res->bin_factor_y ?
					res->bin_factor_y : 1;

	return 0;
}

static long __bf3a20_set_exposure(struct v4l2_subdev *sd, int coarse_itg,
				 int gain, int digitgain)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	/* set exposure */
	ret = bf3a20_write_reg(client, BF3A20_8BIT,
					BF3A20_COARSE_INTG_TIME_MSB, (coarse_itg >> 8) & 0xff);
	ret = bf3a20_write_reg(client, BF3A20_8BIT,
					BF3A20_COARSE_INTG_TIME_LSB, (coarse_itg) & 0xff);

	/* set analog gain */
	if (gain > 0x4f)
		gain = 0x4f;
	else if (gain < 0x18)
		gain = 0x18;
	bf3a20_write_reg(client,BF3A20_8BIT, BF3A20_GLOBAL_GAIN0_REG, gain);

	return 0;
}

static int bf3a20_set_exposure(struct v4l2_subdev *sd, int exposure,
	int gain, int digitgain)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
	int ret;

	mutex_lock(&dev->input_lock);
	ret = __bf3a20_set_exposure(sd, exposure, gain, digitgain);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static long bf3a20_s_exposure(struct v4l2_subdev *sd,
			       struct atomisp_exposure *exposure)
{
	int exp = exposure->integration_time[0];
	int gain = exposure->gain[0];
	int digitgain = exposure->gain[1];

	return bf3a20_set_exposure(sd, exp, gain, digitgain);
}

static long bf3a20_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{

	switch (cmd) {
	case ATOMISP_IOC_S_EXPOSURE:
		return bf3a20_s_exposure(sd, arg);
	default:
		return -EINVAL;
	}
	return 0;
}

/* This returns the exposure time being used. This should only be used
   for filling in EXIF data, not for actual image processing. */
static int bf3a20_q_exposure(struct v4l2_subdev *sd, s32 *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 high, low, exposure_value;
	int ret;

	/* get exposure */
	ret = bf3a20_read_reg(client, BF3A20_8BIT, BF3A20_COARSE_INTG_TIME_MSB, &high);
	if (ret < 0) {
		return ret;
	}

	ret = bf3a20_read_reg(client, BF3A20_8BIT, BF3A20_COARSE_INTG_TIME_LSB, &low);
	if (ret < 0) {
		return ret;
	}

	exposure_value = ((((u16) high) << 8) | (u16) low);
	*value = exposure_value;

	return ret;
}

struct bf3a20_control bf3a20_controls[] = {
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
		.query = bf3a20_q_exposure,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCAL_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focal length",
			.minimum = BF3A20_FOCAL_LENGTH_DEFAULT,
			.maximum = BF3A20_FOCAL_LENGTH_DEFAULT,
			.step = 0x01,
			.default_value = BF3A20_FOCAL_LENGTH_DEFAULT,
			.flags = 0,
		},
		.query = bf3a20_g_focal,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number",
			.minimum = BF3A20_F_NUMBER_DEFAULT,
			.maximum = BF3A20_F_NUMBER_DEFAULT,
			.step = 0x01,
			.default_value = BF3A20_F_NUMBER_DEFAULT,
			.flags = 0,
		},
		.query = bf3a20_g_fnumber,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_RANGE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number range",
			.minimum = BF3A20_F_NUMBER_RANGE,
			.maximum =  BF3A20_F_NUMBER_RANGE,
			.step = 0x01,
			.default_value = BF3A20_F_NUMBER_RANGE,
			.flags = 0,
		},
		.query = bf3a20_g_fnumber_range,
	},
};
#define N_CONTROLS (ARRAY_SIZE(bf3a20_controls))

static struct bf3a20_control *bf3a20_find_control(u32 id)
{
	int i;

	for (i = 0; i < N_CONTROLS; i++)
		if (bf3a20_controls[i].qc.id == id)
			return &bf3a20_controls[i];

	return NULL;
}

static int bf3a20_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	struct bf3a20_control *ctrl = bf3a20_find_control(qc->id);
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);

	if (ctrl == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	*qc = ctrl->qc;
	mutex_unlock(&dev->input_lock);

	return 0;
}

/* control set/get */
static int bf3a20_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct bf3a20_control *s_ctrl;
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
	int ret;

	if (!ctrl)
		return -EINVAL;

	s_ctrl = bf3a20_find_control(ctrl->id);
	if ((s_ctrl == NULL) || (s_ctrl->query == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = s_ctrl->query(sd, &ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int bf3a20_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct bf3a20_control *octrl = bf3a20_find_control(ctrl->id);
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
	int ret;

	if ((octrl == NULL) || (octrl->tweak == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = octrl->tweak(sd, ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int bf3a20_init(struct v4l2_subdev *sd)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);

	mutex_lock(&dev->input_lock);

	/* restore settings */
	bf3a20_res = bf3a20_res_preview;
	N_RES = N_RES_PREVIEW;

	mutex_unlock(&dev->input_lock);

	return 0;
}

static int power_up(struct v4l2_subdev *sd)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	if (NULL == dev->platform_data) {
		dev_err(&client->dev,
			"no camera_sensor_platform_data");
		return -ENODEV;
	}
	/* flis clock control */
	ret = dev->platform_data->flisclk_ctrl(sd, 1);
	if (ret)
		goto fail_clk;

	msleep(10);

	/* power control */
	ret = dev->platform_data->power_ctrl(sd, 1);
	if (ret)
		goto fail_power;

	msleep(10);

	/* gpio ctrl */
	ret = dev->platform_data->gpio_ctrl(sd, 1);
	if (ret) {
		ret = dev->platform_data->gpio_ctrl(sd, 1);
		if (ret)
			goto fail_power;
	}

	msleep(10);

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
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	if (NULL == dev->platform_data) {
		dev_err(&client->dev,
			"no camera_sensor_platform_data");
		return -ENODEV;
	}

	/* gpio ctrl */
	ret = dev->platform_data->gpio_ctrl(sd, 0);
	if (ret) {
		ret = dev->platform_data->gpio_ctrl(sd, 0);
		if (ret)
			dev_err(&client->dev, "gpio failed 2\n");
	}

	msleep(10);
	/* power control */
	ret = dev->platform_data->power_ctrl(sd, 0);
	if (ret)
		dev_err(&client->dev, "vprog failed.\n");

	msleep(10);
	ret = dev->platform_data->flisclk_ctrl(sd, 0);
	if (ret)
		dev_err(&client->dev, "flisclk failed\n");

	return ret;
}

static int bf3a20_s_power(struct v4l2_subdev *sd, int on)
{
	int ret;

	if (on == 0)
		return power_down(sd);
	else {
		ret = power_up(sd); {
		if (!ret)
			return bf3a20_init(sd);
		}
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
static int distance(struct bf3a20_resolution *res, u32 w, u32 h)
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

/* return the nearest higher resolution index */
static int nearest_resolution_index(int w, int h)
{
	int i;
	int idx = -1;
	int dist;
	int min_dist = INT_MAX;
	struct bf3a20_resolution *tmp_res = NULL;

	for (i = 0; i < N_RES; i++) {
		tmp_res = &bf3a20_res[i];
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
		if (w != bf3a20_res[i].width)
			continue;
		if (h != bf3a20_res[i].height)
			continue;

		return i;
	}

	return -1;
}

static int bf3a20_try_mbus_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *fmt)
{
	int idx;

	if (!fmt)
		return -EINVAL;
    
	idx = nearest_resolution_index(fmt->width, fmt->height);
	if (idx == -1) {
		/* if requested resolutions larger than the supported
		 * resolutions,then return the largest resolution
		 * that sensor can support */
		fmt->width = bf3a20_res[N_RES - 1].width;
		fmt->height = bf3a20_res[N_RES - 1].height;
	} else {
		/* return the requested resolutions */
		fmt->width = bf3a20_res[idx].width;
		fmt->height = bf3a20_res[idx].height;
	}
	fmt->code = V4L2_MBUS_FMT_SGBRG8_1X8;

	return 0;
}

static int startup(struct v4l2_subdev *sd)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 pwdn_reg;
	int ret = 0;

	/* load recommended data */
	ret = bf3a20_write_array(sd, bf3a20_init_sensor);
	if (ret) {
		dev_err(&client->dev, "bf3a20 write register err.\n");
		return ret;
	}

	/* load default timing data */
	ret = bf3a20_write_array(sd, bf3a20_res[dev->fmt_idx].regs);
	if (ret) {
		dev_err(&client->dev, "bf3a20 write register err.\n");
		return ret;
	}

	ret = bf3a20_read_reg(client, BF3A20_8BIT, BF3A20_PWDN_REG, &pwdn_reg);
	pwdn_reg &= BF3A20_START_STREAMING;
	ret = bf3a20_write_reg(client, 1, BF3A20_PWDN_REG, pwdn_reg);
	register_reset = 0;

	return ret;
}

static int bf3a20_s_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_mipi_info *bf3a20_info = NULL;
	int ret = 0;

	bf3a20_info = v4l2_get_subdev_hostdata(sd);
	if (bf3a20_info == NULL)
		return -EINVAL;
	
	mutex_lock(&dev->input_lock);
	ret = bf3a20_try_mbus_fmt(sd, fmt);
	if (ret == -1) {
		dev_err(&client->dev, "try fmt fail\n");
		goto err;
	}
 
	dev->fmt_idx = get_resolution_index(fmt->width, fmt->height);
	if (dev->fmt_idx == -1) {
		dev_err(&client->dev, "get resolution fail\n");
		mutex_unlock(&dev->input_lock);
		return -EINVAL;
	}

	ret = startup(sd);
	if (ret) {
		dev_err(&client->dev, "bf3a20 startup err\n");
		goto err;
	}

	/* get the exspoure/again/dgain from sensor  */
	ret = bf3a20_get_intg_factor(client, bf3a20_info,
					&bf3a20_res[dev->fmt_idx]);
	if (ret)
		dev_err(&client->dev, "failed to get integration_factor\n");

err:
	mutex_unlock(&dev->input_lock);
	return ret;
}
static int bf3a20_g_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);

	if (!fmt)
		return -EINVAL;

	fmt->width = bf3a20_res[dev->fmt_idx].width;
	fmt->height = bf3a20_res[dev->fmt_idx].height;
	fmt->code = V4L2_MBUS_FMT_SGBRG8_1X8;

	return 0;
}

static int bf3a20_detect(struct i2c_client *client)
{
	struct i2c_adapter *adapter = client->adapter;
	u16 high=0, low=0;
	u16 id;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -ENODEV;

	/* Read Product ID*/
	ret = bf3a20_read_reg(client, BF3A20_8BIT, BF3A20_PID_BME, &high);
	if (ret) {
		dev_err(&client->dev, "sensor_id_high = 0x%x\n", high);
		return -ENODEV;
	}

	ret = bf3a20_read_reg(client, BF3A20_8BIT, BF3A20_VER_BME, &low);
	if (ret) {
		dev_err(&client->dev, "sensor_id_low = 0x%x\n", low);
		return -ENODEV;
	}

	id = ((((u16) low) << 8) | (u16) high);
	if (id != BF3A20_PRODUCT_ID) {
		dev_err(&client->dev, "sensor ID error\n");
		return -ENODEV;
	}
	dev_dbg(&client->dev, "bf3a20 sensor product ID = 0x%x\n", id);

	return 0;
}

static int bf3a20_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	u16 pwdn_reg;

	mutex_lock(&dev->input_lock);

	ret = bf3a20_read_reg(client, BF3A20_8BIT, BF3A20_PWDN_REG, &pwdn_reg);
	if (enable) {
		pwdn_reg &= BF3A20_START_STREAMING;
	}
	else {
		pwdn_reg |= BF3A20_STOP_STREAMING;
		register_reset = 1;
	}

	ret = bf3a20_write_reg(client, 1, BF3A20_PWDN_REG, pwdn_reg);

	if(enable && register_reset) {
		mdelay(10);
		startup(sd);
		ret = bf3a20_write_reg(client, 1, BF3A20_PWDN_REG, pwdn_reg);
    }

	mutex_unlock(&dev->input_lock);
	return ret;
}

/* bf3a20 enum frame size, frame intervals */
static int bf3a20_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	unsigned int index = fsize->index;

	if (index >= N_RES)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = bf3a20_res[index].width;
	fsize->discrete.height = bf3a20_res[index].height;
	fsize->reserved[0] = bf3a20_res[index].used;

	return 0;
}

static int bf3a20_enum_frameintervals(struct v4l2_subdev *sd,
				      struct v4l2_frmivalenum *fival)
{
	unsigned int index = fival->index;

	if (index >= N_RES)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->width = bf3a20_res[index].width;
	fival->height = bf3a20_res[index].height;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = bf3a20_res[index].fps;

	return 0;
}

static int bf3a20_enum_mbus_fmt(struct v4l2_subdev *sd,
				unsigned int index,
				enum v4l2_mbus_pixelcode *code)
{
	*code = V4L2_MBUS_FMT_SGBRG8_1X8;

	return 0;
}

static int bf3a20_s_config(struct v4l2_subdev *sd,
			   int irq, void *platform_data)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
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
		dev_err(&client->dev, "bf3a20 power-off err.\n");
		goto fail_power_off;
	}

	ret = power_up(sd);
	if (ret) {
		dev_err(&client->dev, "bf3a20 power-up err.\n");
		goto fail_power_on;
	}

	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret)
		goto fail_csi_cfg;

	/* config & detect sensor */
	ret = bf3a20_detect(client);
	if (ret) {
		dev_err(&client->dev, "bf3a20_detect err in s_config.\n");
		goto fail_csi_cfg;
	}

	/* turn off sensor, after probed */
	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "bf3a20 power-off err.\n");
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

static int bf3a20_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
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
			bf3a20_res[dev->fmt_idx].fps;
	}
	return 0;
}

static int bf3a20_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
	dev->run_mode = param->parm.capture.capturemode;

	mutex_lock(&dev->input_lock);
	switch (dev->run_mode) {
	case CI_MODE_VIDEO:
		bf3a20_res = bf3a20_res_video;
		N_RES = N_RES_VIDEO;
		break;
	case CI_MODE_STILL_CAPTURE:
		bf3a20_res = bf3a20_res_still;
		N_RES = N_RES_STILL;
		break;
	default:
		bf3a20_res = bf3a20_res_preview;
		N_RES = N_RES_PREVIEW;
	}
	mutex_unlock(&dev->input_lock);

	return 0;
}

static int bf3a20_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);

	interval->interval.numerator = 1;
	interval->interval.denominator = bf3a20_res[dev->fmt_idx].fps;

	return 0;
}

static int bf3a20_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= MAX_FMTS)
		return -EINVAL;

	code->code = V4L2_MBUS_FMT_SGBRG8_1X8;
	return 0;
}

static int bf3a20_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_frame_size_enum *fse)
{
	int index = fse->index;

	if (index >= N_RES)
		return -EINVAL;

	fse->min_width = bf3a20_res[index].width;
	fse->min_height = bf3a20_res[index].height;
	fse->max_width = bf3a20_res[index].width;
	fse->max_height = bf3a20_res[index].height;

	return 0;

}

static struct v4l2_mbus_framefmt *
__bf3a20_get_pad_format(struct bf3a20_device *sensor,
			struct v4l2_subdev_fh *fh, unsigned int pad,
			enum v4l2_subdev_format_whence which)
{
	struct i2c_client *client = v4l2_get_subdevdata(&sensor->sd);

	if (pad != 0) {
		dev_err(&client->dev,
			"__bf3a20_get_pad_format err. pad %x\n", pad);
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

static int bf3a20_get_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct bf3a20_device *snr = to_bf3a20_sensor(sd);
	struct v4l2_mbus_framefmt *format =
			__bf3a20_get_pad_format(snr, fh, fmt->pad, fmt->which);
	if (!format)
		return -EINVAL;

	fmt->format = *format;
	return 0;
}

static int bf3a20_set_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct bf3a20_device *snr = to_bf3a20_sensor(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		snr->format = fmt->format;

	return 0;
}

static int bf3a20_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);

	mutex_lock(&dev->input_lock);
	*frames = bf3a20_res[dev->fmt_idx].skip_frames;
	mutex_unlock(&dev->input_lock);

	return 0;
}

static const struct v4l2_subdev_sensor_ops bf3a20_sensor_ops = {
	.g_skip_frames	= bf3a20_g_skip_frames,
};

static const struct v4l2_subdev_video_ops bf3a20_video_ops = {
	.s_stream = bf3a20_s_stream,
	.g_parm = bf3a20_g_parm,
	.s_parm = bf3a20_s_parm,
	.enum_framesizes = bf3a20_enum_framesizes,
	.enum_frameintervals = bf3a20_enum_frameintervals,
	.enum_mbus_fmt = bf3a20_enum_mbus_fmt,
	.try_mbus_fmt = bf3a20_try_mbus_fmt,
	.g_mbus_fmt = bf3a20_g_mbus_fmt,
	.s_mbus_fmt = bf3a20_s_mbus_fmt,
	.g_frame_interval = bf3a20_g_frame_interval,
};

static const struct v4l2_subdev_core_ops bf3a20_core_ops = {
	.s_power = bf3a20_s_power,
	.queryctrl = bf3a20_queryctrl,
	.g_ctrl = bf3a20_g_ctrl,
	.s_ctrl = bf3a20_s_ctrl,
	.ioctl = bf3a20_ioctl,
};

static const struct v4l2_subdev_pad_ops bf3a20_pad_ops = {
	.enum_mbus_code = bf3a20_enum_mbus_code,
	.enum_frame_size = bf3a20_enum_frame_size,
	.get_fmt = bf3a20_get_pad_format,
	.set_fmt = bf3a20_set_pad_format,
};

static const struct v4l2_subdev_ops bf3a20_ops = {
	.core = &bf3a20_core_ops,
	.video = &bf3a20_video_ops,
	.pad = &bf3a20_pad_ops,
	.sensor = &bf3a20_sensor_ops,
};

static int bf3a20_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct bf3a20_device *dev = to_bf3a20_sensor(sd);
	dev_dbg(&client->dev, "bf3a20_remove...\n");

	dev->platform_data->csi_cfg(sd, 0);

	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	kfree(dev);

	return 0;
}

static int bf3a20_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct bf3a20_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&client->dev, "out of memory\n");
		return -ENOMEM;
	}

	mutex_init(&dev->input_lock);

	dev->fmt_idx = 0;
	v4l2_i2c_subdev_init(&(dev->sd), client, &bf3a20_ops);

	if (client->dev.platform_data) {
		ret = bf3a20_s_config(&dev->sd, client->irq,
				       client->dev.platform_data);
		if (ret)
			goto out_free;
	}

	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dev->pad.flags = MEDIA_PAD_FL_SOURCE;
	dev->format.code = V4L2_MBUS_FMT_SGBRG8_1X8;
	dev->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;

	ret = media_entity_init(&dev->sd.entity, 1, &dev->pad, 0);
	if (ret)
		bf3a20_remove(client);

	dev_err(&client->dev,"bf3a20: sensor probe is success.\n");

	return ret;

out_free:
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);
	return ret;
}

MODULE_DEVICE_TABLE(i2c, bf3a20_id);
static struct i2c_driver bf3a20_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = BF3A20_NAME,
	},
	.probe = bf3a20_probe,
	.remove = bf3a20_remove,
	.id_table = bf3a20_id,
};

static int init_bf3a20(void)
{
	return i2c_add_driver(&bf3a20_driver);
}

static void exit_bf3a20(void)
{

	i2c_del_driver(&bf3a20_driver);
}

module_init(init_bf3a20);
module_exit(exit_bf3a20);

MODULE_AUTHOR("senthilnathan, selvamani <senthilnathan.selvamani@intel.com>");
MODULE_DESCRIPTION("A low-level driver for BYD BF3A20 sensors");
MODULE_LICENSE("GPL");
