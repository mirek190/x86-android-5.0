/*
 * Support for OmniVision OV5693 1080p HD camera sensor.
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

#include "ov5693.h"
#include "dw9714.h"

/* i2c read/write stuff */

//#define  ov5693_FLIP_BIT1  0x02
//#define  ov5693_FLIP_BIT2  0x04
//#define  ov5693_FLIP_BIT6  ( 0x04<<4 )
//#define  ov5693_VFLIP_MAKE 0x42
//#define  ov5693_HFLIP_MAKE 0x06
#define  ov5693_CTRL_HFLIP_ADDR   0x3821
#define  ov5693_CTRL_VFLIP_ADDR   0x3820
//#define to_ov5693_sensor(sd) container_of(sd, struct ov5693_device, sd)

static int ov5693_t_vflip(struct v4l2_subdev *sd, int value);
static int ov5693_t_hflip(struct v4l2_subdev *sd, int value);

static int  v_flag=0;
static int  h_flag=0;
/*
static enum v4l2_mbus_pixelcode ov5693_translate_bayer_order(enum atomisp_bayer_order code)
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

static enum atomisp_bayer_order ov5693_bayer_order_mapping[] = {

atomisp_bayer_order_bggr,
atomisp_bayer_order_gbrg,
atomisp_bayer_order_rggb,
atomisp_bayer_order_grbg,

};
*/

/* i2c read/write stuff */
static int ov5693_read_reg(struct i2c_client *client,
			   u16 data_length, u16 reg, u16 * val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[6];

	if (!client->adapter) {
		dev_err(&client->dev, "%s error, no client->adapter\n",
			__func__);
		return -ENODEV;
	}

	if (data_length != OV5693_8BIT && data_length != OV5693_16BIT
	    && data_length != OV5693_32BIT) {
		dev_err(&client->dev, "%s error, invalid data length\n",
			__func__);
		return -EINVAL;
	}

	memset(msg, 0, sizeof(msg));

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = I2C_MSG_LENGTH;
	msg[0].buf = data;

	/* high byte goes out first */
	data[0] = (u8) (reg >> 8);
	data[1] = (u8) (reg & 0xff);

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
	if (data_length == OV5693_8BIT)
		*val = (u8) data[0];
	else if (data_length == OV5693_16BIT)
		*val = be16_to_cpu(*(u16 *) & data[0]);
	else
		*val = be32_to_cpu(*(u32 *) & data[0]);

	return 0;
}

static int ov5693_i2c_write(struct i2c_client *client, u16 len, u8 * data)
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

static int ov5693_write_reg(struct i2c_client *client, u16 data_length,
			    u16 reg, u16 val)
{
	int ret;
	unsigned char data[4] = { 0 };
	u16 *wreg = (u16 *) data;
	const u16 len = data_length + sizeof(u16);	/* 16-bit address + data */

	if (data_length != OV5693_8BIT && data_length != OV5693_16BIT) {
		dev_err(&client->dev,
			"%s error, invalid data_length\n", __func__);
		return -EINVAL;
	}

	/* high byte goes out first */
	*wreg = cpu_to_be16(reg);

	if (data_length == OV5693_8BIT) {
		data[2] = (u8) (val);
	} else {
		/* OV5693_16BIT */
		u16 *wdata = (u16 *) & data[2];
		*wdata = cpu_to_be16(val);
	}

	ret = ov5693_i2c_write(client, len, data);
	if (ret)
		dev_err(&client->dev,
			"write error: wrote 0x%x to offset 0x%x error %d",
			val, reg, ret);

	return ret;
}

/*
 * ov5693_write_reg_array - Initializes a list of OV5693 registers
 * @client: i2c driver client structure
 * @reglist: list of registers to be written
 *
 * This function initializes a list of registers. When consecutive addresses
 * are found in a row on the list, this function creates a buffer and sends
 * consecutive data in a single i2c_transfer().
 *
 * __ov5693_flush_reg_array, __ov5693_buf_reg_array() and
 * __ov5693_write_reg_is_consecutive() are internal functions to
 * ov5693_write_reg_array_fast() and should be not used anywhere else.
 *
 */

static int __ov5693_flush_reg_array(struct i2c_client *client,
				    struct ov5693_write_ctrl *ctrl)
{
	u16 size;

	if (ctrl->index == 0)
		return 0;

	size = sizeof(u16) + ctrl->index;	/* 16-bit address + data */
	ctrl->buffer.addr = cpu_to_be16(ctrl->buffer.addr);
	ctrl->index = 0;

	return ov5693_i2c_write(client, size, (u8 *) & ctrl->buffer);
}

static int __ov5693_buf_reg_array(struct i2c_client *client,
				  struct ov5693_write_ctrl *ctrl,
				  const struct ov5693_reg *next)
{
	int size;
	u16 *data16;

	switch (next->type) {
	case OV5693_8BIT:
		size = 1;
		ctrl->buffer.data[ctrl->index] = (u8) next->val;
		break;
	case OV5693_16BIT:
		size = 2;
		data16 = (u16 *) & ctrl->buffer.data[ctrl->index];
		*data16 = cpu_to_be16((u16) next->val);
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
	if (ctrl->index + sizeof(u16) >= OV5693_MAX_WRITE_BUF_SIZE)
		return __ov5693_flush_reg_array(client, ctrl);

	return 0;
}

static int __ov5693_write_reg_is_consecutive(struct i2c_client *client,
					     struct ov5693_write_ctrl *ctrl,
					     const struct ov5693_reg *next)
{
	if (ctrl->index == 0)
		return 1;

	return ctrl->buffer.addr + ctrl->index == next->reg;
}

static int ov5693_write_reg_array(struct i2c_client *client,
				  const struct ov5693_reg *reglist)
{
	const struct ov5693_reg *next = reglist;
	struct ov5693_write_ctrl ctrl;
	int err;

	ctrl.index = 0;
	for (; next->type != OV5693_TOK_TERM; next++) {
		switch (next->type & OV5693_TOK_MASK) {
		case OV5693_TOK_DELAY:
			err = __ov5693_flush_reg_array(client, &ctrl);
			if (err)
				return err;
			msleep(next->val);
			break;
		default:
			/*
			 * If next address is not consecutive, data needs to be
			 * flushed before proceed.
			 */
			if (!__ov5693_write_reg_is_consecutive(client, &ctrl,
							       next)) {
				err = __ov5693_flush_reg_array(client, &ctrl);
				if (err)
					return err;
			}
			err = __ov5693_buf_reg_array(client, &ctrl, next);
			if (err) {
				dev_err(&client->dev,
					"%s: write error, aborted\n", __func__);
				return err;
			}
			break;
		}
	}

	return __ov5693_flush_reg_array(client, &ctrl);
}

static int ov5693_g_focal(struct v4l2_subdev *sd, s32 * val)
{
	*val = (OV5693_FOCAL_LENGTH_NUM << 16) | OV5693_FOCAL_LENGTH_DEM;
	return 0;
}

static int ov5693_g_fnumber(struct v4l2_subdev *sd, s32 * val)
{
	/*const f number for ov5693 */
	*val = (OV5693_F_NUMBER_DEFAULT_NUM << 16) | OV5693_F_NUMBER_DEM;
	return 0;
}

static int ov5693_g_fnumber_range(struct v4l2_subdev *sd, s32 * val)
{
	*val = (OV5693_F_NUMBER_DEFAULT_NUM << 24) |
	    (OV5693_F_NUMBER_DEM << 16) |
	    (OV5693_F_NUMBER_DEFAULT_NUM << 8) | OV5693_F_NUMBER_DEM;
	return 0;
}

static int ov5693_g_bin_factor_x(struct v4l2_subdev *sd, s32 *val)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);

	*val = ov5693_res[dev->fmt_idx].bin_factor_x;

	return 0;
}

static int ov5693_g_bin_factor_y(struct v4l2_subdev *sd, s32 *val)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);

	*val = ov5693_res[dev->fmt_idx].bin_factor_y;

	return 0;
}


static int ov5693_get_intg_factor(struct i2c_client *client,
				  struct camera_mipi_info *info,
				  const struct ov5693_resolution *res)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	struct atomisp_sensor_mode_data *buf = &info->data;
	//const unsigned int ext_clk_freq_hz = 19200000;
	//const unsigned int pll_invariant_div = 10;
	unsigned int pix_clk_freq_hz;
	//u16 pre_pll_clk_div;
	//u16 pll_multiplier;
	u16 reg_val;
	int ret;

	if (info == NULL)
		return -EINVAL;

	/* pixel clock */
	pix_clk_freq_hz = res->pix_clk_freq * 1000000;

	dev->vt_pix_clk_freq_mhz = pix_clk_freq_hz;
	buf->vt_pix_clk_freq_mhz = pix_clk_freq_hz;

	/* get integration time */
	buf->coarse_integration_time_min = OV5693_COARSE_INTG_TIME_MIN;
	buf->coarse_integration_time_max_margin =
	    OV5693_COARSE_INTG_TIME_MAX_MARGIN;

	buf->fine_integration_time_min = OV5693_FINE_INTG_TIME_MIN;
	buf->fine_integration_time_max_margin =
	    OV5693_FINE_INTG_TIME_MAX_MARGIN;

	buf->fine_integration_time_def = OV5693_FINE_INTG_TIME_MIN;
	buf->frame_length_lines = res->lines_per_frame;
	buf->line_length_pck = res->pixels_per_line;
	buf->read_mode = res->bin_mode;

	/* get the cropping and output resolution to ISP for this mode. */
	ret = ov5693_read_reg(client, OV5693_16BIT,
			      OV5693_HORIZONTAL_START_H, &reg_val);
	if (ret)
		return ret;
	buf->crop_horizontal_start = reg_val;

	ret = ov5693_read_reg(client, OV5693_16BIT,
			      OV5693_VERTICAL_START_H, &reg_val);
	if (ret)
		return ret;
	buf->crop_vertical_start = reg_val;

	ret = ov5693_read_reg(client, OV5693_16BIT,
			      OV5693_HORIZONTAL_END_H, &reg_val);
	if (ret)
		return ret;
	buf->crop_horizontal_end = reg_val;

	ret = ov5693_read_reg(client, OV5693_16BIT,
			      OV5693_VERTICAL_END_H, &reg_val);
	if (ret)
		return ret;
	buf->crop_vertical_end = reg_val;

	ret = ov5693_read_reg(client, OV5693_16BIT,
			      OV5693_HORIZONTAL_OUTPUT_SIZE_H, &reg_val);
	if (ret)
		return ret;
	buf->output_width = reg_val;

	ret = ov5693_read_reg(client, OV5693_16BIT,
			      OV5693_VERTICAL_OUTPUT_SIZE_H, &reg_val);
	if (ret)
		return ret;
	buf->output_height = reg_val;

	buf->binning_factor_x = res->bin_factor_x ? ((res->bin_factor_x)*2) : 1;
	buf->binning_factor_y = res->bin_factor_y ? ((res->bin_factor_y)*2) : 1;
	return 0;
}

static long __ov5693_set_exposure(struct v4l2_subdev *sd, int coarse_itg,
				  int gain, int digitgain)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	//const struct ov5693_resolution *res;
	u16 vts, hts;
	//int frame_length;
	int ret, exp_val;

	hts = ov5693_res[dev->fmt_idx].pixels_per_line;
	vts = ov5693_res[dev->fmt_idx].lines_per_frame;

	/* group hold */
	ret = ov5693_write_reg(client, OV5693_8BIT, OV5693_GROUP_ACCESS, 0x00);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, OV5693_GROUP_ACCESS);
		return ret;
	}

	/* Increase the VTS to match exposure + MARGIN */
	if (coarse_itg > vts - OV5693_INTEGRATION_TIME_MARGIN)
		vts = (u16) coarse_itg + OV5693_INTEGRATION_TIME_MARGIN;

	ret = ov5693_write_reg(client, OV5693_16BIT, OV5693_TIMING_VTS_H, vts);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, OV5693_TIMING_VTS_H);
		return ret;
	}

	/* set exposure */

	/* Lower four bit should be 0 */
	exp_val = coarse_itg << 4;
	ret = ov5693_write_reg(client, OV5693_8BIT,
			       OV5693_EXPOSURE_L, exp_val & 0xFF);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, OV5693_EXPOSURE_L);
		return ret;
	}

	ret = ov5693_write_reg(client, OV5693_8BIT,
			       OV5693_EXPOSURE_M, (exp_val >> 8) & 0xFF);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, OV5693_EXPOSURE_M);
		return ret;
	}

	ret = ov5693_write_reg(client, OV5693_8BIT,
			       OV5693_EXPOSURE_H, (exp_val >> 16) & 0x0F);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, OV5693_EXPOSURE_H);
		return ret;
	}

	/* Analog gain */
	ret = ov5693_write_reg(client, OV5693_16BIT, OV5693_AGC_H, gain);
	if (ret) {
		dev_err(&client->dev, "%s: write %x error, aborted\n",
			__func__, OV5693_AGC_H);
		return ret;
	}
	/* Digital gain */
	if (digitgain) {
		ret = ov5693_write_reg(client, OV5693_16BIT,
				       OV5693_MWB_RED_GAIN_H, digitgain);
		if (ret) {
			dev_err(&client->dev, "%s: write %x error, aborted\n",
				__func__, OV5693_MWB_RED_GAIN_H);
			return ret;
		}

		ret = ov5693_write_reg(client, OV5693_16BIT,
				       OV5693_MWB_GREEN_GAIN_H, digitgain);
		if (ret) {
			dev_err(&client->dev, "%s: write %x error, aborted\n",
				__func__, OV5693_MWB_RED_GAIN_H);
			return ret;
		}

		ret = ov5693_write_reg(client, OV5693_16BIT,
				       OV5693_MWB_BLUE_GAIN_H, digitgain);
		if (ret) {
			dev_err(&client->dev, "%s: write %x error, aborted\n",
				__func__, OV5693_MWB_RED_GAIN_H);
			return ret;
		}
	}

	/* End group */
	ret = ov5693_write_reg(client, OV5693_8BIT, OV5693_GROUP_ACCESS, 0x10);
	if (ret)
		return ret;

	/* Delay launch group */
	//ret = ov5693_write_reg(client, OV5693_8BIT, OV5693_GROUP_ACCESS, 0xa0);
	ret = ov5693_write_reg(client, OV5693_8BIT, OV5693_GROUP_ACCESS, 0xe0);
	if (ret)
		return ret;
	return ret;
}

static int ov5693_set_exposure(struct v4l2_subdev *sd, int exposure,
			       int gain, int digitgain)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	int ret;

	mutex_lock(&dev->input_lock);
	ret = __ov5693_set_exposure(sd, exposure, gain, digitgain);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static long ov5693_s_exposure(struct v4l2_subdev *sd,
			      struct atomisp_exposure *exposure)
{
	u16 coarse_itg = exposure->integration_time[0];
	//u16 fine_itg = exposure->integration_time[1];
	u16 analog_gain = exposure->gain[0];
	u16 digital_gain = exposure->gain[1];

	/* we should not accept the invalid value below */
	if (analog_gain == 0) {
		struct i2c_client *client = v4l2_get_subdevdata(sd);
		v4l2_err(client, "%s: invalid value\n", __func__);
		return -EINVAL;
	}
	return ov5693_set_exposure(sd, coarse_itg, analog_gain, digital_gain);
}

/*
* return 0: empty otp
*           1: invalid otp
*           2: valid otp
*/
#if 0
static int ov5693_otp_check(struct i2c_client *client, int index)
{
	int flag, i;
	int bank, addr;
	int ret;
	u16 val;

	//select bank index
	bank = (0xc0 | index);
	ret = ov5693_write_reg(client, OV5693_8BIT, OV5693_OTP_BANK_REG, bank);

	//read otp into buffer
	ret =
	    ov5693_write_reg(client, OV5693_8BIT, OV5693_OTP_READ_REG,
			     OV5693_OTP_MODE_READ);
	msleep(5);

	//read flag
	addr = OV5693_OTP_START_ADDR;
	ret = ov5693_read_reg(client, OV5693_8BIT, addr, &val);
	flag = (val & 0xc0);

	//clear otp buffer
	for (i = 0; i < 16; i++)
		ov5693_write_reg(client, OV5693_8BIT, addr + i, 0x00);

	if (flag == 0x00) {
		return 0;
	} else if (flag & 0x80) {
		return 1;
	} else
		return 2;

}


static int ov5693_read_otp_data(struct i2c_client *client, u16 len, u16 reg,
				void *val)
{
	struct i2c_msg msg[2];
	u16 data[OV5693_OTP_SHORT_MAX] = { 0 };
	int err;

	if (len > OV5693_OTP_BYTE_MAX) {
		dev_err(&client->dev, "%s error, invalid data length\n",
			__func__);
		return -EINVAL;
	}

	memset(msg, 0, sizeof(msg));
	memset(data, 0, sizeof(data));

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = I2C_MSG_LENGTH;
	msg[0].buf = (u8 *) data;
	/* high byte goes first */
	//data[0] = cpu_to_be16(reg);
	data[0] = (u8) (reg >> 8);
	data[1] = (u8) (reg & 0xff);

	msg[1].addr = client->addr;
	msg[1].len = len;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = (u8 *) data;

	err = i2c_transfer(client->adapter, msg, 2);
	if (err != 2) {
		if (err >= 0)
			err = -EIO;
		goto error;
	}

	memcpy(val, data, len);

	v4l2_info(client, "\n%s 0x%x", __func__, data[0]);

	return 0;

error:
	dev_err(&client->dev, "read from offset 0x%x error %d", reg, err);
	return err;
}
#endif

static int ov5693_read_otp_reg_array(struct i2c_client *client, u16 size,
				     u16 addr, u8 * buf)
{
	u16 index;
	int ret;
    u16 *pVal = 0;

#if 0
	for (index = 0; index + OV5693_OTP_READ_ONETIME <= size;
	     index += OV5693_OTP_READ_ONETIME) {
		ret = ov5693_read_otp_data(client, OV5693_OTP_READ_ONETIME,
					   addr + index, &buf[index]);
		if (ret)
			return ret;
	}
#else
	for (index = 0; index <= size; index++) {
/*
		ret =
		    ov5693_read_reg(client, OV5693_8BIT, addr + index,
				    &buf[index]);
*/
        pVal = (u16 *) (buf + index);
		ret =
		    ov5693_read_reg(client, OV5693_8BIT, addr + index,
				    pVal);
		if (ret)
			return ret;
	}
#endif
	return 0;
}

static int __ov5693_otp_read(struct v4l2_subdev *sd, u8 * buf)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	int ret;
	int i;
	u8 *b = buf;
	dev->otp_size = 0;
	for (i = 1; i < OV5693_OTP_BANK_MAX; i++) {
		/*check otp if needed  */
		//ret = ov5693_otp_check(client,i);
		//v4l2_info(client, "\n%s 0-empty 1-invalid 2-valid Bank[%d] OTP?:%d",__func__,i,ret);

		/*set bank NO and OTP read mode. */
		ret = ov5693_write_reg(client, OV5693_8BIT, OV5693_OTP_BANK_REG, (i | 0xc0));	//[7:6] 2'b11 [5:0] bank no
		if (ret) {
			dev_err(&client->dev, "failed to prepare OTP page\n");
			return ret;
		}
		//v4l2_info(client, "\nwrite 0x%x->0x%x",OV5693_OTP_BANK_REG,(i|0xc0));

		/*enable read */
		ret = ov5693_write_reg(client, OV5693_8BIT, OV5693_OTP_READ_REG, OV5693_OTP_MODE_READ);	// enable :1
		if (ret) {
			dev_err(&client->dev,
				"failed to set OTP reading mode page");
			return ret;
		}
		//v4l2_info(client, "\nwrite 0x%x->0x%x",OV5693_OTP_READ_REG,OV5693_OTP_MODE_READ);

		/* Reading the OTP data array */
		ret = ov5693_read_otp_reg_array(client, OV5693_OTP_BANK_SIZE,
						OV5693_OTP_START_ADDR,
						b);
		if (ret) {
			dev_err(&client->dev, "failed to read OTP data\n");
			return ret;
		}

		//v4l2_info(client, "BANK[%2d] %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", i, *b, *(b+1), *(b+2), *(b+3), *(b+4), *(b+5), *(b+6), *(b+7), *(b+8), *(b+9), *(b+10), *(b+11), *(b+12), *(b+13), *(b+14), *(b+15));

		if (21 == i) {
			if ((*b) == 0) {
				dev->otp_size = 320;
				break;
			} else {
				b = buf;
				continue;
			}
		} else if (24 == i) {
			if ((*b) == 0) {
				dev->otp_size = 32;
				break;
		} else {
				b = buf;
				continue;
			}
		} else if (27 == i) {
			if ((*b) == 0) {
				dev->otp_size = 32;
				break;
			} else {
				dev->otp_size = 0;
				break;
			}
		}

		b = b + OV5693_OTP_BANK_SIZE;
	}
	return 0;
}

/*
 * Read otp data and store it into a kmalloced buffer.
 * The caller must kfree the buffer when no more needed.
 * @size: set to the size of the returned otp data.
 */
static void *ov5693_otp_read(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 *buf;
	int ret;

	buf = devm_kzalloc(&client->dev, (OV5693_OTP_DATA_SIZE + 16), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	//otp valid after mipi on and sw stream on
	ret = ov5693_write_reg(client, OV5693_8BIT, OV5693_FRAME_OFF_NUM, 0x00);

	ret = ov5693_write_reg(client, OV5693_8BIT,
			       OV5693_SW_STREAM, OV5693_START_STREAMING);

	ret = __ov5693_otp_read(sd, buf);

	//mipi off and sw stream off after otp read
	ret = ov5693_write_reg(client, OV5693_8BIT, OV5693_FRAME_OFF_NUM, 0x0f);

	ret = ov5693_write_reg(client, OV5693_8BIT,
			       OV5693_SW_STREAM, OV5693_STOP_STREAMING);

	/* Driver has failed to find valid data */
	if (ret) {
		dev_err(&client->dev, "sensor found no valid OTP data\n");
		return ERR_PTR(ret);
	}

	return buf;
}

static int ov5693_g_priv_int_data(struct v4l2_subdev *sd,
				  struct v4l2_private_int_data *priv)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	u8 __user *to = priv->data;
	u32 read_size = priv->size;
	int ret;

	/* No need to copy data if size is 0 */
	if (!read_size)
		goto out;

	if (IS_ERR(dev->otp_data)) {
		dev_err(&client->dev, "OTP data not available");
		return PTR_ERR(dev->otp_data);
	}

	/* Correct read_size value only if bigger than maximum */
	if (read_size > OV5693_OTP_DATA_SIZE)
		read_size = OV5693_OTP_DATA_SIZE;

	ret = copy_to_user(to, dev->otp_data, read_size);
	if (ret) {
		dev_err(&client->dev, "%s: failed to copy OTP data to user\n",
			__func__);
		return -EFAULT;
	}

	v4l2_info(client, "\n%s read_size:%d", __func__, read_size);

out:
	/* Return correct size */
	priv->size = dev->otp_size;

	return 0;

}

static long ov5693_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{

	switch (cmd) {
	case ATOMISP_IOC_S_EXPOSURE:
		return ov5693_s_exposure(sd, arg);
	case ATOMISP_IOC_G_SENSOR_PRIV_INT_DATA:
		return ov5693_g_priv_int_data(sd, arg);
	default:
		return -EINVAL;
	}
	return 0;
}

/* This returns the exposure time being used. This should only be used
   for filling in EXIF data, not for actual image processing. */
static int ov5693_q_exposure(struct v4l2_subdev *sd, s32 * value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 reg_v, reg_v2;
	int ret;

	/* get exposure */
	ret = ov5693_read_reg(client, OV5693_8BIT, OV5693_EXPOSURE_L, &reg_v);
	if (ret)
		goto err;

	ret = ov5693_read_reg(client, OV5693_8BIT, OV5693_EXPOSURE_M, &reg_v2);
	if (ret)
		goto err;

	reg_v += reg_v2 << 8;
	ret = ov5693_read_reg(client, OV5693_8BIT, OV5693_EXPOSURE_H, &reg_v2);
	if (ret)
		goto err;

	*value = reg_v + (((u32) reg_v2 << 16));
err:
	return ret;
}

int ov5693_vcm_power_up(struct v4l2_subdev *sd)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->power_up)
		return dev->vcm_driver->power_up(sd);
	return 0;
}

int ov5693_vcm_power_down(struct v4l2_subdev *sd)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->power_down)
		return dev->vcm_driver->power_down(sd);
	return 0;
}

int ov5693_vcm_init(struct v4l2_subdev *sd)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->init)
		return dev->vcm_driver->init(sd);
	return 0;
}

int ov5693_t_focus_vcm(struct v4l2_subdev *sd, u16 val)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_focus_vcm)
		return dev->vcm_driver->t_focus_vcm(sd, val);
	return 0;
}

int ov5693_t_focus_abs(struct v4l2_subdev *sd, s32 value)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_focus_abs)
		return dev->vcm_driver->t_focus_abs(sd, value);
	return 0;
}

int ov5693_t_focus_rel(struct v4l2_subdev *sd, s32 value)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_focus_rel)
		return dev->vcm_driver->t_focus_rel(sd, value);
	return 0;
}

int ov5693_q_focus_status(struct v4l2_subdev *sd, s32 * value)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->q_focus_status)
		return dev->vcm_driver->q_focus_status(sd, value);
	return 0;
}

int ov5693_q_focus_abs(struct v4l2_subdev *sd, s32 * value)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->q_focus_abs)
		return dev->vcm_driver->q_focus_abs(sd, value);
	return 0;
}

int ov5693_t_vcm_slew(struct v4l2_subdev *sd, s32 value)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_vcm_slew)
		return dev->vcm_driver->t_vcm_slew(sd, value);
	return 0;
}

int ov5693_t_vcm_timing(struct v4l2_subdev *sd, s32 value)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_vcm_timing)
		return dev->vcm_driver->t_vcm_timing(sd, value);
	return 0;
}
#if 0
struct ov5693_control ov5693_controls[] = {
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
	 .query = ov5693_q_exposure,
	 },
	{
	 .qc = {
		.id = V4L2_CID_FOCAL_ABSOLUTE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "focal length",
		.minimum = OV5693_FOCAL_LENGTH_DEFAULT,
		.maximum = OV5693_FOCAL_LENGTH_DEFAULT,
		.step = 0x01,
		.default_value = OV5693_FOCAL_LENGTH_DEFAULT,
		.flags = 0,
		},
	 .query = ov5693_g_focal,
	 },
	{
	 .qc = {
		.id = V4L2_CID_FNUMBER_ABSOLUTE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "f-number",
		.minimum = OV5693_F_NUMBER_DEFAULT,
		.maximum = OV5693_F_NUMBER_DEFAULT,
		.step = 0x01,
		.default_value = OV5693_F_NUMBER_DEFAULT,
		.flags = 0,
		},
	 .query = ov5693_g_fnumber,
	 },
	{
	 .qc = {
		.id = V4L2_CID_FNUMBER_RANGE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "f-number range",
		.minimum = OV5693_F_NUMBER_RANGE,
		.maximum = OV5693_F_NUMBER_RANGE,
		.step = 0x01,
		.default_value = OV5693_F_NUMBER_RANGE,
		.flags = 0,
		},
	 .query = ov5693_g_fnumber_range,
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
	 .tweak = ov5693_t_focus_abs,
	 .query = ov5693_q_focus_abs,
	 },
	{
	 .qc = {
		.id = V4L2_CID_FOCUS_RELATIVE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "focus move relative",
		.minimum = OV5693_MAX_FOCUS_NEG,
		.maximum = OV5693_MAX_FOCUS_POS,
		.step = 1,
		.default_value = 0,
		.flags = 0,
		},
	 .tweak = ov5693_t_focus_rel,
	 },
	{
	 .qc = {
		.id = V4L2_CID_FOCUS_STATUS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "focus status",
		.minimum = 0,
		.maximum = 100,	/* allow enum to grow in the future */
		.step = 1,
		.default_value = 0,
		.flags = 0,
		},
	 .query = ov5693_q_focus_status,
	 },
	{
	 .qc = {
		.id = V4L2_CID_VCM_SLEW,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "vcm slew",
		.minimum = 0,
		.maximum = OV5693_VCM_SLEW_STEP_MAX,
		.step = 1,
		.default_value = 0,
		.flags = 0,
		},
	 .tweak = ov5693_t_vcm_slew,
	 },
	{
	 .qc = {
		.id = V4L2_CID_VCM_TIMEING,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "vcm step time",
		.minimum = 0,
		.maximum = OV5693_VCM_SLEW_TIME_MAX,
		.step = 1,
		.default_value = 0,
		.flags = 0,
		},
	 .tweak = ov5693_t_vcm_timing,
	 },
	{
		.qc = {
			.id = V4L2_CID_BIN_FACTOR_HORZ,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "horizontal binning factor",
			.minimum = 0,
			.maximum = OV5693_BIN_FACTOR_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = ov5693_g_bin_factor_x,
	},
	{
		.qc = {
			.id = V4L2_CID_BIN_FACTOR_VERT,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "vertical binning factor",
			.minimum = 0,
			.maximum = OV5693_BIN_FACTOR_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = ov5693_g_bin_factor_y,
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
			.flags = 0,
		},
		.tweak = ov5693_t_vflip,
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
			.flags = 0,
		},
		.tweak = ov5693_t_hflip,
	},
};

#define N_CONTROLS (ARRAY_SIZE(ov5693_controls))

static struct ov5693_control *ov5693_find_control(u32 id)
{
	int i;

	for (i = 0; i < N_CONTROLS; i++)
		if (ov5693_controls[i].qc.id == id)
			return &ov5693_controls[i];
	return NULL;
}

static int ov5693_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	struct ov5693_control *ctrl = ov5693_find_control(qc->id);
	struct ov5693_device *dev = to_ov5693_sensor(sd);

	if (ctrl == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	*qc = ctrl->qc;
	mutex_unlock(&dev->input_lock);

	return 0;
}
#endif
static struct ov5693_reg const  Normal_full[]={
//Mirror_on_full
{OV5693_8BIT, 0x3820, 0x00},
{OV5693_8BIT, 0x3821, 0x06},
{OV5693_8BIT, 0x5a01, 0x01},
{OV5693_8BIT, 0x5a03, 0x00},
{OV5693_TOK_TERM, 0, 0},
};
static struct ov5693_reg const   Mirror_on_full[]={
//Mirror_Flip_on_full
{OV5693_8BIT, 0x3820, 0x42},
{OV5693_8BIT, 0x3821, 0x06},
{OV5693_8BIT, 0x5a01, 0x01},
{OV5693_8BIT, 0x5a03, 0x01},
{OV5693_TOK_TERM, 0, 0},
};
static struct ov5693_reg const    Flip_On_full[]={
//Normal_full
{OV5693_8BIT, 0x3820, 0x00},
{OV5693_8BIT, 0x3821, 0x00},
{OV5693_8BIT, 0x5a01, 0x00},
{OV5693_8BIT, 0x5a03, 0x00},
{OV5693_TOK_TERM, 0, 0},
};
static struct ov5693_reg const    Mirror_Flip_on_full[]={
//Flip_On_full
{OV5693_8BIT, 0x3820, 0x42},
{OV5693_8BIT, 0x3821, 0x00},
{OV5693_8BIT, 0x5a01, 0x00},
{OV5693_8BIT, 0x5a03, 0x01},
{OV5693_TOK_TERM, 0, 0},
};
static struct ov5693_reg const    Normal_1296x972[]={
//Mirror_on_1296x972
{OV5693_8BIT, 0x3820, 0x04},
{OV5693_8BIT, 0x3821, 0x1f},
{OV5693_8BIT, 0x5a01, 0x00},
{OV5693_8BIT, 0x5a03, 0x00},
{OV5693_TOK_TERM, 0, 0},
};
static struct ov5693_reg const    Mirror_on_1296x972[]={
//Mirror_Flip_on_1296x972
{OV5693_8BIT, 0x3820, 0x06},
{OV5693_8BIT, 0x3821, 0x1f},
{OV5693_8BIT, 0x5a01, 0x00},
{OV5693_8BIT, 0x5a03, 0x00},
{OV5693_TOK_TERM, 0, 0},
};
static struct ov5693_reg const    Flip_On_1296x972[]={
//Normal_1296x972
{OV5693_8BIT, 0x3820, 0x04},
{OV5693_8BIT, 0x3821, 0x19},
{OV5693_8BIT, 0x5a01, 0x00},
{OV5693_8BIT, 0x5a03, 0x00},
{OV5693_TOK_TERM, 0, 0},
};
static struct ov5693_reg const    Mirror_Flip_on_1296x972[]={
//Flip_On_1296x972
{OV5693_8BIT, 0x3820, 0x06},
{OV5693_8BIT, 0x3821, 0x19},
{OV5693_8BIT, 0x5a01, 0x00},
{OV5693_8BIT, 0x5a03, 0x00},
{OV5693_TOK_TERM, 0, 0},
};

/* Horizontal flip the image. */
static int ov5693_t_hflip(struct v4l2_subdev *sd, int value)
{
	struct camera_mipi_info *ov5693_info = NULL;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	int err;
	u16	val;
	u16 data_tem_hflip,data_tem_vflip;

	u16 bayer_order_tem_h;
	/* set for direct mode */
	u16  data_h_offset,data_v_offset;
	u16  height_windows_size,data_tem_window_l,data_tem_window_h;

	err=ov5693_read_reg(client, OV5693_8BIT,	0x3808, &data_tem_window_h);
	err=ov5693_read_reg(client, OV5693_8BIT,	0x3809, &data_tem_window_l);
	height_windows_size=((data_tem_window_h<<8)&0xff00) |data_tem_window_l;

	if(height_windows_size<=1296)
	{
			if (value)
				err = ov5693_write_reg_array(client, Mirror_on_1296x972);

			if((v_flag & h_flag)==1)
				err = ov5693_write_reg_array(client, Mirror_Flip_on_1296x972);
	}
	else
	{
			if (value)
			err = ov5693_write_reg_array(client, Mirror_on_full);

			if((v_flag & h_flag)==1)
			err = ov5693_write_reg_array(client, Mirror_Flip_on_full);
	}

	return !!err;
}


static int ov5693_t_vflip(struct v4l2_subdev *sd, int value)
{
	struct camera_mipi_info *ov5693_info = NULL;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	int err;
	u16	val;
	u16 data_tem_hflip,data_tem_vflip;

	u16 bayer_order_tem_h;
	/* set for direct mode */
	u16  data_h_offset,data_v_offset;
	u16  height_windows_size,data_tem_window_l,data_tem_window_h;

	err=ov5693_read_reg(client, OV5693_8BIT,	0x3808, &data_tem_window_h);
	err=ov5693_read_reg(client, OV5693_8BIT,	0x3809, &data_tem_window_l);
	height_windows_size=((data_tem_window_h<<8)&0xff00) |data_tem_window_l;

	if(height_windows_size<=1296)
	{
			if (value)
			err = ov5693_write_reg_array(client, Flip_On_1296x972);

			if((v_flag & h_flag)==1)
			err = ov5693_write_reg_array(client, Mirror_Flip_on_1296x972);
	}
	else
	{
			if (value)
				err = ov5693_write_reg_array(client, Flip_On_full);
			if((v_flag & h_flag)==1)
				err = ov5693_write_reg_array(client, Mirror_Flip_on_full);
	}

	return !!err;
}


static int ov5693_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov5693_device *dev = container_of(
		ctrl->handler, struct ov5693_device, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		dev_info(&client->dev, "@%s: vflip:%d", __func__, ctrl->val);
		if(ctrl->val) v_flag=1;else  v_flag=0;
		break;
	case V4L2_CID_HFLIP:
		dev_info(&client->dev, "@%s: hflip:%d", __func__, ctrl->val);
		if(ctrl->val) h_flag=1;else  h_flag=0;
		break;
	case V4L2_CID_FOCUS_ABSOLUTE:
		ret = ov5693_t_focus_abs(&dev->sd, ctrl->val);
		break;
	case V4L2_CID_FOCUS_RELATIVE:
		ret = ov5693_t_focus_rel(&dev->sd, ctrl->val);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int ov5693_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov5693_device *dev = container_of(
		ctrl->handler, struct ov5693_device, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE_ABSOLUTE:
		ret = ov5693_q_exposure(&dev->sd, &ctrl->val);
		break;
	case V4L2_CID_FOCAL_ABSOLUTE:
		ret = ov5693_g_focal(&dev->sd, &ctrl->val);
		break;
	case V4L2_CID_FNUMBER_ABSOLUTE:
		ret = ov5693_g_fnumber(&dev->sd, &ctrl->val);
		break;
	case V4L2_CID_FNUMBER_RANGE:
		ret = ov5693_g_fnumber_range(&dev->sd, &ctrl->val);
		break;
	case V4L2_CID_FOCUS_ABSOLUTE:
		ret = ov5693_q_focus_abs(&dev->sd, &ctrl->val);
		break;
	case V4L2_CID_FOCUS_STATUS:
		ret = ov5693_q_focus_status(&dev->sd, &ctrl->val);
		break;
	case V4L2_CID_BIN_FACTOR_HORZ:
		ctrl->val = ov5693_g_bin_factor_x(&dev->sd, &ctrl->val);
		break;
	case V4L2_CID_BIN_FACTOR_VERT:
		ctrl->val = ov5693_g_bin_factor_y(&dev->sd, &ctrl->val);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}
static const struct v4l2_ctrl_ops ctrl_ops = {
       .s_ctrl = ov5693_s_ctrl,
       .g_volatile_ctrl = ov5693_g_ctrl
};

struct v4l2_ctrl_config ov5693_controls[] = {
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_EXPOSURE_ABSOLUTE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "exposure",
		.min = 0x0,
		.max = 0xffff,
		.step = 0x01,
		.def = 0x00,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_VFLIP,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Flip",
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_HFLIP,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Mirror",
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_FOCAL_ABSOLUTE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "focal length",
		.min = OV5693_FOCAL_LENGTH_DEFAULT,
		.max = OV5693_FOCAL_LENGTH_DEFAULT,
		.step = 0x01,
		.def = OV5693_FOCAL_LENGTH_DEFAULT,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_FNUMBER_ABSOLUTE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "f-number",
		.min = OV5693_F_NUMBER_DEFAULT,
		.max = OV5693_F_NUMBER_DEFAULT,
		.step = 0x01,
		.def = OV5693_F_NUMBER_DEFAULT,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_FNUMBER_RANGE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "f-number range",
		.min = OV5693_F_NUMBER_RANGE,
		.max =  OV5693_F_NUMBER_RANGE,
		.step = 0x01,
		.def = OV5693_F_NUMBER_RANGE,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_FOCUS_ABSOLUTE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "focus move absolute",
		.min = 0,
		.max = VCM_MAX_FOCUS_POS,
		.step = 1,
		.def = 0,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_FOCUS_RELATIVE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "focus move relative",
		.min = OV5693_MAX_FOCUS_NEG,
		.max = OV5693_MAX_FOCUS_POS,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_FOCUS_STATUS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "focus status",
		.min = 0,
		.max = 100,	/* allow enum to grow in the future */
		.step = 1,
		.def = 0,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_VCM_SLEW,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "vcm slew",
		.min = 0,
		.max = OV5693_VCM_SLEW_STEP_MAX,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_VCM_TIMEING,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "vcm step time",
		.min = 0,
		.max = OV5693_VCM_SLEW_TIME_MAX,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_BIN_FACTOR_HORZ,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "horizontal binning factor",
		.min = 0,
		.max = OV5693_BIN_FACTOR_MAX,
		.step = 1,
		.def = 0,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_BIN_FACTOR_VERT,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "vertical binning factor",
		.min = 0,
		.max = OV5693_BIN_FACTOR_MAX,
		.step = 1,
		.def = 0,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
};

#if 0
/* ov5693 control set/get */
static int ov5693_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ov5693_control *s_ctrl;
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	int ret;

	if (!ctrl)
		return -EINVAL;

	s_ctrl = ov5693_find_control(ctrl->id);
	if ((s_ctrl == NULL) || (s_ctrl->query == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = s_ctrl->query(sd, &ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int ov5693_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ov5693_control *octrl = ov5693_find_control(ctrl->id);
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	int ret;

	if ((octrl == NULL) || (octrl->tweak == NULL))
		return -EINVAL;

		switch(ctrl->id)
		{
			case V4L2_CID_VFLIP:if(ctrl->value) v_flag=1;else  v_flag=0;
				break;
			case V4L2_CID_HFLIP:if(ctrl->value) h_flag=1;else  h_flag=0;
				break;
			default:break;
		};

	mutex_lock(&dev->input_lock);
	ret = octrl->tweak(sd, ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}
#endif

static int ov5693_init_registers(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	ret = ov5693_write_reg(client, OV5693_8BIT, OV5693_SW_RESET, 0x01);
	ret |= ov5693_write_reg_array(client, ov5693_global_setting);

	return ret;
}

static int ov5693_init(struct v4l2_subdev *sd)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	mutex_lock(&dev->input_lock);

	/* restore settings */
	ov5693_res = ov5693_res_preview;
	N_RES = N_RES_PREVIEW;

	ret = ov5693_init_registers(sd);

	mutex_unlock(&dev->input_lock);

	return ret;
}

static int power_up(struct v4l2_subdev *sd)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
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
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	//v_flag = 0;
	//h_flag = 0;

	dev->focus = OV5693_INVALID_CONFIG;
	if (NULL == dev->platform_data) {
		dev_err(&client->dev, "no camera_sensor_platform_data");
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

static int ov5693_s_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5693_device *dev = to_ov5693_sensor(sd);

	if (on == 0) {
		ret = power_down(sd);
		if (dev->vcm_driver && dev->vcm_driver->power_down)
			ret = dev->vcm_driver->power_down(sd);
	} else {
		if (dev->vcm_driver && dev->vcm_driver->power_up)
			ret = dev->vcm_driver->power_up(sd);
		if (ret)
			return ret;

		ret = power_up(sd);

		if (!ret)
			return ov5693_init(sd);
	}
	return ret;
}

#ifdef POWER_ALWAYS_ON_BEFORE_SUSPEND
static int ov5693_set_suspend(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	ret = ov5693_write_reg(client, OV5693_8BIT, 0x0100, 0x00);
	if (ret) {
		dev_err(&client->dev, "ov5693 write err.\n");
	}

	return ret;
}

static int ov5693_sub_init(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	return ov5693_init(sd);
}

static int ov5693_s_power_always_on(struct v4l2_subdev *sd, int on)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	if (!dev->second_power_on_at_boot_done) {
		if (!on) {
			dev->second_power_on_at_boot_done = 1;
		}
		return ov5693_s_power(sd, on);
	}

	if (on == 0) {
		//ret = power_down(sd);
		//dev->power = 0;
		// For case camera is stream-on, and then closed without a stream-off.
		//v_flag = 0;
		//h_flag = 0;

		ret = ov5693_set_suspend(sd);

		//if (dev->vcm_driver && dev->vcm_driver->power_down)
		//	ret = dev->vcm_driver->power_down(sd);
	} else {
		if (!dev->power) {
			if (dev->vcm_driver && dev->vcm_driver->power_up)
				ret = dev->vcm_driver->power_up(sd);
			if (ret)
				return ret;

			ret = power_up(sd);
			if (!ret) {
				dev->power = 1;
				dev->once_launched = 1;
				ret = ov5693_sub_init(client);
				if (ret) {
					dev_err(&client->dev, "i2c write error for ov5693_sub_init()\n");
				}
			}
		} else {
			ret = ov5693_sub_init(client);
			if (ret) {
				dev_err(&client->dev, "i2c write error for ov5693_sub_init()\n");
			}
		}
	}

	return ret;
}
#endif

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
static int distance(struct ov5693_resolution *res, u32 w, u32 h)
{
	unsigned int w_ratio = ((res->width << 13) / w);
	unsigned int h_ratio;
	int match;

	if (h == 0)
		return -1;
	h_ratio = ((res->height << 13) / h);
	if (h_ratio == 0)
		return -1;
	match = abs(((w_ratio << 13) / h_ratio) - ((int)8192));

	if ((w_ratio < (int)8192) || (h_ratio < (int)8192) ||
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
	struct ov5693_resolution *tmp_res = NULL;

	for (i = 0; i < N_RES; i++) {
		tmp_res = &ov5693_res[i];
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
		if (w != ov5693_res[i].width)
			continue;
		if (h != ov5693_res[i].height)
			continue;

		return i;
	}

	return -1;
}

static int ov5693_try_mbus_fmt(struct v4l2_subdev *sd,
			       struct v4l2_mbus_framefmt *fmt)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int idx = 0;
	if (!fmt)
		return -EINVAL;
	v4l2_info(client, "+ %s idx %d width %d height %d\n", __func__, idx,
		  fmt->width, fmt->height);
	idx = nearest_resolution_index(fmt->width, fmt->height);
	if (idx == -1) {
		/* return the largest resolution */
		fmt->width = ov5693_res[N_RES - 1].width;
		fmt->height = ov5693_res[N_RES - 1].height;
	} else {
		fmt->width = ov5693_res[idx].width;
		fmt->height = ov5693_res[idx].height;
	}
	v4l2_info(client, "- %s idx %d width %d height %d\n", __func__, idx,
		  fmt->width, fmt->height);
	fmt->code = V4L2_MBUS_FMT_SBGGR10_1X10;

	return 0;
}

static int ov5693_s_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_mipi_info *ov5693_info = NULL;
	int ret = 0;
	int err;
	u16	val;
	u16 data_tem_hflip;
	u16 data_tem_vflip;
	u16 bayer_order_tem_v;
	u16  height_windows_size,data_tem_window_l,data_tem_window_h;

	ov5693_info = v4l2_get_subdev_hostdata(sd);
	if (ov5693_info == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = ov5693_try_mbus_fmt(sd, fmt);
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
	v4l2_info(client, "__s_mbus_fmt i=%d, w=%d, h=%d\n", dev->fmt_idx,
		  fmt->width, fmt->height);

	ret = ov5693_write_reg_array(client, ov5693_res[dev->fmt_idx].regs);
	if (ret)
		dev_err(&client->dev, "ov5693 write resolution register err\n");

	ret = ov5693_get_intg_factor(client, ov5693_info,
				     &ov5693_res[dev->fmt_idx]);
	if (ret) {
		dev_err(&client->dev, "failed to get integration_factor\n");
		goto err;
	}

	v4l2_info(client,"\n%s idx %d \n", __func__, dev->fmt_idx);

	ov5693_read_reg(client, OV5693_8BIT,	0x3808, &data_tem_window_h);
	ov5693_read_reg(client, OV5693_8BIT,	0x3809, &data_tem_window_l);
	height_windows_size=((data_tem_window_h<<8)&0xff00) |data_tem_window_l;

	if (v_flag)
		ov5693_t_vflip(sd, v_flag);
	if (h_flag)
		ov5693_t_hflip(sd, h_flag);
/*
	if (((v_flag & h_flag)==0)|((v_flag | h_flag)==0))
		if(height_windows_size<=1296)
		{
   			ov5693_write_reg_array(client, Normal_1296x972);
  		}
  		else 
		{
  			ov5693_write_reg_array(client, Normal_full);
  		}
	err=ov5693_read_reg(client, OV5693_8BIT,	ov5693_CTRL_HFLIP_ADDR, &data_tem_hflip);
	err=ov5693_read_reg(client, OV5693_8BIT,	ov5693_CTRL_VFLIP_ADDR, &data_tem_vflip);		
		
	ov5693_info = v4l2_get_subdev_hostdata(sd);
*/
err:
	mutex_unlock(&dev->input_lock);
	return ret;
}

static int ov5693_g_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);

	if (!fmt)
		return -EINVAL;

	fmt->width = ov5693_res[dev->fmt_idx].width;
	fmt->height = ov5693_res[dev->fmt_idx].height;
	fmt->code = V4L2_MBUS_FMT_SBGGR10_1X10;

	return 0;
}

static int ov5693_detect(struct i2c_client *client)
{
	struct i2c_adapter *adapter = client->adapter;
	u16 high, low;
	int ret;
	u16 id;
	u8 revision;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -ENODEV;

	ret = ov5693_read_reg(client, OV5693_8BIT,
			      OV5693_SC_CMMN_CHIP_ID_H, &high);
	if (ret) {
		dev_err(&client->dev, "sensor_id_high = 0x%x\n", high);
		return -ENODEV;
	}
	ret = ov5693_read_reg(client, OV5693_8BIT,
			      OV5693_SC_CMMN_CHIP_ID_L, &low);
	id = ((((u16) high) << 8) | (u16) low);

	if (id != OV5693_ID) {
		dev_err(&client->dev, "sensor ID error 0x%x\n", id);
		return -ENODEV;
	}

	ret = ov5693_read_reg(client, OV5693_8BIT,
			      OV5693_SC_CMMN_SUB_ID, &high);
	revision = (u8) high & 0x0f;

	dev_dbg(&client->dev, "sensor_revision = 0x%x\n", revision);
	dev_dbg(&client->dev, "detect ov5693 success\n");
	return 0;
}

static int ov5693_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	mutex_lock(&dev->input_lock);

	ret = ov5693_write_reg(client, OV5693_8BIT, OV5693_SW_STREAM,
			       enable ? OV5693_START_STREAMING :
			       OV5693_STOP_STREAMING);
#if 0
	/* restore settings */
	ov5693_res = ov5693_res_preview;
	N_RES = N_RES_PREVIEW;
#endif

	//otp valid at stream on state
	//if(!dev->otp_data)
	//      dev->otp_data = ov5693_otp_read(sd);

	mutex_unlock(&dev->input_lock);

	return ret;
}

/* ov5693 enum frame size, frame intervals */
static int ov5693_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	unsigned int index = fsize->index;

	if (index >= N_RES)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = ov5693_res[index].width;
	fsize->discrete.height = ov5693_res[index].height;
	fsize->reserved[0] = ov5693_res[index].used;

	return 0;
}

static int ov5693_enum_frameintervals(struct v4l2_subdev *sd,
				      struct v4l2_frmivalenum *fival)
{
	unsigned int index = fival->index;

	if (index >= N_RES)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->width = ov5693_res[index].width;
	fival->height = ov5693_res[index].height;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = ov5693_res[index].fps;

	return 0;
}

static int ov5693_enum_mbus_fmt(struct v4l2_subdev *sd,
				unsigned int index,
				enum v4l2_mbus_pixelcode *code)
{
	*code = V4L2_MBUS_FMT_SBGGR10_1X10;

	return 0;
}

static int __ov5693_init_ctrl_handler(struct ov5693_device *dev)
{
	struct v4l2_ctrl_handler *hdl;
	int i;

	hdl = &dev->ctrl_handler;

	v4l2_ctrl_handler_init(&dev->ctrl_handler, ARRAY_SIZE(ov5693_controls));

	for (i = 0; i < ARRAY_SIZE(ov5693_controls); i++)
		v4l2_ctrl_new_custom(&dev->ctrl_handler,
				&ov5693_controls[i], NULL);

	dev->ctrl_handler.lock = &dev->input_lock;
	dev->sd.ctrl_handler = hdl;
	v4l2_ctrl_handler_setup(&dev->ctrl_handler);

	return 0;
}

static int ov5693_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	if (platform_data == NULL)
		return -ENODEV;

	dev->platform_data =
	    (struct camera_sensor_platform_data *)platform_data;

//	mutex_lock(&dev->input_lock); // because v4l2_ctrl_handler_free() has the same log
	/* power off the module, then power on it in future
	 * as first power on by board may not fulfill the
	 * power on sequqence needed by the module
	 */
	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "ov5693 power-off err.\n");
		goto fail_power_off;
	}

	ret = power_up(sd);
	if (ret) {
		dev_err(&client->dev, "ov5693 power-up err.\n");
		goto fail_power_on;
	}

	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret)
		goto fail_csi_cfg;

	/* config & detect sensor */
	ret = ov5693_detect(client);
	if (ret) {
		dev_err(&client->dev, "ov5693_detect err s_config.\n");
		goto fail_csi_cfg;
	}
	dev->otp_data = ov5693_otp_read(sd);

	ret = __ov5693_init_ctrl_handler(dev);
	if (ret)
		v4l2_ctrl_handler_free(&dev->ctrl_handler);

	/* turn off sensor, after probed */
	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "ov5693 power-off err.\n");
		goto fail_csi_cfg;
	}
//	mutex_unlock(&dev->input_lock);

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

static int ov5693_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!param)
		return -EINVAL;

	if (param->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		dev_err(&client->dev, "unsupported buffer type.\n");
		return -EINVAL;
	}

	memset(param, 0, sizeof(*param));
	param->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (dev->fmt_idx >= 0 && dev->fmt_idx < N_RES) {
		param->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
		param->parm.capture.timeperframe.numerator = 1;
		param->parm.capture.capturemode = dev->run_mode;
		param->parm.capture.timeperframe.denominator =
		    ov5693_res[dev->fmt_idx].fps;
	}
	return 0;
}

static int ov5693_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev->run_mode = param->parm.capture.capturemode;

	v4l2_info(client, "\n%s:run_mode :%x\n", __func__, dev->run_mode);

	mutex_lock(&dev->input_lock);
	switch (dev->run_mode) {
	case CI_MODE_VIDEO:
		ov5693_res = ov5693_res_video;
		N_RES = N_RES_VIDEO;
		break;
	case CI_MODE_STILL_CAPTURE:
		ov5693_res = ov5693_res_still;
		N_RES = N_RES_STILL;
		break;
	default:
		ov5693_res = ov5693_res_preview;
		N_RES = N_RES_PREVIEW;
	}
	mutex_unlock(&dev->input_lock);
	return 0;
}

static int ov5693_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);

	interval->interval.numerator = 1;
	interval->interval.denominator = ov5693_res[dev->fmt_idx].fps;

	return 0;
}

static int ov5693_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_fh *fh,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= MAX_FMTS)
		return -EINVAL;

	code->code = V4L2_MBUS_FMT_SBGGR10_1X10;
	return 0;
}

static int ov5693_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_fh *fh,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	int index = fse->index;

	if (index >= N_RES)
		return -EINVAL;

	fse->min_width = ov5693_res[index].width;
	fse->min_height = ov5693_res[index].height;
	fse->max_width = ov5693_res[index].width;
	fse->max_height = ov5693_res[index].height;

	return 0;

}

static struct v4l2_mbus_framefmt *__ov5693_get_pad_format(struct ov5693_device
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
			"__ov5693_get_pad_format err. pad %x\n", pad);
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

static int ov5693_get_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_fh *fh,
				 struct v4l2_subdev_format *fmt)
{
	struct ov5693_device *snr = to_ov5693_sensor(sd);
	struct v4l2_mbus_framefmt *format =
	    __ov5693_get_pad_format(snr, fh, fmt->pad, fmt->which);
	if (!format)
		return -EINVAL;

	fmt->format = *format;
	return 0;
}

static int ov5693_set_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_fh *fh,
				 struct v4l2_subdev_format *fmt)
{
	struct ov5693_device *snr = to_ov5693_sensor(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		snr->format = fmt->format;

	return 0;
}

static int ov5693_g_skip_frames(struct v4l2_subdev *sd, u32 * frames)
{
	struct ov5693_device *dev = to_ov5693_sensor(sd);

	mutex_lock(&dev->input_lock);
	*frames = ov5693_res[dev->fmt_idx].skip_frames;
	mutex_unlock(&dev->input_lock);

	return 0;
}

static const struct v4l2_subdev_video_ops ov5693_video_ops = {
	.s_stream = ov5693_s_stream,
	.g_parm = ov5693_g_parm,
	.s_parm = ov5693_s_parm,
	.enum_framesizes = ov5693_enum_framesizes,
	.enum_frameintervals = ov5693_enum_frameintervals,
	.enum_mbus_fmt = ov5693_enum_mbus_fmt,
	.try_mbus_fmt = ov5693_try_mbus_fmt,
	.g_mbus_fmt = ov5693_g_mbus_fmt,
	.s_mbus_fmt = ov5693_s_mbus_fmt,
	.g_frame_interval = ov5693_g_frame_interval,
};

static const struct v4l2_subdev_sensor_ops ov5693_sensor_ops = {
	.g_skip_frames = ov5693_g_skip_frames,
};

static const struct v4l2_subdev_core_ops ov5693_core_ops = {
#ifdef POWER_ALWAYS_ON_BEFORE_SUSPEND
	.s_power = ov5693_s_power_always_on,
#else
	.s_power = ov5693_s_power,
#endif
	.queryctrl = v4l2_subdev_queryctrl,
	.g_ctrl = v4l2_subdev_g_ctrl,
	.s_ctrl = v4l2_subdev_s_ctrl,
	.ioctl = ov5693_ioctl,
};

static const struct v4l2_subdev_pad_ops ov5693_pad_ops = {
	.enum_mbus_code = ov5693_enum_mbus_code,
	.enum_frame_size = ov5693_enum_frame_size,
	.get_fmt = ov5693_get_pad_format,
	.set_fmt = ov5693_set_pad_format,
};

static const struct v4l2_subdev_ops ov5693_ops = {
	.core = &ov5693_core_ops,
	.video = &ov5693_video_ops,
	.pad = &ov5693_pad_ops,
	.sensor = &ov5693_sensor_ops,
};

static int ov5693_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov5693_device *dev = to_ov5693_sensor(sd);
	dev_dbg(&client->dev, "ov5693_remove...\n");

	dev->platform_data->csi_cfg(sd, 0);

	v4l2_ctrl_handler_free(&dev->ctrl_handler);
	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	kfree(dev);

	return 0;
}

static int ov5693_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct ov5693_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&client->dev, "out of memory\n");
		return -ENOMEM;
	}

	mutex_init(&dev->input_lock);

	dev->fmt_idx = 0;
	v4l2_i2c_subdev_init(&(dev->sd), client, &ov5693_ops);

#ifdef POWER_ALWAYS_ON_BEFORE_SUSPEND
	dev->second_power_on_at_boot_done = 0;
	dev->once_launched = 0;
#endif

	if (client->dev.platform_data) {
		ret = ov5693_s_config(&dev->sd, client->irq,
				      client->dev.platform_data);
		if (ret)
			goto out_free;
	}
#ifdef CONFIG_VIDEO_AD5823
	dev->vcm_driver = &ov5693_vcms[AD5823];
	dev->vcm_driver->init(&dev->sd);
#elif defined (CONFIG_VIDEO_DW9714)
	dev->vcm_driver = &ov5693_vcms[DW9714];
	dev->vcm_driver->init(&dev->sd);
#endif

	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dev->pad.flags = MEDIA_PAD_FL_SOURCE;
	dev->format.code = V4L2_MBUS_FMT_SBGGR10_1X10;
	dev->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;

	ret = media_entity_init(&dev->sd.entity, 1, &dev->pad, 0);
	if (ret)
		ov5693_remove(client);

	return ret;
out_free:
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);
	return ret;
}

#ifdef POWER_ALWAYS_ON_BEFORE_SUSPEND
static int ov5693_suspend(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov5693_device *ov_dev = to_ov5693_sensor(sd);
	int ret = 0;

	//printk("%s() in\n", __func__);

	if (ov_dev->once_launched) {
		ret = ov5693_s_power(sd, 0);
		if (ret) {
			v4l2_err(client, "ov5693 power-down err.\n");
		}
	}

	return 0;
}

static int ov5693_resume(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov5693_device *ov_dev = to_ov5693_sensor(sd);
	int ret = 0;

	//printk("%s() in\n", __func__);

	if (ov_dev->once_launched) {
		ret = ov5693_s_power(sd, 1);
		if (ret) {
			v4l2_err(client, "ov5693 power-up err.\n");
		}

		ret = ov5693_set_suspend(sd);
	}

	return 0;
}

SIMPLE_DEV_PM_OPS(ov5693_pm_ops, ov5693_suspend, ov5693_resume);
#endif

MODULE_DEVICE_TABLE(i2c, ov5693_id);
static struct i2c_driver ov5693_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = OV5693_NAME,
#ifdef POWER_ALWAYS_ON_BEFORE_SUSPEND
		   .pm = &ov5693_pm_ops,
#endif
		   },
	.probe = ov5693_probe,
	.remove = ov5693_remove,
	.id_table = ov5693_id,
};

static int init_ov5693(void)
{
	return i2c_add_driver(&ov5693_driver);
}

static void exit_ov5693(void)
{

	i2c_del_driver(&ov5693_driver);
}

module_init(init_ov5693);
module_exit(exit_ov5693);

MODULE_AUTHOR("Meng Li <meng.a.li@intel.com>");
MODULE_DESCRIPTION("A low-level driver for OmniVision 5693 sensors");
MODULE_LICENSE("GPL");
