/*
 * Support for OmniVision OV8858 8M camera sensor.
 * Based on OmniVision 8858 driver.
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

#ifndef __OV8858_H__
#define __OV8858_H__
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

#define OV8858_NAME		"ov8858"

/* Defines for register writes and register array processing */
#define I2C_MSG_LENGTH		0x2
#define I2C_RETRY_COUNT		5

#define OV8858_FOCAL_LENGTH_NUM	334	/*3.34mm*/
#define OV8858_FOCAL_LENGTH_DEM	100
#define OV8858_F_NUMBER_DEFAULT_NUM	28
#define OV8858_F_NUMBER_DEM	10

#define MAX_FMTS		1

/* sensor_mode_data read_mode adaptation */
#define OV8858_READ_MODE_BINNING_ON	0x0400
#define OV8858_READ_MODE_BINNING_OFF	0x00
#define OV8858_INTEGRATION_TIME_MARGIN	8

#define OV8858_MAX_EXPOSURE_VALUE	0xFFF1
#define OV8858_MAX_GAIN_VALUE		0xFF

/*
 * focal length bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define OV8858_FOCAL_LENGTH_DEFAULT 0x1B70064

/*
 * current f-number bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define OV8858_F_NUMBER_DEFAULT 0x18000a

/*
 * f-number range bits definition:
 * bits 31-24: max f-number numerator
 * bits 23-16: max f-number denominator
 * bits 15-8: min f-number numerator
 * bits 7-0: min f-number denominator
 */
#define OV8858_F_NUMBER_RANGE 0x180a180a
#define OV8858_ID	0x8858

#define OV8858_FINE_INTG_TIME_MIN 0
#define OV8858_FINE_INTG_TIME_MAX_MARGIN 0
#define OV8858_COARSE_INTG_TIME_MIN 1
#define OV8858_COARSE_INTG_TIME_MAX_MARGIN (0xffff - 6)

#define OV8858_BIN_FACTOR_MAX 4
/*
 * OV8858 System control registers
 */
#define OV8858_SW_SLEEP				0x0100
#define OV8858_SW_RESET				0x0103
#define OV8858_SW_STREAM			0x0100

#define OV8858_SC_CMMN_CHIP_ID_H		0x300B
#define OV8858_SC_CMMN_CHIP_ID_L		0x300C
#define OV8858_SC_CMMN_SCCB_ID			0x300C
#define OV8858_SC_CMMN_SUB_ID			0x302A /* process, version*/

#define OV8858_GROUP_ACCESS							0x3208 /*Bit[7:4] Group control, Bit[3:0] Group ID*/

#define OV8858_EXPOSURE_H							0x3500 /*Bit[3:0] Bit[19:16] of exposure, remaining 16 bits lies in Reg0x3501&Reg0x3502*/
#define OV8858_EXPOSURE_M							0x3501
#define OV8858_EXPOSURE_L							0x3502
#define OV8858_AGC_H								0x3508 /*Bit[1:0] means Bit[9:8] of gain*/
#define OV8858_AGC_L								0x3509 /*Bit[7:0] of gain*/

#define OV8858_HORIZONTAL_START_H					0x3800 /*Bit[11:8]*/
#define OV8858_HORIZONTAL_START_L					0x3801 /*Bit[7:0]*/
#define OV8858_VERTICAL_START_H						0x3802 /*Bit[11:8]*/
#define OV8858_VERTICAL_START_L						0x3803 /*Bit[7:0]*/
#define OV8858_HORIZONTAL_END_H						0x3804 /*Bit[11:8]*/
#define OV8858_HORIZONTAL_END_L						0x3805 /*Bit[7:0]*/
#define OV8858_VERTICAL_END_H						0x3806 /*Bit[11:8]*/
#define OV8858_VERTICAL_END_L						0x3807 /*Bit[7:0]*/
#define OV8858_HORIZONTAL_OUTPUT_SIZE_H				0x3808 /*Bit[3:0]*/
#define OV8858_HORIZONTAL_OUTPUT_SIZE_L				0x3809 /*Bit[7:0]*/
#define OV8858_VERTICAL_OUTPUT_SIZE_H				0x380a /*Bit[3:0]*/
#define OV8858_VERTICAL_OUTPUT_SIZE_L				0x380b /*Bit[7:0]*/
#define OV8858_TIMING_HTS_H							0x380C  /*High 8-bit, and low 8-bit HTS address is 0x380d*/
#define OV8858_TIMING_HTS_L							0x380D  /*High 8-bit, and low 8-bit HTS address is 0x380d*/
#define OV8858_TIMING_VTS_H							0x380e  /*High 8-bit, and low 8-bit HTS address is 0x380f*/
#define OV8858_TIMING_VTS_L							0x380f  /*High 8-bit, and low 8-bit HTS address is 0x380f*/
#define OV8858_VFLIP_REG							0x3820  /*bit [1,2] value 0x06 to enable*/
#define OV8858_HFLIP_REG							0x3821  /*bit [1,2] value 0x06 to enable*/
#define OV8858_VFLIP_VALUE			0x06
#define OV8858_HFLIP_VALUE			0x06
#define OV8858_DIGI_GAIN_H			0x350a
#define OV8858_DIGI_GAIN_L			0x350b

#define OV8858_MWB_GAIN_MAX				0x0fff

#define OV8858_START_STREAMING			0x01
#define OV8858_STOP_STREAMING			0x00

#define VCM_ADDR           0x0c
#define VCM_CODE_MSB       0x03
#define VCM_CODE_LSB       0x04
#define VCM_MAX_FOCUS_POS  1023

#define OV8858_VCM_SLEW_STEP			0x30F0
#define OV8858_VCM_SLEW_STEP_MAX		0x7
#define OV8858_VCM_SLEW_STEP_MASK		0x7
#define OV8858_VCM_CODE					0x30F2
#define OV8858_VCM_SLEW_TIME			0x30F4
#define OV8858_VCM_SLEW_TIME_MAX		0xffff
#define OV8858_VCM_ENABLE			0x8000

#define OV8858_MAX_FOCUS_POS	255
#define OV8858_MAX_FOCUS_NEG	(-255)

// Add OTP operation
#define BG_Ratio_Typical  0x16E
#define RG_Ratio_Typical  0x189

struct ov8858_otp_struct {
		int otp_en;
		int module_integrator_id;
		int lens_id;
		int production_year;
		int production_month;
		int production_day;
		int rg_ratio;
		int bg_ratio;
		int user_data[2];
		int light_rg;
		int light_bg;
		int R_gain;
		int G_gain;
		int B_gain;
		int lenc[110];
		int VCM_start;
		int VCM_end;
		int VCM_dir;
};

struct ov8858_vcm {
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

struct regval_list {
	u16 reg_num;
	u8 value;
};

struct ov8858_resolution {
	u8 *desc;
	const struct ov8858_reg *regs;
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

struct ov8858_format {
	u8 *desc;
	u32 pixelformat;
	struct ov8858_reg *regs;
};

struct ov8858_control {
	struct v4l2_queryctrl qc;
	int (*query)(struct v4l2_subdev *sd, s32 *value);
	int (*tweak)(struct v4l2_subdev *sd, s32 value);
};

/*
 * ov8858 device structure.
 */
struct ov8858_device {
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
	struct ov8858_vcm *vcm_driver;
	struct ov8858_otp_struct current_otp;
};


enum ov8858_tok_type {
	OV8858_8BIT  = 0x0001,
	OV8858_16BIT = 0x0002,
	OV8858_32BIT = 0x0004,
	OV8858_TOK_TERM   = 0xf000,	/* terminating token for reg list */
	OV8858_TOK_DELAY  = 0xfe00,	/* delay token for reg list */
	OV8858_TOK_MASK = 0xfff0
};

/**
 * struct ov8858_reg - MI sensor  register format
 * @type: type of the register
 * @reg: 16-bit offset to register
 * @val: 8/16/32-bit register value
 *
 * Define a structure for sensor register initialization values
 */
struct ov8858_reg {
	enum ov8858_tok_type type;
	u16 reg;
	u32 val;	/* @set value for read/mod/write, @mask */
};

#define to_ov8858_sensor(x) container_of(x, struct ov8858_device, sd)

#define OV8858_MAX_WRITE_BUF_SIZE	30

struct ov8858_write_buffer {
	u16 addr;
	u8 data[OV8858_MAX_WRITE_BUF_SIZE];
};

struct ov8858_write_ctrl {
	int index;
	struct ov8858_write_buffer buffer;
};

static const struct i2c_device_id ov8858_id[] = {
	{OV8858_NAME, 0},
	{}
};

/*
 * Register settings for various resolution
 */
static struct ov8858_reg const ov8858_BasicSettings[] = {
	{ OV8858_8BIT,  0x0103 , 0x01 },
	{ OV8858_8BIT,  0x0100 , 0x00 },
	{ OV8858_8BIT,  0x0300 , 0x05 },
	{ OV8858_8BIT,  0x0302 , 0x96 },
	{ OV8858_8BIT,  0x0303 , 0x00 },
	{ OV8858_8BIT,  0x0304 , 0x03 },
	{ OV8858_8BIT,  0x030b , 0x05 },
	{ OV8858_8BIT,  0x030d , 0x96 },
	{ OV8858_8BIT,  0x030e , 0x00 },
	{ OV8858_8BIT,  0x0312 , 0x01 },
	{ OV8858_8BIT,  0x031e , 0x0c },
	{ OV8858_8BIT,  0x3018 , 0x32 },// 2 lane mode
	{ OV8858_8BIT,  0x3600 , 0x00 },
	{ OV8858_8BIT,  0x3601 , 0x00 },
	{ OV8858_8BIT,  0x3602 , 0x00 },
	{ OV8858_8BIT,  0x3603 , 0x00 },
	{ OV8858_8BIT,  0x3604 , 0x22 },
	{ OV8858_8BIT,  0x3605 , 0x30 },
	{ OV8858_8BIT,  0x3606 , 0x00 },
	{ OV8858_8BIT,  0x3607 , 0x20 },
	{ OV8858_8BIT,  0x3608 , 0x11 },
	{ OV8858_8BIT,  0x3609 , 0x28 },
	{ OV8858_8BIT,  0x360a , 0x00 },
	{ OV8858_8BIT,  0x360b , 0x06 },
	{ OV8858_8BIT,  0x360c , 0xdc },
	{ OV8858_8BIT,  0x360d , 0x40 },
	{ OV8858_8BIT,  0x360e , 0x0c },
	{ OV8858_8BIT,  0x360f , 0x20 },
	{ OV8858_8BIT,  0x3610 , 0x07 },
	{ OV8858_8BIT,  0x3611 , 0x20 },
	{ OV8858_8BIT,  0x3612 , 0x88 },
	{ OV8858_8BIT,  0x3613 , 0x80 },
	{ OV8858_8BIT,  0x3614 , 0x58 },
	{ OV8858_8BIT,  0x3615 , 0x00 },
	{ OV8858_8BIT,  0x3616 , 0x4a },
	{ OV8858_8BIT,  0x3617 , 0x90 },
	{ OV8858_8BIT,  0x3618 , 0x56 },
	{ OV8858_8BIT,  0x3619 , 0x70 },
	{ OV8858_8BIT,  0x361a , 0x99 },
	{ OV8858_8BIT,  0x361b , 0x00 },
	{ OV8858_8BIT,  0x361c , 0x07 },
	{ OV8858_8BIT,  0x361d , 0x00 },
	{ OV8858_8BIT,  0x361e , 0x00 },
	{ OV8858_8BIT,  0x361f , 0x00 },
	{ OV8858_8BIT,  0x3638 , 0xff },
	{ OV8858_8BIT,  0x3633 , 0x0c },
	{ OV8858_8BIT,  0x3634 , 0x0c },
	{ OV8858_8BIT,  0x3635 , 0x0c },
	{ OV8858_8BIT,  0x3636 , 0x0c },
	{ OV8858_8BIT,  0x3645 , 0x13 },
	{ OV8858_8BIT,  0x3646 , 0x83 },
	{ OV8858_8BIT,  0x364a , 0x07 },
	{ OV8858_8BIT,  0x3015 , 0x01 },
	{ OV8858_8BIT,  0x3020 , 0x93 },
	{ OV8858_8BIT,  0x3022 , 0x01 },
	{ OV8858_8BIT,  0x3031 , 0x0a },
	{ OV8858_8BIT,  0x3034 , 0x00 },
	{ OV8858_8BIT,  0x3106 , 0x01 },
	{ OV8858_8BIT,  0x3305 , 0xf1 },
	{ OV8858_8BIT,  0x3308 , 0x00 },
	{ OV8858_8BIT,  0x3309 , 0x28 },
	{ OV8858_8BIT,  0x330a , 0x00 },
	{ OV8858_8BIT,  0x330b , 0x20 },
	{ OV8858_8BIT,  0x330c , 0x00 },
	{ OV8858_8BIT,  0x330d , 0x00 },
	{ OV8858_8BIT,  0x330e , 0x00 },
	{ OV8858_8BIT,  0x330f , 0x40 },
	{ OV8858_8BIT,  0x3307 , 0x04 },
	{ OV8858_8BIT,  0x3500 , 0x00 },
	{ OV8858_8BIT,  0x3503 , 0x00 },
	{ OV8858_8BIT,  0x3505 , 0x80 },
	{ OV8858_8BIT,  0x3509 , 0x00 },
	{ OV8858_8BIT,  0x350c , 0x00 },
	{ OV8858_8BIT,  0x350d , 0x80 },
	{ OV8858_8BIT,  0x3510 , 0x00 },
	{ OV8858_8BIT,  0x3511 , 0x02 },
	{ OV8858_8BIT,  0x3512 , 0x00 },
	{ OV8858_8BIT,  0x3705 , 0x00 },
	{ OV8858_8BIT,  0x3719 , 0x31 },
	{ OV8858_8BIT,  0x3714 , 0x24 },
	{ OV8858_8BIT,  0x3733 , 0x10 },
	{ OV8858_8BIT,  0x3734 , 0x40 },
	{ OV8858_8BIT,  0x3755 , 0x10 },
	{ OV8858_8BIT,  0x3758 , 0x00 },
	{ OV8858_8BIT,  0x3759 , 0x4c },
	{ OV8858_8BIT,  0x375c , 0x20 },
	{ OV8858_8BIT,  0x375e , 0x00 },
	{ OV8858_8BIT,  0x3761 , 0x00 },
	{ OV8858_8BIT,  0x3762 , 0x00 },
	{ OV8858_8BIT,  0x3763 , 0x00 },
	{ OV8858_8BIT,  0x3766 , 0xff },
	{ OV8858_8BIT,  0x376b , 0x00 },
	{ OV8858_8BIT,  0x3777 , 0x00 },
	{ OV8858_8BIT,  0x37a3 , 0x00 },
	{ OV8858_8BIT,  0x37a4 , 0x00 },
	{ OV8858_8BIT,  0x37a5 , 0x00 },
	{ OV8858_8BIT,  0x37a6 , 0x00 },
	{ OV8858_8BIT,  0x3760 , 0x00 },
	{ OV8858_8BIT,  0x376f , 0x01 },
	{ OV8858_8BIT,  0x37b0 , 0x00 },
	{ OV8858_8BIT,  0x37b1 , 0x00 },
	{ OV8858_8BIT,  0x37b2 , 0x00 },
	{ OV8858_8BIT,  0x37b6 , 0x00 },
	{ OV8858_8BIT,  0x37b7 , 0x00 },
	{ OV8858_8BIT,  0x37b8 , 0x00 },
	{ OV8858_8BIT,  0x37b9 , 0xff },
	{ OV8858_8BIT,  0x3800 , 0x00 },
	{ OV8858_8BIT,  0x3801 , 0x0c },
	{ OV8858_8BIT,  0x3802 , 0x00 },
	{ OV8858_8BIT,  0x3803 , 0x0c },
	{ OV8858_8BIT,  0x3804 , 0x0c },
	{ OV8858_8BIT,  0x3805 , 0xd3 },
	{ OV8858_8BIT,  0x3806 , 0x09 },
	{ OV8858_8BIT,  0x3807 , 0xa3 },
	{ OV8858_8BIT,  0x3810 , 0x00 },
	{ OV8858_8BIT,  0x3811 , 0x04 },
	{ OV8858_8BIT,  0x3813 , 0x02 },
	{ OV8858_8BIT,  0x3815 , 0x01 },
	{ OV8858_8BIT,  0x3820 , 0x00 },
	{ OV8858_8BIT,  0x3837 , 0x18 },
	{ OV8858_8BIT,  0x3841 , 0xff },
	{ OV8858_8BIT,  0x3846 , 0x48 },
	{ OV8858_8BIT,  0x3d85 , 0x14 },
	{ OV8858_8BIT,  0x4000 , 0xf1 },
	{ OV8858_8BIT,  0x4005 , 0x10 },
	{ OV8858_8BIT,  0x4002 , 0x27 },
	{ OV8858_8BIT,  0x4009 , 0x81 },
	{ OV8858_8BIT,  0x400b , 0x0c },
	{ OV8858_8BIT,  0x401b , 0x00 },
	{ OV8858_8BIT,  0x401d , 0x00 },
	{ OV8858_8BIT,  0x4020 , 0x00 },
	{ OV8858_8BIT,  0x4021 , 0x04 },
	{ OV8858_8BIT,  0x4028 , 0x00 },
	{ OV8858_8BIT,  0x4029 , 0x02 },
	{ OV8858_8BIT,  0x402f , 0x02 },
	{ OV8858_8BIT,  0x401f , 0x00 },
	{ OV8858_8BIT,  0x4034 , 0x3f },
	{ OV8858_8BIT,  0x403d , 0x04 },
	{ OV8858_8BIT,  0x4300 , 0xff },
	{ OV8858_8BIT,  0x4301 , 0x00 },
	{ OV8858_8BIT,  0x4302 , 0x0f },
	{ OV8858_8BIT,  0x4316 , 0x00 },
	{ OV8858_8BIT,  0x4503 , 0x18 },
	{ OV8858_8BIT,  0x481f , 0x32 },
	{ OV8858_8BIT,  0x4837 , 0x16 },
	{ OV8858_8BIT,  0x4850 , 0x10 },
	{ OV8858_8BIT,  0x4851 , 0x32 },
	{ OV8858_8BIT,  0x4b00 , 0x2a },
	{ OV8858_8BIT,  0x4b0d , 0x00 },
	{ OV8858_8BIT,  0x4d00 , 0x04 },
	{ OV8858_8BIT,  0x4d01 , 0x18 },
	{ OV8858_8BIT,  0x4d02 , 0xc3 },
	{ OV8858_8BIT,  0x4d03 , 0xff },
	{ OV8858_8BIT,  0x4d04 , 0xff },
	{ OV8858_8BIT,  0x4d05 , 0xff },
	{ OV8858_8BIT,  0x5000 , 0x7e },
	{ OV8858_8BIT,  0x5001 , 0x01 },
	{ OV8858_8BIT,  0x5002 , 0x08 },
	{ OV8858_8BIT,  0x5003 , 0x20 },
	{ OV8858_8BIT,  0x5046 , 0x12 },
	{ OV8858_8BIT,  0x5780 , 0xfc },
	{ OV8858_8BIT,  0x5784 , 0x0c },
	{ OV8858_8BIT,  0x5787 , 0x40 },
	{ OV8858_8BIT,  0x5788 , 0x08 },
	{ OV8858_8BIT,  0x578a , 0x02 },
	{ OV8858_8BIT,  0x578b , 0x01 },
	{ OV8858_8BIT,  0x578c , 0x01 },
	{ OV8858_8BIT,  0x578e , 0x02 },
	{ OV8858_8BIT,  0x578f , 0x01 },
	{ OV8858_8BIT,  0x5790 , 0x01 },
	{ OV8858_8BIT,  0x5b00 , 0x02 },
	{ OV8858_8BIT,  0x5b01 , 0x10 },
	{ OV8858_8BIT,  0x5b02 , 0x03 },
	{ OV8858_8BIT,  0x5b03 , 0xcf },
	{ OV8858_8BIT,  0x5b05 , 0x6c },
	{ OV8858_8BIT,  0x5e00 , 0x00 },
	{ OV8858_8BIT,  0x5e01 , 0x41 },
	{ OV8858_8BIT,  0x4825 , 0x3a },
	{ OV8858_8BIT,  0x4826 , 0x40 },
	{ OV8858_8BIT,  0x4808 , 0x25 },

	{ OV8858_TOK_TERM, 0, 0}
};

/*****************************STILL********************************/

static struct ov8858_reg const ov8858_cont_cap_720P[] = {
	/* Output size 1296x736 */
	{ OV8858_8BIT, 0x030f , 0x09},
	{ OV8858_8BIT, 0x3501 , 0x4d},
	{ OV8858_8BIT, 0x3502 , 0x40},
	{ OV8858_8BIT, 0x3508 , 0x04},
	{ OV8858_8BIT, 0x3700 , 0x18},
	{ OV8858_8BIT, 0x3701 , 0x0c},
	{ OV8858_8BIT, 0x3702 , 0x28},
	{ OV8858_8BIT, 0x3703 , 0x19},
	{ OV8858_8BIT, 0x3704 , 0x14},
	{ OV8858_8BIT, 0x3706 , 0x35},
	{ OV8858_8BIT, 0x3707 , 0x04},
	{ OV8858_8BIT, 0x3708 , 0x24},
	{ OV8858_8BIT, 0x3709 , 0x33},
	{ OV8858_8BIT, 0x370a , 0x00},
	{ OV8858_8BIT, 0x370b , 0xb5},
	{ OV8858_8BIT, 0x370c , 0x04},
	{ OV8858_8BIT, 0x3718 , 0x12},
	{ OV8858_8BIT, 0x3712 , 0x42},
	{ OV8858_8BIT, 0x371e , 0x19},
	{ OV8858_8BIT, 0x371f , 0x40},
	{ OV8858_8BIT, 0x3720 , 0x05},
	{ OV8858_8BIT, 0x3721 , 0x05},
	{ OV8858_8BIT, 0x3724 , 0x06},
	{ OV8858_8BIT, 0x3725 , 0x01},
	{ OV8858_8BIT, 0x3726 , 0x06},
	{ OV8858_8BIT, 0x3728 , 0x05},
	{ OV8858_8BIT, 0x3729 , 0x02},
	{ OV8858_8BIT, 0x372a , 0x03},
	{ OV8858_8BIT, 0x372b , 0x53},
	{ OV8858_8BIT, 0x372c , 0xa3},
	{ OV8858_8BIT, 0x372d , 0x53},
	{ OV8858_8BIT, 0x372e , 0x06},
	{ OV8858_8BIT, 0x372f , 0x10},
	{ OV8858_8BIT, 0x3730 , 0x01},
	{ OV8858_8BIT, 0x3731 , 0x06},
	{ OV8858_8BIT, 0x3732 , 0x14},
	{ OV8858_8BIT, 0x3736 , 0x20},
	{ OV8858_8BIT, 0x373a , 0x05},
	{ OV8858_8BIT, 0x373b , 0x06},
	{ OV8858_8BIT, 0x373c , 0x0a},
	{ OV8858_8BIT, 0x373e , 0x03},
	{ OV8858_8BIT, 0x375a , 0x06},
	{ OV8858_8BIT, 0x375b , 0x13},
	{ OV8858_8BIT, 0x375d , 0x02},
	{ OV8858_8BIT, 0x375f , 0x14},
	{ OV8858_8BIT, 0x3768 , 0x22},
	{ OV8858_8BIT, 0x3769 , 0x44},
	{ OV8858_8BIT, 0x376a , 0x44},
	{ OV8858_8BIT, 0x3772 , 0x23},
	{ OV8858_8BIT, 0x3773 , 0x02},
	{ OV8858_8BIT, 0x3774 , 0x16},
	{ OV8858_8BIT, 0x3775 , 0x12},
	{ OV8858_8BIT, 0x3776 , 0x04},
	{ OV8858_8BIT, 0x3778 , 0x1b},
	{ OV8858_8BIT, 0x37a0 , 0x44},
	{ OV8858_8BIT, 0x37a1 , 0x3d},
	{ OV8858_8BIT, 0x37a2 , 0x3d},
	{ OV8858_8BIT, 0x37a7 , 0x44},
	{ OV8858_8BIT, 0x37a8 , 0x4c},
	{ OV8858_8BIT, 0x37a9 , 0x4c},
	{ OV8858_8BIT, 0x37aa , 0x44},
	{ OV8858_8BIT, 0x37ab , 0x2e},
	{ OV8858_8BIT, 0x37ac , 0x2e},
	{ OV8858_8BIT, 0x37ad , 0x33},
	{ OV8858_8BIT, 0x37ae , 0x0d},
	{ OV8858_8BIT, 0x37af , 0x0d},
	{ OV8858_8BIT, 0x37b3 , 0x42},
	{ OV8858_8BIT, 0x37b4 , 0x42},
	{ OV8858_8BIT, 0x37b5 , 0x33},
	{ OV8858_8BIT, 0x3808 , 0x05},
	{ OV8858_8BIT, 0x3809 , 0x10},
	{ OV8858_8BIT, 0x380a , 0x02},
	{ OV8858_8BIT, 0x380b , 0xe0},
	{ OV8858_8BIT, 0x380c , 0x07},
	{ OV8858_8BIT, 0x380d , 0x88},
	{ OV8858_8BIT, 0x380e , 0x04},
	{ OV8858_8BIT, 0x380f , 0xdc},
	{ OV8858_8BIT, 0x3814 , 0x03},
	{ OV8858_8BIT, 0x3821 , 0x61},
	{ OV8858_8BIT, 0x382a , 0x03},
	{ OV8858_8BIT, 0x382b , 0x01},
	{ OV8858_8BIT, 0x3830 , 0x08},
	{ OV8858_8BIT, 0x3836 , 0x02},
	{ OV8858_8BIT, 0x3f08 , 0x08},
	{ OV8858_8BIT, 0x3f0a , 0x80},
	{ OV8858_8BIT, 0x4001 , 0x10},
	{ OV8858_8BIT, 0x4022 , 0x04},
	{ OV8858_8BIT, 0x4023 , 0xbd},
	{ OV8858_8BIT, 0x4024 , 0x05},
	{ OV8858_8BIT, 0x4025 , 0x2E},
	{ OV8858_8BIT, 0x4026 , 0x05},
	{ OV8858_8BIT, 0x4027 , 0x2F},
	{ OV8858_8BIT, 0x402a , 0x04},
	{ OV8858_8BIT, 0x402b , 0x04},
	{ OV8858_8BIT, 0x402c , 0x02},
	{ OV8858_8BIT, 0x402d , 0x02},
	{ OV8858_8BIT, 0x402e , 0x08},
	{ OV8858_8BIT, 0x4500 , 0x38},
	{ OV8858_8BIT, 0x4600 , 0x00},
	{ OV8858_8BIT, 0x4601 , 0xa0},
	{ OV8858_8BIT, 0x5901 , 0x00},
	{ OV8858_8BIT, 0x382d , 0x7f},
	{ OV8858_TOK_TERM,0, 0}
};

static struct ov8858_reg const ov8858_1080P_STILL[] = {
	/* Output size 1936x1096 */
	{ OV8858_8BIT, 0x030f , 0x04 },
	{ OV8858_8BIT, 0x3501 , 0x9a },
	{ OV8858_8BIT, 0x3502 , 0x20 },
	{ OV8858_8BIT, 0x3508 , 0x02 },
	{ OV8858_8BIT, 0x3700 , 0x30 },
	{ OV8858_8BIT, 0x3701 , 0x18 },
	{ OV8858_8BIT, 0x3702 , 0x50 },
	{ OV8858_8BIT, 0x3703 , 0x32 },
	{ OV8858_8BIT, 0x3704 , 0x28 },
	{ OV8858_8BIT, 0x3706 , 0x6a },
	{ OV8858_8BIT, 0x3707 , 0x08 },
	{ OV8858_8BIT, 0x3708 , 0x48 },
	{ OV8858_8BIT, 0x3709 , 0x66 },
	{ OV8858_8BIT, 0x370a , 0x01 },
	{ OV8858_8BIT, 0x370b , 0x6a },
	{ OV8858_8BIT, 0x370c , 0x07 },
	{ OV8858_8BIT, 0x3718 , 0x14 },
	{ OV8858_8BIT, 0x3712 , 0x44 },
	{ OV8858_8BIT, 0x371e , 0x31 },
	{ OV8858_8BIT, 0x371f , 0x7f },
	{ OV8858_8BIT, 0x3720 , 0x0a },
	{ OV8858_8BIT, 0x3721 , 0x0a },
	{ OV8858_8BIT, 0x3724 , 0x0c },
	{ OV8858_8BIT, 0x3725 , 0x02 },
	{ OV8858_8BIT, 0x3726 , 0x0c },
	{ OV8858_8BIT, 0x3728 , 0x0a },
	{ OV8858_8BIT, 0x3729 , 0x03 },
	{ OV8858_8BIT, 0x372a , 0x06 },
	{ OV8858_8BIT, 0x372b , 0xa6 },
	{ OV8858_8BIT, 0x372c , 0xa6 },
	{ OV8858_8BIT, 0x372d , 0xa6 },
	{ OV8858_8BIT, 0x372e , 0x0c },
	{ OV8858_8BIT, 0x372f , 0x20 },
	{ OV8858_8BIT, 0x3730 , 0x02 },
	{ OV8858_8BIT, 0x3731 , 0x0c },
	{ OV8858_8BIT, 0x3732 , 0x28 },
	{ OV8858_8BIT, 0x3736 , 0x30 },
	{ OV8858_8BIT, 0x373a , 0x0a },
	{ OV8858_8BIT, 0x373b , 0x0b },
	{ OV8858_8BIT, 0x373c , 0x14 },
	{ OV8858_8BIT, 0x373e , 0x06 },
	{ OV8858_8BIT, 0x375a , 0x0c },
	{ OV8858_8BIT, 0x375b , 0x26 },
	{ OV8858_8BIT, 0x375d , 0x04 },
	{ OV8858_8BIT, 0x375f , 0x28 },
	{ OV8858_8BIT, 0x3768 , 0x22 },
	{ OV8858_8BIT, 0x3769 , 0x44 },
	{ OV8858_8BIT, 0x376a , 0x44 },
	{ OV8858_8BIT, 0x3772 , 0x46 },
	{ OV8858_8BIT, 0x3773 , 0x04 },
	{ OV8858_8BIT, 0x3774 , 0x2c },
	{ OV8858_8BIT, 0x3775 , 0x13 },
	{ OV8858_8BIT, 0x3776 , 0x08 },
	{ OV8858_8BIT, 0x3778 , 0x16 },
	{ OV8858_8BIT, 0x37a0 , 0x88 },
	{ OV8858_8BIT, 0x37a1 , 0x7a },
	{ OV8858_8BIT, 0x37a2 , 0x7a },
	{ OV8858_8BIT, 0x37a7 , 0x88 },
	{ OV8858_8BIT, 0x37a8 , 0x98 },
	{ OV8858_8BIT, 0x37a9 , 0x98 },
	{ OV8858_8BIT, 0x37aa , 0x88 },
	{ OV8858_8BIT, 0x37ab , 0x5c },
	{ OV8858_8BIT, 0x37ac , 0x5c },
	{ OV8858_8BIT, 0x37ad , 0x55 },
	{ OV8858_8BIT, 0x37ae , 0x19 },
	{ OV8858_8BIT, 0x37af , 0x19 },
	{ OV8858_8BIT, 0x37b3 , 0x84 },
	{ OV8858_8BIT, 0x37b4 , 0x84 },
	{ OV8858_8BIT, 0x37b5 , 0x66 },
	{ OV8858_8BIT, 0x3808 , 0x07 },
	{ OV8858_8BIT, 0x3809 , 0x90 },
	{ OV8858_8BIT, 0x380a , 0x04 },
	{ OV8858_8BIT, 0x380b , 0x48 },
	{ OV8858_8BIT, 0x380c , 0x0d },
	{ OV8858_8BIT, 0x380d , 0xbc },
	{ OV8858_8BIT, 0x380e , 0x0a },
	{ OV8858_8BIT, 0x380f , 0xaa },
	{ OV8858_8BIT, 0x3814 , 0x01 },
	{ OV8858_8BIT, 0x3821 , 0x40 },
	{ OV8858_8BIT, 0x382a , 0x01 },
	{ OV8858_8BIT, 0x382b , 0x01 },
	{ OV8858_8BIT, 0x3830 , 0x06 },
	{ OV8858_8BIT, 0x3836 , 0x01 },
	{ OV8858_8BIT, 0x3f08 , 0x10 },
	{ OV8858_8BIT, 0x4001 , 0x00 },
	{ OV8858_8BIT, 0x4022 , 0x07 },
	{ OV8858_8BIT, 0x4023 , 0x2d },
	{ OV8858_8BIT, 0x4024 , 0x07 },
	{ OV8858_8BIT, 0x4025 , 0x9e },
	{ OV8858_8BIT, 0x4026 , 0x07 },
	{ OV8858_8BIT, 0x4027 , 0x9F },
	{ OV8858_8BIT, 0x402a , 0x04 },
	{ OV8858_8BIT, 0x402b , 0x08 },
	{ OV8858_8BIT, 0x402c , 0x02 },
	{ OV8858_8BIT, 0x402d , 0x02 },
	{ OV8858_8BIT, 0x402e , 0x0c },
	{ OV8858_8BIT, 0x4600 , 0x00 },
	{ OV8858_8BIT, 0x4601 , 0xf0 },
	{ OV8858_8BIT, 0x5901 , 0x00 },
	{ OV8858_TOK_TERM, 0, 0}
};


static struct ov8858_reg const ov8858_VGA_STILL[] = {
	/* Ouput Size 656x496 */
	{ OV8858_8BIT, 0x030f, 0x09},
	{ OV8858_8BIT, 0x3501, 0x27},
	{ OV8858_8BIT, 0x3502, 0x80},
	{ OV8858_8BIT, 0x3508, 0x04},
	{ OV8858_8BIT, 0x3700, 0x18},
	{ OV8858_8BIT, 0x3701, 0x0c},
	{ OV8858_8BIT, 0x3702, 0x28},
	{ OV8858_8BIT, 0x3703, 0x19},
	{ OV8858_8BIT, 0x3704, 0x14},
	{ OV8858_8BIT, 0x3706, 0x35},
	{ OV8858_8BIT, 0x3707, 0x04},
	{ OV8858_8BIT, 0x3708, 0x24},
	{ OV8858_8BIT, 0x3709, 0x33},
	{ OV8858_8BIT, 0x370a, 0x00},
	{ OV8858_8BIT, 0x370b, 0xb5},
	{ OV8858_8BIT, 0x370c, 0x04},
	{ OV8858_8BIT, 0x3718, 0x12},
	{ OV8858_8BIT, 0x3712, 0x42},
	{ OV8858_8BIT, 0x371e, 0x19},
	{ OV8858_8BIT, 0x371f, 0x40},
	{ OV8858_8BIT, 0x3720, 0x05},
	{ OV8858_8BIT, 0x3721, 0x05},
	{ OV8858_8BIT, 0x3724, 0x06},
	{ OV8858_8BIT, 0x3725, 0x01},
	{ OV8858_8BIT, 0x3726, 0x06},
	{ OV8858_8BIT, 0x3728, 0x05},
	{ OV8858_8BIT, 0x3729, 0x02},
	{ OV8858_8BIT, 0x372a, 0x03},
	{ OV8858_8BIT, 0x372b, 0x53},
	{ OV8858_8BIT, 0x372c, 0xa3},
	{ OV8858_8BIT, 0x372d, 0x53},
	{ OV8858_8BIT, 0x372e, 0x06},
	{ OV8858_8BIT, 0x372f, 0x10},
	{ OV8858_8BIT, 0x3730, 0x01},
	{ OV8858_8BIT, 0x3731, 0x06},
	{ OV8858_8BIT, 0x3732, 0x14},
	{ OV8858_8BIT, 0x3736, 0x20},
	{ OV8858_8BIT, 0x373a, 0x05},
	{ OV8858_8BIT, 0x373b, 0x06},
	{ OV8858_8BIT, 0x373c, 0x0a},
	{ OV8858_8BIT, 0x373e, 0x03},
	{ OV8858_8BIT, 0x375a, 0x06},
	{ OV8858_8BIT, 0x375b, 0x13},
	{ OV8858_8BIT, 0x375d, 0x02},
	{ OV8858_8BIT, 0x375f, 0x14},
	{ OV8858_8BIT, 0x3768, 0x00},
	{ OV8858_8BIT, 0x3769, 0xc0},
	{ OV8858_8BIT, 0x376a, 0x42},
	{ OV8858_8BIT, 0x3772, 0x23},
	{ OV8858_8BIT, 0x3773, 0x02},
	{ OV8858_8BIT, 0x3774, 0x16},
	{ OV8858_8BIT, 0x3775, 0x12},
	{ OV8858_8BIT, 0x3776, 0x04},
	{ OV8858_8BIT, 0x3778, 0x17},
	{ OV8858_8BIT, 0x37a0, 0x44},
	{ OV8858_8BIT, 0x37a1, 0x3d},
	{ OV8858_8BIT, 0x37a2, 0x3d},
	{ OV8858_8BIT, 0x37a7, 0x44},
	{ OV8858_8BIT, 0x37a8, 0x4c},
	{ OV8858_8BIT, 0x37a9, 0x4c},
	{ OV8858_8BIT, 0x37aa, 0x44},
	{ OV8858_8BIT, 0x37ab, 0x2e},
	{ OV8858_8BIT, 0x37ac, 0x2e},
	{ OV8858_8BIT, 0x37ad, 0x33},
	{ OV8858_8BIT, 0x37ae, 0x0d},
	{ OV8858_8BIT, 0x37af, 0x0d},
	{ OV8858_8BIT, 0x37b3, 0x42},
	{ OV8858_8BIT, 0x37b4, 0x42},
	{ OV8858_8BIT, 0x37b5, 0x33},
	{ OV8858_8BIT, 0x3808, 0x02},
	{ OV8858_8BIT, 0x3809, 0x90},
	{ OV8858_8BIT, 0x380a, 0x01},
	{ OV8858_8BIT, 0x380b, 0xf0},
	{ OV8858_8BIT, 0x380c, 0x04},
	{ OV8858_8BIT, 0x380d, 0xe2},
	{ OV8858_8BIT, 0x380e, 0x07}, 
	{ OV8858_8BIT, 0x380f, 0x80},
	{ OV8858_8BIT, 0x3814, 0x03},
	{ OV8858_8BIT, 0x3821, 0x69},
	{ OV8858_8BIT, 0x382a, 0x05},
	{ OV8858_8BIT, 0x382b, 0x03},
	{ OV8858_8BIT, 0x3830, 0x0c},
	{ OV8858_8BIT, 0x3836, 0x02},
	{ OV8858_8BIT, 0x3f08, 0x08},
	{ OV8858_8BIT, 0x3f0a, 0x80},
	{ OV8858_8BIT, 0x4001, 0x10},
	{ OV8858_8BIT, 0x4022, 0x04},
	{ OV8858_8BIT, 0x4023, 0xbd},
	{ OV8858_8BIT, 0x4024, 0x05},
	{ OV8858_8BIT, 0x4025, 0x2E},
	{ OV8858_8BIT, 0x4026, 0x05},
	{ OV8858_8BIT, 0x4027, 0x2F},
	{ OV8858_8BIT, 0x402a, 0x02},
	{ OV8858_8BIT, 0x402b, 0x04},
	{ OV8858_8BIT, 0x402c, 0x00},
	{ OV8858_8BIT, 0x402d, 0x00},
	{ OV8858_8BIT, 0x402e, 0x06},
	{ OV8858_8BIT, 0x4500, 0x38},
	{ OV8858_8BIT, 0x4600, 0x00},
	{ OV8858_8BIT, 0x4601, 0x50},
	{ OV8858_8BIT, 0x5901, 0x04},
	{ OV8858_8BIT, 0x382d, 0x7f},
	{ OV8858_TOK_TERM, 0, 0}
};

static struct ov8858_reg const ov8858_1M_STILL[] = {
	/* Ouput Size 1040x784  */
	{ OV8858_8BIT,0x030f, 0x09},
	{ OV8858_8BIT,0x3501, 0x4d},
	{ OV8858_8BIT,0x3502, 0x40},
	{ OV8858_8BIT,0x3508, 0x04},
	{ OV8858_8BIT,0x3700, 0x18},
	{ OV8858_8BIT,0x3701, 0x0c},
	{ OV8858_8BIT,0x3702, 0x28},
	{ OV8858_8BIT,0x3703, 0x19},
	{ OV8858_8BIT,0x3704, 0x14},
	{ OV8858_8BIT,0x3706, 0x35},
	{ OV8858_8BIT,0x3707, 0x04},
	{ OV8858_8BIT,0x3708, 0x24},
	{ OV8858_8BIT,0x3709, 0x33},
	{ OV8858_8BIT,0x370a, 0x00},
	{ OV8858_8BIT,0x370b, 0xb5},
	{ OV8858_8BIT,0x370c, 0x04},
	{ OV8858_8BIT,0x3718, 0x12},
	{ OV8858_8BIT,0x3712, 0x42},
	{ OV8858_8BIT,0x371e, 0x19},
	{ OV8858_8BIT,0x371f, 0x40},
	{ OV8858_8BIT,0x3720, 0x05},
	{ OV8858_8BIT,0x3721, 0x05},
	{ OV8858_8BIT,0x3724, 0x06},
	{ OV8858_8BIT,0x3725, 0x01},
	{ OV8858_8BIT,0x3726, 0x06},
	{ OV8858_8BIT,0x3728, 0x05},
	{ OV8858_8BIT,0x3729, 0x02},
	{ OV8858_8BIT,0x372a, 0x03},
	{ OV8858_8BIT,0x372b, 0x53},
	{ OV8858_8BIT,0x372c, 0xa3},
	{ OV8858_8BIT,0x372d, 0x53},
	{ OV8858_8BIT,0x372e, 0x06},
	{ OV8858_8BIT,0x372f, 0x10},
	{ OV8858_8BIT,0x3730, 0x01},
	{ OV8858_8BIT,0x3731, 0x06},
	{ OV8858_8BIT,0x3732, 0x14},
	{ OV8858_8BIT,0x3736, 0x20},
	{ OV8858_8BIT,0x373a, 0x05},
	{ OV8858_8BIT,0x373b, 0x06},
	{ OV8858_8BIT,0x373c, 0x0a},
	{ OV8858_8BIT,0x373e, 0x03},
	{ OV8858_8BIT,0x375a, 0x06},
	{ OV8858_8BIT,0x375b, 0x13},
	{ OV8858_8BIT,0x375d, 0x02},
	{ OV8858_8BIT,0x375f, 0x14},
	{ OV8858_8BIT,0x3768, 0x22},
	{ OV8858_8BIT,0x3769, 0x44},
	{ OV8858_8BIT,0x376a, 0x44},
	{ OV8858_8BIT,0x3772, 0x23},
	{ OV8858_8BIT,0x3773, 0x02},
	{ OV8858_8BIT,0x3774, 0x16},
	{ OV8858_8BIT,0x3775, 0x12},
	{ OV8858_8BIT,0x3776, 0x04},
	{ OV8858_8BIT,0x3778, 0x1b},
	{ OV8858_8BIT,0x37a0, 0x44},
	{ OV8858_8BIT,0x37a1, 0x3d},
	{ OV8858_8BIT,0x37a2, 0x3d},
	{ OV8858_8BIT,0x37a7, 0x44},
	{ OV8858_8BIT,0x37a8, 0x4c},
	{ OV8858_8BIT,0x37a9, 0x4c},
	{ OV8858_8BIT,0x37aa, 0x44},
	{ OV8858_8BIT,0x37ab, 0x2e},
	{ OV8858_8BIT,0x37ac, 0x2e},
	{ OV8858_8BIT,0x37ad, 0x33},
	{ OV8858_8BIT,0x37ae, 0x0d},
	{ OV8858_8BIT,0x37af, 0x0d},
	{ OV8858_8BIT,0x37b3, 0x42},
	{ OV8858_8BIT,0x37b4, 0x42},
	{ OV8858_8BIT,0x37b5, 0x33},
	{ OV8858_8BIT,0x3808, 0x04},
	{ OV8858_8BIT,0x3809, 0x10},
	{ OV8858_8BIT,0x380a, 0x03},
	{ OV8858_8BIT,0x380b, 0x10},
	{ OV8858_8BIT,0x380c, 0x07},
	{ OV8858_8BIT,0x380d, 0x88},
	{ OV8858_8BIT,0x380e, 0x04},
	{ OV8858_8BIT,0x380f, 0xdc},
	{ OV8858_8BIT,0x3814, 0x03},
	{ OV8858_8BIT,0x3821, 0x61},
	{ OV8858_8BIT,0x382a, 0x03},
	{ OV8858_8BIT,0x382b, 0x01},
	{ OV8858_8BIT,0x3830, 0x08},
	{ OV8858_8BIT,0x3836, 0x02},
	{ OV8858_8BIT,0x3f08, 0x08},
	{ OV8858_8BIT,0x3f0a, 0x80},
	{ OV8858_8BIT,0x4001, 0x10},
	{ OV8858_8BIT,0x4022, 0x03},
	{ OV8858_8BIT,0x4023, 0xbd},
	{ OV8858_8BIT,0x4024, 0x04},
	{ OV8858_8BIT,0x4025, 0x2E},
	{ OV8858_8BIT,0x4026, 0x04},
	{ OV8858_8BIT,0x4027, 0x2F},
	{ OV8858_8BIT,0x402a, 0x04},
	{ OV8858_8BIT,0x402b, 0x04},
	{ OV8858_8BIT,0x402c, 0x02},
	{ OV8858_8BIT,0x402d, 0x02},
	{ OV8858_8BIT,0x402e, 0x08},
	{ OV8858_8BIT,0x4500, 0x38},
	{ OV8858_8BIT,0x4600, 0x00},
	{ OV8858_8BIT,0x4601, 0x80},
	{ OV8858_8BIT,0x5901, 0x00},
	{ OV8858_8BIT,0x382d, 0x7f},
	{ OV8858_TOK_TERM, 0, 0}
};

static struct ov8858_reg const ov8858_2M_STILL[] = {
	/* Ouput Size 1632x1224 */
	{ OV8858_8BIT, 0x030f, 0x09 },
	{ OV8858_8BIT, 0x3501, 0x4d },
	{ OV8858_8BIT, 0x3502, 0x40 },
	{ OV8858_8BIT, 0x3508, 0x04 },
	{ OV8858_8BIT, 0x3700, 0x18 },
	{ OV8858_8BIT, 0x3701, 0x0c },
	{ OV8858_8BIT, 0x3702, 0x28 },
	{ OV8858_8BIT, 0x3703, 0x19 },
	{ OV8858_8BIT, 0x3704, 0x14 },
	{ OV8858_8BIT, 0x3706, 0x35 },
	{ OV8858_8BIT, 0x3707, 0x04 },
	{ OV8858_8BIT, 0x3708, 0x24 },
	{ OV8858_8BIT, 0x3709, 0x33 },
	{ OV8858_8BIT, 0x370a, 0x00 },
	{ OV8858_8BIT, 0x370b, 0xb5 },
	{ OV8858_8BIT, 0x370c, 0x04 },
	{ OV8858_8BIT, 0x3718, 0x12 },
	{ OV8858_8BIT, 0x3712, 0x42 },
	{ OV8858_8BIT, 0x371e, 0x19 },
	{ OV8858_8BIT, 0x371f, 0x40 },
	{ OV8858_8BIT, 0x3720, 0x05 },
	{ OV8858_8BIT, 0x3721, 0x05 },
	{ OV8858_8BIT, 0x3724, 0x06 },
	{ OV8858_8BIT, 0x3725, 0x01 },
	{ OV8858_8BIT, 0x3726, 0x06 },
	{ OV8858_8BIT, 0x3728, 0x05 },
	{ OV8858_8BIT, 0x3729, 0x02 },
	{ OV8858_8BIT, 0x372a, 0x03 },
	{ OV8858_8BIT, 0x372b, 0x53 },
	{ OV8858_8BIT, 0x372c, 0xa3 },
	{ OV8858_8BIT, 0x372d, 0x53 },
	{ OV8858_8BIT, 0x372e, 0x06 },
	{ OV8858_8BIT, 0x372f, 0x10 },
	{ OV8858_8BIT, 0x3730, 0x01 },
	{ OV8858_8BIT, 0x3731, 0x06 },
	{ OV8858_8BIT, 0x3732, 0x14 },
	{ OV8858_8BIT, 0x3736, 0x20 },
	{ OV8858_8BIT, 0x373a, 0x05 },
	{ OV8858_8BIT, 0x373b, 0x06 },
	{ OV8858_8BIT, 0x373c, 0x0a },
	{ OV8858_8BIT, 0x373e, 0x03 },
	{ OV8858_8BIT, 0x375a, 0x06 },
	{ OV8858_8BIT, 0x375b, 0x13 },
	{ OV8858_8BIT, 0x375d, 0x02 },
	{ OV8858_8BIT, 0x375f, 0x14 },
	{ OV8858_8BIT, 0x3768, 0x22 },
	{ OV8858_8BIT, 0x3769, 0x44 },
	{ OV8858_8BIT, 0x376a, 0x44 },
	{ OV8858_8BIT, 0x3772, 0x23 },
	{ OV8858_8BIT, 0x3773, 0x02 },
	{ OV8858_8BIT, 0x3774, 0x16 },
	{ OV8858_8BIT, 0x3775, 0x12 },
	{ OV8858_8BIT, 0x3776, 0x04 },
	{ OV8858_8BIT, 0x3778, 0x1b },
	{ OV8858_8BIT, 0x37a0, 0x44 },
	{ OV8858_8BIT, 0x37a1, 0x3d },
	{ OV8858_8BIT, 0x37a2, 0x3d },
	{ OV8858_8BIT, 0x37a7, 0x44 },
	{ OV8858_8BIT, 0x37a8, 0x4c },
	{ OV8858_8BIT, 0x37a9, 0x4c },
	{ OV8858_8BIT, 0x37aa, 0x44 },
	{ OV8858_8BIT, 0x37ab, 0x2e },
	{ OV8858_8BIT, 0x37ac, 0x2e },
	{ OV8858_8BIT, 0x37ad, 0x33 },
	{ OV8858_8BIT, 0x37ae, 0x0d },
	{ OV8858_8BIT, 0x37af, 0x0d },
	{ OV8858_8BIT, 0x37b3, 0x42 },
	{ OV8858_8BIT, 0x37b4, 0x42 },
	{ OV8858_8BIT, 0x37b5, 0x33 },
	{ OV8858_8BIT, 0x3808, 0x06 },
	{ OV8858_8BIT, 0x3809, 0x60 },
	{ OV8858_8BIT, 0x380a, 0x04 },
	{ OV8858_8BIT, 0x380b, 0xc8 },
	{ OV8858_8BIT, 0x380c, 0x07 },
	{ OV8858_8BIT, 0x380d, 0x88 },
	{ OV8858_8BIT, 0x380e, 0x04 },
	{ OV8858_8BIT, 0x380f, 0xdc },
	{ OV8858_8BIT, 0x3814, 0x03 },
	{ OV8858_8BIT, 0x3821, 0x61 },
	{ OV8858_8BIT, 0x382a, 0x03 },
	{ OV8858_8BIT, 0x382b, 0x01 },
	{ OV8858_8BIT, 0x3830, 0x08 },
	{ OV8858_8BIT, 0x3836, 0x02 },
	{ OV8858_8BIT, 0x3f08, 0x08 },
	{ OV8858_8BIT, 0x3f0a, 0x80 },
	{ OV8858_8BIT, 0x4001, 0x10 },
	{ OV8858_8BIT, 0x4022, 0x05 },//change by annie on 0507
	{ OV8858_8BIT, 0x4023, 0xfd },
	{ OV8858_8BIT, 0x4024, 0x06 },
	{ OV8858_8BIT, 0x4025, 0x6e },
	{ OV8858_8BIT, 0x4026, 0x06 },
	{ OV8858_8BIT, 0x4027, 0x6f },
	{ OV8858_8BIT, 0x402a, 0x04 },
	{ OV8858_8BIT, 0x402b, 0x04 },
	{ OV8858_8BIT, 0x402c, 0x02 },
	{ OV8858_8BIT, 0x402d, 0x02 },
	{ OV8858_8BIT, 0x402e, 0x08 },
	{ OV8858_8BIT, 0x4500, 0x38 },
	{ OV8858_8BIT, 0x4600, 0x00 },
	{ OV8858_8BIT, 0x4601, 0xcb },
	{ OV8858_8BIT, 0x5901, 0x00 },
	{ OV8858_8BIT, 0x382d, 0x7f },
	{ OV8858_TOK_TERM, 0, 0}
};

static struct ov8858_reg const ov8858_3M_STILL[] = {
	/* Ouput Size 2064x1552 */
	{ OV8858_8BIT, 0x030f , 0x04 },
	{ OV8858_8BIT, 0x3501 , 0x9a },
	{ OV8858_8BIT, 0x3502 , 0x20 },
	{ OV8858_8BIT, 0x3508 , 0x02 },
	{ OV8858_8BIT, 0x3700 , 0x30 },
	{ OV8858_8BIT, 0x3701 , 0x18 },
	{ OV8858_8BIT, 0x3702 , 0x50 },
	{ OV8858_8BIT, 0x3703 , 0x32 },
	{ OV8858_8BIT, 0x3704 , 0x28 },
	{ OV8858_8BIT, 0x3706 , 0x6a },
	{ OV8858_8BIT, 0x3707 , 0x08 },
	{ OV8858_8BIT, 0x3708 , 0x48 },
	{ OV8858_8BIT, 0x3709 , 0x66 },
	{ OV8858_8BIT, 0x370a , 0x01 },
	{ OV8858_8BIT, 0x370b , 0x6a },
	{ OV8858_8BIT, 0x370c , 0x07 },
	{ OV8858_8BIT, 0x3718 , 0x14 },
	{ OV8858_8BIT, 0x3712 , 0x44 },
	{ OV8858_8BIT, 0x371e , 0x31 },
	{ OV8858_8BIT, 0x371f , 0x7f },
	{ OV8858_8BIT, 0x3720 , 0x0a },
	{ OV8858_8BIT, 0x3721 , 0x0a },
	{ OV8858_8BIT, 0x3724 , 0x0c },
	{ OV8858_8BIT, 0x3725 , 0x02 },
	{ OV8858_8BIT, 0x3726 , 0x0c },
	{ OV8858_8BIT, 0x3728 , 0x0a },
	{ OV8858_8BIT, 0x3729 , 0x03 },
	{ OV8858_8BIT, 0x372a , 0x06 },
	{ OV8858_8BIT, 0x372b , 0xa6 },
	{ OV8858_8BIT, 0x372c , 0xa6 },
	{ OV8858_8BIT, 0x372d , 0xa6 },
	{ OV8858_8BIT, 0x372e , 0x0c },
	{ OV8858_8BIT, 0x372f , 0x20 },
	{ OV8858_8BIT, 0x3730 , 0x02 },
	{ OV8858_8BIT, 0x3731 , 0x0c },
	{ OV8858_8BIT, 0x3732 , 0x28 },
	{ OV8858_8BIT, 0x3736 , 0x30 },
	{ OV8858_8BIT, 0x373a , 0x0a },
	{ OV8858_8BIT, 0x373b , 0x0b },
	{ OV8858_8BIT, 0x373c , 0x14 },
	{ OV8858_8BIT, 0x373e , 0x06 },
	{ OV8858_8BIT, 0x375a , 0x0c },
	{ OV8858_8BIT, 0x375b , 0x26 },
	{ OV8858_8BIT, 0x375d , 0x04 },
	{ OV8858_8BIT, 0x375f , 0x28 },
	{ OV8858_8BIT, 0x3768 , 0x22 },
	{ OV8858_8BIT, 0x3769 , 0x44 },
	{ OV8858_8BIT, 0x376a , 0x44 },
	{ OV8858_8BIT, 0x3772 , 0x46 },
	{ OV8858_8BIT, 0x3773 , 0x04 },
	{ OV8858_8BIT, 0x3774 , 0x2c },
	{ OV8858_8BIT, 0x3775 , 0x13 },
	{ OV8858_8BIT, 0x3776 , 0x08 },
	{ OV8858_8BIT, 0x3778 , 0x16 },
	{ OV8858_8BIT, 0x37a0 , 0x88 },
	{ OV8858_8BIT, 0x37a1 , 0x7a },
	{ OV8858_8BIT, 0x37a2 , 0x7a },
	{ OV8858_8BIT, 0x37a7 , 0x88 },
	{ OV8858_8BIT, 0x37a8 , 0x98 },
	{ OV8858_8BIT, 0x37a9 , 0x98 },
	{ OV8858_8BIT, 0x37aa , 0x88 },
	{ OV8858_8BIT, 0x37ab , 0x5c },
	{ OV8858_8BIT, 0x37ac , 0x5c },
	{ OV8858_8BIT, 0x37ad , 0x55 },
	{ OV8858_8BIT, 0x37ae , 0x19 },
	{ OV8858_8BIT, 0x37af , 0x19 },
	{ OV8858_8BIT, 0x37b3 , 0x84 },
	{ OV8858_8BIT, 0x37b4 , 0x84 },
	{ OV8858_8BIT, 0x37b5 , 0x66 },
	{ OV8858_8BIT, 0x3808 , 0x08 }, 
	{ OV8858_8BIT, 0x3809 , 0x10 },
	{ OV8858_8BIT, 0x380a , 0x06 }, 
	{ OV8858_8BIT, 0x380b , 0x10 },
	{ OV8858_8BIT, 0x380c , 0x0a },
	{ OV8858_8BIT, 0x380d , 0x06 },
	{ OV8858_8BIT, 0x380e , 0x07 },
	{ OV8858_8BIT, 0x380f , 0x50 },
	{ OV8858_8BIT, 0x3814 , 0x01 },
	{ OV8858_8BIT, 0x3821 , 0x40 },
	{ OV8858_8BIT, 0x382a , 0x01 },
	{ OV8858_8BIT, 0x382b , 0x01 },
	{ OV8858_8BIT, 0x3830 , 0x06 },
	{ OV8858_8BIT, 0x3836 , 0x01 },
	{ OV8858_8BIT, 0x3f08 , 0x10 },
	{ OV8858_8BIT, 0x4001 , 0x00 },
	{ OV8858_8BIT, 0x4022 , 0x07 },
	{ OV8858_8BIT, 0x4023 , 0xad },
	{ OV8858_8BIT, 0x4024 , 0x08 },
	{ OV8858_8BIT, 0x4025 , 0x1E },
	{ OV8858_8BIT, 0x4026 , 0x08 },
	{ OV8858_8BIT, 0x4027 , 0x1F },
	{ OV8858_8BIT, 0x402a , 0x04 },
	{ OV8858_8BIT, 0x402b , 0x08 },
	{ OV8858_8BIT, 0x402c , 0x02 },
	{ OV8858_8BIT, 0x402d , 0x02 },
	{ OV8858_8BIT, 0x402e , 0x0c },
	{ OV8858_8BIT, 0x4600 , 0x01 },
	{ OV8858_8BIT, 0x4601 , 0x00 },
	{ OV8858_8BIT, 0x5901 , 0x00 },
	{ OV8858_TOK_TERM, 0, 0}
};

static struct ov8858_reg const ov8858_5M_STILL[] = {
	/* Ouput Size 2576x1936 */
	{ OV8858_8BIT, 0x030f, 0x04 },
	{ OV8858_8BIT, 0x3501, 0x9a },
	{ OV8858_8BIT, 0x3502, 0x20 },
	{ OV8858_8BIT, 0x3508, 0x02 },
	{ OV8858_8BIT, 0x3700, 0x30 },
	{ OV8858_8BIT, 0x3701, 0x18 },
	{ OV8858_8BIT, 0x3702, 0x50 },
	{ OV8858_8BIT, 0x3703, 0x32 },
	{ OV8858_8BIT, 0x3704, 0x28 },
	{ OV8858_8BIT, 0x3706, 0x6a },
	{ OV8858_8BIT, 0x3707, 0x08 },
	{ OV8858_8BIT, 0x3708, 0x48 },
	{ OV8858_8BIT, 0x3709, 0x66 },
	{ OV8858_8BIT, 0x370a, 0x01 },
	{ OV8858_8BIT, 0x370b, 0x6a },
	{ OV8858_8BIT, 0x370c, 0x07 },
	{ OV8858_8BIT, 0x3718, 0x14 },
	{ OV8858_8BIT, 0x3712, 0x44 },
	{ OV8858_8BIT, 0x371e, 0x31 },
	{ OV8858_8BIT, 0x371f, 0x7f },
	{ OV8858_8BIT, 0x3720, 0x0a },
	{ OV8858_8BIT, 0x3721, 0x0a },
	{ OV8858_8BIT, 0x3724, 0x0c },
	{ OV8858_8BIT, 0x3725, 0x02 },
	{ OV8858_8BIT, 0x3726, 0x0c },
	{ OV8858_8BIT, 0x3728, 0x0a },
	{ OV8858_8BIT, 0x3729, 0x03 },
	{ OV8858_8BIT, 0x372a, 0x06 },
	{ OV8858_8BIT, 0x372b, 0xa6 },
	{ OV8858_8BIT, 0x372c, 0xa6 },
	{ OV8858_8BIT, 0x372d, 0xa6 },
	{ OV8858_8BIT, 0x372e, 0x0c },
	{ OV8858_8BIT, 0x372f, 0x20 },
	{ OV8858_8BIT, 0x3730, 0x02 },
	{ OV8858_8BIT, 0x3731, 0x0c },
	{ OV8858_8BIT, 0x3732, 0x28 },
	{ OV8858_8BIT, 0x3736, 0x30 },
	{ OV8858_8BIT, 0x373a, 0x0a },
	{ OV8858_8BIT, 0x373b, 0x0b },
	{ OV8858_8BIT, 0x373c, 0x14 },
	{ OV8858_8BIT, 0x373e, 0x06 },
	{ OV8858_8BIT, 0x375a, 0x0c },
	{ OV8858_8BIT, 0x375b, 0x26 },
	{ OV8858_8BIT, 0x375d, 0x04 },
	{ OV8858_8BIT, 0x375f, 0x28 },
	{ OV8858_8BIT, 0x3768, 0x22 },
	{ OV8858_8BIT, 0x3769, 0x44 },
	{ OV8858_8BIT, 0x376a, 0x44 },
	{ OV8858_8BIT, 0x3772, 0x46 },
	{ OV8858_8BIT, 0x3773, 0x04 },
	{ OV8858_8BIT, 0x3774, 0x2c },
	{ OV8858_8BIT, 0x3775, 0x13 },
	{ OV8858_8BIT, 0x3776, 0x08 },
	{ OV8858_8BIT, 0x3778, 0x16 },
	{ OV8858_8BIT, 0x37a0, 0x88 },
	{ OV8858_8BIT, 0x37a1, 0x7a },
	{ OV8858_8BIT, 0x37a2, 0x7a },
	{ OV8858_8BIT, 0x37a7, 0x88 },
	{ OV8858_8BIT, 0x37a8, 0x98 },
	{ OV8858_8BIT, 0x37a9, 0x98 },
	{ OV8858_8BIT, 0x37aa, 0x88 },
	{ OV8858_8BIT, 0x37ab, 0x5c },
	{ OV8858_8BIT, 0x37ac, 0x5c },
	{ OV8858_8BIT, 0x37ad, 0x55 },
	{ OV8858_8BIT, 0x37ae, 0x19 },
	{ OV8858_8BIT, 0x37af, 0x19 },
	{ OV8858_8BIT, 0x37b3, 0x84 },
	{ OV8858_8BIT, 0x37b4, 0x84 },
	{ OV8858_8BIT, 0x37b5, 0x66 },
	{ OV8858_8BIT, 0x3808, 0x0a }, 
	{ OV8858_8BIT, 0x3809, 0x10 },
	{ OV8858_8BIT, 0x380a, 0x07 }, 
	{ OV8858_8BIT, 0x380b, 0x90 },
	{ OV8858_8BIT, 0x380c, 0x0d }, 
	{ OV8858_8BIT, 0x380d, 0xbc }, 
	{ OV8858_8BIT, 0x380e, 0x0a }, 
	{ OV8858_8BIT, 0x380f, 0xaa }, 
	{ OV8858_8BIT, 0x3814, 0x01 },
	{ OV8858_8BIT, 0x3821, 0x40 },
	{ OV8858_8BIT, 0x382a, 0x01 },
	{ OV8858_8BIT, 0x382b, 0x01 },
	{ OV8858_8BIT, 0x3830, 0x06 },
	{ OV8858_8BIT, 0x3836, 0x01 },
	{ OV8858_8BIT, 0x3f08, 0x10 },
	{ OV8858_8BIT, 0x4001, 0x00 },
	{ OV8858_8BIT, 0x4022, 0x09 },
	{ OV8858_8BIT, 0x4023, 0xad },
	{ OV8858_8BIT, 0x4024, 0x0a },
	{ OV8858_8BIT, 0x4025, 0x1E },
	{ OV8858_8BIT, 0x4026, 0x0A },
	{ OV8858_8BIT, 0x4027, 0x1F },
	{ OV8858_8BIT, 0x402a, 0x04 },
	{ OV8858_8BIT, 0x402b, 0x08 },
	{ OV8858_8BIT, 0x402c, 0x02 },
	{ OV8858_8BIT, 0x402d, 0x02 },
	{ OV8858_8BIT, 0x402e, 0x0c },
	{ OV8858_8BIT, 0x4600, 0x01 },
	{ OV8858_8BIT, 0x4601, 0x40 },
	{ OV8858_8BIT, 0x5901, 0x00 },
	{ OV8858_TOK_TERM, 0, 0}
};


static  struct ov8858_reg const ov8858_8M_STILL[] = {
	{ OV8858_8BIT, 0x030f, 0x04},
	{ OV8858_8BIT, 0x3501, 0x9a},
	{ OV8858_8BIT, 0x3502, 0x20},
	{ OV8858_8BIT, 0x3508, 0x02},
	{ OV8858_8BIT, 0x3700, 0x30},
	{ OV8858_8BIT, 0x3701, 0x18},
	{ OV8858_8BIT, 0x3702, 0x50},
	{ OV8858_8BIT, 0x3703, 0x32},
	{ OV8858_8BIT, 0x3704, 0x28},
	{ OV8858_8BIT, 0x3706, 0x6a},
	{ OV8858_8BIT, 0x3707, 0x08},
	{ OV8858_8BIT, 0x3708, 0x48},
	{ OV8858_8BIT, 0x3709, 0x66},
	{ OV8858_8BIT, 0x370a, 0x01},
	{ OV8858_8BIT, 0x370b, 0x6a},
	{ OV8858_8BIT, 0x370c, 0x07},
	{ OV8858_8BIT, 0x3718, 0x14},
	{ OV8858_8BIT, 0x3712, 0x44},
	{ OV8858_8BIT, 0x371e, 0x31},
	{ OV8858_8BIT, 0x371f, 0x7f},
	{ OV8858_8BIT, 0x3720, 0x0a},
	{ OV8858_8BIT, 0x3721, 0x0a},
	{ OV8858_8BIT, 0x3724, 0x0c},
	{ OV8858_8BIT, 0x3725, 0x02},
	{ OV8858_8BIT, 0x3726, 0x0c},
	{ OV8858_8BIT, 0x3728, 0x0a},
	{ OV8858_8BIT, 0x3729, 0x03},
	{ OV8858_8BIT, 0x372a, 0x06},
	{ OV8858_8BIT, 0x372b, 0xa6},
	{ OV8858_8BIT, 0x372c, 0xa6},
	{ OV8858_8BIT, 0x372d, 0xa6},
	{ OV8858_8BIT, 0x372e, 0x0c},
	{ OV8858_8BIT, 0x372f, 0x20},
	{ OV8858_8BIT, 0x3730, 0x02},
	{ OV8858_8BIT, 0x3731, 0x0c},
	{ OV8858_8BIT, 0x3732, 0x28},
	{ OV8858_8BIT, 0x3736, 0x30},
	{ OV8858_8BIT, 0x373a, 0x0a},
	{ OV8858_8BIT, 0x373b, 0x0b},
	{ OV8858_8BIT, 0x373c, 0x14},
	{ OV8858_8BIT, 0x373e, 0x06},
	{ OV8858_8BIT, 0x375a, 0x0c},
	{ OV8858_8BIT, 0x375b, 0x26},
	{ OV8858_8BIT, 0x375d, 0x04},
	{ OV8858_8BIT, 0x375f, 0x28},
	{ OV8858_8BIT, 0x3768, 0x22},
	{ OV8858_8BIT, 0x3769, 0x44},
	{ OV8858_8BIT, 0x376a, 0x44},
	{ OV8858_8BIT, 0x3772, 0x46},
	{ OV8858_8BIT, 0x3773, 0x04},
	{ OV8858_8BIT, 0x3774, 0x2c},
	{ OV8858_8BIT, 0x3775, 0x13},
	{ OV8858_8BIT, 0x3776, 0x08},
	{ OV8858_8BIT, 0x3778, 0x16},
	{ OV8858_8BIT, 0x37a0, 0x88},
	{ OV8858_8BIT, 0x37a1, 0x7a},
	{ OV8858_8BIT, 0x37a2, 0x7a},
	{ OV8858_8BIT, 0x37a7, 0x88},
	{ OV8858_8BIT, 0x37a8, 0x98},
	{ OV8858_8BIT, 0x37a9, 0x98},
	{ OV8858_8BIT, 0x37aa, 0x88},
	{ OV8858_8BIT, 0x37ab, 0x5c},
	{ OV8858_8BIT, 0x37ac, 0x5c},
	{ OV8858_8BIT, 0x37ad, 0x55},
	{ OV8858_8BIT, 0x37ae, 0x19},
	{ OV8858_8BIT, 0x37af, 0x19},
	{ OV8858_8BIT, 0x37b3, 0x84},
	{ OV8858_8BIT, 0x37b4, 0x84},
	{ OV8858_8BIT, 0x37b5, 0x66},
	{ OV8858_8BIT, 0x3808, 0x0c}, 
	{ OV8858_8BIT, 0x3809, 0xd0}, 
	{ OV8858_8BIT, 0x380a, 0x09}, 
	{ OV8858_8BIT, 0x380b, 0xa0},
	{ OV8858_8BIT, 0x380c, 0x0d}, 
	{ OV8858_8BIT, 0x380d, 0xbc}, 
	{ OV8858_8BIT, 0x380e, 0x0a}, 
	{ OV8858_8BIT, 0x380f, 0xaa}, 
	{ OV8858_8BIT, 0x3814, 0x01},
	{ OV8858_8BIT, 0x3821, 0x40},
	{ OV8858_8BIT, 0x382a, 0x01},
	{ OV8858_8BIT, 0x382b, 0x01},
	{ OV8858_8BIT, 0x3830, 0x06},
	{ OV8858_8BIT, 0x3836, 0x01},
	{ OV8858_8BIT, 0x3f08, 0x10},
	{ OV8858_8BIT, 0x4001, 0x00},
	{ OV8858_8BIT, 0x4022, 0x0c},//change by annie on 0507
	{ OV8858_8BIT, 0x4023, 0x6d},
	{ OV8858_8BIT, 0x4024, 0x0c},
	{ OV8858_8BIT, 0x4025, 0xde},
	{ OV8858_8BIT, 0x4026, 0x0c},
	{ OV8858_8BIT, 0x4027, 0xdf},
	{ OV8858_8BIT, 0x402a, 0x04},
	{ OV8858_8BIT, 0x402b, 0x08},
	{ OV8858_8BIT, 0x402c, 0x02},
	{ OV8858_8BIT, 0x402d, 0x02},
	{ OV8858_8BIT, 0x402e, 0x0c},
	{ OV8858_8BIT, 0x4600, 0x01},
	{ OV8858_8BIT, 0x4601, 0x98},
	{ OV8858_8BIT, 0x5901, 0x00}, /* Output size 3280x2464 */
	{ OV8858_TOK_TERM, 0, 0}
};
/*****************************OV8858 PREVIEW********************************/
static struct ov8858_reg const ov8858_480p[] = {
	/*outputsize 480p=736x496*/
	{ OV8858_8BIT, 0x030f, 0x09},
	{ OV8858_8BIT, 0x3501, 0x27},
	{ OV8858_8BIT, 0x3502, 0x80},
	{ OV8858_8BIT, 0x3508, 0x04},
	{ OV8858_8BIT, 0x3700, 0x18},
	{ OV8858_8BIT, 0x3701, 0x0c},
	{ OV8858_8BIT, 0x3702, 0x28},
	{ OV8858_8BIT, 0x3703, 0x19},
	{ OV8858_8BIT, 0x3704, 0x14},
	{ OV8858_8BIT, 0x3706, 0x35},
	{ OV8858_8BIT, 0x3707, 0x04},
	{ OV8858_8BIT, 0x3708, 0x24},
	{ OV8858_8BIT, 0x3709, 0x33},
	{ OV8858_8BIT, 0x370a, 0x00},
	{ OV8858_8BIT, 0x370b, 0xb5},
	{ OV8858_8BIT, 0x370c, 0x04},
	{ OV8858_8BIT, 0x3718, 0x12},
	{ OV8858_8BIT, 0x3712, 0x42},
	{ OV8858_8BIT, 0x371e, 0x19},
	{ OV8858_8BIT, 0x371f, 0x40},
	{ OV8858_8BIT, 0x3720, 0x05},
	{ OV8858_8BIT, 0x3721, 0x05},
	{ OV8858_8BIT, 0x3724, 0x06},
	{ OV8858_8BIT, 0x3725, 0x01},
	{ OV8858_8BIT, 0x3726, 0x06},
	{ OV8858_8BIT, 0x3728, 0x05},
	{ OV8858_8BIT, 0x3729, 0x02},
	{ OV8858_8BIT, 0x372a, 0x03},
	{ OV8858_8BIT, 0x372b, 0x53},
	{ OV8858_8BIT, 0x372c, 0xa3},
	{ OV8858_8BIT, 0x372d, 0x53},
	{ OV8858_8BIT, 0x372e, 0x06},
	{ OV8858_8BIT, 0x372f, 0x10},
	{ OV8858_8BIT, 0x3730, 0x01},
	{ OV8858_8BIT, 0x3731, 0x06},
	{ OV8858_8BIT, 0x3732, 0x14},
	{ OV8858_8BIT, 0x3736, 0x20},
	{ OV8858_8BIT, 0x373a, 0x05},
	{ OV8858_8BIT, 0x373b, 0x06},
	{ OV8858_8BIT, 0x373c, 0x0a},
	{ OV8858_8BIT, 0x373e, 0x03},
	{ OV8858_8BIT, 0x375a, 0x06},
	{ OV8858_8BIT, 0x375b, 0x13},
	{ OV8858_8BIT, 0x375d, 0x02},
	{ OV8858_8BIT, 0x375f, 0x14},
	{ OV8858_8BIT, 0x3768, 0x00},
	{ OV8858_8BIT, 0x3769, 0xc0},
	{ OV8858_8BIT, 0x376a, 0x42},
	{ OV8858_8BIT, 0x3772, 0x23},
	{ OV8858_8BIT, 0x3773, 0x02},
	{ OV8858_8BIT, 0x3774, 0x16},
	{ OV8858_8BIT, 0x3775, 0x12},
	{ OV8858_8BIT, 0x3776, 0x04},
	{ OV8858_8BIT, 0x3778, 0x17},
	{ OV8858_8BIT, 0x37a0, 0x44},
	{ OV8858_8BIT, 0x37a1, 0x3d},
	{ OV8858_8BIT, 0x37a2, 0x3d},
	{ OV8858_8BIT, 0x37a7, 0x44},
	{ OV8858_8BIT, 0x37a8, 0x4c},
	{ OV8858_8BIT, 0x37a9, 0x4c},
	{ OV8858_8BIT, 0x37aa, 0x44},
	{ OV8858_8BIT, 0x37ab, 0x2e},
	{ OV8858_8BIT, 0x37ac, 0x2e},
	{ OV8858_8BIT, 0x37ad, 0x33},
	{ OV8858_8BIT, 0x37ae, 0x0d},
	{ OV8858_8BIT, 0x37af, 0x0d},
	{ OV8858_8BIT, 0x37b3, 0x42},
	{ OV8858_8BIT, 0x37b4, 0x42},
	{ OV8858_8BIT, 0x37b5, 0x33},
	{ OV8858_8BIT, 0x3808, 0x02},
	{ OV8858_8BIT, 0x3809, 0xe0},
	{ OV8858_8BIT, 0x380a, 0x01},
	{ OV8858_8BIT, 0x380b, 0xf0},
	{ OV8858_8BIT, 0x380c, 0x04},
	{ OV8858_8BIT, 0x380d, 0xe2},
	{ OV8858_8BIT, 0x380e, 0x07},
	{ OV8858_8BIT, 0x380f, 0x80},
	{ OV8858_8BIT, 0x3814, 0x03},
	{ OV8858_8BIT, 0x3821, 0x6f},
	{ OV8858_8BIT, 0x382a, 0x05},
	{ OV8858_8BIT, 0x382b, 0x03},
	{ OV8858_8BIT, 0x3830, 0x0c},
	{ OV8858_8BIT, 0x3836, 0x02},
	{ OV8858_8BIT, 0x3f08, 0x08},
	{ OV8858_8BIT, 0x3f0a, 0x80},
	{ OV8858_8BIT, 0x4001, 0x10},
	{ OV8858_8BIT, 0x4022, 0x04},
	{ OV8858_8BIT, 0x4023, 0x00},
	{ OV8858_8BIT, 0x4024, 0x05},
	{ OV8858_8BIT, 0x4025, 0x2E},
	{ OV8858_8BIT, 0x4026, 0x05},
	{ OV8858_8BIT, 0x4027, 0x2F},
	{ OV8858_8BIT, 0x402a, 0x02},
	{ OV8858_8BIT, 0x402b, 0x04},
	{ OV8858_8BIT, 0x402c, 0x00},
	{ OV8858_8BIT, 0x402d, 0x00},
	{ OV8858_8BIT, 0x402e, 0x06},
	{ OV8858_8BIT, 0x4500, 0x38},
	{ OV8858_8BIT, 0x4600, 0x00},
	{ OV8858_8BIT, 0x4601, 0x50},
	{ OV8858_8BIT, 0x5901, 0x04},
	{ OV8858_8BIT, 0x382d, 0x7f},
	{ OV8858_TOK_TERM, 0, 0}

};
static struct ov8858_reg const ov8858_cif_strong_dvs[] = {
	/*outputsize cif=352x288*/
	{ OV8858_8BIT, 0x030f , 0x09},
	{ OV8858_8BIT, 0x3501 , 0x27},
	{ OV8858_8BIT, 0x3502 , 0x80},
	{ OV8858_8BIT, 0x3508 , 0x04},
	{ OV8858_8BIT, 0x3700 , 0x18},
	{ OV8858_8BIT, 0x3701 , 0x0c},
	{ OV8858_8BIT, 0x3702 , 0x28},
	{ OV8858_8BIT, 0x3703 , 0x19},
	{ OV8858_8BIT, 0x3704 , 0x14},
	{ OV8858_8BIT, 0x3706 , 0x35},
	{ OV8858_8BIT, 0x3707 , 0x04},
	{ OV8858_8BIT, 0x3708 , 0x24},
	{ OV8858_8BIT, 0x3709 , 0x33},
	{ OV8858_8BIT, 0x370a , 0x00},
	{ OV8858_8BIT, 0x370b , 0xb5},
	{ OV8858_8BIT, 0x370c , 0x04},
	{ OV8858_8BIT, 0x3718 , 0x12},
	{ OV8858_8BIT, 0x3712 , 0x42},
	{ OV8858_8BIT, 0x371e , 0x19},
	{ OV8858_8BIT, 0x371f , 0x40},
	{ OV8858_8BIT, 0x3720 , 0x05},
	{ OV8858_8BIT, 0x3721 , 0x05},
	{ OV8858_8BIT, 0x3724 , 0x06},
	{ OV8858_8BIT, 0x3725 , 0x01},
	{ OV8858_8BIT, 0x3726 , 0x06},
	{ OV8858_8BIT, 0x3728 , 0x05},
	{ OV8858_8BIT, 0x3729 , 0x02},
	{ OV8858_8BIT, 0x372a , 0x03},
	{ OV8858_8BIT, 0x372b , 0x53},
	{ OV8858_8BIT, 0x372c , 0xa3},
	{ OV8858_8BIT, 0x372d , 0x53},
	{ OV8858_8BIT, 0x372e , 0x06},
	{ OV8858_8BIT, 0x372f , 0x10},
	{ OV8858_8BIT, 0x3730 , 0x01},
	{ OV8858_8BIT, 0x3731 , 0x06},
	{ OV8858_8BIT, 0x3732 , 0x14},
	{ OV8858_8BIT, 0x3736 , 0x20},
	{ OV8858_8BIT, 0x373a , 0x05},
	{ OV8858_8BIT, 0x373b , 0x06},
	{ OV8858_8BIT, 0x373c , 0x0a},
	{ OV8858_8BIT, 0x373e , 0x03},
	{ OV8858_8BIT, 0x375a , 0x06},
	{ OV8858_8BIT, 0x375b , 0x13},
	{ OV8858_8BIT, 0x375d , 0x02},
	{ OV8858_8BIT, 0x375f , 0x14},
	{ OV8858_8BIT, 0x3768 , 0x00},
	{ OV8858_8BIT, 0x3769 , 0xc0},
	{ OV8858_8BIT, 0x376a , 0x42},
	{ OV8858_8BIT, 0x3772 , 0x23},
	{ OV8858_8BIT, 0x3773 , 0x02},
	{ OV8858_8BIT, 0x3774 , 0x16},
	{ OV8858_8BIT, 0x3775 , 0x12},
	{ OV8858_8BIT, 0x3776 , 0x04},
	{ OV8858_8BIT, 0x3778 , 0x17},
	{ OV8858_8BIT, 0x37a0 , 0x44},
	{ OV8858_8BIT, 0x37a1 , 0x3d},
	{ OV8858_8BIT, 0x37a2 , 0x3d},
	{ OV8858_8BIT, 0x37a7 , 0x44},
	{ OV8858_8BIT, 0x37a8 , 0x4c},
	{ OV8858_8BIT, 0x37a9 , 0x4c},
	{ OV8858_8BIT, 0x37aa , 0x44},
	{ OV8858_8BIT, 0x37ab , 0x2e},
	{ OV8858_8BIT, 0x37ac , 0x2e},
	{ OV8858_8BIT, 0x37ad , 0x33},
	{ OV8858_8BIT, 0x37ae , 0x0d},
	{ OV8858_8BIT, 0x37af , 0x0d},
	{ OV8858_8BIT, 0x37b3 , 0x42},
	{ OV8858_8BIT, 0x37b4 , 0x42},
	{ OV8858_8BIT, 0x37b5 , 0x33},
	{ OV8858_8BIT, 0x3808 , 0x01},
	{ OV8858_8BIT, 0x3809 , 0x60},
	{ OV8858_8BIT, 0x380a , 0x01},
	{ OV8858_8BIT, 0x380b , 0x20},
	{ OV8858_8BIT, 0x380c , 0x04},
	{ OV8858_8BIT, 0x380d , 0xe2},
	{ OV8858_8BIT, 0x380e , 0x07},
	{ OV8858_8BIT, 0x380f , 0x80},
	{ OV8858_8BIT, 0x3814 , 0x03},
	{ OV8858_8BIT, 0x3821 , 0x69},
	{ OV8858_8BIT, 0x382a , 0x05},
	{ OV8858_8BIT, 0x382b , 0x03},
	{ OV8858_8BIT, 0x3830 , 0x0c},
	{ OV8858_8BIT, 0x3836 , 0x02},
	{ OV8858_8BIT, 0x3f08 , 0x08},
	{ OV8858_8BIT, 0x3f0a , 0x80},
	{ OV8858_8BIT, 0x4001 , 0x10},
	{ OV8858_8BIT, 0x4022 , 0x02},
	{ OV8858_8BIT, 0x4023 , 0x5d},
	{ OV8858_8BIT, 0x4024 , 0x02},
	{ OV8858_8BIT, 0x4025 , 0xCE},
	{ OV8858_8BIT, 0x4026 , 0x02},
	{ OV8858_8BIT, 0x4027 , 0xCf},
	{ OV8858_8BIT, 0x402a , 0x02},
	{ OV8858_8BIT, 0x402b , 0x04},
	{ OV8858_8BIT, 0x402c , 0x00},
	{ OV8858_8BIT, 0x402d , 0x00},
	{ OV8858_8BIT, 0x402e , 0x06},
	{ OV8858_8BIT, 0x4500 , 0x38},
	{ OV8858_8BIT, 0x4600 , 0x00},
	{ OV8858_8BIT, 0x4601 , 0x2a},
	{ OV8858_8BIT, 0x5901 , 0x04},
	{ OV8858_8BIT, 0x382d , 0x7f},
	{ OV8858_TOK_TERM, 0, 0}
};

static struct ov8858_reg const ov8858_960p_strong_dvs[] = {
	/*output size 1296x976*/
	{ OV8858_8BIT,  0x030f , 0x09},
	{ OV8858_8BIT,  0x3501 , 0x4d},
	{ OV8858_8BIT,  0x3502 , 0x40},
	{ OV8858_8BIT,  0x3508 , 0x04},
	{ OV8858_8BIT,  0x3700 , 0x18},
	{ OV8858_8BIT,  0x3701 , 0x0c},
	{ OV8858_8BIT,  0x3702 , 0x28},
	{ OV8858_8BIT,  0x3703 , 0x19},
	{ OV8858_8BIT,  0x3704 , 0x14},
	{ OV8858_8BIT,  0x3706 , 0x35},
	{ OV8858_8BIT,  0x3707 , 0x04},
	{ OV8858_8BIT,  0x3708 , 0x24},
	{ OV8858_8BIT,  0x3709 , 0x33},
	{ OV8858_8BIT,  0x370a , 0x00},
	{ OV8858_8BIT,  0x370b , 0xb5},
	{ OV8858_8BIT,  0x370c , 0x04},
	{ OV8858_8BIT,  0x3718 , 0x12},
	{ OV8858_8BIT,  0x3712 , 0x42},
	{ OV8858_8BIT,  0x371e , 0x19},
	{ OV8858_8BIT,  0x371f , 0x40},
	{ OV8858_8BIT,  0x3720 , 0x05},
	{ OV8858_8BIT,  0x3721 , 0x05},
	{ OV8858_8BIT,  0x3724 , 0x06},
	{ OV8858_8BIT,  0x3725 , 0x01},
	{ OV8858_8BIT,  0x3726 , 0x06},
	{ OV8858_8BIT,  0x3728 , 0x05},
	{ OV8858_8BIT,  0x3729 , 0x02},
	{ OV8858_8BIT,  0x372a , 0x03},
	{ OV8858_8BIT,  0x372b , 0x53},
	{ OV8858_8BIT,  0x372c , 0xa3},
	{ OV8858_8BIT,  0x372d , 0x53},
	{ OV8858_8BIT,  0x372e , 0x06},
	{ OV8858_8BIT,  0x372f , 0x10},
	{ OV8858_8BIT,  0x3730 , 0x01},
	{ OV8858_8BIT,  0x3731 , 0x06},
	{ OV8858_8BIT,  0x3732 , 0x14},
	{ OV8858_8BIT,  0x3736 , 0x20},
	{ OV8858_8BIT,  0x373a , 0x05},
	{ OV8858_8BIT,  0x373b , 0x06},
	{ OV8858_8BIT,  0x373c , 0x0a},
	{ OV8858_8BIT,  0x373e , 0x03},
	{ OV8858_8BIT,  0x375a , 0x06},
	{ OV8858_8BIT,  0x375b , 0x13},
	{ OV8858_8BIT,  0x375d , 0x02},
	{ OV8858_8BIT,  0x375f , 0x14},
	{ OV8858_8BIT,  0x3768 , 0x22},
	{ OV8858_8BIT,  0x3769 , 0x44},
	{ OV8858_8BIT,  0x376a , 0x44},
	{ OV8858_8BIT,  0x3772 , 0x23},
	{ OV8858_8BIT,  0x3773 , 0x02},
	{ OV8858_8BIT,  0x3774 , 0x16},
	{ OV8858_8BIT,  0x3775 , 0x12},
	{ OV8858_8BIT,  0x3776 , 0x04},
	{ OV8858_8BIT,  0x3778 , 0x1b},
	{ OV8858_8BIT,  0x37a0 , 0x44},
	{ OV8858_8BIT,  0x37a1 , 0x3d},
	{ OV8858_8BIT,  0x37a2 , 0x3d},
	{ OV8858_8BIT,  0x37a7 , 0x44},
	{ OV8858_8BIT,  0x37a8 , 0x4c},
	{ OV8858_8BIT,  0x37a9 , 0x4c},
	{ OV8858_8BIT,  0x37aa , 0x44},
	{ OV8858_8BIT,  0x37ab , 0x2e},
	{ OV8858_8BIT,  0x37ac , 0x2e},
	{ OV8858_8BIT,  0x37ad , 0x33},
	{ OV8858_8BIT,  0x37ae , 0x0d},
	{ OV8858_8BIT,  0x37af , 0x0d},
	{ OV8858_8BIT,  0x37b3 , 0x42},
	{ OV8858_8BIT,  0x37b4 , 0x42},
	{ OV8858_8BIT,  0x37b5 , 0x33},
	{ OV8858_8BIT,  0x3808 , 0x05},
	{ OV8858_8BIT,  0x3809 , 0x10},
	{ OV8858_8BIT,  0x380a , 0x03},
	{ OV8858_8BIT,  0x380b , 0xd0},
	{ OV8858_8BIT,  0x380c , 0x07}, 
	{ OV8858_8BIT,  0x380d , 0x88}, 
	{ OV8858_8BIT,  0x380e , 0x04},
	{ OV8858_8BIT,  0x380f , 0xdc},
	{ OV8858_8BIT,  0x3814 , 0x03},
	{ OV8858_8BIT,  0x3821 , 0x61},
	{ OV8858_8BIT,  0x382a , 0x03},
	{ OV8858_8BIT,  0x382b , 0x01},
	{ OV8858_8BIT,  0x3830 , 0x08},
	{ OV8858_8BIT,  0x3836 , 0x02},
	{ OV8858_8BIT,  0x3f08 , 0x08},
	{ OV8858_8BIT,  0x3f0a , 0x80},
	{ OV8858_8BIT,  0x4001 , 0x10},
	{ OV8858_8BIT,  0x4022 , 0x04},
	{ OV8858_8BIT,  0x4023 , 0xbd},
	{ OV8858_8BIT,  0x4024 , 0x05},
	{ OV8858_8BIT,  0x4025 , 0x2E},
	{ OV8858_8BIT,  0x4026 , 0x05},
	{ OV8858_8BIT,  0x4027 , 0x2F},
	{ OV8858_8BIT,  0x402a , 0x04},
	{ OV8858_8BIT,  0x402b , 0x04},
	{ OV8858_8BIT,  0x402c , 0x02},
	{ OV8858_8BIT,  0x402d , 0x02},
	{ OV8858_8BIT,  0x402e , 0x08},
	{ OV8858_8BIT,  0x4500 , 0x38},
	{ OV8858_8BIT,  0x4600 , 0x00},
	{ OV8858_8BIT,  0x4601 , 0xa0},
	{ OV8858_8BIT,  0x5901 , 0x00},
	{ OV8858_8BIT,  0x382d , 0x7f},
	{ OV8858_TOK_TERM, 0, 0}
};

struct ov8858_resolution ov8858_res_preview[] = {
	{
		.desc = "ov8858_cont_cap_vga",
		.width = 656,
		.height = 496,
		.fps = 30,
		.pix_clk_freq = 72,
		.used = 0,
		.pixels_per_line = 1250,
		.lines_per_frame = 1920,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 1,
		.skip_frames = 3,
		.regs = ov8858_VGA_STILL,
	},
	{
		.desc = "ov8858_cont_cap_720P",
		.width = 1296,
		.height = 736,
		.fps = 30,
		.pix_clk_freq = 72,
		.used = 0,
		.pixels_per_line = 1928,
		.lines_per_frame = 1244,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 1,
		.skip_frames = 3,
		.regs = ov8858_cont_cap_720P,
	},
	{
		.desc = "ov8858_cont_cap_1M",
		.width = 1040,
		.height = 784,
		.fps = 30,
		.pix_clk_freq = 72,
		.used = 0,
		.pixels_per_line = 1928,
		.lines_per_frame = 1244,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 1,
		.skip_frames = 3,
		.regs = ov8858_1M_STILL,
	},
	{
		 .desc = "OV8858_PREVIEW1600x1200",
		 .width = 1632,
		 .height = 1224,
		.fps = 30,
		.pix_clk_freq = 72,
		.used = 0,
		.pixels_per_line = 1928,
		.lines_per_frame = 1244,
		.bin_factor_x = 0,
		.bin_factor_y = 0,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = ov8858_2M_STILL,
	},
	{
		.desc = "ov8858_cont_cap_3M",
		.width = 2064,
		.height = 1552,
		.fps = 30,
		.pix_clk_freq = 144,
		.used = 0,
		.pixels_per_line = 2566,
		.lines_per_frame = 1872,
		.bin_factor_x = 0,
		.bin_factor_y = 0,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = ov8858_3M_STILL,
	},
	{
		.desc = "ov8858_cont_cap_5M",
		.width = 2576,//2608,
		.height = 1936,//1960,
		.fps = 15,
		.pix_clk_freq = 144,
		.used = 0,
		.pixels_per_line = 3516,
		.lines_per_frame = 2730,
		.bin_factor_x = 0,
		.bin_factor_y = 0,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = ov8858_5M_STILL,
	},
	{
		.desc = "ov8858_cont_cap_8M",
		.width = 3280,
		.height = 2464,
		.fps = 15,
		.pix_clk_freq = 144,
		.used = 0,
		.pixels_per_line = 3516,
		.lines_per_frame = 2730,
		.bin_factor_x = 0,
		.bin_factor_y = 0,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = ov8858_8M_STILL,
	},
};
#define N_RES_PREVIEW (ARRAY_SIZE(ov8858_res_preview))

struct ov8858_resolution ov8858_res_still[] = {
	{
		.desc = "ov8858_still_vga",
		.width = 656,
		.height = 496,
		.fps = 30,
		.pix_clk_freq = 72,
		.used = 0,
		.pixels_per_line = 1250,
		.lines_per_frame = 1920,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 1,
		.skip_frames = 3,
		.regs = ov8858_VGA_STILL,
	},
	{
		.desc = "ov8858_cont_cap_1M",
		.width = 1040,
		.height = 784,
		.fps = 30,
		.pix_clk_freq = 72,
		.used = 0,
		.pixels_per_line = 1928,
		.lines_per_frame = 1244,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 1,
		.skip_frames = 3,
		.regs = ov8858_1M_STILL,
	},
	{
		.desc = "OV8858_STILL_1600x1200",
		.width = 1632,
		.height = 1224,
		.fps = 30,
		.pix_clk_freq = 72,
		.used = 0,
		.pixels_per_line = 1928,
		.lines_per_frame = 1244,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 1,
		.skip_frames = 3,
		.regs = ov8858_2M_STILL,
	},
	{
		.desc = "ov8858_cont_cap_3M",
		.width = 2064,
		.height = 1552,
		.fps = 30,
		.pix_clk_freq = 144,
		.used = 0,
		.pixels_per_line = 2566,
		.lines_per_frame = 1872,
		.bin_factor_x = 0,
		.bin_factor_y = 0,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = ov8858_3M_STILL,
	},
	{
		.desc = "ov8858_STILL_5M",
		.width = 2576,//2608,
		.height = 1936,//1944,
		.fps = 15,
		.pix_clk_freq = 144,
		.used = 0,
		.pixels_per_line = 3516,
		.lines_per_frame = 2730,
		.bin_factor_x = 0,
		.bin_factor_y = 0,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = ov8858_5M_STILL,
	},
	{
		.desc = "ov8858_STILL_8M",
		.width = 3280,
		.height = 2464,
		.fps = 15,
		.pix_clk_freq = 144,
		.used = 0,
		.pixels_per_line = 3516,
		.lines_per_frame = 2730,
		.bin_factor_x = 0,
		.bin_factor_y = 0,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = ov8858_8M_STILL,
	},
};
#define N_RES_STILL (ARRAY_SIZE(ov8858_res_still))

struct ov8858_resolution ov8858_res_video[] = {
	{
		.desc = "OV8858_CIF",
		.width = 352,
		.height = 288,
		.fps = 30,
		.pix_clk_freq = 72,
		.used = 0,
		.pixels_per_line = 1250,
		.lines_per_frame = 1920,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 1,
		.skip_frames = 3,
		.regs = ov8858_cif_strong_dvs,
	},
	{
		.desc = "OV8858_VGA",
		.width = 656,
		.height = 496,
		.fps = 30,
		.pix_clk_freq = 72,
		.used = 0,
		.pixels_per_line = 1250,
		.lines_per_frame = 1920,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 1,
		.skip_frames = 3,
		.regs = ov8858_VGA_STILL,
	},
	{
		.desc = "ov8858_480p",
		.width = 736,
		.height = 496,
		.fps = 30,
		.pix_clk_freq = 72,
		.used = 0,
		.pixels_per_line = 1250,
		.lines_per_frame = 1920,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 1,
		.skip_frames = 3,
		.regs = ov8858_480p,
	},
	{
		.desc = "ov8858_VIDEO_720P",
		.width = 1296,
		.height = 736,
		.fps = 30,
		.pix_clk_freq = 72,
		.used = 0,
		.pixels_per_line = 1928,
		.lines_per_frame = 1244,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 1,
		.skip_frames = 3,
		.regs = ov8858_cont_cap_720P,
	},
	{
		.desc = "ov8858_VIDEO_1080P",
		.width = 1936,
		.height = 1096,
		.fps = 15,
		.pix_clk_freq = 144,
		.used = 0,
		.pixels_per_line = 3516,
		.lines_per_frame = 2730,
		.bin_factor_x = 0,
		.bin_factor_y = 0,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = ov8858_1080P_STILL,
	},
};
#define N_RES_VIDEO (ARRAY_SIZE(ov8858_res_video))

static struct ov8858_resolution *ov8858_res = ov8858_res_preview;
static int N_RES = N_RES_PREVIEW;


//#define CONFIG_VIDEO_WV511
//#define CONFIG_VIDEO_DW9714
#define CONFIG_VIDEO_VM149
#define WV511  0x0
#define DW9714 0x1
#define VM149  0x2

extern int dw9714_vcm_power_up(struct v4l2_subdev *sd);
extern int dw9714_vcm_power_down(struct v4l2_subdev *sd);
extern int dw9714_vcm_init(struct v4l2_subdev *sd);
extern int dw9714_t_focus_vcm(struct v4l2_subdev *sd, u16 val);
extern int dw9714_t_focus_abs(struct v4l2_subdev *sd, s32 value);
extern int dw9714_t_focus_rel(struct v4l2_subdev *sd, s32 value);
extern int dw9714_q_focus_status(struct v4l2_subdev *sd, s32 *value);
extern int dw9714_q_focus_abs(struct v4l2_subdev *sd, s32 *value);
extern int dw9714_t_vcm_slew(struct v4l2_subdev *sd, s32 value);
extern int dw9714_t_vcm_timing(struct v4l2_subdev *sd, s32 value);

extern int wv511_vcm_power_up(struct v4l2_subdev *sd);
extern int wv511_vcm_power_down(struct v4l2_subdev *sd);
extern int wv511_vcm_init(struct v4l2_subdev *sd);
extern int wv511_t_focus_vcm(struct v4l2_subdev *sd, u16 val);
extern int wv511_t_focus_abs(struct v4l2_subdev *sd, s32 value);
extern int wv511_t_focus_rel(struct v4l2_subdev *sd, s32 value);
extern int wv511_q_focus_status(struct v4l2_subdev *sd, s32 *value);
extern int wv511_q_focus_abs(struct v4l2_subdev *sd, s32 *value);
extern int wv511_t_vcm_slew(struct v4l2_subdev *sd, s32 value);
extern int wv511_t_vcm_timing(struct v4l2_subdev *sd, s32 value);

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


struct ov8858_vcm ov8858_vcms[] = {
	[WV511] = {
		.power_up = wv511_vcm_power_up,
		.power_down = wv511_vcm_power_down,
		.init = wv511_vcm_init,
		.t_focus_vcm = wv511_t_focus_vcm,
		.t_focus_abs = wv511_t_focus_abs,
		.t_focus_rel = wv511_t_focus_rel,
		.q_focus_status = wv511_q_focus_status,
		.q_focus_abs = wv511_q_focus_abs,
		.t_vcm_slew = wv511_t_vcm_slew,
		.t_vcm_timing = wv511_t_vcm_timing,
	},
	[DW9714] = {
		.power_up = dw9714_vcm_power_up,
		.power_down = dw9714_vcm_power_down,
		.init = dw9714_vcm_init,
		.t_focus_vcm = dw9714_t_focus_vcm,
		.t_focus_abs = dw9714_t_focus_abs,
		.t_focus_rel = dw9714_t_focus_rel,
		.q_focus_status = dw9714_q_focus_status,
		.q_focus_abs = dw9714_q_focus_abs,
		.t_vcm_slew = dw9714_t_vcm_slew,
		.t_vcm_timing = dw9714_t_vcm_timing,
	},
	[VM149] = {
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
