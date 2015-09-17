/**
 * file xf86drmcsc.c
 * User-level interface to DRM device
 * to control color space conversion
 * author Uma Shankar <uma.shankar@intel.com>
 */

/*
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#define stat_t struct stat
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <stdarg.h>
#include "i915_drm.h"

#define false 0
#define true 1
#define MAX_CSC_COEFFICIENTS 9
#define CSC_BIT_SHIFT(x) (1 << (x))
#define CSC_SETBIT(x,y) ((x) |= (y))
#define VLV2CSC_MAX_MANTISSA_PRECISION 10
#define CSC_TWOSCOMPLEMENT(x) ((~x)+1)
#define BIT10 (1<<10)

static float g_defaultCSCInit[9] = { 1, 0 , 0, 0, 1, 0, 0, 0, 1};
static struct drm_intel_csc_params g_defaultCSCParamas;

union VLV2_CSC_FLOAT {
    unsigned short Value;
    struct {
        unsigned short Binary:11; /* Bit 10:0 */
        unsigned short Sign:1; /* Bit 11 */
        unsigned short Reserved:4; /* Bit 15:12 */
    };
};

struct VLV2_CSC_REG_COEFF {
    union VLV2_CSC_FLOAT wgCSCCoeff[MAX_CSC_COEFFICIENTS];
};

static void BitReversal_func(unsigned short *pArg)
{
    unsigned short tmp1 = 0;
    unsigned short tmp2, cnt;

    tmp2 = *pArg;
    for (cnt = 0; cnt < VLV2CSC_MAX_MANTISSA_PRECISION; cnt++ ) {
        if(tmp2 & CSC_BIT_SHIFT(cnt))
            CSC_SETBIT(tmp1, CSC_BIT_SHIFT(VLV2CSC_MAX_MANTISSA_PRECISION -1 - cnt));
    }

    *pArg = tmp1;
}

static short int Convert_Coeff_ToBinary(struct drm_intel_csc_params *csc_params,
        struct VLV2_CSC_REG_COEFF *Coeff_binary)
{
    float coeff = 0;
    unsigned short twosCompliment, Binary;
    unsigned short  Bit_Count = 0;
    unsigned short count;
    short int bGreaterThanOne = false;
    union VLV2_CSC_FLOAT wgCSCFloat = {0};

    for (count = 0; count < MAX_CSC_COEFFICIENTS; count++) {
        coeff = csc_params->m_CSCCoeff[count];
        Bit_Count = 0;
        Binary = 0;
        bGreaterThanOne = false;

        if (coeff == 0) {
            Coeff_binary->wgCSCCoeff[count].Binary  = 0;
            Coeff_binary->wgCSCCoeff[count].Sign = 0;
            continue;
        }

        if (coeff < 0) {
            coeff = coeff * - 1;
        }

        /* Clip to valid range [-1.999 to +1.999] */
        if (coeff > 1.999f)
            coeff = 1.999f;

        if (coeff >= 1) {
            coeff = coeff - 1;
            bGreaterThanOne = 1;
        }

        do {
            coeff = coeff * 2;
            if (coeff >= 1) {
                CSC_SETBIT(Binary, CSC_BIT_SHIFT(Bit_Count));
                coeff = coeff - 1;
            }

            Bit_Count++;
        } while(coeff != 0 &&  Bit_Count < VLV2CSC_MAX_MANTISSA_PRECISION); /* 10-bit wide */

        /* Reverse  last 10 bits. */
        BitReversal_func(&Binary);
        if (bGreaterThanOne)
            CSC_SETBIT(Binary, BIT10); /* 11th bit is for first digit before radix 1.xxxxx */
        twosCompliment = Binary;

        /* convert to 2's compliment */
        if (csc_params->m_CSCCoeff[count] < 0) {
            twosCompliment = CSC_TWOSCOMPLEMENT(Binary);
            twosCompliment &= 0xFFF; /* mask other bits except  bit [11-0] */
        }

        wgCSCFloat.Value =  twosCompliment;
        Coeff_binary->wgCSCCoeff[count].Binary  = wgCSCFloat.Binary;
        Coeff_binary->wgCSCCoeff[count].Sign    = (short int)wgCSCFloat.Sign;
    }

    return true;
}

static short int Convert_Coeff_ToBSpecFormat(union CSC_COEFFICIENT_WG *wgCSCCoeff, struct VLV2_CSC_REG_COEFF *Coeff_binary)
{
    short int i;
    short int j;
    union VLV2_CSC_FLOAT wgFloat = {0};

    for(i = 0, j = 0; i < 6; i = i + 2, j = j + 3) {
        wgFloat.Binary = Coeff_binary->wgCSCCoeff[j].Binary;
        wgFloat.Sign = Coeff_binary->wgCSCCoeff[j].Sign;
        wgCSCCoeff[i].Coeff_2   = wgFloat.Value;

        wgFloat.Binary = Coeff_binary->wgCSCCoeff[j+1].Binary;
        wgFloat.Sign = Coeff_binary->wgCSCCoeff[j+1].Sign;
        wgCSCCoeff[i].Coeff_1   = wgFloat.Value;

        wgFloat.Binary = Coeff_binary->wgCSCCoeff[j+2].Binary;
        wgFloat.Sign = Coeff_binary->wgCSCCoeff[j+2].Sign;
        wgCSCCoeff[i+1].Coeff_2 = wgFloat.Value;

        wgCSCCoeff[i+1].Coeff_1 = 0;
    }

    return 0;
}

int calc_coeff(float *CSCCoeff, union CSC_COEFFICIENT_WG *wgCSCCoeff)
{
    struct drm_intel_csc_params input_csc_params;
    struct VLV2_CSC_REG_COEFF wgCSCRegCoeff_Binary;

    memcpy(input_csc_params.m_CSCCoeff, CSCCoeff, sizeof(float) * 9);
    Convert_Coeff_ToBinary(&input_csc_params,&wgCSCRegCoeff_Binary);
    Convert_Coeff_ToBSpecFormat(wgCSCCoeff, &wgCSCRegCoeff_Binary);

    return 0;
}
