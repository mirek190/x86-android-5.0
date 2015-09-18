/*
 * Support for OminVision Camera Sensor.
 *
 * Copyright (c) 2014 Intel Corporation. All Rights Reserved.
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
#include "ov.h"
#ifdef CONFIG_VIDEO_OV2685
#include "ov_2685.h"
#include "ov_2685f.h"
#endif

static const struct i2c_device_id ov_ids[] = {
#ifdef CONFIG_VIDEO_OV2685
	{"ov2685", (kernel_ulong_t)&ov2685_product_info},
	{"ov2685f", (kernel_ulong_t)&ov2685f_product_info},
#endif
	{},
};

#define to_ov_sensor(sd) container_of(sd, struct ov_device, sd)
static int
__ov_read_reg(struct i2c_client *client,
			u16 data_length,
			u16 reg,
			u32 *val)
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
	msg[0].len = I2C_MSG_LEN_OFFSET;
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
__ov_write_reg(struct i2c_client *client,
		u16 data_length,
		u16 reg,
		u32 val)
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
 * ov_write_reg_array - Initializes a list
 * @client: i2c driver client structure
 * @reglist: list of registers to be written
 *
 * Initializes a list of MT9T111 registers. The list of registers is
 * terminated by MISENSOR_TOK_TERM.
 */
static int
__ov_write_reg_array(struct i2c_client *client,
				struct misensor_reg *reglist)
{
	const struct misensor_reg *next = reglist;
	int err;
	u32 tmp_val;
	u32 targetVal;
	u16 bitLen = MISENSOR_8BIT;

	for (; next->length != MISENSOR_TOK_TERM; next++) {
		if (next->length == MISENSOR_TOK_DELAY) {
			msleep(next->val);
		} else {
            targetVal = next->val;
            bitLen = (next->length & MISENSOR_8BIT) ? (MISENSOR_8BIT) :
                                                    ((next->length & MISENSOR_16BIT) ? MISENSOR_16BIT : MISENSOR_32BIT);

            if (next->length & MISENSOR_RMW_AND) {
			    err = __ov_read_reg(client, bitLen, next->reg, &tmp_val);
                if (err) {
                    dev_err(&client->dev, "%s err. read %0x aborted\n", __func__, next->reg);
                    continue;
                }

                targetVal = tmp_val & next->val;
            }

            if (next->length & MISENSOR_RMW_OR) {
			    err = __ov_read_reg(client, bitLen, next->reg, &tmp_val);
                if (err) {
                    dev_err(&client->dev, "%s err. read %0x aborted\n", __func__, next->reg);
                    continue;
                }

                targetVal = tmp_val | next->val;
            }

            err = __ov_write_reg(client, bitLen, next->reg, targetVal);
            if (err) {
                dev_err(&client->dev, "%s err. write %0x aborted\n", __func__, next->reg);
                continue;
            }

            udelay(10);
		}
	}

	return 0;
}

static int
__ov_program_reglist(struct v4l2_subdev *sd,
		enum settings_type setting_id,
		s32 value)
{
	struct ov_device *dev = to_ov_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov_table_info *table_info = NULL;
	struct ov_product_info *pinfo = dev->product_info;
	int ret;
	int i;

	/* find out which setting type it is: awb/exposure/scene mode/color effect/...*/
	for (i = 0; i < OV_NUM_SETTINGS; i++) {
		if (pinfo->settings_tbl[i].type_id == setting_id) {
			table_info = &pinfo->settings_tbl[i];
			break;
		}
	}
	if (!table_info) {
		pr_err("%s: failed to find the reg table!\n", __func__);
		return -EINVAL;
	}

	/* find out which value is set in this type */
	for (i = 0; i < table_info->num_tables; i++) {
		if (table_info->reglist[i].id == value) {
			ret = __ov_write_reg_array(client,
					table_info->reglist[i].reg_list);
			break;
		}
	}
	if (i >= table_info->num_tables) {
		pr_err("%s: failed to find the reg list!\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int __ov_init(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov_device *dev = to_ov_sensor(sd);
	int ret = 0;

	if (dev->product_info->ov_init) {
		ret = __ov_write_reg_array(client,
			dev->product_info->ov_init->regs);
	}
	if (ret)
		return ret;

	/* delay 5ms to wait for sensor initialization finish.*/
	usleep_range(5000, 6000);
	return 0;
}

static int __ov_init_devid(struct v4l2_subdev *sd)
{
	struct ov_device *dev = to_ov_sensor(sd);

	dev->id_exposure = 0;
	dev->id_awb = V4L2_WHITE_BALANCE_AUTO;
	dev->id_scene = V4L2_SCENE_MODE_NONE;
	dev->id_coloreffect = V4L2_COLORFX_NONE;

	return 0;
}

static int __ov_power_up(struct v4l2_subdev *sd)
{
	struct ov_device *dev = to_ov_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

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

	__ov_init_devid(sd);

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

static int __ov_power_down(struct v4l2_subdev *sd)
{
	struct ov_device *dev = to_ov_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;
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

/* Horizontal flip the image. */
static int ov_g_hflip(struct v4l2_subdev *sd, s32 * val)
{
	struct ov_device *dev = to_ov_sensor(sd);
	*val = dev->id_hflip;

	return 0;
}

static int ov_g_vflip(struct v4l2_subdev *sd, s32 * val)
{
	struct ov_device *dev = to_ov_sensor(sd);
	*val = dev->id_vflip;

	return 0;
}

/* Horizontal flip the image. */
static int ov_s_hflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct ov_device *dev = to_ov_sensor(sd);

	ret = __ov_program_reglist(sd, OV_SETTING_HFLIP, value);
	if (!ret)
		dev->id_hflip = value;
	return ret;
}

/* Vertically flip the image */
static int ov_s_vflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct ov_device *dev = to_ov_sensor(sd);

	ret = __ov_program_reglist(sd, OV_SETTING_VFLIP, value);
	if (!ret)
		dev->id_vflip = value;
	return ret;
}

static int ov_s_freq(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct ov_device *dev = to_ov_sensor(sd);

	ret = __ov_program_reglist(sd, OV_SETTING_FREQ, value);
	if (!ret)
		dev->id_freq = value;
	return ret;
}

static int ov_g_scene(struct v4l2_subdev *sd, s32 *value)
{
	struct ov_device *dev = to_ov_sensor(sd);
	*value = dev->id_scene;

	return 0;
}

static int ov_s_scene(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct ov_device *dev = to_ov_sensor(sd);
	ret = __ov_program_reglist(sd, OV_SETTING_SCENE_MODE, value);
	if (!ret)
		dev->id_scene = value;
	return ret;
}

int ov_g_focal_absolute(struct v4l2_subdev *sd, s32 *val)
{
	struct ov_device *dev = to_ov_sensor(sd);

	*val = dev->product_info->focal;

	return 0;
}

int ov_g_fnumber(struct v4l2_subdev *sd, s32 *val)
{
	struct ov_device *dev = to_ov_sensor(sd);

	*val = dev->product_info->f_number;
	return 0;
}

int ov_g_fnumber_range(struct v4l2_subdev *sd, s32 *val)
{
	struct ov_device *dev = to_ov_sensor(sd);

	*val = dev->product_info->f_number_range;
	return 0;
}

static int ov_g_wb(struct v4l2_subdev *sd, s32 *value)
{
	struct ov_device *dev = to_ov_sensor(sd);
	*value = dev->id_awb;

	return 0;
}

static int ov_s_wb(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct ov_device *dev = to_ov_sensor(sd);

	ret = __ov_program_reglist(sd, OV_SETTING_AWB_MODE, value);
	if (!ret)
		dev->id_awb = value;
	return ret;
}

static int ov_g_exposure(struct v4l2_subdev *sd, s32 *value)
{
	struct ov_device *dev = to_ov_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 coarse;
	u32 reg_val_h, reg_val_m, reg_val_l;
	int ret;

	ret = __ov_read_reg(client, MISENSOR_8BIT, dev->product_info->reg_expo_coarse, &reg_val_h);
	if (ret)
		return ret;
	coarse = ((u16)(reg_val_h & 0x0000000f)) << 12;

	ret =__ov_read_reg(client, MISENSOR_8BIT, dev->product_info->reg_expo_coarse+1, &reg_val_m);
	if (ret)
		return ret;
	coarse |= ((u16)(reg_val_m & 0x000000ff)) << 4;

	ret = __ov_read_reg(client, MISENSOR_8BIT, dev->product_info->reg_expo_coarse+2, &reg_val_l);
	if (ret)
		return ret;
	coarse |= ((u16)(reg_val_l & 0x000000ff)) >> 4;
	/*WA: exposure time of max coarse lines is 1/fps,
	 *so curent exposure time = coarse / fps / max_exposure_lines.
	 *Additional, 10000 is as denominater in hal. So multiply 10000 here.
	 */
	if (dev->cur_fps * dev->max_exposure_lines)
		coarse = coarse * 10000 / (dev->cur_fps * dev->max_exposure_lines);
	*value = coarse;
	return 0;
}

static int ov_s_exposure(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct ov_device *dev = to_ov_sensor(sd);

	ret = __ov_program_reglist(sd, OV_SETTING_EXPOSURE, value);
	if (!ret)
		dev->id_exposure = value;
	return ret;
}

/* NOTE:
 * v4l2_ctrl_handler_setup must be called before this function
 */
static void reset_v4l2_ctrl_value(struct v4l2_ctrl_handler *hdl)
{
	struct v4l2_ctrl *ctrl;

	if (hdl == NULL)
		return;

	mutex_lock(hdl->lock);

	list_for_each_entry(ctrl, &hdl->ctrls, node)
		ctrl->done = false;

	list_for_each_entry(ctrl, &hdl->ctrls, node) {
		struct v4l2_ctrl *master = ctrl->cluster[0];
		int i;

		/* Skip if this control was already handled by a cluster. */
		/* Skip button controls and read-only controls. */
		if (ctrl->done || ctrl->type == V4L2_CTRL_TYPE_BUTTON ||
		    (ctrl->flags & V4L2_CTRL_FLAG_READ_ONLY))
			continue;

		for (i = 0; i < master->ncontrols; i++) {
			if (master->cluster[i]) {
				master->cluster[i]->is_new = 1;
				master->cluster[i]->done = true;
			}
		}
		master->cur.val = master->val = master->default_value;
	}

	mutex_unlock(hdl->lock);

}

static int ov_s_power(struct v4l2_subdev *sd, int power)
{
	int ret;
	struct ov_device *dev = to_ov_sensor(sd);

	mutex_lock(&dev->input_lock);
	if (power == 0) {
		ret = __ov_power_down(sd);
		mutex_unlock(&dev->input_lock);
		/* as v4l2 framework would cache the value set from upper layer
		* even though exit camera,it would not call ctrl function because
                * that the cache value is equal with the value set by upper layer
                * when entry camera again.
                * So, we will reset the v4l2 ctrl value to be default after power off.
		*/
		reset_v4l2_ctrl_value(&dev->ctrl_handler);
		return ret;
	}

	ret = __ov_power_up(sd);
	if (ret != 0) {
		mutex_unlock(&dev->input_lock);
		return ret;
	}

	ret = __ov_init(sd);
	mutex_unlock(&dev->input_lock);
	return ret;
}

static long ov_ioctl(struct v4l2_subdev *sd,
		unsigned int cmd, void *arg)
{
	/* TODO: add support for debug register read/write ioctls */
	return 0;
}

static struct ov_res_struct*
to_res(struct ov_device *dev, u32 w, u32 h)
{
	int  index;
	struct ov_res_struct *res = dev->product_info->ov_res;

	for (index = 0; index < dev->product_info->num_res; index++) {
		if (res[index].width == w &&
		    res[index].height == h)
			break;
	}

	/* No match found */
	if (index >= dev->product_info->num_res) {
		pr_err("%s: fail to find match resolution!\n", __func__);
		return NULL;
	}
	return &res[index];
}

static struct ov_mbus_fmt *
to_fmt(struct ov_device *dev, enum v4l2_mbus_pixelcode code)
{
	int  index;
	struct ov_mbus_fmt *fmt = dev->product_info->ov_fmt;

	for (index = 0; index < dev->product_info->num_fmt; index++) {
		if (code == fmt[index].code)
			break;
	}

	/* No match found */
	if (index >= dev->product_info->num_fmt) {
		pr_err("%s: fail to find match format!\n", __func__);
		return NULL;
	}
	return &fmt[index];
}

static int
to_size(struct ov_device *dev, int *h_size, int *v_size)
{
	int i;
	unsigned int hsize = 0;
	unsigned int vsize = 0;
	struct ov_res_struct* res = dev->product_info->ov_res;

	for (i = 0; i < dev->product_info->num_res; i++) {
		if (dev->cur_res == res[i].res_id) {
			hsize = res[i].width;
			vsize = res[i].height;
			break;
		}
	}
	if (hsize == 0 || vsize == 0) {
		pr_err("%s: failed to find the match width and height!\n",
			__func__);
		return -EINVAL;
	}

	if (h_size != NULL)
		*h_size = hsize;
	if (v_size != NULL)
		*v_size = vsize;
	return 0;
}

#define MAX_RES_NUM 20
static int __try_res(struct ov_device *dev,
			u32 src_w, u32 src_h,
			u32 *dst_w, u32 *dst_h)
{
	unsigned int ratio_required, ratio_mapped;
	enum res_type res[MAX_RES_NUM];
	enum res_type res_idx = 0;
	int i, j = 0;
	struct ov_res_struct* ov_res = dev->product_info->ov_res;

	BUG_ON((NULL == dst_w) || (NULL == dst_h));

	/* aspect ratio calculate, could be optimized further... */
	ratio_required = src_w * 1000 / src_h;

	if (ratio_required < RATIO_3_TO_2) {
		ratio_mapped = RATIO_4_3;
	}
	else if (ratio_required < RATIO_16_TO_9) {
		ratio_mapped = RATIO_3_2;
	}
	else {
		ratio_mapped = RATIO_16_9;
	}

	/* find out the max res which fit the aspect ratio requirement */
	for (i = 0; i < dev->product_info->num_res; i++) {
		if (ov_res[i].aspt_ratio & ratio_mapped) {
			res[j] = ov_res[i].res_id;
			j++;
		}
	}

	for (i = j-1; i >= 0; i--) {
		if (res_idx < res[i])
			res_idx = res[i];
	}

	for (i = 0; i < dev->product_info->num_res; i++) {
		if (res_idx == ov_res[i].res_id) {
			break;
		}
	}
	if (i >= dev->product_info->num_res)
		BUG_ON(1);

	*dst_w = ov_res[i].width;
	*dst_h = ov_res[i].height;
	return 0;
}

static int ov_try_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct ov_device *dev = to_ov_sensor(sd);
	u32 width, height;

	if (dev->product_info->num_res >= MAX_RES_NUM)
		WARN(1, "%s: exceed the res array size!\n", __func__);

	pr_info("%s: sensor is asked for w=%d h=%d\n",
			__func__, fmt->width, fmt->height);
	mutex_lock(&dev->input_lock);

	__try_res(dev, fmt->width, fmt->height, &width, &height);

	fmt->width = width;
	fmt->height = height;
	/* FixME: hard code as UYVY */
	fmt->code = V4L2_MBUS_FMT_UYVY8_1X16;

	mutex_unlock(&dev->input_lock);
	pr_info("%s: return w%d, h%d, fmt 0x%x to user\n",
		__func__, fmt->width, fmt->height, fmt->code);
	return 0;
}

static int ov_g_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct ov_device *dev = to_ov_sensor(sd);
	int width = 0;
	int height = 0;
	int ret;

	/* FixME: hard code as UYVY */
	fmt->code = V4L2_MBUS_FMT_UYVY8_1X16;
	ret = to_size(dev, &width, &height);
	if (ret)
		return ret;
	fmt->width = width;
	fmt->height = height;

	return 0;
}

static int __ov_reset_control(struct v4l2_subdev *sd)
{
	struct ov_device *dev = to_ov_sensor(sd);
	int ret = 0;

	ret = ov_s_exposure(sd, dev->id_exposure);
	ret |= ov_s_wb(sd, dev->id_awb);
	ret |= ov_s_scene(sd, dev->id_scene);
	
	return ret;
}


static int ov_s_mbus_fmt(struct v4l2_subdev *sd,
			      struct v4l2_mbus_framefmt *fmt)
{
	int index;
	struct i2c_client *c = v4l2_get_subdevdata(sd);
	struct ov_device *dev = to_ov_sensor(sd);
	struct ov_res_struct *ov_res;
	struct camera_mipi_info *mipi_info = NULL;
	u32 width = fmt->width;
	u32 height = fmt->height;

	mutex_lock(&dev->input_lock);

	mipi_info = v4l2_get_subdev_hostdata(sd);
	if (mipi_info == NULL) {
		dev_err(&c->dev, "%s: can not find mipi info!!!\n", __func__);
		mutex_unlock(&dev->input_lock);
		return -EINVAL;
	}

	__try_res(dev, fmt->width, fmt->height, &width, &height);

	ov_res = to_res(dev, width, height);
	if (unlikely(!ov_res)) {
		WARN_ON(1);
		mutex_unlock(&dev->input_lock);
		return -EINVAL;
	}

	pr_info("%s: set ov: res id %d, w%d, h%d\n",
		__func__, ov_res->res_id, ov_res->width, ov_res->height);

	mipi_info->num_lanes = ov_res->csi_lanes;
	__ov_write_reg_array(c, ov_res->regs);

	if (dev->cur_res != ov_res->res_id) {

		for (index = 0; index < dev->product_info->num_res; index++)
			dev->product_info->ov_res[index].used = 0;
		ov_res->used = 1;
	}

	dev->cur_fps = ov_res->fps;
	dev->cur_res = ov_res->res_id;
	dev->max_exposure_lines = ov_res->max_exposure_lines;
	fmt->width = width;
	fmt->height = height;
	__ov_reset_control(sd);
	mutex_unlock(&dev->input_lock);
	return 0;
}

static struct ov_control ov_controls[] = {
	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_EXPOSURE_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "exposure",
			.min = -2,//0  lkl
			.max = 2,//4  lkl
			.step = 1,
			.def = 0,
		},
		.g_ctrl = ov_g_exposure,
		.s_ctrl = ov_s_exposure,
	}, {
		.config = {
			.ops = NULL,
			.id = V4L2_CID_EXPOSURE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "exposure",
			.min = -2,//0  lkl
			.max = 2,//4  lkl
			.step = 1,
			.def = 0,
		},
		.g_ctrl = ov_g_exposure,
		.s_ctrl = ov_s_exposure,
	}, {
		.config = {
			.ops = NULL,
			.id = V4L2_CID_SCENE_MODE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Scene Mode",
			.min = V4L2_SCENE_MODE_NONE,
			.max = V4L2_SCENE_MODE_TEXT,
			.step = 1,
			.def = V4L2_SCENE_MODE_NONE,
		},
		.g_ctrl = ov_g_scene,
		.s_ctrl = ov_s_scene,
	}, {
		.config = {
			.ops = NULL,
			.id = V4L2_CID_FOCAL_ABSOLUTE,
			.name = "Focal length",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = OV_FOCAL_LENGTH_DEFAULT,
			.max = OV_FOCAL_LENGTH_DEFAULT,
			.step = 1,
			.def = OV_FOCAL_LENGTH_DEFAULT,
		},
		.s_ctrl = NULL,
		.g_ctrl = ov_g_focal_absolute,
	}, {
		.config = {
			.ops = NULL,
			.id = V4L2_CID_FNUMBER_ABSOLUTE,
			.name = "F-number",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 1,
			.max = OV_F_NUMBER_DEFAULT,
			.step = 1,
			.def = 1,
		},
		.s_ctrl = NULL,
		.g_ctrl = ov_g_fnumber,
	}, {
		.config = {
			.ops = NULL,
			.id = V4L2_CID_FNUMBER_RANGE,
			.name = "F-number range",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 1,
			.max = OV_F_NUMBER_RANGE,
			.step = 1,
			.def = 1,
		},
		.s_ctrl = NULL,
		.g_ctrl = ov_g_fnumber_range,
	}, {
		.config = {
			.ops = NULL,
			.id = V4L2_CID_POWER_LINE_FREQUENCY,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Light frequency filter",
			.min = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED,
			.max =	V4L2_CID_POWER_LINE_FREQUENCY_AUTO, /* 1: 50Hz, 2:60Hz */
			.step = 1,
			.def = V4L2_CID_POWER_LINE_FREQUENCY_AUTO,
		},
		.s_ctrl = ov_s_freq,
	}, {
		.config = {
			.ops = NULL,
			.id = V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "White Balance",
			.min = V4L2_WHITE_BALANCE_MANUAL,
			.max = V4L2_WHITE_BALANCE_SHADE,
			.step = 1,
			.def = V4L2_WHITE_BALANCE_AUTO,
		},
		.g_ctrl = ov_g_wb,
		.s_ctrl = ov_s_wb,
	}, {
		.config = {
			.ops = NULL,
			.id = V4L2_CID_VFLIP,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Image v-Flip",
			.min = 0,
			.max = 1,
			.step = 1,
			.def = 0,
		},
		.g_ctrl = ov_g_vflip,
		.s_ctrl = ov_s_vflip,
	}, {
		.config = {
			.ops = NULL,
			.id = V4L2_CID_HFLIP,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Image h-Flip",
			.min = 0,
			.max = 1,
			.step = 1,
			.def = 0,
		},
		.g_ctrl = ov_g_hflip,
		.s_ctrl = ov_s_hflip,
	},
};

static int ov_get_ctrl(struct v4l2_ctrl *ctrl)
{
	int i;
	int ret = 0;
	struct ov_device *dev = container_of(ctrl->handler,
							struct ov_device, ctrl_handler);
	mutex_lock(&dev->input_lock);

	for (i = 0; i < dev->num_ctrls; i++) {
		if (dev->ctrl_config[i].config.id == ctrl->id) {
			if (dev->ctrl_config[i].g_ctrl) {
				ret = dev->ctrl_config[i].g_ctrl(&dev->sd, &ctrl->val);
				mutex_unlock(&dev->input_lock);
				return ret;
			} else {
				mutex_unlock(&dev->input_lock);
				return 0;
			}
		}
	}

	if (i == dev->num_ctrls)
		pr_err("%s: not support ctrl\n", __func__);

	mutex_unlock(&dev->input_lock);
	return -EINVAL;
}

static int ov_set_ctrl(struct v4l2_ctrl *ctrl)
{
	int i;
	int ret = 0;
	struct ov_device *dev = container_of(ctrl->handler,
							struct ov_device, ctrl_handler);

	mutex_lock(&dev->input_lock);

	for (i = 0; i < dev->num_ctrls; i++) {
		if (dev->ctrl_config[i].config.id == ctrl->id) {

			if (dev->ctrl_config[i].s_ctrl) {
				ret = dev->ctrl_config[i].s_ctrl(&dev->sd, ctrl->val);
				mutex_unlock(&dev->input_lock);
				return ret;
			} else {
				mutex_unlock(&dev->input_lock);
				return 0;
			}
		}
	}
	if (i == dev->num_ctrls)
		pr_err("%s: not support ctrl\n", __func__);

	mutex_unlock(&dev->input_lock);
	return -EINVAL;
}

static int ov_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret;
	struct ov_device *dev = to_ov_sensor(sd);

	mutex_lock(&dev->input_lock);
	ret = __ov_program_reglist(sd, OV_SETTING_STREAM, enable);
	mutex_unlock(&dev->input_lock);
	return ret;
}

static int
ov_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
	unsigned int index = fsize->index;
	struct ov_device* dev = to_ov_sensor(sd);
	struct ov_res_struct *ov_res = dev->product_info->ov_res;

	if (index >= dev->product_info->num_res) {
		pr_err("%s: the idx has exceeded res array!\n", __func__);
		return -EINVAL;
	}

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = ov_res[index].width;
	fsize->discrete.height = ov_res[index].height;
	fsize->reserved[0] = ov_res[index].used;
	return 0;
}

static int ov_enum_frameintervals(struct v4l2_subdev *sd,
					struct v4l2_frmivalenum *fival)
{
	unsigned int index = fival->index;
	int i;
	struct ov_device* dev = to_ov_sensor(sd);
	struct ov_res_struct *ov_res = dev->product_info->ov_res;

	if (index >= dev->product_info->num_res) {
		pr_err("%s: the idx has exceeded res array!\n", __func__);
		return -EINVAL;
	}

	/* find out the first equal or bigger size */
	for (i = 0; i < dev->product_info->num_res; i++) {
		if (ov_res[i].width >= fival->width &&
		    ov_res[i].height >= fival->height)
			break;
	}

	if (index >= dev->product_info->num_res) {
		pr_err("%s: failed to find the matched res?!\n", __func__);
		i -= 1;
	}
	index = i;
	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = ov_res[index].fps;

	return 0;
}

static int
ov_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_OV, 0);
}

static int ov_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_fh *fh,
				  struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index) {
		pr_err("%s: unsupport code idx!\n", __func__);
		return -EINVAL;
	}
	code->code = V4L2_MBUS_FMT_UYVY8_1X16; /* FixME: hard code as UYVY */

	return 0;
}

static int ov_enum_frame_size(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh,
	struct v4l2_subdev_frame_size_enum *fse)
{
	unsigned int index = fse->index;
	struct ov_device* dev = to_ov_sensor(sd);
	struct ov_res_struct *ov_res = dev->product_info->ov_res;

	if (index >= dev->product_info->num_res) {
		pr_err("%s: the idx has exceeded res array!\n", __func__);
		return -EINVAL;
	}

	fse->min_width = ov_res[index].width;
	fse->min_height = ov_res[index].height;
	fse->max_width = ov_res[index].width;
	fse->max_height = ov_res[index].height;
	return 0;
}

static int
ov_get_pad_format(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		       struct v4l2_subdev_format *fmt)
{
	struct ov_device *snr = to_ov_sensor(sd);

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
ov_set_pad_format(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		       struct v4l2_subdev_format *fmt)
{
	struct ov_device *snr = to_ov_sensor(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		snr->format = fmt->format;

	return 0;
}

/* set focus zone */
static int
ov_set_selection(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			     struct v4l2_subdev_selection *sel)
{
	return 0;
}

static int ov_g_parm(struct v4l2_subdev *sd,
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

static int ov_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct ov_device *dev = to_ov_sensor(sd);

	mutex_lock(&dev->input_lock);
	dev->run_mode = param->parm.capture.capturemode;
	mutex_unlock(&dev->input_lock);
	return 0;
}

static int
ov_g_frame_interval(struct v4l2_subdev *sd,
				struct v4l2_subdev_frame_interval *interval)
{
	struct ov_device *dev = to_ov_sensor(sd);

	interval->interval.numerator = 1;
	interval->interval.denominator = dev->cur_fps;

	return 0;
}

static int
ov_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
	int index;
	struct ov_device *dev = to_ov_sensor(sd);
	struct ov_res_struct *ov_res = dev->product_info->ov_res;

	for (index = 0; index < dev->product_info->num_res; index++) {
		if (ov_res[index].res_id == dev->cur_res) {
			*frames = ov_res[index].skip_frames;
			return 0;
		}
	}
	pr_err("%s: failed to find the match resolution id!\n", __func__);
	return -EINVAL;
}

struct v4l2_ctrl_ops ov_ctrl_ops = {
	.s_ctrl				= ov_set_ctrl,
	.g_volatile_ctrl	= ov_get_ctrl,
};

static const struct v4l2_subdev_video_ops ov_video_ops = {
	.try_mbus_fmt = ov_try_mbus_fmt,
	.g_mbus_fmt = ov_g_mbus_fmt,
	.s_mbus_fmt = ov_s_mbus_fmt,
	.s_parm = ov_s_parm,
	.g_parm = ov_g_parm,
	.s_stream = ov_s_stream,
	.enum_framesizes = ov_enum_framesizes,
	.enum_frameintervals = ov_enum_frameintervals,
	.g_frame_interval = ov_g_frame_interval,
};

static const struct v4l2_subdev_sensor_ops ov_sensor_ops = {
	.g_skip_frames	= ov_g_skip_frames,
};

static const struct v4l2_subdev_core_ops ov_core_ops = {
	.g_chip_ident = ov_g_chip_ident,
	.s_power = ov_s_power,
	.ioctl = ov_ioctl,

	.queryctrl = v4l2_subdev_queryctrl,
	.g_ctrl = v4l2_subdev_g_ctrl,
	.s_ctrl = v4l2_subdev_s_ctrl,
};

static const struct v4l2_subdev_pad_ops ov_pad_ops = {
	.enum_mbus_code = ov_enum_mbus_code,
	.enum_frame_size = ov_enum_frame_size,
	.get_fmt = ov_get_pad_format,
	.set_fmt = ov_set_pad_format,
	.set_selection = ov_set_selection,
};

static const struct v4l2_subdev_ops ov_ops = {
	.core = &ov_core_ops,
	.video = &ov_video_ops,
	.sensor = &ov_sensor_ops,
	.pad = &ov_pad_ops,
};

static int __ov_init_ctrl_handler(struct ov_device *dev)
{
	int i;
	int ret;

	dev->ctrl_config = ov_controls;
	dev->num_ctrls = ARRAY_SIZE(ov_controls);

	ret = v4l2_ctrl_handler_init(&dev->ctrl_handler, dev->num_ctrls);
	if (ret)
		return ret;

	for (i = 0; i < dev->num_ctrls; i++) {
		dev->ctrl_config[i].config.ops = &ov_ctrl_ops;
		dev->ctrl_config[i].config.flags |= V4L2_CTRL_FLAG_VOLATILE;

		v4l2_ctrl_new_custom(&dev->ctrl_handler,
					&(dev->ctrl_config[i].config), NULL);

		if (dev->ctrl_handler.error)
			return dev->ctrl_handler.error;
	}
	dev->sd.ctrl_handler = &dev->ctrl_handler;
	/*FixME: ignore if ctrl handler failed, as not all ctrls we supportted*/
	v4l2_ctrl_handler_setup(&dev->ctrl_handler);
	return ret;
}

static int __ov_detect(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_adapter *adapter = client->adapter;
	struct ov_device *dev = to_ov_sensor(sd);
	u32 retvalue = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "%s: i2c error", __func__);
		return -ENODEV;
	}

	if (__ov_read_reg(client, MISENSOR_16BIT,
		dev->reg_id, &retvalue)) {
		dev_err(&client->dev, "sensor_id_high = 0x%x\n", retvalue);
		return -ENODEV;
	}

	dev_info(&client->dev, "sensor_id = 0x%x\n", retvalue);
	if (retvalue != dev->sensor_id) {
		dev_err(&client->dev, "%s: failed: client->addr = %x\n",
			__func__, client->addr);
		return -ENODEV;
	}

	return 0;
}

static int
__ov_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct ov_device *dev = to_ov_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
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

	ret = __ov_power_up(sd);
	if (ret) {
		dev_err(&client->dev, "power_ctrl failed");
		return ret;
	}

	/* config & detect sensor */
	ret = __ov_detect(sd);
	if (ret) {
		dev_err(&client->dev, "ov_detect err s_config.\n");
		goto fail_detect;
	}

	ret = __ov_init(sd);
	if (ret) {
		dev_err(&client->dev, "__ov_init error.\n");
		return -ENODEV;
	}

	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret)
		goto fail_csi_cfg;

	ret = __ov_power_down(sd);
	if (ret)
		dev_err(&client->dev, "sensor power-gating failed\n");

	return ret;

fail_csi_cfg:
	dev->platform_data->csi_cfg(sd, 0);
fail_detect:
	__ov_power_down(sd);
	dev_err(&client->dev, "sensor power-gating failed\n");
	return ret;
}

static int ov_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov_device *dev = container_of(sd,
					struct ov_device, sd);

	dev->platform_data->csi_cfg(sd, 0);
	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	v4l2_ctrl_handler_free(&dev->ctrl_handler);
	kfree(dev);

	return 0;
}

static int ov_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct ov_device *dev;
	int ret;
	struct ov_product_info *pinfo;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&client->dev, "out of memory!\n");
		return -ENOMEM;
	}

	v4l2_i2c_subdev_init(&(dev->sd), client, &ov_ops);
	mutex_init(&dev->input_lock);

	pinfo = (struct ov_product_info*)id->driver_data;
	dev->product_info = pinfo;
	dev->sensor_id = pinfo->sensor_id;
	dev->reg_id = pinfo->reg_id;

	if (client->dev.platform_data) {
		ret = __ov_s_config(&dev->sd, client->irq,
				       client->dev.platform_data);
		if (ret) {
			dev_err(&client->dev, "fail to init ov sensor!\n");
			goto out_free;
		}
	} else {
		dev_err(&client->dev, "no platform data!\n");
		return -ENODEV;
	}

	ret = __ov_init_ctrl_handler(dev);
	if (ret) {
		dev_err(&client->dev, "fail to init ctrl ops!\n");
		goto out_ctrl_free;
	}
	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dev->pad.flags = MEDIA_PAD_FL_SOURCE;
	dev->format.code = V4L2_MBUS_FMT_UYVY8_1X16;
	dev->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;

	ret = media_entity_init(&dev->sd.entity, 1, &dev->pad, 0);
	if (ret) {
		dev_err(&client->dev, "fail to init media entity!\n");
		ov_remove(client);
	}
	return ret;

out_ctrl_free:
	v4l2_ctrl_handler_free(&dev->ctrl_handler);

out_free:
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);
	return ret;
}

MODULE_DEVICE_TABLE(i2c, ov_ids);
static struct i2c_driver ov_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ov2685",
	},
	.probe = ov_probe,
	.remove = __exit_p(ov_remove),
	.id_table = ov_ids,
};

static __init int ov_init_mod(void)
{
	return i2c_add_driver(&ov_driver);
}

static __exit void ov_exit_mod(void)
{
	i2c_del_driver(&ov_driver);
}

module_init(ov_init_mod);
module_exit(ov_exit_mod);

MODULE_AUTHOR("Xu Qing <qing.xu@intel.com>");
MODULE_DESCRIPTION("A low-level class driver for Omnivision sensors");
MODULE_LICENSE("GPL");
