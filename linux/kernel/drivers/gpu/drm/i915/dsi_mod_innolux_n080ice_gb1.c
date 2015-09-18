/*
 * Copyright Â© 2013 Intel Corporation
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
#include "dsi_mod_innolux_n080ice_gb1.h"

static u8 innolux_init_sequence_1[] = {0xFF, 0xAA, 0x55, 0xA5, 0x80};
static u8 innolux_init_sequence_2[] = {0x6F, 0x11, 0x00};
static u8 innolux_init_sequence_3[] = {0xF7, 0x20, 0x00};
static u8 innolux_init_sequence_4[] = {0x6F, 0x06};
static u8 innolux_init_sequence_5[] = {0xF7, 0xA0};
static u8 innolux_init_sequence_6[] = {0x6F, 0x19};
static u8 innolux_init_sequence_7[] = {0xF7, 0x12};
static u8 innolux_init_sequence_8[] = {0x6F, 0x08};
static u8 innolux_init_sequence_9[] = {0xFA, 0x40};
static u8 innolux_init_sequence_10[] = {0x6F, 0x11};
static u8 innolux_init_sequence_11[] = {0xF3, 0x01};
static u8 innolux_init_sequence_12[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00};
static u8 innolux_init_sequence_13[] = {0xC8, 0x80};
static u8 innolux_init_sequence_14[] = {0xB1, 0x6C, 0x07};
static u8 innolux_init_sequence_15[] = {0xB6, 0x08};
static u8 innolux_init_sequence_16[] = {0x6F, 0x02};
static u8 innolux_init_sequence_17[] = {0xB8, 0x08};
static u8 innolux_init_sequence_18[] = {0xBB, 0x74, 0x44};
static u8 innolux_init_sequence_19[] = {0xBC, 0x00, 0x00};
static u8 innolux_init_sequence_20[] = {0xBD, 0x02, 0xB0, 0x0C, 0x0A, 0x00};
static u8 innolux_init_sequence_21[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01};
static u8 innolux_init_sequence_22[] = {0xB0, 0x05, 0x05};
static u8 innolux_init_sequence_23[] = {0xB1, 0x05, 0x05};
static u8 innolux_init_sequence_24[] = {0xBC, 0x90, 0x01};
static u8 innolux_init_sequence_25[] = {0xBD, 0x90, 0x01};
static u8 innolux_init_sequence_26[] = {0xCA, 0x00};
static u8 innolux_init_sequence_27[] = {0xC0, 0x04};
static u8 innolux_init_sequence_28[] = {0xBE, 0x29};
static u8 innolux_init_sequence_29[] = {0xB3, 0x37, 0x37};
static u8 innolux_init_sequence_30[] = {0xB4, 0x19, 0x19};
static u8 innolux_init_sequence_31[] = {0xB9, 0x44, 0x44};
static u8 innolux_init_sequence_32[] = {0xBA, 0x24, 0x24};
static u8 innolux_init_sequence_33[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x02};
static u8 innolux_init_sequence_34[] = {0xEE, 0x01};
static u8 innolux_init_sequence_35[] = {0xEF, 0x09, 0x06, 0x15, 0x18};
static u8 innolux_init_sequence_36[] = {0xB0, 0x00, 0x00, 0x00, 0x25, 0x00, 0x43};
static u8 innolux_init_sequence_37[] = {0x6F, 0x06};
static u8 innolux_init_sequence_38[] = {0xB0, 0x00, 0x54, 0x00, 0x68, 0x00, 0xA0};
static u8 innolux_init_sequence_39[] = {0x6F, 0x0C};
static u8 innolux_init_sequence_40[] = {0xB0, 0x00, 0xC0, 0x01, 0x00};
static u8 innolux_init_sequence_41[] = {0xB1, 0x01, 0x30, 0x01, 0x78, 0x01, 0xAE};
static u8 innolux_init_sequence_42[] = {0x6F, 0x06};
static u8 innolux_init_sequence_43[] = {0xB1, 0x02, 0x08, 0x02, 0x52, 0x02, 0x54};
static u8 innolux_init_sequence_44[] = {0x6F, 0x0C};
static u8 innolux_init_sequence_45[] = {0xB1, 0x02, 0x99, 0x02, 0xF0};
static u8 innolux_init_sequence_46[] = {0xB2, 0x03, 0x20, 0x03, 0x56, 0x03, 0x76};
static u8 innolux_init_sequence_47[] = {0x6F, 0x06};
static u8 innolux_init_sequence_48[] = {0xB2, 0x03, 0x93, 0x03, 0xA4, 0x03, 0xB9};
static u8 innolux_init_sequence_49[] = {0x6F, 0x0C};
static u8 innolux_init_sequence_50[] = {0xB2, 0x03, 0xC9, 0x03, 0xE3};
static u8 innolux_init_sequence_51[] = {0xB3, 0x03, 0xFC, 0x03, 0xFF};
static u8 innolux_init_sequence_52[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x06};
static u8 innolux_init_sequence_53[] = {0xB0, 0x00, 0x10};
static u8 innolux_init_sequence_54[] = {0xB1, 0x12, 0x14};
static u8 innolux_init_sequence_55[] = {0xB2, 0x16, 0x18};
static u8 innolux_init_sequence_56[] = {0xB3, 0x1A, 0x29};
static u8 innolux_init_sequence_57[] = {0xB4, 0x2A, 0x08};
static u8 innolux_init_sequence_58[] = {0xB5, 0x31, 0x31};
static u8 innolux_init_sequence_59[] = {0xB6, 0x31, 0x31};
static u8 innolux_init_sequence_60[] = {0xB7, 0x31, 0x31};
static u8 innolux_init_sequence_61[] = {0xB8, 0x31, 0x0A};
static u8 innolux_init_sequence_62[] = {0xB9, 0x31, 0x31};
static u8 innolux_init_sequence_63[] = {0xBA, 0x31, 0x31};
static u8 innolux_init_sequence_64[] = {0xBB, 0x0B, 0x31};
static u8 innolux_init_sequence_65[] = {0xBC, 0x31, 0x31};
static u8 innolux_init_sequence_66[] = {0xBD, 0x31, 0x31};
static u8 innolux_init_sequence_67[] = {0xBE, 0x31, 0x31};
static u8 innolux_init_sequence_68[] = {0xBF, 0x09, 0x2A};
static u8 innolux_init_sequence_69[] = {0xC0, 0x29, 0x1B};
static u8 innolux_init_sequence_70[] = {0xC1, 0x19, 0x17};
static u8 innolux_init_sequence_71[] = {0xC2, 0x15, 0x13};
static u8 innolux_init_sequence_72[] = {0xC3, 0x11, 0x01};
static u8 innolux_init_sequence_73[] = {0xE5, 0x31, 0x31};
static u8 innolux_init_sequence_74[] = {0xC4, 0x09, 0x1B};
static u8 innolux_init_sequence_75[] = {0xC5, 0x19, 0x17};
static u8 innolux_init_sequence_76[] = {0xC6, 0x15, 0x13};
static u8 innolux_init_sequence_77[] = {0xC7, 0x11, 0x29};
static u8 innolux_init_sequence_78[] = {0xC8, 0x2A, 0x01};
static u8 innolux_init_sequence_79[] = {0xC9, 0x31, 0x31};
static u8 innolux_init_sequence_80[] = {0xCA, 0x31, 0x31};
static u8 innolux_init_sequence_81[] = {0xCB, 0x31, 0x31};
static u8 innolux_init_sequence_82[] = {0xCC, 0x31, 0x0B};
static u8 innolux_init_sequence_83[] = {0xCD, 0x31, 0x31};
static u8 innolux_init_sequence_84[] = {0xCE, 0x31, 0x31};
static u8 innolux_init_sequence_85[] = {0xCF, 0x0A, 0x31};
static u8 innolux_init_sequence_86[] = {0xD0, 0x31, 0x31};
static u8 innolux_init_sequence_87[] = {0xD1, 0x31, 0x31};
static u8 innolux_init_sequence_88[] = {0xD2, 0x31, 0x31};
static u8 innolux_init_sequence_89[] = {0xD3, 0x00, 0x2A};
static u8 innolux_init_sequence_90[] = {0xD4, 0x29, 0x10};
static u8 innolux_init_sequence_91[] = {0xD5, 0x12, 0x14};
static u8 innolux_init_sequence_92[] = {0xD6, 0x16, 0x18};
static u8 innolux_init_sequence_93[] = {0xD7, 0x1A, 0x08};
static u8 innolux_init_sequence_94[] = {0xE6, 0x31, 0x31};
static u8 innolux_init_sequence_95[] = {0xD8, 0x00, 0x00, 0x00, 0x54, 0x00};
static u8 innolux_init_sequence_96[] = {0xD9, 0x00, 0x15, 0x00, 0x00, 0x00};
static u8 innolux_init_sequence_97[] = {0xE7, 0x00};
static u8 innolux_init_sequence_98[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x03};
static u8 innolux_init_sequence_99[] = {0xB0, 0x20, 0x00};
static u8 innolux_init_sequence_100[] = {0xB1, 0x20, 0x00};
static u8 innolux_init_sequence_101[] = {0xB2, 0x05, 0x00, 0x00, 0x00, 0x00};
static u8 innolux_init_sequence_102[] = {0xB6, 0x05, 0x00, 0x00, 0x00, 0x00};
static u8 innolux_init_sequence_103[] = {0xB7, 0x05, 0x00, 0x00, 0x00, 0x00};
static u8 innolux_init_sequence_104[] = {0xBA, 0x57, 0x00, 0x00, 0x00, 0x00};
static u8 innolux_init_sequence_105[] = {0xBB, 0x57, 0x00, 0x00, 0x00, 0x00};
static u8 innolux_init_sequence_106[] = {0xC0, 0x00, 0x00, 0x00, 0x00};
static u8 innolux_init_sequence_107[] = {0xC1, 0x00, 0x00, 0x00, 0x00};
static u8 innolux_init_sequence_108[] = {0xC4, 0x60};
static u8 innolux_init_sequence_109[] = {0xC5, 0x40};
static u8 innolux_init_sequence_110[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x05};
static u8 innolux_init_sequence_111[] = {0xBD, 0x03, 0x01, 0x03, 0x03, 0x03};
static u8 innolux_init_sequence_112[] = {0xB0, 0x17, 0x06};
static u8 innolux_init_sequence_113[] = {0xB1, 0x17, 0x06};
static u8 innolux_init_sequence_114[] = {0xB2, 0x17, 0x06};
static u8 innolux_init_sequence_115[] = {0xB3, 0x17, 0x06};
static u8 innolux_init_sequence_116[] = {0xB4, 0x17, 0x06};
static u8 innolux_init_sequence_117[] = {0xB5, 0x17, 0x06};
static u8 innolux_init_sequence_118[] = {0xB8, 0x00};
static u8 innolux_init_sequence_119[] = {0xB9, 0x00};
static u8 innolux_init_sequence_120[] = {0xBA, 0x00};
static u8 innolux_init_sequence_121[] = {0xBB, 0x02};
static u8 innolux_init_sequence_122[] = {0xBC, 0x00};
static u8 innolux_init_sequence_123[] = {0xC0, 0x07};
static u8 innolux_init_sequence_124[] = {0xC4, 0x80};
static u8 innolux_init_sequence_125[] = {0xC5, 0xA4};
static u8 innolux_init_sequence_126[] = {0xC8, 0x05, 0x30};
static u8 innolux_init_sequence_127[] = {0xC9, 0x01, 0x31};
static u8 innolux_init_sequence_128[] = {0xCC, 0x00, 0x00, 0x3C};
static u8 innolux_init_sequence_129[] = {0xCD, 0x00, 0x00, 0x3C};
static u8 innolux_init_sequence_130[] = {0xD1, 0x00, 0x04, 0xFD, 0x07, 0x10};
static u8 innolux_init_sequence_131[] = {0xD2, 0x00, 0x05, 0x02, 0x07, 0x10};
static u8 innolux_init_sequence_132[] = {0xE5, 0x06};
static u8 innolux_init_sequence_133[] = {0xE6, 0x06};
static u8 innolux_init_sequence_134[] = {0xE7, 0x06};
static u8 innolux_init_sequence_135[] = {0xE8, 0x06};
static u8 innolux_init_sequence_136[] = {0xE9, 0x06};
static u8 innolux_init_sequence_137[] = {0xEA, 0x06};
static u8 innolux_init_sequence_138[] = {0xED, 0x30};
static u8 innolux_init_sequence_139[] = {0x6F, 0x11};
static u8 innolux_init_sequence_140[] = {0xF3, 0x01};

void n080ice_gb1_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_1, sizeof(innolux_init_sequence_1));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_2, sizeof(innolux_init_sequence_2));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_3, sizeof(innolux_init_sequence_3));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_4, sizeof(innolux_init_sequence_4));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_5, sizeof(innolux_init_sequence_5));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_6, sizeof(innolux_init_sequence_6));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_7, sizeof(innolux_init_sequence_7));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_8, sizeof(innolux_init_sequence_8));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_9, sizeof(innolux_init_sequence_9));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_10, sizeof(innolux_init_sequence_10));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_11, sizeof(innolux_init_sequence_11));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_12, sizeof(innolux_init_sequence_12));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_13, sizeof(innolux_init_sequence_13));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_14, sizeof(innolux_init_sequence_14));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_15, sizeof(innolux_init_sequence_15));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_16, sizeof(innolux_init_sequence_16));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_17, sizeof(innolux_init_sequence_17));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_18, sizeof(innolux_init_sequence_18));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_19, sizeof(innolux_init_sequence_19));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_20, sizeof(innolux_init_sequence_20));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_21, sizeof(innolux_init_sequence_21));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_22, sizeof(innolux_init_sequence_22));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_23, sizeof(innolux_init_sequence_23));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_24, sizeof(innolux_init_sequence_24));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_25, sizeof(innolux_init_sequence_25));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_26, sizeof(innolux_init_sequence_26));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_27, sizeof(innolux_init_sequence_27));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_28, sizeof(innolux_init_sequence_28));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_29, sizeof(innolux_init_sequence_29));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_30, sizeof(innolux_init_sequence_30));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_31, sizeof(innolux_init_sequence_31));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_32, sizeof(innolux_init_sequence_32));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_33, sizeof(innolux_init_sequence_33));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_34, sizeof(innolux_init_sequence_34));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_35, sizeof(innolux_init_sequence_35));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_36, sizeof(innolux_init_sequence_36));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_37, sizeof(innolux_init_sequence_37));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_38, sizeof(innolux_init_sequence_38));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_39, sizeof(innolux_init_sequence_39));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_40, sizeof(innolux_init_sequence_40));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_41, sizeof(innolux_init_sequence_41));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_42, sizeof(innolux_init_sequence_42));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_43, sizeof(innolux_init_sequence_43));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_44, sizeof(innolux_init_sequence_44));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_45, sizeof(innolux_init_sequence_45));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_46, sizeof(innolux_init_sequence_46));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_47, sizeof(innolux_init_sequence_47));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_48, sizeof(innolux_init_sequence_48));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_49, sizeof(innolux_init_sequence_49));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_50, sizeof(innolux_init_sequence_50));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_51, sizeof(innolux_init_sequence_51));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_52, sizeof(innolux_init_sequence_52));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_53, sizeof(innolux_init_sequence_53));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_54, sizeof(innolux_init_sequence_54));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_55, sizeof(innolux_init_sequence_55));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_56, sizeof(innolux_init_sequence_56));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_57, sizeof(innolux_init_sequence_57));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_58, sizeof(innolux_init_sequence_58));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_59, sizeof(innolux_init_sequence_59));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_60, sizeof(innolux_init_sequence_60));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_61, sizeof(innolux_init_sequence_61));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_62, sizeof(innolux_init_sequence_62));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_63, sizeof(innolux_init_sequence_63));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_64, sizeof(innolux_init_sequence_64));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_65, sizeof(innolux_init_sequence_65));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_66, sizeof(innolux_init_sequence_66));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_67, sizeof(innolux_init_sequence_67));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_68, sizeof(innolux_init_sequence_68));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_69, sizeof(innolux_init_sequence_69));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_70, sizeof(innolux_init_sequence_70));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_71, sizeof(innolux_init_sequence_71));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_72, sizeof(innolux_init_sequence_72));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_73, sizeof(innolux_init_sequence_73));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_74, sizeof(innolux_init_sequence_74));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_75, sizeof(innolux_init_sequence_75));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_76, sizeof(innolux_init_sequence_76));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_77, sizeof(innolux_init_sequence_77));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_78, sizeof(innolux_init_sequence_78));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_79, sizeof(innolux_init_sequence_79));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_80, sizeof(innolux_init_sequence_80));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_81, sizeof(innolux_init_sequence_81));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_82, sizeof(innolux_init_sequence_82));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_83, sizeof(innolux_init_sequence_83));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_84, sizeof(innolux_init_sequence_84));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_85, sizeof(innolux_init_sequence_85));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_86, sizeof(innolux_init_sequence_86));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_87, sizeof(innolux_init_sequence_87));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_88, sizeof(innolux_init_sequence_88));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_89, sizeof(innolux_init_sequence_89));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_90, sizeof(innolux_init_sequence_90));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_91, sizeof(innolux_init_sequence_91));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_92, sizeof(innolux_init_sequence_92));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_93, sizeof(innolux_init_sequence_93));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_94, sizeof(innolux_init_sequence_94));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_95, sizeof(innolux_init_sequence_95));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_96, sizeof(innolux_init_sequence_96));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_97, sizeof(innolux_init_sequence_97));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_98, sizeof(innolux_init_sequence_98));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_99, sizeof(innolux_init_sequence_99));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_100, sizeof(innolux_init_sequence_100));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_101, sizeof(innolux_init_sequence_101));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_102, sizeof(innolux_init_sequence_102));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_103, sizeof(innolux_init_sequence_103));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_104, sizeof(innolux_init_sequence_104));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_105, sizeof(innolux_init_sequence_105));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_106, sizeof(innolux_init_sequence_106));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_107, sizeof(innolux_init_sequence_107));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_108, sizeof(innolux_init_sequence_108));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_109, sizeof(innolux_init_sequence_109));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_110, sizeof(innolux_init_sequence_110));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_111, sizeof(innolux_init_sequence_111));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_112, sizeof(innolux_init_sequence_112));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_113, sizeof(innolux_init_sequence_113));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_114, sizeof(innolux_init_sequence_114));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_115, sizeof(innolux_init_sequence_115));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_116, sizeof(innolux_init_sequence_116));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_117, sizeof(innolux_init_sequence_117));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_118, sizeof(innolux_init_sequence_118));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_119, sizeof(innolux_init_sequence_119));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_120, sizeof(innolux_init_sequence_120));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_121, sizeof(innolux_init_sequence_121));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_122, sizeof(innolux_init_sequence_122));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_123, sizeof(innolux_init_sequence_123));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_124, sizeof(innolux_init_sequence_124));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_125, sizeof(innolux_init_sequence_125));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_126, sizeof(innolux_init_sequence_126));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_127, sizeof(innolux_init_sequence_127));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_128, sizeof(innolux_init_sequence_128));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_129, sizeof(innolux_init_sequence_129));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_130, sizeof(innolux_init_sequence_130));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_131, sizeof(innolux_init_sequence_131));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_132, sizeof(innolux_init_sequence_132));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_133, sizeof(innolux_init_sequence_133));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_134, sizeof(innolux_init_sequence_134));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_135, sizeof(innolux_init_sequence_135));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_136, sizeof(innolux_init_sequence_136));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_137, sizeof(innolux_init_sequence_137));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_138, sizeof(innolux_init_sequence_138));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_139, sizeof(innolux_init_sequence_139));
	dsi_vc_dcs_write(intel_dsi, 0, innolux_init_sequence_140, sizeof(innolux_init_sequence_140));
}

static void  n080ice_gb1_get_panel_info(int pipe,
					struct drm_connector *connector)
{
	DRM_DEBUG_KMS("\n");
	if (!connector) {
		DRM_DEBUG_KMS("Cpt: Invalid input to get_info\n");
	return;
	}

	if (pipe == 0) {
		connector->display_info.width_mm = 184;
		connector->display_info.height_mm = 114;
	}

	return;
}

static void n080ice_gb1_destroy(struct intel_dsi_device *dsi)
{
}

static void n080ice_gb1_dump_regs(struct intel_dsi_device *dsi)
{
}

static void n080ice_gb1_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *n080ice_gb1_get_modes(
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

	mode->hdisplay = 800;
	mode->hsync_start = mode->hdisplay + 16;
	mode->hsync_end = mode->hsync_start + 48;
	mode->htotal = mode->hsync_end  + 16;

	mode->vdisplay = 1280;
	mode->vsync_start = mode->vdisplay + 4;
	mode->vsync_end = mode->vsync_start + 8;
	mode->vtotal = mode->vsync_end + 4;

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

static bool n080ice_gb1_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	return true;
}

static enum drm_connector_status n080ice_gb1_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	dev_priv->is_mipi = true;
	DRM_DEBUG_KMS("\n");
	return connector_status_connected;
}

static bool n080ice_gb1_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}
#define DISP_RST_N 107
void n080ice_gb1_panel_reset(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	DRM_DEBUG_KMS("\n");

	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PAD, 0x00000005);
	msleep(20);
	/* panel disable */
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PAD, 0x00000004);
	usleep_range(100000, 120000);

	/* panel enable */
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PAD, 0x00000005);
	usleep_range(100000, 120000);

	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PAD, 0x00000004);
	msleep(20);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PAD, 0x00000005);
	msleep(5);
}

static int n080ice_gb1_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("\n");
	return MODE_OK;
}

static void n080ice_gb1_dpms(struct intel_dsi_device *dsi, bool enable)
{
	DRM_DEBUG_KMS("\n");
}

static void n080ice_gb1_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x35);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(5);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
}

static void n080ice_gb1_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(30);
	msleep(15);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(30);
}

bool n080ice_gb1_init(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

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

	DRM_DEBUG_KMS("Init: n080ice_gb1 panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to n080ice_gb1\n");
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
	intel_dsi->hs_to_lp_count = 0x18;
	/*b060*/
	intel_dsi->lp_byte_clk = 0x03;
	/*b080*/
	intel_dsi->dphy_reg = 0x170d340b;
	/* b088 high 16bits */
	intel_dsi->clk_lp_to_hs_count = 0x1e;
	/* b088 low 16bits */
	intel_dsi->clk_hs_to_lp_count = 0x0d;
	/* BTA sending at the last blanking line of VFP is disabled */
	intel_dsi->video_frmt_cfg_bits = 1<<3;
	intel_dsi->lane_count = 4;
	intel_dsi->port = 0; /* PORT_A by default */
	intel_dsi->burst_mode_ratio = 100;
	intel_dsi->backlight_off_delay = 20;
	intel_dsi->send_shutdown = true;
	intel_dsi->shutdown_pkt_delay = 20;

	return true;
}

/* Callbacks. We might not need them all. */
struct intel_dsi_dev_ops n080ice_gb1_dsi_display_ops = {
	.init = n080ice_gb1_init,
	.get_info = n080ice_gb1_get_panel_info,
	.create_resources = n080ice_gb1_create_resources,
	.dpms = n080ice_gb1_dpms,
	.mode_valid = n080ice_gb1_mode_valid,
	.mode_fixup = n080ice_gb1_mode_fixup,
	.panel_reset = n080ice_gb1_panel_reset,
	.detect = n080ice_gb1_detect,
	.get_hw_state = n080ice_gb1_get_hw_state,
	.get_modes = n080ice_gb1_get_modes,
	.destroy = n080ice_gb1_destroy,
	.dump_regs = n080ice_gb1_dump_regs,
	.enable = n080ice_gb1_enable,
	.disable = n080ice_gb1_disable,
	.send_otp_cmds = n080ice_gb1_send_otp_cmds,
};
