/*
 * Support for Intel Camera Imaging ISP subsystem.
 *
 * Copyright (c) 2010 - 2014 Intel Corporation. All Rights Reserved.
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

#ifndef __ISP_BMA_BFA_TYPES_H_INCLUDED__
#define __ISP_BMA_BFA_TYPES_H_INCLUDED__

/* NUM_OF_SADS = ((SEARCH_AREA_HEIGHT - REF_BLOCK_HEIGHT)/PIXEL_SHIFT + 1)* \
					((SEARCH_AREA_WIDTH - REF_BLOCK_WIDTH)/PIXEL_SHIFT + 1) */

#define SADS(sw_h, sw_w, ref_h, ref_w, p_sh)    (((sw_h - ref_h)/p_sh + 1) * ((sw_w - ref_w)/p_sh + 1))
#define SADS_16x16_1    SADS(16, 16, 8, 8, 1)
#define SADS_16x16_2    SADS(16, 16, 8, 8, 2)
#define SADS_14x14_1    SADS(14, 14, 8, 8, 1)
#define SADS_14x14_2    SADS(14, 14, 8, 8, 2)

#define MATRIX_4x4_ROW    4
#define MATRIX_7x7_ROW    7
#define MATRIX_5x5_ROW    5
#define MATRIX_9x9_ROW    9

#define MATRIX_COL(r)     ((r % 2) ? (r + 1) : r)
#define MATRIX_4x4_COL    MATRIX_COL(4)
#define MATRIX_7x7_COL    MATRIX_COL(7)
#define MATRIX_5x5_COL    MATRIX_COL(5)
#define MATRIX_9x9_COL    MATRIX_COL(9)

#define STORE_SIZE_CALC(r)  CEIL_MUL(r*2, sizeof(int))
#define STORE_SIZE_4x4      STORE_SIZE_CALC(4)
#define STORE_SIZE_7x7      STORE_SIZE_CALC(7)
#define STORE_SIZE_5x5      STORE_SIZE_CALC(5)
#define STORE_SIZE_9x9      STORE_SIZE_CALC(9)

typedef tvector1w bma_output_16_2;
typedef tvector1w bma_output_14_2;

#ifdef IS_ISP_2600_SYSTEM
#if ISP_NWAY == 32 /*ISP_NWAY specific implementation */
typedef __register struct {
	tvector1w A_1;
	tvector1w A_2;
	tvector1w B_1;
	tvector1w B_2;
	tvector1w C_1;
	tvector1w C_2;
	tvector1w D_1;
	tvector1w D_2;

} bma_16x16_search_window;

typedef __register struct {
	tvector1w Ref_1;
	tvector1w Ref_2;
} ref_block_8x8;
#else /*ISP_NWAY == 32 */
	#error "only ISP_NWAY = 32 supported"
#endif /* ISP_NWAY == 32 */
#endif
typedef __register struct {
	tvector1w bma_lower;
	tvector1w bma_middle;
	tvector1w bma_upper;
} bma_output_16_1;

typedef __register struct {
	tvector1w bma_lower;
	tvector1w bma_upper;
} bma_output_14_1;

/* BFA structure definations */
typedef tvector1w tvector1wb4x8;
typedef __register struct {
	/* bnm: 4x8 (height x width) block of pixels at position row = n; col = m */
	tvector1wb4x8 b00;
	tvector1wb4x8 b08;
	tvector1wb4x8 b40;
	tvector1wb4x8 b48;
	tvector1wb4x8 b80;
	tvector1wb4x8 b88;
} s_12x16packed_matrix;

typedef __register struct {
	/* sw0 - sw2 : spatial weight LUT vectors. BFA_MAX_KWAY numbers of
	 * spatial weights are kept as in these vectors */
	tvector1w sw0;
	tvector1w sw1;
	tvector1w sw2;
	/* rw: range weight LUT vector. BFA_RW_LUT_SIZE numbers of range weights
	 * are kept in this vector */
	tvector1w rw;
} bfa_weights;


#endif /* __ISP_BMA_BFA_TYPES_H_INCLUDED__ */
