/* Private header file for GC0310
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
#ifndef __GC_0310_H__
#define __GC_0310_H__

#define GC0310_IMG_ORIENTATION			0x17
#define GC0310_FOCAL_LENGTH_NUM	208	/*2.08mm*/
#define GC0310_FOCAL_LENGTH_DEM	100
#define GC0310_F_NUMBER_DEFAULT_NUM	24
#define GC0310_F_NUMBER_DEM	10
#define GC0310_REG_EXPO_COARSE		  0x03
#define GC0310_REG_MAX_AEC			  0x3c
#define GC0310_REG_COLOR_EFFECT		 0x83
#define GC0310_REG_BLOCK_ENABLE			0x42

/*
 * focal length bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define GC_FOCAL_LENGTH_DEFAULT 0x1710064
/*
 * current f-number bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define GC_F_NUMBER_DEFAULT 0x16000a

/*
 * f-number range bits definition:
 * bits 31-24: max f-number numerator
 * bits 23-16: max f-number denominator
 * bits 15-8: min f-number numerator
 * bits 7-0: min f-number denominator
 */
#define GC_F_NUMBER_RANGE 0x160a160a

#define GC0310_FOCAL_VALUE ((GC0310_FOCAL_LENGTH_NUM << 16) | GC0310_FOCAL_LENGTH_DEM)

#define GC0310_FNUMBER_VALUE ((GC0310_F_NUMBER_DEFAULT_NUM << 16) | GC0310_F_NUMBER_DEM)

#define GC0310_FNUMBER_RANGE_VALUE ((GC0310_F_NUMBER_DEFAULT_NUM << 24) | \
		(GC0310_F_NUMBER_DEM << 16) |\
		(GC0310_F_NUMBER_DEFAULT_NUM << 8) | GC0310_F_NUMBER_DEM)





static struct gc_register  gc0310_exposure_neg4[] = {
	{GC_8BIT, 0xfe, 0x01},
	{GC_8BIT, 0x13, 0x40},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0xd5, 0xc0},
	{GC_8BIT, 0xfe, 0x00},
	/* end of the list marker */
	{GC_TOK_TERM, 0, 0},

};

static struct gc_register  gc0310_exposure_neg3[] = {
	{GC_8BIT, 0xfe, 0x01},
	{GC_8BIT, 0x13, 0x50},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0xd5, 0xd0},
	{GC_8BIT, 0xfe, 0x00},
	/* end of the list marker */
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register  gc0310_exposure_neg2[] = {
	{GC_8BIT, 0xfe, 0x01},
	{GC_8BIT, 0x13, 0x60},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0xd5, 0xe0},
	{GC_8BIT, 0xfe, 0x00},
	/* end of the list marker */
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register  gc0310_exposure_neg1[] = {
	{GC_8BIT, 0xfe, 0x01},
	{GC_8BIT, 0x13, 0x70},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0xd5, 0xf0},
	{GC_8BIT, 0xfe, 0x00},
	/* end of the list marker */
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register  gc0310_exposure_zero[] = {
	{GC_8BIT, 0xfe, 0x01},
	{GC_8BIT, 0x13, 0x80},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0xd5, 0x00},
	{GC_8BIT, 0xfe, 0x00},
	/* end of the list marker */
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register  gc0310_exposure_pos1[] = {
	{GC_8BIT, 0xfe, 0x01},
	{GC_8BIT, 0x13, 0x98},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0xd5, 0x10},
	{GC_8BIT, 0xfe, 0x00},
	/* end of the list marker */
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register  gc0310_exposure_pos2[] = {
	{GC_8BIT, 0xfe, 0x01},
	{GC_8BIT, 0x13, 0xb0},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0xd5, 0x20},
	{GC_8BIT, 0xfe, 0x00},
	/* end of the list marker */
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register  gc0310_exposure_pos3[] = {
	{GC_8BIT, 0xfe, 0x01},
	{GC_8BIT, 0x13, 0xc0},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0xd5, 0x30},
	{GC_8BIT, 0xfe, 0x00},
	/* end of the list marker */
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register  gc0310_exposure_pos4[] = {
	{GC_8BIT, 0xfe, 0x01},
	{GC_8BIT, 0x13, 0xd0},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0xd5, 0x50},
	{GC_8BIT, 0xfe, 0x00},
	/* end of the list marker */
	{GC_TOK_TERM, 0, 0},
};

/* For GC0310 setting, the value is skewed by 2 */
static struct gc_table_map gc0310_exposure_tables[] = {
	{-4, gc0310_exposure_neg4},
	{-3, gc0310_exposure_neg3},
	{-2, gc0310_exposure_neg2},
	{-1, gc0310_exposure_neg1},
	{0, gc0310_exposure_zero},
	{1, gc0310_exposure_pos1},
	{2, gc0310_exposure_pos2},
	{3, gc0310_exposure_pos3},
	{4, gc0310_exposure_pos4},
};

/* 640x480 settings */
static struct gc_register const gc0310_640x480[] = {
	{ GC_TOK_TERM, 0, 0}
};

/* global settings, 640x480 */
static struct gc_register const gc0310_global_settings[] = {

	{ GC_8BIT, 0xfe, 0xf0 },
	{ GC_8BIT, 0xfe, 0xf0 },
	{ GC_8BIT, 0xfe, 0x00 },
	{ GC_8BIT, 0xfc, 0x0e },
	{ GC_8BIT, 0xfc, 0x0e },
	{ GC_8BIT, 0xf2, 0x80 },
	{ GC_8BIT, 0xf3, 0x00 },

	{ GC_8BIT, 0xf7, 0x1b },
	{ GC_8BIT, 0xf8, 0x05 },

	{ GC_8BIT, 0xf9, 0x8e },
	{ GC_8BIT, 0xfa, 0x11 },
	
	{ GC_8BIT, 0x00, 0x2f },
	{ GC_8BIT, 0x01, 0x0f },
	{ GC_8BIT, 0x02, 0x04 },
	{ GC_8BIT, 0x03, 0x02 },
	{ GC_8BIT, 0x04, 0x40 },
	{ GC_8BIT, 0x09, 0x00 },
	{ GC_8BIT, 0x0a, 0x00 },
	{ GC_8BIT, 0x0b, 0x00 },
	{ GC_8BIT, 0x0c, 0x06 },
	{ GC_8BIT, 0x0d, 0x01 },
	{ GC_8BIT, 0x0e, 0xe8 },
	{ GC_8BIT, 0x0f, 0x02 },
	{ GC_8BIT, 0x10, 0x88 },
	{ GC_8BIT, 0x16, 0x00 },
	{ GC_8BIT, 0x17, 0x14 },
	{ GC_8BIT, 0x18, 0x1a },
	{ GC_8BIT, 0x19, 0x14 },
	{ GC_8BIT, 0x1b, 0x48 },
	{ GC_8BIT, 0x1e, 0x6b },
	{ GC_8BIT, 0x1f, 0x28 },
	{ GC_8BIT, 0x20, 0x89 },
	{ GC_8BIT, 0x21, 0x49 },
	{ GC_8BIT, 0x22, 0xb0 },
	{ GC_8BIT, 0x23, 0x04 },
	{ GC_8BIT, 0x24, 0x16 },
	{ GC_8BIT, 0x34, 0x20 },
	
	{ GC_8BIT, 0x26, 0x23 },
	{ GC_8BIT, 0x28, 0xff },
	{ GC_8BIT, 0x29, 0x00 },
	{ GC_8BIT, 0x33, 0x18 },
	{ GC_8BIT, 0x37, 0x20 },
	{ GC_8BIT, 0x47, 0x80 },
	{ GC_8BIT, 0x4e, 0x66 },
	{ GC_8BIT, 0xa8, 0x02 },
	{ GC_8BIT, 0xa9, 0x80 },
	
	{ GC_8BIT, 0x40, 0xff },
	{ GC_8BIT, 0x41, 0x21 },
	{ GC_8BIT, 0x42, 0xcf },
	{ GC_8BIT, 0x44, 0x01 },
	{ GC_8BIT, 0x45, 0xaf },
	{ GC_8BIT, 0x46, 0x02 },
	{ GC_8BIT, 0x4a, 0x11 },
	{ GC_8BIT, 0x4b, 0x01 },
	{ GC_8BIT, 0x4c, 0x20 },
	{ GC_8BIT, 0x4d, 0x05 },
	{ GC_8BIT, 0x4f, 0x01 },
	{ GC_8BIT, 0x50, 0x01 },
	{ GC_8BIT, 0x55, 0x01 },
	{ GC_8BIT, 0x56, 0xe0 },
	{ GC_8BIT, 0x57, 0x02 },
	{ GC_8BIT, 0x58, 0x80 },
	
	{ GC_8BIT, 0x70, 0x70 },
	{ GC_8BIT, 0x5a, 0x84 },
	{ GC_8BIT, 0x5b, 0xc9 },
	{ GC_8BIT, 0x5c, 0xed },
	{ GC_8BIT, 0x77, 0x74 },
	{ GC_8BIT, 0x78, 0x40 },
	{ GC_8BIT, 0x79, 0x5f },
	
	{ GC_8BIT, 0x82, 0x1f },
	{ GC_8BIT, 0x83, 0x0b },
	{ GC_8BIT, 0x89, 0xf0 },
	
	{ GC_8BIT, 0x8f, 0xff },
	{ GC_8BIT, 0x90, 0x9f },
	{ GC_8BIT, 0x91, 0x90 },
	{ GC_8BIT, 0x92, 0x03 },
	{ GC_8BIT, 0x93, 0x03 },
	{ GC_8BIT, 0x94, 0x05 },
	{ GC_8BIT, 0x95, 0x65 },
	{ GC_8BIT, 0x96, 0xf0 },
	
	{ GC_8BIT, 0xfe, 0x00 },
	{ GC_8BIT, 0x9a, 0x20 },
	{ GC_8BIT, 0x9b, 0x80 },
	{ GC_8BIT, 0x9c, 0x40 },
	{ GC_8BIT, 0x9d, 0x80 },
	{ GC_8BIT, 0xa1, 0x30 },
	{ GC_8BIT, 0xa2, 0x32 },
	{ GC_8BIT, 0xa4, 0x30 },
	{ GC_8BIT, 0xa5, 0x30 },
	{ GC_8BIT, 0xaa, 0x50 },
	{ GC_8BIT, 0xac, 0x22 },
	 
	{ GC_8BIT, 0xBF, 0x08 },
	{ GC_8BIT, 0xc0, 0x16 },
	{ GC_8BIT, 0xc1, 0x28 },
	{ GC_8BIT, 0xc2, 0x41 },
	{ GC_8BIT, 0xc3, 0x5a },
	{ GC_8BIT, 0xc4, 0x6c },
	{ GC_8BIT, 0xc5, 0x7a },
	{ GC_8BIT, 0xc6, 0x96 },
	{ GC_8BIT, 0xc7, 0xac },
	{ GC_8BIT, 0xc8, 0xbc },
	{ GC_8BIT, 0xc9, 0xC9 },
	{ GC_8BIT, 0xcA, 0xD3 },
	{ GC_8BIT, 0xcB, 0xDD },
	{ GC_8BIT, 0xcC, 0xE5 },
	{ GC_8BIT, 0xcD, 0xF1 },
	{ GC_8BIT, 0xcE, 0xFA },
	{ GC_8BIT, 0xcF, 0xFF },
	
	{ GC_8BIT, 0xd0, 0x40 },
	{ GC_8BIT, 0xd1, 0x22 },
	{ GC_8BIT, 0xd2, 0x22 },
	{ GC_8BIT, 0xd3, 0x3c },
	{ GC_8BIT, 0xd6, 0xf2 },
	{ GC_8BIT, 0xd7, 0x1b },
	{ GC_8BIT, 0xd8, 0x18 },
	{ GC_8BIT, 0xdd, 0x03 },
	
	{ GC_8BIT, 0xfe, 0x01 },
	{ GC_8BIT, 0x05, 0x30 },
	{ GC_8BIT, 0x06, 0x75 },
	{ GC_8BIT, 0x07, 0x40 },
	{ GC_8BIT, 0x08, 0xb0 },
	{ GC_8BIT, 0x0a, 0xc5 },
	{ GC_8BIT, 0x0b, 0x11 },
	{ GC_8BIT, 0x0c, 0x00 },
	{ GC_8BIT, 0x12, 0x52 },
	{ GC_8BIT, 0x13, 0x38 },
	{ GC_8BIT, 0x18, 0x95 },
	{ GC_8BIT, 0x19, 0x96 },
	{ GC_8BIT, 0x1f, 0x20 },
	{ GC_8BIT, 0x20, 0xc0 },
	{ GC_8BIT, 0x3e, 0x40 },
	{ GC_8BIT, 0x3f, 0x57 },
	{ GC_8BIT, 0x40, 0x7d },
	{ GC_8BIT, 0x03, 0x60 },
	{ GC_8BIT, 0x44, 0x02 },
	
	{ GC_8BIT, 0x1c, 0x91 },
	{ GC_8BIT, 0x21, 0x15 },
	{ GC_8BIT, 0x50, 0x80 },
	{ GC_8BIT, 0x56, 0x04 },
	{ GC_8BIT, 0x59, 0x08 },
	{ GC_8BIT, 0x5b, 0x02 },
	{ GC_8BIT, 0x61, 0x8d },
	{ GC_8BIT, 0x62, 0xa7 },
	{ GC_8BIT, 0x63, 0xd0 },
	{ GC_8BIT, 0x65, 0x06 },
	{ GC_8BIT, 0x66, 0x06 },
	{ GC_8BIT, 0x67, 0x84 },
	{ GC_8BIT, 0x69, 0x08 },
	{ GC_8BIT, 0x6a, 0x25 },
	{ GC_8BIT, 0x6b, 0x01 },
	{ GC_8BIT, 0x6c, 0x00 },
	{ GC_8BIT, 0x6d, 0x02 },
	{ GC_8BIT, 0x6e, 0xf0 },
	{ GC_8BIT, 0x6f, 0x80 },
	{ GC_8BIT, 0x76, 0x80 },
	{ GC_8BIT, 0x78, 0xaf },
	{ GC_8BIT, 0x79, 0x75 },
	{ GC_8BIT, 0x7a, 0x40 },
	{ GC_8BIT, 0x7b, 0x50 },
	{ GC_8BIT, 0x7c, 0x0c },
	{ GC_8BIT, 0xa4, 0xb9 },
	{ GC_8BIT, 0xa5, 0xa0 },
	{ GC_8BIT, 0x90, 0xc9 },
	{ GC_8BIT, 0x91, 0xbe },
	{ GC_8BIT, 0xa6, 0xb8 },
	{ GC_8BIT, 0xa7, 0x95 },
	{ GC_8BIT, 0x92, 0xe6 },
	{ GC_8BIT, 0x93, 0xca },
	{ GC_8BIT, 0xa9, 0xb6 },
	{ GC_8BIT, 0xaa, 0x89 },
	{ GC_8BIT, 0x95, 0x23 },
	{ GC_8BIT, 0x96, 0xe7 },
	{ GC_8BIT, 0xab, 0x9d },
	{ GC_8BIT, 0xac, 0x80 },
	{ GC_8BIT, 0x97, 0x43 },
	{ GC_8BIT, 0x98, 0x24 },
	{ GC_8BIT, 0xae, 0xb7 },
	{ GC_8BIT, 0xaf, 0x9e },
	{ GC_8BIT, 0x9a, 0x43 },
	{ GC_8BIT, 0x9b, 0x24 },
	{ GC_8BIT, 0xb0, 0xc8 },
	{ GC_8BIT, 0xb1, 0x97 },
	{ GC_8BIT, 0x9c, 0xc4 },
	{ GC_8BIT, 0x9d, 0x44 },
	{ GC_8BIT, 0xb3, 0xb7 },
	{ GC_8BIT, 0xb4, 0x7f },
	{ GC_8BIT, 0x9f, 0xc7 },
	{ GC_8BIT, 0xa0, 0xc8 },
	{ GC_8BIT, 0xb5, 0x00 },
	{ GC_8BIT, 0xb6, 0x00 },
	{ GC_8BIT, 0xa1, 0x00 },
	{ GC_8BIT, 0xa2, 0x00 },
	{ GC_8BIT, 0x86, 0x60 },
	{ GC_8BIT, 0x87, 0x08 },
	{ GC_8BIT, 0x88, 0x00 },
	{ GC_8BIT, 0x89, 0x00 },
	{ GC_8BIT, 0x8b, 0xde },
	{ GC_8BIT, 0x8c, 0x80 },
	{ GC_8BIT, 0x8d, 0x00 },
	{ GC_8BIT, 0x8e, 0x00 },
	{ GC_8BIT, 0x94, 0x55 },
	{ GC_8BIT, 0x99, 0xa6 },
	{ GC_8BIT, 0x9e, 0xaa },
	{ GC_8BIT, 0xa3, 0x0a },
	{ GC_8BIT, 0x8a, 0x0a },
	{ GC_8BIT, 0xa8, 0x55 },
	{ GC_8BIT, 0xad, 0x55 },
	{ GC_8BIT, 0xb2, 0x55 },
	{ GC_8BIT, 0xb7, 0x05 },
	{ GC_8BIT, 0x8f, 0x05 },

	{ GC_8BIT, 0xfe, 0x01 },
	{ GC_8BIT, 0x90, 0x00 },
	{ GC_8BIT, 0x91, 0x00 },
	{ GC_8BIT, 0x92, 0xf5 },
	{ GC_8BIT, 0x93, 0xdf },
	{ GC_8BIT, 0x95, 0x1e },
	{ GC_8BIT, 0x96, 0xf5 },
	{ GC_8BIT, 0x97, 0x41 },
	{ GC_8BIT, 0x98, 0x1f },
	{ GC_8BIT, 0x9a, 0x41 },
	{ GC_8BIT, 0x9b, 0x1f },
	{ GC_8BIT, 0x9c, 0x92 },
	{ GC_8BIT, 0x9d, 0x41 },
	{ GC_8BIT, 0x9f, 0x00 },
	{ GC_8BIT, 0xa0, 0x00 },
	{ GC_8BIT, 0xa1, 0x00 },
	{ GC_8BIT, 0xa2, 0x00 },
	{ GC_8BIT, 0x86, 0x00 },
	{ GC_8BIT, 0x87, 0x00 },
	{ GC_8BIT, 0x88, 0x00 },
	{ GC_8BIT, 0x89, 0x00 },
	{ GC_8BIT, 0xa4, 0x00 },
	{ GC_8BIT, 0xa5, 0x00 },
	{ GC_8BIT, 0xa6, 0xbe },
	{ GC_8BIT, 0xa7, 0xa7 },
	{ GC_8BIT, 0xa9, 0xc6 },
	{ GC_8BIT, 0xaa, 0x9a },
	{ GC_8BIT, 0xab, 0xaf },
	{ GC_8BIT, 0xac, 0x94 },
	{ GC_8BIT, 0xae, 0xc4 },
	{ GC_8BIT, 0xaf, 0xaf },
	{ GC_8BIT, 0xb0, 0xc8 },
	{ GC_8BIT, 0xb1, 0xac },
	{ GC_8BIT, 0xb3, 0x00 },
	{ GC_8BIT, 0xb4, 0x00 },
	{ GC_8BIT, 0xb5, 0x00 },
	{ GC_8BIT, 0xb6, 0x00 },
	{ GC_8BIT, 0x8b, 0x00 },
	{ GC_8BIT, 0x8c, 0x00 },
	{ GC_8BIT, 0x8d, 0x00 },
	{ GC_8BIT, 0x8e, 0x00 },
	{ GC_8BIT, 0x94, 0x50 },
	{ GC_8BIT, 0x99, 0xa6 },
	{ GC_8BIT, 0x9e, 0xaa },
	{ GC_8BIT, 0xa3, 0x00 },
	{ GC_8BIT, 0x8a, 0x00 },
	{ GC_8BIT, 0xa8, 0x50 },
	{ GC_8BIT, 0xad, 0x55 },
	{ GC_8BIT, 0xb2, 0x55 },
	{ GC_8BIT, 0xb7, 0x00 },
	{ GC_8BIT, 0x8f, 0x00 },
	{ GC_8BIT, 0xb8, 0xd6 },
	{ GC_8BIT, 0xb9, 0x8c },

	{ GC_8BIT, 0xfe, 0x01 },
	{ GC_8BIT, 0xd0, 0x4c },
	{ GC_8BIT, 0xd1, 0xf1 },
	{ GC_8BIT, 0xd2, 0x00 },
	{ GC_8BIT, 0xd3, 0x05 },
	{ GC_8BIT, 0xd4, 0x5f },
	{ GC_8BIT, 0xd5, 0xf0 },
	{ GC_8BIT, 0xd6, 0x40 },
	{ GC_8BIT, 0xd7, 0xf0 },
	{ GC_8BIT, 0xd8, 0xf8 },
	{ GC_8BIT, 0xd9, 0xf8 },
	{ GC_8BIT, 0xda, 0x46 },
	{ GC_8BIT, 0xdb, 0xe8 },
	
	{ GC_8BIT, 0xfe, 0x01 },
	{ GC_8BIT, 0xc1, 0x3c },
	{ GC_8BIT, 0xc2, 0x50 },
	{ GC_8BIT, 0xc3, 0x00 },
	{ GC_8BIT, 0xc4, 0x40 },
	{ GC_8BIT, 0xc5, 0x30 },
	{ GC_8BIT, 0xc6, 0x30 },
	{ GC_8BIT, 0xc7, 0x1c },
	{ GC_8BIT, 0xc8, 0x00 },
	{ GC_8BIT, 0xc9, 0x00 },
	{ GC_8BIT, 0xdc, 0x20 },
	{ GC_8BIT, 0xdd, 0x10 },
	{ GC_8BIT, 0xdf, 0x00 },
	{ GC_8BIT, 0xde, 0x00 },
	
	{ GC_8BIT, 0x01, 0x10 },
	{ GC_8BIT, 0x0b, 0x31 },
	{ GC_8BIT, 0x0e, 0x50 },
	{ GC_8BIT, 0x0f, 0x0f },
	{ GC_8BIT, 0x10, 0x6e },
	{ GC_8BIT, 0x12, 0xa0 },
	{ GC_8BIT, 0x15, 0x60 },
	{ GC_8BIT, 0x16, 0x60 },
	{ GC_8BIT, 0x17, 0xe0 },
	
	{ GC_8BIT, 0xcc, 0x0c },
	{ GC_8BIT, 0xcd, 0x10 },
	{ GC_8BIT, 0xce, 0xa0 },
	{ GC_8BIT, 0xcf, 0xe6 },
	
	{ GC_8BIT, 0x45, 0xf7 },
	{ GC_8BIT, 0x46, 0xff },
	{ GC_8BIT, 0x47, 0x15 },
	{ GC_8BIT, 0x48, 0x03 },
	{ GC_8BIT, 0x4f, 0x60 },
	

	{ GC_8BIT, 0xfe, 0x00 },

	{ GC_8BIT, 0x05, 0x01 },
	{ GC_8BIT, 0x06, 0x26 },
	{ GC_8BIT, 0x07, 0x00 },
	{ GC_8BIT, 0x08, 0x4e },


	{ GC_8BIT, 0xfe, 0x01 },
	{ GC_8BIT, 0x25, 0x00 },
	{ GC_8BIT, 0x26, 0x60 },


	{ GC_8BIT, 0x27, 0x02 },
	{ GC_8BIT, 0x28, 0x40 },
	{ GC_8BIT, 0x29, 0x03 },
	{ GC_8BIT, 0x2a, 0x00 },
	{ GC_8BIT, 0x2b, 0x03 },
	{ GC_8BIT, 0x2c, 0xc0 },
	{ GC_8BIT, 0x2d, 0x05 },
	{ GC_8BIT, 0x2e, 0xa0 },
	{ GC_8BIT, 0x3c, 0x20 },
	{ GC_8BIT, 0xfe, 0x00 },

	{ GC_8BIT, 0xfe, 0x00 },
	


	
	{ GC_8BIT, 0xfe, 0x03 },
	{ GC_8BIT, 0x10, 0x80 },
	{ GC_8BIT, 0xfe, 0x00 },
	
	{ GC_8BIT, 0xfe, 0x03 },
	{ GC_8BIT, 0x01, 0x03 },
	{ GC_8BIT, 0x02, 0x22 },
	{ GC_8BIT, 0x03, 0x94 },
	{ GC_8BIT, 0x04, 0x01 },
	{ GC_8BIT, 0x05, 0x00 },
	{ GC_8BIT, 0x06, 0x80 },
	{ GC_8BIT, 0x11, 0x1e },
	{ GC_8BIT, 0x12, 0x00 },
	{ GC_8BIT, 0x13, 0x05 },
	{ GC_8BIT, 0x15, 0x12 },
	{ GC_8BIT, 0x17, 0xf0 },

	{ GC_8BIT, 0x21, 0x02 },
	{ GC_8BIT, 0x22, 0x02 },
	{ GC_8BIT, 0x23, 0x01 },
	{ GC_8BIT, 0x29, 0x02 },
	{ GC_8BIT, 0x2a, 0x02 },
	{ GC_8BIT, 0xfe, 0x00 },
	{ GC_8BIT, 0x4d, 0x01 },
	{ GC_8BIT, 0x86, 0x50 },
	{ GC_8BIT, 0x87, 0x30 },
	{ GC_8BIT, 0x88, 0x15 },

	{ GC_TOK_TERM, 0, 0}

};

 

struct gc_resolution gc0310_res_video[] = {
	{
		.desc = "gc0310_640x480",
		.regs = gc0310_640x480,
		.width = 640,
		.height = 480,
		.fps = 30,
		.pixels_per_line = 640,
		.lines_per_frame = 480,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.used = 0,
		.skip_frames = 4,
	},
};

#define gc0310_res_preview	gc0310_res_video
#define gc0310_res_still	gc0310_res_video

struct gc_mode_info gc0310_mode_info[] = {
	[0] = {
		.init_settings = gc0310_global_settings,
		.res_preview = gc0310_res_preview,
		.res_still = gc0310_res_still,
		.res_video = gc0310_res_video,
		.n_res_preview = ARRAY_SIZE(gc0310_res_preview),
		.n_res_still = ARRAY_SIZE(gc0310_res_still),
		.n_res_video = ARRAY_SIZE(gc0310_res_video),
	},
};

#define GC0310_RES_WIDTH_MAX	640
#define GC0310_RES_HEIGHT_MAX	480

struct gc_max_res gc0310_max_res[] = {
	[0] = {
		.res_max_width = GC0310_RES_WIDTH_MAX,
		.res_max_height = GC0310_RES_HEIGHT_MAX,
	},
};


static struct gc_register gc0310_fmt_yuv422_uyvy16[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x44, 0x00},
	{GC_TOK_TERM, 0, 0},
};


static struct gc_register gc0310_fmt_yuv422_yuyv[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x44, 0x02},
	{GC_TOK_TERM, 0, 0},
};


static struct gc_register gc0310_fmt_yuv422_yvyu[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x44, 0x03},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_fmt_yuv422_vyuy[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x44, 0x01},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_fmt_yuv422_uyvy[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x44, 0x00},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_fmt_raw[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x44, 0x18},
	{GC_TOK_TERM, 0, 0},
};


static struct gc_mbus_fmt gc0310_mbus_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.code		= V4L2_MBUS_FMT_UYVY8_1X16,
		.regs		= gc0310_fmt_yuv422_uyvy16,
		.size		= ARRAY_SIZE(gc0310_fmt_yuv422_uyvy16),
		.bpp		= 2,
	},
	{
		.desc		= "YUYV 4:2:2",
		.code		= V4L2_MBUS_FMT_YUYV8_2X8,
		.regs		= gc0310_fmt_yuv422_yuyv,
		.size		= ARRAY_SIZE(gc0310_fmt_yuv422_yuyv),
		.bpp		= 2,
	},
	{
		.desc		= "YVYU 4:2:2",
		.code		= V4L2_MBUS_FMT_YVYU8_2X8,
		.regs		= gc0310_fmt_yuv422_yvyu,
		.size		= ARRAY_SIZE(gc0310_fmt_yuv422_yvyu),
		.bpp		= 2,
	},
	{
		.desc		= "UYVY 4:2:2",
		.code		= V4L2_MBUS_FMT_UYVY8_2X8,
		.regs		= gc0310_fmt_yuv422_uyvy,
		.size		= ARRAY_SIZE(gc0310_fmt_yuv422_uyvy),
		.bpp		= 2,
	},
	{
		.desc		= "VYUY 4:2:2",
		.code		= V4L2_MBUS_FMT_VYUY8_2X8,
		.regs		= gc0310_fmt_yuv422_vyuy,
		.size		= ARRAY_SIZE(gc0310_fmt_yuv422_vyuy),
		.bpp		= 2,
	},
	{
		.desc		= "Raw RGB Bayer",
		.code		= V4L2_MBUS_FMT_SBGGR8_1X8,
		.regs		= gc0310_fmt_raw,
		.size		= ARRAY_SIZE(gc0310_fmt_raw),
		.bpp		= 1
	},

};

/* GC0310 stream_on and stream_off reg sequences */
static struct gc_register gc0310_stream_on[] = {
    {GC_8BIT, 0xfe, 0x30},
	{GC_8BIT, 0xfe, 0x03},
	{GC_8BIT, 0x10, 0x94},
	{GC_8BIT, 0xfe, 0x00},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_stream_off[] = {
	{GC_8BIT, 0xfe, 0x03},
	{GC_8BIT, 0x10, 0x84},
	{GC_8BIT, 0xfe, 0x00},
	{GC_TOK_TERM, 0, 0},
};


static struct gc_table_map gc0310_stream_tables[] = {
	{ 0x0, gc0310_stream_off },
	{ 0x1, gc0310_stream_on  },
};

/* GC0310 vertical flip settings  */
static struct gc_register gc0310_vflip_on_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT_RMW_OR, GC0310_IMG_ORIENTATION, 0x02},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_vflip_off_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT_RMW_AND, GC0310_IMG_ORIENTATION, ~0x02},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_table_map gc0310_vflip_tables[] = {
	{0x0, gc0310_vflip_off_table },
	{0x1, gc0310_vflip_on_table  },
};

/* GC0310 horizontal flip settings */
static struct gc_register gc0310_hflip_on_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT_RMW_OR, GC0310_IMG_ORIENTATION, 0x01},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_hflip_off_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT_RMW_AND, GC0310_IMG_ORIENTATION, ~0x01},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_table_map gc0310_hflip_tables[] = {
	{ 0x0, gc0310_hflip_off_table },
	{ 0x1, gc0310_hflip_on_table  },
};

/* GC0310 scene mode settings */
static struct gc_register gc0310_night_mode_on_table[] = {
	{GC_8BIT, 0xfe, 0x01},
	{GC_8BIT, GC0310_REG_MAX_AEC, 0x60},
	{GC_TOK_TERM, 0, 0},
};

/* Normal scene mode */
static struct gc_register gc0310_night_mode_off_table[] = {
	{GC_8BIT, 0xfe, 0x01},
	{GC_8BIT, GC0310_REG_MAX_AEC, 0x40},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_table_map gc0310_scene_mode_tables[] = {
	{ SCENE_MODE_AUTO, gc0310_night_mode_off_table },
	{ SCENE_MODE_NIGHT, gc0310_night_mode_on_table },
};

/* GC0310 color settings - need to check  works*/
static struct gc_register gc0310_color_none_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x43, 0x00},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_color_mono_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x43, 0x12},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_color_inverse_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x43, 0x01},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_color_sepia_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x43, 0x32},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_color_blue_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x43, 0x62},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_color_green_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x43, 0x52},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_color_skin_low_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x43, 0x92},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_color_skin_med_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x43, 0x72},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_color_skin_high_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x43, 0x82},
	{GC_TOK_TERM, 0, 0},
};

/* GC0310 settings for effect modes */
/* TODO: Get values from application - right now these are assumed values */
static struct gc_table_map gc0310_color_effect_tables[] = {
	{ 0x1, gc0310_color_none_table },
	{ 0x0, gc0310_color_mono_table },
	{ 0x2, gc0310_color_inverse_table },
	{ 0x3, gc0310_color_sepia_table },
	{ 0x4, gc0310_color_blue_table },
	{ 0x5, gc0310_color_green_table },
	{ 0x6, gc0310_color_skin_low_table },
	{ 0x7, gc0310_color_skin_med_table },
	{ 0x8, gc0310_color_skin_high_table },

};

/* GC0310 settings for auto white balance */
static struct gc_register gc0310_awb_mode_auto_table[] = {
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x77, 0x61},
	{GC_8BIT, 0x78, 0x40},
	{GC_8BIT, 0x79, 0x61},
	{GC_8BIT_RMW_OR, GC0310_REG_BLOCK_ENABLE, 0x02},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_awb_mode_incandescent_table[] = {
	{GC_8BIT_RMW_AND, GC0310_REG_BLOCK_ENABLE, 0xfd},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x77, 0x50},
	{GC_8BIT, 0x78, 0x40},
	{GC_8BIT, 0x79, 0xa8},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_awb_mode_fluorescent_table[] = {
	{GC_8BIT_RMW_AND, GC0310_REG_BLOCK_ENABLE, 0xfd},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x77, 0x72},
	{GC_8BIT, 0x78, 0x40},
	{GC_8BIT, 0x79, 0x5b},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_awb_mode_daylight_table[] = {
	{GC_8BIT_RMW_AND, GC0310_REG_BLOCK_ENABLE, 0xfd},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x77, 0x70},
	{GC_8BIT, 0x78, 0x40},
	{GC_8BIT, 0x79, 0x50},
	{GC_TOK_TERM, 0, 0},
};

static struct gc_register gc0310_awb_mode_cloudy_table[] = {
	{GC_8BIT_RMW_AND, GC0310_REG_BLOCK_ENABLE, 0xfd},
	{GC_8BIT, 0xfe, 0x00},
	{GC_8BIT, 0x77, 0x58},
	{GC_8BIT, 0x78, 0x40},
	{GC_8BIT, 0x79, 0x50},
	{GC_TOK_TERM, 0, 0},
};


static struct gc_table_map gc0310_awb_mode_tables[] = {
	{ AWB_MODE_AUTO, gc0310_awb_mode_auto_table },
	{ AWB_MODE_INCANDESCENT, gc0310_awb_mode_incandescent_table },
	{ AWB_MODE_FLUORESCENT, gc0310_awb_mode_fluorescent_table },
	{ AWB_MODE_DAYLIGHT, gc0310_awb_mode_daylight_table },
	{ AWB_MODE_CLOUDY, gc0310_awb_mode_cloudy_table },
};


struct gc_product_info gc0310_product_info = {
	.name = "gc0310soc",
	.sensor_id = 0xa310,
	.mode_info = gc0310_mode_info,

	/* Optional controls */
	/*.ctrl_config = gc0310_ctrls,
	 .num_ctrls = ARRAY_SIZE(gc0310_ctrls),
*/

	.max_res = gc0310_max_res,

	.mbus_formats = gc0310_mbus_formats,
	.num_mbus_formats = ARRAY_SIZE(gc0310_mbus_formats),

	.focal	= GC0310_FOCAL_VALUE,
	.f_number =  GC0310_FNUMBER_VALUE,
	.f_number_range = GC0310_FNUMBER_RANGE_VALUE,
   
    .reg_expo_coarse = GC0310_REG_EXPO_COARSE,
    

	.settings_tables = {
		GC_ADD_SETTINGS_TABLES(GC_SETTING_STREAM, gc0310_stream_tables),
		GC_ADD_SETTINGS_TABLES(GC_SETTING_EXPOSURE, gc0310_exposure_tables),
		GC_ADD_SETTINGS_TABLES(GC_SETTING_VFLIP, gc0310_vflip_tables),
		GC_ADD_SETTINGS_TABLES(GC_SETTING_HFLIP, gc0310_hflip_tables),
		GC_ADD_SETTINGS_TABLES(GC_SETTING_SCENE_MODE, gc0310_scene_mode_tables),
		GC_ADD_SETTINGS_TABLES(GC_SETTING_COLOR_EFFECT, gc0310_color_effect_tables),
		GC_ADD_SETTINGS_TABLES(GC_SETTING_AWB_MODE, gc0310_awb_mode_tables),
	},

};


#endif /* __GC_0310_H__ */
