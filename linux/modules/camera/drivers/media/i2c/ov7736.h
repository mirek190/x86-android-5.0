/*
 * Support for ov7736 Camera Sensor.
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

#ifndef __OV7736_H__
#define __OV7736_H__

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
#include <linux/atomisp.h>

#define OV7736_NAME	"ov7736"
#define V4L2_IDENT_OV7736 1111
#define	LAST_REG_SETING	{0xffff, 0xff}

/* #defines for register writes and register array processing */
#define I2C_RETRY_COUNT		5
#define MSG_LEN_OFFSET		2

#define OV7736_INTEGRATION_TIME_MARGIN 8
#define OV7736_FOCAL_LENGTH_NUM	278	/*2.78mm*/
#define OV7736_FOCAL_LENGTH_DEM	100
#define OV7736_F_NUMBER_DEFAULT_NUM	26
#define OV7736_F_NUMBER_DEM	10

#define MAX_FMTS		1

/* OV7736_DEVICE_ID */
#define OV7736_ID		0x7736

/*
 * focal length bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define OV7736_FOCAL_LENGTH_DEFAULT 0x1160064

/*
 * current f-number bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define OV7736_F_NUMBER_DEFAULT 0x1a000a

/*
 * f-number range bits definition:
 * bits 31-24: max f-number numerator
 * bits 23-16: max f-number denominator
 * bits 15-8: min f-number numerator
 * bits 7-0: min f-number denominator
 */
#define OV7736_F_NUMBER_RANGE 0x1a0a1a0
#define OV7736_FINE_INTG_TIME_MIN 0
#define OV7736_FINE_INTG_TIME_MAX_MARGIN 0
#define OV7736_COARSE_INTG_TIME_MIN 1
#define OV7736_COARSE_INTG_TIME_MAX_MARGIN (0xffff - 6)

/*
 * OV7736 System control registers
 */
#define OV7736_SW_SLEEP				0x3008//0100
#define OV7736_SW_RESET				0x3008//0103
#define OV7736_SW_STREAM			0x3008//0100

#define OV7736_SC_CMMN_CHIP_ID_H		0x300A
#define OV7736_SC_CMMN_CHIP_ID_L		0x300B
//#define OV7736_SC_CMMN_SCCB_ID			0x300C
#define OV7736_SC_CMMN_SUB_ID			0x302A /* process, version*/

#define OV7736_SC_CMMN_PLL_CTRL0		0x3034
#define OV7736_SC_CMMN_PLL_CTRL1		0x3035
#define OV7736_SC_CMMN_PLL_CTRL2		0x3039
#define OV7736_SC_CMMN_PLL_CTRL3		0x3012 //annie

#define OV7736_SC_CMMN_PLL_MULTIPLIER		0x3011 //annie
#define OV7736_SC_CMMN_PLL_DEBUG_OPT		0x300f //annie
#define OV7736_SC_CMMN_PLLS_CTRL0		0x303A
#define OV7736_SC_CMMN_PLLS_CTRL1		0x303B
#define OV7736_SC_CMMN_PLLS_CTRL2		0x303C
#define OV7736_SC_CMMN_PLLS_CTRL3		0x303D

#define OV7736_SC_CMMN_MIPI_PHY_16		0x3016
#define OV7736_SC_CMMN_MIPI_PHY_17		0x3017
#define OV7736_SC_CMMN_MIPI_SC_CTRL_18		0x3018
#define OV7736_SC_CMMN_MIPI_SC_CTRL_19		0x3019
#define OV7736_SC_CMMN_MIPI_SC_CTRL_21		0x3021
#define OV7736_SC_CMMN_MIPI_SC_CTRL_22		0x3022

#define OV7736_AEC_PK_EXPO_H			0x3500
#define OV7736_AEC_PK_EXPO_M			0x3501
#define OV7736_AEC_PK_EXPO_L			0x3502
#define OV7736_AEC_MANUAL_CTRL			0x3503
#define OV7736_AGC_ADJ_H			0x350a//8
#define OV7736_AGC_ADJ_L			0x350b//9 sensor gain
#define OV7736_VTS_DIFF_H			0x380e
#define OV7736_VTS_DIFF_L			0x380f
#define OV7736_GROUP_ACCESS			0x3212

#define OV7736_MWB_GAIN_R_H			0x504E//5186
#define OV7736_MWB_GAIN_R_L			0x504F//5187
#define OV7736_MWB_GAIN_G_H			0x5050//5188
#define OV7736_MWB_GAIN_G_L			0x5051//5189
#define OV7736_MWB_GAIN_B_H			0x5052//518a
#define OV7736_MWB_GAIN_B_L			0x5053//518b 

#define OV7736_H_CROP_START_H			0x3800
#define OV7736_H_CROP_START_L			0x3801
#define OV7736_V_CROP_START_H			0x3802
#define OV7736_V_CROP_START_L			0x3803
#define OV7736_H_CROP_END_H			0x3804
#define OV7736_H_CROP_END_L			0x3805
#define OV7736_V_CROP_END_H			0x3806
#define OV7736_V_CROP_END_L			0x3807
#define OV7736_H_OUTSIZE_H			0x3808
#define OV7736_H_OUTSIZE_L			0x3809
#define OV7736_V_OUTSIZE_H			0x380a
#define OV7736_V_OUTSIZE_L			0x380b

#define OV7736_START_STREAMING			0x02//01
#define OV7736_STOP_STREAMING			0x42//00
#define OV7736_RESET                    0x82	

/* frame control reg */
#define OV7736_REG_FRAME_CTRL	0x4202
//#define OV7736_FRAME_START		0x00
//#define OV7736_FRAME_STOP		0x0f


/*register */
#define OV7736_REG_GAIN_0	0x350a  //sensor gain
#define OV7736_REG_GAIN_1	0x350b
#define OV7736_REG_EXPOSURE_0	0x3500
#define OV7736_REG_EXPOSURE_1	0x3501
#define OV7736_REG_EXPOSURE_2	0x3502
#define OV7736_REG_EXPOSURE_AUTO	0x3503
#define OV7736_REG_SMIA	0x0100
#define OV7736_REG_PID	0x300a
#define OV7736_REG_SYS_RESET	0x3000
#define OV7736_REG_FW_START	0x8000

/*value */
#define OV7736_FRAME_START	0x01
#define OV7736_FRAME_STOP	0x00
#define OV7736_AWB_GAIN_AUTO	0
#define OV7736_AWB_GAIN_MANUAL	1

#define MIN_SYSCLK		10
#define MIN_VTS			8
#define MIN_HTS			8
#define MIN_SHUTTER		0
#define MIN_GAIN		0

#define OV7736_RES_VGA_SIZE_H		640
#define OV7736_RES_VGA_SIZE_V		480
#define OV7736_RES_QVGA_SIZE_H		320
#define OV7736_RES_QVGA_SIZE_V		240

#define to_ov7736_sensor(sd) container_of(sd, struct ov7736_device, sd)


/*
 * struct misensor_reg - MI sensor  register format
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

struct regval_list {
	u16 reg_num;
	u8 value;
};

enum ov7736_tok_type {
	OV7736_8BIT  = 0x0001,
	OV7736_16BIT = 0x0002,
	OV7736_32BIT = 0x0004,
	OV7736_RMW_AND = 0x0008,
	OV7736_RMW_OR = 0x0010,
	OV7736_TOK_TERM   = 0xf000,	/* terminating token for reg list */
	OV7736_TOK_DELAY  = 0xfe00	/* delay token for reg list */
};


struct ov7736_reg {
	enum ov7736_tok_type type;
	u16 reg;
	u32 val;	/* @set value for read/mod/write, @mask */
};


struct ov7736_resolution {
	u8 *desc;
	const struct ov7736_reg *regs;
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

struct ov7736_format {
	u8 *desc;
	u32 pixelformat;
	struct ov7736_reg *regs;
};

struct ov7736_control {
	struct v4l2_queryctrl qc;
	int (*query)(struct v4l2_subdev *sd, s32 *value);
	int (*tweak)(struct v4l2_subdev *sd, s32 value);
};


struct ov7736_device {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt format;
	struct firmware *firmware;
	struct mutex input_lock;

	struct camera_sensor_platform_data *platform_data;
	int run_mode;
	int focus_mode;
	int night_mode;
	bool focus_mode_change;
	int color_effect;
	bool streaming;
	bool preview_ag_ae;
	u16 sensor_id;
	u8 sensor_revision;
	unsigned int ae_high;
	unsigned int ae_low;
	unsigned int preview_shutter;
	unsigned int preview_gain16;
	unsigned int average;
	unsigned int preview_sysclk;
	unsigned int preview_hts;
	unsigned int preview_vts;
	unsigned int res;
};

struct ov7736_priv_data {
	u32 port;
	u32 num_of_lane;
	u32 input_format;
	u32 raw_bayer_order;
};

struct ov7736_format_struct {
	u8 *desc;
	u32 pixelformat;
	struct regval_list *regs;
};

struct ov7736_res_struct {
	u8 *desc;
	int res;
	int width;
	int height;
	int fps;
	int skip_frames;
	bool used;
	struct regval_list *regs;
};

#define OV7736_MAX_WRITE_BUF_SIZE	32
struct ov7736_write_buffer {
	u16 addr;
	u8 data[OV7736_MAX_WRITE_BUF_SIZE];
};

struct ov7736_write_ctrl {
	int index;
	struct ov7736_write_buffer buffer;
};

/* Supported resolutions */
enum {
	OV7736_RES_QVGA,
	OV7736_RES_VGA,
};

static struct ov7736_res_struct ov7736_res[] = {
#if 0
	{
	.desc	= "QVGA",
	.res	= OV7736_RES_QVGA,
	.width	= 320,
	.height	= 240,
	.fps	= 30,
	.used	= 0,
	.regs	= NULL,
	.skip_frames = 1,
	},
#endif
	{
	.desc	= "VGA",
	.res	= OV7736_RES_VGA,
	.width	= 640,
	.height	= 480,
	.fps	= 30,
	.used	= 0,
	.regs	= NULL,
	.skip_frames = 2,
	},
};
#define N_RES (ARRAY_SIZE(ov7736_res))

static struct misensor_reg const ov7736_hflip_on_table[] = {
{OV7736_8BIT | OV7736_RMW_OR, 0x3818, 0x40},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_hflip_off_table[] = {
{OV7736_8BIT | OV7736_RMW_AND, 0x3818, ~0x40},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_vflip_on_table[] = {
{OV7736_8BIT | OV7736_RMW_OR, 0x3818, 0x20},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_vflip_off_table[] = {
{OV7736_8BIT | OV7736_RMW_AND, 0x3818, ~0x20},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_auto_scene[] = {
{OV7736_8BIT, 0x3212, 0x00}, // enable group 0
{OV7736_8BIT, 0x5186, 0x02}, //AWB auto
{OV7736_8BIT, 0x3212, 0x10}, // end group 0
{OV7736_8BIT, 0x3212, 0xa0}, // launch group 0
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_sunny_scene[] = {
{OV7736_8BIT, 0x3212, 0x00}, // enable group 0
{OV7736_8BIT, 0x5186, 0x03}, //AWB manual
{OV7736_8BIT, 0x5052, 0x05}, // b gain high
{OV7736_8BIT, 0x5053, 0xd7}, // b gain low
{OV7736_8BIT, 0x5050, 0x04}, // g gain high
{OV7736_8BIT, 0x5051, 0x00}, // g gain low
{OV7736_8BIT, 0x504e, 0x06}, // r gain high
{OV7736_8BIT, 0x504f, 0x35}, // r gain low
{OV7736_8BIT, 0x3212, 0x10}, // end group 0
{OV7736_8BIT, 0x3212, 0xa0}, // launch group 0
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_cloudy_scene[] = {
{OV7736_8BIT, 0x3212, 0x00},
{OV7736_8BIT, 0x5186, 0x03}, //AWB manual
{OV7736_8BIT, 0x5052, 0x04}, // b gain
{OV7736_8BIT, 0x5053, 0xb4},
{OV7736_8BIT, 0x5050, 0x04}, // g gain
{OV7736_8BIT, 0x5051, 0x00},
{OV7736_8BIT, 0x504e, 0x07}, // r gain
{OV7736_8BIT, 0x504f, 0x6d},
{OV7736_8BIT, 0x3212, 0x10},
{OV7736_8BIT, 0x3212, 0xa0},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_office_scene[] = {
{OV7736_8BIT, 0x3212, 0x00},
{OV7736_8BIT, 0x5186, 0x03}, //AWB manual
{OV7736_8BIT, 0x5052, 0x08}, // b gain
{OV7736_8BIT, 0x5053, 0xab},
{OV7736_8BIT, 0x5050, 0x04}, // g gain
{OV7736_8BIT, 0x5051, 0x00},
{OV7736_8BIT, 0x504e, 0x05}, // r gain
{OV7736_8BIT, 0x504f, 0x41},
{OV7736_8BIT, 0x3212, 0x10},
{OV7736_8BIT, 0x3212, 0xa0},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_home_scene[] = {
{OV7736_8BIT, 0x3212, 0x00},
{OV7736_8BIT, 0x5186, 0x03}, //AWB manual
{OV7736_8BIT, 0x5052, 0x09}, // b gain
{OV7736_8BIT, 0x5053, 0x18},
{OV7736_8BIT, 0x5050, 0x04}, // g gain
{OV7736_8BIT, 0x5051, 0x06},
{OV7736_8BIT, 0x504e, 0x04}, // r gain
{OV7736_8BIT, 0x504f, 0x00},
{OV7736_8BIT, 0x3212, 0x10},
{OV7736_8BIT, 0x3212, 0xa0},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_night_mode_on[] = {
{OV7736_8BIT, 0x3a00, 0x7e},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_night_mode_off[] = {
{OV7736_8BIT, 0x3a00, 0x7a},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_exposure_negative2[] = {
{OV7736_8BIT,0x3a0f, 0x18},
{OV7736_8BIT,0x3a10, 0x10},
{OV7736_8BIT,0x3a11, 0x18},
{OV7736_8BIT,0x3a1b, 0x10},
{OV7736_8BIT,0x3a1e, 0x30},
{OV7736_8BIT,0x3a1f, 0x10},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_exposure_negative1[] = {
{OV7736_8BIT, 0x3a0f, 0x28},
{OV7736_8BIT, 0x3a10, 0x20},
{OV7736_8BIT, 0x3a11, 0x51},
{OV7736_8BIT, 0x3a1b, 0x28},
{OV7736_8BIT, 0x3a1e, 0x20},
{OV7736_8BIT, 0x3a1f, 0x10},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_exposure_negative0[] = {
{OV7736_8BIT, 0x3a0f, 0x38},
{OV7736_8BIT, 0x3a10, 0x30},
{OV7736_8BIT, 0x3a11, 0x61},
{OV7736_8BIT, 0x3a1b, 0x38},
{OV7736_8BIT, 0x3a1e, 0x30},
{OV7736_8BIT, 0x3a1f, 0x10},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_exposure_positive1[] = {
{OV7736_8BIT, 0x3a0f, 0x48},
{OV7736_8BIT, 0x3a10, 0x40},
{OV7736_8BIT, 0x3a11, 0x80},
{OV7736_8BIT, 0x3a1b, 0x48},
{OV7736_8BIT, 0x3a1e, 0x40},
{OV7736_8BIT, 0x3a1f, 0x20},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_exposure_positive2[] = {
{OV7736_8BIT, 0x3a0f, 0x58},
{OV7736_8BIT, 0x3a10, 0x50},
{OV7736_8BIT, 0x3a11, 0x91},
{OV7736_8BIT, 0x3a1b, 0x58},
{OV7736_8BIT, 0x3a1e, 0x50},
{OV7736_8BIT, 0x3a1f, 0x20},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_normal_effect[] = {
{OV7736_8BIT, 0x3212, 0x00}, // enable group 0
{OV7736_8BIT, 0x5001, 0xc7},
{OV7736_8BIT, 0x5580, 0x06},
{OV7736_8BIT, 0x5583, 0x40},
{OV7736_8BIT, 0x5584, 0x26},
{OV7736_8BIT, 0x3212, 0x10}, // end group 0
{OV7736_8BIT, 0x3212, 0xa0},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_BW_effect[] = {
{OV7736_8BIT, 0x3212, 0x00},
{OV7736_8BIT, 0x5001, 0xc7},
{OV7736_8BIT, 0x5580, 0x24},
{OV7736_8BIT, 0x5583, 0x80},
{OV7736_8BIT, 0x5584, 0x80},
{OV7736_8BIT, 0x3212, 0x10},
{OV7736_8BIT, 0x3212, 0xa0},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_Bluish_effect[] = {
{OV7736_8BIT, 0x3212, 0x00},
{OV7736_8BIT, 0x5001, 0xc7},
{OV7736_8BIT, 0x5580, 0x1c},
{OV7736_8BIT, 0x5583, 0xa0},
{OV7736_8BIT, 0x5584, 0x40},
{OV7736_8BIT, 0x3212, 0x10},
{OV7736_8BIT, 0x3212, 0xa0},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_Sepia_effect[] = {
{OV7736_8BIT, 0x3212, 0x00},
{OV7736_8BIT, 0x5001, 0xc7},
{OV7736_8BIT, 0x5580, 0x1c},
{OV7736_8BIT, 0x5583, 0x40},
{OV7736_8BIT, 0x5584, 0xa0},
{OV7736_8BIT, 0x3212, 0x10},
{OV7736_8BIT, 0x3212, 0xa0},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_Redish_effect[] = {
{OV7736_8BIT, 0x3212, 0x00},
{OV7736_8BIT, 0x5001, 0xc7},
{OV7736_8BIT, 0x5580, 0x1c},
{OV7736_8BIT, 0x5583, 0x80},
{OV7736_8BIT, 0x5584, 0xc0},
{OV7736_8BIT, 0x3212, 0x10},
{OV7736_8BIT, 0x3212, 0xa0},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_Greenish_effect[] = {
{OV7736_8BIT, 0x3212, 0x00},
{OV7736_8BIT, 0x5001, 0xc7},
{OV7736_8BIT, 0x5580, 0x1c},
{OV7736_8BIT, 0x5583, 0x60},
{OV7736_8BIT, 0x5584, 0x60},
{OV7736_8BIT, 0x3212, 0x10},
{OV7736_8BIT, 0x3212, 0xa0},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_Negative_effect[] = {
{OV7736_8BIT, 0x3212, 0x00},
{OV7736_8BIT, 0x5001, 0xc7},
{OV7736_8BIT, 0x5580, 0x44},
{OV7736_8BIT, 0x5583, 0x40},
{OV7736_8BIT, 0x5584, 0x26},
{OV7736_8BIT, 0x3212, 0x10},
{OV7736_8BIT, 0x3212, 0xa0},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_auto_wb[] = {
{OV7736_8BIT, 0x3212, 0x00},
{OV7736_8BIT, 0x5186, 0x02},
{OV7736_8BIT, 0x504e, 0x04},
{OV7736_8BIT, 0x504f, 0x00},
{OV7736_8BIT, 0x5050, 0x04},
{OV7736_8BIT, 0x5051, 0x00},
{OV7736_8BIT, 0x5052, 0x04},
{OV7736_8BIT, 0x5053, 0x00},
{OV7736_8BIT, 0x3212, 0x10},
{OV7736_8BIT, 0x3212, 0xa0},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_sunny_wb[] = {
{OV7736_8BIT, 0x3212, 0x00}, // enable group 0
{OV7736_8BIT, 0x5186, 0x03},
{OV7736_8BIT, 0x504e, 0x06},
{OV7736_8BIT, 0x504f, 0x1c},
{OV7736_8BIT, 0x5050, 0x04},
{OV7736_8BIT, 0x5051, 0x00},
{OV7736_8BIT, 0x5052, 0x04},
{OV7736_8BIT, 0x5053, 0xf3},
{OV7736_8BIT, 0x3212, 0x10}, // end group 0
{OV7736_8BIT, 0x3212, 0xa0}, // launch group 0
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_cloudy_wb[] = {
{OV7736_8BIT, 0x3212, 0x00}, // enable group 0
{OV7736_8BIT, 0x5186, 0x03},
{OV7736_8BIT, 0x504e, 0x06},
{OV7736_8BIT, 0x504f, 0x48},
{OV7736_8BIT, 0x5050, 0x04},
{OV7736_8BIT, 0x5051, 0x00},
{OV7736_8BIT, 0x5052, 0x04},
{OV7736_8BIT, 0x5053, 0xd3},
{OV7736_8BIT, 0x3212, 0x10}, // end group 0
{OV7736_8BIT, 0x3212, 0xa0}, // launch group 0
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_office_wb[] = {
{OV7736_8BIT, 0x3212, 0x00}, // enable group 0
{OV7736_8BIT, 0x5186, 0x03},
{OV7736_8BIT, 0x504e, 0x05},
{OV7736_8BIT, 0x504f, 0x48},
{OV7736_8BIT, 0x5050, 0x04},
{OV7736_8BIT, 0x5051, 0x00},
{OV7736_8BIT, 0x5052, 0x07},
{OV7736_8BIT, 0x5053, 0xcf},
{OV7736_8BIT, 0x3212, 0x10}, // end group 0
{OV7736_8BIT, 0x3212, 0xa0}, // launch group 0
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_home_wb[] = {
{OV7736_8BIT, 0x3212, 0x00}, // enable group 0
{OV7736_8BIT, 0x5186, 0x03},
{OV7736_8BIT, 0x504e, 0x04},
{OV7736_8BIT, 0x504f, 0x10},
{OV7736_8BIT, 0x5050, 0x04},
{OV7736_8BIT, 0x5051, 0x00},
{OV7736_8BIT, 0x5052, 0x08},
{OV7736_8BIT, 0x5053, 0x40},
{OV7736_8BIT, 0x3212, 0x10}, // end group 0
{OV7736_8BIT, 0x3212, 0xa0}, // launch group 0
{OV7736_TOK_TERM, 0, 0}
};
static const struct i2c_device_id ov7736_id[] = {
	{"ov7736", 0},
	{}
};

static struct misensor_reg const ov7736_standby_reg[] = {
{OV7736_8BIT, 0x302e, 0x08},
{OV7736_8BIT, 0x3623, 0x00},
{OV7736_8BIT, 0x3614, 0x20},
{OV7736_8BIT, 0x4805, 0xd0},
{OV7736_8BIT, 0x300e, 0x14},
{OV7736_8BIT, 0x3008, 0x42},
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_wakeup_reg[] = {
{OV7736_8BIT, 0x302e, 0x00},
{OV7736_8BIT, 0x3623, 0x03},
{OV7736_8BIT, 0x3614, 0x00},
{OV7736_8BIT, 0x4805, 0x10},
{OV7736_8BIT, 0x300e, 0x04},
{OV7736_8BIT, 0x3008, 0x02},
{OV7736_TOK_TERM, 0, 0}
};




static struct misensor_reg const ov7736_basic_init[] = {
{OV7736_8BIT, 0x3008, 0x82},
{OV7736_TOK_DELAY, 0x3008, 0x10},// sleep 5ms
{OV7736_8BIT, 0x3008, 0x42},
{OV7736_8BIT, 0x3630, 0x11},
{OV7736_8BIT, 0x3104, 0x03},
{OV7736_8BIT, 0x3017, 0x7f},
{OV7736_8BIT, 0x3018, 0xfc},
{OV7736_8BIT, 0x3600, 0x1c},
{OV7736_8BIT, 0x3602, 0x04},
{OV7736_8BIT, 0x3611, 0x44},
{OV7736_8BIT, 0x3612, 0x63},
{OV7736_8BIT, 0x3631, 0x22},
{OV7736_8BIT, 0x3622, 0x00},
{OV7736_8BIT, 0x3633, 0x25},
{OV7736_8BIT, 0x370d, 0x07},
{OV7736_8BIT, 0x3620, 0x42},
{OV7736_8BIT, 0x3714, 0x19},
{OV7736_8BIT, 0x3715, 0xfa},
{OV7736_8BIT, 0x370b, 0x43},
{OV7736_8BIT, 0x3713, 0x1a},
{OV7736_8BIT, 0x401c, 0x00},
{OV7736_8BIT, 0x401e, 0x11},
{OV7736_8BIT, 0x4702, 0x01},
{OV7736_8BIT, 0x3a00, 0x7a},
{OV7736_8BIT, 0x3a18, 0x00},
{OV7736_8BIT, 0x3a19, 0x3f},
{OV7736_8BIT, 0x300f, 0x88},
{OV7736_8BIT, 0x3011, 0x08},
{OV7736_8BIT, 0x4303, 0xff},
{OV7736_8BIT, 0x4307, 0xff},
{OV7736_8BIT, 0x430b, 0xff},
{OV7736_8BIT, 0x4305, 0x00},
{OV7736_8BIT, 0x4309, 0x00},
{OV7736_8BIT, 0x430d, 0x00},
{OV7736_8BIT, 0x5181, 0x04},
{OV7736_8BIT, 0x5481, 0x26},
{OV7736_8BIT, 0x5482, 0x35},
{OV7736_8BIT, 0x5483, 0x48},
{OV7736_8BIT, 0x5484, 0x63},
{OV7736_8BIT, 0x5485, 0x6e},
{OV7736_8BIT, 0x5486, 0x77},
{OV7736_8BIT, 0x5487, 0x80},
{OV7736_8BIT, 0x5488, 0x88},
{OV7736_8BIT, 0x5489, 0x8f},
{OV7736_8BIT, 0x548a, 0x96},
{OV7736_8BIT, 0x548b, 0xa3},
{OV7736_8BIT, 0x548c, 0xaf},
{OV7736_8BIT, 0x548d, 0xc5},
{OV7736_8BIT, 0x548e, 0xd7},
{OV7736_8BIT, 0x548f, 0xe8},
{OV7736_8BIT, 0x5490, 0x0f},
{OV7736_8BIT, 0x4001, 0x02},
{OV7736_8BIT, 0x3810, 0x08},
{OV7736_8BIT, 0x3811, 0x02},
{OV7736_8BIT, 0x4300, 0x30},
{OV7736_8BIT, 0x501f, 0x01},
{OV7736_8BIT, 0x5000, 0x4f},
{OV7736_8BIT, 0x5001, 0x47},
{OV7736_8BIT, 0x3017, 0x00},
{OV7736_8BIT, 0x3018, 0x00},
{OV7736_8BIT, 0x300e, 0x04},
{OV7736_8BIT, 0x4801, 0x0f},
{OV7736_8BIT, 0x4601, 0x02},
{OV7736_8BIT, 0x300f, 0x8a},
{OV7736_8BIT, 0x3011, 0x02},
{OV7736_8BIT, 0x302d, 0x50},
{OV7736_8BIT, 0x3632, 0x39},
{OV7736_8BIT, 0x3010, 0x00},
{OV7736_8BIT, 0x4300, 0x3f},
{OV7736_8BIT, 0x3715, 0x1a},
{OV7736_8BIT, 0x370e, 0x00},
{OV7736_8BIT, 0x3713, 0x08},
{OV7736_8BIT, 0x3703, 0x2c},
{OV7736_8BIT, 0x3620, 0xc2},
{OV7736_8BIT, 0x3714, 0x36},
{OV7736_8BIT, 0x3716, 0x01},
{OV7736_8BIT, 0x3623, 0x03},
{OV7736_8BIT, 0x3c00, 0x00},
{OV7736_8BIT, 0x3c01, 0x32},
{OV7736_8BIT, 0x3c04, 0x12},
{OV7736_8BIT, 0x3c05, 0x60},
{OV7736_8BIT, 0x3c06, 0x00},
{OV7736_8BIT, 0x3c07, 0x20},
{OV7736_8BIT, 0x3c08, 0x00},
{OV7736_8BIT, 0x3c09, 0xc2},
{OV7736_8BIT, 0x300d, 0x22},
{OV7736_8BIT, 0x3c0a, 0x9c},
{OV7736_8BIT, 0x3c0b, 0x40},
{OV7736_8BIT, 0x3008, 0x02},//
{OV7736_8BIT, 0x5180, 0x02},
{OV7736_8BIT, 0x5181, 0x02},
{OV7736_8BIT, 0x3a0f, 0x35},
{OV7736_8BIT, 0x3a10, 0x2c},
{OV7736_8BIT, 0x3a1b, 0x36},
{OV7736_8BIT, 0x3a1e, 0x2d},
{OV7736_8BIT, 0x3a11, 0x90},
{OV7736_8BIT, 0x3a1f, 0x10},
{OV7736_8BIT, 0x5000, 0xcf},
{OV7736_8BIT, 0x5481, 0x0a},
{OV7736_8BIT, 0x5482, 0x13},
{OV7736_8BIT, 0x5483, 0x23},
{OV7736_8BIT, 0x5484, 0x40},
{OV7736_8BIT, 0x5485, 0x4d},
{OV7736_8BIT, 0x5486, 0x58},
{OV7736_8BIT, 0x5487, 0x64},
{OV7736_8BIT, 0x5488, 0x6e},
{OV7736_8BIT, 0x5489, 0x78},
{OV7736_8BIT, 0x548a, 0x81},
{OV7736_8BIT, 0x548b, 0x92},
{OV7736_8BIT, 0x548c, 0xa1},
{OV7736_8BIT, 0x548d, 0xbb},
{OV7736_8BIT, 0x548e, 0xcf},
{OV7736_8BIT, 0x548f, 0xe3},
{OV7736_8BIT, 0x5490, 0x26},
{OV7736_8BIT, 0x5380, 0x42},
{OV7736_8BIT, 0x5381, 0x33},
{OV7736_8BIT, 0x5382, 0x0f},
{OV7736_8BIT, 0x5383, 0x0b},
{OV7736_8BIT, 0x5384, 0x42},
{OV7736_8BIT, 0x5385, 0x4d},
{OV7736_8BIT, 0x5392, 0x1e},
{OV7736_8BIT, 0x5801, 0x00},
{OV7736_8BIT, 0x5802, 0x00},
{OV7736_8BIT, 0x5803, 0x00},
{OV7736_8BIT, 0x5804, 0x12},
{OV7736_8BIT, 0x5805, 0x15},
{OV7736_8BIT, 0x5806, 0x08},
{OV7736_8BIT, 0x5001, 0xc7},
{OV7736_8BIT, 0x5580, 0x02},
{OV7736_8BIT, 0x5583, 0x40},
{OV7736_8BIT, 0x5584, 0x26},
{OV7736_8BIT, 0x5589, 0x10},
{OV7736_8BIT, 0x558a, 0x00},
{OV7736_8BIT, 0x558b, 0x3e},
{OV7736_8BIT, 0x5300, 0x0f},
{OV7736_8BIT, 0x5301, 0x30},
{OV7736_8BIT, 0x5302, 0x0d},
{OV7736_8BIT, 0x5303, 0x02},
{OV7736_8BIT, 0x5300, 0x0f},
{OV7736_8BIT, 0x5301, 0x30},
{OV7736_8BIT, 0x5302, 0x0d},
{OV7736_8BIT, 0x5303, 0x02},
{OV7736_8BIT, 0x5304, 0x0e},
{OV7736_8BIT, 0x5305, 0x30},
{OV7736_8BIT, 0x5306, 0x06},
{OV7736_8BIT, 0x5307, 0x40},
{OV7736_8BIT, 0x5680, 0x00},
{OV7736_8BIT, 0x5681, 0x50},
{OV7736_8BIT, 0x5682, 0x00},
{OV7736_8BIT, 0x5683, 0x3c},
{OV7736_8BIT, 0x5684, 0x11},
{OV7736_8BIT, 0x5685, 0xe0},
{OV7736_8BIT, 0x5686, 0x0d},
{OV7736_8BIT, 0x5687, 0x68},
{OV7736_8BIT, 0x5688, 0x03},
{OV7736_8BIT, 0x3012, 0x02},
{OV7736_8BIT, 0x3011, 0x05},
{OV7736_8BIT, 0x4004, 0x06},
{OV7736_8BIT, 0x3800, 0x00},
{OV7736_8BIT, 0x3801, 0x8e},
{OV7736_8BIT, 0x3802, 0x00},
{OV7736_8BIT, 0x3803, 0x0a},	
{OV7736_8BIT, 0x3804, 0x02},
{OV7736_8BIT, 0x3805, 0x80},
{OV7736_8BIT, 0x3806, 0x01},
{OV7736_8BIT, 0x3807, 0xe0},
{OV7736_8BIT, 0x3808, 0x02},
{OV7736_8BIT, 0x3809, 0x80},
{OV7736_8BIT, 0x380a, 0x01},
{OV7736_8BIT, 0x380b, 0xe0},
{OV7736_8BIT, 0x380c, 0x03},
{OV7736_8BIT, 0x380d, 0x20},
{OV7736_8BIT, 0x380e, 0x01},
{OV7736_8BIT, 0x380f, 0xf4},
{OV7736_8BIT, 0x3622, 0x00},
{OV7736_8BIT | OV7736_RMW_OR, 0x3818, 0x80},
{OV7736_8BIT, 0x3a08, 0x00},
{OV7736_8BIT, 0x3a09, 0x96},
{OV7736_8BIT, 0x3a0a, 0x00}, 
{OV7736_8BIT, 0x3a0b, 0x7d},
{OV7736_8BIT, 0x370d, 0x0b},
{OV7736_8BIT, 0x4837, 0x31},//31~3c
{OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_vga_init[] = {
{OV7736_8BIT, 0x4004, 0x06},
{OV7736_8BIT, 0x3800, 0x00},
{OV7736_8BIT, 0x3801, 0x8e},
{OV7736_8BIT, 0x3802, 0x00},
{OV7736_8BIT, 0x3803, 0x0a},	
{OV7736_8BIT, 0x3804, 0x02},
{OV7736_8BIT, 0x3805, 0x80},
{OV7736_8BIT, 0x3806, 0x01},
{OV7736_8BIT, 0x3807, 0xe0},
{OV7736_8BIT, 0x3808, 0x02},
{OV7736_8BIT, 0x3809, 0x80},
{OV7736_8BIT, 0x380a, 0x01},
{OV7736_8BIT, 0x380b, 0xe0},
{OV7736_8BIT, 0x380c, 0x03},
{OV7736_8BIT, 0x380d, 0x20},
{OV7736_8BIT, 0x380e, 0x01},
{OV7736_8BIT, 0x380f, 0xf4},
{OV7736_8BIT, 0x3622, 0x00},
{OV7736_8BIT | OV7736_RMW_OR, 0x3818, 0x80},
{OV7736_8BIT, 0x3a08, 0x00},
{OV7736_8BIT, 0x3a09, 0x96},
{OV7736_8BIT, 0x3a0a, 0x00}, 
{OV7736_8BIT, 0x3a0b, 0x7d},
{OV7736_8BIT, 0x370d, 0x0b},
{OV7736_8BIT, 0x4837, 0x31},//31~3c
{OV7736_TOK_TERM, 0, 0}
};

#if 0
static struct misensor_reg const ov7736_qvga_init[] = {
{OV7736_8BIT, 0x4004, 0x02},
{OV7736_8BIT, 0x3800, 0x01},
{OV7736_8BIT, 0x3801, 0x08},
{OV7736_8BIT, 0x3804, 0x01},
{OV7736_8BIT, 0x3805, 0x40},
{OV7736_8BIT, 0x3802, 0x00},
{OV7736_8BIT, 0x3803, 0x0a},
{OV7736_8BIT, 0x3806, 0x00},
{OV7736_8BIT, 0x3807, 0xf0},
{OV7736_8BIT, 0x3808, 0x01},
{OV7736_8BIT, 0x3809, 0x40},
{OV7736_8BIT, 0x380a, 0x00},
{OV7736_8BIT, 0x380b, 0xf0},
{OV7736_8BIT, 0x380c, 0x06},
{OV7736_8BIT, 0x380d, 0x1c},
{OV7736_8BIT, 0x380e, 0x01},
{OV7736_8BIT, 0x380f, 0x00},
{OV7736_8BIT, 0x3622, 0x08},
{OV7736_8BIT | OV7736_RMW_OR, 0x3818, 0x81},
{OV7736_8BIT, 0x3a08, 0x00},
{OV7736_8BIT, 0x3a09, 0x4c},
{OV7736_8BIT, 0x3a0a, 0x00},
{OV7736_8BIT, 0x3a0b, 0x3f},
{OV7736_8BIT, 0x3a0d, 0x04},
{OV7736_8BIT, 0x3a0e, 0x03},
{OV7736_8BIT, 0x3705, 0xdd},
{OV7736_8BIT, 0x3a1a, 0x06},
{OV7736_8BIT, 0x370d, 0x4b},
{OV7736_8BIT, 0x4837, 0x31},//31~3c
{OV7736_TOK_TERM, 0, 0}
};
#endif

static struct misensor_reg const ov7736_common[] = {
	 {OV7736_TOK_TERM, 0, 0}
};

static struct misensor_reg const ov7736_iq[] = {
	{OV7736_TOK_TERM, 0, 0}
};

#endif
