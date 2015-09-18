/*
 * Support for ov2685f Camera Sensor.
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

#ifndef __OV2685F_H__
#define __OV2685F_H__

#define OV2685F_FOCAL_LENGTH_NUM	208	/*2.08mm*/
#define OV2685F_FOCAL_LENGTH_DEM	100
#define OV_F_NUMBER_DEFAULT_NUM	24
#define OV2685F_F_NUMBER_DEM	10
/*
 * focal length bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */

#define OV_FOCAL_LENGTH_DEFAULT 0x1710064
/*
 * current f-number bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define OV_F_NUMBER_DEFAULT 0x16000a

/*
 * f-number range bits definition:
 * bits 31-24: max f-number numerator
 * bits 23-16: max f-number denominator
 * bits 15-8: min f-number numerator
 * bits 7-0: min f-number denominator
 */
#define OV_F_NUMBER_RANGE 0x160a160a

#define OV2685F_FOCAL_VALUE ((OV2685F_FOCAL_LENGTH_NUM << 16) | OV2685F_FOCAL_LENGTH_DEM)

#define OV2685F_FNUMBER_VALUE ((OV_F_NUMBER_DEFAULT_NUM << 16) | OV2685F_F_NUMBER_DEM)

#define OV2685F_FNUMBER_RANGE_VALUE ((OV_F_NUMBER_DEFAULT_NUM << 24) | \
		(OV2685F_F_NUMBER_DEM << 16) |\
		(OV_F_NUMBER_DEFAULT_NUM << 8) | OV2685F_F_NUMBER_DEM)


/************************************
	resolution reglist

************************************/

static struct misensor_reg ov2685f_init_settings[] = {
	/*2lanes, 30fps*/
	{MISENSOR_8BIT, 0x0103 , 0x01},
	{MISENSOR_8BIT, 0x3002 , 0x00},
	{MISENSOR_8BIT, 0x3016 , 0x1c},
	{MISENSOR_8BIT, 0x3018 , 0x44},// 0x84, 2Lane; 0x44, 1Lane;
	{MISENSOR_8BIT, 0x301d , 0xf0},
	{MISENSOR_8BIT, 0x3020 , 0x00},
	{MISENSOR_8BIT, 0x3082 , 0x37},/*mclk = 19.2Mhz*/
	{MISENSOR_8BIT, 0x3083 , 0x03},
	{MISENSOR_8BIT, 0x3084 , 0x07},// 0x0f -> 0x07;
	{MISENSOR_8BIT, 0x3085 , 0x03},
	{MISENSOR_8BIT, 0x3086 , 0x00},
	{MISENSOR_8BIT, 0x3087 , 0x00},
	{MISENSOR_8BIT, 0x3501 , 0x4e},/*****/
	{MISENSOR_8BIT, 0x3502 , 0xe0},/*****/
	{MISENSOR_8BIT, 0x3503 , 0x03},
	{MISENSOR_8BIT, 0x350b , 0x36},
	{MISENSOR_8BIT, 0x3600 , 0xb4},
	{MISENSOR_8BIT, 0x3603 , 0x35},
	{MISENSOR_8BIT, 0x3604 , 0x24},
	{MISENSOR_8BIT, 0x3605 , 0x00},
	{MISENSOR_8BIT, 0x3620 , 0x24},/*****/
	{MISENSOR_8BIT, 0x3621 , 0x34},/*****/
	{MISENSOR_8BIT, 0x3622 , 0x03},/*****/
	{MISENSOR_8BIT, 0x3628 , 0x10},
	{MISENSOR_8BIT, 0x3705 , 0x3c},
	{MISENSOR_8BIT, 0x370a , 0x21},
	{MISENSOR_8BIT, 0x370c , 0x50},
	{MISENSOR_8BIT, 0x370d , 0xc0},
	{MISENSOR_8BIT, 0x3717 , 0x58},
	{MISENSOR_8BIT, 0x3718 , 0x80},/*****/
	{MISENSOR_8BIT, 0x3720 , 0x00},
	{MISENSOR_8BIT, 0x3721 , 0x09},/*****/
	{MISENSOR_8BIT, 0x3722 , 0x06},/*****/
	{MISENSOR_8BIT, 0x3723 , 0x59},/*****/
	{MISENSOR_8BIT, 0x3738 , 0x99},/*****/
	{MISENSOR_8BIT, 0x3781 , 0x80},
	{MISENSOR_8BIT, 0x3784 , 0x0c},/*****/
	{MISENSOR_8BIT, 0x3789 , 0x60},
	{MISENSOR_8BIT, 0x3800 , 0x00},
	{MISENSOR_8BIT, 0x3801 , 0x00},/*****/
	{MISENSOR_8BIT, 0x3802 , 0x00},
	{MISENSOR_8BIT, 0x3803 , 0x00},/*****/
	{MISENSOR_8BIT, 0x3804 , 0x06},/*****/
	{MISENSOR_8BIT, 0x3805 , 0x4f},/*****/
	{MISENSOR_8BIT, 0x3806 , 0x04},/*****/
	{MISENSOR_8BIT, 0x3807 , 0xbf},/*****/
	{MISENSOR_8BIT, 0x3808 , 0x06},/*****/
	{MISENSOR_8BIT, 0x3809 , 0x40},/*****/
	{MISENSOR_8BIT, 0x380a , 0x04},/*****/
	{MISENSOR_8BIT, 0x380b , 0xb0},/*****/
	{MISENSOR_8BIT, 0x380c , 0x06},/*****/
	{MISENSOR_8BIT, 0x380d , 0xa4},/*****/
	{MISENSOR_8BIT, 0x380e , 0x05},/*****/
	{MISENSOR_8BIT, 0x380f , 0x0e},/*****/
	{MISENSOR_8BIT, 0x3810 , 0x00},
	{MISENSOR_8BIT, 0x3811 , 0x08},
	{MISENSOR_8BIT, 0x3812 , 0x00},
	{MISENSOR_8BIT, 0x3813 , 0x08},/*****/
	{MISENSOR_8BIT, 0x3814 , 0x11},
	{MISENSOR_8BIT, 0x3815 , 0x11},
	{MISENSOR_8BIT, 0x3819 , 0x04},
	{MISENSOR_8BIT, 0x3820 , 0xc0},
	{MISENSOR_8BIT, 0x3821 , 0x00},
	{MISENSOR_8BIT, 0x3a06 , 0x01},/*****/
	{MISENSOR_8BIT, 0x3a07 , 0x84},/*****/
	{MISENSOR_8BIT, 0x3a08 , 0x01},/*****/
	{MISENSOR_8BIT, 0x3a09 , 0x43},/*****/
	{MISENSOR_8BIT, 0x3a0a , 0x24},/*****/
	{MISENSOR_8BIT, 0x3a0b , 0x60},
	{MISENSOR_8BIT, 0x3a0c , 0x28},/*****/
	{MISENSOR_8BIT, 0x3a0d , 0x60},/*****/
	{MISENSOR_8BIT, 0x3a0e , 0x04},/*****/
	{MISENSOR_8BIT, 0x3a0f , 0x8c},/*****/
	{MISENSOR_8BIT, 0x3a10 , 0x05},/*****/
	{MISENSOR_8BIT, 0x3a11 , 0x0c},/*****/
	{MISENSOR_8BIT, 0x4000 , 0x81},
	{MISENSOR_8BIT, 0x4001 , 0x40},
	{MISENSOR_8BIT, 0x4008 , 0x02},
	{MISENSOR_8BIT, 0x4009 , 0x09},
	{MISENSOR_8BIT, 0x4300 , 0x32},
	{MISENSOR_8BIT, 0x430e , 0x00},
	{MISENSOR_8BIT, 0x4602 , 0x02},
	{MISENSOR_8BIT, 0x4837 , 0x1e},
	{MISENSOR_8BIT, 0x5000 , 0xff},
	{MISENSOR_8BIT, 0x5001 , 0x05},
	{MISENSOR_8BIT, 0x5002 , 0x32},
	{MISENSOR_8BIT, 0x5003 , 0x04},
	{MISENSOR_8BIT, 0x5004 , 0xff},
	{MISENSOR_8BIT, 0x5005 , 0x12},
	{MISENSOR_8BIT, 0x0100 , 0x01},
	{MISENSOR_8BIT, 0x5180 , 0xf4},
	{MISENSOR_8BIT, 0x5181 , 0x11},
	{MISENSOR_8BIT, 0x5182 , 0x41},
	{MISENSOR_8BIT, 0x5183 , 0x42},
	{MISENSOR_8BIT, 0x5184 , 0x78},
	{MISENSOR_8BIT, 0x5185 , 0x58},
	{MISENSOR_8BIT, 0x5186 , 0xb5},
	{MISENSOR_8BIT, 0x5187 , 0xb2},
	{MISENSOR_8BIT, 0x5188 , 0x08},
	{MISENSOR_8BIT, 0x5189 , 0x0e},
	{MISENSOR_8BIT, 0x518a , 0x0c},
	{MISENSOR_8BIT, 0x518b , 0x4c},
	{MISENSOR_8BIT, 0x518c , 0x38},
	{MISENSOR_8BIT, 0x518d , 0xf8},
	{MISENSOR_8BIT, 0x518e , 0x04},
	{MISENSOR_8BIT, 0x518f , 0x7f},
	{MISENSOR_8BIT, 0x5190 , 0x40},
	{MISENSOR_8BIT, 0x5191 , 0x5f},
	{MISENSOR_8BIT, 0x5192 , 0x40},
	{MISENSOR_8BIT, 0x5193 , 0xff},
	{MISENSOR_8BIT, 0x5194 , 0x40},
	{MISENSOR_8BIT, 0x5195 , 0x07},
	{MISENSOR_8BIT, 0x5196 , 0x04},
	{MISENSOR_8BIT, 0x5197 , 0x04},
	{MISENSOR_8BIT, 0x5198 , 0x00},
	{MISENSOR_8BIT, 0x5199 , 0x05},
	{MISENSOR_8BIT, 0x519a , 0xd2},
	{MISENSOR_8BIT, 0x519b , 0x10},
	{MISENSOR_8BIT, 0x5200 , 0x09},
	{MISENSOR_8BIT, 0x5201 , 0x00},
	{MISENSOR_8BIT, 0x5202 , 0x06},
	{MISENSOR_8BIT, 0x5203 , 0x20},
	{MISENSOR_8BIT, 0x5204 , 0x41},
	{MISENSOR_8BIT, 0x5205 , 0x16},
	{MISENSOR_8BIT, 0x5206 , 0x00},
	{MISENSOR_8BIT, 0x5207 , 0x05},
	{MISENSOR_8BIT, 0x520b , 0x30},
	{MISENSOR_8BIT, 0x520c , 0x75},
	{MISENSOR_8BIT, 0x520d , 0x00},
	{MISENSOR_8BIT, 0x520e , 0x30},
	{MISENSOR_8BIT, 0x520f , 0x75},
	{MISENSOR_8BIT, 0x5210 , 0x00},
	{MISENSOR_8BIT, 0x5280 , 0x14},
	{MISENSOR_8BIT, 0x5281 , 0x02},
	{MISENSOR_8BIT, 0x5282 , 0x02},
	{MISENSOR_8BIT, 0x5283 , 0x04},
	{MISENSOR_8BIT, 0x5284 , 0x06},
	{MISENSOR_8BIT, 0x5285 , 0x08},
	{MISENSOR_8BIT, 0x5286 , 0x0c},
	{MISENSOR_8BIT, 0x5287 , 0x10},
	{MISENSOR_8BIT, 0x5300 , 0xc5},
	{MISENSOR_8BIT, 0x5301 , 0xa0},
	{MISENSOR_8BIT, 0x5302 , 0x06},
	{MISENSOR_8BIT, 0x5303 , 0x0a},
	{MISENSOR_8BIT, 0x5304 , 0x30},
	{MISENSOR_8BIT, 0x5305 , 0x60},
	{MISENSOR_8BIT, 0x5306 , 0x90},
	{MISENSOR_8BIT, 0x5307 , 0xc0},
	{MISENSOR_8BIT, 0x5308 , 0x82},
	{MISENSOR_8BIT, 0x5309 , 0x00},
	{MISENSOR_8BIT, 0x530a , 0x26},
	{MISENSOR_8BIT, 0x530b , 0x02},
	{MISENSOR_8BIT, 0x530c , 0x02},
	{MISENSOR_8BIT, 0x530d , 0x00},
	{MISENSOR_8BIT, 0x530e , 0x0c},
	{MISENSOR_8BIT, 0x530f , 0x14},
	{MISENSOR_8BIT, 0x5310 , 0x1a},
	{MISENSOR_8BIT, 0x5311 , 0x20},
	{MISENSOR_8BIT, 0x5312 , 0x80},
	{MISENSOR_8BIT, 0x5313 , 0x4b},
	{MISENSOR_8BIT, 0x5380 , 0x01},
	{MISENSOR_8BIT, 0x5381 , 0x52},
	{MISENSOR_8BIT, 0x5382 , 0x00},
	{MISENSOR_8BIT, 0x5383 , 0x4a},
	{MISENSOR_8BIT, 0x5384 , 0x00},
	{MISENSOR_8BIT, 0x5385 , 0xb6},
	{MISENSOR_8BIT, 0x5386 , 0x00},
	{MISENSOR_8BIT, 0x5387 , 0x8d},
	{MISENSOR_8BIT, 0x5388 , 0x00},
	{MISENSOR_8BIT, 0x5389 , 0x3a},
	{MISENSOR_8BIT, 0x538a , 0x00},
	{MISENSOR_8BIT, 0x538b , 0xa6},
	{MISENSOR_8BIT, 0x538c , 0x00},
	{MISENSOR_8BIT, 0x5400 , 0x0d},
	{MISENSOR_8BIT, 0x5401 , 0x18},
	{MISENSOR_8BIT, 0x5402 , 0x31},
	{MISENSOR_8BIT, 0x5403 , 0x5a},
	{MISENSOR_8BIT, 0x5404 , 0x65},
	{MISENSOR_8BIT, 0x5405 , 0x6f},
	{MISENSOR_8BIT, 0x5406 , 0x77},
	{MISENSOR_8BIT, 0x5407 , 0x80},
	{MISENSOR_8BIT, 0x5408 , 0x87},
	{MISENSOR_8BIT, 0x5409 , 0x8f},
	{MISENSOR_8BIT, 0x540a , 0xa2},
	{MISENSOR_8BIT, 0x540b , 0xb2},
	{MISENSOR_8BIT, 0x540c , 0xcc},
	{MISENSOR_8BIT, 0x540d , 0xe4},
	{MISENSOR_8BIT, 0x540e , 0xf0},
	{MISENSOR_8BIT, 0x540f , 0xa0},
	{MISENSOR_8BIT, 0x5410 , 0x6e},
	{MISENSOR_8BIT, 0x5411 , 0x06},
	{MISENSOR_8BIT, 0x5480 , 0x19},
	{MISENSOR_8BIT, 0x5481 , 0x00},
	{MISENSOR_8BIT, 0x5482 , 0x09},
	{MISENSOR_8BIT, 0x5483 , 0x12},
	{MISENSOR_8BIT, 0x5484 , 0x04},
	{MISENSOR_8BIT, 0x5485 , 0x06},
	{MISENSOR_8BIT, 0x5486 , 0x08},
	{MISENSOR_8BIT, 0x5487 , 0x0c},
	{MISENSOR_8BIT, 0x5488 , 0x10},
	{MISENSOR_8BIT, 0x5489 , 0x18},
	{MISENSOR_8BIT, 0x5500 , 0x02},
	{MISENSOR_8BIT, 0x5501 , 0x03},
	{MISENSOR_8BIT, 0x5502 , 0x04},
	{MISENSOR_8BIT, 0x5503 , 0x05},
	{MISENSOR_8BIT, 0x5504 , 0x06},
	{MISENSOR_8BIT, 0x5505 , 0x08},
	{MISENSOR_8BIT, 0x5506 , 0x00},
	{MISENSOR_8BIT, 0x5600 , 0x02},
	{MISENSOR_8BIT, 0x5603 , 0x40},
	{MISENSOR_8BIT, 0x5604 , 0x28},
	{MISENSOR_8BIT, 0x5609 , 0x20},
	{MISENSOR_8BIT, 0x560a , 0x60},
	{MISENSOR_8BIT, 0x5800 , 0x03},
	{MISENSOR_8BIT, 0x5801 , 0x24},
	{MISENSOR_8BIT, 0x5802 , 0x02},
	{MISENSOR_8BIT, 0x5803 , 0x40},
	{MISENSOR_8BIT, 0x5804 , 0x34},
	{MISENSOR_8BIT, 0x5805 , 0x05},
	{MISENSOR_8BIT, 0x5806 , 0x12},
	{MISENSOR_8BIT, 0x5807 , 0x05},
	{MISENSOR_8BIT, 0x5808 , 0x03},
	{MISENSOR_8BIT, 0x5809 , 0x3c},
	{MISENSOR_8BIT, 0x580a , 0x02},
	{MISENSOR_8BIT, 0x580b , 0x40},
	{MISENSOR_8BIT, 0x580c , 0x26},
	{MISENSOR_8BIT, 0x580d , 0x05},
	{MISENSOR_8BIT, 0x580e , 0x52},
	{MISENSOR_8BIT, 0x580f , 0x06},
	{MISENSOR_8BIT, 0x5810 , 0x03},
	{MISENSOR_8BIT, 0x5811 , 0x28},
	{MISENSOR_8BIT, 0x5812 , 0x02},
	{MISENSOR_8BIT, 0x5813 , 0x40},
	{MISENSOR_8BIT, 0x5814 , 0x24},
	{MISENSOR_8BIT, 0x5815 , 0x05},
	{MISENSOR_8BIT, 0x5816 , 0x42},
	{MISENSOR_8BIT, 0x5817 , 0x06},
	{MISENSOR_8BIT, 0x5818 , 0x0d},
	{MISENSOR_8BIT, 0x5819 , 0x40},
	{MISENSOR_8BIT, 0x581a , 0x04},
	{MISENSOR_8BIT, 0x581b , 0x0c},
	{MISENSOR_8BIT, 0x3a03 , 0x4c},
	{MISENSOR_8BIT, 0x3a04 , 0x40},
	{MISENSOR_8BIT, 0x3503 , 0x00},
#if 1
	// enable below code if run 1 Lane. and change variable csi_lanes to be 1.
	{MISENSOR_8BIT, 0x3084, 0xf },
	{MISENSOR_8BIT, 0x3018, 0x44},
#endif
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg ov2685f_2M[] = {
	{MISENSOR_8BIT, 0x3503, 0x03},
	{MISENSOR_8BIT, 0x3501, 0x4e},
	{MISENSOR_8BIT, 0x3502, 0xe0},
	{MISENSOR_8BIT, 0x3620, 0x24},
	{MISENSOR_8BIT, 0x3621, 0x34},
	{MISENSOR_8BIT, 0x3622, 0x03},
	{MISENSOR_8BIT, 0x370a, 0x21},
	{MISENSOR_8BIT, 0x370d, 0xc0},
	{MISENSOR_8BIT, 0x3718, 0x80},
	{MISENSOR_8BIT, 0x3721, 0x09},
	{MISENSOR_8BIT, 0x3722, 0x06},
	{MISENSOR_8BIT, 0x3723, 0x59},
	{MISENSOR_8BIT, 0x3738, 0x99},
	{MISENSOR_8BIT, 0x3801, 0x00},
	{MISENSOR_8BIT, 0x3803, 0x00},
	{MISENSOR_8BIT, 0x3804, 0x06},
	{MISENSOR_8BIT, 0x3805, 0x4f},
	{MISENSOR_8BIT, 0x3806, 0x04},
	{MISENSOR_8BIT, 0x3807, 0xbf},
	{MISENSOR_8BIT, 0x3808, 0x06},
	{MISENSOR_8BIT, 0x3809, 0x40},
	{MISENSOR_8BIT, 0x380a, 0x04},
	{MISENSOR_8BIT, 0x380b, 0xb0},
	{MISENSOR_8BIT, 0x380c, 0x06},
	{MISENSOR_8BIT, 0x380d, 0xa4},
	{MISENSOR_8BIT, 0x380e, 0x05},
	{MISENSOR_8BIT, 0x380f, 0x0e},
	{MISENSOR_8BIT, 0x3811, 0x08},
	{MISENSOR_8BIT, 0x3813, 0x08},
	{MISENSOR_8BIT, 0x3814, 0x11},
	{MISENSOR_8BIT, 0x3815, 0x11},
	{MISENSOR_8BIT | MISENSOR_RMW_OR, 0x3820, 0xc0},
	{MISENSOR_8BIT | MISENSOR_RMW_OR, 0x3821, 0x00},
	{MISENSOR_8BIT, 0x382a, 0x08},
	{MISENSOR_8BIT, 0x3a00, 0x41},
	{MISENSOR_8BIT, 0x3a07, 0xc2},
	{MISENSOR_8BIT, 0x3a09, 0xa1},
	{MISENSOR_8BIT, 0x3a0a, 0x09},
	{MISENSOR_8BIT, 0x3a0b, 0xda},
	{MISENSOR_8BIT, 0x3a0c, 0x0a},
	{MISENSOR_8BIT, 0x3a0d, 0x10},
	{MISENSOR_8BIT, 0x3a0e, 0x04},
	{MISENSOR_8BIT, 0x3a0f, 0x8c},
	{MISENSOR_8BIT, 0x3a10, 0x05},
	{MISENSOR_8BIT, 0x3a11, 0x08},
	{MISENSOR_8BIT, 0x4008, 0x02},
	{MISENSOR_8BIT, 0x4009, 0x09},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg ov2685f_720p[] = {
	/*1lane 30fps*/
	{MISENSOR_8BIT, 0x3503, 0x0 },
	{MISENSOR_8BIT, 0x3501, 0x2f},
	{MISENSOR_8BIT, 0x3502, 0x40},
	{MISENSOR_8BIT, 0x3620, 0x26},
	{MISENSOR_8BIT, 0x3621, 0x37},
	{MISENSOR_8BIT, 0x3622, 0x4 },
	{MISENSOR_8BIT, 0x370a, 0x21},
	{MISENSOR_8BIT, 0x370d, 0xc0},
	{MISENSOR_8BIT, 0x3718, 0x88},
	{MISENSOR_8BIT, 0x3721, 0x0 },
	{MISENSOR_8BIT, 0x3722, 0x0 },
	{MISENSOR_8BIT, 0x3723, 0x0 },
	{MISENSOR_8BIT, 0x3738, 0x0 },
	{MISENSOR_8BIT, 0x3801, 0xa0},
	{MISENSOR_8BIT, 0x3803, 0xf2},
	{MISENSOR_8BIT, 0x3804, 0x5 },
	{MISENSOR_8BIT, 0x3805, 0xaf},
	{MISENSOR_8BIT, 0x3806, 0x3 },
	{MISENSOR_8BIT, 0x3807, 0xcd},
	{MISENSOR_8BIT, 0x3808, 0x5 },
	{MISENSOR_8BIT, 0x3809, 0x0 },
	{MISENSOR_8BIT, 0x380a, 0x2 },
	{MISENSOR_8BIT, 0x380b, 0xd0},
	{MISENSOR_8BIT, 0x380c, 0x5 },
	{MISENSOR_8BIT, 0x380d, 0xa6},
	{MISENSOR_8BIT, 0x380e, 0x2 },
	{MISENSOR_8BIT, 0x380f, 0xf8},
	{MISENSOR_8BIT, 0x3811, 0x8 },
	{MISENSOR_8BIT, 0x3813, 0x6 },
	{MISENSOR_8BIT, 0x3814, 0x11},
	{MISENSOR_8BIT, 0x3815, 0x11},
	{MISENSOR_8BIT | MISENSOR_RMW_OR, 0x3820, 0xc0},
	{MISENSOR_8BIT | MISENSOR_RMW_OR, 0x3821, 0x0 },
	{MISENSOR_8BIT, 0x382a, 0x0 },
	{MISENSOR_8BIT, 0x3a00, 0x41},
	{MISENSOR_8BIT, 0x3a07, 0xe4},
	{MISENSOR_8BIT, 0x3a09, 0xbe},
	{MISENSOR_8BIT, 0x3a0a, 0x15},
	{MISENSOR_8BIT, 0x3a0b, 0x60},
	{MISENSOR_8BIT, 0x3a0c, 0x17},
	{MISENSOR_8BIT, 0x3a0d, 0xc0},
	{MISENSOR_8BIT, 0x3a0e, 0x2 },
	{MISENSOR_8BIT, 0x3a0f, 0xac},
	{MISENSOR_8BIT, 0x3a10, 0x2 },
	{MISENSOR_8BIT, 0x3a11, 0xf8},
	{MISENSOR_8BIT, 0x4008, 0x2 },
	{MISENSOR_8BIT, 0x4009, 0x9 },
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg ov2685f_800x600[] = {
	{MISENSOR_8BIT, 0x3501, 0x26},
	{MISENSOR_8BIT, 0x3502, 0x40},
	{MISENSOR_8BIT, 0x3620, 0x24},
	{MISENSOR_8BIT, 0x3621, 0x34},
	{MISENSOR_8BIT, 0x3622, 0x03},
	{MISENSOR_8BIT, 0x370a, 0x23},
	{MISENSOR_8BIT, 0x370d, 0x00},
	{MISENSOR_8BIT, 0x3718, 0x80},
	{MISENSOR_8BIT, 0x3721, 0x09},
	{MISENSOR_8BIT, 0x3722, 0x0b},
	{MISENSOR_8BIT, 0x3723, 0x48},
	{MISENSOR_8BIT, 0x3738, 0x99},
	{MISENSOR_8BIT, 0x3801, 0x00},
	{MISENSOR_8BIT, 0x3803, 0x00},
	{MISENSOR_8BIT, 0x3804, 0x06},
	{MISENSOR_8BIT, 0x3805, 0x4f},
	{MISENSOR_8BIT, 0x3806, 0x04},
	{MISENSOR_8BIT, 0x3807, 0xbf},
	{MISENSOR_8BIT, 0x3808, 0x03},
	{MISENSOR_8BIT, 0x3809, 0x20},
	{MISENSOR_8BIT, 0x380a, 0x02},
	{MISENSOR_8BIT, 0x380b, 0x58},
	{MISENSOR_8BIT, 0x380c, 0x06},
	{MISENSOR_8BIT, 0x380d, 0xac},
	{MISENSOR_8BIT, 0x380e, 0x02},
	{MISENSOR_8BIT, 0x380f, 0x84},
	{MISENSOR_8BIT, 0x3811, 0x04},
	{MISENSOR_8BIT, 0x3813, 0x04},
	{MISENSOR_8BIT, 0x3814, 0x31},
	{MISENSOR_8BIT, 0x3815, 0x31},
	{MISENSOR_8BIT | MISENSOR_RMW_OR, 0x3820, 0xc2},
	{MISENSOR_8BIT | MISENSOR_RMW_OR, 0x3821, 0x01},
	{MISENSOR_8BIT, 0x382a, 0x08},
	{MISENSOR_8BIT, 0x3a00, 0x43},
	{MISENSOR_8BIT, 0x3a07, 0xc1},
	{MISENSOR_8BIT, 0x3a09, 0xa1},
	{MISENSOR_8BIT, 0x3a0a, 0x07},
	{MISENSOR_8BIT, 0x3a0b, 0x8a},
	{MISENSOR_8BIT, 0x3a0c, 0x07},
	{MISENSOR_8BIT, 0x3a0d, 0x8c},
	{MISENSOR_8BIT, 0x3a0e, 0x02},
	{MISENSOR_8BIT, 0x3a0f, 0x43},
	{MISENSOR_8BIT, 0x3a10, 0x02},
	{MISENSOR_8BIT, 0x3a11, 0x84},
	{MISENSOR_8BIT, 0x3a13, 0x80},
	{MISENSOR_8BIT, 0x4008, 0x00},
	{MISENSOR_8BIT, 0x4009, 0x03},
	{MISENSOR_8BIT, 0x5003, 0x0c},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct ov_res_struct ov2685f_res[] = {
	{
		.desc	= "ov2685f_800x600",
		.res_id	= RES_SVGA,
		.width	= 800,
		.height	= 600,
		.fps	= 30,
		.used	= 0,
		.skip_frames = 1,
		.regs = ov2685f_800x600,
		.aspt_ratio = RATIO_4_3,
		.csi_lanes = 2,
		.max_exposure_lines = 1290,
	},
	{
		.desc	= "720P",
		.res_id	= RES_720P,
		.width	= RES_720P_SIZE_H,
		.height	= RES_720P_SIZE_V,
		.fps	= 30,
		.used	= 0,
		.skip_frames = 1,
		.regs = ov2685f_720p,
		.aspt_ratio = RATIO_16_9,
		.csi_lanes = 1, // 2 -> 1;
		.max_exposure_lines = 1290,
	},
	{
		.desc	= "2M",
		.res_id	= RES_2M,
		.width	= RES_2M_SIZE_H,
		.height	= RES_2M_SIZE_V,
		.fps	= 30,
		.used	= 0,
		.skip_frames = 2, // 0 -> 2;
		.regs = ov2685f_2M,
		.aspt_ratio = RATIO_4_3,
		.csi_lanes = 1, // 2 -> 1;
		.max_exposure_lines = 1290,
	},
};

/************************************
	exposure reglist

************************************/

static struct misensor_reg ov2685f_exp_0[] = {
	{MISENSOR_8BIT, 0x3503, 0x3},
	{MISENSOR_8BIT, 0x3501, 0x16},
	{MISENSOR_8BIT, 0x350b, 0x26},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg ov2685f_exp_1[] = {
	{MISENSOR_8BIT, 0x3503, 0x3},
	{MISENSOR_8BIT, 0x3501, 0x26},
	{MISENSOR_8BIT, 0x350b, 0x36},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg ov2685f_exp_2[] = {
	{MISENSOR_8BIT, 0x3503, 0x0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg ov2685f_exp_3[] = {
	{MISENSOR_8BIT, 0x3503, 0x3},
	{MISENSOR_8BIT, 0x3501, 0x46},
	{MISENSOR_8BIT, 0x350b, 0x46},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg ov2685f_exp_4[] = {
	{MISENSOR_8BIT, 0x3503, 0x3},
	{MISENSOR_8BIT, 0x3501, 0x56},
	{MISENSOR_8BIT, 0x350b, 0x56},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct ov_reglist_map ov2685f_exposure_table[] = {
	{ -2, ov2685f_exp_0 },
	{ -1, ov2685f_exp_1 },
	{ 0, ov2685f_exp_2 },
	{ 1, ov2685f_exp_3 },
	{ 2, ov2685f_exp_4 },
};

/************************************
	streaming reglist

************************************/

static struct misensor_reg  ov2685f_stream_off[] = {
	{MISENSOR_8BIT, 0x4202, 0x0f},
	{MISENSOR_8BIT, 0x301c, 0xf4},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_stream_on[] = {
	{MISENSOR_8BIT, 0x4202, 0x00},
	{MISENSOR_8BIT, 0x301c, 0xf0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct ov_reglist_map ov2685f_stream_table[] = {
	{ 0x0, ov2685f_stream_off },
	{ 0x1, ov2685f_stream_on  },
};

/************************************
	flip reglist

************************************/
/* FIXME: vflip/hflip tables just suit for
 * more then quarter of full size(>800x600)
 */
static struct misensor_reg  ov2685f_vflip_0[] = {
	{MISENSOR_8BIT | MISENSOR_RMW_AND, 0x3820, ~0x4},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_vflip_1[] = {
	{MISENSOR_8BIT | MISENSOR_RMW_OR, 0x3820, 0x4},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_hflip_0[] = {
	{MISENSOR_8BIT | MISENSOR_RMW_AND, 0x3821, ~0x04},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_hflip_1[] = {
	{MISENSOR_8BIT | MISENSOR_RMW_OR, 0x3821, 0x04},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct ov_reglist_map ov2685f_vflip_table[] = {
	{ 0x0, ov2685f_vflip_0 },
	{ 0x1, ov2685f_vflip_1 },
};

static struct ov_reglist_map ov2685f_hflip_table[] = {
	{ 0x0, ov2685f_hflip_0 },
	{ 0x1, ov2685f_hflip_1 },
};

/************************************
	scene mode reglist

************************************/
#if 0
static struct misensor_reg  ov2685f_scene_0[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x06},
	{MISENSOR_8BIT, 0x5603, 0x40},
	{MISENSOR_8BIT, 0x5604, 0x28},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_1[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x1c},
	{MISENSOR_8BIT, 0x5603, 0xa0},
	{MISENSOR_8BIT, 0x5604, 0x40},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_2[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x1c},
	{MISENSOR_8BIT, 0x5603, 0x80},
	{MISENSOR_8BIT, 0x5604, 0xc0},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_3[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x1c},
	{MISENSOR_8BIT, 0x5603, 0x80},
	{MISENSOR_8BIT, 0x5604, 0x80},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_4[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x1c},
	{MISENSOR_8BIT, 0x5603, 0x40},
	{MISENSOR_8BIT, 0x5604, 0xa0},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_5[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x1c},
	{MISENSOR_8BIT, 0x5603, 0x40},
	{MISENSOR_8BIT, 0x5604, 0xa0},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_6[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x46},
	{MISENSOR_8BIT, 0x5603, 0x40},
	{MISENSOR_8BIT, 0x5604, 0x28},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_7[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x1c},
	{MISENSOR_8BIT, 0x5603, 0x60},
	{MISENSOR_8BIT, 0x5604, 0x60},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_8[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x1c},
	{MISENSOR_8BIT, 0x5603, 0x80},
	{MISENSOR_8BIT, 0x5604, 0xc0},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_9[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x1c},
	{MISENSOR_8BIT, 0x5603, 0x80},
	{MISENSOR_8BIT, 0x5604, 0xc0},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_10[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x06},
	{MISENSOR_8BIT, 0x5603, 0x40},
	{MISENSOR_8BIT, 0x5604, 0x28},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_11[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x06},
	{MISENSOR_8BIT, 0x5603, 0x40},
	{MISENSOR_8BIT, 0x5604, 0x28},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_12[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x1c},
	{MISENSOR_8BIT, 0x5603, 0x40},
	{MISENSOR_8BIT, 0x5604, 0xa0},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_scene_13[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5600, 0x06},
	{MISENSOR_8BIT, 0x5603, 0x40},
	{MISENSOR_8BIT, 0x5604, 0x28},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct ov_reglist_map ov2685f_scenemode_table[] = {
	{ V4L2_SCENE_MODE_NONE, ov2685f_scene_0 },
	{ V4L2_SCENE_MODE_BACKLIGHT, ov2685f_scene_1 },
	{ V4L2_SCENE_MODE_BEACH_SNOW, ov2685f_scene_2 },
	{ V4L2_SCENE_MODE_CANDLE_LIGHT, ov2685f_scene_3 },
	{ V4L2_SCENE_MODE_DAWN_DUSK, ov2685f_scene_4 },
	{ V4L2_SCENE_MODE_FALL_COLORS, ov2685f_scene_5 },
	{ V4L2_SCENE_MODE_FIREWORKS, ov2685f_scene_6 },
	{ V4L2_SCENE_MODE_LANDSCAPE, ov2685f_scene_7 },
	{ V4L2_SCENE_MODE_NIGHT, ov2685f_scene_8 },
	{ V4L2_SCENE_MODE_PARTY_INDOOR, ov2685f_scene_9 },
	{ V4L2_SCENE_MODE_PORTRAIT, ov2685f_scene_10 },
	{ V4L2_SCENE_MODE_SPORTS, ov2685f_scene_11 },
	{ V4L2_SCENE_MODE_SUNSET, ov2685f_scene_12 },
	{ V4L2_SCENE_MODE_TEXT, ov2685f_scene_13 },
};
#else
static struct misensor_reg ov2685f_scene_nightmode_off[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x3a00, 0x41},
	{MISENSOR_8BIT, 0x382a, 0x00},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg ov2685f_scene_nightmode_on[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x3a00, 0x43},
	{MISENSOR_8BIT, 0x3a0a, 0x07},
	{MISENSOR_8BIT, 0x3a0b, 0x8c},
	{MISENSOR_8BIT, 0x3a0c, 0x07},
	{MISENSOR_8BIT, 0x3a0d, 0x8c},
	{MISENSOR_8BIT, 0x382a, 0x08},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};
static struct ov_reglist_map ov2685f_scenemode_table[] = {
	{ V4L2_SCENE_MODE_NONE, ov2685f_scene_nightmode_off},
	{ V4L2_SCENE_MODE_NIGHT, ov2685f_scene_nightmode_on},
};
#endif

/************************************
	white balance reglist

************************************/

static struct misensor_reg  ov2685f_awb_0[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5180, 0xf6},
	{MISENSOR_8BIT, 0x5195, 0x07},
	{MISENSOR_8BIT, 0x5196, 0x9c},
	{MISENSOR_8BIT, 0x5197, 0x04},
	{MISENSOR_8BIT, 0x5198, 0x00},
	{MISENSOR_8BIT, 0x5199, 0x05},
	{MISENSOR_8BIT, 0x519a, 0xf3},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_awb_1[] = {
	// {MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5180, 0xf4},
	// {MISENSOR_8BIT, 0x3208, 0x10},
	// {MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_awb_2[] = {
	// {MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5180, 0xf6},
	{MISENSOR_8BIT, 0x5195, 0x04},
	{MISENSOR_8BIT, 0x5196, 0x90},
	{MISENSOR_8BIT, 0x5197, 0x04},
	{MISENSOR_8BIT, 0x5198, 0x00},
	{MISENSOR_8BIT, 0x5199, 0x09},
	{MISENSOR_8BIT, 0x519a, 0x20},
	// {MISENSOR_8BIT, 0x3208, 0x10},
	// {MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_awb_3[] = {
	// {MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5195, 0x5 },
	{MISENSOR_8BIT, 0x5196, 0xa8},
	{MISENSOR_8BIT, 0x5197, 0x4 },
	{MISENSOR_8BIT, 0x5198, 0x0 },
	{MISENSOR_8BIT, 0x5199, 0x7 },
	{MISENSOR_8BIT, 0x519a, 0x75},
	// {MISENSOR_8BIT, 0x3208, 0x10},
	// {MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_awb_4[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5180, 0xf6},
	{MISENSOR_8BIT, 0x5195, 0x07},
	{MISENSOR_8BIT, 0x5196, 0xdc},
	{MISENSOR_8BIT, 0x5197, 0x04},
	{MISENSOR_8BIT, 0x5198, 0x00},
	{MISENSOR_8BIT, 0x5199, 0x05},
	{MISENSOR_8BIT, 0x519a, 0xd3},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_awb_5[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5180, 0xf6},
	{MISENSOR_8BIT, 0x5195, 0x04},
	{MISENSOR_8BIT, 0x5196, 0x90},
	{MISENSOR_8BIT, 0x5197, 0x04},
	{MISENSOR_8BIT, 0x5198, 0x00},
	{MISENSOR_8BIT, 0x5199, 0x09},
	{MISENSOR_8BIT, 0x519a, 0x20},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_awb_6[] = {
	// {MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5180, 0xf6},
	{MISENSOR_8BIT, 0x5195, 0x07},
	{MISENSOR_8BIT, 0x5196, 0x9c},
	{MISENSOR_8BIT, 0x5197, 0x04},
	{MISENSOR_8BIT, 0x5198, 0x00},
	{MISENSOR_8BIT, 0x5199, 0x05},
	{MISENSOR_8BIT, 0x519a, 0xf3},
	// {MISENSOR_8BIT, 0x3208, 0x10},
	// {MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_awb_7[] = {
	// {MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5180, 0xf6},
	{MISENSOR_8BIT, 0x5195, 0x07},
	{MISENSOR_8BIT, 0x5196, 0xdc},
	{MISENSOR_8BIT, 0x5197, 0x04},
	{MISENSOR_8BIT, 0x5198, 0x00},
	{MISENSOR_8BIT, 0x5199, 0x05},
	{MISENSOR_8BIT, 0x519a, 0xd3},
	// {MISENSOR_8BIT, 0x3208, 0x10},
	// {MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_awb_8[] = {
	{MISENSOR_8BIT, 0x3208, 0x00},
	{MISENSOR_8BIT, 0x5180, 0xf6},
	{MISENSOR_8BIT, 0x5195, 0x04},
	{MISENSOR_8BIT, 0x5196, 0x90},
	{MISENSOR_8BIT, 0x5197, 0x04},
	{MISENSOR_8BIT, 0x5198, 0x00},
	{MISENSOR_8BIT, 0x5199, 0x09},
	{MISENSOR_8BIT, 0x519a, 0x20},
	{MISENSOR_8BIT, 0x3208, 0x10},
	{MISENSOR_8BIT, 0x3208, 0xa0},
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct ov_reglist_map ov2685f_awb_table[] = {
	{ V4L2_WHITE_BALANCE_MANUAL, ov2685f_awb_0 },
	{ V4L2_WHITE_BALANCE_AUTO, ov2685f_awb_1  },
	{ V4L2_WHITE_BALANCE_INCANDESCENT, ov2685f_awb_2  },
	{ V4L2_WHITE_BALANCE_FLUORESCENT, ov2685f_awb_3  },
	{ V4L2_WHITE_BALANCE_FLUORESCENT_H, ov2685f_awb_4  },
	{ V4L2_WHITE_BALANCE_HORIZON, ov2685f_awb_5  },
	{ V4L2_WHITE_BALANCE_DAYLIGHT, ov2685f_awb_6  },
	{ V4L2_WHITE_BALANCE_CLOUDY, ov2685f_awb_7  },
	{ V4L2_WHITE_BALANCE_SHADE, ov2685f_awb_8  },
};

/************************************
	freq reglist

************************************/

static struct misensor_reg	ov2685f_freq_0[] = {
	{MISENSOR_TOK_TERM, 0, 0},
};

static struct misensor_reg	ov2685f_freq_1[] = {
	{MISENSOR_TOK_TERM, 0, 0},
};

static struct misensor_reg	ov2685f_freq_2[] = {
	{MISENSOR_TOK_TERM, 0, 0},
};

static struct misensor_reg	ov2685f_freq_3[] = {
	{MISENSOR_TOK_TERM, 0, 0},
};

static struct ov_reglist_map ov2685f_freq_table[] = {
	{ V4L2_CID_POWER_LINE_FREQUENCY_DISABLED, ov2685f_freq_0 },
	{ V4L2_CID_POWER_LINE_FREQUENCY_50HZ, ov2685f_freq_1  },
	{ V4L2_CID_POWER_LINE_FREQUENCY_60HZ, ov2685f_freq_2  },
	{ V4L2_CID_POWER_LINE_FREQUENCY_AUTO, ov2685f_freq_3  },
};

/************************************
	format reglist

************************************/

static struct misensor_reg  ov2685f_uyvy16[] = {
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct misensor_reg  ov2685f_yuyv16[] = {
	{MISENSOR_TOK_TERM, 0, 0}
};

static struct ov_mbus_fmt ov2685f_fmts[] = {
	{
		.desc		= "YUYV 4:2:2",
		.code		= V4L2_MBUS_FMT_UYVY8_1X16,
		.reg_list	= ov2685f_uyvy16,
		.size		= ARRAY_SIZE(ov2685f_uyvy16),
		.bpp		= 2,
	}, {
		.desc		= "YUYV 4:2:2",
		.code		= V4L2_MBUS_FMT_YUYV8_1X16,
		.reg_list	= ov2685f_yuyv16,
		.size		= ARRAY_SIZE(ov2685f_yuyv16),
		.bpp		= 2,
	},
};

/************************************
	init reglist

************************************/

static struct ov_res_struct ov2685f_init = {
	.desc	= "2M",
	.res_id	= RES_2M,
	.width	= RES_2M_SIZE_H,
	.height = RES_2M_SIZE_V,
	.fps	= 30,
	.used	= 0,
	.skip_frames = 0,
	.regs = ov2685f_init_settings,
	.csi_lanes = 1, // 2 -> 1,
};

struct ov_product_info ov2685f_product_info = {
	/* TODO: platform resource */
	.sensor_id = 0x2685,
	.reg_id = 0x300A,
	.gpio_pin = 0,
	.regulator_id = 0,
	.clk_id = 0,
	.clk_freq = 0,

	.focal	= OV2685F_FOCAL_VALUE,
	.f_number =  OV2685F_FNUMBER_VALUE,
	.f_number_range = OV2685F_FNUMBER_RANGE_VALUE,
	.reg_expo_coarse = 0x3500,
	/* sensor functionality: init/streaming/exposure/flip/scene/color/awb/fmt/res */
	.ov_init = &ov2685f_init,
	.settings_tbl = {
		ADD_SETTINGS_TABLES(OV_SETTING_STREAM, ov2685f_stream_table),
		ADD_SETTINGS_TABLES(OV_SETTING_EXPOSURE, ov2685f_exposure_table),
		ADD_SETTINGS_TABLES(OV_SETTING_VFLIP, ov2685f_vflip_table),
		ADD_SETTINGS_TABLES(OV_SETTING_HFLIP, ov2685f_hflip_table),
		ADD_SETTINGS_TABLES(OV_SETTING_SCENE_MODE, ov2685f_scenemode_table),
		ADD_SETTINGS_TABLES(OV_SETTING_AWB_MODE, ov2685f_awb_table),
		ADD_SETTINGS_TABLES(OV_SETTING_FREQ, ov2685f_freq_table),
	},
	.ov_res = ov2685f_res,
	.ov_fmt = ov2685f_fmts,
	.num_res = ARRAY_SIZE(ov2685f_res),
	.num_fmt = ARRAY_SIZE(ov2685f_fmts),
};

#endif
