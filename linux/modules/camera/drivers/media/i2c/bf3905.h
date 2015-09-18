/*
 * Support for BYD BF3905 0.3M camera sensor.
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

#ifndef __BF3905_H__
#define __BF3905_H__
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

#define BF3905_NAME		"bf3905"

/* Defines for register writes and register array processing */
#define I2C_MSG_LENGTH		0x1
#define I2C_RETRY_COUNT		5

#define BF3905_FOCAL_LENGTH_NUM	278	/*2.78mm*/
#define BF3905_FOCAL_LENGTH_DEM	100
#define BF3905_F_NUMBER_DEFAULT_NUM	26
#define BF3905_F_NUMBER_DEM	10

#define MAX_FMTS		1

/*
 * focal length bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define BF3905_FOCAL_LENGTH_DEFAULT 0x1160064

/*
 * current f-number bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define BF3905_F_NUMBER_DEFAULT 0x1a000a

/*
 * f-number range bits definition:
 * bits 31-24: max f-number numerator
 * bits 23-16: max f-number denominator
 * bits 15-8: min f-number numerator
 * bits 7-0: min f-number denominator
 */
#define BF3905_F_NUMBER_RANGE 0x1a0a1a0a
#define BF3905_ID	0x3905

#define BF3905_FINE_INTG_TIME_MIN 0
#define BF3905_FINE_INTG_TIME_MAX_MARGIN 0
#define BF3905_COARSE_INTG_TIME_MIN 1
#define BF3905_COARSE_INTG_TIME_MAX_MARGIN (0xffff - 6)

/*
 * BF3905 System control registers
 */
#define BF3905_SW_SLEEP			0x09//	0x0100
#define BF3905_SW_RESET			0x12//	0x0103
#define BF3905_SW_STREAM			0x09
#define	BF3905_COARSE_INTG_TIME_MSB 0x8C
#define	BF3905_COARSE_INTG_TIME_LSB 0x8D
#define	BF3905_GLOBAL_GAIN0_REG		0x9D

#define BF3905_SC_CMMN_CHIP_ID_H	0xfc
#define BF3905_SC_CMMN_CHIP_ID_L	0xfd//0x300B
#define BF3905_SC_CMMN_SCCB_ID		0x6e//0x300C


#if 0

#endif

#define BF3905_START_STREAMING		0x01//0x01
#define BF3905_STOP_STREAMING		0x81//0x00
#define DEBUG 0

struct regval_list {
	u16 reg_num;
	u8 value;
};

struct bf3905_resolution {
	u8 *desc;
	const struct bf3905_reg *regs;
	int res;
	int width;
	int height;
	int fps;
	int pix_clk_freq;
	u32 skip_frames;
	u16 pixels_per_line;
	u16 lines_per_frame;
	u8 bin_factor_x;
	u8 bin_factor_y;
	u8 bin_mode;
	bool used;
};

struct bf3905_format {
	u8 *desc;
	u32 pixelformat;
	struct bf3905_reg *regs;
};

struct bf3905_control {
	struct v4l2_queryctrl qc;
	int (*query)(struct v4l2_subdev *sd, s32 *value);
	int (*tweak)(struct v4l2_subdev *sd, s32 value);
};

/*
 * bf3905 device structure.
 */
struct bf3905_device {
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

enum bf3905_tok_type {
	BF3905_8BIT  = 0x0001,
	BF3905_16BIT = 0x0002,
	BF3905_32BIT = 0x0004,
	BF3905_TOK_TERM   = 0xf000,	/* terminating token for reg list */
	BF3905_TOK_DELAY  = 0xfe00,	/* delay token for reg list */
	BF3905_TOK_MASK = 0xfff0
};

#define to_bf3905_sensor(x) container_of(x, struct bf3905_device, sd)
/**
 * struct bf3905_reg - MI sensor  register format
 * @type: type of the register
 * @reg: 16-bit offset to register
 * @val: 8/16/32-bit register value
 *
 * Define a structure for sensor register initialization values
 */
struct bf3905_reg {
	enum bf3905_tok_type type;
	u16 reg;
	u32 val;	/* @set value for read/mod/write, @mask */
};

#define to_bf3905_sensor(x) container_of(x, struct bf3905_device, sd)

#define BF3905_MAX_WRITE_BUF_SIZE	30

struct bf3905_write_buffer {
	u16 addr;
	u8 data[BF3905_MAX_WRITE_BUF_SIZE];
};

struct bf3905_write_ctrl {
	int index;
	struct bf3905_write_buffer buffer;
};

static const struct i2c_device_id bf3905_id[] = {
	{BF3905_NAME, 0},
	{}
};

/*
 * Register settings for various resolution
 */
#if 0
static struct bf3905_reg const bf3905_QVGA_30fps[] = {
	{BF3905_8BIT, 0xda, 0x00},
	{BF3905_8BIT, 0xdb, 0xa2},
	{BF3905_8BIT, 0xdc, 0x00},
	{BF3905_8BIT, 0xdd, 0x7a},
	{BF3905_8BIT, 0xde, 0x00},
	{BF3905_8BIT, 0x4a, 0x00},//BIT 1:0
	{BF3905_8BIT, 0x0a, 0x21},
	{BF3905_8BIT, 0x10, 0x21},
	{BF3905_8BIT, 0x3d, 0x59},
	{BF3905_8BIT, 0x6b, 0x02},
	{BF3905_8BIT, 0x17, 0x00},
	{BF3905_8BIT, 0x18, 0xa0},
	{BF3905_8BIT, 0x19, 0x00},
	{BF3905_8BIT, 0x1a, 0x78},
	{BF3905_8BIT, 0x03, 0x00},
	//{BF3905_8BIT, 0x12, 0x01},//BIT 4   //VGA
	{BF3905_8BIT, 0x12, 0x11},//BIT 4   

	{BF3905_TOK_TERM, 0, 0},
};
#endif

static struct bf3905_reg const bf3905_QVGA_30fps[] = {
	{BF3905_8BIT, 0xda, 0x00},
	{BF3905_8BIT, 0xdb, 0xa2},
	{BF3905_8BIT, 0xdc, 0x00},
	{BF3905_8BIT, 0xdd, 0x7a},
	{BF3905_8BIT, 0xde, 0x00},
	{BF3905_8BIT, 0x4a, 0x00},//BIT 1:0
	{BF3905_8BIT, 0x0a, 0x21},
	{BF3905_8BIT, 0x10, 0x21},
	{BF3905_8BIT, 0x3d, 0x59},
	{BF3905_8BIT, 0x6b, 0x02},
	{BF3905_8BIT, 0x17, 0x00},
	{BF3905_8BIT, 0x18, 0x54},//A0//336
	{BF3905_8BIT, 0x19, 0x00},
	{BF3905_8BIT, 0x1a, 0x40},//78//256
	{BF3905_8BIT, 0x03, 0x00},
	{BF3905_8BIT, 0x12, 0x01},//BIT 4   //VGA
	//{BF3905_8BIT, 0x12, 0x11},//BIT 4 

	{BF3905_TOK_TERM, 0, 0},
};



static struct bf3905_reg const bf3905_VGA_30fps[] = {
    {BF3905_8BIT, 0xda, 0x00},
    {BF3905_8BIT, 0xdb, 0xa2},
    {BF3905_8BIT, 0xdc, 0x00},
    {BF3905_8BIT, 0xdd, 0x7a},
    {BF3905_8BIT, 0xde, 0x00},
    {BF3905_8BIT, 0x4a, 0x0e},
    {BF3905_8BIT, 0x17, 0x00},
    {BF3905_8BIT, 0x18, 0xa0},
    {BF3905_8BIT, 0x19, 0x00},
    {BF3905_8BIT, 0x1a, 0x78},
    {BF3905_8BIT, 0x03, 0x00},
	{BF3905_TOK_TERM, 0, 0},
};

struct bf3905_resolution bf3905_res_preview[] = {
#if (DEBUG == 1)
	{
		.desc = "bf3905_QVGA_30fps",
		.width = 336,
		.height = 256,
		.fps = 30,
		.pix_clk_freq = 25.6,//19.2*4/3
		.used = 0,
		.pixels_per_line = 800,//784+16
	.lines_per_frame = 510,//510+9
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = bf3905_QVGA_30fps,
	},
#endif
	{
		.desc = "bf3905_VGA_30fps",
		.width = 640,
		.height = 480,
		.fps = 30,
		.pix_clk_freq = 12800000,//1/2*Mclk, 25.6M MCLK
		.used = 0,
		.pixels_per_line = 816,//784+32
		.lines_per_frame = 510,//510+9

		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = bf3905_VGA_30fps,
	},
};
#define N_RES_PREVIEW (ARRAY_SIZE(bf3905_res_preview))

struct bf3905_resolution bf3905_res_still[] = {
#if (DEBUG==1)
{
	.desc = "bf3905_QVGA_30fps",
	.width = 336,
	.height = 256,
	.fps = 30,
	.pix_clk_freq = 25.6,//19.2*4/3
	.used = 0,
	.pixels_per_line = 800,//784+16
	.lines_per_frame = 510,//510+9
	.bin_factor_x = 1,
	.bin_factor_y = 1,
	.bin_mode = 0,
	.skip_frames = 3,
	.regs = bf3905_QVGA_30fps,
},
#endif
{
	.desc = "bf3905_VGA_30fps",
	.width = 640,
	.height = 480,
	.fps = 30,
	.pix_clk_freq = 12800000,//1/2*Mclk, 25.6M MCLK
	.used = 0,
	.pixels_per_line = 816,//784+16
	.lines_per_frame = 510,//510
	.bin_factor_x = 1,
	.bin_factor_y = 1,
	.bin_mode = 0,
	.skip_frames = 3,
	.regs = bf3905_VGA_30fps,
},

};
#define N_RES_STILL (ARRAY_SIZE(bf3905_res_still))

struct bf3905_resolution bf3905_res_video[] = {
#if (DEBUG==1)
	{
		.desc = "bf3905_QVGA_30fps",
		.width = 336,
		.height = 256,
		.fps = 30,
		.pix_clk_freq = 25.6,//19.2*4/3
		.used = 0,
		.pixels_per_line = 800,//784+16
		.lines_per_frame = 510,//5100
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = bf3905_QVGA_30fps,
	},
#endif
	{
		.desc = "bf3905_VGA_30fps",
		.width = 640,
		.height = 480,
		.fps = 30,
		.pix_clk_freq = 12800000,//1/2*Mclk, 25.6M MCLK
		.used = 0,
		.pixels_per_line = 816,//784+32
		.lines_per_frame = 510,//5100
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = bf3905_VGA_30fps,
	},

};
#define N_RES_VIDEO (ARRAY_SIZE(bf3905_res_video))

static struct bf3905_resolution *bf3905_res = bf3905_res_preview;
static int N_RES = N_RES_PREVIEW;
#endif
