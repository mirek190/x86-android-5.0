/*
 * Support for Sony gc5004 8MP camera sensor.
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
#include "gc5004.h"
#include <asm/intel-mid.h>
//test
static enum atomisp_bayer_order gc5004_bayer_order_mapping[] = {
	atomisp_bayer_order_gbrg,
	atomisp_bayer_order_bggr,
	atomisp_bayer_order_rggb,
	atomisp_bayer_order_grbg,
};

static u16 g_exposure, g_gain;

static int
gc5004_read_reg(struct i2c_client *client, u8 len, u8 reg, u8 *val)
{
	struct i2c_msg msg[2];
	unsigned char data[GC5004_SHORT_MAX];
	int err, i;
	int tmp = 0;

	if (len > GC5004_BYTE_MAX) {
		dev_err(&client->dev, "%s error, invalid data length\n",
			__func__);
		return -EINVAL;
	}

loop:
	memset(msg, 0 , sizeof(msg));
	memset(data, 0 , sizeof(data));
	
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;//I2C_MSG_LENGTH;
	msg[0].buf = data;
	/* high byte goes first */
//	data[0] = cpu_to_be16(reg);
        /* high byte goes out first */
//        data[0] = (u8)(reg >> 8);
  //      data[1] = (u8)(reg & 0xff);


        data[0] = reg;
	
	msg[1].addr = client->addr;
	msg[1].len = len;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = data;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err != 2) {
		if (err >= 0){
			err = -EIO;
		}
	//	if (client->addr == 0x36 && tmp != 0x80)
	//	{
	//		tmp++;	
	//		goto loop;
	//	}
		goto error;
	}

	/* high byte comes first */
	if (len == GC5004_8BIT) {
		*val = (u8)data[0];
	} else {
		/* 16-bit access is default when len > 1 */
		for (i = 0; i < (len >> 1); i++)
			val[i] = be16_to_cpu(data[i]);
	}

	return 0;

error:
	dev_err(&client->dev, "read from offset 0x%x error %d", reg, err);
	return err;
}

static int
gc5004_read_otp_data(struct i2c_client *client, u16 len, u16 reg, void *val)
{
	struct i2c_msg msg[2];
	u16 data[GC5004_SHORT_MAX] = { 0 };
	int err;

	if (len > GC5004_BYTE_MAX) {
		dev_err(&client->dev, "%s error, invalid data length\n",
			__func__);
		return -EINVAL;
	}

	memset(msg, 0 , sizeof(msg));
	memset(data, 0 , sizeof(data));

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = I2C_MSG_LENGTH;
	msg[0].buf = (u8 *)data;
	/* high byte goes first */
	data[0] = cpu_to_be16(reg);

	msg[1].addr = client->addr;
	msg[1].len = len;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = (u8 *)data;

	err = i2c_transfer(client->adapter, msg, 2);
	if (err != 2) {
		if (err >= 0)
			err = -EIO;
		goto error;
	}

	memcpy(val, data, len);
	return 0;

error:
	dev_err(&client->dev, "read from offset 0x%x error %d", reg, err);
	return err;
}

static int gc5004_i2c_write(struct i2c_client *client, u8 len, u8 *data)
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

static int
gc5004_write_reg(struct i2c_client *client, u16 data_length, u16 reg, u16 val)
{
	int ret;
	unsigned char data[6] = {0};
	u16 *wreg = (u16 *)data;
	const u16 len = data_length + 1; /* 16-bit address + data */
	u8 tmp;

	if (data_length != GC5004_8BIT && data_length != GC5004_16BIT) {
		v4l2_err(client, "%s error, invalid data_length\n", __func__);
		return -EINVAL;
	}

	/* high byte goes out first */
//	*wreg = cpu_to_be16(reg);
	data[0] = reg;
//		data[1] = (u8)(val);
	if (data_length == GC5004_8BIT)
		data[1] = (u8)(val);
	else {
		/* GC5004_16BIT */
		u16 *wdata = (u16 *)&data[2];
		*wdata = cpu_to_be16(val);
	}

	ret = gc5004_i2c_write(client, len, data);
	if (ret)
		dev_err(&client->dev,
			"write error: wrote 0x%x to offset 0x%x error %d",
			val, reg, ret);

	return ret;
}

/*
 * gc5004_write_reg_array - Initializes a list of gc5004 registers
 * @client: i2c driver client structure
 * @reglist: list of registers to be written
 *
 * This function initializes a list of registers. When consecutive addresses
 * are found in a row on the list, this function creates a buffer and sends
 * consecutive data in a single i2c_transfer().
 *
 * __gc5004_flush_reg_array, __gc5004_buf_reg_array() and
 * __gc5004_write_reg_is_consecutive() are internal functions to
 * gc5004_write_reg_array_fast() and should be not used anywhere else.
 *
 */

static int __gc5004_flush_reg_array(struct i2c_client *client,
				     struct gc5004_write_ctrl *ctrl)
{
	u16 size;

	if (ctrl->index == 0)
		return 0;

	size = sizeof(u8) + ctrl->index; /* 16-bit address + data */
	ctrl->buffer.addr = cpu_to_be16(ctrl->buffer.addr);
	ctrl->index = 0;

	return gc5004_i2c_write(client, size, (u8 *)&ctrl->buffer);
}

static int __gc5004_buf_reg_array(struct i2c_client *client,
				   struct gc5004_write_ctrl *ctrl,
				   const struct gc5004_reg *next)
{
	int size;
	u16 *data16;

	switch (next->type) {
	case GC5004_8BIT:
		size = 1;
		ctrl->buffer.data[ctrl->index] = (u8)next->val;
		break;
	case GC5004_16BIT:
		size = 2;
		data16 = (u16 *)&ctrl->buffer.data[ctrl->index];
		*data16 = cpu_to_be16((u16)next->val);
		break;
	default:
		return -EINVAL;
	}

	/* When first item is added, we need to store its starting address */
	if (ctrl->index == 0)
		ctrl->buffer.addr = next->sreg;

	ctrl->index += size;

	/*
	 * Buffer cannot guarantee free space for u32? Better flush it to avoid
	 * possible lack of memory for next item.
	 */
	if (ctrl->index + sizeof(u16) >= GC5004_MAX_WRITE_BUF_SIZE)
		return __gc5004_flush_reg_array(client, ctrl);

	return 0;
}

static int
__gc5004_write_reg_is_consecutive(struct i2c_client *client,
				   struct gc5004_write_ctrl *ctrl,
				   const struct gc5004_reg *next)
{
	if (ctrl->index == 0)
		return 1;

	return ctrl->buffer.addr + ctrl->index == next->sreg;
}

static int gc5004_write_reg_array(struct i2c_client *client,
				   const struct gc5004_reg *reglist)
{
	const struct gc5004_reg *next = reglist;
	struct gc5004_write_ctrl ctrl;
	int err;

	ctrl.index = 0;
	for (; next->type != GC5004_TOK_TERM; next++) {
		switch (next->type & GC5004_TOK_MASK) {
		case GC5004_TOK_DELAY:
			err = __gc5004_flush_reg_array(client, &ctrl);
			if (err)
				return err;
			msleep(next->val);
			break;

		default:
			gc5004_write_reg(client, GC5004_8BIT,
			       next->sreg, next->val);
	

			/*
			 * If next address is not consecutive, data needs to be
			 * flushed before proceed.
			 */
		#if 0
			if (!__gc5004_write_reg_is_consecutive(client, &ctrl,
								next)) {
				err = __gc5004_flush_reg_array(client, &ctrl);
				if (err)
					return err;
			}
			err = __gc5004_buf_reg_array(client, &ctrl, next);
			if (err) {
				v4l2_err(client, "%s: write error, aborted\n",
					 __func__);
				return err;
			}
			break;
		#endif
		}
		
	

	}

	return 0;//__gc5004_flush_reg_array(client, &ctrl);
}

static int gc5004_read_otp_reg_array(struct i2c_client *client, u16 size, u16 addr,
				  u8 *buf)
{
	u16 index;
	int ret;

	for (index = 0; index + GC5004_OTP_READ_ONETIME <= size;
					index += GC5004_OTP_READ_ONETIME) {
		ret = gc5004_read_otp_data(client, GC5004_OTP_READ_ONETIME,
					addr + index, &buf[index]);
		if (ret)
			return ret;
	}
	return 0;
}

static int __gc5004_otp_read(struct v4l2_subdev *sd, u8 *buf)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	int i;

	for (i = 0; i < GC5004_OTP_PAGE_MAX; i++) {

		/*set page NO.*/
		ret = gc5004_write_reg(client, GC5004_8BIT,
			       GC5004_OTP_PAGE_REG, i & 0xff);
		if (ret) {
			dev_err(&client->dev, "failed to prepare OTP page\n");
			return ret;
		}

		/*set read mode*/
		ret = gc5004_write_reg(client, GC5004_8BIT,
			       GC5004_OTP_MODE_REG, GC5004_OTP_MODE_READ);
		if (ret) {
			dev_err(&client->dev, "failed to set OTP reading mode page");
			return ret;
		}

		/* Reading the OTP data array */
		ret = gc5004_read_otp_reg_array(client, GC5004_OTP_PAGE_SIZE,
			GC5004_OTP_START_ADDR, buf + i * GC5004_OTP_PAGE_SIZE);
		if (ret) {
			dev_err(&client->dev, "failed to read OTP data\n");
			return ret;
		}
	}

	return 0;
}

static void *gc5004_otp_read(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 *buf;
	int ret;

	buf = devm_kzalloc(&client->dev, GC5004_OTP_DATA_SIZE, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	ret = __gc5004_otp_read(sd, buf);

	/* Driver has failed to find valid data */
	if (ret) {
		dev_err(&client->dev, "sensor found no valid OTP data\n");
		return ERR_PTR(ret);
	}

	return buf;
}

static int __gc5004_get_max_fps_index(
				const struct gc5004_fps_setting *fps_settings)
{
	int i;

	for (i = 0; i < MAX_FPS_OPTIONS_SUPPORTED; i++) {
	//	if (fps_settings[i].fps == 0)
			break;
	}

	return i - 1;
}

static int __gc5004_update_exposure_timing(struct i2c_client *client, u16 exposure,
			u16 llp, u16 fll)
{
	int ret = 0;
    	u8 expo_coarse_h,expo_coarse_l;
	u8 tmp;

	if (!exposure) exposure = 4;
	if (exposure < 4) exposure = 4;
	if (exposure > 8191) exposure = 8191;
	exposure = exposure / 4;
	exposure = exposure * 4;
	tmp = exposure % 4;
	if (tmp > 2)	
		exposure+=4;

	if (exposure < 32) {
		gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);
		gc5004_write_reg(client, GC5004_8BIT, 0x01, 0x00);
		gc5004_write_reg(client, GC5004_8BIT, 0xb0, 0x80);//travis 20140625
	} else {
		gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);
		gc5004_write_reg(client, GC5004_8BIT, 0x01, 0x10);
		gc5004_write_reg(client, GC5004_8BIT, 0xb0, 0x70);
	}

	expo_coarse_h = (u8)(exposure >> 8);
	expo_coarse_l = (u8)(exposure & 0xff);
	ret = gc5004_write_reg(client, GC5004_8BIT, GC5004_REG_EXPO_COARSE, expo_coarse_h);
	if (ret)
		return ret;
	ret = gc5004_write_reg(client, GC5004_8BIT, GC5004_REG_EXPO_COARSE+1, expo_coarse_l);
	
	return ret;
}

static int __gc5004_update_gain(struct v4l2_subdev *sd, u16 gain)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 iReg,temp;

	iReg = gain;
	
	if(iReg < 0x40)
		iReg = 0x40;
	else if((ANALOG_GAIN_1<= iReg)&&(iReg < ANALOG_GAIN_2))
	{
		//analog gain
		gc5004_write_reg(client, GC5004_8BIT, 0xb6,  0x00);// 
		temp = iReg;
		gc5004_write_reg(client, GC5004_8BIT, 0xb1, temp>>6);
		gc5004_write_reg(client, GC5004_8BIT, 0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC5004MIPI analogic gain 1x , GC5004MIPI add pregain = %d\n",temp);
	}
	else if((ANALOG_GAIN_2<= iReg)&&(iReg < ANALOG_GAIN_3))
	{
		//analog gain
		gc5004_write_reg(client, GC5004_8BIT, 0xb6,  0x01);// 
		temp = 64*iReg/ANALOG_GAIN_2;
		gc5004_write_reg(client, GC5004_8BIT, 0xb1, temp>>6);
		gc5004_write_reg(client, GC5004_8BIT, 0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC5004MIPI analogic gain 1.45x , GC5004MIPI add pregain = %d\n",temp);
	}

	else if((ANALOG_GAIN_3<= iReg)&&(iReg < ANALOG_GAIN_4))
	{
		//analog gain
		gc5004_write_reg(client, GC5004_8BIT, 0xb6,  0x02);//
		temp = 64*iReg/ANALOG_GAIN_3;
		gc5004_write_reg(client, GC5004_8BIT, 0xb1, temp>>6);
		gc5004_write_reg(client, GC5004_8BIT, 0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC5004MIPI analogic gain 2.02x , GC5004MIPI add pregain = %d\n",temp);
	}
	else if((ANALOG_GAIN_4<= iReg)&&(iReg < ANALOG_GAIN_5))
	{
		//analog gain
		gc5004_write_reg(client, GC5004_8BIT, 0xb6,  0x03);//
		temp = 64*iReg/ANALOG_GAIN_4;
		gc5004_write_reg(client, GC5004_8BIT, 0xb1, temp>>6);
		gc5004_write_reg(client, GC5004_8BIT, 0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC5004MIPI analogic gain 2.86x , GC5004MIPI add pregain = %d\n",temp);
	}

	//else if((ANALOG_GAIN_5<= iReg)&&(iReg)&&(iReg < ANALOG_GAIN_6))
	else if(ANALOG_GAIN_5<= iReg)
	{
		//analog gain
		gc5004_write_reg(client, GC5004_8BIT, 0xb6,  0x04);//
		temp = 64*iReg/ANALOG_GAIN_5;
		gc5004_write_reg(client, GC5004_8BIT, 0xb1, temp>>6);
		gc5004_write_reg(client, GC5004_8BIT, 0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC5004MIPI analogic gain 3.95x , GC5004MIPI add pregain = %d\n",temp);
	}

	return 0;
}

static int gc5004_set_exposure_gain(struct v4l2_subdev *sd, u16 coarse_itg,
	u16 gain, u16 digitgain)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	g_exposure = coarse_itg;
	g_gain = gain;

	/* Validate exposure:  cannot exceed VTS-4 where VTS is 16bit */
	coarse_itg = clamp_t(u16, coarse_itg, 0, GC5004_MAX_EXPOSURE_SUPPORTED);//fix me, GC5004_MAX_EXPOSURE_SUPPORTED need to be checked

	/* Validate gain: must not exceed maximum 8bit value */
	gain = clamp_t(u16, gain, 0, GC5004_MAX_GLOBAL_GAIN_SUPPORTED);//fix me, max need to be checked

	/* Validate digital gain: must not exceed 12 bit value*/
	digitgain = clamp_t(u16, digitgain, 0, GC5004_MAX_DIGITAL_GAIN_SUPPORTED);//fix me, max need to be checked

	mutex_lock(&dev->input_lock);

	ret = __gc5004_update_exposure_timing(client, coarse_itg,
			dev->pixels_per_line, dev->lines_per_frame);
	if (ret)
		goto out;
	dev->coarse_itg = coarse_itg;
	
	ret = __gc5004_update_gain(sd, gain);
	if (ret)
		goto out;
	dev->gain = gain;
out:
	mutex_unlock(&dev->input_lock);
	return ret;
}

static long gc5004_s_exposure(struct v4l2_subdev *sd,
			       struct atomisp_exposure *exposure)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	return gc5004_set_exposure_gain(sd, exposure->integration_time[0],
				exposure->gain[0], exposure->gain[1]);
}

/* FIXME -To be updated with real OTP reading */
static int gc5004_g_priv_int_data(struct v4l2_subdev *sd,
				   struct v4l2_private_int_data *priv)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	u8 __user *to = priv->data;
	u32 read_size = priv->size;
	int ret;

	priv->size = 0;
	return -EFAULT;



	/* No need to copy data if size is 0 */
	if (!read_size)
		goto out;

	if (IS_ERR(dev->otp_data)) {
		dev_err(&client->dev, "OTP data not available");
		return PTR_ERR(dev->otp_data);
	}
	/* Correct read_size value only if bigger than maximum */
	if (read_size > GC5004_OTP_DATA_SIZE)
		read_size = GC5004_OTP_DATA_SIZE;

	ret = copy_to_user(to, dev->otp_data, read_size);
	if (ret) {
		dev_err(&client->dev, "%s: failed to copy OTP data to user\n",
			 __func__);
		return -EFAULT;
	}
out:
	/* Return correct size */
	priv->size = GC5004_OTP_DATA_SIZE;

	return 0;
}
#if 0
static int __gc5004_init(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	if (dev->sensor_id == GC5004_ID_DEFAULT)
		return 0;

	/* Sets the default FPS 
	dev->fps_index = 0;
	dev->curr_res_table = dev->mode_tables->res_preview;
	dev->entries_curr_table = dev->mode_tables->n_res_preview;
i	*/

	return gc5004_write_reg_array(client,
			dev->mode_tables->init_settings);
}

static int gc5004_init(struct v4l2_subdev *sd, u32 val)//fix me, just see the func sequence
{
//	printk("%s++\n",__func__);

	return 0;
}
#endif
static long gc5004_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	switch (cmd) {
	case ATOMISP_IOC_S_EXPOSURE:
		return gc5004_s_exposure(sd, arg);
//	case ATOMISP_IOC_G_SENSOR_PRIV_INT_DATA:
//		return gc5004_g_priv_int_data(sd, arg);
	default:
		return -EINVAL;
	}
	return 0;
}

static int power_up(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	int ret;
	dev_info(&client->dev, "@%s ++\n", __func__);

       /* power control */
	ret = dev->platform_data->power_ctrl(sd, 1);
	if (ret)
		goto fail_power;

	/* flis clock control*/
	ret = dev->platform_data->flisclk_ctrl(sd, 1);
	if (ret)
		goto fail_clk;

	/* gpio ctrl*/
	ret = dev->platform_data->gpio_ctrl(sd, 1);
	if (ret) {
		dev_err(&client->dev, "gpio failed\n");
		goto fail_gpio;
	}
	
        /*
         * according to DS, 44ms is needed between power up and first i2c
         * commend
         */
	msleep(50);//fix me, does it need?
	return 0;

fail_gpio:
	dev->platform_data->gpio_ctrl(sd, 0);
fail_clk:
	dev->platform_data->flisclk_ctrl(sd, 0);
fail_power:
	dev->platform_data->power_ctrl(sd, 0);
	dev_err(&client->dev, "sensor power-up failed\n");

	return ret;
}

static int power_down(struct v4l2_subdev *sd)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	dev_info(&client->dev, "@%s ++\n", __func__);
	ret = dev->platform_data->flisclk_ctrl(sd, 0);
	if (ret)
		dev_err(&client->dev, "flisclk failed\n");

	/* gpio ctrl*/ 
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

static int gc5004_set_suspend(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "@%s ++\n", __func__);
	gc5004_write_reg(client, GC5004_8BIT,  0xfe, 0x03); //10bit raw disable
	gc5004_write_reg(client, GC5004_8BIT,  0x10, 0x80); //10bit raw disable
	gc5004_write_reg(client, GC5004_8BIT,  0xfe, 0x0); //10bit raw disable

	return 0;
	//return gc5004_write_reg_array(client, gc5004_suspend, POST_POLLING);
}

static int gc5004_set_streaming(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "@%s ++\n", __func__);

	gc5004_write_reg(client, GC5004_8BIT,  0xfe, 0x03); //10bit raw disable
	gc5004_write_reg(client, GC5004_8BIT,  0x10, 0x91); //10bit raw enable
	gc5004_write_reg(client, GC5004_8BIT,  0xfe, 0x0); //10bit raw disable	

	return 0;
	//return gc5004_write_reg_array(client, gc5004_streaming, POST_POLLING);
}

static int gc5004_init_common(struct v4l2_subdev *sd)  //fix me need write the regs
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = -1;

#if 0// v1

////////////////////////////////////////////////////
/////////////////////   SYS   //////////////////////
////////////////////////////////////////////////////
   gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x80);
   gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x80);
   gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x80);
   gc5004_write_reg(client, GC5004_8BIT, 0xf2, 0x00); //sync_pad_io_ebi
   gc5004_write_reg(client, GC5004_8BIT, 0xf6, 0x00); //up down
   gc5004_write_reg(client, GC5004_8BIT, 0xfc, 0x06);
   gc5004_write_reg(client, GC5004_8BIT, 0xf7, 0x1d); //pll enable
   gc5004_write_reg(client, GC5004_8BIT, 0xf8, 0x84);//0x84); //Pll mode 2
   gc5004_write_reg(client, GC5004_8BIT, 0xf9, 0xfe); //[0] pll enable
   gc5004_write_reg(client, GC5004_8BIT, 0xfa, 0x00); //div
   gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);

////////////////////////////////////////////////
///////////   ANALOG & CISCTL   ////////////////
////////////////////////////////////////////////
   gc5004_write_reg(client, GC5004_8BIT, 0x00, 0x40); //10/[4]rowskip_skip_sh
   gc5004_write_reg(client, GC5004_8BIT, 0x03, 0x07); //10fps 1800
   gc5004_write_reg(client, GC5004_8BIT, 0x04, 0x08); 
   gc5004_write_reg(client, GC5004_8BIT, 0x05, 0x01); //HB 806
   gc5004_write_reg(client, GC5004_8BIT, 0x06, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x07, 0x00); //VB 28
   gc5004_write_reg(client, GC5004_8BIT, 0x08, 0x1c); //4
   gc5004_write_reg(client, GC5004_8BIT, 0x09, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x0a, 0x02); //02//row start
   gc5004_write_reg(client, GC5004_8BIT, 0x0b, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x0c, 0x00); //0c//col start
   gc5004_write_reg(client, GC5004_8BIT, 0x0d, 0x07); //Window height 1960
   gc5004_write_reg(client, GC5004_8BIT, 0x0e, 0xa8); 
   gc5004_write_reg(client, GC5004_8BIT, 0x0f, 0x0a); //Window setting width 2624
   gc5004_write_reg(client, GC5004_8BIT, 0x10, 0x50);
   gc5004_write_reg(client, GC5004_8BIT, 0x17, 0x16); //01//14//[0]mirror [1]flip
   gc5004_write_reg(client, GC5004_8BIT, 0x18, 0x02); //sdark off
   gc5004_write_reg(client, GC5004_8BIT, 0x19, 0x06); 
   gc5004_write_reg(client, GC5004_8BIT, 0x1a, 0x13); 
   gc5004_write_reg(client, GC5004_8BIT, 0x1b, 0x48); 
   gc5004_write_reg(client, GC5004_8BIT, 0x1c, 0x05); 
   gc5004_write_reg(client, GC5004_8BIT, 0x1e, 0xb8);
   gc5004_write_reg(client, GC5004_8BIT, 0x1f, 0x78); 
   gc5004_write_reg(client, GC5004_8BIT, 0x20, 0xc5); //03/[7:6]ref_r [3:1]comv_r 
   gc5004_write_reg(client, GC5004_8BIT, 0x21, 0x4f); //7f
   gc5004_write_reg(client, GC5004_8BIT, 0x22, 0x82); //b2 
   gc5004_write_reg(client, GC5004_8BIT, 0x23, 0x43); //f1/[7:3]opa_r [1:0]sRef
   gc5004_write_reg(client, GC5004_8BIT, 0x24, 0x2f); //PAD drive 
   gc5004_write_reg(client, GC5004_8BIT, 0x2b, 0x01); 
   gc5004_write_reg(client, GC5004_8BIT, 0x2c, 0x68); //[6:4]rsgh_r 

////////////////////////////////////////////////
/////////////////   BLK   //////////////////////
////////////////////////////////////////////////
   gc5004_write_reg(client, GC5004_8BIT, 0x18, 0x02);
   gc5004_write_reg(client, GC5004_8BIT, 0x1a, 0x13);
   gc5004_write_reg(client, GC5004_8BIT, 0x40, 0x23);
   gc5004_write_reg(client, GC5004_8BIT, 0x70, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x74, 0x20);

////////////////////////////////////////////////
////////////////   Gain   //////////////////////
////////////////////////////////////////////////
   gc5004_write_reg(client, GC5004_8BIT, 0xb0, 0x40);
   gc5004_write_reg(client, GC5004_8BIT, 0xb1, 0x01);
   gc5004_write_reg(client, GC5004_8BIT, 0xb2, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xb3, 0x78);
   gc5004_write_reg(client, GC5004_8BIT, 0xb4, 0x40);
   gc5004_write_reg(client, GC5004_8BIT, 0xb5, 0x60);
   gc5004_write_reg(client, GC5004_8BIT, 0xb6, 0x00);

////////////////////////////////////////////////
/////////////////   ISP   //////////////////////
////////////////////////////////////////////////
   gc5004_write_reg(client, GC5004_8BIT, 0x80, 0x50); //10//[6]BFF_en
   gc5004_write_reg(client, GC5004_8BIT, 0x86, 0x0a); //06 //02/[3]AAA_hpclk_save_mode
   gc5004_write_reg(client, GC5004_8BIT, 0x89, 0x03); //[4]pregain bypass
   gc5004_write_reg(client, GC5004_8BIT, 0x8a, 0x83); //[7]ISP quite mode [0]DIV_gatedclk_en 
   gc5004_write_reg(client, GC5004_8BIT, 0x8b, 0x61); //[7:6]BFF gate mode [3:2]pipe gate mode
   gc5004_write_reg(client, GC5004_8BIT, 0x8c, 0x10); //test image
   gc5004_write_reg(client, GC5004_8BIT, 0x8d, 0x01); //[2]INBF_clock_gate
   gc5004_write_reg(client, GC5004_8BIT, 0x90, 0x01); 
   gc5004_write_reg(client, GC5004_8BIT, 0x92, 0x02); //00/crop win y
   gc5004_write_reg(client, GC5004_8BIT, 0x94, 0x01); //04/crop win x
   gc5004_write_reg(client, GC5004_8BIT, 0x95, 0x07); //crop win height
   gc5004_write_reg(client, GC5004_8BIT, 0x96, 0x98);
   gc5004_write_reg(client, GC5004_8BIT, 0x97, 0x0a); //crop win width
   gc5004_write_reg(client, GC5004_8BIT, 0x98, 0x20);

////////////////////////////////////////////////
/////////////////   BLK   //////////////////////
////////////////////////////////////////////////
   gc5004_write_reg(client, GC5004_8BIT, 0x40, 0x22);
   gc5004_write_reg(client, GC5004_8BIT, 0x41, 0x00); //38
   gc5004_write_reg(client, GC5004_8BIT, 0x50, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x51, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x52, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x53, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x54, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x55, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x56, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x57, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x58, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x59, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x5a, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x5b, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x5c, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x5d, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x5e, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x5f, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xd0, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xd1, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xd2, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xd3, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xd4, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xd5, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xd6, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xd7, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xd8, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xd9, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xda, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xdb, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xdc, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xdd, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xde, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0xdf, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x70, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x71, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x72, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x73, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x74, 0x20);
   gc5004_write_reg(client, GC5004_8BIT, 0x75, 0x20);
   gc5004_write_reg(client, GC5004_8BIT, 0x76, 0x20);
   gc5004_write_reg(client, GC5004_8BIT, 0x77, 0x20);

////////////////////////////////////////////////
/////////////////   GAIN   /////////////////////
////////////////////////////////////////////////
   gc5004_write_reg(client, GC5004_8BIT, 0xb0, 0x40); 
   gc5004_write_reg(client, GC5004_8BIT, 0xb1, 0x01); 
   gc5004_write_reg(client, GC5004_8BIT, 0xb2, 0x02); 
   gc5004_write_reg(client, GC5004_8BIT, 0xb3, 0x40); 
   gc5004_write_reg(client, GC5004_8BIT, 0xb4, 0x40); 
   gc5004_write_reg(client, GC5004_8BIT, 0xb5, 0x40);
   gc5004_write_reg(client, GC5004_8BIT, 0xb6, 0x00); 

////////////////////////////////////////////////
/////////////////   DNDD   /////////////////////
////////////////////////////////////////////////
   gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x02); 
   gc5004_write_reg(client, GC5004_8BIT, 0x89, 0x15); 
   gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);

////////////////////////////////////////////////
/////////////////   MIPI   /////////////////////
////////////////////////////////////////////////
   gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x03);
   gc5004_write_reg(client, GC5004_8BIT, 0x01, 0x07);
   gc5004_write_reg(client, GC5004_8BIT, 0x02, 0x33);
   gc5004_write_reg(client, GC5004_8BIT, 0x03, 0x93); //[7]lowpowerÀ­µÍÒ»µµ 
   gc5004_write_reg(client, GC5004_8BIT, 0x04, 0x80);
   gc5004_write_reg(client, GC5004_8BIT, 0x05, 0x02);
   gc5004_write_reg(client, GC5004_8BIT, 0x06, 0x80);
 //  gc5004_write_reg(client, GC5004_8BIT, 0x10, 0x91); //1 or 2 lane////1lane 90 //2lane 91
   gc5004_write_reg(client, GC5004_8BIT, 0x11, 0x2b);//2a //LDI set
   gc5004_write_reg(client, GC5004_8BIT, 0x12, 0xa8); //20
   gc5004_write_reg(client, GC5004_8BIT, 0x13, 0x0c);//0a
   gc5004_write_reg(client, GC5004_8BIT, 0x15, 0x12); //DPHYY_MODE read_ready 
   gc5004_write_reg(client, GC5004_8BIT, 0x17, 0xb0); //wdiv set 
   gc5004_write_reg(client, GC5004_8BIT, 0x18, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x19, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x1a, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x1d, 0x00);
   gc5004_write_reg(client, GC5004_8BIT, 0x42, 0x20);
   gc5004_write_reg(client, GC5004_8BIT, 0x43, 0x0a);

	gc5004_write_reg(client, GC5004_8BIT, 0x21, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0x22, 0x02);
	gc5004_write_reg(client, GC5004_8BIT, 0x23, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0x29, 0x02);
	gc5004_write_reg(client, GC5004_8BIT, 0x2a, 0x01);

   gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xf2, 0x00);
#endif

#if 0
//BGGR
//2592x1944

	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x80);
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x80);
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x80);
	gc5004_write_reg(client, GC5004_8BIT, 0xf2, 0x00); //sync_pad_io_ebi
	gc5004_write_reg(client, GC5004_8BIT, 0xf6, 0x00); //up down
	gc5004_write_reg(client, GC5004_8BIT, 0xfc, 0x06);
	gc5004_write_reg(client, GC5004_8BIT, 0xf7, 0x37); //pll enable
	gc5004_write_reg(client, GC5004_8BIT, 0xf8, 0x92); //Pll mode 2
	gc5004_write_reg(client, GC5004_8BIT, 0xf9, 0xfe); //[0] pll enable  change at 17:37 04/19
	gc5004_write_reg(client, GC5004_8BIT, 0xfa, 0x11); //div
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);

	/////////////////////////////////////////////////////
	////////////////   ANALOG & CISCTL   ////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0x00, 0x40); //10/[4]rowskip_skip_sh
	gc5004_write_reg(client, GC5004_8BIT, 0x03, 0x07); //15fps
	gc5004_write_reg(client, GC5004_8BIT, 0x04, 0x80); 
	gc5004_write_reg(client, GC5004_8BIT, 0x05, 0x01); //HB
	gc5004_write_reg(client, GC5004_8BIT, 0x06, 0x00); 
	gc5004_write_reg(client, GC5004_8BIT, 0x07, 0x00); //VB
	gc5004_write_reg(client, GC5004_8BIT, 0x08, 0x28);
	gc5004_write_reg(client, GC5004_8BIT, 0x09, 0x00); 
	gc5004_write_reg(client, GC5004_8BIT, 0x0a, 0x02); //02//row start
	gc5004_write_reg(client, GC5004_8BIT, 0x0b, 0x00);
 	gc5004_write_reg(client, GC5004_8BIT, 0x0c, 0x00); //0c//col start
	gc5004_write_reg(client, GC5004_8BIT, 0x0d, 0x07); 
	gc5004_write_reg(client, GC5004_8BIT, 0x0e, 0xa8); 
	gc5004_write_reg(client, GC5004_8BIT, 0x0f, 0x0a); //Window setting
	gc5004_write_reg(client, GC5004_8BIT, 0x10, 0x50); //50 
	gc5004_write_reg(client, GC5004_8BIT, 0x17, 0x16); //01//14//[0]mirror [1]flip
	gc5004_write_reg(client, GC5004_8BIT, 0x18, 0x02); //sdark off
	gc5004_write_reg(client, GC5004_8BIT, 0x19, 0x0c); 
	gc5004_write_reg(client, GC5004_8BIT, 0x1a, 0x13); 
	gc5004_write_reg(client, GC5004_8BIT, 0x1b, 0x48); 
	gc5004_write_reg(client, GC5004_8BIT, 0x1c, 0x05); 
	gc5004_write_reg(client, GC5004_8BIT, 0x1e, 0xb8);
	gc5004_write_reg(client, GC5004_8BIT, 0x1f, 0x78); 
	gc5004_write_reg(client, GC5004_8BIT, 0x20, 0xc5); //03/[7:6]ref_r [3:1]comv_r 
	gc5004_write_reg(client, GC5004_8BIT, 0x21, 0x4f); //7f
	gc5004_write_reg(client, GC5004_8BIT, 0x22, 0x82); //b2 
	gc5004_write_reg(client, GC5004_8BIT, 0x23, 0x43); //f1/[7:3]opa_r [1:0]sRef
	gc5004_write_reg(client, GC5004_8BIT, 0x24, 0x2f); //PAD drive 
	gc5004_write_reg(client, GC5004_8BIT, 0x2b, 0x01); 
	gc5004_write_reg(client, GC5004_8BIT, 0x2c, 0x68); //[6:4]rsgh_r 

	/////////////////////////////////////////////////////
	//////////////////////   ISP   //////////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0x86, 0x0a);
	gc5004_write_reg(client, GC5004_8BIT, 0x89, 0x03);
	gc5004_write_reg(client, GC5004_8BIT, 0x8a, 0x83);
	gc5004_write_reg(client, GC5004_8BIT, 0x8b, 0x61);
	gc5004_write_reg(client, GC5004_8BIT, 0x8c, 0x10);
	gc5004_write_reg(client, GC5004_8BIT, 0x8d, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0x90, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0x92, 0x01); //00/crop win y
	gc5004_write_reg(client, GC5004_8BIT, 0x94, 0x04); //04/crop win x  0d
	gc5004_write_reg(client, GC5004_8BIT, 0x95, 0x07); //crop win height
	gc5004_write_reg(client, GC5004_8BIT, 0x96, 0x98);
	gc5004_write_reg(client, GC5004_8BIT, 0x97, 0x0a); //crop win width
	gc5004_write_reg(client, GC5004_8BIT, 0x98, 0x20);

	/////////////////////////////////////////////////////
	//////////////////////   BLK   //////////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0x40, 0x22);
	gc5004_write_reg(client, GC5004_8BIT, 0x41, 0x00);//38
	
	gc5004_write_reg(client, GC5004_8BIT, 0x50, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x51, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x52, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x53, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x54, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x55, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x56, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x57, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x58, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x59, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5a, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5b, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5c, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5d, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5e, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5f, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd0, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd1, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd2, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd3, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd4, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd5, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd6, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd7, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd8, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd9, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xda, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xdb, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xdc, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xdd, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xde, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xdf, 0x00);
	
	gc5004_write_reg(client, GC5004_8BIT, 0x70, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x71, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x72, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x73, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x74, 0x20);
	gc5004_write_reg(client, GC5004_8BIT, 0x75, 0x20);
	gc5004_write_reg(client, GC5004_8BIT, 0x76, 0x20);
	gc5004_write_reg(client, GC5004_8BIT, 0x77, 0x20);


	/////////////////////////////////////////////////////
	//////////////////////   GAIN   /////////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0xb0, 0x50);
	gc5004_write_reg(client, GC5004_8BIT, 0xb1, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0xb2, 0x02);
	gc5004_write_reg(client, GC5004_8BIT, 0xb3, 0x40);
	gc5004_write_reg(client, GC5004_8BIT, 0xb4, 0x40);
	gc5004_write_reg(client, GC5004_8BIT, 0xb5, 0x40);
	gc5004_write_reg(client, GC5004_8BIT, 0xb6, 0x00);
	/////////////////////////////////////////////////////
	//////////////////////   DNDD ///////////////////////
	/////////////////////////////////////////////////////
  gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x02);
  gc5004_write_reg(client, GC5004_8BIT, 0x89, 0x15); 
  gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00); 

	/////////////////////////////////////////////////////
	//////////////////////   SCALER   /////////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x18, 0x02); //42
	gc5004_write_reg(client, GC5004_8BIT, 0x80, 0x10); //[4]first_dd_en  18
	gc5004_write_reg(client, GC5004_8BIT, 0x84, 0x23); //[5]auto_DD,[1:0]scaler CFA
	gc5004_write_reg(client, GC5004_8BIT, 0x87, 0x12);
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x02);
	gc5004_write_reg(client, GC5004_8BIT, 0x86, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x95, 0x07);
	gc5004_write_reg(client, GC5004_8BIT, 0x96, 0x98);
	gc5004_write_reg(client, GC5004_8BIT, 0x97, 0x0a);
	gc5004_write_reg(client, GC5004_8BIT, 0x98, 0x20);
	
	/////////////////////////////////////////////////////
	//////////////////////   MIPI   /////////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x03);
	gc5004_write_reg(client, GC5004_8BIT, 0x01, 0x07);
	gc5004_write_reg(client, GC5004_8BIT, 0x02, 0x33);
	gc5004_write_reg(client, GC5004_8BIT, 0x03, 0x93);
	gc5004_write_reg(client, GC5004_8BIT, 0x04, 0x80);
	gc5004_write_reg(client, GC5004_8BIT, 0x05, 0x02);
	gc5004_write_reg(client, GC5004_8BIT, 0x06, 0x80);
//	gc5004_write_reg(client, GC5004_8BIT, 0x10, 0x81);
	gc5004_write_reg(client, GC5004_8BIT, 0x11, 0x2b);
	gc5004_write_reg(client, GC5004_8BIT, 0x12, 0xa8);
	gc5004_write_reg(client, GC5004_8BIT, 0x13, 0x0c);
	gc5004_write_reg(client, GC5004_8BIT, 0x15, 0x10); // 0X10
	gc5004_write_reg(client, GC5004_8BIT, 0x17, 0xb0);
	gc5004_write_reg(client, GC5004_8BIT, 0x18, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x19, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x1a, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x1d, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x42, 0x20);
	gc5004_write_reg(client, GC5004_8BIT, 0x43, 0x0a);
	
	gc5004_write_reg(client, GC5004_8BIT, 0x21, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0x22, 0x02);
	gc5004_write_reg(client, GC5004_8BIT, 0x23, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0x29, 0x03);
	gc5004_write_reg(client, GC5004_8BIT, 0x2a, 0x07);
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);

#endif



#if 1
//v4
//BGGR
//2592x1944

	if (0 != (ret = gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x80))) {
		dev_err(&client->dev, "%s:init common error", __func__);
		return ret;
	}
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x80);
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x80);
	gc5004_write_reg(client, GC5004_8BIT, 0xf2, 0x00); //sync_pad_io_ebi
	gc5004_write_reg(client, GC5004_8BIT, 0xf6, 0x00); //up down
	gc5004_write_reg(client, GC5004_8BIT, 0xfc, 0x06);
	gc5004_write_reg(client, GC5004_8BIT, 0xf7, 0x1d); //pll enable
	gc5004_write_reg(client, GC5004_8BIT, 0xf8, 0x84); //Pll mode 2
	gc5004_write_reg(client, GC5004_8BIT, 0xf9, 0xfe); //[0] pll enable  change at 17:37 04/19
	gc5004_write_reg(client, GC5004_8BIT, 0xfa, 0x00); //div
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);

	/////////////////////////////////////////////////////
	////////////////   ANALOG & CISCTL   ////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0x00, 0x40); //10/[4]rowskip_skip_sh
	gc5004_write_reg(client, GC5004_8BIT, 0x03, 0x07); //15fps
	gc5004_write_reg(client, GC5004_8BIT, 0x04, 0x80); 
	gc5004_write_reg(client, GC5004_8BIT, 0x05, 0x03); //HB
	gc5004_write_reg(client, GC5004_8BIT, 0x06, 0x26); 
	gc5004_write_reg(client, GC5004_8BIT, 0x07, 0x00); //VB
	gc5004_write_reg(client, GC5004_8BIT, 0x08, 0x28);
	gc5004_write_reg(client, GC5004_8BIT, 0x09, 0x00); 
	gc5004_write_reg(client, GC5004_8BIT, 0x0a, 0x02); //02//row start
	gc5004_write_reg(client, GC5004_8BIT, 0x0b, 0x00);
 	gc5004_write_reg(client, GC5004_8BIT, 0x0c, 0x00); //0c//col start
	gc5004_write_reg(client, GC5004_8BIT, 0x0d, 0x07); 
	gc5004_write_reg(client, GC5004_8BIT, 0x0e, 0xa8); 
	gc5004_write_reg(client, GC5004_8BIT, 0x0f, 0x0a); //Window setting
	gc5004_write_reg(client, GC5004_8BIT, 0x10, 0x50); //50 
	gc5004_write_reg(client, GC5004_8BIT, 0x17, 0x16); //01//14//[0]mirror [1]flip
	gc5004_write_reg(client, GC5004_8BIT, 0x18, 0x02); //sdark off
	gc5004_write_reg(client, GC5004_8BIT, 0x19, 0x0c); 
	gc5004_write_reg(client, GC5004_8BIT, 0x1a, 0x13); 
	gc5004_write_reg(client, GC5004_8BIT, 0x1b, 0x48); 
	gc5004_write_reg(client, GC5004_8BIT, 0x1c, 0x05); 
	gc5004_write_reg(client, GC5004_8BIT, 0x1e, 0xb8);
	gc5004_write_reg(client, GC5004_8BIT, 0x1f, 0x78); 
	gc5004_write_reg(client, GC5004_8BIT, 0x20, 0xc5); //03/[7:6]ref_r [3:1]comv_r 
	gc5004_write_reg(client, GC5004_8BIT, 0x21, 0x4f); //7f
	gc5004_write_reg(client, GC5004_8BIT, 0x22, 0xb2); //82 travis 20140625
	gc5004_write_reg(client, GC5004_8BIT, 0x23, 0x43); //f1/[7:3]opa_r [1:0]sRef
	gc5004_write_reg(client, GC5004_8BIT, 0x24, 0x2f); //PAD drive 
	gc5004_write_reg(client, GC5004_8BIT, 0x2b, 0x01); 
	gc5004_write_reg(client, GC5004_8BIT, 0x2c, 0x68); //[6:4]rsgh_r 

	/////////////////////////////////////////////////////
	//////////////////////   ISP   //////////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0x86, 0x0a);
	gc5004_write_reg(client, GC5004_8BIT, 0x89, 0x03);
	gc5004_write_reg(client, GC5004_8BIT, 0x8a, 0x83);
	gc5004_write_reg(client, GC5004_8BIT, 0x8b, 0x61);
	gc5004_write_reg(client, GC5004_8BIT, 0x8c, 0x10);
	gc5004_write_reg(client, GC5004_8BIT, 0x8d, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0x90, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0x92, 0x02); //00/crop win y
	gc5004_write_reg(client, GC5004_8BIT, 0x94, 0x01); //04/crop win x  0d
	gc5004_write_reg(client, GC5004_8BIT, 0x95, 0x07); //crop win height
	gc5004_write_reg(client, GC5004_8BIT, 0x96, 0x98);
	gc5004_write_reg(client, GC5004_8BIT, 0x97, 0x0a); //crop win width
	gc5004_write_reg(client, GC5004_8BIT, 0x98, 0x20);

	/////////////////////////////////////////////////////
	//////////////////////   BLK   //////////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0x40, 0x72);//22 travis 20140625
	gc5004_write_reg(client, GC5004_8BIT, 0x41, 0x00);
	
	gc5004_write_reg(client, GC5004_8BIT, 0x50, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x51, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x52, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x53, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x54, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x55, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x56, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x57, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x58, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x59, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5a, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5b, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5c, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5d, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5e, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x5f, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd0, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd1, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd2, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd3, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd4, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd5, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd6, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd7, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd8, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xd9, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xda, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xdb, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xdc, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xdd, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xde, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xdf, 0x00);
	
	gc5004_write_reg(client, GC5004_8BIT, 0x70, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x71, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x72, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x73, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x74, 0x20);
	gc5004_write_reg(client, GC5004_8BIT, 0x75, 0x20);
	gc5004_write_reg(client, GC5004_8BIT, 0x76, 0x20);
	gc5004_write_reg(client, GC5004_8BIT, 0x77, 0x20);


	/////////////////////////////////////////////////////
	//////////////////////   GAIN   /////////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0xb0, 0x70);//50 travis 20140625
	gc5004_write_reg(client, GC5004_8BIT, 0xb1, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0xb2, 0x02);
	gc5004_write_reg(client, GC5004_8BIT, 0xb3, 0x40);
	gc5004_write_reg(client, GC5004_8BIT, 0xb4, 0x40);
	gc5004_write_reg(client, GC5004_8BIT, 0xb5, 0x40);
	gc5004_write_reg(client, GC5004_8BIT, 0xb6, 0x00);
	/////////////////////////////////////////////////////
	//////////////////////   DNDD ///////////////////////
	/////////////////////////////////////////////////////
  gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x02);
  gc5004_write_reg(client, GC5004_8BIT, 0x89, 0x15); 
  gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00); 

	/////////////////////////////////////////////////////
	//////////////////////   SCALER   /////////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x18, 0x02); //42
	gc5004_write_reg(client, GC5004_8BIT, 0x80, 0x10); //[4]first_dd_en  18
	//gc5004_write_reg(client, GC5004_8BIT, 0x84, 0x23); //[5]auto_DD,[1:0]scaler CFA
	gc5004_write_reg(client, GC5004_8BIT, 0x87, 0x12);
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x02);
	gc5004_write_reg(client, GC5004_8BIT, 0x86, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x95, 0x07);
	gc5004_write_reg(client, GC5004_8BIT, 0x96, 0x98);
	gc5004_write_reg(client, GC5004_8BIT, 0x97, 0x0a);
	gc5004_write_reg(client, GC5004_8BIT, 0x98, 0x20);
	
	/////////////////////////////////////////////////////
	//////////////////////   MIPI   /////////////////////
	/////////////////////////////////////////////////////
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x03);
	gc5004_write_reg(client, GC5004_8BIT, 0x01, 0x07);
	gc5004_write_reg(client, GC5004_8BIT, 0x02, 0x33);
	gc5004_write_reg(client, GC5004_8BIT, 0x03, 0x93);
	gc5004_write_reg(client, GC5004_8BIT, 0x04, 0x80);
	gc5004_write_reg(client, GC5004_8BIT, 0x05, 0x02);
	gc5004_write_reg(client, GC5004_8BIT, 0x06, 0x80);
	//gc5004_write_reg(client, GC5004_8BIT, 0x10, 0x81);
	gc5004_write_reg(client, GC5004_8BIT, 0x11, 0x2b);
	gc5004_write_reg(client, GC5004_8BIT, 0x12, 0xa8);
	gc5004_write_reg(client, GC5004_8BIT, 0x13, 0x0c);
	gc5004_write_reg(client, GC5004_8BIT, 0x15, 0x12); // 0X10
	gc5004_write_reg(client, GC5004_8BIT, 0x17, 0xb0);
	gc5004_write_reg(client, GC5004_8BIT, 0x18, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x19, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x1a, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x1d, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0x42, 0x20);
	gc5004_write_reg(client, GC5004_8BIT, 0x43, 0x0a);
	
	gc5004_write_reg(client, GC5004_8BIT, 0x21, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0x22, 0x02);
	gc5004_write_reg(client, GC5004_8BIT, 0x23, 0x01);
	gc5004_write_reg(client, GC5004_8BIT, 0x29, 0x02);/**/
	gc5004_write_reg(client, GC5004_8BIT, 0x2a, 0x07);/*01 */
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x00);
	gc5004_write_reg(client, GC5004_8BIT, 0xf2, 0x00);

#endif

	return 0;
}

static int __gc5004_s_power(struct v4l2_subdev *sd, int on)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	int ret = 0;

	if (on == 0) {
		ret = power_down(sd);
		dev->power = 0;
	} else {
		ret = power_up(sd);
		if (!ret) {
			dev->power = 1;
			return gc5004_init_common(sd);
		}
	}

	return ret;
}

static int gc5004_s_power(struct v4l2_subdev *sd, int on)
{
	int ret;
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	mutex_lock(&dev->input_lock);
	ret = __gc5004_s_power(sd, on);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int gc5004_g_chip_ident(struct v4l2_subdev *sd,
				struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!chip)
		return -EINVAL;

	v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_GC, 0);//fix me, what the V4L2_IDENT_GC mean?

	return 0;
}

static int gc5004_get_intg_factor(struct i2c_client *client,
				struct camera_mipi_info *info,
				const struct gc5004_reg *reglist)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	u32 vt_pix_clk_div;
	u32 vt_sys_clk_div;
	u32 pre_pll_clk_div;
	u32 pll_multiplier;
	u32 op_pix_clk_div;
	u32 op_sys_clk_div;

	const int ext_clk_freq_hz = 19200000;
	struct atomisp_sensor_mode_data *buf = &info->data;
	int ret;
	u8 data[GC5004_INTG_BUF_COUNT];

	u32 vt_pix_clk_freq_mhz;
	u32 coarse_integration_time_min;
	u32 coarse_integration_time_max_margin;
	u32 frame_length_lines;
	u32 line_length_pck;
	u32 read_mode;
	u32 div;
	u16 tmp;

	if (info == NULL)
		return -EINVAL;

	ret =  gc5004_write_reg(client, 1, 0xfe, 0);
	if (ret)
		return ret;

	memset(data, 0, GC5004_INTG_BUF_COUNT);
//	ret = gc5004_read_reg(client, 4, GC5004_COARSE_INTG_TIME_MIN, data);
//	if (ret)
//		return ret;
	coarse_integration_time_min = 1; //data[0];
	coarse_integration_time_max_margin = 6;//data[1];

	/* Get the cropping and output resolution to ISP for this mode. */


	tmp = 0;
	ret = gc5004_read_reg(client, 1, GC5004_HORIZONTAL_START_H, data);
	if (ret)
		return ret;
	tmp = (data[0] & 0x0f) << 8;
	ret =  gc5004_read_reg(client, 1, GC5004_HORIZONTAL_START_L, data);
	if (ret)
		return ret;
	tmp += data[0];
	buf->crop_horizontal_start = tmp;

	tmp = 0;
	ret = gc5004_read_reg(client, 1, GC5004_VERTICAL_START_H, data);
	if (ret)
		return ret;
	tmp = (data[0] & 0x07) << 8;
	ret =  gc5004_read_reg(client, 1, GC5004_VERTICAL_START_L, data);
	if (ret)
		return ret;
	tmp += data[0];
	buf->crop_vertical_start = tmp;

	tmp = 0;
	ret = gc5004_read_reg(client, 1, GC5004_HORIZONTAL_OUTPUT_SIZE_H, data);
	if (ret)
		return ret;
	tmp = (data[0] & 0x0f) << 8;
	ret = gc5004_read_reg(client, 1, GC5004_HORIZONTAL_OUTPUT_SIZE_L, data);
	if (ret)
		return ret;
	tmp += data[0];
	buf->output_width = dev->curr_res_table[dev->fmt_idx].width;
	buf->crop_horizontal_end = buf->crop_horizontal_start + tmp - 1;

	tmp = 0;
	ret = gc5004_read_reg(client, 1, GC5004_VERTICAL_OUTPUT_SIZE_H, data);
	if (ret)
		return ret;
	tmp = (data[0] & 0x07) << 8;
	ret = gc5004_read_reg(client, 1, GC5004_VERTICAL_OUTPUT_SIZE_L, data);
	if (ret)
		return ret;
	tmp += data[0];
	buf->output_height = dev->curr_res_table[dev->fmt_idx].height;
	buf->crop_vertical_end = tmp + buf->crop_vertical_start - 1;


	vt_pix_clk_freq_mhz = 96000000;//ext_clk_freq_hz / 2;

	dev->vt_pix_clk_freq_mhz = vt_pix_clk_freq_mhz;

	buf->vt_pix_clk_freq_mhz = vt_pix_clk_freq_mhz;
	buf->coarse_integration_time_min = coarse_integration_time_min;
	buf->coarse_integration_time_max_margin =
				coarse_integration_time_max_margin;

	buf->fine_integration_time_min = GC5004_FINE_INTG_TIME;
	buf->fine_integration_time_max_margin = GC5004_FINE_INTG_TIME;
	buf->fine_integration_time_def = GC5004_FINE_INTG_TIME;
	buf->frame_length_lines = dev->curr_res_table[dev->fmt_idx].lines_per_frame; //frame_length_lines;
	buf->line_length_pck = dev->curr_res_table[dev->fmt_idx].pixels_per_line;//line_length_pck;
	buf->read_mode = 0; //res->bin_mode;//read_mode;

	tmp =0;
	ret = gc5004_read_reg(client, GC5004_8BIT, REG_HORI_BLANKING_H, data);
	tmp = (data[0] & 0x0f) << 8;
	ret = gc5004_read_reg(client, GC5004_8BIT, REG_HORI_BLANKING_L, data);
	tmp += data[0];

	ret = gc5004_read_reg(client, GC5004_8BIT,  REG_SH_DELAY_H, data);
	if(ret)
		return ret;
	tmp += data[0] << 8;

	ret = gc5004_read_reg(client, GC5004_8BIT,  REG_SH_DELAY_L, data);
	if(ret)
		return ret;
	tmp += data[0];
	//buf->line_length_pck = buf->output_width + tmp + 4;

	tmp =0;
	ret = gc5004_read_reg(client, GC5004_8BIT,  REG_VERT_DUMMY_H, data);
	tmp = (data[0] & 0x1f) << 8;
	ret = gc5004_read_reg(client, GC5004_8BIT,  REG_VERT_DUMMY_L, data);
	tmp += data[0];
	//buf->frame_length_lines = buf->output_height + tmp;

	buf->binning_factor_x = 1;
	buf->binning_factor_y = 1;

	return 0;
}


/* This returns the exposure time being used. This should only be used
   for filling in EXIF data, not for actual image processing. */
static int gc5004_q_exposure(struct v4l2_subdev *sd, s32 *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 coarse;
	u8 tmp;
	int ret;

	/* the fine integration time is currently not calculated */
	ret = gc5004_read_reg(client, GC5004_8BIT,
			       GC5004_REG_EXPO_COARSE, &tmp);
	if(ret != 0)
		return ret;	

	coarse = (u16)((tmp & 0x1f) << 8);

	ret = gc5004_read_reg(client, GC5004_8BIT,
			       GC5004_REG_EXPO_COARSE + 1, &tmp);

	if(ret != 0)
		return ret;	
	
	coarse += (u16)tmp;

	*value = coarse;

	return ret;
}

static int gc5004_test_pattern(struct v4l2_subdev *sd, s32 value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return gc5004_write_reg(client, GC5004_8BIT, GC5004_TEST_PATTERN_MODE, value);
}

static enum v4l2_mbus_pixelcode
gc5004_translate_bayer_order(enum atomisp_bayer_order code)
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

static int gc5004_v_flip(struct v4l2_subdev *sd, s32 value)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	struct camera_mipi_info *gc5004_info = NULL;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	u8 val;

//	ret = gc5004_write_reg_array(client, gc5004_param_hold);  fix me, don't need?
//	if (ret)
//		return ret;
	ret = gc5004_read_reg(client, GC5004_8BIT, GC5004_IMG_ORIENTATION, &val);
	if (ret)
		return ret;
	if (value)
		val |= GC5004_VFLIP_BIT;
	else
		val &= ~GC5004_VFLIP_BIT;
	ret = gc5004_write_reg(client, GC5004_8BIT,
			GC5004_IMG_ORIENTATION, val);
	if (ret)
		return ret;

	gc5004_info = v4l2_get_subdev_hostdata(sd);
	if (gc5004_info) {
		val &= (GC5004_VFLIP_BIT|GC5004_HFLIP_BIT);
		gc5004_info->raw_bayer_order = gc5004_bayer_order_mapping[val];
		dev->format.code = gc5004_translate_bayer_order(
			gc5004_info->raw_bayer_order);
	}

	return 0;
}

static int gc5004_h_flip(struct v4l2_subdev *sd, s32 value)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	struct camera_mipi_info *gc5004_info = NULL;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	u8 val;

	ret = gc5004_read_reg(client, GC5004_8BIT, GC5004_IMG_ORIENTATION, &val);
	if (ret)
		return ret;
	if (value)
		val |= GC5004_HFLIP_BIT;
	else
		val &= ~GC5004_HFLIP_BIT;
	ret = gc5004_write_reg(client, GC5004_8BIT,
			GC5004_IMG_ORIENTATION, val);
	if (ret)
		return ret;

	gc5004_info = v4l2_get_subdev_hostdata(sd);
	if (gc5004_info) {
		val &= (GC5004_VFLIP_BIT|GC5004_HFLIP_BIT);
		gc5004_info->raw_bayer_order = gc5004_bayer_order_mapping[val];
		dev->format.code = gc5004_translate_bayer_order(
		gc5004_info->raw_bayer_order);
	}

	return 0;
}

static int gc5004_g_focal(struct v4l2_subdev *sd, s32 *val)
{
	*val = (GC5004_FOCAL_LENGTH_NUM << 16) | GC5004_FOCAL_LENGTH_DEM;
	return 0;
}

static int gc5004_g_fnumber(struct v4l2_subdev *sd, s32 *val)
{
	/*const f number for gc5004*/
	*val = (GC5004_F_NUMBER_DEFAULT_NUM << 16) | GC5004_F_NUMBER_DEM;
	return 0;
}

static int gc5004_g_fnumber_range(struct v4l2_subdev *sd, s32 *val)
{
	*val = (GC5004_F_NUMBER_DEFAULT_NUM << 24) |
		(GC5004_F_NUMBER_DEM << 16) |
		(GC5004_F_NUMBER_DEFAULT_NUM << 8) | GC5004_F_NUMBER_DEM;
	return 0;
}

static int gc5004_g_bin_factor_x(struct v4l2_subdev *sd, s32 *val)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	*val = dev->curr_res_table[dev->fmt_idx].bin_factor_x;

	return 0;
}

static int gc5004_g_bin_factor_y(struct v4l2_subdev *sd, s32 *val)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	*val = dev->curr_res_table[dev->fmt_idx].bin_factor_y;

	return 0;
}

int gc5004_vcm_power_up(struct v4l2_subdev *sd)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	
	if (dev->vcm_driver && dev->vcm_driver->power_up)
		return dev->vcm_driver->power_up(sd);
	return 0;
}

int gc5004_vcm_power_down(struct v4l2_subdev *sd)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	
	if (dev->vcm_driver && dev->vcm_driver->power_down)
		return dev->vcm_driver->power_down(sd);
	return 0;
}

int gc5004_vcm_init(struct v4l2_subdev *sd)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	
	if (dev->vcm_driver && dev->vcm_driver->init)
		return dev->vcm_driver->init(sd);
	return 0;
}

int gc5004_t_focus_vcm(struct v4l2_subdev *sd, u16 val)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	
	if (dev->vcm_driver && dev->vcm_driver->t_focus_vcm)
		return dev->vcm_driver->t_focus_vcm(sd, val);
	return 0;
}

int gc5004_t_focus_abs(struct v4l2_subdev *sd, s32 value)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	
	if (dev->vcm_driver && dev->vcm_driver->t_focus_abs)
		return dev->vcm_driver->t_focus_abs(sd, value);
	return 0;
}

int gc5004_t_vcm_slew(struct v4l2_subdev *sd, s32 value)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_vcm_slew)
		return dev->vcm_driver->t_vcm_slew(sd, value);
	return 0;
}

int gc5004_t_vcm_timing(struct v4l2_subdev *sd, s32 value)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	if (dev->vcm_driver && dev->vcm_driver->t_vcm_timing)
		return dev->vcm_driver->t_vcm_timing(sd, value);
	return 0;
}


int gc5004_t_focus_rel(struct v4l2_subdev *sd, s32 value)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	
	if (dev->vcm_driver && dev->vcm_driver->t_focus_rel)
		return dev->vcm_driver->t_focus_rel(sd, value);
	return 0;
}

int gc5004_q_focus_status(struct v4l2_subdev *sd, s32 *value)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	
	if (dev->vcm_driver && dev->vcm_driver->q_focus_status)
		return dev->vcm_driver->q_focus_status(sd, value);
	return 0;
}

int gc5004_q_focus_abs(struct v4l2_subdev *sd, s32 *value)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	
	if (dev->vcm_driver && dev->vcm_driver->q_focus_abs)
		return dev->vcm_driver->q_focus_abs(sd, value);
	return 0;
}

/* This returns the exposure time being used. This should only be used
   for filling in EXIF data, not for actual image processing. */
static int gc5004_g_exposure(struct v4l2_subdev *sd, s32 *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 coarse;
	u8 reg_val_h,reg_val_l;
	int ret;

	/* the fine integration time is currently not calculated */
	ret = gc5004_read_reg(client, GC5004_8BIT,
			       GC5004_REG_EXPO_COARSE, &reg_val_h);
	if (ret)
		return ret;

	coarse = ((u16)(reg_val_h & 0x1f)) <<8;
	
	ret = gc5004_read_reg(client, GC5004_8BIT,
			       GC5004_REG_EXPO_COARSE + 1, &reg_val_l);
	if (ret)
		return ret;

	coarse |= reg_val_l;

	*value = coarse;
	return 0;
}

#if 0
struct gc5004_control gc5004_controls[] = {
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
		.query = gc5004_q_exposure,
	},
	{
		.qc = {
			.id = V4L2_CID_TEST_PATTERN,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Test pattern",
			.minimum = 0,
			.maximum = 0xffff,
			.step = 1,
			.default_value = 0,
		},
		.tweak = gc5004_test_pattern,
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
		.tweak = gc5004_v_flip,
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
		.tweak = gc5004_h_flip,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCUS_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focus move absolute",
			.minimum = 0,
			.maximum = GC5004_MAX_FOCUS_POS,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = gc5004_t_focus_abs,
		.query = gc5004_q_focus_abs,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCUS_RELATIVE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focus move relative",
			.minimum = GC5004_MAX_FOCUS_NEG,
			.maximum = GC5004_MAX_FOCUS_POS,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = gc5004_t_focus_rel,
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
		.tweak = gc5004_t_focus_abs,
		.query = gc5004_q_focus_status,
	},
	
	{
		.qc = {
			.id = V4L2_CID_VCM_SLEW,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "vcm slew",
			.minimum = 0,
			.maximum = GC5004_VCM_SLEW_STEP_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = gc5004_t_vcm_slew,
	},
	{
		.qc = {
			.id = V4L2_CID_VCM_TIMEING,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "vcm step time",
			.minimum = 0,
			.maximum = GC5004_VCM_SLEW_TIME_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = gc5004_t_vcm_timing,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCAL_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focal length",
			.minimum = GC5004_FOCAL_LENGTH_DEFAULT,
			.maximum = GC5004_FOCAL_LENGTH_DEFAULT,
			.step = 0x01,
			.default_value = GC5004_FOCAL_LENGTH_DEFAULT,
			.flags = 0,
		},
		.query = gc5004_g_focal,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number",
			.minimum = GC5004_F_NUMBER_DEFAULT,
			.maximum = GC5004_F_NUMBER_DEFAULT,
			.step = 0x01,
			.default_value = GC5004_F_NUMBER_DEFAULT,
			.flags = 0,
		},
		.query = gc5004_g_fnumber,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_RANGE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number range",
			.minimum = GC5004_F_NUMBER_RANGE,
			.maximum =  GC5004_F_NUMBER_RANGE,
			.step = 0x01,
			.default_value = GC5004_F_NUMBER_RANGE,
			.flags = 0,
		},
		.query = gc5004_g_fnumber_range,
	},
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
		.query = gc5004_g_exposure,
	},
};

#define N_CONTROLS (ARRAY_SIZE(gc5004_controls))

static struct gc5004_control *gc5004_find_control(u32 id)
{
	int i;

	for (i = 0; i < N_CONTROLS; i++)
		if (gc5004_controls[i].qc.id == id)
			return &gc5004_controls[i];
	return NULL;
}
#endif
static int gc5004_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct gc5004_device *dev = container_of(
		ctrl->handler, struct gc5004_device, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		dev_dbg(&client->dev, "%s: CID_VFLIP:%d.\n", __func__, ctrl->val);
		ret = gc5004_v_flip(&dev->sd, ctrl->val);
		break;
	case V4L2_CID_HFLIP:
		dev_dbg(&client->dev, "%s: CID_HFLIP:%d.\n", __func__, ctrl->val);
		ret = gc5004_h_flip(&dev->sd, ctrl->val);
		break;
	}

	return ret;
}

static int gc5004_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct gc5004_device *dev = container_of(
		ctrl->handler, struct gc5004_device, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE_ABSOLUTE:
		ret = gc5004_g_exposure(&dev->sd, &ctrl->val);
		break;
	case V4L2_CID_FOCAL_ABSOLUTE:
		ret = gc5004_g_focal(&dev->sd, &ctrl->val);
		break;
	case V4L2_CID_FNUMBER_ABSOLUTE:
		ret = gc5004_g_fnumber(&dev->sd, &ctrl->val);
		break;
	case V4L2_CID_FNUMBER_RANGE:
		ret = gc5004_g_fnumber_range(&dev->sd, &ctrl->val);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}
static const struct v4l2_ctrl_ops ctrl_ops = {
	.s_ctrl = gc5004_s_ctrl,
	.g_volatile_ctrl = gc5004_g_ctrl
};

struct v4l2_ctrl_config gc5004_controls[] = {
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
		.min = GC5004_FOCAL_LENGTH_DEFAULT,
		.max = GC5004_FOCAL_LENGTH_DEFAULT,
		.step = 0x01,
		.def = GC5004_FOCAL_LENGTH_DEFAULT,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_FNUMBER_ABSOLUTE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "f-number",
		.min = GC5004_F_NUMBER_DEFAULT,
		.max = GC5004_F_NUMBER_DEFAULT,
		.step = 0x01,
		.def = GC5004_F_NUMBER_DEFAULT,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &ctrl_ops,
		.id = V4L2_CID_FNUMBER_RANGE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "f-number range",
		.min = GC5004_F_NUMBER_RANGE,
		.max =  GC5004_F_NUMBER_RANGE,
		.step = 0x01,
		.def = GC5004_F_NUMBER_RANGE,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
};


#if 0
static int gc5004_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	struct gc5004_control *ctrl = gc5004_find_control(qc->id);
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	if (ctrl == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	*qc = ctrl->qc;
	mutex_unlock(&dev->input_lock);

	return 0;
}

/* gc5004 control set/get */
static int gc5004_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct gc5004_control *s_ctrl;
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	int ret;

	if (!ctrl)
		return -EINVAL;

	s_ctrl = gc5004_find_control(ctrl->id);
	if ((s_ctrl == NULL) || (s_ctrl->query == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = s_ctrl->query(sd, &ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int gc5004_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct gc5004_control *octrl = gc5004_find_control(ctrl->id);
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	int ret;

	if ((octrl == NULL) || (octrl->tweak == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = octrl->tweak(sd, ctrl->value);
	mutex_unlock(&dev->input_lock);

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
#define LARGEST_ALLOWED_RATIO_MISMATCH 600
static int distance(struct gc5004_resolution const *res, u32 w, u32 h)
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
static int nearest_resolution_index(struct v4l2_subdev *sd, int w, int h)
{
	int i;
	int idx = -1;
	int dist;
	int min_dist = INT_MAX;
	const struct gc5004_resolution *tmp_res = NULL;
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	for (i = 0; i < dev->entries_curr_table; i++) {
		tmp_res = &dev->curr_res_table[i];
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

static int get_resolution_index(struct v4l2_subdev *sd, int w, int h)
{
	int i;
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	for (i = 0; i < dev->entries_curr_table; i++) {
		if (w != dev->curr_res_table[i].width)
			continue;
		if (h != dev->curr_res_table[i].height)
			continue;
		/* Found it */
		return i;
	}
	return -1;
}

static int gc5004_try_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	int idx = 0;

	mutex_lock(&dev->input_lock);

	if ((fmt->width > gc5004_max_res[0].res_max_width)
		|| (fmt->height > gc5004_max_res[0].res_max_height)) {
		fmt->width =  gc5004_max_res[0].res_max_width;
		fmt->height = gc5004_max_res[0].res_max_height;
	} else {
		idx = nearest_resolution_index(sd, fmt->width, fmt->height);

		/*
		 * nearest_resolution_index() doesn't return smaller
		 *  resolutions. If it fails, it means the requested
		 *  resolution is higher than wecan support. Fallback
		 *  to highest possible resolution in this case.
		 */
		if (idx == -1)
			idx = dev->entries_curr_table - 1;

		fmt->width = dev->curr_res_table[idx].width;
		fmt->height = dev->curr_res_table[idx].height;
	}
	fmt->code = dev->format.code;

	mutex_unlock(&dev->input_lock);
	return 0;
}

static int gc5004_s_mbus_fmt(struct v4l2_subdev *sd,
			      struct v4l2_mbus_framefmt *fmt)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	const struct gc5004_reg *gc5004_def_reg;
	struct camera_mipi_info *gc5004_info = NULL;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	u16 val;
	dev_info(&client->dev, "@%s ++\n", __func__);

	gc5004_info = v4l2_get_subdev_hostdata(sd);
	if (gc5004_info == NULL)
		return -EINVAL;

	ret = gc5004_try_mbus_fmt(sd, fmt);
	if (ret)
		return ret;

	mutex_lock(&dev->input_lock);

	dev->fmt_idx = get_resolution_index(sd, fmt->width, fmt->height);
	printk(KERN_INFO"@%s: width=%d, height=%d\n", __func__, fmt->width, fmt->height);
	/* Sanity check */
	if (unlikely(dev->fmt_idx == -1)) {
		ret = -EINVAL;
		goto out;
	}

	gc5004_def_reg = dev->curr_res_table[dev->fmt_idx].regs;
	ret = gc5004_write_reg_array(client, gc5004_def_reg);
	if (ret)
		goto out;

	dev->fps = dev->curr_res_table[dev->fmt_idx].fps;
	dev->pixels_per_line =
		dev->curr_res_table[dev->fmt_idx].pixels_per_line;
	dev->lines_per_frame =
		dev->curr_res_table[dev->fmt_idx].lines_per_frame;
	ret = __gc5004_update_exposure_timing(client, dev->coarse_itg,
		dev->pixels_per_line, dev->lines_per_frame);

	if (ret)
		goto out;

	ret = gc5004_get_intg_factor(client, gc5004_info, gc5004_def_reg);
	if (ret)
		goto out;

	ret = gc5004_read_reg(client, GC5004_8BIT, GC5004_IMG_ORIENTATION, &val);
	if (ret)
		goto out;
	val &= (GC5004_VFLIP_BIT|GC5004_HFLIP_BIT);
	gc5004_info->raw_bayer_order = gc5004_bayer_order_mapping[val];
	dev->format.code = gc5004_translate_bayer_order(
		gc5004_info->raw_bayer_order);
	
	gc5004_write_reg(client, GC5004_8BIT, 0xfe, 0x30);
	__gc5004_update_exposure_timing(client, g_exposure, 0, 0);
	__gc5004_update_gain(sd, g_gain);
	msleep(20);
out:
	mutex_unlock(&dev->input_lock);
	return ret;
}


static int gc5004_g_mbus_fmt(struct v4l2_subdev *sd,
			      struct v4l2_mbus_framefmt *fmt)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	if (!fmt)
		return -EINVAL;

	fmt->width = dev->curr_res_table[dev->fmt_idx].width;
	fmt->height = dev->curr_res_table[dev->fmt_idx].height;
	fmt->code = dev->format.code;

	return 0;
}

static int gc5004_detect(struct i2c_client *client, u16 *id)
{
	struct i2c_adapter *adapter = client->adapter;
	u8 id_l, id_h;

	/* i2c check */
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -ENODEV;

	/* check sensor chip ID	 */
	if (gc5004_read_reg(client, GC5004_8BIT, GC5004_REG_SENSOR_ID_HIGH_BIT, &id_h)) {
		v4l2_err(client, "sensor id = 0x%x\n", id_h);
		return -ENODEV;
	}
	*id = (u16)(id_h << 0x8);

	if (gc5004_read_reg(client, GC5004_8BIT, GC5004_REG_SENSOR_ID_LOW_BIT, &id_l)) {
		v4l2_err(client, "sensor_id = 0x%x\n", id_l);
		return -ENODEV;
	}
	*id = *id | (u16)id_l;

	if (*id == GC5004_ID)
		goto found;
	else {
		v4l2_err(client, "no gc5004 sensor found\n");
		return -ENODEV;
	}
found:
	v4l2_info(client, "gc5004_detect: sensor_id = 0x%x\n", *id);

	return 0;
}

/*
 * gc5004 stream on/off
 */
static int gc5004_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret;
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "@%s ++\n", __func__);
	mutex_lock(&dev->input_lock);
	if (enable) {
		ret = gc5004_set_streaming(sd);
		if (ret != 0) {
			v4l2_err(client, "write_reg_array err\n");
			mutex_unlock(&dev->input_lock);
			return ret;
		}
		dev->streaming = 1;
	} else {
		ret = gc5004_set_suspend(sd);
		dev->streaming = 0;
		dev->fps_index = 0;
	}
	mutex_unlock(&dev->input_lock);

	return 0;
}

/*
 * gc5004 enum frame size, frame intervals
 */
static int gc5004_enum_framesizes(struct v4l2_subdev *sd,
				   struct v4l2_frmsizeenum *fsize)
{
	unsigned int index = fsize->index;
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	if (index >= dev->entries_curr_table)//fix me,
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = dev->curr_res_table[index].width;
	fsize->discrete.height = dev->curr_res_table[index].height;
	fsize->reserved[0] = dev->curr_res_table[index].used;

	return 0;
}

static int gc5004_enum_frameintervals(struct v4l2_subdev *sd,
				       struct v4l2_frmivalenum *fival)
{
	unsigned int index = fival->index;
	int i;
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	/* since the isp will donwscale the resolution to the right size,
	  * find the nearest one that will allow the isp to do so
	  * important to ensure that the resolution requested is padded
	  * correctly by the requester, which is the atomisp driver in
	  * this case.
	  */
	i = nearest_resolution_index(sd, fival->width, fival->height);

	if (i == -1)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->width = dev->curr_res_table[i].width;
	fival->height = dev->curr_res_table[i].height;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = dev->curr_res_table[i].fps;

	return 0;
}

static int gc5004_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
				 enum v4l2_mbus_pixelcode *code)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	if (index >= MAX_FMTS)
		return -EINVAL;
	*code = dev->format.code;
	return 0;
}

static int gc5004_s_config(struct v4l2_subdev *sd,
			    int irq, void *pdata)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 sensor_id;
	int ret;

	if (NULL == pdata)
		return -ENODEV;

	dev->platform_data = 
		(struct camera_sensor_platform_data *)pdata;

	mutex_lock(&dev->input_lock);

	if (dev->platform_data->platform_init) {
		ret = dev->platform_data->platform_init(client);//fix me imx set voltage here
		if (ret) {
			mutex_unlock(&dev->input_lock);
			dev_err(&client->dev, "gc5004 platform init err\n");
			return ret;
		}
	}

	ret = __gc5004_s_power(sd, 1);
	if (ret) {
		v4l2_err(client, "gc5004 power-up err.\n");
		goto fail_csi_cfg;
	}
	
	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret)
		goto fail_csi_cfg;

	/* config & detect sensor */
	ret = gc5004_detect(client, &sensor_id);
	if (ret) {
		v4l2_err(client, "gc5004_detect err s_config.\n");
		goto fail_detect;
	}

	dev->sensor_id = sensor_id;

	/* Read sensor's OTP data 
	if (dev->sensor_id == GC5004_ID)
		dev->otp_data = devm_kzalloc(&client->dev,
				GC5004_OTP_DATA_SIZE, GFP_KERNEL);
	else
		dev->otp_data = gc5004_otp_read(sd);*/
	/* power off sensor */
	ret = __gc5004_s_power(sd, 0);
	mutex_unlock(&dev->input_lock);
	if (ret)
		v4l2_err(client, "gc5004 power-down err.\n");

	return ret;

fail_detect:
	dev->platform_data->csi_cfg(sd, 0);
fail_csi_cfg:
	__gc5004_s_power(sd, 0);
	mutex_unlock(&dev->input_lock);
	dev_err(&client->dev, "sensor power-gating failed\n");
	return ret;
}

static int
gc5004_enum_mbus_code(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		       struct v4l2_subdev_mbus_code_enum *code)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	if (code->index >= MAX_FMTS)
		return -EINVAL;
	code->code = dev->format.code;
	return 0;
}

static int
gc5004_enum_frame_size(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			struct v4l2_subdev_frame_size_enum *fse)
{
	int index = fse->index;
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	if (index >= dev->entries_curr_table)
		return -EINVAL;

	fse->min_width = dev->curr_res_table[index].width;
	fse->min_height = dev->curr_res_table[index].height;
	fse->max_width = dev->curr_res_table[index].width;
	fse->max_height = dev->curr_res_table[index].height;

	return 0;
}

static struct v4l2_mbus_framefmt *
__gc5004_get_pad_format(struct gc5004_device *sensor,
			 struct v4l2_subdev_fh *fh, unsigned int pad,
			 enum v4l2_subdev_format_whence which)
{
	switch (which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		return v4l2_subdev_get_try_format(fh, pad);
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		return &sensor->format;
	default:
		return NULL;
	}
}

static int
gc5004_get_pad_format(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		       struct v4l2_subdev_format *fmt)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	struct v4l2_mbus_framefmt *format =
			__gc5004_get_pad_format(dev, fh, fmt->pad, fmt->which);

	fmt->format = *format;

	return 0;
}

static int
gc5004_set_pad_format(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		       struct v4l2_subdev_format *fmt)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		dev->format = fmt->format;

	return 0;
}

static int
gc5004_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	dev->run_mode = param->parm.capture.capturemode;

	mutex_lock(&dev->input_lock);
	switch (dev->run_mode) {
	case CI_MODE_VIDEO:
		dev->curr_res_table = dev->mode_tables->res_video;
		dev->entries_curr_table = dev->mode_tables->n_res_video;
		break;
	case CI_MODE_STILL_CAPTURE:
		dev->curr_res_table = dev->mode_tables->res_still;
		dev->entries_curr_table = dev->mode_tables->n_res_still;
		break;
	default:
		dev->curr_res_table = dev->mode_tables->res_preview;
		dev->entries_curr_table = dev->mode_tables->n_res_preview;
	}
	mutex_unlock(&dev->input_lock);
	return 0;
}

int
gc5004_g_frame_interval(struct v4l2_subdev *sd,
				struct v4l2_subdev_frame_interval *interval)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 lines_per_frame;

	/*
	 * if no specific information to calculate the fps,
	 * just used the value in sensor settings
	 */
	if (!dev->pixels_per_line || !dev->lines_per_frame) {
		interval->interval.numerator = 1;
		interval->interval.denominator = dev->fps;
		return 0;
	}

	/*
	 * DS: if coarse_integration_time is set larger than
	 * lines_per_frame the frame_size will be expanded to
	 * coarse_integration_time+1
	 */
	if (dev->coarse_itg > dev->lines_per_frame) {
		if ((dev->coarse_itg + 4) < dev->coarse_itg) {
			/*
			 * we can not add 4 according to ds, as this will
			 * cause over flow
			 */
			v4l2_warn(client, "%s: abnormal coarse_itg:0x%x\n",
				  __func__, dev->coarse_itg);
			lines_per_frame = dev->coarse_itg;
		} else {
			lines_per_frame = dev->coarse_itg + 4;
		}
	} else {
		lines_per_frame = dev->lines_per_frame;
	}
	interval->interval.numerator = dev->pixels_per_line *
					lines_per_frame;
	interval->interval.denominator = dev->vt_pix_clk_freq_mhz;

	return 0;
}

static int gc5004_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	mutex_lock(&dev->input_lock);
	*frames = dev->curr_res_table[dev->fmt_idx].skip_frames;
	mutex_unlock(&dev->input_lock);

	return 0;
}

static const struct v4l2_subdev_sensor_ops gc5004_sensor_ops = {
	.g_skip_frames	= gc5004_g_skip_frames,
};

static const struct v4l2_subdev_video_ops gc5004_video_ops = {
	.s_stream = gc5004_s_stream,
	.enum_framesizes = gc5004_enum_framesizes,
	.enum_frameintervals = gc5004_enum_frameintervals,
//	.enum_mbus_fmt = gc5004_enum_mbus_fmt,  fix me, don't need it?
	.try_mbus_fmt = gc5004_try_mbus_fmt,
	.g_mbus_fmt = gc5004_g_mbus_fmt,
	.s_mbus_fmt = gc5004_s_mbus_fmt,
	.s_parm = gc5004_s_parm,
	.g_frame_interval = gc5004_g_frame_interval,
};

static const struct v4l2_subdev_core_ops gc5004_core_ops = {
	.g_chip_ident = gc5004_g_chip_ident,
	.queryctrl = v4l2_subdev_queryctrl,
	.g_ctrl = v4l2_subdev_g_ctrl,
	.s_ctrl = v4l2_subdev_s_ctrl,
	.s_power = gc5004_s_power,
	.ioctl = gc5004_ioctl,
//	.init = gc5004_init,
};

static const struct v4l2_subdev_pad_ops gc5004_pad_ops = {
	.enum_mbus_code = gc5004_enum_mbus_code,
	.enum_frame_size = gc5004_enum_frame_size,
	.get_fmt = gc5004_get_pad_format,
	.set_fmt = gc5004_set_pad_format,
};

static const struct v4l2_subdev_ops gc5004_ops = {
	.core = &gc5004_core_ops,
	.video = &gc5004_video_ops,
	.pad = &gc5004_pad_ops,
	.sensor = &gc5004_sensor_ops,
};
/*
static const struct media_entity_operations gc5004_entity_ops = {
	.link_setup = NULL,
};
*/
static int gc5004_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gc5004_device *dev = to_gc5004_sensor(sd);

	if (dev->platform_data->platform_deinit)
		dev->platform_data->platform_deinit();
	v4l2_ctrl_handler_free(&dev->ctrl_handler);
	v4l2_device_unregister_subdev(sd);

//	dev->platform_data->csi_cfg(sd, 0);
	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	kfree(dev);

	return 0;
}

static int __update_gc5004_device_settings(struct gc5004_device *dev, u16 sensor_id)
{
	switch (sensor_id) {
	case GC5004_ID:
		dev->mode_tables = &gc5004_sets[0];
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
 
static int __gc5004_init_ctrl_handler(struct gc5004_device *dev)
{
	struct v4l2_ctrl_handler *hdl;
	int i;

	hdl = &dev->ctrl_handler;

	v4l2_ctrl_handler_init(&dev->ctrl_handler, ARRAY_SIZE(gc5004_controls));

	for (i = 0; i < ARRAY_SIZE(gc5004_controls); i++)
		v4l2_ctrl_new_custom(&dev->ctrl_handler,
				&gc5004_controls[i], NULL);

	dev->ctrl_handler.lock = &dev->input_lock;
	dev->sd.ctrl_handler = hdl;
	v4l2_ctrl_handler_setup(&dev->ctrl_handler);

	return 0;
}

static int gc5004_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct gc5004_device *dev;
	struct camera_mipi_info *gc5004_info = NULL;
	int ret;

	/* allocate sensor device & init sub device */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		v4l2_err(client, "%s: out of memory\n", __func__);
		return -ENOMEM;
	}

	mutex_init(&dev->input_lock);

	dev->fmt_idx = 0;
	dev->sensor_id = GC5004_ID_DEFAULT;//fix me

	v4l2_i2c_subdev_init(&(dev->sd), client, &gc5004_ops);

	if (client->dev.platform_data) {
		ret = gc5004_s_config(&dev->sd, client->irq,
				       client->dev.platform_data);
		if (ret)
			goto out_free;
	}

	ret = __gc5004_init_ctrl_handler(dev);
	if (ret) {
		v4l2_ctrl_handler_free(&dev->ctrl_handler);
		v4l2_device_unregister_subdev(&dev->sd);
		kfree(dev);
		return ret;
	}

	dev->vcm_driver = &gc5004_vcms[0];
	dev->vcm_driver->init(&dev->sd);

	gc5004_info = v4l2_get_subdev_hostdata(&dev->sd);

	/*
	 * sd->name is updated with sensor driver name by the v4l2.
	 * change it to sensor name in this case.
	 */
	snprintf(dev->sd.name, sizeof(dev->sd.name), "%s%x %d-%04x",
		GC5004_SUBDEV_PREFIX, dev->sensor_id,
		i2c_adapter_id(client->adapter), client->addr);

	/* Resolution settings depend on sensor type and platform */
	ret = __update_gc5004_device_settings(dev, dev->sensor_id);
	if (ret)
		goto out_free;

	ret = __gc5004_init_ctrl_handler(dev);
	if (ret) {
		v4l2_ctrl_handler_free(&dev->ctrl_handler);
		v4l2_device_unregister_subdev(&dev->sd);
		kfree(dev);
		return ret;
	}

	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dev->pad.flags = MEDIA_PAD_FL_SOURCE;
	dev->format.code = V4L2_MBUS_FMT_SGRBG10_1X10;

//	dev->sd.entity.ops = &gc5004_entity_ops;
	dev->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;

	ret = media_entity_init(&dev->sd.entity, 1, &dev->pad, 0);
	if (ret)
		gc5004_remove(client);

	return ret;
out_free:
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);
	return ret;
}

static const struct i2c_device_id gc5004_ids[] = {
	{GC5004_NAME, GC5004_ID},
	{}
};

MODULE_DEVICE_TABLE(i2c, gc5004_ids);

static struct i2c_driver gc5004_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = GC5004_DRIVER,
	},
	.probe = gc5004_probe,
	.remove = gc5004_remove,//__devexit_p(gc5004_remove),
	.id_table = gc5004_ids,
};


static __init int init_gc5004(void)
{
	return i2c_add_driver(&gc5004_driver);
}

static __exit void exit_gc5004(void)
{
	i2c_del_driver(&gc5004_driver);
}

module_init(init_gc5004);
module_exit(exit_gc5004);

MODULE_DESCRIPTION("A low-level driver for  sensors");
MODULE_AUTHOR("Kun Jiang <kunx.jiang@intel.com>");
MODULE_LICENSE("GPL");

