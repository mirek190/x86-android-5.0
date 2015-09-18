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

#include "ad5823.h"

struct ad5823_device ad5823_dev;

static int ad5823_i2c_rd8(struct i2c_client *client, u8 reg, u8 *val)
{
	struct i2c_msg msg[2];
	u8 buf[2];
	buf[0] = reg;
	buf[1] = 0;

	msg[0].addr = AD5823_VCM_ADDR;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &buf[0];

	msg[1].addr = AD5823_VCM_ADDR;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = &buf[1];
	*val = 0;
	if (i2c_transfer(client->adapter, msg, 2) != 2)
		return -EIO;
	*val = buf[1];
	return 0;
}

static int ad5823_i2c_wr8(struct i2c_client *client, u8 reg, u8 val)
{
	struct i2c_msg msg;
	u8 buf[2];
	buf[0] = reg;
	buf[1] = val;
	msg.addr = AD5823_VCM_ADDR;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = &buf[0];
	if (i2c_transfer(client->adapter, &msg, 1) != 1)
		return -EIO;
	return 0;
}

static int ad5823_i2c_rd16(struct i2c_client *client, u8 reg, u16 *val)
{
	struct i2c_msg msg[2];
	u8 buf[6];
	int err;

	memset(msg, 0 , sizeof(msg));

	msg[0].addr = AD5823_VCM_ADDR;
	msg[0].flags = 0;
	msg[0].len = I2C_MSG_LENGTH;
	msg[0].buf = buf;

	/* high byte goes out first */
	buf[0] = (u8)(reg >> 8);
	buf[1] = (u8)(reg & 0xff);
	
	msg[1].addr = AD5823_VCM_ADDR;
	msg[1].len = AD5823_16BIT;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = buf;

	err = i2c_transfer(client->adapter, msg, 2);
	if (err != 2) {
		if (err >= 0)
			err = -EIO;
		dev_err(&client->dev,
			"read from offset 0x%x error %d", reg, err);
		return err;
	}

	*val = be16_to_cpu(*(u16 *)&buf[0]);

	return 0;
}


static int ad5823_i2c_wr16(struct i2c_client *client, u8 reg, u16 val)
{
	struct i2c_msg msg;
	u8 buf[3];
	buf[0] = reg;
	buf[1] = (u8)(val >> 8);
	buf[2] = (u8)(val & 0xff);
	msg.addr = AD5823_VCM_ADDR;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = &buf[0];
	if (i2c_transfer(client->adapter, &msg, 1) != 1)
		return -EIO;
	return 0;
}

static int ad5823_set_arc_mode(struct i2c_client *client)
{
	int ret;

	ret = ad5823_i2c_wr8(client, AD5823_MODE, AD5823_ARC_EN);
	if (ret)
		return ret;

	ret = ad5823_i2c_wr8(client, AD5823_MODE,
				AD5823_MODE_1M_SWITCH_CLOCK);
	if (ret)
		return ret;

	ret = ad5823_i2c_wr8(client, AD5823_VCM_MOVE_TIME, AD5823_DEF_FREQ);
	return ret;
}

int ad5823_vcm_power_up(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	u8 ad5823_id;

	/* Enable power */
	ret = ad5823_dev.platform_data->power_ctrl(sd, 1);
	if (ret)
		return ret;
	/* waiting time AD5823(vcm) - t1 + t2
	  * t1(1ms) -Time from VDD high to first i2c cmd
	  * t2(100us) - exit power-down mode time
	  */
	  
	usleep_range(1100, 1100);

#if 0	
	/* Detect device */
	ret = ad5823_i2c_rd8(client, AD5823_IC_INFO, &ad5823_id);
	if (ret < 0)
		goto fail_powerdown;
	if (ad5823_id != AD5823_ID) {
		ret = -ENXIO;
		goto fail_powerdown;
	}
	ret = ad5823_set_arc_mode(client);
	if (ret)
		return ret;

	/* set the VCM_THRESHOLD */
	ret = ad5823_i2c_wr8(client, AD5823_VCM_THRESHOLD,
		AD5823_DEF_THRESHOLD);
#endif
	return ret;

fail_powerdown:
	ad5823_dev.platform_data->power_ctrl(sd, 0);
	return ret;
}

int ad5823_vcm_power_down(struct v4l2_subdev *sd)
{
	return ad5823_dev.platform_data->power_ctrl(sd, 0);
}


int ad5823_t_focus_vcm(struct v4l2_subdev *sd, u16 val)
{
#if 0
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 data = val & VCM_CODE_MASK;

	return ad5823_i2c_wr8(client, AD5823_VCM_CODE_MSB, data);
#else
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 msb, lsb;
	s32 value;

    //val = clamp(val, 0, AD5823_MAX_FOCUS_POS);

    msb = (val >> 8 & 0xff);
    lsb = (val & 0xff);
    ad5823_i2c_wr8(client, AD5823_VCM_CODE_MSB, msb);
    ad5823_i2c_wr8(client, AD5823_VCM_CODE_LSB, lsb);

	return 0;
#endif
}

int ad5823_t_focus_abs(struct v4l2_subdev *sd, s32 value)
{
	int ret = 0;

#if 1 //set position as MAX-value to match HAL
	value = min(value, AD5823_MAX_FOCUS_POS);
	ret = ad5823_t_focus_vcm(sd, AD5823_MAX_FOCUS_POS - value);
	//ret = ad5823_t_focus_vcm(sd, value);
	if (ret == 0) {
		ad5823_dev.number_of_steps = value - ad5823_dev.focus;
		ad5823_dev.focus = value;
		getnstimeofday(&(ad5823_dev.timestamp_t_focus_abs));
	}
#else
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 msb, lsb;

        value = clamp(value, 0, AD5823_MAX_FOCUS_POS);
        msb = (value >> 8 & 0xff);
        lsb = (value & 0xff);

        ad5823_i2c_wr8(client, AD5823_VCM_CODE_MSB, msb);
        ad5823_i2c_wr8(client, AD5823_VCM_CODE_LSB, lsb);
#endif

	return ret;
}

int ad5823_t_focus_rel(struct v4l2_subdev *sd, s32 value)
{

	return ad5823_t_focus_abs(sd, ad5823_dev.focus + value);
}

int ad5823_q_focus_status(struct v4l2_subdev *sd, s32 *value)
{
	u32 status = 0;
	struct timespec temptime;
	const struct timespec timedelay = {
		0,
		min_t(u32, abs(ad5823_dev.number_of_steps) * DELAY_PER_STEP_NS,
			DELAY_MAX_PER_STEP_NS),
	};

	ktime_get_ts(&temptime);

	temptime = timespec_sub(temptime, (ad5823_dev.timestamp_t_focus_abs));

	if (timespec_compare(&temptime, &timedelay) <= 0) {
		status |= ATOMISP_FOCUS_STATUS_MOVING;
		status |= ATOMISP_FOCUS_HP_IN_PROGRESS;
	} else {
		status |= ATOMISP_FOCUS_STATUS_ACCEPTS_NEW_MOVE;
		status |= ATOMISP_FOCUS_HP_COMPLETE;
	}
	*value = status;

	return 0;
}

int ad5823_q_focus_abs(struct v4l2_subdev *sd, s32 *value)
{
	s32 val;

	ad5823_q_focus_status(sd, &val);

	if (val & ATOMISP_FOCUS_STATUS_MOVING)
		*value  = ad5823_dev.focus - ad5823_dev.number_of_steps;
	else
		*value  = ad5823_dev.focus ;

	return 0;
}

int ad5823_t_vcm_slew(struct v4l2_subdev *sd, s32 value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err;
	u16 read_data;

	if (value > AD5823_VCM_SLEW_STEP_MAX)
		return -EINVAL;

	err = ad5823_i2c_rd16(client,AD5823_VCM_SLEW_STEP,&read_data);
	if (err) {
		v4l2_err(client, "%s error exit, read failed\n", __func__);
		return err;
	}

	read_data &= ~AD5823_VCM_SLEW_STEP_MASK;
	value <<= ffs(AD5823_VCM_SLEW_STEP_MASK) -1;
	read_data |= value & AD5823_VCM_SLEW_STEP_MASK;
	err = ad5823_i2c_wr16(client, AD5823_VCM_SLEW_STEP, read_data);
	if (err)
		v4l2_err(client, "%s error exit, write failed\n", __func__);

	return err;
}

int ad5823_t_vcm_timing(struct v4l2_subdev *sd, s32 value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	/* Max 16 bits */
	if (value > AD5823_VCM_SLEW_TIME_MAX)
		return -EINVAL;

	return ad5823_i2c_wr16(client,AD5823_VCM_SLEW_TIME,value);

}

int ad5823_vcm_init(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	ad5823_i2c_wr8(client, AD5823_RESET, 0x01);

	ad5823_dev.platform_data = camera_get_af_platform_data();

	return (NULL == ad5823_dev.platform_data) ? -ENODEV : 0;

}



