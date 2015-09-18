/*
 * Support for Sony GC5004 camera sensor.
 *
 * Copyright (c) 2010 Intel Corporation. All Rights Reserved.
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

#ifndef __GC5004_H__
#define __GC5004_H__
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

#define I2C_MSG_LENGTH		0x2

#define GC5004_MCLK		192

/* TODO - This should be added into include/linux/videodev2.h */
#ifndef V4L2_IDENT_GC
#define V4L2_IDENT_GC	2235
#endif

/* #defines for register writes and register array processing */
#define GCSENSOR_8BIT		1//fix me, it is right?
#define GCSENSOR_16BIT		2
#define GCSENSOR_32BIT		4

/*
 * gc5004 System control registers
 */
#define GC5004_MASK_5BIT	0x1F
#define GC5004_MASK_4BIT	0xF
#define GC5004_MASK_2BIT	0x3
#define GC5004_MASK_11BIT	0x7FF
#define GC5004_INTG_BUF_COUNT		2

#define GC5004_FINE_INTG_TIME		0x0

#define GC5004_ID_DEFAULT 0x5004

#define GC5004_VT_PIX_CLK_DIV			0x0301
#define GC5004_VT_SYS_CLK_DIV			0x0303
#define GC5004_PRE_PLL_CLK_DIV			0x0305
#define GC5004_PLL_MULTIPLIER			0x030C
#define GC5004_PLL_MULTIPLIER			0x030C
#define GC5004_OP_PIX_DIV			0x0309
#define GC5004_OP_SYS_DIV			0x030B
#define GC5004_LINES_PER_FRAME		0x0d
#define GC5004_PIXELS_PER_LINE		0x0f
#define GC5004_COARSE_INTG_TIME_MIN	0x1004
#define GC5004_COARSE_INTG_TIME_MAX	0x1006
#define GC5004_BINNING_ENABLE		0x0390
#define GC5004_BINNING_TYPE		0x0391

//#define GC5004_READ_MODE				0x0390

#define REG_HORI_BLANKING_H 0x05
#define REG_HORI_BLANKING_L 0x06
#define REG_VERT_DUMMY_H 0x07
#define REG_VERT_DUMMY_L 0x08

#define GC5004_HORIZONTAL_START_H 0x0b
#define GC5004_HORIZONTAL_START_L 0x0c
#define GC5004_VERTICAL_START_H 0x09
#define GC5004_VERTICAL_START_L 0x0a
#define REG_SH_DELAY_L 0x12
#define REG_SH_DELAY_H 0x11


#define GC5004_HORIZONTAL_OUTPUT_SIZE_H 0x0f
#define GC5004_HORIZONTAL_OUTPUT_SIZE_L 0x10


#define GC5004_VERTICAL_OUTPUT_SIZE_H 0x0d
#define GC5004_VERTICAL_OUTPUT_SIZE_L 0x0e

#define GC5004_COARSE_INTEGRATION_TIME		0x0202
#define GC5004_TEST_PATTERN_MODE			0x0600

#define GC5004_IMG_ORIENTATION			0x17
#define GC5004_VFLIP_BIT			2
#define GC5004_HFLIP_BIT			1
#define GC5004_GLOBAL_GAIN			0xb0
#define ANALOG_GAIN_1 64  // 1.00x
#define ANALOG_GAIN_2 90  // 1.41x
#define ANALOG_GAIN_3 128  // 2.00x
#define ANALOG_GAIN_4 178  // 2.78x
#define ANALOG_GAIN_5 247  // 3.85x
#define ANALOG_GAIN_6 332  // 5.18x
#define ANALOG_GAIN_7 435  // 6.80x


#define GC5004_SHORT_AGC_GAIN		0x0233
#define GC5004_DGC_ADJ		0x020E
#define GC5004_DGC_LEN		10
#define GC5004_MAX_EXPOSURE_SUPPORTED 8191
#define GC5004_MAX_GLOBAL_GAIN_SUPPORTED 0x3ffff
#define GC5004_MAX_DIGITAL_GAIN_SUPPORTED 0x3ffff
#define GC5004_REG_EXPO_COARSE                 0x03

/* Defines for register writes and register array processing */
#define GC5004_BYTE_MAX	32 /* change to 32 as needed by otpdata */
#define GC5004_SHORT_MAX	16
#define I2C_RETRY_COUNT		5
#define GC5004_TOK_MASK	0xfff0


#define GC5004_FOCAL_LENGTH_NUM	208	/*2.08mm*/
#define GC5004_FOCAL_LENGTH_DEM	100
#define GC5004_F_NUMBER_DEFAULT_NUM	24
#define GC5004_F_NUMBER_DEM	10
#define GC5004_WAIT_STAT_TIMEOUT	100
#define GC5004_FLICKER_MODE_50HZ	1
#define GC5004_FLICKER_MODE_60HZ	2


/* Defines for OTP Data Registers */
#define GC5004_OTP_START_ADDR		0x3B04
#define GC5004_OTP_DATA_SIZE		1280
#define GC5004_OTP_PAGE_SIZE		64
#define GC5004_OTP_READY_REG		0x3B01
#define GC5004_OTP_PAGE_REG		0x3B02
#define GC5004_OTP_MODE_REG		0x3B00
#define GC5004_OTP_PAGE_MAX		20
#define GC5004_OTP_READY_REG_DONE		1
#define GC5004_OTP_READ_ONETIME		32
#define GC5004_OTP_MODE_READ		1

#define MAX_FMTS 1

#define GC5004_SUBDEV_PREFIX "gc"
#define GC5004_DRIVER	"gc5004"
#define GC5004_NAME	"gc5004"
#define GC5004_ID	0x5004

/* gc5004175 - use dw9714 vcm */
#define GC5004_MERRFLD 0x175
#define GC5004_VALLEYVIEW 0x176
#define GC5004_SALTBAY 0x135
#define GC5004_VICTORIABAY 0x136

#define GC5004_BAYTRAIL 0x1



#define GC5004_I2C_OPEN_EN  0xf3

#define GC5004_REG_SENSOR_ID_HIGH_BIT	0xf0
#define GC5004_REG_SENSOR_ID_LOW_BIT	0xf1

#define GC5004_RES_WIDTH_MAX	2592
#define GC5004_RES_HEIGHT_MAX	1944

#define GC5004_BIN_FACTOR_MAX			4
#define GC5004_INTEGRATION_TIME_MARGIN	4
/*
 * focal length bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define GC5004_FOCAL_LENGTH_DEFAULT 0x1710064

/*
 * current f-number bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define GC5004_F_NUMBER_DEFAULT 0x16000a

/*
 * f-number range bits definition:
 * bits 31-24: max f-number numerator
 * bits 23-16: max f-number denominator
 * bits 15-8: min f-number numerator
 * bits 7-0: min f-number denominator
 */
#define GC5004_F_NUMBER_RANGE 0x160a160a


#define GC5004_MAX_FOCUS_POS 255
#define GC5004_MAX_FOCUS_NEG (-255)
#define GC5004_VCM_SLEW_TIME_MAX		0xffff
#define GC5004_VCM_SLEW_STEP_MAX                0x7


struct gc5004_vcm {
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

struct max_res {
	int res_max_width;
	int res_max_height;
};

struct max_res gc5004_max_res[] = {
	[0] = {
		.res_max_width = GC5004_RES_WIDTH_MAX,
		.res_max_height = GC5004_RES_HEIGHT_MAX,
	},
};

#define MAX_FPS_OPTIONS_SUPPORTED       3

enum gc5004_tok_type {
        GC5004_8BIT  = 0x0001,
        GC5004_16BIT = 0x0002,
        GC5004_TOK_TERM   = 0xf000,        /* terminating token for reg list */
        GC5004_TOK_DELAY  = 0xfe00 /* delay token for reg list */
};

#define GROUPED_PARAMETER_HOLD_ENABLE  {GC5004_8BIT, 0x0104, 0x1}
#define GROUPED_PARAMETER_HOLD_DISABLE  {GC5004_8BIT, 0x0104, 0x0}

/**
 * struct gc5004_reg - MI sensor  register format
 * @type: type of the register
 * @reg: 16-bit offset to register
 * @val: 8/16/32-bit register value
 *
 * Define a structure for sensor register initialization values
 */
struct gc5004_reg {
        enum gc5004_tok_type type;
        u16 sreg;
        u32 val;        /* @set value for read/mod/write, @mask */
};

struct gc5004_resolution {
        u8 *desc;
        const struct gc5004_reg *regs;
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
};



struct gc5004_settings {
	struct gc5004_reg const *init_settings;
	struct gc5004_resolution *res_preview;
	struct gc5004_resolution *res_still;
	struct gc5004_resolution *res_video;
	int n_res_preview;
	int n_res_still;
	int n_res_video;
};

static struct gc5004_reg const gc5004_1296x976_30fps[] = {
	//////////1296x976//////////////////////	
	{GC5004_8BIT,0x18, 0x42},//skip on
	{GC5004_8BIT,0x80, 0x18},//scaler en
	{GC5004_8BIT,0x05, 0x01},
	{GC5004_8BIT,0x06, 0xfa}, //row start 506

	{GC5004_8BIT,0x09, 0x00},
	{GC5004_8BIT,0x0a, 0x02}, //row start 240
	{GC5004_8BIT,0x0b, 0x00},
	{GC5004_8BIT,0x0c, 0x06}, //col start
	{GC5004_8BIT,0x0d, 0x07}, //1960 Window setting
	{GC5004_8BIT,0x0e, 0xa8}, 
	{GC5004_8BIT,0x0f, 0x0a}, //2640
	{GC5004_8BIT,0x10, 0x50},

	{GC5004_8BIT,0x4e, 0x00}, //add by travis 20140625
	{GC5004_8BIT,0x4f, 0x06},

	//{GC5004_8BIT,0x17, 0x35}, 

	{GC5004_8BIT, 0x92, 0x00}, //00/crop win y
	{GC5004_8BIT, 0x94, 0x00}, //04/crop win x  0d

	{GC5004_8BIT,0x95, 0x03},//976
	{GC5004_8BIT,0x96, 0xd0},
	{GC5004_8BIT,0x97, 0x05},//1296
	{GC5004_8BIT,0x98, 0x10},

	{GC5004_8BIT,0xfe, 0x03},
	{GC5004_8BIT,0x04, 0x40},//fifo
	{GC5004_8BIT,0x05, 0x01},
	{GC5004_8BIT,0x12, 0x54},//win_width/8*10
	{GC5004_8BIT,0x13, 0x06},
	{GC5004_8BIT,0x42, 0x10},//buf
	{GC5004_8BIT,0x43, 0x05},
	{GC5004_8BIT,0xfe, 0x00},

	{GC5004_TOK_TERM,0 ,0},
};

static struct gc5004_reg const gc5004_1296x736_30fps[] = {
	/*/////////1296*736/////////////////////*/
	{GC5004_8BIT,0x18, 0x42},//skip on
	{GC5004_8BIT,0x80, 0x18},//scaler en

	{GC5004_8BIT,0x05, 0x01},	
	{GC5004_8BIT,0x06, 0xfa}, //HB 506

	{GC5004_8BIT,0x09, 0x00},	
	{GC5004_8BIT,0x0a, 0xdc}, //row start 220
	{GC5004_8BIT,0x0b, 0x00},
	{GC5004_8BIT,0x0c, 0x00}, //col start
	{GC5004_8BIT,0x0d, 0x05}, //1480  Window setting
	{GC5004_8BIT,0x0e, 0xc8}, 
	{GC5004_8BIT,0x0f, 0x0a}, //2640
	{GC5004_8BIT,0x10, 0x50}, 

	{GC5004_8BIT,0x4e, 0x00}, //add by travis 20140625
	{GC5004_8BIT,0x4f, 0x06},

	{GC5004_8BIT, 0x92, 0x00}, //00/crop win y
	{GC5004_8BIT, 0x94, 0x00}, //04/crop win x  0d

	{GC5004_8BIT,0x95, 0x02},//736
	{GC5004_8BIT,0x96, 0xe0},
	{GC5004_8BIT,0x97, 0x05},//1296
	{GC5004_8BIT,0x98, 0x10},

	{GC5004_8BIT,0xfe, 0x03},
	{GC5004_8BIT,0x04, 0x40},//fifo
	{GC5004_8BIT,0x05, 0x01},
	{GC5004_8BIT,0x12, 0x54},//win_width/8*10
	{GC5004_8BIT,0x13, 0x06},
	{GC5004_8BIT,0x42, 0x10},//buf
	{GC5004_8BIT,0x43, 0x05},
	{GC5004_8BIT,0xfe, 0x00},
	{GC5004_TOK_TERM,0 ,0},
};

static struct gc5004_reg const gc5004_1296x864_30fps[] = {
	//////////1296*864//////////////////////
	{GC5004_8BIT,0x18, 0x42},//skip on
	{GC5004_8BIT,0x80, 0x18},//scaler en

	{GC5004_8BIT,0x05, 0x01},   
	{GC5004_8BIT,0x06, 0xfa}, //HB 506

	{GC5004_8BIT,0x09, 0x00},   
	{GC5004_8BIT,0x0a, 0x70}, //row start 112
	{GC5004_8BIT,0x0b, 0x00},
	{GC5004_8BIT,0x0c, 0x00}, //col start
	{GC5004_8BIT,0x0d, 0x06}, //1736 Window setting
	{GC5004_8BIT,0x0e, 0xc8}, 
	{GC5004_8BIT,0x0f, 0x0a}, //2640
	{GC5004_8BIT,0x10, 0x50}, 

	{GC5004_8BIT,0x4e, 0x00}, //add by travis 20140625
	{GC5004_8BIT,0x4f, 0x06},

	{GC5004_8BIT, 0x92, 0x00}, //00/crop win y
	{GC5004_8BIT, 0x94, 0x00}, //04/crop win x  0d
	
	{GC5004_8BIT,0x95, 0x03},//864
	{GC5004_8BIT,0x96, 0x60},
	{GC5004_8BIT,0x97, 0x05},//1296
	{GC5004_8BIT,0x98, 0x10},

	{GC5004_8BIT,0xfe, 0x03},
	{GC5004_8BIT,0x04, 0x40},//fifo
	{GC5004_8BIT,0x05, 0x01},
	{GC5004_8BIT,0x12, 0x54},//win_width/8*10
	{GC5004_8BIT,0x13, 0x06},
	{GC5004_8BIT,0x42, 0x10},//buf
	{GC5004_8BIT,0x43, 0x05},
	{GC5004_8BIT,0xfe, 0x00},   
	{GC5004_TOK_TERM,0 ,0},
};

static struct gc5004_reg const gc5004_2592x1944_30fps[] = {

	{GC5004_8BIT, 0x18, 0x02},//skip on
	{GC5004_8BIT, 0x80, 0x10},//scaler en
	
	{GC5004_8BIT, 0x05, 0x03}, //HB 806
	{GC5004_8BIT, 0x06, 0x26},
	{GC5004_8BIT, 0x09, 0x00},
	{GC5004_8BIT, 0x0a, 0x02}, //row start 2
	{GC5004_8BIT, 0x0b, 0x00},
	{GC5004_8BIT, 0x0c, 0x00}, //col start 0
	{GC5004_8BIT, 0x0d, 0x07}, //window height 1960
	{GC5004_8BIT, 0x0e, 0xa8}, 
	{GC5004_8BIT, 0x0f, 0x0a}, //Window setting width 2640
	{GC5004_8BIT, 0x10, 0x50},  

	{GC5004_8BIT, 0x4e, 0x3c}, //add by travis 20140625
	{GC5004_8BIT, 0x4f, 0x00},

	{GC5004_8BIT, 0x92, 0x02}, //00/crop win y
	{GC5004_8BIT, 0x94, 0x01}, //04/crop win x  0d

	{GC5004_8BIT, 0x95, 0x07}, //out_height 1944
	{GC5004_8BIT, 0x96, 0x98},
	{GC5004_8BIT, 0x97, 0x0a}, //out_width 2592
	{GC5004_8BIT, 0x98, 0x20},

	{GC5004_8BIT, 0xfe, 0x03},
	{GC5004_8BIT, 0x04, 0x80},//FIFO_prog_full_level 640
	{GC5004_8BIT, 0x05, 0x02},
	{GC5004_8BIT, 0x12, 0xa8},//3240 * 8 / 10 = 2592
	{GC5004_8BIT, 0x13, 0x0c},
	{GC5004_8BIT, 0x42, 0x20},//2592
	{GC5004_8BIT, 0x43, 0x0a},
	{GC5004_8BIT, 0xfe, 0x00},
	{GC5004_TOK_TERM, 0 , 0},
};

#if 0
static struct gc5004_reg const gc5004_2592x1944_12fps[] = {

	{GC5004_8BIT, 0x18, 0x02},//skip on
	{GC5004_8BIT, 0x80, 0x10},//scaler en

	{GC5004_8BIT, 0x05, 0x01}, //HB 306
	{GC5004_8BIT, 0x06, 0x32},
	{GC5004_8BIT, 0x09, 0x00},
	{GC5004_8BIT, 0x0a, 0x02}, //row start 2
	{GC5004_8BIT, 0x0b, 0x00},
	{GC5004_8BIT, 0x0c, 0x00}, //col start 0
	{GC5004_8BIT, 0x0d, 0x07}, //window height 1960
	{GC5004_8BIT, 0x0e, 0xa8},
	{GC5004_8BIT, 0x0f, 0x0a}, //Window setting width 2640
	{GC5004_8BIT, 0x10, 0x50},

	{GC5004_8BIT, 0x92, 0x02}, //00/crop win y
	{GC5004_8BIT, 0x94, 0x01}, //04/crop win x  0d

	{GC5004_8BIT, 0x95, 0x07}, //out_height 1944
	{GC5004_8BIT, 0x96, 0x98},
	{GC5004_8BIT, 0x97, 0x0a}, //out_width 2592
	{GC5004_8BIT, 0x98, 0x20},

	{GC5004_8BIT, 0xfe, 0x03},
	{GC5004_8BIT, 0x04, 0x80},//FIFO_prog_full_level 640
	{GC5004_8BIT, 0x05, 0x02},
	{GC5004_8BIT, 0x12, 0xa8},//3240 * 8 / 10 = 2592
	{GC5004_8BIT, 0x13, 0x0c},
	{GC5004_8BIT, 0x42, 0x20},//2592
	{GC5004_8BIT, 0x43, 0x0a},
	{GC5004_8BIT, 0xfe, 0x00},
	{GC5004_TOK_TERM, 0 , 0},
};
#endif

/* TODO settings of preview/still/video will be updated with new use case */
struct gc5004_resolution gc5004_res_preview[] = {
	{
		.desc = "gc5004_720p_30fps",
		.regs = gc5004_1296x736_30fps,
		.width = 1296,
		.height = 736,
		.fps = 26,
		.pixels_per_line = 4800,
		.lines_per_frame = 780,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.used = 0,
		.skip_frames = 3,
	},
	{
		.desc = "gc5004_720p_30fps",
		.regs = gc5004_1296x976_30fps,
		.width = 1296,
		.height = 976,
		.fps = 20,
		.pixels_per_line = 2400,
		.lines_per_frame = 2000,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.used = 0,
		.skip_frames = 3,
	},
	{
		.desc = "gc5004_5M_30fps",
		.regs = gc5004_2592x1944_30fps,
		.width = 2592,
		.height = 1944,
		.fps = 8,
		.pixels_per_line = 6000,//3800
		.lines_per_frame = 2000,//2000
		.bin_factor_x = 0,
		.bin_factor_y = 0,
		.used = 0,
		.skip_frames = 2,
	},
#if 0
	/*TODO: Need verify whether works on other boards*/
	{
		.desc = "gc5004_5M_12fps",
		.regs = gc5004_2592x1944_12fps,
		.width = 2592,
		.height = 1944,
		.fps = 12,
		.pixels_per_line = 4000,//3800
		.lines_per_frame = 2000,//2000
		.bin_factor_x = 0,
		.bin_factor_y = 0,
		.used = 0,
		.skip_frames = 2,
	},
#endif
};

struct gc5004_resolution gc5004_res_still[] = {
	{
		.desc = "gc5004_5M_30fps",
		.regs = gc5004_2592x1944_30fps,
		.width = 2592,
		.height = 1944,
		.fps = 8,
		.pixels_per_line = 6000,
		.lines_per_frame = 2000,
		.bin_factor_x = 0,
		.bin_factor_y = 0,
		.used = 0,
		.skip_frames = 2,
	},
};

struct gc5004_resolution gc5004_res_video[] = {
	{
		.desc = "gc5004_720p_30fps",
		.regs = gc5004_1296x736_30fps,
		.width = 1296,
		.height = 736,
		.fps = 26,
		.pixels_per_line = 4800,
		.lines_per_frame = 780,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.used = 0,
		.skip_frames = 3,
	},
	{
		.desc = "gc5004_480p_30fps",
		.regs = gc5004_1296x864_30fps,
		.width = 1296,
		.height = 864,
		.fps = 22,
		.pixels_per_line = 4800,
		.lines_per_frame = 908,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.used = 0,
		.skip_frames = 3,
	},
};

/********************** settings for imx - reference *********************/
static struct gc5004_reg const gc5004_init_settings[] = {
	{ GC5004_TOK_TERM, 0, 0}
};

struct gc5004_settings gc5004_sets[] = {
	//[GC5004_BAYTRAIL] = 
	{
		.init_settings = gc5004_init_settings,
		.res_preview = gc5004_res_preview,
		.res_still = gc5004_res_preview,//gc5004_res_still,
		.res_video = gc5004_res_video, //gc5004_res_video,
		.n_res_preview = ARRAY_SIZE(gc5004_res_preview),
		.n_res_still = ARRAY_SIZE(gc5004_res_preview),//ARRAY_SIZE(gc5004_res_still),
		.n_res_video = ARRAY_SIZE(gc5004_res_video),//ARRAY_SIZE(gc5004_res_video),
	},
};

#define	v4l2_format_capture_type_entry(_width, _height, \
		_pixelformat, _bytesperline, _colorspace) \
	{\
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,\
		.fmt.pix.width = (_width),\
		.fmt.pix.height = (_height),\
		.fmt.pix.pixelformat = (_pixelformat),\
		.fmt.pix.bytesperline = (_bytesperline),\
		.fmt.pix.colorspace = (_colorspace),\
		.fmt.pix.sizeimage = (_height)*(_bytesperline),\
	}

#define	s_output_format_entry(_width, _height, _pixelformat, \
		_bytesperline, _colorspace, _fps) \
	{\
		.v4l2_fmt = v4l2_format_capture_type_entry(_width, \
			_height, _pixelformat, _bytesperline, \
				_colorspace),\
		.fps = (_fps),\
	}

#define	s_output_format_reg_entry(_width, _height, _pixelformat, \
		_bytesperline, _colorspace, _fps, _reg_setting) \
	{\
		.s_fmt = s_output_format_entry(_width, _height,\
				_pixelformat, _bytesperline, \
				_colorspace, _fps),\
		.reg_setting = (_reg_setting),\
	}

struct s_ctrl_id {
	struct v4l2_queryctrl qc;
	int (*s_ctrl)(struct v4l2_subdev *sd, u32 val);
	int (*g_ctrl)(struct v4l2_subdev *sd, u32 *val);
};

#define	v4l2_queryctrl_entry_integer(_id, _name,\
		_minimum, _maximum, _step, \
		_default_value, _flags)	\
	{\
		.id = (_id), \
		.type = V4L2_CTRL_TYPE_INTEGER, \
		.name = _name, \
		.minimum = (_minimum), \
		.maximum = (_maximum), \
		.step = (_step), \
		.default_value = (_default_value),\
		.flags = (_flags),\
	}
#define	v4l2_queryctrl_entry_boolean(_id, _name,\
		_default_value, _flags)	\
	{\
		.id = (_id), \
		.type = V4L2_CTRL_TYPE_BOOLEAN, \
		.name = _name, \
		.minimum = 0, \
		.maximum = 1, \
		.step = 1, \
		.default_value = (_default_value),\
		.flags = (_flags),\
	}

#define	s_ctrl_id_entry_integer(_id, _name, \
		_minimum, _maximum, _step, \
		_default_value, _flags, \
		_s_ctrl, _g_ctrl)	\
	{\
		.qc = v4l2_queryctrl_entry_integer(_id, _name,\
				_minimum, _maximum, _step,\
				_default_value, _flags), \
		.s_ctrl = _s_ctrl, \
		.g_ctrl = _g_ctrl, \
	}

#define	s_ctrl_id_entry_boolean(_id, _name, \
		_default_value, _flags, \
		_s_ctrl, _g_ctrl)	\
	{\
		.qc = v4l2_queryctrl_entry_boolean(_id, _name,\
				_default_value, _flags), \
		.s_ctrl = _s_ctrl, \
		.g_ctrl = _g_ctrl, \
	}


struct gc5004_control {
	struct v4l2_queryctrl qc;
	int (*query)(struct v4l2_subdev *sd, s32 *value);
	int (*tweak)(struct v4l2_subdev *sd, s32 value);
};

/* gc5004 device structure */
struct gc5004_device {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt format;
	struct v4l2_ctrl_handler ctrl_handler;
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
	u16 sensor_id;
	u16 coarse_itg;
	u16 fine_itg;
	u16 gain;
	u16 pixels_per_line;
	u16 lines_per_frame;
	u8 fps;
	u8 res;
	u8 type;
	u8 *otp_data;
	struct gc5004_settings *mode_tables;
	struct gc5004_vcm *vcm_driver;
	const struct gc5004_resolution *curr_res_table;
	int entries_curr_table;
};

#define to_gc5004_sensor(x) container_of(x, struct gc5004_device, sd)

#define GC5004_MAX_WRITE_BUF_SIZE	32
struct gc5004_write_buffer {
	u16 addr;
	u8 data[GC5004_MAX_WRITE_BUF_SIZE];
};

struct gc5004_write_ctrl {
	int index;
	struct gc5004_write_buffer buffer;
};

static const struct gc5004_reg gc5004_soft_standby[] = {
	{GC5004_8BIT, 0x0100, 0x00},
	{GC5004_TOK_TERM, 0, 0}
};

static const struct gc5004_reg gc5004_streaming[] = {
//	{GC5004_8BIT, 0x0100, 0x01},
	{GC5004_TOK_TERM, 0, 0}
};

static const struct gc5004_reg gc5004_param_hold[] = {
//	{GC5004_8BIT, 0x0104, 0x01},	/* GROUPED_PARAMETER_HOLD */
	{GC5004_TOK_TERM, 0, 0}
};

static const struct gc5004_reg gc5004_param_update[] = {
	{GC5004_8BIT, 0x0104, 0x00},	/* GROUPED_PARAMETER_HOLD */
	{GC5004_TOK_TERM, 0, 0}
};

/* FIXME - to be removed when real OTP data is ready */
static const u8 otpdata[] = {
	2, 1, 3, 10, 0, 1, 233, 2, 92, 2, 207, 0, 211, 1, 70, 1,
	185, 0, 170, 1, 29, 1, 144, 2, 92, 2, 207, 3, 66, 2, 78, 169,
	94, 151, 9, 7, 5, 163, 121, 96, 77, 71, 78, 96, 123, 160, 132, 97,
	66, 47, 40, 47, 66, 97, 132, 117, 80, 49, 28, 21, 28, 49, 80, 119,
	113, 74, 43, 22, 15, 22, 42, 74, 115, 118, 81, 51, 31, 23, 30, 50,
	81, 119, 131, 98, 70, 51, 44, 51, 68, 98, 131, 181, 123, 100, 83, 76,
	83, 100, 123, 162, 74, 52, 40, 31, 28, 31, 40, 53, 72, 58, 40, 25,
	16, 13, 16, 25, 40, 57, 50, 32, 16, 6, 3, 7, 17, 31, 51, 48,
	29, 14, 3, 0, 4, 14, 29, 49, 50, 32, 17, 8, 4, 8, 17, 32,
	51, 58, 40, 27, 18, 15, 18, 26, 41, 58, 84, 53, 43, 34, 31, 34,
	42, 54, 75, 74, 52, 40, 30, 27, 31, 39, 52, 72, 59, 40, 25, 16,
	12, 16, 25, 40, 58, 51, 33, 17, 7, 3, 7, 17, 32, 52, 49, 30,
	14, 3, 0, 4, 14, 30, 50, 52, 33, 18, 8, 4, 8, 18, 33, 52,
	59, 41, 27, 18, 15, 18, 26, 41, 58, 85, 54, 43, 34, 31, 34, 42,
	54, 76, 123, 91, 74, 61, 57, 62, 74, 93, 123, 99, 73, 52, 39, 35,
	40, 53, 74, 100, 88, 61, 40, 26, 21, 27, 41, 62, 90, 85, 57, 36,
	21, 17, 22, 37, 58, 88, 89, 61, 41, 27, 22, 28, 41, 62, 91, 98,
	73, 53, 41, 37, 41, 53, 74, 100, 137, 91, 76, 64, 60, 64, 76, 93,
	127, 5, 114, 83, 64, 49, 45, 50, 64, 83, 110, 92, 65, 41, 26, 21,
	26, 41, 65, 91, 80, 52, 28, 11, 6, 11, 27, 52, 82, 77, 48, 23,
	7, 2, 7, 23, 47, 79, 81, 53, 29, 14, 8, 13, 29, 53, 82, 90,
	66, 44, 30, 24, 29, 43, 66, 91, 127, 83, 67, 54, 49, 54, 67, 84,
	113, 77, 54, 42, 32, 30, 33, 42, 56, 77, 60, 41, 26, 17, 14, 17,
	27, 42, 61, 52, 33, 17, 7, 3, 7, 18, 33, 54, 50, 30, 14, 3,
	0, 4, 15, 31, 53, 53, 33, 18, 8, 4, 8, 18, 34, 55, 60, 42,
	28, 19, 16, 19, 28, 43, 62, 88, 56, 45, 36, 33, 37, 45, 58, 81,
	79, 55, 42, 32, 29, 32, 42, 56, 77, 62, 43, 27, 17, 13, 17, 27,
	43, 62, 55, 35, 18, 7, 3, 7, 18, 35, 56, 53, 32, 15, 4, 0,
	4, 15, 32, 54, 55, 35, 19, 8, 4, 8, 19, 35, 57, 62, 43, 28,
	19, 15, 19, 28, 44, 63, 90, 57, 45, 36, 32, 35, 45, 58, 82, 184,
	136, 111, 93, 88, 96, 114, 141, 186, 147, 109, 80, 63, 57, 65, 83, 113,
	153, 131, 93, 63, 44, 38, 46, 66, 96, 138, 127, 87, 58, 37, 31, 39,
	60, 91, 135, 132, 93, 64, 46, 39, 47, 66, 97, 139, 147, 109, 82, 65,
	59, 66, 83, 113, 152, 203, 138, 114, 97, 91, 98, 116, 141, 192, 2, 8,
	2, 223, 2, 222, 1, 249, 2, 186, 2, 223, 2, 221, 1, 147
};

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

struct gc5004_vcm gc5004_vcms[] = {
	{
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
	},
};
#endif

