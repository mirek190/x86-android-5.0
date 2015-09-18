/*
 * Copyright ?? 2013 Intel Corporation
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Jani Nikula <jani.nikula@intel.com>
 *	   Shobhit Kumar <shobhit.kumar@intel.com>
 *     Lingyan Guo <lingyan.guo@intel.com>
 *
 *
 */

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_edid.h>
#include <drm/i915_drm.h>
#include <linux/slab.h>
#include <video/mipi_display.h>
#include <asm/intel-mid.h>
#include "i915_drv.h"
#include "intel_drv.h"
#include "intel_dsi.h"
#include "intel_dsi_cmd.h"
#include "dsi_mod_cpt_claa080fp01.h"

static u8 cpt_init_sequence3[]		= {0x8C, 0x0C};
static u8 cpt_init_sequence4[]		= {0xFD, 0x1A};
static u8 cpt_init_sequence5_1[]	= {0x83, 0xAA};
static u8 cpt_init_sequence5_2[]	= {0x84, 0x11};
static u8 cpt_init_sequence5_3[]	= {0xC0, 0x0E};
static u8 cpt_init_sequence5_4[]	= {0xC1, 0x12};
static u8 cpt_init_sequence5_5[]	= {0xC2, 0x25};
static u8 cpt_init_sequence5_6[]	= {0xC3, 0x34};
static u8 cpt_init_sequence5_7[]	= {0xC4, 0x3F};
static u8 cpt_init_sequence5_8[]	= {0xC5, 0x49};
static u8 cpt_init_sequence5_9[]	= {0xC6, 0x52};
static u8 cpt_init_sequence5_10[]	= {0xC7, 0x59};
static u8 cpt_init_sequence5_11[]	= {0xC8, 0x60};
static u8 cpt_init_sequence5_12[]	= {0xC9, 0xC8};
static u8 cpt_init_sequence5_13[]	= {0xCA, 0xC7};
static u8 cpt_init_sequence5_14[]	= {0xCB, 0xD6};
static u8 cpt_init_sequence5_15[]	= {0xCC, 0xD9};
static u8 cpt_init_sequence5_16[]	= {0xCD, 0xD7};
static u8 cpt_init_sequence5_17[]	= {0xCE, 0xD1};
static u8 cpt_init_sequence5_18[]	= {0xCF, 0xD3};
static u8 cpt_init_sequence5_19[]	= {0xD0, 0xD4};
static u8 cpt_init_sequence5_20[]	= {0xD1, 0xFF};
static u8 cpt_init_sequence5_21[]	= {0xD2, 0x08};
static u8 cpt_init_sequence5_22[]	= {0xD3, 0x2E};
static u8 cpt_init_sequence5_23[]	= {0xD4, 0x3B};
static u8 cpt_init_sequence5_24[]	= {0xD5, 0xAA};
static u8 cpt_init_sequence5_25[]	= {0xD6, 0xB3};
static u8 cpt_init_sequence5_26[]	= {0xD7, 0xBC};
static u8 cpt_init_sequence5_27[]	= {0xD8, 0xC5};
static u8 cpt_init_sequence5_28[]	= {0xD9, 0xD0};
static u8 cpt_init_sequence5_29[]	= {0xDA, 0xDB};
static u8 cpt_init_sequence5_30[]	= {0xDB, 0xE7};
static u8 cpt_init_sequence5_31[]	= {0xDC, 0xF7};
static u8 cpt_init_sequence5_32[]	= {0xDD, 0xFE};
static u8 cpt_init_sequence5_33[]	= {0xDE, 0x00};
static u8 cpt_init_sequence5_34[]	= {0xDF, 0x2E};
static u8 cpt_init_sequence5_35[]	= {0xE0, 0x0E};
static u8 cpt_init_sequence5_36[]	= {0xE1, 0x12};
static u8 cpt_init_sequence5_37[]	= {0xE2, 0x25};
static u8 cpt_init_sequence5_38[]	= {0xE3, 0x34};
static u8 cpt_init_sequence5_39[]	= {0xE4, 0x3F};
static u8 cpt_init_sequence5_40[]	= {0xE5, 0x49};
static u8 cpt_init_sequence5_41[]	= {0xE6, 0x52};
static u8 cpt_init_sequence5_42[]	= {0xE7, 0x59};
static u8 cpt_init_sequence5_43[]	= {0xE8, 0x60};
static u8 cpt_init_sequence5_44[]	= {0xE9, 0xC8};
static u8 cpt_init_sequence5_45[]	= {0xEA, 0xC7};
static u8 cpt_init_sequence5_46[]	= {0xEB, 0xD6};
static u8 cpt_init_sequence5_47[]	= {0xEC, 0xD9};
static u8 cpt_init_sequence5_48[]	= {0xED, 0xD7};
static u8 cpt_init_sequence5_49[]	= {0xEE, 0xD1};
static u8 cpt_init_sequence5_50[]	= {0xEF, 0xD3};
static u8 cpt_init_sequence5_51[]	= {0xF0, 0xD4};
static u8 cpt_init_sequence5_52[]	= {0xF1, 0xFF};
static u8 cpt_init_sequence5_53[]	= {0xF2, 0x08};
static u8 cpt_init_sequence5_54[]	= {0xF3, 0x2E};
static u8 cpt_init_sequence5_55[]	= {0xF4, 0x3B};
static u8 cpt_init_sequence5_56[]	= {0xF5, 0xAA};
static u8 cpt_init_sequence5_57[]	= {0xF6, 0xB3};
static u8 cpt_init_sequence5_58[]	= {0xF7, 0xBC};
static u8 cpt_init_sequence5_59[]	= {0xF8, 0xC5};
static u8 cpt_init_sequence5_60[]	= {0xF9, 0xD0};
static u8 cpt_init_sequence5_61[]	= {0xFA, 0xDB};
static u8 cpt_init_sequence5_62[]	= {0xFB, 0xE7};
static u8 cpt_init_sequence5_63[]	= {0xFC, 0xF7};
static u8 cpt_init_sequence5_64[]	= {0xFD, 0xFE};
static u8 cpt_init_sequence5_65[]	= {0xFE, 0x00};
static u8 cpt_init_sequence5_66[]	= {0xFF, 0x2E};
static u8 cpt_init_sequence6_1[]	= {0x83, 0xBB};
static u8 cpt_init_sequence6_2[]	= {0x84, 0x22};
static u8 cpt_init_sequence6_3[]	= {0xC0, 0x0E};
static u8 cpt_init_sequence6_4[]	= {0xC1, 0x12};
static u8 cpt_init_sequence6_5[]	= {0xC2, 0x25};
static u8 cpt_init_sequence6_6[]	= {0xC3, 0x34};
static u8 cpt_init_sequence6_7[]	= {0xC4, 0x3F};
static u8 cpt_init_sequence6_8[]	= {0xC5, 0x49};
static u8 cpt_init_sequence6_9[]	= {0xC6, 0x52};
static u8 cpt_init_sequence6_10[]	= {0xC7, 0x59};
static u8 cpt_init_sequence6_11[]	= {0xC8, 0x60};
static u8 cpt_init_sequence6_12[]	= {0xC9, 0xC8};
static u8 cpt_init_sequence6_13[]	= {0xCA, 0xC7};
static u8 cpt_init_sequence6_14[]	= {0xCB, 0xD6};
static u8 cpt_init_sequence6_15[]	= {0xCC, 0xD9};
static u8 cpt_init_sequence6_16[]	= {0xCD, 0xD7};
static u8 cpt_init_sequence6_17[]	= {0xCE, 0xD1};
static u8 cpt_init_sequence6_18[]	= {0xCF, 0xD3};
static u8 cpt_init_sequence6_19[]	= {0xD0, 0xD4};
static u8 cpt_init_sequence6_20[]	= {0xD1, 0xFF};
static u8 cpt_init_sequence6_21[]	= {0xD2, 0x08};
static u8 cpt_init_sequence6_22[]	= {0xD3, 0x2E};
static u8 cpt_init_sequence6_23[]	= {0xD4, 0x3B};
static u8 cpt_init_sequence6_24[]	= {0xD5, 0xAA};
static u8 cpt_init_sequence6_25[]	= {0xD6, 0xB3};
static u8 cpt_init_sequence6_26[]	= {0xD7, 0xBC};
static u8 cpt_init_sequence6_27[]	= {0xD8, 0xC5};
static u8 cpt_init_sequence6_28[]	= {0xD9, 0xD0};
static u8 cpt_init_sequence6_29[]	= {0xDA, 0xDB};
static u8 cpt_init_sequence6_30[]	= {0xDB, 0xE7};
static u8 cpt_init_sequence6_31[]	= {0xDC, 0xF7};
static u8 cpt_init_sequence6_32[]	= {0xDD, 0xFE};
static u8 cpt_init_sequence6_33[]	= {0xDE, 0x00};
static u8 cpt_init_sequence6_34[]	= {0xDF, 0x2E};
static u8 cpt_init_sequence6_35[]	= {0xE0, 0x0E};
static u8 cpt_init_sequence6_36[]	= {0xE1, 0x12};
static u8 cpt_init_sequence6_37[]	= {0xE2, 0x25};
static u8 cpt_init_sequence6_38[]	= {0xE3, 0x34};
static u8 cpt_init_sequence6_39[]	= {0xE4, 0x3F};
static u8 cpt_init_sequence6_40[]	= {0xE5, 0x49};
static u8 cpt_init_sequence6_41[]	= {0xE6, 0x52};
static u8 cpt_init_sequence6_42[]	= {0xE7, 0x59};
static u8 cpt_init_sequence6_43[]	= {0xE8, 0x60};
static u8 cpt_init_sequence6_44[]	= {0xE9, 0xC8};
static u8 cpt_init_sequence6_45[]	= {0xEA, 0xC7};
static u8 cpt_init_sequence6_46[]	= {0xEB, 0xD6};
static u8 cpt_init_sequence6_47[]	= {0xEC, 0xD9};
static u8 cpt_init_sequence6_48[]	= {0xED, 0xD7};
static u8 cpt_init_sequence6_49[]	= {0xEE, 0xD1};
static u8 cpt_init_sequence6_50[]	= {0xEF, 0xD3};
static u8 cpt_init_sequence6_51[]	= {0xF0, 0xD4};
static u8 cpt_init_sequence6_52[]	= {0xF1, 0xFF};
static u8 cpt_init_sequence6_53[]	= {0xF2, 0x08};
static u8 cpt_init_sequence6_54[]	= {0xF3, 0x2E};
static u8 cpt_init_sequence6_55[]	= {0xF4, 0x3B};
static u8 cpt_init_sequence6_56[]	= {0xF5, 0xAA};
static u8 cpt_init_sequence6_57[]	= {0xF6, 0xB3};
static u8 cpt_init_sequence6_58[]	= {0xF7, 0xBC};
static u8 cpt_init_sequence6_59[]	= {0xF8, 0xC5};
static u8 cpt_init_sequence6_60[]	= {0xF9, 0xD0};
static u8 cpt_init_sequence6_61[]	= {0xFA, 0xDB};
static u8 cpt_init_sequence6_62[]	= {0xFB, 0xE7};
static u8 cpt_init_sequence6_63[]	= {0xFC, 0xF7};
static u8 cpt_init_sequence6_64[]	= {0xFD, 0xFE};
static u8 cpt_init_sequence6_65[]	= {0xFE, 0x00};
static u8 cpt_init_sequence6_66[]	= {0xFF, 0x2E};

static u8 cpt_init_sequence7_1[]	= {0x83, 0xCC};
static u8 cpt_init_sequence7_2[]	= {0x84, 0x33};
static u8 cpt_init_sequence7_3[]	= {0xC0, 0x0E};
static u8 cpt_init_sequence7_4[]	= {0xC1, 0x12};
static u8 cpt_init_sequence7_5[]	= {0xC2, 0x25};
static u8 cpt_init_sequence7_6[]	= {0xC3, 0x34};
static u8 cpt_init_sequence7_7[]	= {0xC4, 0x3F};
static u8 cpt_init_sequence7_8[]	= {0xC5, 0x49};
static u8 cpt_init_sequence7_9[]	= {0xC6, 0x52};
static u8 cpt_init_sequence7_10[]	= {0xC7, 0x59};
static u8 cpt_init_sequence7_11[]	= {0xC8, 0x60};
static u8 cpt_init_sequence7_12[]	= {0xC9, 0xC8};
static u8 cpt_init_sequence7_13[]	= {0xCA, 0xC7};
static u8 cpt_init_sequence7_14[]	= {0xCB, 0xD6};
static u8 cpt_init_sequence7_15[]	= {0xCC, 0xD9};
static u8 cpt_init_sequence7_16[]	= {0xCD, 0xD7};
static u8 cpt_init_sequence7_17[]	= {0xCE, 0xD1};
static u8 cpt_init_sequence7_18[]	= {0xCF, 0xD3};
static u8 cpt_init_sequence7_19[]	= {0xD0, 0xD4};
static u8 cpt_init_sequence7_20[]	= {0xD1, 0xFF};
static u8 cpt_init_sequence7_21[]	= {0xD2, 0x08};
static u8 cpt_init_sequence7_22[]	= {0xD3, 0x2E};
static u8 cpt_init_sequence7_23[]	= {0xD4, 0x3B};
static u8 cpt_init_sequence7_24[]	= {0xD5, 0xAA};
static u8 cpt_init_sequence7_25[]	= {0xD6, 0xB3};
static u8 cpt_init_sequence7_26[]	= {0xD7, 0xBC};
static u8 cpt_init_sequence7_27[]	= {0xD8, 0xC5};
static u8 cpt_init_sequence7_28[]	= {0xD9, 0xD0};
static u8 cpt_init_sequence7_29[]	= {0xDA, 0xDB};
static u8 cpt_init_sequence7_30[]	= {0xDB, 0xE7};
static u8 cpt_init_sequence7_31[]	= {0xDC, 0xF7};
static u8 cpt_init_sequence7_32[]	= {0xDD, 0xFE};
static u8 cpt_init_sequence7_33[]	= {0xDE, 0x00};
static u8 cpt_init_sequence7_34[]	= {0xDF, 0x2E};
static u8 cpt_init_sequence7_35[]	= {0xE0, 0x0E};
static u8 cpt_init_sequence7_36[]	= {0xE1, 0x12};
static u8 cpt_init_sequence7_37[]	= {0xE2, 0x25};
static u8 cpt_init_sequence7_38[]	= {0xE3, 0x34};
static u8 cpt_init_sequence7_39[]	= {0xE4, 0x3F};
static u8 cpt_init_sequence7_40[]	= {0xE5, 0x49};
static u8 cpt_init_sequence7_41[]	= {0xE6, 0x52};
static u8 cpt_init_sequence7_42[]	= {0xE7, 0x59};
static u8 cpt_init_sequence7_43[]	= {0xE8, 0x60};
static u8 cpt_init_sequence7_44[]	= {0xE9, 0xC8};
static u8 cpt_init_sequence7_45[]	= {0xEA, 0xC7};
static u8 cpt_init_sequence7_46[]	= {0xEB, 0xD6};
static u8 cpt_init_sequence7_47[]	= {0xEC, 0xD9};
static u8 cpt_init_sequence7_48[]	= {0xED, 0xD7};
static u8 cpt_init_sequence7_49[]	= {0xEE, 0xD1};
static u8 cpt_init_sequence7_50[]	= {0xEF, 0xD3};
static u8 cpt_init_sequence7_51[]	= {0xF0, 0xD4};
static u8 cpt_init_sequence7_52[]	= {0xF1, 0xFF};
static u8 cpt_init_sequence7_53[]	= {0xF2, 0x08};
static u8 cpt_init_sequence7_54[]	= {0xF3, 0x2E};
static u8 cpt_init_sequence7_55[]	= {0xF4, 0x3B};
static u8 cpt_init_sequence7_56[]	= {0xF5, 0xAA};
static u8 cpt_init_sequence7_57[]	= {0xF6, 0xB3};
static u8 cpt_init_sequence7_58[]	= {0xF7, 0xBC};
static u8 cpt_init_sequence7_59[]	= {0xF8, 0xC5};
static u8 cpt_init_sequence7_60[]	= {0xF9, 0xD0};
static u8 cpt_init_sequence7_61[]	= {0xFA, 0xDB};
static u8 cpt_init_sequence7_62[]	= {0xFB, 0xE7};
static u8 cpt_init_sequence7_63[]	= {0xFC, 0xF7};
static u8 cpt_init_sequence7_64[]	= {0xFD, 0xFE};
static u8 cpt_init_sequence7_65[]	= {0xFE, 0x00};
static u8 cpt_init_sequence7_66[]	= {0xFF, 0x2E};

void claa080fp01_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");

	msleep(20);

	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence3, sizeof(cpt_init_sequence3));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence4, sizeof(cpt_init_sequence4));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_1, sizeof(cpt_init_sequence5_1));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_2, sizeof(cpt_init_sequence5_2));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_3, sizeof(cpt_init_sequence5_3));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_4, sizeof(cpt_init_sequence5_4));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_5, sizeof(cpt_init_sequence5_5));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_6, sizeof(cpt_init_sequence5_6));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_7, sizeof(cpt_init_sequence5_7));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_8, sizeof(cpt_init_sequence5_8));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_9, sizeof(cpt_init_sequence5_9));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_10, sizeof(cpt_init_sequence5_10));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_11, sizeof(cpt_init_sequence5_11));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_12, sizeof(cpt_init_sequence5_12));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_13, sizeof(cpt_init_sequence5_13));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_14, sizeof(cpt_init_sequence5_14));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_15, sizeof(cpt_init_sequence5_15));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_16, sizeof(cpt_init_sequence5_16));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_17, sizeof(cpt_init_sequence5_17));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_18, sizeof(cpt_init_sequence5_18));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_19, sizeof(cpt_init_sequence5_19));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_20, sizeof(cpt_init_sequence5_20));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_21, sizeof(cpt_init_sequence5_21));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_22, sizeof(cpt_init_sequence5_22));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_23, sizeof(cpt_init_sequence5_23));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_24, sizeof(cpt_init_sequence5_24));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_25, sizeof(cpt_init_sequence5_25));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_26, sizeof(cpt_init_sequence5_26));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_27, sizeof(cpt_init_sequence5_27));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_28, sizeof(cpt_init_sequence5_28));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_29, sizeof(cpt_init_sequence5_29));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_30, sizeof(cpt_init_sequence5_30));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_31, sizeof(cpt_init_sequence5_31));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_32, sizeof(cpt_init_sequence5_32));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_33, sizeof(cpt_init_sequence5_33));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_34, sizeof(cpt_init_sequence5_34));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_35, sizeof(cpt_init_sequence5_35));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_36, sizeof(cpt_init_sequence5_36));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_37, sizeof(cpt_init_sequence5_37));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_38, sizeof(cpt_init_sequence5_38));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_39, sizeof(cpt_init_sequence5_39));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_40, sizeof(cpt_init_sequence5_40));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_41, sizeof(cpt_init_sequence5_41));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_42, sizeof(cpt_init_sequence5_42));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_43, sizeof(cpt_init_sequence5_43));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_44, sizeof(cpt_init_sequence5_44));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_45, sizeof(cpt_init_sequence5_45));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_46, sizeof(cpt_init_sequence5_46));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_47, sizeof(cpt_init_sequence5_47));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_48, sizeof(cpt_init_sequence5_48));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_49, sizeof(cpt_init_sequence5_49));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_50, sizeof(cpt_init_sequence5_50));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_51, sizeof(cpt_init_sequence5_51));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_52, sizeof(cpt_init_sequence5_52));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_53, sizeof(cpt_init_sequence5_53));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_54, sizeof(cpt_init_sequence5_54));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_55, sizeof(cpt_init_sequence5_55));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_56, sizeof(cpt_init_sequence5_56));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_57, sizeof(cpt_init_sequence5_57));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_58, sizeof(cpt_init_sequence5_58));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_59, sizeof(cpt_init_sequence5_59));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_60, sizeof(cpt_init_sequence5_60));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_61, sizeof(cpt_init_sequence5_61));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_62, sizeof(cpt_init_sequence5_62));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_63, sizeof(cpt_init_sequence5_63));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_64, sizeof(cpt_init_sequence5_64));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_65, sizeof(cpt_init_sequence5_65));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5_66, sizeof(cpt_init_sequence5_66));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_1, sizeof(cpt_init_sequence6_1));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_2, sizeof(cpt_init_sequence6_2));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_3, sizeof(cpt_init_sequence6_3));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_4, sizeof(cpt_init_sequence6_4));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_5, sizeof(cpt_init_sequence6_5));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_6, sizeof(cpt_init_sequence6_6));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_7, sizeof(cpt_init_sequence6_7));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_8, sizeof(cpt_init_sequence6_8));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_9, sizeof(cpt_init_sequence6_9));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_10, sizeof(cpt_init_sequence6_10));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_11, sizeof(cpt_init_sequence6_11));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_12, sizeof(cpt_init_sequence6_12));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_13, sizeof(cpt_init_sequence6_13));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_14, sizeof(cpt_init_sequence6_14));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_15, sizeof(cpt_init_sequence6_15));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_16, sizeof(cpt_init_sequence6_16));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_17, sizeof(cpt_init_sequence6_17));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_18, sizeof(cpt_init_sequence6_18));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_19, sizeof(cpt_init_sequence6_19));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_20, sizeof(cpt_init_sequence6_20));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_21, sizeof(cpt_init_sequence6_21));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_22, sizeof(cpt_init_sequence6_22));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_23, sizeof(cpt_init_sequence6_23));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_24, sizeof(cpt_init_sequence6_24));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_25, sizeof(cpt_init_sequence6_25));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_26, sizeof(cpt_init_sequence6_26));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_27, sizeof(cpt_init_sequence6_27));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_28, sizeof(cpt_init_sequence6_28));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_29, sizeof(cpt_init_sequence6_29));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_30, sizeof(cpt_init_sequence6_30));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_31, sizeof(cpt_init_sequence6_31));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_32, sizeof(cpt_init_sequence6_32));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_33, sizeof(cpt_init_sequence6_33));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_34, sizeof(cpt_init_sequence6_34));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_35, sizeof(cpt_init_sequence6_35));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_36, sizeof(cpt_init_sequence6_36));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_37, sizeof(cpt_init_sequence6_37));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_38, sizeof(cpt_init_sequence6_38));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_39, sizeof(cpt_init_sequence6_39));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_40, sizeof(cpt_init_sequence6_40));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_41, sizeof(cpt_init_sequence6_41));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_42, sizeof(cpt_init_sequence6_42));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_43, sizeof(cpt_init_sequence6_43));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_44, sizeof(cpt_init_sequence6_44));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_45, sizeof(cpt_init_sequence6_45));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_46, sizeof(cpt_init_sequence6_46));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_47, sizeof(cpt_init_sequence6_47));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_48, sizeof(cpt_init_sequence6_48));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_49, sizeof(cpt_init_sequence6_49));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_50, sizeof(cpt_init_sequence6_50));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_51, sizeof(cpt_init_sequence6_51));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_52, sizeof(cpt_init_sequence6_52));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_53, sizeof(cpt_init_sequence6_53));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_54, sizeof(cpt_init_sequence6_54));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_55, sizeof(cpt_init_sequence6_55));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_56, sizeof(cpt_init_sequence6_56));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_57, sizeof(cpt_init_sequence6_57));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_58, sizeof(cpt_init_sequence6_58));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_59, sizeof(cpt_init_sequence6_59));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_60, sizeof(cpt_init_sequence6_60));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_61, sizeof(cpt_init_sequence6_61));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_62, sizeof(cpt_init_sequence6_62));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_63, sizeof(cpt_init_sequence6_63));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_64, sizeof(cpt_init_sequence6_64));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_65, sizeof(cpt_init_sequence6_65));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6_66, sizeof(cpt_init_sequence6_66));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_1, sizeof(cpt_init_sequence7_1));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_2, sizeof(cpt_init_sequence7_2));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_3, sizeof(cpt_init_sequence7_3));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_4, sizeof(cpt_init_sequence7_4));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_5, sizeof(cpt_init_sequence7_5));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_6, sizeof(cpt_init_sequence7_6));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_7, sizeof(cpt_init_sequence7_7));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_8, sizeof(cpt_init_sequence7_8));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_9, sizeof(cpt_init_sequence7_9));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_10, sizeof(cpt_init_sequence7_10));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_11, sizeof(cpt_init_sequence7_11));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_12, sizeof(cpt_init_sequence7_12));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_13, sizeof(cpt_init_sequence7_13));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_14, sizeof(cpt_init_sequence7_14));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_15, sizeof(cpt_init_sequence7_15));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_16, sizeof(cpt_init_sequence7_16));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_17, sizeof(cpt_init_sequence7_17));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_18, sizeof(cpt_init_sequence7_18));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_19, sizeof(cpt_init_sequence7_19));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_20, sizeof(cpt_init_sequence7_20));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_21, sizeof(cpt_init_sequence7_21));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_22, sizeof(cpt_init_sequence7_22));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_23, sizeof(cpt_init_sequence7_23));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_24, sizeof(cpt_init_sequence7_24));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_25, sizeof(cpt_init_sequence7_25));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_26, sizeof(cpt_init_sequence7_26));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_27, sizeof(cpt_init_sequence7_27));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_28, sizeof(cpt_init_sequence7_28));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_29, sizeof(cpt_init_sequence7_29));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_30, sizeof(cpt_init_sequence7_30));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_31, sizeof(cpt_init_sequence7_31));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_32, sizeof(cpt_init_sequence7_32));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_33, sizeof(cpt_init_sequence7_33));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_34, sizeof(cpt_init_sequence7_34));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_35, sizeof(cpt_init_sequence7_35));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_36, sizeof(cpt_init_sequence7_36));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_37, sizeof(cpt_init_sequence7_37));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_38, sizeof(cpt_init_sequence7_38));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_39, sizeof(cpt_init_sequence7_39));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_40, sizeof(cpt_init_sequence7_40));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_41, sizeof(cpt_init_sequence7_41));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_42, sizeof(cpt_init_sequence7_42));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_43, sizeof(cpt_init_sequence7_43));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_44, sizeof(cpt_init_sequence7_44));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_45, sizeof(cpt_init_sequence7_45));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_46, sizeof(cpt_init_sequence7_46));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_47, sizeof(cpt_init_sequence7_47));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_48, sizeof(cpt_init_sequence7_48));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_49, sizeof(cpt_init_sequence7_49));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_50, sizeof(cpt_init_sequence7_50));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_51, sizeof(cpt_init_sequence7_51));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_52, sizeof(cpt_init_sequence7_52));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_53, sizeof(cpt_init_sequence7_53));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_54, sizeof(cpt_init_sequence7_54));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_55, sizeof(cpt_init_sequence7_55));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_56, sizeof(cpt_init_sequence7_56));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_57, sizeof(cpt_init_sequence7_57));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_58, sizeof(cpt_init_sequence7_58));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_59, sizeof(cpt_init_sequence7_59));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_60, sizeof(cpt_init_sequence7_60));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_61, sizeof(cpt_init_sequence7_61));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_62, sizeof(cpt_init_sequence7_62));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_63, sizeof(cpt_init_sequence7_63));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_64, sizeof(cpt_init_sequence7_64));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_65, sizeof(cpt_init_sequence7_65));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7_66, sizeof(cpt_init_sequence7_66));
}

static void  claa080fp01_get_panel_info(int pipe,
					struct drm_connector *connector)
{
	DRM_DEBUG_KMS("\n");
	if (!connector) {
		DRM_DEBUG_KMS("Cpt: Invalid input to get_info\n");
		return;
	}

	if (pipe == 0) {
		connector->display_info.width_mm = 108;
		connector->display_info.height_mm = 172;
	}

	return;
}

static void claa080fp01_destroy(struct intel_dsi_device *dsi)
{
}

static void claa080fp01_dump_regs(struct intel_dsi_device *dsi)
{
}

static void claa080fp01_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *claa080fp01_get_modes(
	struct intel_dsi_device *dsi)
{
	struct drm_display_mode *mode = NULL;
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	/* Allocate */
	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode) {
		DRM_DEBUG_KMS("Cpt panel: No memory\n");
		return NULL;
	}

	/* Hardcode 1200*1920 */
	/*HFP = 42, HSYNC = 1, HBP = 32, HP = 1995*/
	/*VFP = 35, VSYNC = 1, VBP = 25, VP = 1261*/
	mode->hdisplay = 1200;
	mode->hsync_start = mode->hdisplay + 42;
	mode->hsync_end = mode->hsync_start + 1;
	mode->htotal = mode->hsync_end  + 32;

	mode->vdisplay = 1920;
	mode->vsync_start = mode->vdisplay + 35;
	mode->vsync_end = mode->vsync_start + 1;
	mode->vtotal = mode->vsync_end + 25;

	mode->vrefresh = 60;
	mode->clock =  mode->vrefresh * mode->vtotal *
	mode->htotal / 1000;
	intel_dsi->pclk = mode->clock;
	/* Configure */
	drm_mode_set_name(mode);
	drm_mode_set_crtcinfo(mode, 0);
	mode->type |= DRM_MODE_TYPE_PREFERRED;

	return mode;
}

static bool claa080fp01_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	return true;
}

static enum drm_connector_status claa080fp01_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	dev_priv->is_mipi = true;
	DRM_DEBUG_KMS("\n");
	return connector_status_connected;
}

static bool claa080fp01_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}
#define DISP_RST_N 107
void claa080fp01_reset(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	DRM_DEBUG_KMS("\n");

	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PAD, 0x00000004);

	/* panel disable */
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PAD, 0x00000004);
	usleep_range(100000, 120000);

	/* panel enable */
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PAD, 0x00000005);
	usleep_range(100000, 120000);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PAD, 0x00000005);
}

static int claa080fp01_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("\n");
	return MODE_OK;
}

static void claa080fp01_dpms(struct intel_dsi_device *dsi, bool enable)
{
/*	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev); */

	DRM_DEBUG_KMS("\n");
}
static void claa080fp01_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");

	msleep(100);  /* wait for 100ms */

	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(200);  /* wait for 150ms */
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	msleep(200);  /* wait for 200ms */

}
static void claa080fp01_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");

	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(200);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(100);

}

bool claa080fp01_init(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
/*	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
*/
	/* create private data, slam to dsi->dev_priv. could support many panels
	 * based on dsi->name. This panal supports both command and video mode,
	 * so check the type. */

	/* where to get all the board info style stuff:
	 *
	 * - gpio numbers, if any (external te, reset)
	 * - pin config, mipi lanes
	 * - dsi backlight? (->create another bl device if needed)
	 * - esd interval, ulps timeout
	 *
	 */

	DRM_DEBUG_KMS("Init: claa080fp01 panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to claa080fp01\n");
		return false;
	}

	intel_dsi->eotp_pkt = 1;
	intel_dsi->operation_mode = DSI_VIDEO_MODE;
	intel_dsi->video_mode_type = DSI_VIDEO_NBURST_SEVENT;
	intel_dsi->pixel_format = VID_MODE_FORMAT_RGB888;
	intel_dsi->port_bits = 0;
	intel_dsi->turn_arnd_val = 0x14;
	intel_dsi->rst_timer_val = 0xffff;
	intel_dsi->bw_timer = 0x820;
	/*b044*/
	intel_dsi->hs_to_lp_count = 0x2E;
	/*b060*/
	intel_dsi->lp_byte_clk = 0x06;
	/*b080*/
	intel_dsi->dphy_reg = 0x331C7217;
	/* b088 high 16bits */
	intel_dsi->clk_lp_to_hs_count = 0x3d;
	/* b088 low 16bits */
	intel_dsi->clk_hs_to_lp_count = 0x17;
	/* BTA sending at the last blanking line of VFP is disabled */
	intel_dsi->video_frmt_cfg_bits = 1<<3;
	intel_dsi->lane_count = 4;

	intel_dsi->port = 0; /* PORT_A by default */
	intel_dsi->burst_mode_ratio = 100;

	intel_dsi->backlight_off_delay = 20;
	intel_dsi->send_shutdown = true;
	intel_dsi->shutdown_pkt_delay = 20;
	/* dev_priv->mipi.panel_bpp = 24; */

	return true;
}

/* Callbacks. We might not need them all. */
struct intel_dsi_dev_ops cpt_claa080fp01_dsi_display_ops = {
	.init = claa080fp01_init,
	.get_info = claa080fp01_get_panel_info,
	.create_resources = claa080fp01_create_resources,
	.dpms = claa080fp01_dpms,
	.mode_valid = claa080fp01_mode_valid,
	.mode_fixup = claa080fp01_mode_fixup,
	.panel_reset = claa080fp01_reset,
	.detect = claa080fp01_detect,
	.get_hw_state = claa080fp01_get_hw_state,
	.get_modes = claa080fp01_get_modes,
	.destroy = claa080fp01_destroy,
	.dump_regs = claa080fp01_dump_regs,
	.enable = claa080fp01_enable,
	.disable = claa080fp01_disable,
	.send_otp_cmds = claa080fp01_send_otp_cmds,
};
