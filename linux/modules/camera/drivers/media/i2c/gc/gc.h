/*
 * GC camera 'class' driver
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

#ifndef __GC_H__
#define __GC_H__

#include <linux/atomisp_platform.h>
#include <linux/atomisp.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/v4l2-mediabus.h>
#include <media/media-entity.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>


#define AWB_MODE_AUTO					1
#define AWB_MODE_INCANDESCENT			2
#define AWB_MODE_FLUORESCENT			3
#define AWB_MODE_DAYLIGHT				6
#define AWB_MODE_CLOUDY					8

#define SCENE_MODE_AUTO				 0
#define SCENE_MODE_NIGHT				8

enum gc_tok_type {
		GC_8BIT			= 0x0001,	/* Regular 8bit write */
		GC_8BIT_RMW_AND = 0x0002,	/* Read-modify-write 8bit using AND operator */
		GC_8BIT_RMW_OR  = 0x0003,	/* Read-modify-write 8bit using OR operator */

		GC_TOK_TERM   = 0xf000,		/* terminating token for reg list */
		GC_TOK_DELAY  = 0xfe00, /* delay token for reg list */
		GC_TOK_MASK	= 0xfff0
};

/**
 * struct gc_register
 * @type: type of operation to perform on a register
 * @reg: 16-bit offset to register
 * @val: 8/16/32-bit register value
 *
 * Define a structure for sensor register initialization values
 */
struct gc_register {
		enum gc_tok_type type;
		u16 sreg;
		u32 val;
};


/* Defines for register writes and register array processing */
#define GC_BYTE_MAX	32 /* change to 32 as needed by otpdata */
#define GC_SHORT_MAX	16
#define I2C_RETRY_COUNT		5

#define GC_MAX_WRITE_BUF_SIZE	32
#define GC_REG_UPDATE_RETRY_LIMIT 100
struct gc_write_buffer {
	u16 addr;
	u8 data[GC_MAX_WRITE_BUF_SIZE];
};

struct gc_write_ctrl {
	int index;
	struct gc_write_buffer buffer;
};

int gc_read_reg(struct i2c_client *client, u8 len, u8 reg, u8 *val);
int gc_write_reg(struct i2c_client *client, u16 data_length, u16 reg, u16 val);

int gc_write_reg_array(struct i2c_client *client,
				   const struct gc_register *reglist);


/* TODO - This should be added into include/linux/videodev2.h */
#ifndef V4L2_IDENT_GC
#define V4L2_IDENT_GC	2155
#endif

#define GC_ID_DEFAULT 0


/* SHAMIM: register deinition for GC2155 */
/**/


#define MAX_FMTS 1

#define GC_SUBDEV_PREFIX "gc"
#define GC_DRIVER	"gc2155"

#define GC2155_NAME	"gc2155"
#define GC2155_ID	0x2155


#define GC0310_NAME "gc0310soc"
#define GC0310_ID	0xa310









#define GC_REG_SENSOR_ID_HIGH_BIT	0xf0
#define GC_REG_SENSOR_ID_LOW_BIT	0xf1



struct gc_max_res {
	int res_max_width;
	int res_max_height;
};


struct gc_fps_setting {
		int fps;
		unsigned short pixels_per_line;
		unsigned short lines_per_frame;
};

struct gc_resolution {
		u8 *desc;
		const struct gc_register *regs;
		int res;
		int width;
		int height;
		int fps;
		unsigned short pixels_per_line;
		unsigned short lines_per_frame;
		unsigned short skip_frames;
		u8 bin_factor_x;
		u8 bin_factor_y;
		bool used;
		u8 bin_mode;
};



struct gc_mode_info {
	struct gc_register const *init_settings;
	struct gc_resolution *res_preview;
	struct gc_resolution *res_still;
	struct gc_resolution *res_video;
	int n_res_preview;
	int n_res_still;
	int n_res_video;
};

struct gc_ctrl_config {
	struct v4l2_ctrl_config config;
	int (*s_ctrl)(struct v4l2_subdev *sd, s32 value);
	int (*g_ctrl)(struct v4l2_subdev *sd, s32 *value);
};

struct gc_mbus_fmt {
	__u8 *desc;
	enum v4l2_mbus_pixelcode code;
	struct gc_register *regs;
	int	size;
	int bpp;   /* Bytes per pixel */
};


enum gc_setting_enum {
	GC_SETTING_STREAM,
	GC_SETTING_EXPOSURE,
	GC_SETTING_VFLIP,
	GC_SETTING_HFLIP,
	GC_SETTING_SCENE_MODE,
	GC_SETTING_COLOR_EFFECT,
	GC_SETTING_AWB_MODE,

	GC_NUM_SETTINGS,
} gc_setting_enum;


struct gc_table_map {
	int lookup_value;
	struct gc_register *reg_table;
};


struct gc_table_info {
	enum gc_setting_enum		setting_id;
	struct gc_table_map *tables;
	int num_tables;
};



struct gc_product_info {
	char *name;
	u16 sensor_id;
	struct gc_mode_info *mode_info;
	struct gc_ctrl_config *ctrl_config;
	int num_ctrls;
	struct gc_max_res *max_res;
	struct gc_mbus_fmt *mbus_formats;
	int num_mbus_formats;


	s32 focal;
	s32	f_number;
	s32 f_number_range;

	u8 reg_expo_coarse;

	struct gc_table_info settings_tables[GC_NUM_SETTINGS];
};



struct gc_product_mapping {
	int gc_id;
	struct gc_product_info *product_info;
};




/* gc device structure */
struct gc_device {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt format;
	struct camera_sensor_platform_data *platform_data;
	struct mutex input_lock; /* serialize sensor's ioctl */
	int fmt_idx;
	int status;
	int streaming;
	int power;
	int run_mode;
	int vt_pix_clk_freq_mhz;
	int fps_index;
	u32 focus;
	u16 coarse_itg;
	u16 fine_itg;
	u16 gain;
	u16 pixels_per_line;
	u16 lines_per_frame;
	u8 fps;
	u8 res;
	u8 type;
	const struct gc_resolution *curr_res_table;
	int entries_curr_table;

	struct v4l2_ctrl_handler ctrl_handler;

	struct gc_product_info *product_info;
};

#define to_gc_sensor(x) container_of(x, struct gc_device, sd)


#define GC_ADD_SETTINGS_TABLES(_setting_id, _tables) \
	{\
		.setting_id = _setting_id, \
		.tables = _tables, \
		.num_tables = ARRAY_SIZE(_tables), \
	}


#endif /* __GC_H__ */
