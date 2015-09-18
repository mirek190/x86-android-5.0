/*
 * Support for ov Camera Sensor.
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

#ifndef __OV_H__
#define __OV_H__

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/spinlock.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <linux/v4l2-mediabus.h>
#include <media/media-entity.h>
#include <linux/atomisp_platform.h>
#include <linux/atomisp.h>

#define V4L2_IDENT_OV 1111
/* #defines for i2c register writes and register array processing */
#define MISENSOR_8BIT		0x0001
#define MISENSOR_16BIT		0x0002
#define MISENSOR_32BIT		0x0004
#define MISENSOR_RMW_AND    0x0008
#define MISENSOR_RMW_OR     0x0010

#define MISENSOR_TOK_TERM	0xf000	/* terminating token for reg list */
#define MISENSOR_TOK_DELAY	0xfe00	/* delay token for reg list */
#define MISENSOR_TOK_FWLOAD	0xfd00	/* token indicating load FW */
#define MISENSOR_TOK_POLL	0xfc00	/* token indicating poll instruction */

#define I2C_RETRY_COUNT		5
#define I2C_MSG_LEN_OFFSET	2

/* #defines for resolutions */
#define RES_5M_SIZE_H		2560
#define RES_5M_SIZE_V		1920
#define RES_2M_SIZE_H		1600
#define RES_2M_SIZE_V		1200
#define RES_1080P_SIZE_H	1920
#define RES_1080P_SIZE_V	1080
#define RES_720P_SIZE_H		1280
#define RES_720P_SIZE_V		720
#define RES_480P_SIZE_H		720
#define RES_480P_SIZE_V		480
#define RES_VGA_SIZE_H		640
#define RES_VGA_SIZE_V		480
#define RES_QVGA_SIZE_H		320
#define RES_QVGA_SIZE_V		240

/* aspect ratio */
#define RATIO_4_TO_3 1333 /* 4:3*//* such as, 1600x1200*/
#define RATIO_3_TO_2 1500 /* 3:2*//* such as, 720x480*/
#define RATIO_16_TO_9 1777 /* 16:9*//* such as, 1280x720*/

#define RATIO_4_3 (1 << 0)
#define RATIO_3_2 (1 << 1)
#define RATIO_16_9 (1 << 2)

/*
 resolution table
 NOTE: make sure this enum is define in sequence of width
*/
enum res_type {
	RES_QVGA,	/* 320x240 */
	RES_VGA,	/* 640x480 */
	RES_480P,	/* 720x480 */
	RES_SVGA,	/* 800x600 */
	RES_720P,	/* 1280x720 */
	RES_2M,		/* 1600x1200 */
	RES_1080P,	/* 1920x1080 */
	RES_5M,		/* 2560x1920 */
};

/*
 * struct misensor_reg - MI sensor register format
 * @length: length of the register
 * @reg: 16-bit offset to register
 * @val: 8/16/32-bit register value
 * Define a structure for sensor register initialization values
 */
struct misensor_reg {
	u16 length;
	u16 reg;
	u32 val;	/* value or for read/mod/write */
};

struct ov_reglist_map {
	int id;
	struct misensor_reg *reg_list;
};

struct ov_control {
	struct v4l2_ctrl_config config;
	int (*s_ctrl)(struct v4l2_subdev *sd, s32 value);
	int (*g_ctrl)(struct v4l2_subdev *sd, s32 *value);
};

struct ov_res_struct {
	u8 *desc;
	enum res_type res_id;
	u32 width;
	u32 height;
	int fps;
	int skip_frames;
	bool used;
	int aspt_ratio;
	int csi_lanes;
	int max_exposure_lines;
	struct misensor_reg* regs;
};

struct ov_mbus_fmt {
	__u8 *desc;
	enum v4l2_mbus_pixelcode code;
	int	size;
	int bpp;
	struct misensor_reg* reg_list;
};

enum settings_type {
	OV_SETTING_STREAM,
	OV_SETTING_EXPOSURE,
	OV_SETTING_VFLIP,
	OV_SETTING_HFLIP,
	OV_SETTING_SCENE_MODE,
	OV_SETTING_COLOR_EFFECT,
	OV_SETTING_AWB_MODE,
	OV_SETTING_RESOLUTION,
	OV_SETTING_FORMAT,
	OV_SETTING_FREQ,

	OV_NUM_SETTINGS,
};

struct ov_table_info {
	enum settings_type type_id;
	struct ov_reglist_map *reglist;
	int num_tables;
};

struct ov_product_info {
	/* TODO: platform resource */
	u16 sensor_id;
	u16 reg_id;
	int gpio_pin;
	int regulator_id;
	int clk_id;
	int clk_freq;

	/* sensor i2c */
	int i2c_reg_len;
	int i2c_val_len;

	/* sensor features */
	s32 focal;
	s32 f_number;
	s32 f_number_range;
	u16 reg_expo_coarse;
	/* awb/scene mode/coloreffect/flip/exposure/... */
	struct ov_table_info settings_tbl[OV_NUM_SETTINGS];
	struct ov_res_struct* ov_init;	/* initialize */
	struct ov_res_struct* ov_res;	/* resolution */
	struct ov_mbus_fmt* ov_fmt;		/* data format */
	int num_fmt;
	int num_res;
};

/* each sensor has its ov_device instance */
struct ov_device {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt format;
	struct camera_sensor_platform_data *platform_data;
	int run_mode;
	struct mutex input_lock;

	u16 reg_id;
	u16 sensor_id;

	struct ov_product_info *product_info;
	enum res_type cur_res;
	u32 cur_fps;
	int max_exposure_lines;
	struct ov_res_struct* ov_res;
	struct ov_mbus_fmt* ov_format;

	struct v4l2_ctrl_handler ctrl_handler;
	struct ov_control *ctrl_config;
	int num_ctrls;

	int id_exposure;
	int id_awb;
	int id_scene;
	int id_coloreffect;
	int id_freq;
	int id_vflip;
	int id_hflip;
};

#define ADD_SETTINGS_TABLES(_type_id, _reglist) \
{\
		.type_id = _type_id, \
		.reglist = _reglist, \
		.num_tables = ARRAY_SIZE(_reglist), \
}

#endif
