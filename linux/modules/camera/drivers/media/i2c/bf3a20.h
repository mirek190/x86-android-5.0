/*
 * Support for BYD BF3A20 2MP camera sensor.
 *
 * Copyright (c) 2013 Intel Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 it under the terms of the GNU General Public License version
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

#ifndef __BF3A20_H__
#define __BF3A20_H__
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/spinlock.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <linux/v4l2-mediabus.h>
#include <media/media-entity.h>

#include <linux/atomisp_platform.h>

#define BF3A20_NAME		"bf3a20"

/* Defines for register writes and register array processing */
#define I2C_MSG_LENGTH		1
#define BF3A20_I2C_RETRY_COUNT		10

#define BF3A20_FOCAL_LENGTH_NUM	264	/*2.64mm*/
#define BF3A20_FOCAL_LENGTH_DEM	100
#define BF3A20_F_NUMBER_DEFAULT_NUM	28
#define BF3A20_F_NUMBER_DEM	10

#define MAX_FMTS		1

/*
 * focal length bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define BF3A20_FOCAL_LENGTH_DEFAULT 0x1080064

/*
 * current f-number bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define BF3A20_F_NUMBER_DEFAULT 0x1c000a

/*
 * f-number range bits definition:
 * bits 31-24: max f-number numerator
 * bits 23-16: max f-number denominator
 * bits 15-8: min f-number numerator
 * bits 7-0: min f-number denominator
 */
#define BF3A20_F_NUMBER_RANGE 0x1c0a1c0a

#define BF3A20_FINE_INTG_TIME_MIN 0
#define BF3A20_FINE_INTG_TIME_MAX_MARGIN 0
#define BF3A20_COARSE_INTG_TIME_MIN 1
#define BF3A20_COARSE_INTG_TIME_MAX_MARGIN (0xffff - 6)

/* BF3A20 product id */
#define BF3A20_PID_BME		0xFC
#define BF3A20_VER_BME		0xFD
#define BF3A20_PRODUCT_ID	0x2039
/*
 * BF3A20 System control registers
 */
#define BF3A20_SW_RESET_VAL		0x80
#define BF3A20_START_STREAMING	0x7F
#define BF3A20_STOP_STREAMING	0x80

#define	BF3A20_COARSE_INTG_TIME_MSB 0x8C
#define	BF3A20_COARSE_INTG_TIME_LSB 0x8D
#define	BF3A20_GLOBAL_GAIN0_REG		0x87
#define BF3A20_PWDN_REG				0x09

#define BF3A20_SW_RESET_REG	0x12

#define BF3A20_REG_END	  0xfa
#define BF3A20_REG_DELAY  0xfb

struct regval_list {
	u16 reg_num;
	u8 value;
};

struct bf3a20_resolution {
	u8 *desc;
	const struct regval_list *regs;
	int res;
	int width;
	int height;
	int fps;
	unsigned int crop_h_start; /* Sensor crop start (x0,y0)*/
	unsigned int crop_v_start;
	unsigned int crop_h_end; /* Sensor crop end (x1,y1)*/
	unsigned int crop_v_end;
	unsigned int output_width; /* input size to ISP after binning/scaling */
	unsigned int output_height;
	int pix_clk_freq;
	u32 skip_frames;
	u16 pixels_per_line;
	u16 lines_per_frame;
	u8 bin_factor_x;
	u8 bin_factor_y;
	u8 bin_mode;
	bool used;
};

struct bf3a20_format {
	u8 *desc;
	u32 pixelformat;
	struct regval_list *regs;
};

struct bf3a20_control {
	struct v4l2_queryctrl qc;
	int (*query)(struct v4l2_subdev *sd, s32 *value);
	int (*tweak)(struct v4l2_subdev *sd, s32 value);
};

/*
 * bf3a20 device structure.
 */
struct bf3a20_device {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt format;
	struct mutex input_lock;

	struct camera_sensor_platform_data *platform_data;
	int vt_pix_clk_freq_mhz;
	int fmt_idx;
	int run_mode;
	u8 res;
	u8 type;
};

enum bf3a20_tok_type {
	BF3A20_8BIT  = 0x0001,
	BF3A20_16BIT = 0x0002,
	BF3A20_32BIT = 0x0004,
	BF3A20_TOK_TERM   = 0xf000,	/* terminating token for reg list */
	BF3A20_TOK_DELAY  = 0xfe00,	/* delay token for reg list */
	BF3A20_TOK_MASK = 0xfff0
};

#define to_bf3a20_sensor(x) container_of(x, struct bf3a20_device, sd)
#define BF3A20_MAX_WRITE_BUF_SIZE	30

struct bf3a20_write_buffer {
	u16 addr;
	u8 data[BF3A20_MAX_WRITE_BUF_SIZE];
};

struct bf3a20_write_ctrl {
	int index;
	struct bf3a20_write_buffer buffer;
};

static const struct i2c_device_id bf3a20_id[] = {
	{BF3A20_NAME, 0},
	{}
};

/*
 * Register settings for various resolution
 */
static struct regval_list const bf3a20_init_sensor[] = {
/*{0x12, 0x80},*/
{0x15, 0x82},
{0x09, 0x42},
{0x21, 0x15},
{0x12, 0x01},
{0x1e, 0x76}, /* 0x76 - GBRG , 0x46 - BGGR */
{0x1b, 0x24},
{0x11, 0x2f}, /* 72M */
{0x2a, 0x0c},
{0x2b, 0x10},
{0x92, 0x02},
{0x93, 0x00},
{0x8a, 0x60},
{0x8b, 0x50},
{0x13, 0x00},
{0x8c, 0x02},
{0x8d, 0x31},
{0x6a, 0x84},
{0x23, 0x00},
{0x01, 0x20},
{0x02, 0x20},
{0x24, 0xc8},
{0x87, 0x2d},
{0x27, 0x98},
{0x28, 0xff},
{0x29, 0x60},
{0x1f, 0x20},
{0x22, 0x20},
{0x20, 0x20},
{0x26, 0x20},
{0xbb, 0x30},
{0xe2, 0xc4},
{0xe7, 0x4e},
{0x08, 0x16},
{0x16, 0x08},
{0x1c, 0xc0},
{0x1d, 0xf1},
{0xed, 0x8f},
{0x3e, 0x80},
{0xf9, 0x80},
/* 72M mipi setting  */
{0xdb,0x80}, 
{0xf8,0x83}, 
{0xde,0x2a}, 
{0xf9,0xc0}, 
{0x59,0x18}, 
{0x5f,0x28}, 
{0x6b,0x70}, 
{0x6c,0x00}, 
{0x6d,0x10}, 
{0x6e,0x08}, 
{0xf9,0x80}, 
{0x59,0x00}, 
{0x5f,0x10}, 
{0x6b,0x00}, 
{0x6c,0x28}, 
{0x6d,0x08}, 
{0x6e,0x00}, 
{0xfb,0x0c},
{0x13,0x00},
{0x6a,0x84},
{0x23,0x00},
{0x01,0x20},
{0x02,0x20},
{0x24,0xc8},
{0x87,0x18},
{0x8c,0x04},
{0x8d,0x12},

{0xf1, 0xff}, /* ISP Bypass */
{BF3A20_REG_END, 0},
};

/* 1600x1200@30fps	*/
static struct regval_list const bf3a20_1600_1200_15fps[] = {
/* 72M */
{0x11,0x2f},
{0x1b,0x24},/* 72M */
{0x4a,0x84},
{0xcc,0x60},
{0xca,0x00},
{0xcb,0x50},
{0xcf,0x40},
{0xcd,0x00},
{0xce,0xc0},
{0xc0,0x00},
{0xc1,0x00},
{0xc2,0x00},
{0xc3,0x00},
{0xc4,0x00},
{0xb5,0x00},
{0x03,0x60},
{0x17,0x00},
{0x18,0x50},
{0x10,0x40},
{0x19,0x00},
{0x1a,0xc0},
{0x0b,0x00},
{BF3A20_REG_END, 0},
};

/* 1600x1066@30fps	*/
static struct regval_list const bf3a20_1600_1064_20fps[] = {
{0x11,0x2f},
{0x1b,0x24},/* 72M */
{0x4a,0x84},
{0xcc,0x60},
{0xca,0x00},
{0xcb,0x50},
{0xcf,0x40},
{0xcd,0x78},
{0xce,0xb0},
{0xc0,0x00},
{0xc1,0x00},
{0xc2,0x00},
{0xc3,0x00},
{0xc4,0x00},
{0xb5,0x00},
{0x03,0x60},
{0x17,0x00},
{0x18,0x50},
{0x10,0x40},
{0x19,0x00},
{0x1a,0x38},
{0x0b,0x00},
{BF3A20_REG_END, 0},
};

/* 1600x900@30fps	*/
static struct regval_list const bf3a20_1600_900_20fps[] = {
{0x11,0x2f},
{0x1b,0x24},/* 72M */
{0x4a,0x84},
{0xcc,0x60},
{0xca,0x00},
{0xcb,0x50},
{0xcf,0x40},
{0xcd,0x78},
{0xce,0x0c},
{0xc0,0x00},
{0xc1,0x00},
{0xc2,0x00},
{0xc3,0x00},
{0xc4,0x00},
{0xb5,0x00},
{0x03,0x60},
{0x17,0x00},
{0x18,0x50},
{0x10,0x30},
{0x19,0x00},
{0x1a,0x94},
{0x0b,0x00},
{BF3A20_REG_END, 0},
};

/* 1280x720@30fps  */
static struct regval_list const bf3a20_720P_30fps[] = {
/* 72M */
{0x11,0x2f},
{0x1b,0x24},/* 72M */
{0x4a,0x84},
{0xcc,0x50},
{0xca,0xa0},
{0xcb,0xb0},
{0xcf,0x30},
{0xcd,0xf0},
{0xce,0xd0},
{0xc0,0x00},
{0xc1,0x00},
{0xc2,0x00},
{0xc3,0x00},
{0xc4,0x00},
{0xb5,0x00},
{0x03,0x50},
{0x17,0x00},
{0x18,0x10},
{0x10,0x20},
{0x19,0x00},
{0x1a,0xe0},
{0x0b,0x00},
{BF3A20_REG_END, 0},
};

/* 808x608@30fps */
static struct regval_list const bf3a20_808_608_30fps[] = {
/* 40.8M */
{0x11,0x31},
{0x1b,0x2c},  /* 40.8M */
{0x4a,0x86},
{0xcc,0x60},
{0xca,0x00},
{0xcb,0x50},
{0xcf,0x40},
{0xcd,0x00},
{0xce,0xc0},
{0xc0,0x00},
{0xc1,0x00},
{0xc2,0x00},
{0xc3,0x00},
{0xc4,0x00},
{0xb5,0x00},
{0x03,0x30},
{0x17,0x00},
{0x18,0x28},
{0x10,0x20},
{0x19,0x00},
{0x1a,0x60},
{0x0b,0x00},
{BF3A20_REG_END, 0},
};

struct bf3a20_resolution bf3a20_res_preview[] = {
	{
		.desc = "bf3a20_808_608_30fps",
		.width = 808,
		.height = 608,
		.crop_h_start = 0,
		.crop_h_end = 1616,
		.crop_v_start = 0,
		.crop_v_end = 1216,
		.output_width = 808,
		.output_height = 608,
		.pix_clk_freq = (408*100*1000)/2,
		.fps = 30,
		.used = 0,
		.pixels_per_line = 1072,
		.lines_per_frame = 622,
		.bin_factor_x = 2,
		.bin_factor_y = 2,
		.bin_mode = 0,
		.skip_frames = 0,
		.regs = bf3a20_808_608_30fps,
	},
	{
		.desc = "bf3a20_1600_900_20fps",
		.width = 1616,
		.height = 916,
		.crop_h_start = 0,
		.crop_h_end = 1616,
		.crop_v_start = 120,
		.crop_v_end = 1036,
		.output_width = 1616,
		.output_height = 916,
		.pix_clk_freq = (72*1000*1000)/2,
		.fps = 20,
		.used = 0,
		.pixels_per_line = 1880,
		.lines_per_frame = 930,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 0,
		.regs = bf3a20_1600_900_20fps,
	}, 
	{
		.desc = "bf3a20_1600_1064_20fps",
		.width = 1616,
		.height = 1080,
		.crop_h_start = 0,
		.crop_h_end = 1616,
		.crop_v_start = 120,
		.crop_v_end = 1200,
		.output_width = 1616,
		.output_height = 1080,
		.pix_clk_freq = (72*1000*1000)/2,
		.fps = 20,
		.used = 0,
		.pixels_per_line = 1880,
		.lines_per_frame = 1094,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 0,
		.regs = bf3a20_1600_1064_20fps,
	},
	{
		.desc = "bf3a20_1600_1200_15fps",
		.width = 1616,
		.height = 1216,
		.crop_h_start = 0,
		.crop_h_end = 1616,
		.crop_v_start = 0,
		.crop_v_end = 1216,
		.output_width = 1616,
		.output_height = 1216,
		.pix_clk_freq = (72*1000*1000)/2,
		.fps = 15,
		.used = 0,
		.pixels_per_line = 1880,
		.lines_per_frame = 1230,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 0,
		.regs = bf3a20_1600_1200_15fps,
	},
};
#define N_RES_PREVIEW (ARRAY_SIZE(bf3a20_res_preview))

struct bf3a20_resolution bf3a20_res_still[] = {
	{
		.desc = "bf3a20_808_608_30fps",
		.width = 808,
		.height = 608,
		.crop_h_start = 0,
		.crop_h_end = 1616,
		.crop_v_start = 0,
		.crop_v_end = 1216,
		.output_width = 808,
		.output_height = 608,
		.pix_clk_freq = (408*100*1000)/2,
		.fps = 30,
		.used = 0,
		.pixels_per_line = 1072,
		.lines_per_frame = 622,
		.bin_factor_x = 2,
		.bin_factor_y = 2,
		.bin_mode = 0,
		.skip_frames = 0,
		.regs = bf3a20_808_608_30fps,
	},
	{
		.desc = "bf3a20_1600_900_20fps",
		.width = 1616,
		.height = 916,
		.crop_h_start = 0,
		.crop_h_end = 1616,
		.crop_v_start = 120,
		.crop_v_end = 1036,
		.output_width = 1616,
		.output_height = 916,
		.pix_clk_freq = (72*1000*1000)/2,
		.fps = 15,
		.used = 0,
		.pixels_per_line = 1880,
		.lines_per_frame = 930,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 0,
		.regs = bf3a20_1600_900_20fps,
	}, 
	{
		.desc = "bf3a20_1600_1064_20fps",
		.width = 1616,
		.height = 1080,
		.crop_h_start = 0,
		.crop_h_end = 1616,
		.crop_v_start = 120,
		.crop_v_end = 1200,
		.output_width = 1616,
		.output_height = 1080,
		.pix_clk_freq = (72*1000*1000)/2,
		.fps = 20,
		.used = 0,
		.pixels_per_line = 1880,
		.lines_per_frame = 1094,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 0,
		.regs = bf3a20_1600_1064_20fps,
	},
	{
		.desc = "bf3a20_1600_1200_15fps",
		.width = 1616,
		.height = 1216,
		.crop_h_start = 0,
		.crop_h_end = 1616,
		.crop_v_start = 0,
		.crop_v_end = 1216,
		.output_width = 1616,
		.output_height = 1216,
		.pix_clk_freq = (72*1000*1000)/2,
		.fps = 15,
		.used = 0,
		.pixels_per_line = 1880,
		.lines_per_frame = 1230,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 0,
		.regs = bf3a20_1600_1200_15fps,
	}, 
};
#define N_RES_STILL (ARRAY_SIZE(bf3a20_res_still))

struct bf3a20_resolution bf3a20_res_video[] = {
	{
		.desc = "bf3a20_808_608_30fps",
		.width = 808,
		.height = 608,
		.crop_h_start = 0,
		.crop_h_end = 1616,
		.crop_v_start = 0,
		.crop_v_end = 1216,
		.output_width = 808,
		.output_height = 608,
		.pix_clk_freq = (408*100*1000)/2,
		.fps = 30,
		.used = 0,
		.pixels_per_line = 1072,
		.lines_per_frame = 622,
		.bin_factor_x = 2,
		.bin_factor_y = 2,
		.bin_mode = 0,
		.skip_frames = 0,
		.regs = bf3a20_808_608_30fps,
	},
	{
		.desc = "bf3a20_1600_900_20fps",
		.width = 1616,
		.height = 916,
		.crop_h_start = 0,
		.crop_h_end = 1616,
		.crop_v_start = 120,
		.crop_v_end = 1036,
		.output_width = 1616,
		.output_height = 916,
		.pix_clk_freq = (72*1000*1000)/2,
		.fps = 20,
		.used = 0,
		.pixels_per_line = 1880,
		.lines_per_frame = 930,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 0,
		.regs = bf3a20_1600_900_20fps,
	}, 
	{
		.desc = "bf3a20_1600_1064_20fps",
		.width = 1616,
		.height = 1080,
		.crop_h_start = 0,
		.crop_h_end = 1616,
		.crop_v_start = 120,
		.crop_v_end = 1200,
		.output_width = 1616,
		.output_height = 1080,
		.pix_clk_freq = (72*1000*1000)/2,
		.fps = 20,
		.used = 0,
		.pixels_per_line = 1880,
		.lines_per_frame = 1094,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 0,
		.regs = bf3a20_1600_1064_20fps,
	},	
};
#define N_RES_VIDEO (ARRAY_SIZE(bf3a20_res_video))

static struct bf3a20_resolution *bf3a20_res = bf3a20_res_preview;
static int N_RES = N_RES_PREVIEW;
#endif
