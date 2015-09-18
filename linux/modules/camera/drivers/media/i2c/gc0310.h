/*
 * Support for gc0310 Camera Sensor.
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


#ifndef __gc0310_H__
#define __gc0310_H__

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

#define V4L2_IDENT_gc0310 8245

/* #defines for register writes and register array processing */
#define MISENSOR_8BIT		1
#define MISENSOR_16BIT		2
#define MISENSOR_32BIT		4

#define MISENSOR_FWBURST0	0x80
#define MISENSOR_FWBURST1	0x81
#define MISENSOR_FWBURST4	0x84
#define MISENSOR_FWBURST	0x88

#define MISENSOR_TOK_TERM	0xf000	/* terminating token for reg list */
#define MISENSOR_TOK_DELAY	0xfe00	/* delay token for reg list */
#define MISENSOR_TOK_FWLOAD	0xfd00	/* token indicating load FW */
#define MISENSOR_TOK_POLL	0xfc00	/* token indicating poll instruction */
#define MISENSOR_TOK_RMW	0x0010  /* RMW operation */
#define MISENSOR_TOK_MASK	0xfff0
#define MISENSOR_AWB_STEADY	(1<<0)	/* awb steady */
#define MISENSOR_AE_READY	(1<<3)	/* ae status ready */

/* mask to set sensor read_mode via misensor_rmw_reg */
#define MISENSOR_R_MODE_MASK	0x0330
/* mask to set sensor vert_flip and horz_mirror */
#define MISENSOR_VFLIP_MASK	0x0002
#define MISENSOR_HFLIP_MASK	0x0001
#define MISENSOR_FLIP_EN	1
#define MISENSOR_FLIP_DIS	0

/* bits set to set sensor read_mode via misensor_rmw_reg */
#define MISENSOR_SKIPPING_SET	0x0011
#define MISENSOR_SUMMING_SET	0x0033
#define MISENSOR_NORMAL_SET	0x0000

/* sensor register that control sensor read-mode and mirror */
#define MISENSOR_READ_MODE	0xC834
/* sensor ae-track status register */
#define MISENSOR_AE_TRACK_STATUS	0xA800
/* sensor awb status register */
#define MISENSOR_AWB_STATUS	0xAC00
/* sensor coarse integration time register */
#define MISENSOR_COARSE_INTEGRATION_TIME_H 0x03
#define MISENSOR_COARSE_INTEGRATION_TIME_L 0x04

/*registers*/
#define REG_V_START                     0x0a    //  06
#define REG_H_START                     0x0c   //  08
#define REG_WIDTH_H                      0x0f //  0b
#define REG_WIDTH_L                      0x10   //0c
#define REG_HEIGHT_H                      0x0d   //  09
#define REG_HEIGHT_L                      0x0e   // 0a
#define REG_SH_DELAY                     0x11    //12 
#define REG_DUMMY_H                     0x05    //  0f
#define REG_H_DUMMY_L                     0x06   // 01
#define REG_V_DUMMY_H                     0x07
#define REG_V_DUMMY_L                     0x08  //  02 
#define REG_EXPO_COARSE                 0x03   //  03
#define REG_GLOBAL_GAIN                 0x70   // 50 
#define REG_GAIN                        0x71   //51

#define SENSOR_DETECTED		1
#define SENSOR_NOT_DETECTED	0

#define I2C_RETRY_COUNT		5
#define MSG_LEN_OFFSET		1 /*8-bits addr*/

#ifndef MIPI_CONTROL
#define MIPI_CONTROL		0x3400	/* MIPI_Control */
#endif

/*Consistent value*/
#define VALUE_V_START                     0
#define VALUE_H_START                     0
#define VALUE_V_END                       480
#define VALUE_H_END                       640
#define VALUE_V_OUTPUT                       480
#define VALUE_H_OUTPUT                       640
#define VALUE_PIXEL_CLK                   96

/* GPIO pin on Moorestown */
#define GPIO_SCLK_25		44
#define GPIO_STB_PIN		47

#define GPIO_STDBY_PIN		49   /* ab:new */
#define GPIO_RESET_PIN		50

/* System control register for Aptina A-1040SOC*/
#define gc0310_PID		0xf0

/* MT9P111_DEVICE_ID */
#define gc0310_MOD_ID		0xa3

#define gc0310_FINE_INTG_TIME_MIN 0
#define gc0310_FINE_INTG_TIME_MAX_MARGIN 0
#define gc0310_COARSE_INTG_TIME_MIN 1
#define gc0310_COARSE_INTG_TIME_MAX_MARGIN 6
#define GC0310_IMG_ORIENTATION			0x17
#define GC0310_VFLIP_BIT			2
#define GC0310_HFLIP_BIT			1

/* ulBPat; */

#define gc0310_BPAT_RGRGGBGB	(1 << 0)
#define gc0310_BPAT_GRGRBGBG	(1 << 1)
#define gc0310_BPAT_GBGBRGRG	(1 << 2)
#define gc0310_BPAT_BGBGGRGR	(1 << 3)

#define gc0310_FOCAL_LENGTH_NUM	208	/*2.08mm*/
#define gc0310_FOCAL_LENGTH_DEM	100
#define gc0310_F_NUMBER_DEFAULT_NUM	24
#define gc0310_F_NUMBER_DEM	10
#define gc0310_WAIT_STAT_TIMEOUT	100
#define gc0310_FLICKER_MODE_50HZ	1
#define gc0310_FLICKER_MODE_60HZ	2
/*
 * focal length bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define gc0310_FOCAL_LENGTH_DEFAULT 0xD00064

/*
 * current f-number bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define gc0310_F_NUMBER_DEFAULT 0x18000a

/*
 * f-number range bits definition:
 * bits 31-24: max f-number numerator
 * bits 23-16: max f-number denominator
 * bits 15-8: min f-number numerator
 * bits 7-0: min f-number denominator
 */
#define gc0310_F_NUMBER_RANGE 0x180a180a

#define BASE_GLOBAL_GAIN_VALUE 0x60
/* Supported resolutions */
enum {
	gc0310_RES_VGA,
};

#define gc0310_RES_VGA_SIZE_H		640
#define gc0310_RES_VGA_SIZE_V		480
#define gc0310_RES_CIF_SIZE_H		352
#define gc0310_RES_CIF_SIZE_V		288
#define gc0310_RES_QVGA_SIZE_H		320
#define gc0310_RES_QVGA_SIZE_V		240
#define gc0310_RES_QCIF_SIZE_H		176
#define gc0310_RES_QCIF_SIZE_V		144

/* completion status polling requirements, usage based on Aptina .INI Rev2 */
enum poll_reg {
	NO_POLLING,
	PRE_POLLING,
	POST_POLLING,
};
/*
 * struct misensor_reg - MI sensor  register format
 * @length: length of the register
 * @reg: 16-bit offset to register
 * @val: 8/16/32-bit register value
 * Define a structure for sensor register initialization values
 */
struct misensor_reg {
	u32 length;
	u32 reg;
	u32 val;	/* value or for read/mod/write, AND mask */
	u32 val2;	/* optional; for rmw, OR mask */
};

/*
 * struct misensor_fwreg - Firmware burst command
 * @type: FW burst or 8/16 bit register
 * @addr: 16-bit offset to register or other values depending on type
 * @valx: data value for burst (or other commands)
 *
 * Define a structure for sensor register initialization values
 */
struct misensor_fwreg {
	u32	type;	/* type of value, register or FW burst string */
	u32	addr;	/* target address */
	u32	val0;
	u32	val1;
	u32	val2;
	u32	val3;
	u32	val4;
	u32	val5;
	u32	val6;
	u32	val7;
};

struct regval_list {
	u16 reg_num;
	u8 value;
};

struct gc0310_device {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt format;

	struct camera_sensor_platform_data *platform_data;
	int real_model_id;
	int nctx;
	int power;

	unsigned int bus_width;
	unsigned int mode;
	unsigned int field_inv;
	unsigned int field_sel;
	unsigned int ycseq;
	unsigned int conv422;
	unsigned int bpat;
	unsigned int hpol;
	unsigned int vpol;
	unsigned int edge;
	unsigned int bls;
	unsigned int gamma;
	unsigned int cconv;
	unsigned int res;
	unsigned int dwn_sz;
	unsigned int blc;
	unsigned int agc;
	unsigned int awb;
	unsigned int aec;
	/* extention SENSOR version 2 */
	unsigned int cie_profile;

	/* extention SENSOR version 3 */
	unsigned int flicker_freq;

	/* extension SENSOR version 4 */
	unsigned int smia_mode;
	unsigned int mipi_mode;

	/* Add name here to load shared library */
	unsigned int type;

	/*Number of MIPI lanes*/
	unsigned int mipi_lanes;
	char name[32];

	u8 lightfreq;
};

struct gc0310_format_struct {
	u8 *desc;
	u32 pixelformat;
	struct regval_list *regs;
};

struct gc0310_res_struct {
	u8 *desc;
	int res;
	int width;
	int height;
	int fps;
	int skip_frames;
	bool used;
	struct regval_list *regs;
	u16 pixels_per_line;
	u16 lines_per_frame;
	u8 bin_factor_x;
	u8 bin_factor_y;
	u8 bin_mode;
};

struct gc0310_control {
	struct v4l2_queryctrl qc;
	int (*query)(struct v4l2_subdev *sd, s32 *value);
	int (*tweak)(struct v4l2_subdev *sd, int value);
};

/* 2 bytes used for address: 256 bytes total */
#define gc0310_MAX_WRITE_BUF_SIZE	254
struct gc0310_write_buffer {
	u16 addr;
	u8 data[gc0310_MAX_WRITE_BUF_SIZE];
};

struct gc0310_write_ctrl {
	int index;
	struct gc0310_write_buffer buffer;
};

/*
 * Modes supported by the gc0310 driver.
 * Please, keep them in ascending order.
 */
static struct gc0310_res_struct gc0310_res[] = {
#if 0
{
	.desc	= "CIF",
	.res	= gc0310_RES_CIF,
	.width	= 352,
	.height	= 288,
	.fps	= 30,
	.used	= 0,
	.regs	= NULL,
	.skip_frames = 1,

	.pixels_per_line = 0x01F0,
	.lines_per_frame = 0x014F,
	.bin_factor_x = 1,
	.bin_factor_y = 1,
	.bin_mode = 0,
	},
#endif
	{
	.desc	= "VGA",
	.res	= gc0310_RES_VGA,
	.width	= 656,
	.height	= 496,
	.fps	= 16.7,
	.used	= 0,
	.regs	= NULL,
	.skip_frames = 2,

	.pixels_per_line = 0x03e8,
	.lines_per_frame = 0x0240,
	.bin_factor_x = 1,
	.bin_factor_y = 1,
	.bin_mode = 0,
	},
};
#define N_RES (ARRAY_SIZE(gc0310_res))

static const struct i2c_device_id gc0310_id[] = {
	{"gc0310", 0},
	{}
};

static struct misensor_reg const gc0310_suspend[] = {
	 {MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg const gc0310_streaming[] = {
	 {MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg const gc0310_standby_reg[] = {
	 {MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg const gc0310_wakeup_reg[] = {
	 {MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg const gc0310_chgstat_reg[] = {
	{MISENSOR_TOK_TERM, 0, 0}
};
static struct misensor_reg const gc0310_qcif_init[] = {

	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg const gc0310_qvga_init[] = {

	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg const gc0310_vga_init[] = {

	{MISENSOR_TOK_TERM, 0, 0}
};



static struct misensor_reg const gc0310_common[] = {

	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg const gc0310_antiflicker_50hz[] = {
	 {MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg const gc0310_antiflicker_60hz[] = {
	 {MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg const gc0310_iq[] = {
	 {MISENSOR_TOK_TERM, 0, 0}
};

#endif
