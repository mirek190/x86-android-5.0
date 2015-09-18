/*
 * Support for Himax HM5040 5M camera sensor.
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

#ifndef __HM5040_H__
#define __HM5040_H__
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

#include "vm149.h"

#define HM5040_NAME		"hm5040"

/* Defines for register writes and register array processing */
#define I2C_MSG_LENGTH		0x2
#define I2C_RETRY_COUNT		5

#define HM5040_FOCAL_LENGTH_NUM	334	/*3.34mm*/
#define HM5040_FOCAL_LENGTH_DEM	100
#define HM5040_F_NUMBER_DEFAULT_NUM	24
#define HM5040_F_NUMBER_DEM	10

#define MAX_FMTS		1

#define HM5040_INTEGRATION_TIME_MARGIN	9

/*
 * focal length bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define HM5040_FOCAL_LENGTH_DEFAULT 0x1B70064

/*
 * current f-number bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define HM5040_F_NUMBER_DEFAULT 0x18000a

/*
 * f-number range bits definition:
 * bits 31-24: max f-number numerator
 * bits 23-16: max f-number denominator
 * bits 15-8: min f-number numerator
 * bits 7-0: min f-number denominator
 */
#define HM5040_F_NUMBER_RANGE 0x180a180a
#define HM5040_ID	0x03bb


#define HM5040_BIN_FACTOR_MAX 4
/*
 * HM5040 System control registers
 */
#define HM5040_SW_RESET					0x0103
#define HM5040_SW_STREAM				0x0100

#define HM5040_SC_CMMN_CHIP_ID_H		0x2016
#define HM5040_SC_CMMN_CHIP_ID_L		0x2017

#define HM5040_PRE_PLL_CLK_DIV			0x0305
#define HM5040_VT_PIX_CLK_DIV			0x0301
#define HM5040_VT_SYS_CLK_DIV			0x0303
#define HM5040_PLL_MULTIPLIER			0x0307
#define HM5040_COARSE_INTG_TIME_MIN		0x1004
#define HM5040_COARSE_INTG_TIME_MARGIN	0x1006
#define HM5040_FINE_INTG_TIME_MIN 		0x1008
#define HM5040_FINE_INTG_TIME_MARGIN 	0x100a


#define HM5040_GROUP_ACCESS				0x0104

#define HM5040_EXPOSURE_H				0x0202
#define HM5040_EXPOSURE_L				0x0203
#define HM5040_AGC						0x0205

#define HM5040_HORIZONTAL_START			0x0344
#define HM5040_VERTICAL_START			0x0346
#define HM5040_HORIZONTAL_END			0x0348
#define HM5040_VERTICAL_END				0x034a
#define HM5040_HORIZONTAL_OUTPUT_SIZE	0x034c
#define HM5040_VERTICAL_OUTPUT_SIZE		0x034e

#define HM5040_TIMING_VTS_H				0x0340
#define HM5040_TIMING_VTS_L				0x0341

#define HM5040_DIGITAL_GAIN_GR			0x020e
#define HM5040_DIGITAL_GAIN_R			0x0210
#define HM5040_DIGITAL_GAIN_B			0x0212
#define HM5040_DIGITAL_GAIN_GB			0x0214

#define HM5040_START_STREAMING			0x01
#define HM5040_STOP_STREAMING			0x00

/* Defines for lens/VCM */
#define HM5040_INVALID_CONFIG	0xffffffff
#define HM5040_MAX_FOCUS_POS	1023
#define HM5040_MAX_FOCUS_NEG	(-1023)
#define HM5040_VCM_SLEW_STEP_MAX	0x3f
#define HM5040_VCM_SLEW_TIME_MAX	0x1f

struct regval_list {
	u16 reg_num;
	u8 value;
};

struct hm5040_resolution {
	u8 *desc;
	const struct hm5040_reg *regs;
	int res;
	int width;
	int height;
	int fps;
	int pix_clk_freq;
	u16 pixels_per_line;
	u16 lines_per_frame;
	u8 bin_factor_x;
	u8 bin_factor_y;
	u8 bin_mode;
	bool used;
};

struct hm5040_format {
	u8 *desc;
	u32 pixelformat;
	struct hm5040_reg *regs;
};

struct hm5040_control {
	struct v4l2_queryctrl qc;
	int (*query)(struct v4l2_subdev *sd, s32 *value);
	int (*tweak)(struct v4l2_subdev *sd, s32 value);
};

struct hm5040_vcm {
	int (*power_up)(struct v4l2_subdev *sd);
	int (*power_down)(struct v4l2_subdev *sd);
    int (*init)(struct v4l2_subdev *sd);
    int (*t_focus_vcm)(struct v4l2_subdev *sd, u16 val);
    int (*t_focus_abs)(struct v4l2_subdev *sd, s32 value);
    int (*t_focus_rel)(struct v4l2_subdev *sd, s32 value);
    int (*q_focus_status)(struct v4l2_subdev *sd, s32 *value);
    int (*q_focus_abs)(struct v4l2_subdev *sd, s32 *value);
    int (*t_vcm_slew)(struct v4l2_subdev *sd, s32 value);
    int (*t_vcm_timing)(struct v4l2_subdev *sd, s32 value);
};

/*
 * hm5040 device structure.
 */
struct hm5040_device {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt format;
	struct mutex input_lock;
	struct hm5040_vcm *vcm_driver;

	struct camera_sensor_platform_data *platform_data;
	int vt_pix_clk_freq_mhz;
	int fmt_idx;
	int run_mode;
	u8 res;
	u8 type;
};

enum hm5040_tok_type {
	HM5040_8BIT  = 0x0001,
	HM5040_16BIT = 0x0002,
	HM5040_32BIT = 0x0004,
	HM5040_TOK_TERM   = 0xf000,	/* terminating token for reg list */
	HM5040_TOK_DELAY  = 0xfe00,	/* delay token for reg list */
	HM5040_TOK_MASK = 0xfff0
};

/**
 * struct hm5040_reg - MI sensor  register format
 * @type: type of the register
 * @reg: 16-bit offset to register
 * @val: 8/16/32-bit register value
 *
 * Define a structure for sensor register initialization values
 */
struct hm5040_reg {
	enum hm5040_tok_type type;
	u16 reg;
	u32 val;	/* @set value for read/mod/write, @mask */
};

#define to_hm5040_sensor(x) container_of(x, struct hm5040_device, sd)

#define HM5040_MAX_WRITE_BUF_SIZE	30

struct hm5040_write_buffer {
	u16 addr;
	u8 data[HM5040_MAX_WRITE_BUF_SIZE];
};

struct hm5040_write_ctrl {
	int index;
	struct hm5040_write_buffer buffer;
};

static const struct i2c_device_id hm5040_id[] = {
	{HM5040_NAME, 0},
	{}
};

static struct hm5040_reg const hm5040_global_setting[] = {
	//-------------------------------------
	//	mode select
	//-------------------------------------
	{HM5040_8BIT, 0x0100, 0x00},
	{HM5040_8BIT, 0x3002, 0x32},
	{HM5040_8BIT, 0x3016, 0x46},
	{HM5040_8BIT, 0x3017, 0x29},
	{HM5040_8BIT, 0x3003, 0x03},
	{HM5040_8BIT, 0x3045, 0x03},
	{HM5040_8BIT, 0xFBD7, 0x4C},
	{HM5040_8BIT, 0xFBD8, 0x89},
	{HM5040_8BIT, 0xFBD9, 0x40},
	{HM5040_8BIT, 0xFBDA, 0xE1},
	{HM5040_8BIT, 0xFBDB, 0x4C},
	{HM5040_8BIT, 0xFBDC, 0x73},
	{HM5040_8BIT, 0xFBDD, 0x40},
	{HM5040_8BIT, 0xFBDE, 0xD9},
	{HM5040_8BIT, 0xFBDF, 0x4C},
	{HM5040_8BIT, 0xFBE0, 0x74},
	{HM5040_8BIT, 0xFBE1, 0x40},
	{HM5040_8BIT, 0xFBE2, 0xD9},
	{HM5040_8BIT, 0xFBE3, 0x4C},
	{HM5040_8BIT, 0xFBE4, 0x87},
	{HM5040_8BIT, 0xFBE5, 0x40},
	{HM5040_8BIT, 0xFBE6, 0xD3},
	{HM5040_8BIT, 0xFBE7, 0x4C},
	{HM5040_8BIT, 0xFBE8, 0xB1},
	{HM5040_8BIT, 0xFBE9, 0x40},
	{HM5040_8BIT, 0xFBEA, 0xC7},
	{HM5040_8BIT, 0xFBEB, 0x4C},
	{HM5040_8BIT, 0xFBEC, 0xCC},
	{HM5040_8BIT, 0xFBED, 0x41},
	{HM5040_8BIT, 0xFBEE, 0x13},
	{HM5040_8BIT, 0xFBEF, 0x4C},
	{HM5040_8BIT, 0xFBF0, 0xB9},
	{HM5040_8BIT, 0xFBF1, 0x41},
	{HM5040_8BIT, 0xFBF2, 0x11},
	{HM5040_8BIT, 0xFBF3, 0x4C},
	{HM5040_8BIT, 0xFBF4, 0xB9},
	{HM5040_8BIT, 0xFBF5, 0x41},
	{HM5040_8BIT, 0xFBF6, 0x12},
	{HM5040_8BIT, 0xFBF7, 0x4C},
	{HM5040_8BIT, 0xFBF8, 0xBA},
	{HM5040_8BIT, 0xFBF9, 0x41},
	{HM5040_8BIT, 0xFBFA, 0x40},
	{HM5040_8BIT, 0xFBFB, 0x4C},
	{HM5040_8BIT, 0xFBFC, 0xD3},
	{HM5040_8BIT, 0xFBFD, 0x41},
	{HM5040_8BIT, 0xFBFE, 0x44},
	{HM5040_8BIT, 0xFB00, 0x51},
	{HM5040_8BIT, 0xF800, 0xC0},
	{HM5040_8BIT, 0xF801, 0x24},
	{HM5040_8BIT, 0xF802, 0x7C},
	{HM5040_8BIT, 0xF803, 0xFB},
	{HM5040_8BIT, 0xF804, 0x7D},
	{HM5040_8BIT, 0xF805, 0xC7},
	{HM5040_8BIT, 0xF806, 0x7B},
	{HM5040_8BIT, 0xF807, 0x10},
	{HM5040_8BIT, 0xF808, 0x7F},
	{HM5040_8BIT, 0xF809, 0x72},
	{HM5040_8BIT, 0xF80A, 0x7E},
	{HM5040_8BIT, 0xF80B, 0x30},
	{HM5040_8BIT, 0xF80C, 0x12},
	{HM5040_8BIT, 0xF80D, 0x09},
	{HM5040_8BIT, 0xF80E, 0x47},
	{HM5040_8BIT, 0xF80F, 0xD0},
	{HM5040_8BIT, 0xF810, 0x24},
	{HM5040_8BIT, 0xF811, 0x90},
	{HM5040_8BIT, 0xF812, 0x02},
	{HM5040_8BIT, 0xF813, 0x05},
	{HM5040_8BIT, 0xF814, 0xE0},
	{HM5040_8BIT, 0xF815, 0xF5},
	{HM5040_8BIT, 0xF816, 0x77},
	{HM5040_8BIT, 0xF817, 0xE5},
	{HM5040_8BIT, 0xF818, 0x77},
	{HM5040_8BIT, 0xF819, 0xC3},
	{HM5040_8BIT, 0xF81A, 0x94},
	{HM5040_8BIT, 0xF81B, 0x80},
	{HM5040_8BIT, 0xF81C, 0x50},
	{HM5040_8BIT, 0xF81D, 0x08},
	{HM5040_8BIT, 0xF81E, 0x75},
	{HM5040_8BIT, 0xF81F, 0x7A},
	{HM5040_8BIT, 0xF820, 0xFB},
	{HM5040_8BIT, 0xF821, 0x75},
	{HM5040_8BIT, 0xF822, 0x7B},
	{HM5040_8BIT, 0xF823, 0xD7},
	{HM5040_8BIT, 0xF824, 0x80},
	{HM5040_8BIT, 0xF825, 0x33},
	{HM5040_8BIT, 0xF826, 0xE5},
	{HM5040_8BIT, 0xF827, 0x77},
	{HM5040_8BIT, 0xF828, 0xC3},
	{HM5040_8BIT, 0xF829, 0x94},
	{HM5040_8BIT, 0xF82A, 0xC0},
	{HM5040_8BIT, 0xF82B, 0x50},
	{HM5040_8BIT, 0xF82C, 0x08},
	{HM5040_8BIT, 0xF82D, 0x75},
	{HM5040_8BIT, 0xF82E, 0x7A},
	{HM5040_8BIT, 0xF82F, 0xFB},
	{HM5040_8BIT, 0xF830, 0x75},
	{HM5040_8BIT, 0xF831, 0x7B},
	{HM5040_8BIT, 0xF832, 0xDB},
	{HM5040_8BIT, 0xF833, 0x80},
	{HM5040_8BIT, 0xF834, 0x24},
	{HM5040_8BIT, 0xF835, 0xE5},
	{HM5040_8BIT, 0xF836, 0x77},
	{HM5040_8BIT, 0xF837, 0xC3},
	{HM5040_8BIT, 0xF838, 0x94},
	{HM5040_8BIT, 0xF839, 0xE0},
	{HM5040_8BIT, 0xF83A, 0x50},
	{HM5040_8BIT, 0xF83B, 0x08},
	{HM5040_8BIT, 0xF83C, 0x75},
	{HM5040_8BIT, 0xF83D, 0x7A},
	{HM5040_8BIT, 0xF83E, 0xFB},
	{HM5040_8BIT, 0xF83F, 0x75},
	{HM5040_8BIT, 0xF840, 0x7B},
	{HM5040_8BIT, 0xF841, 0xDF},
	{HM5040_8BIT, 0xF842, 0x80},
	{HM5040_8BIT, 0xF843, 0x15},
	{HM5040_8BIT, 0xF844, 0xE5},
	{HM5040_8BIT, 0xF845, 0x77},
	{HM5040_8BIT, 0xF846, 0xC3},
	{HM5040_8BIT, 0xF847, 0x94},
	{HM5040_8BIT, 0xF848, 0xF0},
	{HM5040_8BIT, 0xF849, 0x50},
	{HM5040_8BIT, 0xF84A, 0x08},
	{HM5040_8BIT, 0xF84B, 0x75},
	{HM5040_8BIT, 0xF84C, 0x7A},
	{HM5040_8BIT, 0xF84D, 0xFB},
	{HM5040_8BIT, 0xF84E, 0x75},
	{HM5040_8BIT, 0xF84F, 0x7B},
	{HM5040_8BIT, 0xF850, 0xE3},
	{HM5040_8BIT, 0xF851, 0x80},
	{HM5040_8BIT, 0xF852, 0x06},
	{HM5040_8BIT, 0xF853, 0x75},
	{HM5040_8BIT, 0xF854, 0x7A},
	{HM5040_8BIT, 0xF855, 0xFB},
	{HM5040_8BIT, 0xF856, 0x75},
	{HM5040_8BIT, 0xF857, 0x7B},
	{HM5040_8BIT, 0xF858, 0xE7},
	{HM5040_8BIT, 0xF859, 0xE5},
	{HM5040_8BIT, 0xF85A, 0x55},
	{HM5040_8BIT, 0xF85B, 0x7F},
	{HM5040_8BIT, 0xF85C, 0x00},
	{HM5040_8BIT, 0xF85D, 0xB4},
	{HM5040_8BIT, 0xF85E, 0x22},
	{HM5040_8BIT, 0xF85F, 0x02},
	{HM5040_8BIT, 0xF860, 0x7F},
	{HM5040_8BIT, 0xF861, 0x01},
	{HM5040_8BIT, 0xF862, 0xE5},
	{HM5040_8BIT, 0xF863, 0x53},
	{HM5040_8BIT, 0xF864, 0x5F},
	{HM5040_8BIT, 0xF865, 0x60},
	{HM5040_8BIT, 0xF866, 0x05},
	{HM5040_8BIT, 0xF867, 0x74},
	{HM5040_8BIT, 0xF868, 0x14},
	{HM5040_8BIT, 0xF869, 0x12},
	{HM5040_8BIT, 0xF86A, 0xFA},
	{HM5040_8BIT, 0xF86B, 0x4C},
	{HM5040_8BIT, 0xF86C, 0x75},
	{HM5040_8BIT, 0xF86D, 0x7C},
	{HM5040_8BIT, 0xF86E, 0xFB},
	{HM5040_8BIT, 0xF86F, 0x75},
	{HM5040_8BIT, 0xF870, 0x7D},
	{HM5040_8BIT, 0xF871, 0xC7},
	{HM5040_8BIT, 0xF872, 0x75},
	{HM5040_8BIT, 0xF873, 0x7E},
	{HM5040_8BIT, 0xF874, 0x30},
	{HM5040_8BIT, 0xF875, 0x75},
	{HM5040_8BIT, 0xF876, 0x7F},
	{HM5040_8BIT, 0xF877, 0x62},
	{HM5040_8BIT, 0xF878, 0xE4},
	{HM5040_8BIT, 0xF879, 0xF5},
	{HM5040_8BIT, 0xF87A, 0x77},
	{HM5040_8BIT, 0xF87B, 0xE5},
	{HM5040_8BIT, 0xF87C, 0x77},
	{HM5040_8BIT, 0xF87D, 0xC3},
	{HM5040_8BIT, 0xF87E, 0x94},
	{HM5040_8BIT, 0xF87F, 0x08},
	{HM5040_8BIT, 0xF880, 0x40},
	{HM5040_8BIT, 0xF881, 0x03},
	{HM5040_8BIT, 0xF882, 0x02},
	{HM5040_8BIT, 0xF883, 0xF9},
	{HM5040_8BIT, 0xF884, 0x0E},
	{HM5040_8BIT, 0xF885, 0x85},
	{HM5040_8BIT, 0xF886, 0x7D},
	{HM5040_8BIT, 0xF887, 0x82},
	{HM5040_8BIT, 0xF888, 0x85},
	{HM5040_8BIT, 0xF889, 0x7C},
	{HM5040_8BIT, 0xF88A, 0x83},
	{HM5040_8BIT, 0xF88B, 0xE0},
	{HM5040_8BIT, 0xF88C, 0xFE},
	{HM5040_8BIT, 0xF88D, 0xA3},
	{HM5040_8BIT, 0xF88E, 0xE0},
	{HM5040_8BIT, 0xF88F, 0xFF},
	{HM5040_8BIT, 0xF890, 0x12},
	{HM5040_8BIT, 0xF891, 0x21},
	{HM5040_8BIT, 0xF892, 0x22},
	{HM5040_8BIT, 0xF893, 0x8E},
	{HM5040_8BIT, 0xF894, 0x78},
	{HM5040_8BIT, 0xF895, 0x8F},
	{HM5040_8BIT, 0xF896, 0x79},
	{HM5040_8BIT, 0xF897, 0x12},
	{HM5040_8BIT, 0xF898, 0xFA},
	{HM5040_8BIT, 0xF899, 0x40},
	{HM5040_8BIT, 0xF89A, 0x12},
	{HM5040_8BIT, 0xF89B, 0x22},
	{HM5040_8BIT, 0xF89C, 0x93},
	{HM5040_8BIT, 0xF89D, 0x50},
	{HM5040_8BIT, 0xF89E, 0x07},
	{HM5040_8BIT, 0xF89F, 0xE4},
	{HM5040_8BIT, 0xF8A0, 0xF5},
	{HM5040_8BIT, 0xF8A1, 0x78},
	{HM5040_8BIT, 0xF8A2, 0xF5},
	{HM5040_8BIT, 0xF8A3, 0x79},
	{HM5040_8BIT, 0xF8A4, 0x80},
	{HM5040_8BIT, 0xF8A5, 0x33},
	{HM5040_8BIT, 0xF8A6, 0x12},
	{HM5040_8BIT, 0xF8A7, 0xFA},
	{HM5040_8BIT, 0xF8A8, 0x40},
	{HM5040_8BIT, 0xF8A9, 0x7B},
	{HM5040_8BIT, 0xF8AA, 0x01},
	{HM5040_8BIT, 0xF8AB, 0xAF},
	{HM5040_8BIT, 0xF8AC, 0x79},
	{HM5040_8BIT, 0xF8AD, 0xAE},
	{HM5040_8BIT, 0xF8AE, 0x78},
	{HM5040_8BIT, 0xF8AF, 0x12},
	{HM5040_8BIT, 0xF8B0, 0x22},
	{HM5040_8BIT, 0xF8B1, 0x4F},
	{HM5040_8BIT, 0xF8B2, 0x74},
	{HM5040_8BIT, 0xF8B3, 0x02},
	{HM5040_8BIT, 0xF8B4, 0x12},
	{HM5040_8BIT, 0xF8B5, 0xFA},
	{HM5040_8BIT, 0xF8B6, 0x4C},
	{HM5040_8BIT, 0xF8B7, 0x85},
	{HM5040_8BIT, 0xF8B8, 0x7B},
	{HM5040_8BIT, 0xF8B9, 0x82},
	{HM5040_8BIT, 0xF8BA, 0xF5},
	{HM5040_8BIT, 0xF8BB, 0x83},
	{HM5040_8BIT, 0xF8BC, 0xE0},
	{HM5040_8BIT, 0xF8BD, 0xFE},
	{HM5040_8BIT, 0xF8BE, 0xA3},
	{HM5040_8BIT, 0xF8BF, 0xE0},
	{HM5040_8BIT, 0xF8C0, 0xFF},
	{HM5040_8BIT, 0xF8C1, 0x7D},
	{HM5040_8BIT, 0xF8C2, 0x03},
	{HM5040_8BIT, 0xF8C3, 0x12},
	{HM5040_8BIT, 0xF8C4, 0x17},
	{HM5040_8BIT, 0xF8C5, 0xD8},
	{HM5040_8BIT, 0xF8C6, 0x12},
	{HM5040_8BIT, 0xF8C7, 0x1B},
	{HM5040_8BIT, 0xF8C8, 0x9B},
	{HM5040_8BIT, 0xF8C9, 0x8E},
	{HM5040_8BIT, 0xF8CA, 0x78},
	{HM5040_8BIT, 0xF8CB, 0x8F},
	{HM5040_8BIT, 0xF8CC, 0x79},
	{HM5040_8BIT, 0xF8CD, 0x74},
	{HM5040_8BIT, 0xF8CE, 0xFE},
	{HM5040_8BIT, 0xF8CF, 0x25},
	{HM5040_8BIT, 0xF8D0, 0x7B},
	{HM5040_8BIT, 0xF8D1, 0xF5},
	{HM5040_8BIT, 0xF8D2, 0x7B},
	{HM5040_8BIT, 0xF8D3, 0x74},
	{HM5040_8BIT, 0xF8D4, 0xFF},
	{HM5040_8BIT, 0xF8D5, 0x35},
	{HM5040_8BIT, 0xF8D6, 0x7A},
	{HM5040_8BIT, 0xF8D7, 0xF5},
	{HM5040_8BIT, 0xF8D8, 0x7A},
	{HM5040_8BIT, 0xF8D9, 0x78},
	{HM5040_8BIT, 0xF8DA, 0x24},
	{HM5040_8BIT, 0xF8DB, 0xE6},
	{HM5040_8BIT, 0xF8DC, 0xFF},
	{HM5040_8BIT, 0xF8DD, 0xC3},
	{HM5040_8BIT, 0xF8DE, 0x74},
	{HM5040_8BIT, 0xF8DF, 0x20},
	{HM5040_8BIT, 0xF8E0, 0x9F},
	{HM5040_8BIT, 0xF8E1, 0x7E},
	{HM5040_8BIT, 0xF8E2, 0x00},
	{HM5040_8BIT, 0xF8E3, 0x25},
	{HM5040_8BIT, 0xF8E4, 0x79},
	{HM5040_8BIT, 0xF8E5, 0xFF},
	{HM5040_8BIT, 0xF8E6, 0xEE},
	{HM5040_8BIT, 0xF8E7, 0x35},
	{HM5040_8BIT, 0xF8E8, 0x78},
	{HM5040_8BIT, 0xF8E9, 0x85},
	{HM5040_8BIT, 0xF8EA, 0x7F},
	{HM5040_8BIT, 0xF8EB, 0x82},
	{HM5040_8BIT, 0xF8EC, 0x85},
	{HM5040_8BIT, 0xF8ED, 0x7E},
	{HM5040_8BIT, 0xF8EE, 0x83},
	{HM5040_8BIT, 0xF8EF, 0xF0},
	{HM5040_8BIT, 0xF8F0, 0xA3},
	{HM5040_8BIT, 0xF8F1, 0xEF},
	{HM5040_8BIT, 0xF8F2, 0xF0},
	{HM5040_8BIT, 0xF8F3, 0x05},
	{HM5040_8BIT, 0xF8F4, 0x77},
	{HM5040_8BIT, 0xF8F5, 0x74},
	{HM5040_8BIT, 0xF8F6, 0x02},
	{HM5040_8BIT, 0xF8F7, 0x25},
	{HM5040_8BIT, 0xF8F8, 0x7D},
	{HM5040_8BIT, 0xF8F9, 0xF5},
	{HM5040_8BIT, 0xF8FA, 0x7D},
	{HM5040_8BIT, 0xF8FB, 0xE4},
	{HM5040_8BIT, 0xF8FC, 0x35},
	{HM5040_8BIT, 0xF8FD, 0x7C},
	{HM5040_8BIT, 0xF8FE, 0xF5},
	{HM5040_8BIT, 0xF8FF, 0x7C},
	{HM5040_8BIT, 0xF900, 0x74},
	{HM5040_8BIT, 0xF901, 0x02},
	{HM5040_8BIT, 0xF902, 0x25},
	{HM5040_8BIT, 0xF903, 0x7F},
	{HM5040_8BIT, 0xF904, 0xF5},
	{HM5040_8BIT, 0xF905, 0x7F},
	{HM5040_8BIT, 0xF906, 0xE4},
	{HM5040_8BIT, 0xF907, 0x35},
	{HM5040_8BIT, 0xF908, 0x7E},
	{HM5040_8BIT, 0xF909, 0xF5},
	{HM5040_8BIT, 0xF90A, 0x7E},
	{HM5040_8BIT, 0xF90B, 0x02},
	{HM5040_8BIT, 0xF90C, 0xF8},
	{HM5040_8BIT, 0xF90D, 0x7B},
	{HM5040_8BIT, 0xF90E, 0x22},
	{HM5040_8BIT, 0xF90F, 0x90},
	{HM5040_8BIT, 0xF910, 0x30},
	{HM5040_8BIT, 0xF911, 0x47},
	{HM5040_8BIT, 0xF912, 0x74},
	{HM5040_8BIT, 0xF913, 0x98},
	{HM5040_8BIT, 0xF914, 0xF0},
	{HM5040_8BIT, 0xF915, 0x90},
	{HM5040_8BIT, 0xF916, 0x30},
	{HM5040_8BIT, 0xF917, 0x36},
	{HM5040_8BIT, 0xF918, 0x74},
	{HM5040_8BIT, 0xF919, 0x1E},
	{HM5040_8BIT, 0xF91A, 0xF0},
	{HM5040_8BIT, 0xF91B, 0x90},
	{HM5040_8BIT, 0xF91C, 0x30},
	{HM5040_8BIT, 0xF91D, 0x42},
	{HM5040_8BIT, 0xF91E, 0x74},
	{HM5040_8BIT, 0xF91F, 0x24},
	{HM5040_8BIT, 0xF920, 0xF0},
	{HM5040_8BIT, 0xF921, 0xE5},
	{HM5040_8BIT, 0xF922, 0x53},
	{HM5040_8BIT, 0xF923, 0x60},
	{HM5040_8BIT, 0xF924, 0x42},
	{HM5040_8BIT, 0xF925, 0x78},
	{HM5040_8BIT, 0xF926, 0x2B},
	{HM5040_8BIT, 0xF927, 0x76},
	{HM5040_8BIT, 0xF928, 0x01},
	{HM5040_8BIT, 0xF929, 0xE5},
	{HM5040_8BIT, 0xF92A, 0x55},
	{HM5040_8BIT, 0xF92B, 0xB4},
	{HM5040_8BIT, 0xF92C, 0x22},
	{HM5040_8BIT, 0xF92D, 0x17},
	{HM5040_8BIT, 0xF92E, 0x90},
	{HM5040_8BIT, 0xF92F, 0x30},
	{HM5040_8BIT, 0xF930, 0x36},
	{HM5040_8BIT, 0xF931, 0x74},
	{HM5040_8BIT, 0xF932, 0x46},
	{HM5040_8BIT, 0xF933, 0xF0},
	{HM5040_8BIT, 0xF934, 0x78},
	{HM5040_8BIT, 0xF935, 0x28},
	{HM5040_8BIT, 0xF936, 0x76},
	{HM5040_8BIT, 0xF937, 0x31},
	{HM5040_8BIT, 0xF938, 0x90},
	{HM5040_8BIT, 0xF939, 0x30},
	{HM5040_8BIT, 0xF93A, 0x0E},
	{HM5040_8BIT, 0xF93B, 0xE0},
	{HM5040_8BIT, 0xF93C, 0xC3},
	{HM5040_8BIT, 0xF93D, 0x13},
	{HM5040_8BIT, 0xF93E, 0x30},
	{HM5040_8BIT, 0xF93F, 0xE0},
	{HM5040_8BIT, 0xF940, 0x04},
	{HM5040_8BIT, 0xF941, 0x78},
	{HM5040_8BIT, 0xF942, 0x26},
	{HM5040_8BIT, 0xF943, 0x76},
	{HM5040_8BIT, 0xF944, 0x40},
	{HM5040_8BIT, 0xF945, 0xE5},
	{HM5040_8BIT, 0xF946, 0x55},
	{HM5040_8BIT, 0xF947, 0xB4},
	{HM5040_8BIT, 0xF948, 0x44},
	{HM5040_8BIT, 0xF949, 0x21},
	{HM5040_8BIT, 0xF94A, 0x90},
	{HM5040_8BIT, 0xF94B, 0x30},
	{HM5040_8BIT, 0xF94C, 0x47},
	{HM5040_8BIT, 0xF94D, 0x74},
	{HM5040_8BIT, 0xF94E, 0x9A},
	{HM5040_8BIT, 0xF94F, 0xF0},
	{HM5040_8BIT, 0xF950, 0x90},
	{HM5040_8BIT, 0xF951, 0x30},
	{HM5040_8BIT, 0xF952, 0x42},
	{HM5040_8BIT, 0xF953, 0x74},
	{HM5040_8BIT, 0xF954, 0x64},
	{HM5040_8BIT, 0xF955, 0xF0},
	{HM5040_8BIT, 0xF956, 0x90},
	{HM5040_8BIT, 0xF957, 0x30},
	{HM5040_8BIT, 0xF958, 0x0E},
	{HM5040_8BIT, 0xF959, 0xE0},
	{HM5040_8BIT, 0xF95A, 0x13},
	{HM5040_8BIT, 0xF95B, 0x13},
	{HM5040_8BIT, 0xF95C, 0x54},
	{HM5040_8BIT, 0xF95D, 0x3F},
	{HM5040_8BIT, 0xF95E, 0x30},
	{HM5040_8BIT, 0xF95F, 0xE0},
	{HM5040_8BIT, 0xF960, 0x0A},
	{HM5040_8BIT, 0xF961, 0x78},
	{HM5040_8BIT, 0xF962, 0x24},
	{HM5040_8BIT, 0xF963, 0xE4},
	{HM5040_8BIT, 0xF964, 0xF6},
	{HM5040_8BIT, 0xF965, 0x80},
	{HM5040_8BIT, 0xF966, 0x04},
	{HM5040_8BIT, 0xF967, 0x78},
	{HM5040_8BIT, 0xF968, 0x2B},
	{HM5040_8BIT, 0xF969, 0xE4},
	{HM5040_8BIT, 0xF96A, 0xF6},
	{HM5040_8BIT, 0xF96B, 0x90},
	{HM5040_8BIT, 0xF96C, 0x30},
	{HM5040_8BIT, 0xF96D, 0x88},
	{HM5040_8BIT, 0xF96E, 0x02},
	{HM5040_8BIT, 0xF96F, 0x1D},
	{HM5040_8BIT, 0xF970, 0x4F},
	{HM5040_8BIT, 0xF971, 0x22},
	{HM5040_8BIT, 0xF972, 0x90},
	{HM5040_8BIT, 0xF973, 0x0C},
	{HM5040_8BIT, 0xF974, 0x1A},
	{HM5040_8BIT, 0xF975, 0xE0},
	{HM5040_8BIT, 0xF976, 0x30},
	{HM5040_8BIT, 0xF977, 0xE2},
	{HM5040_8BIT, 0xF978, 0x18},
	{HM5040_8BIT, 0xF979, 0x90},
	{HM5040_8BIT, 0xF97A, 0x33},
	{HM5040_8BIT, 0xF97B, 0x68},
	{HM5040_8BIT, 0xF97C, 0xE0},
	{HM5040_8BIT, 0xF97D, 0x64},
	{HM5040_8BIT, 0xF97E, 0x05},
	{HM5040_8BIT, 0xF97F, 0x70},
	{HM5040_8BIT, 0xF980, 0x2F},
	{HM5040_8BIT, 0xF981, 0x90},
	{HM5040_8BIT, 0xF982, 0x30},
	{HM5040_8BIT, 0xF983, 0x38},
	{HM5040_8BIT, 0xF984, 0xE0},
	{HM5040_8BIT, 0xF985, 0x70},
	{HM5040_8BIT, 0xF986, 0x02},
	{HM5040_8BIT, 0xF987, 0xA3},
	{HM5040_8BIT, 0xF988, 0xE0},
	{HM5040_8BIT, 0xF989, 0xC3},
	{HM5040_8BIT, 0xF98A, 0x70},
	{HM5040_8BIT, 0xF98B, 0x01},
	{HM5040_8BIT, 0xF98C, 0xD3},
	{HM5040_8BIT, 0xF98D, 0x40},
	{HM5040_8BIT, 0xF98E, 0x21},
	{HM5040_8BIT, 0xF98F, 0x80},
	{HM5040_8BIT, 0xF990, 0x1B},
	{HM5040_8BIT, 0xF991, 0x90},
	{HM5040_8BIT, 0xF992, 0x33},
	{HM5040_8BIT, 0xF993, 0x68},
	{HM5040_8BIT, 0xF994, 0xE0},
	{HM5040_8BIT, 0xF995, 0xB4},
	{HM5040_8BIT, 0xF996, 0x05},
	{HM5040_8BIT, 0xF997, 0x18},
	{HM5040_8BIT, 0xF998, 0xC3},
	{HM5040_8BIT, 0xF999, 0x90},
	{HM5040_8BIT, 0xF99A, 0x30},
	{HM5040_8BIT, 0xF99B, 0x3B},
	{HM5040_8BIT, 0xF99C, 0xE0},
	{HM5040_8BIT, 0xF99D, 0x94},
	{HM5040_8BIT, 0xF99E, 0x0D},
	{HM5040_8BIT, 0xF99F, 0x90},
	{HM5040_8BIT, 0xF9A0, 0x30},
	{HM5040_8BIT, 0xF9A1, 0x3A},
	{HM5040_8BIT, 0xF9A2, 0xE0},
	{HM5040_8BIT, 0xF9A3, 0x94},
	{HM5040_8BIT, 0xF9A4, 0x00},
	{HM5040_8BIT, 0xF9A5, 0x50},
	{HM5040_8BIT, 0xF9A6, 0x02},
	{HM5040_8BIT, 0xF9A7, 0x80},
	{HM5040_8BIT, 0xF9A8, 0x01},
	{HM5040_8BIT, 0xF9A9, 0xC3},
	{HM5040_8BIT, 0xF9AA, 0x40},
	{HM5040_8BIT, 0xF9AB, 0x04},
	{HM5040_8BIT, 0xF9AC, 0x75},
	{HM5040_8BIT, 0xF9AD, 0x10},
	{HM5040_8BIT, 0xF9AE, 0x01},
	{HM5040_8BIT, 0xF9AF, 0x22},
	{HM5040_8BIT, 0xF9B0, 0x02},
	{HM5040_8BIT, 0xF9B1, 0x16},
	{HM5040_8BIT, 0xF9B2, 0xE1},
	{HM5040_8BIT, 0xF9B3, 0x22},
	{HM5040_8BIT, 0xF9B4, 0x90},
	{HM5040_8BIT, 0xF9B5, 0xFF},
	{HM5040_8BIT, 0xF9B6, 0x33},
	{HM5040_8BIT, 0xF9B7, 0xE0},
	{HM5040_8BIT, 0xF9B8, 0x90},
	{HM5040_8BIT, 0xF9B9, 0xFF},
	{HM5040_8BIT, 0xF9BA, 0x34},
	{HM5040_8BIT, 0xF9BB, 0xE0},
	{HM5040_8BIT, 0xF9BC, 0x60},
	{HM5040_8BIT, 0xF9BD, 0x0D},
	{HM5040_8BIT, 0xF9BE, 0x7C},
	{HM5040_8BIT, 0xF9BF, 0xFB},
	{HM5040_8BIT, 0xF9C0, 0x7D},
	{HM5040_8BIT, 0xF9C1, 0xD7},
	{HM5040_8BIT, 0xF9C2, 0x7B},
	{HM5040_8BIT, 0xF9C3, 0x28},
	{HM5040_8BIT, 0xF9C4, 0x7F},
	{HM5040_8BIT, 0xF9C5, 0x34},
	{HM5040_8BIT, 0xF9C6, 0x7E},
	{HM5040_8BIT, 0xF9C7, 0xFF},
	{HM5040_8BIT, 0xF9C8, 0x12},
	{HM5040_8BIT, 0xF9C9, 0x09},
	{HM5040_8BIT, 0xF9CA, 0x47},
	{HM5040_8BIT, 0xF9CB, 0x7F},
	{HM5040_8BIT, 0xF9CC, 0x20},
	{HM5040_8BIT, 0xF9CD, 0x7E},
	{HM5040_8BIT, 0xF9CE, 0x01},
	{HM5040_8BIT, 0xF9CF, 0x7D},
	{HM5040_8BIT, 0xF9D0, 0x00},
	{HM5040_8BIT, 0xF9D1, 0x7C},
	{HM5040_8BIT, 0xF9D2, 0x00},
	{HM5040_8BIT, 0xF9D3, 0x12},
	{HM5040_8BIT, 0xF9D4, 0x12},
	{HM5040_8BIT, 0xF9D5, 0xA4},
	{HM5040_8BIT, 0xF9D6, 0xE4},
	{HM5040_8BIT, 0xF9D7, 0x90},
	{HM5040_8BIT, 0xF9D8, 0x3E},
	{HM5040_8BIT, 0xF9D9, 0x44},
	{HM5040_8BIT, 0xF9DA, 0xF0},
	{HM5040_8BIT, 0xF9DB, 0x02},
	{HM5040_8BIT, 0xF9DC, 0x16},
	{HM5040_8BIT, 0xF9DD, 0x7E},
	{HM5040_8BIT, 0xF9DE, 0x22},
	{HM5040_8BIT, 0xF9DF, 0xE5},
	{HM5040_8BIT, 0xF9E0, 0x44},
	{HM5040_8BIT, 0xF9E1, 0x60},
	{HM5040_8BIT, 0xF9E2, 0x10},
	{HM5040_8BIT, 0xF9E3, 0x90},
	{HM5040_8BIT, 0xF9E4, 0xF6},
	{HM5040_8BIT, 0xF9E5, 0x2C},
	{HM5040_8BIT, 0xF9E6, 0x74},
	{HM5040_8BIT, 0xF9E7, 0x04},
	{HM5040_8BIT, 0xF9E8, 0xF0},
	{HM5040_8BIT, 0xF9E9, 0x90},
	{HM5040_8BIT, 0xF9EA, 0xF6},
	{HM5040_8BIT, 0xF9EB, 0x34},
	{HM5040_8BIT, 0xF9EC, 0xF0},
	{HM5040_8BIT, 0xF9ED, 0x90},
	{HM5040_8BIT, 0xF9EE, 0xF6},
	{HM5040_8BIT, 0xF9EF, 0x3C},
	{HM5040_8BIT, 0xF9F0, 0xF0},
	{HM5040_8BIT, 0xF9F1, 0x80},
	{HM5040_8BIT, 0xF9F2, 0x0E},
	{HM5040_8BIT, 0xF9F3, 0x90},
	{HM5040_8BIT, 0xF9F4, 0xF5},
	{HM5040_8BIT, 0xF9F5, 0xC0},
	{HM5040_8BIT, 0xF9F6, 0x74},
	{HM5040_8BIT, 0xF9F7, 0x04},
	{HM5040_8BIT, 0xF9F8, 0xF0},
	{HM5040_8BIT, 0xF9F9, 0x90},
	{HM5040_8BIT, 0xF9FA, 0xF5},
	{HM5040_8BIT, 0xF9FB, 0xC8},
	{HM5040_8BIT, 0xF9FC, 0xF0},
	{HM5040_8BIT, 0xF9FD, 0x90},
	{HM5040_8BIT, 0xF9FE, 0xF5},
	{HM5040_8BIT, 0xF9FF, 0xD0},
	{HM5040_8BIT, 0xFA00, 0xF0},
	{HM5040_8BIT, 0xFA01, 0x90},
	{HM5040_8BIT, 0xFA02, 0xFB},
	{HM5040_8BIT, 0xFA03, 0x7F},
	{HM5040_8BIT, 0xFA04, 0x02},
	{HM5040_8BIT, 0xFA05, 0x19},
	{HM5040_8BIT, 0xFA06, 0x0B},
	{HM5040_8BIT, 0xFA07, 0x22},
	{HM5040_8BIT, 0xFA08, 0x90},
	{HM5040_8BIT, 0xFA09, 0x0C},
	{HM5040_8BIT, 0xFA0A, 0x1A},
	{HM5040_8BIT, 0xFA0B, 0xE0},
	{HM5040_8BIT, 0xFA0C, 0x20},
	{HM5040_8BIT, 0xFA0D, 0xE2},
	{HM5040_8BIT, 0xFA0E, 0x15},
	{HM5040_8BIT, 0xFA0F, 0xE4},
	{HM5040_8BIT, 0xFA10, 0x90},
	{HM5040_8BIT, 0xFA11, 0x30},
	{HM5040_8BIT, 0xFA12, 0xF8},
	{HM5040_8BIT, 0xFA13, 0xF0},
	{HM5040_8BIT, 0xFA14, 0xA3},
	{HM5040_8BIT, 0xFA15, 0xF0},
	{HM5040_8BIT, 0xFA16, 0x90},
	{HM5040_8BIT, 0xFA17, 0x30},
	{HM5040_8BIT, 0xFA18, 0xF1},
	{HM5040_8BIT, 0xFA19, 0xE0},
	{HM5040_8BIT, 0xFA1A, 0x44},
	{HM5040_8BIT, 0xFA1B, 0x08},
	{HM5040_8BIT, 0xFA1C, 0xF0},
	{HM5040_8BIT, 0xFA1D, 0x90},
	{HM5040_8BIT, 0xFA1E, 0x30},
	{HM5040_8BIT, 0xFA1F, 0xF0},
	{HM5040_8BIT, 0xFA20, 0xE0},
	{HM5040_8BIT, 0xFA21, 0x44},
	{HM5040_8BIT, 0xFA22, 0x08},
	{HM5040_8BIT, 0xFA23, 0xF0},
	{HM5040_8BIT, 0xFA24, 0x02},
	{HM5040_8BIT, 0xFA25, 0x03},
	{HM5040_8BIT, 0xFA26, 0xDE},
	{HM5040_8BIT, 0xFA27, 0x22},
	{HM5040_8BIT, 0xFA28, 0x90},
	{HM5040_8BIT, 0xFA29, 0x0C},
	{HM5040_8BIT, 0xFA2A, 0x1A},
	{HM5040_8BIT, 0xFA2B, 0xE0},
	{HM5040_8BIT, 0xFA2C, 0x30},
	{HM5040_8BIT, 0xFA2D, 0xE2},
	{HM5040_8BIT, 0xFA2E, 0x0D},
	{HM5040_8BIT, 0xFA2F, 0xE0},
	{HM5040_8BIT, 0xFA30, 0x20},
	{HM5040_8BIT, 0xFA31, 0xE0},
	{HM5040_8BIT, 0xFA32, 0x06},
	{HM5040_8BIT, 0xFA33, 0x90},
	{HM5040_8BIT, 0xFA34, 0xFB},
	{HM5040_8BIT, 0xFA35, 0x85},
	{HM5040_8BIT, 0xFA36, 0x74},
	{HM5040_8BIT, 0xFA37, 0x00},
	{HM5040_8BIT, 0xFA38, 0xA5},
	{HM5040_8BIT, 0xFA39, 0x12},
	{HM5040_8BIT, 0xFA3A, 0x16},
	{HM5040_8BIT, 0xFA3B, 0xA0},
	{HM5040_8BIT, 0xFA3C, 0x02},
	{HM5040_8BIT, 0xFA3D, 0x18},
	{HM5040_8BIT, 0xFA3E, 0xAC},
	{HM5040_8BIT, 0xFA3F, 0x22},
	{HM5040_8BIT, 0xFA40, 0x85},
	{HM5040_8BIT, 0xFA41, 0x7B},
	{HM5040_8BIT, 0xFA42, 0x82},
	{HM5040_8BIT, 0xFA43, 0x85},
	{HM5040_8BIT, 0xFA44, 0x7A},
	{HM5040_8BIT, 0xFA45, 0x83},
	{HM5040_8BIT, 0xFA46, 0xE0},
	{HM5040_8BIT, 0xFA47, 0xFC},
	{HM5040_8BIT, 0xFA48, 0xA3},
	{HM5040_8BIT, 0xFA49, 0xE0},
	{HM5040_8BIT, 0xFA4A, 0xFD},
	{HM5040_8BIT, 0xFA4B, 0x22},
	{HM5040_8BIT, 0xFA4C, 0x25},
	{HM5040_8BIT, 0xFA4D, 0x7B},
	{HM5040_8BIT, 0xFA4E, 0xF5},
	{HM5040_8BIT, 0xFA4F, 0x7B},
	{HM5040_8BIT, 0xFA50, 0xE4},
	{HM5040_8BIT, 0xFA51, 0x35},
	{HM5040_8BIT, 0xFA52, 0x7A},
	{HM5040_8BIT, 0xFA53, 0xF5},
	{HM5040_8BIT, 0xFA54, 0x7A},
	{HM5040_8BIT, 0xFA55, 0x22},
	{HM5040_8BIT, 0xFA56, 0xC0},
	{HM5040_8BIT, 0xFA57, 0xD0},
	{HM5040_8BIT, 0xFA58, 0x90},
	{HM5040_8BIT, 0xFA59, 0x35},
	{HM5040_8BIT, 0xFA5A, 0xB5},
	{HM5040_8BIT, 0xFA5B, 0xE0},
	{HM5040_8BIT, 0xFA5C, 0x54},
	{HM5040_8BIT, 0xFA5D, 0xFC},
	{HM5040_8BIT, 0xFA5E, 0x44},
	{HM5040_8BIT, 0xFA5F, 0x01},
	{HM5040_8BIT, 0xFA60, 0xF0},
	{HM5040_8BIT, 0xFA61, 0x12},
	{HM5040_8BIT, 0xFA62, 0x1F},
	{HM5040_8BIT, 0xFA63, 0x5F},
	{HM5040_8BIT, 0xFA64, 0xD0},
	{HM5040_8BIT, 0xFA65, 0xD0},
	{HM5040_8BIT, 0xFA66, 0x02},
	{HM5040_8BIT, 0xFA67, 0x0A},
	{HM5040_8BIT, 0xFA68, 0x16},
	{HM5040_8BIT, 0xFA69, 0x22},
	{HM5040_8BIT, 0xFA6A, 0x90},
	{HM5040_8BIT, 0xFA6B, 0x0C},
	{HM5040_8BIT, 0xFA6C, 0x1A},
	{HM5040_8BIT, 0xFA6D, 0xE0},
	{HM5040_8BIT, 0xFA6E, 0x20},
	{HM5040_8BIT, 0xFA6F, 0xE0},
	{HM5040_8BIT, 0xFA70, 0x06},
	{HM5040_8BIT, 0xFA71, 0x90},
	{HM5040_8BIT, 0xFA72, 0xFB},
	{HM5040_8BIT, 0xFA73, 0x85},
	{HM5040_8BIT, 0xFA74, 0x74},
	{HM5040_8BIT, 0xFA75, 0x00},
	{HM5040_8BIT, 0xFA76, 0xA5},
	{HM5040_8BIT, 0xFA77, 0xE5},
	{HM5040_8BIT, 0xFA78, 0x10},
	{HM5040_8BIT, 0xFA79, 0x02},
	{HM5040_8BIT, 0xFA7A, 0x1E},
	{HM5040_8BIT, 0xFA7B, 0x8F},
	{HM5040_8BIT, 0xFA7C, 0x22},
	{HM5040_8BIT, 0xFA7D, 0x90},
	{HM5040_8BIT, 0xFA7E, 0xFB},
	{HM5040_8BIT, 0xFA7F, 0x85},
	{HM5040_8BIT, 0xFA80, 0x74},
	{HM5040_8BIT, 0xFA81, 0x00},
	{HM5040_8BIT, 0xFA82, 0xA5},
	{HM5040_8BIT, 0xFA83, 0xE5},
	{HM5040_8BIT, 0xFA84, 0x1A},
	{HM5040_8BIT, 0xFA85, 0x60},
	{HM5040_8BIT, 0xFA86, 0x03},
	{HM5040_8BIT, 0xFA87, 0x02},
	{HM5040_8BIT, 0xFA88, 0x17},
	{HM5040_8BIT, 0xFA89, 0x47},
	{HM5040_8BIT, 0xFA8A, 0x22},
	{HM5040_8BIT, 0xFA8B, 0x90},
	{HM5040_8BIT, 0xFA8C, 0xFB},
	{HM5040_8BIT, 0xFA8D, 0x84},
	{HM5040_8BIT, 0xFA8E, 0x02},
	{HM5040_8BIT, 0xFA8F, 0x18},
	{HM5040_8BIT, 0xFA90, 0xD9},
	{HM5040_8BIT, 0xFA91, 0x22},
	{HM5040_8BIT, 0xFA92, 0x02},
	{HM5040_8BIT, 0xFA93, 0x1F},
	{HM5040_8BIT, 0xFA94, 0xB1},
	{HM5040_8BIT, 0xFA95, 0x22},
	{HM5040_8BIT, 0x35D8, 0x01},
	{HM5040_8BIT, 0x35D9, 0x0F},
	{HM5040_8BIT, 0x35DA, 0x01},
	{HM5040_8BIT, 0x35DB, 0x72},
	{HM5040_8BIT, 0x35DC, 0x01},
	{HM5040_8BIT, 0x35DD, 0xB4},
	{HM5040_8BIT, 0x35DE, 0x01},
	{HM5040_8BIT, 0x35DF, 0xDF},
	{HM5040_8BIT, 0x35E0, 0x02},
	{HM5040_8BIT, 0x35E1, 0x08},
	{HM5040_8BIT, 0x35E2, 0x02},
	{HM5040_8BIT, 0x35E3, 0x28},
	{HM5040_8BIT, 0x35E4, 0x02},
	{HM5040_8BIT, 0x35E5, 0x56},
	{HM5040_8BIT, 0x35E6, 0x02},
	{HM5040_8BIT, 0x35E7, 0x6A},
	{HM5040_8BIT, 0x35E8, 0x02},
	{HM5040_8BIT, 0x35E9, 0x7D},
	{HM5040_8BIT, 0x35EA, 0x02},
	{HM5040_8BIT, 0x35EB, 0x8B},
	{HM5040_8BIT, 0x35EC, 0x02},
	{HM5040_8BIT, 0x35ED, 0x92},
	{HM5040_8BIT, 0x35EF, 0x22},
	{HM5040_8BIT, 0x35F1, 0x23},
	{HM5040_8BIT, 0x35F3, 0x22},
	{HM5040_8BIT, 0x35F6, 0x19},
	{HM5040_8BIT, 0x35F7, 0x55},
	{HM5040_8BIT, 0x35F8, 0x1D},
	{HM5040_8BIT, 0x35F9, 0x4C},
	{HM5040_8BIT, 0x35FA, 0x16},
	{HM5040_8BIT, 0x35FB, 0xC7},
	{HM5040_8BIT, 0x35FC, 0x1A},
	{HM5040_8BIT, 0x35FD, 0xA0},
	{HM5040_8BIT, 0x35FE, 0x18},
	{HM5040_8BIT, 0x35FF, 0xD6},
	{HM5040_8BIT, 0x3600, 0x03},
	{HM5040_8BIT, 0x3601, 0xD4},
	{HM5040_8BIT, 0x3602, 0x18},
	{HM5040_8BIT, 0x3603, 0x8A},
	{HM5040_8BIT, 0x3604, 0x0A},
	{HM5040_8BIT, 0x3605, 0x0D},
	{HM5040_8BIT, 0x3606, 0x1E},
	{HM5040_8BIT, 0x3607, 0x8D},
	{HM5040_8BIT, 0x3608, 0x17},
	{HM5040_8BIT, 0x3609, 0x43},
	{HM5040_8BIT, 0x360A, 0x19},
	{HM5040_8BIT, 0x360B, 0x16},
	{HM5040_8BIT, 0x360C, 0x1F},
	{HM5040_8BIT, 0x360D, 0xAD},
	{HM5040_8BIT, 0x360E, 0x19},
	{HM5040_8BIT, 0x360F, 0x08},
	{HM5040_8BIT, 0x3610, 0x14},
	{HM5040_8BIT, 0x3611, 0x26},
	{HM5040_8BIT, 0x3612, 0x1A},
	{HM5040_8BIT, 0x3613, 0xB3},
	{HM5040_8BIT, 0x35D2, 0x7F},
	{HM5040_8BIT, 0x35D3, 0xFF},
	{HM5040_8BIT, 0x35D4, 0x70},
	{HM5040_8BIT, 0x35D0, 0x01},
	{HM5040_8BIT, 0x3E44, 0x01},
	{HM5040_8BIT, 0x3570, 0x01},

	//-------------------------------------
	//	CSI mode
	//-------------------------------------
	{HM5040_8BIT, 0x0111, 0x02},
	{HM5040_8BIT, 0x0114, 0x01},	
	{HM5040_8BIT, 0x2136, 0x0C},
	{HM5040_8BIT, 0x2137, 0x00},

	//-------------------------------------
	//	CSI data format
	//-------------------------------------
	{HM5040_8BIT, 0x0112, 0x0A},
	{HM5040_8BIT, 0x0113, 0x0A},	
	{HM5040_8BIT, 0x3016, 0x46},
	{HM5040_8BIT, 0x3017, 0x29},
	{HM5040_8BIT, 0x3003, 0x03},
	{HM5040_8BIT, 0x3045, 0x03},
	{HM5040_8BIT, 0x3047, 0x98},

	//-------------------------------------
	//	PLL
	//-------------------------------------
	{HM5040_8BIT, 0x0305, 0x02},
	{HM5040_8BIT, 0x0306, 0x00},
	{HM5040_8BIT, 0x0307, 0xAB},
	{HM5040_8BIT, 0x0301, 0x0A},
	{HM5040_8BIT, 0x0309, 0x0A},

	//-------------------------------------
	//	frame length
	//-------------------------------------
	{HM5040_8BIT, 0x0340, 0x07},
	{HM5040_8BIT, 0x0341, 0xC4},

	//-------------------------------------
	//	address increment for odd pixel 
	//-------------------------------------
	{HM5040_8BIT, 0x0383, 0x01},
	{HM5040_8BIT, 0x0387, 0x01},

	//-------------------------------------
	//	integration time
	//-------------------------------------
	{HM5040_8BIT, 0x0202, 0x08},
	{HM5040_8BIT, 0x0203, 0x00},

	//-------------------------------------
	//	Gain
	//-------------------------------------
	{HM5040_8BIT, 0x0205, 0xC0},

	//-------------------------------------
	//	Binning
	//-------------------------------------
	{HM5040_8BIT, 0x0900, 0x00},
	{HM5040_8BIT, 0x0901, 0x00},
	{HM5040_8BIT, 0x0902, 0x00},
	
	//-------------------------------------
	//	Crop image
	//-------------------------------------
	{HM5040_8BIT, 0x040C, 0x0A},
	{HM5040_8BIT, 0x040D, 0x28},
	{HM5040_8BIT, 0x040E, 0x07},
	{HM5040_8BIT, 0x040F, 0xA0},
	{HM5040_TOK_TERM, 0, 0}
};

static struct hm5040_reg const hm5040_1648x1096_30fps[] = {
	//-------------------------------------
	//	start_end address
	//-------------------------------------
	{HM5040_8BIT, 0x0344, 0x01},
	{HM5040_8BIT, 0x0345, 0xDC},	//x_start-476
	{HM5040_8BIT, 0x0346, 0x01},
	{HM5040_8BIT, 0x0347, 0xAC},	//y_start-428
	{HM5040_8BIT, 0x0348, 0x08},
	{HM5040_8BIT, 0x0349, 0x4B},	//x_end-2123
	{HM5040_8BIT, 0x034A, 0x05},
	{HM5040_8BIT, 0x034B, 0xF3},	//y_end-1523

	//-------------------------------------
	//	Data output size
	//-------------------------------------
	{HM5040_8BIT, 0x034C, 0x06},
	{HM5040_8BIT, 0x034D, 0x70},	//x_output_size -1648
	{HM5040_8BIT, 0x034E, 0x04},
	{HM5040_8BIT, 0x034F, 0x48},	//Y_output_size -1096

	//-------------------------------------
	//	digital_crop_offset
	//-------------------------------------
	{HM5040_8BIT, 0x0408, 0x00},
	{HM5040_8BIT, 0x0409, 0x00},	//X_offset_0
	{HM5040_8BIT, 0x040A, 0x00},
	{HM5040_8BIT, 0x040B, 0x00},	//Y_offset_0

	//-------------------------------------
	//	Crop image
	//-------------------------------------
	{HM5040_8BIT, 0x040C, 0x0A},
	{HM5040_8BIT, 0x040D, 0x28},	//X_output_size -2600
	{HM5040_8BIT, 0x040E, 0x07},
	{HM5040_8BIT, 0x040F, 0xA0},	//Y_output_size -1952

	//-------------------------------------
	//	mode select
	//-------------------------------------
	{HM5040_8BIT, 0x0100, 0x01},
	{HM5040_TOK_TERM, 0, 0}
};
#if 0
static struct hm5040_reg const hm5040_1636x1096_30fps[] = {
	//-------------------------------------
	//	start_end address
	//-------------------------------------
	{HM5040_8BIT, 0x0344, 0x00},
	{HM5040_8BIT, 0x0345, 0x00},	//x_start-0
	{HM5040_8BIT, 0x0346, 0x00},
	{HM5040_8BIT, 0x0347, 0x00},	//y_start-0
	{HM5040_8BIT, 0x0348, 0x0A},
	{HM5040_8BIT, 0x0349, 0x27},	//x_end-2599
	{HM5040_8BIT, 0x034A, 0x07},
	{HM5040_8BIT, 0x034B, 0x9F},	//y_end-1951

	//-------------------------------------
	//	Data output size
	//-------------------------------------
	{HM5040_8BIT, 0x034C, 0x0A},
	{HM5040_8BIT, 0x034D, 0x28},	//x_output_size -2600
	{HM5040_8BIT, 0x034E, 0x07},
	{HM5040_8BIT, 0x034F, 0xA0},	//Y_output_size -1952

	//-------------------------------------
	//	digital_crop_offset
	//-------------------------------------
	{HM5040_8BIT, 0x0408, 0x01},
	{HM5040_8BIT, 0x0409, 0xE2},	//X_offset_482
	{HM5040_8BIT, 0x040A, 0x01},
	{HM5040_8BIT, 0x040B, 0xAC},	//Y_offset_428

	//-------------------------------------
	//	Crop image
	//-------------------------------------
	{HM5040_8BIT, 0x040C, 0x06},
	{HM5040_8BIT, 0x040D, 0x64},	//X_output_size -1636
	{HM5040_8BIT, 0x040E, 0x04},
	{HM5040_8BIT, 0x040F, 0x48},	//Y_output_size -1096

	//-------------------------------------
	//	mode select
	//-------------------------------------
	{HM5040_8BIT, 0x0100, 0x01},
	{HM5040_TOK_TERM, 0, 0}
};
#endif
static struct hm5040_reg const hm5040_1936x1096_30fps[] = {
	//-------------------------------------
	//	start_end address
	//-------------------------------------
	{HM5040_8BIT, 0x0344, 0x01},
	{HM5040_8BIT, 0x0345, 0x4C},	//x_start-332
	{HM5040_8BIT, 0x0346, 0x01},
	{HM5040_8BIT, 0x0347, 0xAC},	//y_start-428
	{HM5040_8BIT, 0x0348, 0x08},
	{HM5040_8BIT, 0x0349, 0xDB},	//x_end-2267
	{HM5040_8BIT, 0x034A, 0x05},
	{HM5040_8BIT, 0x034B, 0xF3},	//y_end-1523

	//-------------------------------------
	//	Data output size
	//-------------------------------------
	{HM5040_8BIT, 0x034C, 0x07},
	{HM5040_8BIT, 0x034D, 0x90},	//x_output_size -1936
	{HM5040_8BIT, 0x034E, 0x04},
	{HM5040_8BIT, 0x034F, 0x48},	//Y_output_size -1096

	//-------------------------------------
	//	digital_crop_offset
	//-------------------------------------
	{HM5040_8BIT, 0x0408, 0x00},
	{HM5040_8BIT, 0x0409, 0x00},	//X_offset_0
	{HM5040_8BIT, 0x040A, 0x00},
	{HM5040_8BIT, 0x040B, 0x00},	//Y_offset_0

	//-------------------------------------
	//	Crop image
	//-------------------------------------
	{HM5040_8BIT, 0x040C, 0x0A},
	{HM5040_8BIT, 0x040D, 0x28},	//X_output_size -2600
	{HM5040_8BIT, 0x040E, 0x07},
	{HM5040_8BIT, 0x040F, 0xA0},	//Y_output_size -1952

	//-------------------------------------
	//	mode select
	//-------------------------------------
	{HM5040_8BIT, 0x0100, 0x01},
	{HM5040_TOK_TERM, 0, 0}
};

static struct hm5040_reg const hm5040_2592x1944_30fps[] = {
	//-------------------------------------
	//	start_end address
	//-------------------------------------
	{HM5040_8BIT, 0x0344, 0x00},
	{HM5040_8BIT, 0x0345, 0x04},	//x_start-4
	{HM5040_8BIT, 0x0346, 0x00},
	{HM5040_8BIT, 0x0347, 0x04},	//y_start-4
	{HM5040_8BIT, 0x0348, 0x0A},
	{HM5040_8BIT, 0x0349, 0x23},	//x_end-2595
	{HM5040_8BIT, 0x034A, 0x07},
	{HM5040_8BIT, 0x034B, 0x9B},	//y_end-1947

	//-------------------------------------
	//	Data output size
	//-------------------------------------
	{HM5040_8BIT, 0x034C, 0x0A},
	{HM5040_8BIT, 0x034D, 0x20},	//x_output_size -2592
	{HM5040_8BIT, 0x034E, 0x07},
	{HM5040_8BIT, 0x034F, 0x98},	//Y_output_size -1944

	//-------------------------------------
	//	digital_crop_offset
	//-------------------------------------
	{HM5040_8BIT, 0x0408, 0x00},
	{HM5040_8BIT, 0x0409, 0x00},	//X_offset_0
	{HM5040_8BIT, 0x040A, 0x00},
	{HM5040_8BIT, 0x040B, 0x00},	//Y_offset_0

	//-------------------------------------
	//	Crop image
	//-------------------------------------
	{HM5040_8BIT, 0x040C, 0x0A},
	{HM5040_8BIT, 0x040D, 0x28},	//X_output_size -2600
	{HM5040_8BIT, 0x040E, 0x07},
	{HM5040_8BIT, 0x040F, 0xA0},	//Y_output_size -1952

	//-------------------------------------
	//	mode select
	//-------------------------------------
	{HM5040_8BIT, 0x0100, 0x01},
	{HM5040_TOK_TERM, 0, 0}
};

struct hm5040_resolution hm5040_res_preview[] = {
	{
		.desc = "hm5040_5M_30fps",
		.width = 2592,
		.height = 1944,
		.fps = 30,
		.used = 0,
		.pixels_per_line = 2750,
		.lines_per_frame = 1988,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.regs = hm5040_2592x1944_30fps,
	},
};
#define N_RES_PREVIEW (ARRAY_SIZE(hm5040_res_preview))

struct hm5040_resolution hm5040_res_still[] = {
	{
		.desc = "hm5040_5M_30fps",
		.width = 2592,
		.height = 1944,
		.fps = 30,
		.used = 0,
		.pixels_per_line = 2750,
		.lines_per_frame = 1988,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.regs = hm5040_2592x1944_30fps,
	},
};
#define N_RES_STILL (ARRAY_SIZE(hm5040_res_still))

struct hm5040_resolution hm5040_res_video[] = {
	{
		.desc = "hm5040_1648x1096_30fps",
		.width = 1648,
		.height = 1096,
		.fps = 30,
		.used = 0,
		.pixels_per_line = 2750,
		.lines_per_frame = 1988,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.regs = hm5040_1648x1096_30fps,
	},
#if 0
	{
		.desc = "hm5040_1636x1096_30fps",
		.width = 1636,
		.height = 1096,
		.fps = 30,
		.used = 0,
		.pixels_per_line = 2750,
		.lines_per_frame = 1988,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.regs = hm5040_1636x1096_30fps,
	},
#endif
	{
		.desc = "hm5040_1936x1096_30fps",
		.width = 1936,
		.height = 1096,
		.fps = 30,
		.used = 0,
		.pixels_per_line = 2750,
		.lines_per_frame = 1988,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.regs = hm5040_1936x1096_30fps,
	},
};
#define N_RES_VIDEO (ARRAY_SIZE(hm5040_res_video))

static struct hm5040_resolution *hm5040_res = hm5040_res_preview;
static int N_RES = N_RES_PREVIEW;

extern int vcm_power_up(struct v4l2_subdev *sd);
extern int vcm_power_down(struct v4l2_subdev *sd);

extern int vm149_vcm_power_up(struct v4l2_subdev *sd);
extern int vm149_vcm_power_down(struct v4l2_subdev *sd);
extern int vm149_vcm_init(struct v4l2_subdev *sd);
extern int vm149_t_focus_vcm(struct v4l2_subdev *sd, u16 val);
extern int vm149_t_focus_abs(struct v4l2_subdev *sd, s32 value);
extern int vm149_t_focus_rel(struct v4l2_subdev *sd, s32 value);
extern int vm149_q_focus_status(struct v4l2_subdev *sd, s32 *value);
extern int vm149_q_focus_abs(struct v4l2_subdev *sd, s32 *value);
extern int vm149_t_vcm_slew(struct v4l2_subdev *sd, s32 value);
extern int vm149_t_vcm_timing(struct v4l2_subdev *sd, s32 value);

struct hm5040_vcm hm5040_vcm_ops = {
    .power_up = vm149_vcm_power_up,
    .power_down = vm149_vcm_power_down,
    .init = vm149_vcm_init,
    .t_focus_vcm = vm149_t_focus_vcm,
    .t_focus_abs = vm149_t_focus_abs,
    .t_focus_rel = vm149_t_focus_rel,
    .q_focus_status = vm149_q_focus_status,
    .q_focus_abs = vm149_q_focus_abs,
    .t_vcm_slew = vm149_t_vcm_slew,
    .t_vcm_timing = vm149_t_vcm_timing,
};


#endif
