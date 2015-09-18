/*
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
#include "gc.h"

#ifdef CONFIG_GC_0310
#include "gc_0310.h"
#endif

#ifdef CONFIG_GC_2155
#include "gc_2155.h"
#endif


#define GC_BIN_FACTOR_MAX			4
#define GC_MAX_FOCUS_POS			1023
#define GC_MAX_FOCUS_NEG		(-1023)
#define GC_VCM_SLEW_STEP_MAX	0x3f
#define GC_VCM_SLEW_TIME_MAX	0x1f
#define GC_EXPOSURE_MIN		-4
#define GC_EXPOSURE_MAX			4


/* I2C functions */
int gc_read_reg(struct i2c_client *client, u8 len, u8 reg, u8 *val)
{
	struct i2c_msg msg[2];
	unsigned char data[GC_SHORT_MAX];
	int err, i;

	if (len > GC_BYTE_MAX)
		return -EINVAL;

	memset(msg, 0 , sizeof(msg));
	memset(data, 0 , sizeof(data));

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = (u8 *)data;
	/* high byte goes first */

	data[0] = reg;

	msg[1].addr = client->addr;
	msg[1].len = len;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = (u8 *)data;

	err = i2c_transfer(client->adapter, msg, 2);
	if (err != 2) {

		dev_err(&client->dev, "[gcdriver]: i2c read failed\n");
		if (err >= 0)
			err = -EIO;
		goto error;
	}

	/* high byte comes first */
	if (len == GC_8BIT) {
		*val = (u8)data[0];
	} else {
		/* 16-bit access is default when len > 1 */
		for (i = 0; i < (len >> 1); i++)
			val[i] = be16_to_cpu(data[i]);
	}

	return 0;

error:
	return err;
}


static int gc_i2c_write(struct i2c_client *client, u16 len, u8 *data)
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

	/* END */
}

int gc_write_reg(struct i2c_client *client, u16 data_length, u16 reg, u16 val)
{
	int ret;
	unsigned char data[2] = {0};
	const u16 len = data_length + 1; /* address + data */

	/* high byte goes out first */
	data[0] = reg;
	data[1] = (u8) val;

	ret = gc_i2c_write(client, len, data);
	if (ret) {
		dev_err(&client->dev, "[gcdriver]: i2c write failed\n");
	}

	return ret;
}


#if 0
static int __gc_flush_reg_array(struct i2c_client *client,
					 struct gc_write_ctrl *ctrl)
{
	u16 size;

	if (ctrl->index == 0)
		return 0;

	size = sizeof(u16) + ctrl->index; /* 16-bit address + data */
	ctrl->buffer.addr = cpu_to_be16(ctrl->buffer.addr);
	ctrl->index = 0;

	return gc_i2c_write(client, size, (u8 *)&ctrl->buffer);
}
#endif

#if 0
static int __gc_buf_reg_array(struct i2c_client *client,
				   struct gc_write_ctrl *ctrl,
				   const struct gc_register *next)
{
	int size;
	u16 *data16;

	switch (next->type) {
	case GC_8BIT:
		size = 1;
		ctrl->buffer.data[ctrl->index] = (u8)next->val;
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
	if (ctrl->index + sizeof(u16) >= GC_MAX_WRITE_BUF_SIZE)
		return __gc_flush_reg_array(client, ctrl);

	return 0;
}
#endif

#if 0
static int
__gc_write_reg_is_consecutive(struct i2c_client *client,
				   struct gc_write_ctrl *ctrl,
				   const struct gc_register *next)
{
	if (ctrl->index == 0)
		return 1;

	return ctrl->buffer.addr + ctrl->index == next->sreg;
}
#endif


/* __verify_register_write
	Verify that register write is properly updated. GC seems to require
	somewhere around 4ms for write to register to be reflected back when it is
	read out
*/
static int __verify_register_write(struct i2c_client *client,
									u8 reg, u8 expect_val)
{
	int count = 0;
	int err = 0;
	u8 new_val;

	while (1) {
	   count++;
	   msleep(4);
	   err = gc_read_reg(client, GC_8BIT, reg, &new_val);

		if (err)
			break;

		/* Success */
		if (new_val == expect_val)
			break;

		if (count >= GC_REG_UPDATE_RETRY_LIMIT) {
			dev_err(&client->dev, "Register update failed for GC register!!\n");
			err = -ENODEV;
			break;
		}
	}

	return err;
}


int gc_write_reg_array(struct i2c_client *client,
				   const struct gc_register *reglist)
{
	const struct gc_register *next = reglist;
	u8 tmp_val;

	int err;

	for (; next->type != GC_TOK_TERM; next++) {

		switch (next->type) {
		case GC_8BIT:
			err = gc_write_reg(client, GC_8BIT, next->sreg, (u16) next->val);
			break;

		case GC_8BIT_RMW_AND:
			err = gc_read_reg(client, GC_8BIT, next->sreg, &tmp_val);

			if (err)
				break;

			tmp_val = tmp_val & next->val;

			err = gc_write_reg(client, GC_8BIT, next->sreg, tmp_val);

			if (err)
				break;

			err = __verify_register_write(client, next->sreg, tmp_val);

			break;

		case GC_8BIT_RMW_OR:
			err = gc_read_reg(client, GC_8BIT, next->sreg, &tmp_val);

			if (err)
				break;


			tmp_val = tmp_val | next->val;
  
			err = gc_write_reg(client, GC_8BIT, next->sreg, tmp_val);

			if (err)
				break;

			err = __verify_register_write(client, next->sreg, tmp_val);

			break;

		default:
			break;

		}

	}

#if 0
		switch (next->type & GC_TOK_MASK) {
		case GC_TOK_DELAY:
			err = __gc_flush_reg_array(client, &ctrl);
			if (err)
				return err;
			msleep(next->val);
			break;

		default:
			/*
			 * If next address is not consecutive, data needs to be
			 * flushed before proceed.
			 */
			if (!__gc_write_reg_is_consecutive(client, &ctrl,
								next)) {
				err = __gc_flush_reg_array(client, &ctrl);
				if (err)
					return err;
			}
			err = __gc_buf_reg_array(client, &ctrl, next);
			if (err)
				return err;

			break;
		}
	}

	return __gc_flush_reg_array(client, &ctrl);
#endif
	return 0;
}


/* End of I2C functions */

/************************************************************/

/* Helper function */
static struct gc_table_info*
__get_register_table_info(struct gc_device *dev, enum gc_setting_enum setting_id)
{
	int i;

	for (i = 0; i < GC_NUM_SETTINGS; i++) {
		if (dev->product_info->settings_tables[i].setting_id == setting_id)
			return &dev->product_info->settings_tables[i];
	}

	return NULL;
}

#if 0

static enum atomisp_bayer_order __gc_bayer_order_mapping[] = {
	atomisp_bayer_order_gbrg,
	atomisp_bayer_order_bggr,
	atomisp_bayer_order_rggb,
	atomisp_bayer_order_grbg,
};


static enum v4l2_mbus_pixelcode
__gc_translate_bayer_order(enum atomisp_bayer_order code)
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


static int __gc_program_ctrl_table(struct v4l2_subdev *sd, enum gc_setting_enum setting_id, s32 value)
{
	struct gc_device		*dev = to_gc_sensor(sd);
	struct i2c_client		*client = v4l2_get_subdevdata(sd);
	struct gc_table_info	*table_info;
	int ret;
	int i;
 
	table_info = __get_register_table_info(dev, setting_id);

	if (!table_info) {
		return -EINVAL;
	}

	for (i = 0; i < table_info->num_tables; i++) {
		if (table_info->tables[i].lookup_value == value) {
			ret = gc_write_reg_array(client, table_info->tables[i].reg_table);
			break;
		}
	}

	return 0;
}


int __gc_set_vflip(struct v4l2_subdev *sd, s32 value)
{
	return __gc_program_ctrl_table(sd, GC_SETTING_VFLIP, value);
}

int __gc_set_hflip(struct v4l2_subdev *sd, s32 value)
{
	return __gc_program_ctrl_table(sd, GC_SETTING_HFLIP, value);
}

int __gc_s_exposure(struct v4l2_subdev *sd, s32 value)
{
	return __gc_program_ctrl_table(sd, GC_SETTING_EXPOSURE, value);
}

int __gc_set_scene_mode(struct v4l2_subdev *sd, s32 value)
{
	return __gc_program_ctrl_table(sd, GC_SETTING_SCENE_MODE, value);
}

int __gc_set_color(struct v4l2_subdev *sd, s32 value)
{
	return __gc_program_ctrl_table(sd, GC_SETTING_COLOR_EFFECT, value);
}

int __gc_set_wb(struct v4l2_subdev *sd, s32 value)
{
	return __gc_program_ctrl_table(sd, GC_SETTING_AWB_MODE, value);
}

int __gc_g_vflip(struct v4l2_subdev *sd, s32 *val)
{
	return 0;
}
int __gc_g_hflip(struct v4l2_subdev *sd, s32 *val)
{
	return 0;
}

int __gc_g_wb(struct v4l2_subdev *sd, s32 *val)
{
	return 0;
}
int __gc_g_color(struct v4l2_subdev *sd, s32 *val)
{
		return 0;
}
int __gc_g_scene_mode(struct v4l2_subdev *sd, s32 *val)
{
		return 0;
}


int __gc_g_focal_absolute(struct v4l2_subdev *sd, s32 *val)
{
	struct gc_device *dev = to_gc_sensor(sd);

	*val = dev->product_info->focal;

	return 0;
}

int __gc_g_fnumber(struct v4l2_subdev *sd, s32 *val)
{
	struct gc_device *dev = to_gc_sensor(sd);

	*val = dev->product_info->f_number;
	return 0;
}

int __gc_g_fnumber_range(struct v4l2_subdev *sd, s32 *val)
{
	struct gc_device *dev = to_gc_sensor(sd);

	*val = dev->product_info->f_number_range;
	return 0;
}



/* This returns the exposure time being used. This should only be used
   for filling in EXIF data, not for actual image processing. */
int __gc_g_exposure(struct v4l2_subdev *sd, s32 *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc_device *dev = to_gc_sensor(sd);
	u16 coarse;
	u8 reg_val_h, reg_val_l;
	int ret;

	/* the fine integration time is currently not calculated */
	ret = gc_read_reg(client, GC_8BIT,
				   dev->product_info->reg_expo_coarse, &reg_val_h);
	if (ret)
		return ret;

	coarse = ((u16)(reg_val_h & 0x1f)) << 8;

	ret = gc_read_reg(client, GC_8BIT,
				   dev->product_info->reg_expo_coarse + 1, &reg_val_l);
	if (ret)
		return ret;

	coarse |= reg_val_l;

	*value = coarse;
	return 0;
}

int __gc_g_bin_factor_x(struct v4l2_subdev *sd, s32 *val)
{
	/* fill in */
	return 0;
}

int __gc_g_bin_factor_y(struct v4l2_subdev *sd, s32 *val)
{
	/* fill in*/
	return 0;
}

int __gc_test_pattern(struct v4l2_subdev *sd, s32 val)
{
	/* fill in */
	return 0;
}



/* TODO: fill in with actual menu if needed */
static const char * const ctrl_power_line_frequency_menu[] = {
	NULL,
	"50 Hz",
	"60 Hz",
};
/* TODO: fill in with actual menu if needed */
static const char * const ctrl_exposure_metering_menu[] = {
	NULL,
	"Average",
	"Center Weighted",
	"Spot",
	"Matrix",
};

/* V4L2 control configuration below this line */
static struct gc_ctrl_config __gc_default_ctrls[] = {
	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_EXPOSURE_ABSOLUTE,
			.name = "Absolute exposure",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = GC_EXPOSURE_MIN,
			.max = GC_EXPOSURE_MAX,
			.step = 1,
			.def = -2,
		},
		.s_ctrl = __gc_s_exposure,
		.g_ctrl = __gc_g_exposure,
	},

	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_VFLIP,
			.name = "Vertical flip",
			.type = V4L2_CTRL_TYPE_BOOLEAN,
			.min = 0,
			.max = 1,
			.step = 1,
			.def = 0,
		},
		.s_ctrl = __gc_set_vflip,
		.g_ctrl = __gc_g_vflip,

	},

	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_HFLIP,
			.name = "Horizontal flip",
			.type = V4L2_CTRL_TYPE_BOOLEAN,
			.min = 0,
			.max = 1,
			.step = 1,
			.def = 0,
		},
		.s_ctrl = __gc_set_hflip,
		.g_ctrl = __gc_g_hflip,
	},

	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_FOCAL_ABSOLUTE,
			.name = "Focal length",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = GC_FOCAL_LENGTH_DEFAULT,
			.max = GC_FOCAL_LENGTH_DEFAULT,
			.step = 1,
			.def = GC_FOCAL_LENGTH_DEFAULT,
		},
		.s_ctrl = NULL,
		.g_ctrl = __gc_g_focal_absolute,
	},

	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_FNUMBER_ABSOLUTE,
			.name = "F-number",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 1,
			.max = GC_F_NUMBER_DEFAULT,
			.step = 1,
			.def = 1,
		},
		.s_ctrl = NULL,
		.g_ctrl = __gc_g_fnumber,
	},

	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_FNUMBER_RANGE,
			.name = "F-number range",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 1,
			.max = GC_F_NUMBER_RANGE,
			.step = 1,
			.def = 1,
		},
		.s_ctrl = NULL,
		.g_ctrl = __gc_g_fnumber_range,
	},

	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_BIN_FACTOR_HORZ,
			.name = "Horizontal binning factor",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 0,
			.max = GC_BIN_FACTOR_MAX,
			.step = 1,
			.def = 0,
		},
		.s_ctrl = NULL,
		.g_ctrl = __gc_g_bin_factor_x,
	},

	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_BIN_FACTOR_VERT,
			.name = "Vertical binning factor",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 0,
			.max = GC_BIN_FACTOR_MAX,
			.step = 1,
			.def = 0,
		},
		.s_ctrl = NULL,
		.g_ctrl = __gc_g_bin_factor_y,
	},

	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_TEST_PATTERN,
			.name = "Test pattern",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 0,
			.max = 0xffff,
			.step = 1,
			.def = 0,
		},
		.s_ctrl = __gc_test_pattern,
		.g_ctrl = NULL,
	},

	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_FOCUS_RELATIVE,
			.name = "Focus move relative",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = GC_MAX_FOCUS_NEG,
			.max = GC_MAX_FOCUS_POS,
			.step = 1,
			.def = 0,
		},
		.s_ctrl = NULL,
		.g_ctrl = NULL,
	},
	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_FOCUS_STATUS,
			.name = "Focus status",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 0,
			.max = 100, /* allow enum to grow in future */
			.step = 1,
			.def = 0,
		},
		.s_ctrl = NULL,
		.g_ctrl = NULL,
	},
	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_VCM_SLEW,
			.name = "Vcm slew",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 0,
			.max = GC_VCM_SLEW_STEP_MAX,
			.step = 1,
			.def = 0,
		},
		.s_ctrl = NULL,
		.g_ctrl = NULL,
	},

	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_VCM_TIMEING,
			.name = "Vcm step time",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 0,
			.max = GC_VCM_SLEW_TIME_MAX,
			.step = 1,
			.def = 0,
		},
	},
	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_3A_LOCK,
			.name = "Pause focus",
			.type = V4L2_CTRL_TYPE_BITMASK,
			.min = 0,
			.max = 1 << 2,
			.step = 0, /* set it to 0 always */
			.def = 0,
		},
	},
	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_COLORFX,
			.name = "Color effect",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 0,
			.max = 9,
			.step = 1,
			.def = 1,
		},
/*
		.s_ctrl = __gc_set_color,
*/
		.g_ctrl = __gc_g_color,
	},
	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_POWER_LINE_FREQUENCY,
			.name = "Power line frquency",
			.type = V4L2_CTRL_TYPE_MENU,   /* ????? */
			.min = 1,
			.max = 2,  /* 1: 50 Hz, 2: 60Hz*/
			.def = 2,
			.qmenu = ctrl_power_line_frequency_menu,
		},
	},
	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE,
			.name = "White balance",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 0,
			.max = 9,
			.step = 1,
			.def = 1,
		},
		.s_ctrl = __gc_set_wb,
		.g_ctrl = __gc_g_wb,
	},
	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_EXPOSURE,
			.name = "Exposure",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = GC_EXPOSURE_MIN,
			.max = GC_EXPOSURE_MAX,
			.step = 1,
			.def = -2,
		},
		.s_ctrl = __gc_s_exposure,
		.g_ctrl = NULL,
	},
	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_EXPOSURE_METERING,
			.name = "Exposure metering",
			.type = V4L2_CTRL_TYPE_MENU,
			.min = 1,
			.max = 4,
			.def = 4,
			.qmenu = ctrl_exposure_metering_menu,
		},
	},
	{
		.config = {
			.ops = NULL,
			.id = V4L2_CID_SCENE_MODE,
			.name = "Scene mode",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.min = 0,
			.max = 9,
			.step = 1,
			.def = 0,
		},
		.s_ctrl = __gc_set_scene_mode,
		.g_ctrl = __gc_g_scene_mode,
	},

};


/*******************************************************/
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

static long gc_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	/* TODO: add support for debug register read/write ioctls */
	return 0;
}

static int power_up(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc_device *dev = to_gc_sensor(sd);
	int ret;

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
		ret = dev->platform_data->gpio_ctrl(sd, 1);
		dev_err(&client->dev, "gpio failed\n");
		goto fail_gpio;
	}

		msleep(50);
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
	struct gc_device *dev = to_gc_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

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


static int gc_set_streaming(struct v4l2_subdev *sd, int enable)
{
	int ret;

	ret = __gc_program_ctrl_table(sd, GC_SETTING_STREAM, enable);

	return ret;

}

static int gc_init_common(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc_device *dev = to_gc_sensor(sd);
	int ret = 0;

	if (!dev->product_info) {
		return -ENODEV;
	}

	if (!dev->product_info->mode_info) {
		return -ENODEV;
	}	


	if (!dev->product_info->mode_info->init_settings) {
		/* Not an error as some sensor might not have init sequence */
		return 0;
	}

	ret = gc_write_reg_array(client, dev->product_info->mode_info->init_settings);
	return 0;
}

static int __gc_s_power(struct v4l2_subdev *sd, int on)
{
	struct gc_device *dev = to_gc_sensor(sd);
	int ret = 0;

	if (on == 0) {
		ret = power_down(sd);
		dev->power = 0;
	} else {
		ret = power_up(sd);
		if (!ret) {
			dev->power = 1;
			return gc_init_common(sd);
		}
	}


	return ret;
}

static int gc_s_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;
	struct gc_device *dev = to_gc_sensor(sd);

	mutex_lock(&dev->input_lock);

	ret = __gc_s_power(sd, on);

	mutex_unlock(&dev->input_lock);

	// as v4l2 framework would cache the value set from upper layer even though exit camera,
	// it would not call ctrl function because that the cache value is equal with the value
	// set by upper layer when entry camera again.
	// So, we will reset the v4l2 ctrl value to be default after power off.
	if (0 == on) {
		reset_v4l2_ctrl_value(&dev->ctrl_handler);
	}

	return ret;
}

static int gc_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!chip)
		return -EINVAL;

	v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_GC, 0);

	return 0;
}



static int get_resolution_index(struct v4l2_subdev *sd, int w, int h)
{
	int i;
	struct gc_device *dev = to_gc_sensor(sd);

	for (i = 0; i < dev->entries_curr_table; i++) {
		if (w != dev->curr_res_table[i].width)
			continue;
		if (h != dev->curr_res_table[i].height)
			continue;

		return i;
	}

	return -1;
}


static int gc_try_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct gc_device *dev = to_gc_sensor(sd);
	int idx = 0;
	const struct gc_resolution *tmp_res = NULL;

	mutex_lock(&dev->input_lock);

	if ((fmt->width > dev->product_info->max_res[0].res_max_width)
		|| (fmt->height > dev->product_info->max_res[0].res_max_height)) {
		fmt->width =  dev->product_info->max_res[0].res_max_width;
		fmt->height = dev->product_info->max_res[0].res_max_height;

	} else {

		idx = get_resolution_index(sd, fmt->width, fmt->height);

		if (idx < 0) {
			for (idx = 0; idx < dev->entries_curr_table; idx++) {

			  tmp_res = &dev->curr_res_table[idx];

			  if ((tmp_res->width >= fmt->width) &&
				  (tmp_res->height >= fmt->height))
				  break;
			 }
		}

		if (idx == dev->entries_curr_table)
			idx = dev->entries_curr_table - 1;

		fmt->width = dev->curr_res_table[idx].width;
		fmt->height = dev->curr_res_table[idx].height;

	}

	mutex_unlock(&dev->input_lock);

	return 0;
}

/* Find the product specific info about the format */
static int
gc_find_mbus_fmt_index(struct v4l2_subdev *sd,
				  struct v4l2_mbus_framefmt *fmt)
{
	struct gc_device *dev = to_gc_sensor(sd);
	int i;

	for (i = 0; i < dev->product_info->num_mbus_formats; i++) {
		if (dev->product_info->mbus_formats[i].code == fmt->code)
			return i;
	}

	return 0;
}

static int gc_s_mbus_fmt(struct v4l2_subdev *sd,
				  struct v4l2_mbus_framefmt *fmt)
{
	struct gc_device *dev = to_gc_sensor(sd);
	const struct gc_register *gc_mode_reg_table;
	struct camera_mipi_info *gc_info = NULL;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	int mbus_fmt_idx;

	gc_info = v4l2_get_subdev_hostdata(sd);
	if (gc_info == NULL)
		return -EINVAL;

	ret = gc_try_mbus_fmt(sd, fmt);

	if (ret)
		return ret;

	mutex_lock(&dev->input_lock);

	dev->fmt_idx = get_resolution_index(sd, fmt->width, fmt->height);
	/* Sanity check */
	if (unlikely(dev->fmt_idx == -1)) {
		ret = -EINVAL;
		goto out;
	}

	gc_mode_reg_table = dev->curr_res_table[dev->fmt_idx].regs;

	ret = gc_write_reg_array(client, gc_mode_reg_table);

	if (ret)
		goto out;

	dev->fps = dev->curr_res_table[dev->fmt_idx].fps;

	dev->pixels_per_line = dev->curr_res_table[dev->fmt_idx].pixels_per_line;
	dev->lines_per_frame = dev->curr_res_table[dev->fmt_idx].lines_per_frame;

	mbus_fmt_idx = gc_find_mbus_fmt_index(sd, fmt);

	if (mbus_fmt_idx == -1) {
		ret = -EINVAL;
		goto out;
	}

	dev->format.code = fmt->code;

	gc_mode_reg_table = dev->product_info->mbus_formats[mbus_fmt_idx].regs;

	ret = gc_write_reg_array(client, gc_mode_reg_table);

	if (ret)
		goto out;

out:
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int gc_g_mbus_fmt(struct v4l2_subdev *sd,
				  struct v4l2_mbus_framefmt *fmt)
{
	struct gc_device *dev = to_gc_sensor(sd);

	if (!fmt)
		return -EINVAL;

	fmt->width = dev->curr_res_table[dev->fmt_idx].width;
	fmt->height = dev->curr_res_table[dev->fmt_idx].height;

	fmt->code = V4L2_MBUS_FMT_UYVY8_1X16;

	return 0;
}



static int gc_detect(struct i2c_client *client, u16 *id)
{
	struct i2c_adapter *adapter = client->adapter;
	u8 id_l, id_h;

	/* i2c check */
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -ENODEV;


	/* check sensor chip ID	 */
	if (gc_read_reg(client, GC_8BIT, GC_REG_SENSOR_ID_HIGH_BIT, &id_h))
		return -ENODEV;


	*id = (u16)(id_h << 0x8);

	if (gc_read_reg(client, GC_8BIT, GC_REG_SENSOR_ID_LOW_BIT, &id_l))
		return -ENODEV;

	*id = *id + id_l;

	return 0;
}

/*
 * gc stream on/off
 */
static int gc_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret;
	struct gc_device *dev = to_gc_sensor(sd);


	mutex_lock(&dev->input_lock);
	if (enable) {
		ret = gc_set_streaming(sd, 1);
		dev->streaming = 1;
	} else {
		ret = gc_set_streaming(sd, 0);
		dev->streaming = 0;
		dev->fps_index = 0;
	}
	mutex_unlock(&dev->input_lock);

	return 0;
}


static int gc_enum_framesizes(struct v4l2_subdev *sd,
				   struct v4l2_frmsizeenum *fsize)
{
	unsigned int index = fsize->index;
	struct gc_device *dev = to_gc_sensor(sd);


	if (index >= dev->entries_curr_table)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = dev->curr_res_table[index].width;
	fsize->discrete.height = dev->curr_res_table[index].height;
	fsize->reserved[0] = dev->curr_res_table[index].used;


	return 0;
}

static int gc_enum_frameintervals(struct v4l2_subdev *sd,
					   struct v4l2_frmivalenum *fival)
{
	int i;
	struct gc_device *dev = to_gc_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	const struct gc_resolution *tmp_res = NULL;

	if (dev->curr_res_table == NULL) {
		dev_warn(&client->dev, "null res table, set default table\n");
		dev->curr_res_table = dev->product_info->mode_info->res_video;
		dev->entries_curr_table = dev->product_info->mode_info->n_res_video;
	}
	for (i = 0; i < dev->entries_curr_table; i++) {
		tmp_res = &dev->curr_res_table[i];

		if ((tmp_res->width >= fival->width) &&
			 (tmp_res->height >= fival->height))
			break;
	}

	if (i == dev->entries_curr_table)
		i--;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->width = dev->curr_res_table[i].width;
	fival->height = dev->curr_res_table[i].height;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = dev->curr_res_table[i].fps;


	return 0;
}


static int
gc_enum_mbus_code(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			   struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index)
		return -EINVAL;

	code->code = V4L2_MBUS_FMT_UYVY8_1X16;

	return 0;
}

static int
gc_enum_frame_size(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			struct v4l2_subdev_frame_size_enum *fse)
{
	int index = fse->index;
	struct gc_device *dev = to_gc_sensor(sd);


	if (index >= dev->entries_curr_table)
		return -EINVAL;

	fse->min_width = dev->curr_res_table[index].width;
	fse->min_height = dev->curr_res_table[index].height;
	fse->max_width = dev->curr_res_table[index].width;
	fse->max_height = dev->curr_res_table[index].height;

	return 0;
}


static int gc_get_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct gc_device *dev = to_gc_sensor(sd);

	switch (fmt->which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		fmt->format = *v4l2_subdev_get_try_format(fh, fmt->pad);
		break;
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		fmt->format = dev->format;
	}

		return 0;
}

static int
gc_set_pad_format(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			   struct v4l2_subdev_format *fmt)
{
	struct gc_device *dev = to_gc_sensor(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		dev->format = fmt->format;

	return 0;
}


static int gc_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct gc_device *dev = to_gc_sensor(sd);

	if (!param || param->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	dev->run_mode = param->parm.capture.capturemode;

	memset(param, 0, sizeof(*param));
	param->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	return 0;
}

static int
gc_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct gc_device *dev = to_gc_sensor(sd);
	dev->run_mode = param->parm.capture.capturemode;


	mutex_lock(&dev->input_lock);
	switch (dev->run_mode) {
	case CI_MODE_VIDEO:
		dev->curr_res_table = dev->product_info->mode_info->res_video;
		dev->entries_curr_table = dev->product_info->mode_info->n_res_video;
		break;

	case CI_MODE_STILL_CAPTURE:
		dev->curr_res_table = dev->product_info->mode_info->res_still;
		dev->entries_curr_table = dev->product_info->mode_info->n_res_still;
		break;

	default:
		dev->curr_res_table = dev->product_info->mode_info->res_preview;
		dev->entries_curr_table = dev->product_info->mode_info->n_res_preview;
	}
	mutex_unlock(&dev->input_lock);

	return 0;
}

int
gc_g_frame_interval(struct v4l2_subdev *sd,
				struct v4l2_subdev_frame_interval *interval)
{
	struct gc_device *dev = to_gc_sensor(sd);

	interval->interval.numerator = 1;
	interval->interval.denominator = dev->fps;

	return 0;
}

static int gc_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
	struct gc_device *dev = to_gc_sensor(sd);

	mutex_lock(&dev->input_lock);
	*frames = dev->curr_res_table[dev->fmt_idx].skip_frames;
	mutex_unlock(&dev->input_lock);


	return 0;
}


static int gc_get_ctrl(struct v4l2_ctrl *ctrl)
{
	struct gc_device *dev = container_of(ctrl->handler, struct gc_device, ctrl_handler);

	int i;
	int ret = 0;

	for (i = 0; i < dev->product_info->num_ctrls; i++) {
		if (dev->product_info->ctrl_config[i].config.id == ctrl->id) {

			if (dev->product_info->ctrl_config[i].g_ctrl) {

				ret = dev->product_info->ctrl_config[i].g_ctrl(&dev->sd, &ctrl->val);

				return ret;
			} else {
				return 0;
			}
		}
	}

	return 0;
}


static int gc_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct gc_device *dev = container_of(ctrl->handler, struct gc_device, ctrl_handler);

	int i;
	int ret = 0;

	for (i = 0; i < dev->product_info->num_ctrls; i++) {
		if (dev->product_info->ctrl_config[i].config.id == ctrl->id) {

			if (dev->product_info->ctrl_config[i].s_ctrl) {

				ret = dev->product_info->ctrl_config[i].s_ctrl(&dev->sd, ctrl->val);

				return ret;
			} else {

				return 0;
			}
		}
	}

	return 0;
}


#if 0
static int gc_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
				 enum v4l2_mbus_pixelcode *code)
{
	struct gc_device *dev = to_gc_sensor(sd);

	if (index >= dev->product_info->num_mbus_formats)
		return -EINVAL;

	*code = dev->product_info->mbus_formats[index].code;

	return 0;
}
#endif



/* end */


struct v4l2_ctrl_ops gc_ctrl_ops = {
	.s_ctrl				= gc_set_ctrl,
	.g_volatile_ctrl	= gc_get_ctrl,
};


static const struct v4l2_subdev_sensor_ops gc_sensor_ops = {
	.g_skip_frames	= gc_g_skip_frames,
};

static const struct v4l2_subdev_video_ops gc_video_ops = {
	.s_stream = gc_s_stream,
	.enum_framesizes = gc_enum_framesizes,
	.enum_frameintervals = gc_enum_frameintervals,
	.try_mbus_fmt = gc_try_mbus_fmt,
	.g_mbus_fmt = gc_g_mbus_fmt,
	.s_mbus_fmt = gc_s_mbus_fmt,
	.s_parm = gc_s_parm,
	.g_parm = gc_g_parm,
};

static const struct v4l2_subdev_core_ops gc_core_ops = {
	.g_chip_ident = gc_g_chip_ident,
	.s_power	 = gc_s_power,
	.ioctl		= gc_ioctl,

	.queryctrl	= v4l2_subdev_queryctrl,
	.g_ctrl		= v4l2_subdev_g_ctrl,
	.s_ctrl		= v4l2_subdev_s_ctrl,
};



static const struct v4l2_subdev_pad_ops gc_pad_ops = {
	.enum_mbus_code = gc_enum_mbus_code,
	.enum_frame_size = gc_enum_frame_size,
	.get_fmt = gc_get_pad_format,
	.set_fmt = gc_set_pad_format,
};

static const struct v4l2_subdev_ops gc_ops = {
	.core = &gc_core_ops,
	.video = &gc_video_ops,
	.pad = &gc_pad_ops,
	.sensor = &gc_sensor_ops,
};
static int __query_device_specifics(struct gc_device *dev);

static int gc_s_config(struct v4l2_subdev *sd, int irq, void *pdata)
{
	struct gc_device *dev = to_gc_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 sensor_id = 0x0;
	int ret = 0;

	if (NULL == pdata)
		return -ENODEV;

	dev->platform_data =
		(struct camera_sensor_platform_data *)pdata;

	mutex_lock(&dev->input_lock);

	if (dev->platform_data->platform_init) {
		ret = dev->platform_data->platform_init(client);
		if (ret) {
			mutex_unlock(&dev->input_lock);
			dev_err(&client->dev, "gc platform init err\n");
			return ret;
		}
	}

	/* step1 - power off the module */
	power_down(sd);

	power_up(sd);

	/* config & detect sensor */
	ret = gc_detect(client, &sensor_id);
	if (ret) {
		v4l2_err(client, "gc_detect err s_config.\n");
		goto fail_detect;
	}


	if (dev->product_info->sensor_id != sensor_id) {
		v4l2_err(client, "sensor id didn't match expected\n");
		ret = -ENODEV;
		goto fail_detect;
	}

	/* Query device specifics */
	ret = __query_device_specifics(dev);
	if (ret != 0) {
		v4l2_err(client, "__query_device error.\n");
		goto fail_detect;
	}


	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret)
		goto fail_csi_cfg;



	mutex_unlock(&dev->input_lock);

	/* power off sensor */
	power_down(sd);

	return ret;

fail_detect:
	dev->platform_data->csi_cfg(sd, 0);
fail_csi_cfg:

	power_down(sd);
	mutex_unlock(&dev->input_lock);
	dev_err(&client->dev, "sensor power-gating failed\n");

	return ret;
}


static int __query_device_specifics(struct gc_device *dev)
{

	/* If product specific does not need to override any configs
	   use default ones; otherwise override those that are specified */
	if (!dev->product_info->ctrl_config) {
		dev->product_info->ctrl_config = __gc_default_ctrls;
		dev->product_info->num_ctrls = ARRAY_SIZE(__gc_default_ctrls);
	} else {
		/* TODO: */
	}

	return 0;
}


static int __gc_init_ctrl_handler(struct gc_device *dev)
{
	int i;
	int ret;

	ret = v4l2_ctrl_handler_init(&dev->ctrl_handler, dev->product_info->num_ctrls);

	if (ret)
		return ret;

	for (i = 0; i < dev->product_info->num_ctrls; i++) {
		dev->product_info->ctrl_config[i].config.ops = &gc_ctrl_ops;
		dev->product_info->ctrl_config[i].config.flags |= V4L2_CTRL_FLAG_VOLATILE;

		v4l2_ctrl_new_custom(&dev->ctrl_handler, &(dev->product_info->ctrl_config[i].config), NULL);

		if (dev->ctrl_handler.error)
			return dev->ctrl_handler.error;
	}


	/* Use same lock for controls as for everything else. */
	dev->ctrl_handler.lock = &dev->input_lock;
	dev->sd.ctrl_handler = &dev->ctrl_handler;

	ret = v4l2_ctrl_handler_setup(&dev->ctrl_handler);
	
	return ret;
}



static int gc_remove(struct i2c_client *client);

static int gc_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct gc_device *dev;
	int ret;


	/* allocate sensor device & init sub device */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		v4l2_err(client, "%s: out of memory\n", __func__);
		return -ENOMEM;
	}

	mutex_init(&dev->input_lock);

	dev->fmt_idx = 0;

	dev->product_info = (struct gc_product_info*)id->driver_data;

	if (dev->product_info == NULL) {
		v4l2_err(client, "product_info was null\n");
		ret = -ENODEV;
		goto out_free;
	}

	v4l2_i2c_subdev_init(&(dev->sd), client, &gc_ops);

	if (client->dev.platform_data) {
		ret = gc_s_config(&dev->sd, client->irq,
					   client->dev.platform_data);

		if (ret)
			goto out_free;

	}


	snprintf(dev->sd.name, sizeof(dev->sd.name), "%s %d-%04x",
		dev->product_info->name,
		i2c_adapter_id(client->adapter), client->addr);


	/* Initialize v4l2 control handler */
	ret = __gc_init_ctrl_handler(dev);
	if (ret)
		goto out_ctrl_handler_free;

	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dev->pad.flags = MEDIA_PAD_FL_SOURCE;
	dev->format.code = V4L2_MBUS_FMT_UYVY8_1X16;

	dev->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;

	ret = media_entity_init(&dev->sd.entity, 1, &dev->pad, 0);
	if (ret)
		gc_remove(client);


		return ret;
out_ctrl_handler_free:
	v4l2_ctrl_handler_free(&dev->ctrl_handler);

out_free:
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);

	return ret;
}


static int gc_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gc_device *dev = to_gc_sensor(sd);


	if (dev->platform_data->platform_deinit)
		dev->platform_data->platform_deinit();

	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	v4l2_ctrl_handler_free(&dev->ctrl_handler);
	kfree(dev);

	return 0;
}


static const struct i2c_device_id gc_ids[] = {
#ifdef CONFIG_GC_2155
	{GC2155_NAME, (kernel_ulong_t)&gc2155_product_info },
#endif

#ifdef CONFIG_GC_0310
	{GC0310_NAME, (kernel_ulong_t)&gc0310_product_info },
#endif

	{ },
};

MODULE_DEVICE_TABLE(i2c, gc_ids);

static struct i2c_driver gc_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = GC_DRIVER,
	},
	.probe = gc_probe,
	.remove = __exit_p(gc_remove),
	.id_table = gc_ids,
};



static __init int init_gc(void)
{
	int rc;

	rc = i2c_add_driver(&gc_i2c_driver);

	return rc;
}

static __exit void exit_gc(void)
{
	i2c_del_driver(&gc_i2c_driver);
}

module_init(init_gc);
module_exit(exit_gc);


MODULE_DESCRIPTION("Class driver for GalaxyCore sensors");
MODULE_AUTHOR("Shamim Begum <shamim.begum@intel.com>");
MODULE_LICENSE("GPL");
