/*
 * Support for OmniVision OV8858 5M camera sensor.
 * Based on OmniVision OV5648 driver.
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

#include "ov8858.h"


#define OV8858_DEBUG_EN 1
#define ov8858_debug dev_err

#if 0
static enum atomisp_bayer_order ov8858_bayer_order_mapping[] = {
	atomisp_bayer_order_gbrg,
	atomisp_bayer_order_bggr,
	atomisp_bayer_order_grbg,
	atomisp_bayer_order_rggb
};
#endif
static int g_vflip_flag = 0;
static int g_hflip_flag = 0;
extern int update_awb_gain(struct v4l2_subdev *sd);

/* i2c read/write stuff */
static int ov8858_read_reg(struct i2c_client *client,
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

	if (data_length != OV8858_8BIT && data_length != OV8858_16BIT
					&& data_length != OV8858_32BIT) {
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
	if (data_length == OV8858_8BIT)
		*val = (u8)data[0];
	else if (data_length == OV8858_16BIT)
		*val = be16_to_cpu(*(u16 *)&data[0]);
	else
		*val = be32_to_cpu(*(u32 *)&data[0]);
	//ov8858_debug(&client->dev,  "++++i2c read adr%x = %x\n", reg,*val);
	return 0;
}

static int ov8858_i2c_write(struct i2c_client *client, u16 len, u8 *data)
{
	struct i2c_msg msg;
	const int num_msg = 1;
	int ret;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = len;
	msg.buf = data;
	ret = i2c_transfer(client->adapter, &msg, 1);
	//ov8858_debug(&client->dev,  "+++i2c write reg=%x->%x\n", data[0]*256 +data[1],data[2]);
	return ret == num_msg ? 0 : -EIO;
}

static int ov8858_write_reg(struct i2c_client *client, u16 data_length,
							u16 reg, u16 val)
{
	int ret;
	unsigned char data[4] = {0};
	u16 *wreg = (u16 *)data;
	const u16 len = data_length + sizeof(u16); /* 16-bit address + data */

	if (data_length != OV8858_8BIT && data_length != OV8858_16BIT) {
		dev_err(&client->dev,
			"%s error, invalid data_length\n", __func__);
		return -EINVAL;
	}

	/* high byte goes out first */
	*wreg = cpu_to_be16(reg);

	if (data_length == OV8858_8BIT) {
		data[2] = (u8)(val);
	} else {
		/* OV8858_16BIT */
		u16 *wdata = (u16 *)&data[2];
		*wdata = cpu_to_be16(val);
	}

	ret = ov8858_i2c_write(client, len, data);
	if (ret)
		dev_err(&client->dev,
			"write error: wrote 0x%x to offset 0x%x error %d",
			val, reg, ret);
	//ov8858_debug(&client->dev, "+++++++__wr  adr %x = %x\n",reg,val);

	return ret;
}

/*
 * ov8858_write_reg_array - Initializes a list of OV8858 registers
 * @client: i2c driver client structure
 * @reglist: list of registers to be written
 *
 * This function initializes a list of registers. When consecutive addresses
 * are found in a row on the list, this function creates a buffer and sends
 * consecutive data in a single i2c_transfer().
 *
 * __ov8858_flush_reg_array, __ov8858_buf_reg_array() and
 * __ov8858_write_reg_is_consecutive() are internal functions to
 * ov8858_write_reg_array_fast() and should be not used anywhere else.
 *
 */

static int __ov8858_flush_reg_array(struct i2c_client *client,
				    struct ov8858_write_ctrl *ctrl)
{
	u16 size;

	if (ctrl->index == 0)
		return 0;

	size = sizeof(u16) + ctrl->index; /* 16-bit address + data */
	ctrl->buffer.addr = cpu_to_be16(ctrl->buffer.addr);
	ctrl->index = 0;

	return ov8858_i2c_write(client, size, (u8 *)&ctrl->buffer);
}

static int __ov8858_buf_reg_array(struct i2c_client *client,
				  struct ov8858_write_ctrl *ctrl,
				  const struct ov8858_reg *next)
{
	int size;
	u16 *data16;

	switch (next->type) {
	case OV8858_8BIT:
		size = 1;
		ctrl->buffer.data[ctrl->index] = (u8)next->val;
		break;
	case OV8858_16BIT:
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
	if (ctrl->index + sizeof(u16) >= OV8858_MAX_WRITE_BUF_SIZE)
		return __ov8858_flush_reg_array(client, ctrl);

	return 0;
}

static int __ov8858_write_reg_is_consecutive(struct i2c_client *client,
					     struct ov8858_write_ctrl *ctrl,
					     const struct ov8858_reg *next)
{
	if (ctrl->index == 0)
		return 1;

	return ctrl->buffer.addr + ctrl->index == next->reg;
}

static int ov8858_write_reg_array(struct i2c_client *client,
				  const struct ov8858_reg *reglist)
{
	const struct ov8858_reg *next = reglist;
	struct ov8858_write_ctrl ctrl;
	int err;
	u8 data[3];
	//ov8858_debug(&client->dev,  "++++write reg array\n");
	ctrl.index = 0;
	for (; next->type != OV8858_TOK_TERM; next++) {
		switch (next->type & OV8858_TOK_MASK) {
		case OV8858_TOK_DELAY:
			err = __ov8858_flush_reg_array(client, &ctrl);
			if (err)
				return err;
			msleep(next->val);
			break;
		default:
			/*
			 * If next address is not consecutive, data needs to be
			 * flushed before proceed.
			 */
			//ov8858_debug(&client->dev,  "+++%%reg=%x->%x\n", next->reg,next->val);
			if (!__ov8858_write_reg_is_consecutive(client, &ctrl,
								next)) {
				err = __ov8858_flush_reg_array(client, &ctrl);
			if (err)
				return err;
			}
			err = __ov8858_buf_reg_array(client, &ctrl, next);
			if (err) {
				dev_err(&client->dev, "%s: write error, aborted\n",
					 __func__);
				return err;
			}
			break;
		}
	}

	return __ov8858_flush_reg_array(client, &ctrl);
}
static int ov8858_g_focal(struct v4l2_subdev *sd, s32 *val)
{
	*val = (OV8858_FOCAL_LENGTH_NUM << 16) | OV8858_FOCAL_LENGTH_DEM;
	return 0;
}

static int ov8858_g_fnumber(struct v4l2_subdev *sd, s32 *val)
{
	/*const f number for imx*/
	*val = (OV8858_F_NUMBER_DEFAULT_NUM << 16) | OV8858_F_NUMBER_DEM;
	return 0;
}

static int ov8858_g_fnumber_range(struct v4l2_subdev *sd, s32 *val)
{
	*val = (OV8858_F_NUMBER_DEFAULT_NUM << 24) |
		(OV8858_F_NUMBER_DEM << 16) |
		(OV8858_F_NUMBER_DEFAULT_NUM << 8) | OV8858_F_NUMBER_DEM;
	return 0;
}

static int ov8858_g_bin_factor_x(struct v4l2_subdev *sd, s32 *val)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	//ov8858_debug(dev,  "++++ov8858_g_bin_factor_x\n");
	*val = ov8858_res[dev->fmt_idx].bin_factor_x;

	return 0;
}

static int ov8858_g_bin_factor_y(struct v4l2_subdev *sd, s32 *val)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);

	*val = ov8858_res[dev->fmt_idx].bin_factor_y;

	return 0;
}

static int ov8858_get_intg_factor(struct i2c_client *client,
				struct camera_mipi_info *info,
				const struct ov8858_resolution *res)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	struct atomisp_sensor_mode_data *buf = &info->data;
	unsigned int pix_clk_freq_hz;
	u16 reg_val;
	int ret;

	if (info == NULL)
		return -EINVAL;

	/* pixel clock */
	pix_clk_freq_hz = res->pix_clk_freq * 1000000;

	dev->vt_pix_clk_freq_mhz = pix_clk_freq_hz;
	buf->vt_pix_clk_freq_mhz = pix_clk_freq_hz;

	/* get integration time */
	buf->coarse_integration_time_min = OV8858_COARSE_INTG_TIME_MIN;
	buf->coarse_integration_time_max_margin =
					OV8858_COARSE_INTG_TIME_MAX_MARGIN;

	buf->fine_integration_time_min = OV8858_FINE_INTG_TIME_MIN;
	buf->fine_integration_time_max_margin =
					OV8858_FINE_INTG_TIME_MAX_MARGIN;

	buf->fine_integration_time_def = OV8858_FINE_INTG_TIME_MIN;
	buf->frame_length_lines = res->lines_per_frame;
	buf->line_length_pck = res->pixels_per_line;
	buf->read_mode = res->bin_mode;

	/* get the cropping and output resolution to ISP for this mode. */
	ret =  ov8858_read_reg(client, OV8858_16BIT,
					OV8858_HORIZONTAL_START_H, &reg_val);
	if (ret)
		return ret;
	buf->crop_horizontal_start = reg_val;

	ret =  ov8858_read_reg(client, OV8858_16BIT,
					OV8858_VERTICAL_START_H, &reg_val);
	if (ret)
		return ret;
	buf->crop_vertical_start = reg_val;

	ret = ov8858_read_reg(client, OV8858_16BIT,
					OV8858_HORIZONTAL_END_H, &reg_val);
	if (ret)
		return ret;
	buf->crop_horizontal_end = reg_val;

	ret = ov8858_read_reg(client, OV8858_16BIT,
					OV8858_VERTICAL_END_H, &reg_val);
	if (ret)
		return ret;
	buf->crop_vertical_end = reg_val;

	ret = ov8858_read_reg(client, OV8858_16BIT,
					OV8858_HORIZONTAL_OUTPUT_SIZE_H, &reg_val);
	if (ret)
		return ret;
	buf->output_width = reg_val;

	ret = ov8858_read_reg(client, OV8858_16BIT,
					OV8858_VERTICAL_OUTPUT_SIZE_H, &reg_val);
	if (ret)
		return ret;
	buf->output_height = reg_val;

	buf->binning_factor_x = res->bin_factor_x ?
					res->bin_factor_x : 1;
	buf->binning_factor_y = res->bin_factor_y ?
					res->bin_factor_y : 1;

	return 0;
}

static long __ov8858_set_exposure(struct v4l2_subdev *sd, int coarse_itg,
				 int gain, int digitgain)

{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	u16 vts,hts;
	int ret,exp_val,vts_val;
   // printk(KERN_ERR "__ov8858_set_exposure coarse_itg %d, gain %d, digitgain %d++\n",coarse_itg, gain, digitgain);

	if (dev->run_mode == CI_MODE_VIDEO)
		ov8858_res = ov8858_res_video;
	else if (dev->run_mode == CI_MODE_STILL_CAPTURE)
		ov8858_res = ov8858_res_still;
	else
		ov8858_res = ov8858_res_preview;


	hts = ov8858_res[dev->fmt_idx].pixels_per_line;
	vts = ov8858_res[dev->fmt_idx].lines_per_frame;

  /* group hold */
       ret = ov8858_write_reg(client, OV8858_8BIT,
                                       OV8858_GROUP_ACCESS, 0x00);
  if (ret)
    return ret;

	/* Increase the VTS to match exposure + 8 */
	if (coarse_itg + OV8858_INTEGRATION_TIME_MARGIN > vts)
		vts_val = coarse_itg + OV8858_INTEGRATION_TIME_MARGIN;
	else
		vts_val = vts;
	
	//printk(KERN_ERR "__ov8858_set_exposure  0x%x,0x%x,0x%x,++\n",vts_val,(vts_val & 0xFF),((vts_val >> 8) & 0xFF));
	ret = ov8858_write_reg(client, OV8858_8BIT, OV8858_TIMING_VTS_H, (vts_val >> 8) & 0xFF);
	if (ret)
		return ret;
	ret = ov8858_write_reg(client, OV8858_8BIT, OV8858_TIMING_VTS_L, vts_val & 0xFF);
	if (ret)
		return ret;
	

	/* set exposure */
	/* Lower four bit should be 0*/
	exp_val = coarse_itg << 4;

	ret = ov8858_write_reg(client, OV8858_8BIT,
			       OV8858_EXPOSURE_L, exp_val & 0xFF);
	if (ret)
		return ret;

	ret = ov8858_write_reg(client, OV8858_8BIT,
			       OV8858_EXPOSURE_M, (exp_val >> 8) & 0xFF);
	if (ret)
		return ret;

	ret = ov8858_write_reg(client, OV8858_8BIT,
			       OV8858_EXPOSURE_H, (exp_val >> 16) & 0x0F);
	if (ret)
		return ret;

	/* Digital gain TODO:CHECK FURTHER*/
	//if (digitgain) {
	//	ret = ov8858_write_reg(client, OV8858_16BIT,OV8858_DIGI_GAIN_H, digitgain);
	//	if (ret)
	//		return ret;
	//}

    /* Analog gain */
    ret = ov8858_write_reg(client, OV8858_8BIT, OV8858_AGC_L, gain & 0xff);
    if (ret)
		return ret;

    ret = ov8858_write_reg(client, OV8858_8BIT, OV8858_AGC_H, (gain >> 8) & 0xff);
    if (ret)
		return ret;

	/* End group */
	ret = ov8858_write_reg(client, OV8858_8BIT,
			       OV8858_GROUP_ACCESS, 0x10);
	if (ret)
		return ret;

	/* Delay launch group */
	ret = ov8858_write_reg(client, OV8858_8BIT,
					   OV8858_GROUP_ACCESS, 0xa0);
	if (ret)
		return ret;

	return ret;
}

static int ov8858_set_exposure(struct v4l2_subdev *sd, int exposure,
	int gain, int digitgain)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	int ret;

	mutex_lock(&dev->input_lock);
	ret = __ov8858_set_exposure(sd, exposure, gain, digitgain);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static long ov8858_s_exposure(struct v4l2_subdev *sd,
			       struct atomisp_exposure *exposure)
{
	int exp = exposure->integration_time[0];
	int gain = exposure->gain[0];
	int digitgain = exposure->gain[1];

	/* we should not accept the invalid value below. */
	if (gain == 0) {
		struct i2c_client *client = v4l2_get_subdevdata(sd);
		v4l2_err(client, "%s: invalid value\n", __func__);
		return -EINVAL;
	}

	return ov8858_set_exposure(sd, exp, gain, digitgain);
}

static long ov8858_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{

	switch (cmd) {
	case ATOMISP_IOC_S_EXPOSURE:
		return ov8858_s_exposure(sd, arg);
	default:
		return -EINVAL;
	}
	return 0;
}

/* This returns the exposure time being used. This should only be used
   for filling in EXIF data, not for actual image processing. */
static int ov8858_q_exposure(struct v4l2_subdev *sd, s32 *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 reg_v, reg_v2;
	int ret;

	/* get exposure */
	ret = ov8858_read_reg(client, OV8858_8BIT,
					OV8858_EXPOSURE_L,
					&reg_v);
	if (ret)
		goto err;

	ret = ov8858_read_reg(client, OV8858_8BIT,
					OV8858_EXPOSURE_M,
					&reg_v2);
	if (ret)
		goto err;

	reg_v += reg_v2 << 8;
	ret = ov8858_read_reg(client, OV8858_8BIT,
					OV8858_EXPOSURE_H,
					&reg_v2);
	if (ret)
		goto err;

	*value = reg_v + (((u32)reg_v2 << 16));
err:
	return ret;
}

int ov8858_vcm_power_up(struct v4l2_subdev *sd)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->power_up)
		return dev->vcm_driver->power_up(sd);
	return 0;
}

int ov8858_vcm_power_down(struct v4l2_subdev *sd)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->power_down)
		return dev->vcm_driver->power_down(sd);
	return 0;
}

int ov8858_vcm_init(struct v4l2_subdev *sd)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->init)
		return dev->vcm_driver->init(sd);
	return 0;
}

int ov8858_t_focus_vcm(struct v4l2_subdev *sd, u16 val)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_focus_vcm)
		return dev->vcm_driver->t_focus_vcm(sd, val);
	return 0;
}

int ov8858_t_focus_abs(struct v4l2_subdev *sd, s32 value)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_focus_abs)
		return dev->vcm_driver->t_focus_abs(sd, value);
	return 0;
}
int ov8858_t_focus_rel(struct v4l2_subdev *sd, s32 value)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_focus_rel)
		return dev->vcm_driver->t_focus_rel(sd, value);
	return 0;
}

int ov8858_q_focus_status(struct v4l2_subdev *sd, s32 *value)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->q_focus_status)
		return dev->vcm_driver->q_focus_status(sd, value);
	return 0;
}

int ov8858_q_focus_abs(struct v4l2_subdev *sd, s32 *value)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->q_focus_abs)
		return dev->vcm_driver->q_focus_abs(sd, value);
	return 0;
}

int ov8858_t_vcm_slew(struct v4l2_subdev *sd, s32 value)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_vcm_slew)
		return dev->vcm_driver->t_vcm_slew(sd, value);
	return 0;
}

int ov8858_t_vcm_timing(struct v4l2_subdev *sd, s32 value)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_vcm_timing)
		return dev->vcm_driver->t_vcm_timing(sd, value);
	return 0;
}

#if 0
static enum v4l2_mbus_pixelcode
ov8858_translate_bayer_order(enum atomisp_bayer_order code)
{
	switch (code) {
	case atomisp_bayer_order_rggb:
		return V4L2_MBUS_FMT_SRGGB10_1X10;
	case atomisp_bayer_order_grbg:
		return V4L2_MBUS_FMT_SGRBG10_1X10;
	case atomisp_bayer_order_bggr:
		return V4L2_MBUS_FMT_SBGGR10_1X10;
	case atomisp_bayer_order_gbrg:
		return V4L2_MBUS_FMT_SGBRG10_1X10;
	}
	return 0;
}
#endif
static int ov8858_v_flip(struct v4l2_subdev *sd, s32 value)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	struct camera_mipi_info *ov8858_info = NULL;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	u16 val;
	u8 flip_flag;
	//ov8858_debug(&client->dev, "@%s: value:%d\n", __func__, value);
	ret = ov8858_read_reg(client, OV8858_8BIT, OV8858_VFLIP_REG, &val);
	if (ret)
		return ret;
	if (value) {
		val |= OV8858_VFLIP_VALUE;
		g_vflip_flag = 1;
	} else {
		val &= ~OV8858_VFLIP_VALUE;
		g_vflip_flag = 0;
	}
	ret = ov8858_write_reg(client, OV8858_8BIT,
			OV8858_VFLIP_REG, val);
	if (ret)
		return ret;
#if 0
	ov8858_info = v4l2_get_subdev_hostdata(sd);
	if (ov8858_info) {
		flip_flag = (g_vflip_flag<<1|g_hflip_flag) & 0x03;
		ov8858_info->raw_bayer_order = ov8858_bayer_order_mapping[flip_flag];
		dev->format.code = ov8858_translate_bayer_order(
			ov8858_info->raw_bayer_order);
	}
#endif
	return ret;
}

static int ov8858_h_flip(struct v4l2_subdev *sd, s32 value)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	struct camera_mipi_info *ov8858_info = NULL;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	u16 val;
	u8 flip_flag;
	//ov8858_debug(&client->dev, "@%s: value:%d\n", __func__, value);

	ret = ov8858_read_reg(client, OV8858_8BIT, OV8858_HFLIP_REG, &val);
	if (ret)
		return ret;
	if (value) {
		val |= OV8858_HFLIP_VALUE;
		g_hflip_flag = 1;
	} else {
		val &= ~OV8858_HFLIP_VALUE;
		g_hflip_flag = 0;
	}
	ret = ov8858_write_reg(client, OV8858_8BIT,
			OV8858_HFLIP_REG, val);
	if (ret)
		return ret;
#if 0
	ov8858_info = v4l2_get_subdev_hostdata(sd);
	if (ov8858_info) {
		flip_flag = (g_vflip_flag<<1|g_hflip_flag) & 0x03;
		ov8858_info->raw_bayer_order = ov8858_bayer_order_mapping[flip_flag];
		dev->format.code = ov8858_translate_bayer_order(
		ov8858_info->raw_bayer_order);
	}
#endif
	return ret;
}

static int ov8858_flip(struct v4l2_subdev *sd)
{
	int ret = 0;

	if(g_hflip_flag){
		ret = ov8858_h_flip(sd,1);
	}else if(g_vflip_flag){
		ret = ov8858_v_flip(sd,1);
	}

	return ret;
}

struct ov8858_control ov8858_controls[] = {
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
		.query = ov8858_q_exposure,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCAL_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focal length",
			.minimum = OV8858_FOCAL_LENGTH_DEFAULT,
			.maximum = OV8858_FOCAL_LENGTH_DEFAULT,
			.step = 0x01,
			.default_value = OV8858_FOCAL_LENGTH_DEFAULT,
			.flags = 0,
		},
		.query = ov8858_g_focal,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number",
			.minimum = OV8858_F_NUMBER_DEFAULT,
			.maximum = OV8858_F_NUMBER_DEFAULT,
			.step = 0x01,
			.default_value = OV8858_F_NUMBER_DEFAULT,
			.flags = 0,
		},
		.query = ov8858_g_fnumber,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_RANGE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number range",
			.minimum = OV8858_F_NUMBER_RANGE,
			.maximum =  OV8858_F_NUMBER_RANGE,
			.step = 0x01,
			.default_value = OV8858_F_NUMBER_RANGE,
			.flags = 0,
		},
		.query = ov8858_g_fnumber_range,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCUS_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focus move absolute",
			.minimum = 0,
			.maximum = VCM_MAX_FOCUS_POS,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = ov8858_t_focus_abs,
		.query = ov8858_q_focus_abs,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCUS_RELATIVE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focus move relative",
			.minimum = OV8858_MAX_FOCUS_NEG,
			.maximum = OV8858_MAX_FOCUS_POS,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = ov8858_t_focus_rel,
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
		.query = ov8858_q_focus_status,
	},
	{
		.qc = {
			.id = V4L2_CID_VCM_SLEW,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "vcm slew",
			.minimum = 0,
			.maximum = OV8858_VCM_SLEW_STEP_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = ov8858_t_vcm_slew,
	},
	{
		.qc = {
			.id = V4L2_CID_VCM_TIMEING,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "vcm step time",
			.minimum = 0,
			.maximum = OV8858_VCM_SLEW_TIME_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = ov8858_t_vcm_timing,
	},
	{
		.qc = {
			.id = V4L2_CID_BIN_FACTOR_HORZ,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "horizontal binning factor",
			.minimum = 0,
			.maximum = OV8858_BIN_FACTOR_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = ov8858_g_bin_factor_x,
	},
	{
		.qc = {
			.id = V4L2_CID_BIN_FACTOR_VERT,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "vertical binning factor",
			.minimum = 0,
			.maximum = OV8858_BIN_FACTOR_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = ov8858_g_bin_factor_y,
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
		.tweak = ov8858_v_flip,
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
		.tweak = ov8858_h_flip,
	},
};
#define N_CONTROLS (ARRAY_SIZE(ov8858_controls))

static struct ov8858_control *ov8858_find_control(u32 id)
{
	int i;

	for (i = 0; i < N_CONTROLS; i++)
		if (ov8858_controls[i].qc.id == id)
			return &ov8858_controls[i];
	return NULL;
}

static int ov8858_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	struct ov8858_control *ctrl = ov8858_find_control(qc->id);
	struct ov8858_device *dev = to_ov8858_sensor(sd);

	if (ctrl == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	*qc = ctrl->qc;
	mutex_unlock(&dev->input_lock);

	return 0;
}

/* imx control set/get */
static int ov8858_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ov8858_control *s_ctrl;
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	int ret;

	if (!ctrl)
		return -EINVAL;

	s_ctrl = ov8858_find_control(ctrl->id);
	if ((s_ctrl == NULL) || (s_ctrl->query == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = s_ctrl->query(sd, &ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int ov8858_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ov8858_control *octrl = ov8858_find_control(ctrl->id);
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	int ret;

	if ((octrl == NULL) || (octrl->tweak == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = octrl->tweak(sd, ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}


static int ov8858_init_registers(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	ret = ov8858_write_reg(client, OV8858_8BIT, OV8858_SW_RESET, 0x01);
	ret |= ov8858_write_reg_array(client, ov8858_BasicSettings);

	return ret;
}

static int ov8858_init(struct v4l2_subdev *sd)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	mutex_lock(&dev->input_lock);

	/* restore settings */
	ov8858_res = ov8858_res_preview;
	N_RES = N_RES_PREVIEW;
	
	ret = ov8858_init_registers(sd);
	if (ret) {
		dev_err(&client->dev, "ov8858 reset err.\n");
		return ret;
	}

	mutex_unlock(&dev->input_lock);

	return 0;
}


static int power_up(struct v4l2_subdev *sd)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	dev_err(&client->dev, "@%s: \n", __func__);
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
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

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

static int ov8858_s_power(struct v4l2_subdev *sd, int on)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
	
	int ret = 0;
	if (on == 0) {
		ret = power_down(sd);
		if (dev->vcm_driver && dev->vcm_driver->power_down)
		{
			ret |= dev->vcm_driver->power_down(sd);
			
		}
	} else {
		if (dev->vcm_driver && dev->vcm_driver->power_up)
			ret = dev->vcm_driver->power_up(sd);
		if (ret)
			return ret;

		ret |= power_up(sd);
		if (!ret)
			return ov8858_init(sd);
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
static int distance(struct ov8858_resolution *res, u32 w, u32 h)
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
	struct ov8858_resolution *tmp_res = NULL;

	for (i = 0; i < N_RES; i++) {
		tmp_res = &ov8858_res[i];
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
		if (w != ov8858_res[i].width)
			continue;
		if (h != ov8858_res[i].height)
			continue;

		return i;
	}

	return -1;
}

static int ov8858_try_mbus_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *fmt)
{
	int idx;

	if (!fmt)
		return -EINVAL;
	idx = nearest_resolution_index(fmt->width,
					fmt->height);
	if (idx == -1) {
		/* return the largest resolution */
		fmt->width = ov8858_res[N_RES - 1].width;
		fmt->height = ov8858_res[N_RES - 1].height;
	} else {
		fmt->width = ov8858_res[idx].width;
		fmt->height = ov8858_res[idx].height;
	}
	fmt->code = V4L2_MBUS_FMT_SGRBG10_1X10;

	return 0;
}

/* TODO: remove it. */
static int startup(struct v4l2_subdev *sd)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	
	int ret = 0;
	/*
	ret = ov8858_write_reg(client, OV8858_8BIT,
					OV8858_SW_RESET, 0x01);
	if (ret) {
		dev_err(&client->dev, "ov8858 reset err.\n");
		return ret;
	}*/
	ret = ov8858_write_reg_array(client, ov8858_res[dev->fmt_idx].regs);
	if (ret) {
		dev_err(&client->dev, "ov8858 write register err.\n");
		return ret;
	}
	//ov8858_debug(&client->dev,  "start up dev->current_otp.otp_en= %x\n", dev->current_otp.otp_en);
	if(dev->current_otp.otp_en == 1)
	{
		update_awb_gain(sd);
		
	}
	return ret;
}

static int ov8858_s_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_mipi_info *ov8858_info = NULL;
	int ret = 0;

	ov8858_info = v4l2_get_subdev_hostdata(sd);
	if (ov8858_info == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = ov8858_try_mbus_fmt(sd, fmt);
	if (ret == -1) {
		dev_err(&client->dev, "try fmt fail\n");
		goto err;
	}
	printk("@%s:width:%d, height:%d\n", __func__, fmt->width, fmt->height);
	dev->fmt_idx = get_resolution_index(fmt->width,
					      fmt->height);
	if (dev->fmt_idx == -1) {
		dev_err(&client->dev, "get resolution fail\n");
		mutex_unlock(&dev->input_lock);
		return -EINVAL;
	}

	v4l2_info(client, "__s_mbus_fmt i=%d, w=%d, h=%d\n", dev->fmt_idx,
		  fmt->width, fmt->height);


	ret = startup(sd);
	if (ret)
		dev_err(&client->dev, "ov8858 startup err\n");
		

	ret = ov8858_write_reg_array(client, ov8858_res[dev->fmt_idx].regs);
	if (ret)
		dev_err(&client->dev, "ov5693 write resolution register err\n");

	ret = ov8858_get_intg_factor(client, ov8858_info,
					&ov8858_res[dev->fmt_idx]);
	if (ret) {
		dev_err(&client->dev, "failed to get integration_factor\n");
		goto err;
	}

	v4l2_info(client,"\n%s idx %d \n", __func__, dev->fmt_idx);

err:
	mutex_unlock(&dev->input_lock);
	return ret;
}
static int ov8858_g_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);

	if (!fmt)
		return -EINVAL;

	fmt->width = ov8858_res[dev->fmt_idx].width;
	fmt->height = ov8858_res[dev->fmt_idx].height;
	fmt->code = V4L2_MBUS_FMT_SBGGR10_1X10;

	return 0;
}

static int ov8858_detect(struct i2c_client *client)
{
	struct i2c_adapter *adapter = client->adapter;
	u16 high, low;
	int ret;
	u16 id;
	u8 revision;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -ENODEV;

	ret = ov8858_read_reg(client, OV8858_8BIT,
					OV8858_SC_CMMN_CHIP_ID_H, &high);
	if (ret) {
		dev_err(&client->dev, "sensor_id_high = 0x%x\n", high);
		return -ENODEV;
	}
	ret = ov8858_read_reg(client, OV8858_8BIT,
					OV8858_SC_CMMN_CHIP_ID_L, &low);
	id = ((((u16) high) << 8) | (u16) low);

	if (id != OV8858_ID) {
		dev_err(&client->dev, "sensor ID error\n");
		return -ENODEV;
	}

	ret = ov8858_read_reg(client, OV8858_8BIT,
					OV8858_SC_CMMN_SUB_ID, &high);
	revision = (u8) high & 0x0f;

	ov8858_debug(&client->dev, "sensor_revision id  = 0x%x\n", id);
	ov8858_debug(&client->dev, "detect ov8858 success\n");
	ov8858_debug(&client->dev, "################1##########\n");
	return 0;
}

static int ov8858_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	printk("@%s:\n", __func__);
	mutex_lock(&dev->input_lock);

    ret = ov8858_flip(sd);
	if(ret){
		dev_info(&client->dev, "Set flip failed!\n");
	}

	ret = ov8858_write_reg(client, OV8858_8BIT, OV8858_SW_STREAM,
				enable ? OV8858_START_STREAMING :
				OV8858_STOP_STREAMING);

	mutex_unlock(&dev->input_lock);

	return ret;
}

/* ov8858 enum frame size, frame intervals */
static int ov8858_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	unsigned int index = fsize->index;

	if (index >= N_RES)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = ov8858_res[index].width;
	fsize->discrete.height = ov8858_res[index].height;
	fsize->reserved[0] = ov8858_res[index].used;

	return 0;
}

static int ov8858_enum_frameintervals(struct v4l2_subdev *sd,
				      struct v4l2_frmivalenum *fival)
{
	unsigned int index = fival->index;

	if (index >= N_RES)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->width = ov8858_res[index].width;
	fival->height = ov8858_res[index].height;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = ov8858_res[index].fps;

	return 0;
}

static int ov8858_enum_mbus_fmt(struct v4l2_subdev *sd,
				unsigned int index,
				enum v4l2_mbus_pixelcode *code)
{
	*code = V4L2_MBUS_FMT_SBGGR10_1X10;

	return 0;
}

int ov8858_read_reg2(struct i2c_client *client, u16 data_length, u16 reg)
{
	u16 val = 0;
	int ret = 0;
	ret = ov8858_read_reg(client,data_length,reg,&val);
	//ov8858_debug(&client->dev,  "++++ read otp reg%x = %x\n", reg,&val);
	return (int)val;
}

// index: index of otp group. (1, 2, 3)
// return:0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
int check_otp_info(struct i2c_client *client,int index)
{
    int flag=0;
	ov8858_write_reg(client,OV8858_8BIT,0x3d84, 0xC0);
	//partial mode OTP write start address
	ov8858_write_reg(client,OV8858_8BIT,0x3d88, 0x70);
	ov8858_write_reg(client,OV8858_8BIT,0x3d89, 0x10);
	//partial mode OTP write end address
	ov8858_write_reg(client,OV8858_8BIT,0x3d8A, 0x70);
	ov8858_write_reg(client,OV8858_8BIT,0x3d8B, 0x10);
	// read otp into buffer
	ov8858_write_reg(client,OV8858_8BIT,0x3d81, 0x01);
	msleep(5);
	flag = ov8858_read_reg2(client,OV8858_8BIT,0x7010);
	//select group
	if (index == 1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if (index == 2)
	{
		flag = (flag>>4) & 0x03;
	}
	else if (index ==3)
	{
		flag = (flag>>2) & 0x03;
	}
	// clear otp buffer
	ov8858_write_reg(client,OV8858_8BIT,0x7010, 0x00);
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}
// index: index of otp group. (1, 2, 3)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data

int check_otp_wb(struct i2c_client *client,int index)
{
	int flag=0;
	ov8858_write_reg(client,OV8858_8BIT,0x3d84, 0xC0);
	//partial mode OTP write start address
	ov8858_write_reg(client,OV8858_8BIT,0x3d88, 0x70);
	ov8858_write_reg(client,OV8858_8BIT,0x3d89, 0x20);
	// partial mode OTP write end address
	ov8858_write_reg(client,OV8858_8BIT,0x3d8A, 0x70);
	ov8858_write_reg(client,OV8858_8BIT,0x3d8B, 0x20);
	// read otp into buffer
	ov8858_write_reg(client,OV8858_8BIT,0x3d81, 0x01);
	msleep(5);
	//select group
	flag = ov8858_read_reg2(client,OV8858_8BIT,0x7020);
	
	printk("debug otp flad=%d", flag);
	if (index == 1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if (index == 2)
	{
		flag = (flag>>4) & 0x03;
	}
	else if (index == 3)
	{
		flag = (flag>>2) & 0x03;
	}
	// clear otp buffer
	ov8858_write_reg(client,OV8858_8BIT,0x7020, 0x00);
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}
// index: index of otp group. (1, 2, 3)
// code: 0 for start code, 1 for stop code
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
int check_otp_VCM(struct i2c_client *client,int index, int code)
{
	int flag=0;
	ov8858_write_reg(client,OV8858_8BIT,0x3d84, 0xC0);
	//partial mode OTP write start address
	ov8858_write_reg(client,OV8858_8BIT,0x3d88, 0x70);
	ov8858_write_reg(client,OV8858_8BIT,0x3d89, 0x30);
	// partial mode OTP write end address
	ov8858_write_reg(client,OV8858_8BIT,0x3d8A, 0x70);
	ov8858_write_reg(client,OV8858_8BIT,0x3d8B, 0x30);
	// read otp into buffer
	ov8858_write_reg(client,OV8858_8BIT,0x3d81, 0x01);
	msleep(5);
	//select group
	flag = ov8858_read_reg2(client,OV8858_8BIT,0x7030);
	if (index == 1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if (index == 2)
	{
		flag = (flag>>4) & 0x03;
	}
	else if (index == 3)
	{
		flag = (flag>>2) & 0x03;
	}
	// clear otp buffer
	ov8858_write_reg(client,OV8858_8BIT,0x7030, 0x00);
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}
// index: index of otp group. (1, 2, 3)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
int check_otp_lenc(struct i2c_client *client,int index)
{
	int flag=0;
	ov8858_write_reg(client,OV8858_8BIT,0x3d84, 0xC0);
	//partial mode OTP write start address
	ov8858_write_reg(client,OV8858_8BIT,0x3d88, 0x70);
	ov8858_write_reg(client,OV8858_8BIT,0x3d89, 0x3A);
	// partial mode OTP write end address
	ov8858_write_reg(client,OV8858_8BIT,0x3d8A, 0x70);
	ov8858_write_reg(client,OV8858_8BIT,0x3d8B, 0x3A);
	// read otp into buffer
	ov8858_write_reg(client,OV8858_8BIT,0x3d81, 0x01);
	msleep(5);
	flag = ov8858_read_reg2(client,OV8858_8BIT,0x703a);
	if (index == 1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if (index == 2)
	{
		flag = (flag>>4) & 0x03;
	}
	else if (index == 3)	
	{
		flag = (flag>> 2)& 0x03;
	}
	// clear otp buffer
	ov8858_write_reg(client,OV8858_8BIT, 0x703a, 0x00);
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}
// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of otp_struct
// return: 0,
int read_otp_info(struct i2c_client *client,int index, struct ov8858_otp_struct *otp_ptr)
{
	int i;
	int start_addr, end_addr;
	if (index == 1) {
		start_addr = 0x7011;
		end_addr = 0x7015;
	}
	else if (index == 2) {
		start_addr = 0x7016;
		end_addr = 0x701a;
	}
	else if (index == 3) {
		start_addr = 0x701b;
		end_addr = 0x701f;
	}
	ov8858_write_reg(client,OV8858_8BIT,0x3d84, 0xC0);
	//partial mode OTP write start address
	ov8858_write_reg(client,OV8858_8BIT,0x3d88, (start_addr >> 8) & 0xff);
	ov8858_write_reg(client,OV8858_8BIT,0x3d89, start_addr & 0xff);
	// partial mode OTP write end address
	ov8858_write_reg(client,OV8858_8BIT,0x3d8A, (end_addr >> 8) & 0xff);
	ov8858_write_reg(client,OV8858_8BIT,0x3d8B, end_addr & 0xff);
	// read otp into buffer
	ov8858_write_reg(client,OV8858_8BIT,0x3d81, 0x01);
	msleep(5);	
	otp_ptr->module_integrator_id = ov8858_read_reg2(client,OV8858_8BIT,start_addr);
	otp_ptr->lens_id = ov8858_read_reg2(client,OV8858_8BIT,(start_addr + 1));
	otp_ptr->production_year = ov8858_read_reg2(client,OV8858_8BIT,(start_addr + 2));
	otp_ptr->production_month = ov8858_read_reg2(client,OV8858_8BIT,(start_addr + 3));
	otp_ptr->production_day = ov8858_read_reg2(client,OV8858_8BIT,(start_addr + 4));
	// clear otp buffer
	for (i=start_addr; i<=end_addr; i++) {
		ov8858_write_reg(client,OV8858_8BIT,i, 0x00);
	}
	return 0;
}
// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of otp_struct
// return: 0,
int read_otp_wb(struct i2c_client *client,int index, struct ov8858_otp_struct *otp_ptr)
{
	int i;
	int temp;
	int start_addr, end_addr;
	if (index == 1) {
		start_addr = 0x7021;
		end_addr = 0x7025;
	}
	else if (index == 2) {
		start_addr = 0x7026;
		end_addr = 0x702a;
	}
	else if (index == 3) {
		start_addr = 0x702b;
		end_addr = 0x702f;
	}
	ov8858_write_reg(client,OV8858_8BIT,0x3d84, 0xC0);
	//partial mode OTP write start address
	ov8858_write_reg(client,OV8858_8BIT,0x3d88, (start_addr >> 8) & 0xff);
	ov8858_write_reg(client,OV8858_8BIT,0x3d89, start_addr & 0xff);
	// partial mode OTP write end address
	ov8858_write_reg(client,OV8858_8BIT,0x3d8A, (end_addr >> 8) & 0xff);
	ov8858_write_reg(client,OV8858_8BIT,0x3d8B, end_addr & 0xff);
	// read otp into buffer
	ov8858_write_reg(client,OV8858_8BIT,0x3d81, 0x01);
	msleep(5);
	temp = ov8858_read_reg2(client,OV8858_8BIT,start_addr + 4);
	otp_ptr->rg_ratio = ((ov8858_read_reg2(client,OV8858_8BIT,start_addr))<<2) + ((temp>>6) & 0x03);
	otp_ptr->bg_ratio = (ov8858_read_reg2(client,OV8858_8BIT, start_addr+1)<<2) + ((temp>>4) & 0x03);
	otp_ptr->light_rg = (ov8858_read_reg2(client,OV8858_8BIT, start_addr+2)<<2) + ((temp>>2) & 0x03);
	otp_ptr->light_bg = (ov8858_read_reg2(client,OV8858_8BIT, start_addr+3)<<2) + (temp & 0x03);
	// clear otp buffer
	for (i=start_addr; i<=end_addr; i++) {
		ov8858_write_reg(client,OV8858_8BIT,i, 0x00);
	}
	return 0;
}
// index: index of otp group. (1, 2, 3)
// code: 0 start code, 1 stop code
// return: 0
int read_otp_VCM(struct i2c_client *client,int index, struct ov8858_otp_struct * otp_ptr)
{
	int i;
	int temp=0;
	int start_addr, end_addr;
	if (index == 1) {
		start_addr = 0x7031;
		end_addr = 0x7033;
	}
	else if (index == 2) {
		start_addr = 0x7034;
		end_addr = 0x7036;
	}
	else if (index == 3) {
		start_addr = 0x7037;
		end_addr = 0x7039;
	}
	ov8858_write_reg(client,OV8858_8BIT,0x3d84, 0xC0);
	//ov8858_write_reg(client,OV8858_8BIT,0x3d84, 0x40);
	//partial mode OTP write start address
	ov8858_write_reg(client,OV8858_8BIT,0x3d88, (start_addr >> 8) & 0xff);
	ov8858_write_reg(client,OV8858_8BIT,0x3d89, start_addr & 0xff);
	// partial mode OTP write end address
	ov8858_write_reg(client,OV8858_8BIT,0x3d8A, (end_addr >> 8) & 0xff);
	ov8858_write_reg(client,OV8858_8BIT,0x3d8B, end_addr & 0xff);
	// read otp into buffer
	ov8858_write_reg(client,OV8858_8BIT,0x3d81, 0x01);
	msleep(5);
	//flag and lsb of VCM start code
	temp = ov8858_read_reg2(client,OV8858_8BIT,start_addr+2);
	otp_ptr->VCM_start = (ov8858_read_reg2(client,OV8858_8BIT,start_addr)<<2) | ((temp>>6) & 0x03);
	otp_ptr->VCM_end = (ov8858_read_reg2(client,OV8858_8BIT,start_addr + 1) << 2) | ((temp>>4) & 0x03);
	otp_ptr->VCM_dir = (temp>>2) & 0x03;
	// clear otp buffer
	for (i=start_addr; i<=end_addr; i++) {
	ov8858_write_reg(client,OV8858_8BIT,i, 0x00);
	}
	return 0;
}

// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of otp_struct
// return: 0,
int read_otp_lenc(struct i2c_client *client,int index, struct ov8858_otp_struct *otp_ptr)
{
	int i;
	int start_addr, end_addr;
	if (index == 1) {
		start_addr = 0x703b;
		end_addr = 0x70a8;
	}
	else if (index == 2) {
		start_addr = 0x70a9;
		end_addr = 0x7116;
	}
	else if (index == 3) {
		start_addr = 0x7117;
		end_addr = 0x7184;
	}
	ov8858_write_reg(client,OV8858_8BIT,0x3d84, 0xC0);
	//partial mode OTP write start address
	ov8858_write_reg(client,OV8858_8BIT,0x3d88, (start_addr >> 8) & 0xff);
	ov8858_write_reg(client,OV8858_8BIT,0x3d89, start_addr & 0xff);
	// partial mode OTP write end address
	ov8858_write_reg(client,OV8858_8BIT,0x3d8A, (end_addr >> 8) & 0xff);
	ov8858_write_reg(client,OV8858_8BIT,0x3d8B, end_addr & 0xff);
	// read otp into buffer
	ov8858_write_reg(client,OV8858_8BIT,0x3d81, 0x01);
	msleep(10);
	for(i=0; i<110; i++) {
		otp_ptr->lenc[i]=ov8858_read_reg2(client,OV8858_8BIT,start_addr + i);
	}
	// clear otp buffer
	for (i=start_addr; i<=end_addr; i++) {
		ov8858_write_reg(client,OV8858_8BIT,i, 0x00);
	}
	return 0;
	}
// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;

int update_awb_gain(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	
	int R_gain = dev->current_otp.R_gain;
	int G_gain = dev->current_otp.G_gain;
	int B_gain = dev->current_otp.B_gain;
	if (R_gain>0x400) {
		ov8858_write_reg(client,OV8858_8BIT,0x5032, R_gain>>8);
		ov8858_write_reg(client,OV8858_8BIT,0x5033, R_gain & 0x00ff);
	}
	if (G_gain>0x400) {
		ov8858_write_reg(client,OV8858_8BIT,0x5034, G_gain>>8);
		ov8858_write_reg(client,OV8858_8BIT,0x5035, G_gain & 0x00ff);
	}
	if (B_gain>0x400) {
		ov8858_write_reg(client,OV8858_8BIT,0x5036, B_gain>>8);
		ov8858_write_reg(client,OV8858_8BIT,0x5037, B_gain & 0x00ff);
	}
    #ifdef OV8858_DEBUG_EN
	ov8858_debug(&client->dev, "_ov8858_update_: %s :rgain:%x ggain %x bgain %x\n",__func__,R_gain,G_gain,B_gain);
	#endif
	return 0;
}

// otp_ptr: pointer of otp_struct
int update_lenc(struct i2c_client *client,struct ov8858_otp_struct *otp_ptr)
{
   // struct i2c_client *client = v4l2_get_subdevdata(sd);
	//struct ov8858_device *dev = to_ov8858_sensor(sd);
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, temp=0;
	temp = ov8858_read_reg2(client,OV8858_8BIT,0x5000);
	temp = 0x80 | temp;
	ov8858_write_reg(client,OV8858_8BIT,0x5000, temp);
	for(i=0;i<110;i++) {
		ov8858_write_reg(client,OV8858_8BIT,0x5800 + i, otp_ptr->lenc[i]);
	}
	return 0;
	}
	
// call this function after OV8858 initialization
// return value: 0 update success
// 1, no OTP
int  __ov8858_read_otp_wb(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	//struct otp_struct current_otp;
	int i;
	int otp_index;
	int temp=0;
	int rg=1,bg=1;
	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	for(i=1;i<=3;i++) {
		temp = check_otp_wb(client,i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	//ov8858_debug(&client->dev,  "++++for check read_wb3 i = %d\n", i);
	if (i>3) { //3is not work
		// no valid wb OTP data
		return 1;
	}
	read_otp_wb(client,otp_index, &(dev->current_otp));
	
	if(dev->current_otp.light_rg==0) {
		// no light source information in OTP, light factor = 1
		rg = dev->current_otp.rg_ratio;
	}
	else {
		rg = dev->current_otp.rg_ratio * (dev->current_otp.light_rg +512) / 1024;
	}
	if(dev->current_otp.light_bg==0) {
		// not light source information in OTP, light factor = 1
		bg = dev->current_otp.bg_ratio;
	}
	else {
		bg = dev->current_otp.bg_ratio * (dev->current_otp.light_bg +512) / 1024;
	}
	#ifdef OV8858_DEBUG_EN
	ov8858_debug(&client->dev, "_ov8858_: %s :rg:%x bg %x\n",__func__,rg,bg);
	#endif
	
	if(rg == 0)
		rg = 1;
	if(bg == 0)
		bg = 1;
	//calculate G gain
	//1xgain=??
	int nR_G_gain, nB_G_gain, nG_G_gain; //set value =1000???
	int R_gain=0x400, B_gain=0x400, G_gain=0x400; //0x400=1xgain?
	int nBase_gain;
	nR_G_gain = (RG_Ratio_Typical*1000) / rg;
	nB_G_gain = (BG_Ratio_Typical*1000) / bg;
	nG_G_gain = 1000;
	if (nR_G_gain < 1000 || nB_G_gain < 1000)
	{
		if (nR_G_gain < nB_G_gain)
			nBase_gain = nR_G_gain;
		else
			nBase_gain = nB_G_gain;
	}
	else
	{
		nBase_gain = nG_G_gain;
	}

	R_gain = 0x400 * nR_G_gain / (nBase_gain);
	B_gain = 0x400 * nB_G_gain / (nBase_gain);
	G_gain = 0x400 * nG_G_gain / (nBase_gain);
	
	dev->current_otp.R_gain = R_gain;
	dev->current_otp.G_gain = G_gain;
	dev->current_otp.B_gain = B_gain;
	return 0;
}
// call this function after OV8858 initialization
// return value: 0 update success
// 1, no OTP
int  __ov8858_read_otp_lenc(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	//struct otp_struct current_otp;
	int i;
	int otp_index;
	int temp=0;
	// check first lens correction OTP with valid data
	for(i=1;i<=3;i++) {
		temp = check_otp_lenc(client,i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	//  ov8858_debug(&client->dev,  "++++for check read_lenc i = %d ,temp=%d\n", i,temp);
	}
	if (i>3) {
		// no valid WB OTP data
		return 1;
	}
	read_otp_lenc(client,otp_index, &(dev->current_otp));
	update_lenc(client,&(dev->current_otp));
	
	// success
	return 0;
}
	
static int ov8858_otp_read(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	int ret = 0;
	

	//otp valid after mipi on and sw stream on
	ret = ov8858_write_reg(client,OV8858_8BIT,
		OV8858_SW_STREAM,OV8858_START_STREAMING);


	//opt wb process read
	ret = __ov8858_read_otp_wb(sd);
	//ov8858_debug(&client->dev,"otp_wb_ret=%d",ret);
	if (ret) {
			dev_err(&client->dev, "failed to read OTP wb data\n");
			dev->current_otp.otp_en = 0;
	}
	//opt lenc process read
	ret = __ov8858_read_otp_lenc(sd);
	if (ret) {
			dev_err(&client->dev, "failed to read OTP lenc data\n");
			dev->current_otp.otp_en = 0;
	}
	ret = ov8858_write_reg(client,OV8858_8BIT,
		OV8858_SW_STREAM,OV8858_STOP_STREAMING);	
	return ret ;
}


static int ov8858_s_config(struct v4l2_subdev *sd,
			   int irq, void *platform_data)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	if (platform_data == NULL)
		return -ENODEV;

	dev->platform_data =
		(struct camera_sensor_platform_data *)platform_data;

	mutex_lock(&dev->input_lock);
	//* power off the module, then power on it in future
	// * as first power on by board may not fulfill the
	// * power on sequqence needed by the module
	// *
	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "ov8858 power-off err.\n");
		goto fail_power_off;
	}

	ret = power_up(sd);
	if (ret) {
		dev_err(&client->dev, "ov8858 power-up err.\n");
		goto fail_power_on;
	}

	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret)
		goto fail_csi_cfg;

	/* config & detect sensor */
	ret = ov8858_detect(client);
	if (ret) {
		dev_err(&client->dev, "ov8858_detect err s_config.\n");
		goto fail_csi_cfg;
	}
	if(dev->current_otp.otp_en == 1)
		ov8858_otp_read(sd);
	/* turn off sensor, after probed */
	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "ov8858 power-off err.\n");
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

static int ov8858_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
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
			ov8858_res[dev->fmt_idx].fps;
	}
	return 0;
}

static int ov8858_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);
      struct i2c_client *client = v4l2_get_subdevdata(sd); 
	dev->run_mode = param->parm.capture.capturemode;

	v4l2_info(client, "\n%s:run_mode :%x\n", __func__, dev->run_mode);

	mutex_lock(&dev->input_lock);
	switch (dev->run_mode) {
	case CI_MODE_VIDEO:
		ov8858_res = ov8858_res_video;
		N_RES = N_RES_VIDEO;
		break;
	case CI_MODE_STILL_CAPTURE:
		ov8858_res = ov8858_res_still;
		N_RES = N_RES_STILL;
		break;
	default:
		ov8858_res = ov8858_res_preview;
		N_RES = N_RES_PREVIEW;
	}
	mutex_unlock(&dev->input_lock);
	return 0;
}

static int ov8858_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);

	interval->interval.numerator = 1;
	interval->interval.denominator = ov8858_res[dev->fmt_idx].fps;

	return 0;
}

static int ov8858_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= MAX_FMTS)
		return -EINVAL;

	code->code = V4L2_MBUS_FMT_SBGGR10_1X10;
	return 0;
}

static int ov8858_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_frame_size_enum *fse)
{
	int index = fse->index;

	if (index >= N_RES)
		return -EINVAL;

	fse->min_width = ov8858_res[index].width;
	fse->min_height = ov8858_res[index].height;
	fse->max_width = ov8858_res[index].width;
	fse->max_height = ov8858_res[index].height;

	return 0;

}

static struct v4l2_mbus_framefmt *
__ov8858_get_pad_format(struct ov8858_device *sensor,
			struct v4l2_subdev_fh *fh, unsigned int pad,
			enum v4l2_subdev_format_whence which)
{
	struct i2c_client *client = v4l2_get_subdevdata(&sensor->sd);

	if (pad != 0) {
		dev_err(&client->dev,
			"__ov8858_get_pad_format err. pad %x\n", pad);
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

static int ov8858_get_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct ov8858_device *snr = to_ov8858_sensor(sd);
	struct v4l2_mbus_framefmt *format =
			__ov8858_get_pad_format(snr, fh, fmt->pad, fmt->which);
	if (!format)
		return -EINVAL;

	fmt->format = *format;
	return 0;
}

static int ov8858_set_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct ov8858_device *snr = to_ov8858_sensor(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		snr->format = fmt->format;

	return 0;
}

static int ov8858_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
	struct ov8858_device *dev = to_ov8858_sensor(sd);

	mutex_lock(&dev->input_lock);
	*frames = ov8858_res[dev->fmt_idx].skip_frames;
	mutex_unlock(&dev->input_lock);

	return 0;
}


static const struct v4l2_subdev_sensor_ops ov8858_sensor_ops = {
	.g_skip_frames	= ov8858_g_skip_frames,
};

static const struct v4l2_subdev_video_ops ov8858_video_ops = {
	.s_stream = ov8858_s_stream,
	.g_parm = ov8858_g_parm,
	.s_parm = ov8858_s_parm,
	.enum_framesizes = ov8858_enum_framesizes,
	.enum_frameintervals = ov8858_enum_frameintervals,
	.enum_mbus_fmt = ov8858_enum_mbus_fmt,
	.try_mbus_fmt = ov8858_try_mbus_fmt,
	.g_mbus_fmt = ov8858_g_mbus_fmt,
	.s_mbus_fmt = ov8858_s_mbus_fmt,
	.g_frame_interval = ov8858_g_frame_interval,
};

static const struct v4l2_subdev_core_ops ov8858_core_ops = {
	.s_power = ov8858_s_power,
	.queryctrl = ov8858_queryctrl,
	.g_ctrl = ov8858_g_ctrl,
	.s_ctrl = ov8858_s_ctrl,
	.ioctl = ov8858_ioctl,
};

static const struct v4l2_subdev_pad_ops ov8858_pad_ops = {
	.enum_mbus_code = ov8858_enum_mbus_code,
	.enum_frame_size = ov8858_enum_frame_size,
	.get_fmt = ov8858_get_pad_format,
	.set_fmt = ov8858_set_pad_format,
};

static const struct v4l2_subdev_ops ov8858_ops = {
	.core = &ov8858_core_ops,
	.video = &ov8858_video_ops,
	.pad = &ov8858_pad_ops,
	.sensor = &ov8858_sensor_ops,
};

static int ov8858_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov8858_device *dev = to_ov8858_sensor(sd);
	dev_dbg(&client->dev, "ov8858_remove...\n");

	dev->platform_data->csi_cfg(sd, 0);

	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	kfree(dev);

	return 0;
}

static int ov8858_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct ov8858_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&client->dev, "out of memory\n");
		return -ENOMEM;
	}

	mutex_init(&dev->input_lock);

	dev->fmt_idx = 0;
	//otp functions
	dev->current_otp.otp_en = 1;// enable otp functions
	v4l2_i2c_subdev_init(&(dev->sd), client, &ov8858_ops);

	if (client->dev.platform_data) {
		ret = ov8858_s_config(&dev->sd, client->irq,
				       client->dev.platform_data);
		if (ret)
			goto out_free;
	}
#ifdef CONFIG_VIDEO_WV511
	dev->vcm_driver = &ov8858_vcms[WV511];
	dev->vcm_driver->init(&dev->sd);
	dev_err(&client->dev, "CONFIG_VIDEO_WV511\n");
#elif defined (CONFIG_VIDEO_DW9714)
	dev->vcm_driver = &ov8858_vcms[DW9714];
	dev->vcm_driver->init(&dev->sd);
	dev_err(&client->dev, "CONFIG_VIDEO_DW9714\n");
#elif defined (CONFIG_VIDEO_VM149)
	dev->vcm_driver = &ov8858_vcms[VM149];
	dev->vcm_driver->init(&dev->sd);
	dev_err(&client->dev, "CONFIG_VIDEO_VM149\n");
#endif	

	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dev->pad.flags = MEDIA_PAD_FL_SOURCE;
	dev->format.code = V4L2_MBUS_FMT_SBGGR10_1X10;
	dev->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;

	ret = media_entity_init(&dev->sd.entity, 1, &dev->pad, 0);
	
	
	if (ret)
		ov8858_remove(client);

	return ret;
out_free:
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);
	return ret;
}

MODULE_DEVICE_TABLE(i2c, ov8858_id);
static struct i2c_driver ov8858_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = OV8858_NAME,
	},
	.probe = ov8858_probe,
	.remove = ov8858_remove,
	.id_table = ov8858_id,
};

static int init_ov8858(void)
{
	return i2c_add_driver(&ov8858_driver);
}

static void exit_ov8858(void)
{

	i2c_del_driver(&ov8858_driver);
}

module_init(init_ov8858);
module_exit(exit_ov8858);

MODULE_AUTHOR("Jacky Wang <Jacky_wang@ovt.com>");
MODULE_DESCRIPTION("A low-level driver for OmniVision 8858 sensors");
MODULE_LICENSE("GPL");
