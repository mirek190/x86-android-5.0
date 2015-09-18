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
#include "dsi_mod_boe_kd079d5_31nb_a9.h"

/* Page0 */
static u8 boe_init_sequence001[] = {0xE0, 0x00};
/* PASSWORD*/
static u8 boe_init_sequence002[] = {0xE1, 0x93};
static u8 boe_init_sequence003[] = {0xE2, 0x65};
static u8 boe_init_sequence004[] = {0xE3, 0xF8};
/* Sequence Ctrl */
static u8 boe_init_sequence005[] = {0x70, 0x02};
static u8 boe_init_sequence006[] = {0x71, 0x23};
static u8 boe_init_sequence007[] = {0x72, 0x06};
/* Page1 */
static u8 boe_init_sequence008[] = {0xE0, 0x01};
/* Set VCOM*/
static u8 boe_init_sequence009[] = {0x00, 0x00};
static u8 boe_init_sequence010[] = {0x01, 0xA0};
/* Set VCOM_Reverse*/
static u8 boe_init_sequence011[] = {0x03, 0x00};
static u8 boe_init_sequence012[] = {0x04, 0xA0};
/* Set Gamma Power, VGMP, VGMN, VGSP, VGSN */
static u8 boe_init_sequence013[] = {0x17, 0x00};
static u8 boe_init_sequence014[] = {0x18, 0xB0};
static u8 boe_init_sequence015[] = {0x19, 0x00};
static u8 boe_init_sequence016[] = {0x1A, 0x00};
static u8 boe_init_sequence017[] = {0x1B, 0xB0};
static u8 boe_init_sequence018[] = {0x1C, 0x00};
/* Set Gate Power */
static u8 boe_init_sequence019[] = {0x1F, 0x3E};
static u8 boe_init_sequence020[] = {0x20, 0x2D};
static u8 boe_init_sequence021[] = {0x21, 0x2D};
static u8 boe_init_sequence022[] = {0x22, 0x0E};
/* SETPANEL */
static u8 boe_init_sequence023[] = {0x37, 0x19};
/* SET RGBCYC */
static u8 boe_init_sequence024[] = {0x38, 0x05};
static u8 boe_init_sequence025[] = {0x39, 0x08};
static u8 boe_init_sequence026[] = {0x3A, 0x12};
static u8 boe_init_sequence027[] = {0x3C, 0x80};
static u8 boe_init_sequence028[] = {0x3E, 0x88};
static u8 boe_init_sequence029[] = {0x3F, 0x88};
/* Set TCON */
static u8 boe_init_sequence030[] = {0x40, 0x05};
static u8 boe_init_sequence031[] = {0x41, 0x80};
static u8 boe_init_sequence032[] = {0x42, 0x96};
/* Power Voltage */
static u8 boe_init_sequence033[] = {0x55, 0x01};
static u8 boe_init_sequence034[] = {0x56, 0x01};
static u8 boe_init_sequence035[] = {0x57, 0xA8};
static u8 boe_init_sequence036[] = {0x58, 0x0A};
static u8 boe_init_sequence037[] = {0x59, 0x0A};
static u8 boe_init_sequence038[] = {0x5A, 0x29};
static u8 boe_init_sequence039[] = {0x5B, 0x15};
/* Gamma */
static u8 boe_init_sequence040[] = {0x5D, 0x7C};
static u8 boe_init_sequence041[] = {0x5E, 0x68};
static u8 boe_init_sequence042[] = {0x5F, 0x59};
static u8 boe_init_sequence043[] = {0x60, 0x4E};
static u8 boe_init_sequence044[] = {0x61, 0x4B};
static u8 boe_init_sequence045[] = {0x62, 0x3D};
static u8 boe_init_sequence046[] = {0x63, 0x42};
static u8 boe_init_sequence047[] = {0x64, 0x2D};
static u8 boe_init_sequence048[] = {0x65, 0x48};
static u8 boe_init_sequence049[] = {0x66, 0x48};
static u8 boe_init_sequence050[] = {0x67, 0x48};
static u8 boe_init_sequence051[] = {0x68, 0x68};
static u8 boe_init_sequence052[] = {0x69, 0x56};
static u8 boe_init_sequence053[] = {0x6A, 0x5D};
static u8 boe_init_sequence054[] = {0x6B, 0x51};
static u8 boe_init_sequence055[] = {0x6C, 0x4F};
static u8 boe_init_sequence056[] = {0x6D, 0x3E};
static u8 boe_init_sequence057[] = {0x6E, 0x36};
static u8 boe_init_sequence058[] = {0x6F, 0x00};
static u8 boe_init_sequence059[] = {0x70, 0x7C};
static u8 boe_init_sequence060[] = {0x71, 0x68};
static u8 boe_init_sequence061[] = {0x72, 0x59};
static u8 boe_init_sequence062[] = {0x73, 0x4E};
static u8 boe_init_sequence063[] = {0x74, 0x4B};
static u8 boe_init_sequence064[] = {0x75, 0x3D};
static u8 boe_init_sequence065[] = {0x76, 0x42};
static u8 boe_init_sequence066[] = {0x77, 0x2D};
static u8 boe_init_sequence067[] = {0x78, 0x48};
static u8 boe_init_sequence068[] = {0x79, 0x48};
static u8 boe_init_sequence069[] = {0x7A, 0x48};
static u8 boe_init_sequence070[] = {0x7B, 0x68};
static u8 boe_init_sequence071[] = {0x7C, 0x56};
static u8 boe_init_sequence072[] = {0x7D, 0x5D};
static u8 boe_init_sequence073[] = {0x7E, 0x51};
static u8 boe_init_sequence074[] = {0x7F, 0x4F};
static u8 boe_init_sequence075[] = {0x80, 0x3E};
static u8 boe_init_sequence076[] = {0x81, 0x36};
static u8 boe_init_sequence077[] = {0x82, 0x00};
/* Page, for GIP*/
static u8 boe_init_sequence078[] = {0xE0, 0x02};
/* GIP_L Pin mapping */
static u8 boe_init_sequence079[] = {0x00, 0x47};
static u8 boe_init_sequence080[] = {0x01, 0x47};
static u8 boe_init_sequence081[] = {0x02, 0x45};
static u8 boe_init_sequence082[] = {0x03, 0x45};
static u8 boe_init_sequence083[] = {0x04, 0x4B};
static u8 boe_init_sequence084[] = {0x05, 0x4B};
static u8 boe_init_sequence085[] = {0x06, 0x49};
static u8 boe_init_sequence086[] = {0x07, 0x49};
static u8 boe_init_sequence087[] = {0x08, 0x41};
static u8 boe_init_sequence088[] = {0x09, 0x1F};
static u8 boe_init_sequence089[] = {0x0A, 0x1F};
static u8 boe_init_sequence090[] = {0x0B, 0x1F};
static u8 boe_init_sequence091[] = {0x0C, 0x1F};
static u8 boe_init_sequence092[] = {0x0D, 0x1F};
static u8 boe_init_sequence093[] = {0x0E, 0x1F};
static u8 boe_init_sequence094[] = {0x0F, 0x43};
static u8 boe_init_sequence095[] = {0x10, 0x1F};
static u8 boe_init_sequence096[] = {0x11, 0x1F};
static u8 boe_init_sequence097[] = {0x12, 0x1F};
static u8 boe_init_sequence098[] = {0x13, 0x1F};
static u8 boe_init_sequence099[] = {0x14, 0x1F};
static u8 boe_init_sequence100[] = {0x15, 0x1F};
/* GIP_R Pin mapping */
static u8 boe_init_sequence101[] = {0x16, 0x46};
static u8 boe_init_sequence102[] = {0x17, 0x46};
static u8 boe_init_sequence103[] = {0x18, 0x44};
static u8 boe_init_sequence104[] = {0x19, 0x44};
static u8 boe_init_sequence105[] = {0x1A, 0x4A};
static u8 boe_init_sequence106[] = {0x1B, 0x4A};
static u8 boe_init_sequence107[] = {0x1C, 0x48};
static u8 boe_init_sequence108[] = {0x1D, 0x48};
static u8 boe_init_sequence109[] = {0x1E, 0x40};
static u8 boe_init_sequence110[] = {0x1F, 0x1F};
static u8 boe_init_sequence111[] = {0x20, 0x1F};
static u8 boe_init_sequence112[] = {0x21, 0x1F};
static u8 boe_init_sequence113[] = {0x22, 0x1F};
static u8 boe_init_sequence114[] = {0x23, 0x1F};
static u8 boe_init_sequence115[] = {0x24, 0x1F};
static u8 boe_init_sequence116[] = {0x25, 0x42};
static u8 boe_init_sequence117[] = {0x26, 0x1F};
static u8 boe_init_sequence118[] = {0x27, 0x1F};
static u8 boe_init_sequence119[] = {0x28, 0x1F};
static u8 boe_init_sequence120[] = {0x29, 0x1F};
static u8 boe_init_sequence121[] = {0x2A, 0x1F};
static u8 boe_init_sequence122[] = {0x2B, 0x1F};
/* GIP_L_GS Pin mapping */
static u8 boe_init_sequence123[] = {0x2C, 0x11};
static u8 boe_init_sequence124[] = {0x2D, 0x0F};
static u8 boe_init_sequence125[] = {0x2E, 0x0D};
static u8 boe_init_sequence126[] = {0x2F, 0x0B};
static u8 boe_init_sequence127[] = {0x30, 0x09};
static u8 boe_init_sequence128[] = {0x31, 0x07};
static u8 boe_init_sequence129[] = {0x32, 0x05};
static u8 boe_init_sequence130[] = {0x33, 0x18};
static u8 boe_init_sequence131[] = {0x34, 0x17};
static u8 boe_init_sequence132[] = {0x35, 0x1F};
static u8 boe_init_sequence133[] = {0x36, 0x01};
static u8 boe_init_sequence134[] = {0x37, 0x1F};
static u8 boe_init_sequence135[] = {0x38, 0x1F};
static u8 boe_init_sequence136[] = {0x39, 0x1F};
static u8 boe_init_sequence137[] = {0x3A, 0x1F};
static u8 boe_init_sequence138[] = {0x3B, 0x1F};
static u8 boe_init_sequence139[] = {0x3C, 0x1F};
static u8 boe_init_sequence140[] = {0x3D, 0x1F};
static u8 boe_init_sequence141[] = {0x3E, 0x1F};
static u8 boe_init_sequence142[] = {0x3F, 0x13};
static u8 boe_init_sequence143[] = {0x40, 0x1F};
static u8 boe_init_sequence144[] = {0x41, 0x1F};
/* GIP_R_GS Pin mapping */
static u8 boe_init_sequence145[] = {0x42, 0x10};
static u8 boe_init_sequence146[] = {0x43, 0x0E};
static u8 boe_init_sequence147[] = {0x44, 0x0C};
static u8 boe_init_sequence148[] = {0x45, 0x0A};
static u8 boe_init_sequence149[] = {0x46, 0x08};
static u8 boe_init_sequence150[] = {0x47, 0x06};
static u8 boe_init_sequence151[] = {0x48, 0x04};
static u8 boe_init_sequence152[] = {0x49, 0x18};
static u8 boe_init_sequence153[] = {0x4A, 0x17};
static u8 boe_init_sequence154[] = {0x4B, 0x1F};
static u8 boe_init_sequence155[] = {0x4C, 0x00};
static u8 boe_init_sequence156[] = {0x4D, 0x1F};
static u8 boe_init_sequence157[] = {0x4E, 0x1F};
static u8 boe_init_sequence158[] = {0x4F, 0x1F};
static u8 boe_init_sequence159[] = {0x50, 0x1F};
static u8 boe_init_sequence160[] = {0x51, 0x1F};
static u8 boe_init_sequence161[] = {0x52, 0x1F};
static u8 boe_init_sequence162[] = {0x53, 0x1F};
static u8 boe_init_sequence163[] = {0x54, 0x1F};
static u8 boe_init_sequence164[] = {0x55, 0x12};
static u8 boe_init_sequence165[] = {0x56, 0x1F};
static u8 boe_init_sequence166[] = {0x57, 0x1F};
/* GIP Timing */
static u8 boe_init_sequence167[] = {0x58, 0x10};
static u8 boe_init_sequence168[] = {0x59, 0x00};
static u8 boe_init_sequence169[] = {0x5A, 0x00};
static u8 boe_init_sequence170[] = {0x5B, 0x30};
static u8 boe_init_sequence171[] = {0x5C, 0x03};
static u8 boe_init_sequence172[] = {0x5D, 0x30};
static u8 boe_init_sequence173[] = {0x5E, 0x01};
static u8 boe_init_sequence174[] = {0x5F, 0x02};
static u8 boe_init_sequence175[] = {0x60, 0x30};
static u8 boe_init_sequence176[] = {0x61, 0x01};
static u8 boe_init_sequence177[] = {0x62, 0x02};
static u8 boe_init_sequence178[] = {0x63, 0x14};
static u8 boe_init_sequence179[] = {0x64, 0x80};
static u8 boe_init_sequence180[] = {0x65, 0x05};
static u8 boe_init_sequence181[] = {0x66, 0x12};
static u8 boe_init_sequence182[] = {0x67, 0x73};
static u8 boe_init_sequence183[] = {0x68, 0x05};
static u8 boe_init_sequence184[] = {0x69, 0x14};
static u8 boe_init_sequence185[] = {0x6A, 0x80};
static u8 boe_init_sequence186[] = {0x6B, 0x09};
static u8 boe_init_sequence187[] = {0x6C, 0x00};
static u8 boe_init_sequence188[] = {0x6D, 0x04};
static u8 boe_init_sequence189[] = {0x6E, 0x04};
static u8 boe_init_sequence190[] = {0x6F, 0x88};
static u8 boe_init_sequence191[] = {0x70, 0x00};
static u8 boe_init_sequence192[] = {0x71, 0x00};
static u8 boe_init_sequence193[] = {0x72, 0x06};
static u8 boe_init_sequence194[] = {0x73, 0x7B};
static u8 boe_init_sequence195[] = {0x74, 0x00};
static u8 boe_init_sequence196[] = {0x75, 0x3C};
static u8 boe_init_sequence197[] = {0x76, 0x00};
static u8 boe_init_sequence198[] = {0x77, 0x0D};
static u8 boe_init_sequence199[] = {0x78, 0x2C};
static u8 boe_init_sequence200[] = {0x79, 0x00};
static u8 boe_init_sequence201[] = {0x7A, 0x00};
static u8 boe_init_sequence202[] = {0x7B, 0x00};
static u8 boe_init_sequence203[] = {0x7C, 0x00};
static u8 boe_init_sequence204[] = {0x7D, 0x03};
static u8 boe_init_sequence205[] = {0x7E, 0x7B};

static u8 boe_init_sequence206[] = {0x0E, 0x01};
/* Page3 */
static u8 boe_init_sequence207[] = {0xE0, 0x03};
static u8 boe_init_sequence208[] = {0x98, 0x2F};

static u8 boe_init_sequence210[] = {0x5D, 0x7C};
static u8 boe_init_sequence211[] = {0x57, 0x68};
static u8 boe_init_sequence212[] = {0x58, 0x59};
static u8 boe_init_sequence213[] = {0x59, 0x4E};
static u8 boe_init_sequence214[] = {0x5A, 0x4B};
static u8 boe_init_sequence215[] = {0x5B, 0x3D};
static u8 boe_init_sequence216[] = {0x5C, 0x42};
static u8 boe_init_sequence217[] = {0x5D, 0x2D};
static u8 boe_init_sequence218[] = {0x5D, 0x7C};
static u8 boe_init_sequence219[] = {0x57, 0x68};
static u8 boe_init_sequence220[] = {0x58, 0x59};
static u8 boe_init_sequence221[] = {0x59, 0x4E};
static u8 boe_init_sequence222[] = {0x5A, 0x4B};
static u8 boe_init_sequence223[] = {0x5B, 0x3D};
static u8 boe_init_sequence224[] = {0x5C, 0x42};
static u8 boe_init_sequence225[] = {0x5D, 0x2D};

static void boe_kd079d5_31nb_a9_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");

	msleep(20);
/* Page0 */
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence001, sizeof(boe_init_sequence001));
/* PASSWORD*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence002, sizeof(boe_init_sequence002));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence003, sizeof(boe_init_sequence003));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence004, sizeof(boe_init_sequence004));
/* Page0*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence001, sizeof(boe_init_sequence001));
/* Sequence Ctrl */
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence005, sizeof(boe_init_sequence005));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence006, sizeof(boe_init_sequence006));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence007, sizeof(boe_init_sequence007));
/* Page1 */
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence008, sizeof(boe_init_sequence008));
/* Set VCOM*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence009, sizeof(boe_init_sequence009));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence010, sizeof(boe_init_sequence010));
/* Set VCOM_Reverse*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence011, sizeof(boe_init_sequence011));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence012, sizeof(boe_init_sequence012));
	/* Set Gamma Power, VGMP, VGMN, VGSP, VGSN*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence013, sizeof(boe_init_sequence013));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence014, sizeof(boe_init_sequence014));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence015, sizeof(boe_init_sequence015));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence016, sizeof(boe_init_sequence016));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence017, sizeof(boe_init_sequence017));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence018, sizeof(boe_init_sequence018));
	/* Set Gate Power*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence019, sizeof(boe_init_sequence019));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence020, sizeof(boe_init_sequence020));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence021, sizeof(boe_init_sequence021));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence022, sizeof(boe_init_sequence022));
	/* SETPANEL*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence023, sizeof(boe_init_sequence023));
	/* SET RGBCYC*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence024, sizeof(boe_init_sequence024));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence025, sizeof(boe_init_sequence025));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence026, sizeof(boe_init_sequence026));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence027, sizeof(boe_init_sequence027));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence028, sizeof(boe_init_sequence028));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence029, sizeof(boe_init_sequence029));
	/* Set TCON */
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence030, sizeof(boe_init_sequence030));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence031, sizeof(boe_init_sequence031));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence032, sizeof(boe_init_sequence032));
	/* Power voltage */
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence033, sizeof(boe_init_sequence033));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence034, sizeof(boe_init_sequence034));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence035, sizeof(boe_init_sequence035));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence036, sizeof(boe_init_sequence036));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence037, sizeof(boe_init_sequence037));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence038, sizeof(boe_init_sequence038));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence039, sizeof(boe_init_sequence039));
	/* Gamma*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence040, sizeof(boe_init_sequence040));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence041, sizeof(boe_init_sequence041));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence042, sizeof(boe_init_sequence042));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence043, sizeof(boe_init_sequence043));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence044, sizeof(boe_init_sequence044));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence045, sizeof(boe_init_sequence045));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence046, sizeof(boe_init_sequence046));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence047, sizeof(boe_init_sequence047));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence048, sizeof(boe_init_sequence048));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence049, sizeof(boe_init_sequence049));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence050, sizeof(boe_init_sequence050));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence051, sizeof(boe_init_sequence051));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence052, sizeof(boe_init_sequence052));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence053, sizeof(boe_init_sequence053));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence054, sizeof(boe_init_sequence054));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence055, sizeof(boe_init_sequence055));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence056, sizeof(boe_init_sequence056));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence057, sizeof(boe_init_sequence057));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence058, sizeof(boe_init_sequence058));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence059, sizeof(boe_init_sequence059));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence060, sizeof(boe_init_sequence060));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence061, sizeof(boe_init_sequence061));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence062, sizeof(boe_init_sequence062));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence063, sizeof(boe_init_sequence063));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence064, sizeof(boe_init_sequence064));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence065, sizeof(boe_init_sequence065));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence066, sizeof(boe_init_sequence066));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence067, sizeof(boe_init_sequence067));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence068, sizeof(boe_init_sequence068));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence069, sizeof(boe_init_sequence069));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence070, sizeof(boe_init_sequence070));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence071, sizeof(boe_init_sequence071));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence072, sizeof(boe_init_sequence072));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence073, sizeof(boe_init_sequence073));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence074, sizeof(boe_init_sequence074));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence075, sizeof(boe_init_sequence075));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence076, sizeof(boe_init_sequence076));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence077, sizeof(boe_init_sequence077));
	/* Page2, for GIP */
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence078, sizeof(boe_init_sequence078));
	/* GIP_L Pin mapping*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence079, sizeof(boe_init_sequence079));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence080, sizeof(boe_init_sequence080));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence081, sizeof(boe_init_sequence081));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence082, sizeof(boe_init_sequence082));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence083, sizeof(boe_init_sequence083));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence084, sizeof(boe_init_sequence084));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence085, sizeof(boe_init_sequence085));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence086, sizeof(boe_init_sequence086));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence087, sizeof(boe_init_sequence087));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence088, sizeof(boe_init_sequence088));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence089, sizeof(boe_init_sequence089));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence090, sizeof(boe_init_sequence090));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence091, sizeof(boe_init_sequence091));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence092, sizeof(boe_init_sequence092));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence093, sizeof(boe_init_sequence093));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence094, sizeof(boe_init_sequence094));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence095, sizeof(boe_init_sequence095));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence096, sizeof(boe_init_sequence096));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence097, sizeof(boe_init_sequence097));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence098, sizeof(boe_init_sequence098));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence099, sizeof(boe_init_sequence099));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence100, sizeof(boe_init_sequence100));
	/*GIP_R Pin mapping*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence101, sizeof(boe_init_sequence101));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence102, sizeof(boe_init_sequence102));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence103, sizeof(boe_init_sequence103));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence104, sizeof(boe_init_sequence104));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence105, sizeof(boe_init_sequence105));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence106, sizeof(boe_init_sequence106));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence107, sizeof(boe_init_sequence107));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence108, sizeof(boe_init_sequence108));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence109, sizeof(boe_init_sequence109));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence110, sizeof(boe_init_sequence110));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence111, sizeof(boe_init_sequence111));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence112, sizeof(boe_init_sequence112));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence113, sizeof(boe_init_sequence113));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence114, sizeof(boe_init_sequence114));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence115, sizeof(boe_init_sequence115));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence116, sizeof(boe_init_sequence116));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence117, sizeof(boe_init_sequence117));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence118, sizeof(boe_init_sequence118));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence119, sizeof(boe_init_sequence119));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence120, sizeof(boe_init_sequence120));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence121, sizeof(boe_init_sequence121));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence122, sizeof(boe_init_sequence122));
	/* GIP_L_GS Pin mapping */
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence123, sizeof(boe_init_sequence123));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence124, sizeof(boe_init_sequence124));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence125, sizeof(boe_init_sequence125));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence126, sizeof(boe_init_sequence126));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence127, sizeof(boe_init_sequence127));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence128, sizeof(boe_init_sequence128));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence129, sizeof(boe_init_sequence129));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence130, sizeof(boe_init_sequence130));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence131, sizeof(boe_init_sequence131));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence132, sizeof(boe_init_sequence132));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence133, sizeof(boe_init_sequence133));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence134, sizeof(boe_init_sequence134));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence135, sizeof(boe_init_sequence135));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence136, sizeof(boe_init_sequence136));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence137, sizeof(boe_init_sequence137));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence138, sizeof(boe_init_sequence138));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence139, sizeof(boe_init_sequence139));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence140, sizeof(boe_init_sequence140));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence141, sizeof(boe_init_sequence141));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence142, sizeof(boe_init_sequence142));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence143, sizeof(boe_init_sequence143));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence144, sizeof(boe_init_sequence144));
	/* GIP_R_GS Pin mapping*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence145, sizeof(boe_init_sequence145));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence146, sizeof(boe_init_sequence146));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence147, sizeof(boe_init_sequence147));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence148, sizeof(boe_init_sequence148));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence149, sizeof(boe_init_sequence149));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence150, sizeof(boe_init_sequence150));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence151, sizeof(boe_init_sequence151));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence152, sizeof(boe_init_sequence152));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence153, sizeof(boe_init_sequence153));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence154, sizeof(boe_init_sequence154));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence155, sizeof(boe_init_sequence155));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence156, sizeof(boe_init_sequence156));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence157, sizeof(boe_init_sequence157));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence158, sizeof(boe_init_sequence158));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence159, sizeof(boe_init_sequence159));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence160, sizeof(boe_init_sequence160));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence161, sizeof(boe_init_sequence161));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence162, sizeof(boe_init_sequence162));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence163, sizeof(boe_init_sequence163));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence164, sizeof(boe_init_sequence164));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence165, sizeof(boe_init_sequence165));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence166, sizeof(boe_init_sequence166));
	/* GIP Timing*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence167, sizeof(boe_init_sequence167));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence168, sizeof(boe_init_sequence168));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence169, sizeof(boe_init_sequence169));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence170, sizeof(boe_init_sequence170));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence171, sizeof(boe_init_sequence171));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence172, sizeof(boe_init_sequence172));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence173, sizeof(boe_init_sequence173));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence174, sizeof(boe_init_sequence174));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence175, sizeof(boe_init_sequence175));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence176, sizeof(boe_init_sequence176));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence177, sizeof(boe_init_sequence177));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence178, sizeof(boe_init_sequence178));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence179, sizeof(boe_init_sequence179));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence180, sizeof(boe_init_sequence180));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence181, sizeof(boe_init_sequence181));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence182, sizeof(boe_init_sequence182));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence183, sizeof(boe_init_sequence183));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence184, sizeof(boe_init_sequence184));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence185, sizeof(boe_init_sequence185));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence186, sizeof(boe_init_sequence186));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence187, sizeof(boe_init_sequence187));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence188, sizeof(boe_init_sequence188));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence189, sizeof(boe_init_sequence189));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence190, sizeof(boe_init_sequence190));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence191, sizeof(boe_init_sequence191));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence192, sizeof(boe_init_sequence192));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence193, sizeof(boe_init_sequence193));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence194, sizeof(boe_init_sequence194));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence195, sizeof(boe_init_sequence195));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence196, sizeof(boe_init_sequence196));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence197, sizeof(boe_init_sequence197));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence198, sizeof(boe_init_sequence198));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence199, sizeof(boe_init_sequence199));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence200, sizeof(boe_init_sequence200));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence201, sizeof(boe_init_sequence201));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence202, sizeof(boe_init_sequence202));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence203, sizeof(boe_init_sequence203));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence204, sizeof(boe_init_sequence204));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence205, sizeof(boe_init_sequence205));

	/* Page1*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence008, sizeof(boe_init_sequence008));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence206, sizeof(boe_init_sequence206));
	/* Page3*/
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence207, sizeof(boe_init_sequence207));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence208, sizeof(boe_init_sequence208));

	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence001, sizeof(boe_init_sequence001));

/* Gamma*/

	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence210, sizeof(boe_init_sequence210));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence211, sizeof(boe_init_sequence211));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence212, sizeof(boe_init_sequence212));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence213, sizeof(boe_init_sequence213));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence214, sizeof(boe_init_sequence214));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence215, sizeof(boe_init_sequence215));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence216, sizeof(boe_init_sequence216));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence217, sizeof(boe_init_sequence217));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence218, sizeof(boe_init_sequence218));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence219, sizeof(boe_init_sequence219));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence220, sizeof(boe_init_sequence220));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence221, sizeof(boe_init_sequence221));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence222, sizeof(boe_init_sequence222));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence223, sizeof(boe_init_sequence223));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence224, sizeof(boe_init_sequence224));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence225, sizeof(boe_init_sequence225));


}

static void  boe_kd079d5_31nb_a9_get_panel_info(int pipe,
					struct drm_connector *connector)
{
	DRM_DEBUG_KMS("\n");
	if (!connector) {
		DRM_DEBUG_KMS("boe: Invalid input to get_info\n");
		return;
	}

	if (pipe == 0) {
		connector->display_info.width_mm = 108;
		connector->display_info.height_mm = 172;
	}

	return;
}

static void boe_kd079d5_31nb_a9_destroy(struct intel_dsi_device *dsi)
{
}

static void boe_kd079d5_31nb_a9_dump_regs(struct intel_dsi_device *dsi)
{
}

static void boe_kd079d5_31nb_a9_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *boe_kd079d5_31nb_a9_get_modes(
	struct intel_dsi_device *dsi)
{
	struct drm_display_mode *mode = NULL;
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");
	/* Allocate */
	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode) {
		DRM_DEBUG_KMS("boe panel: No memory\n");
		return NULL;
	}

	/* Hardcode 768*1024 */
	/*HFP = 80, HSYNC = 30, HBP = 20, HP = 1995*/
	/*VFP = 8, VSYNC = 4, VBP = 4, VP = 1261*/
	mode->hdisplay = 768;
	mode->hsync_start = mode->hdisplay + 80;
	mode->hsync_end = mode->hsync_start + 30;
	mode->htotal = mode->hsync_end  + 20;

	mode->vdisplay = 1024;
	mode->vsync_start = mode->vdisplay + 8;
	mode->vsync_end = mode->vsync_start + 4;
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

static bool boe_kd079d5_31nb_a9_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	return true;
}

static enum drm_connector_status boe_kd079d5_31nb_a9_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	dev_priv->is_mipi = true;
	DRM_DEBUG_KMS("\n");
	return connector_status_connected;
}

static bool boe_kd079d5_31nb_a9_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}
#define DISP_RST_N 107
void boe_kd079d5_31nb_a9_reset(struct intel_dsi_device *dsi)
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

static int boe_kd079d5_31nb_a9_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("\n");
	return MODE_OK;
}

static void boe_kd079d5_31nb_a9_dpms(struct intel_dsi_device *dsi, bool enable)
{
/*	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev); */

	DRM_DEBUG_KMS("\n");
}
static void boe_kd079d5_31nb_a9_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");

	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(120);  /* wait for 150ms */
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	msleep(20);  /* wait for 200ms */

}
static void boe_kd079d5_31nb_a9_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");

	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);

	msleep(200);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(100);

}

bool boe_kd079d5_31nb_a9_init(struct intel_dsi_device *dsi)
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

	DRM_DEBUG_KMS("Init: boe_kd079d5_31nb_a9 panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to boe_kd079d5_31nb_a9\n");
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
	/* dev_priv->mipi.panel_bpp = 24; */

	return true;
}

/* Callbacks. We might not need them all. */
struct intel_dsi_dev_ops boe_kd079d5_31nb_a9_dsi_display_ops = {
	.init = boe_kd079d5_31nb_a9_init,
	.get_info = boe_kd079d5_31nb_a9_get_panel_info,
	.create_resources = boe_kd079d5_31nb_a9_create_resources,
	.dpms = boe_kd079d5_31nb_a9_dpms,
	.mode_valid = boe_kd079d5_31nb_a9_mode_valid,
	.mode_fixup = boe_kd079d5_31nb_a9_mode_fixup,
	.panel_reset = boe_kd079d5_31nb_a9_reset,
	.detect = boe_kd079d5_31nb_a9_detect,
	.get_hw_state = boe_kd079d5_31nb_a9_get_hw_state,
	.get_modes = boe_kd079d5_31nb_a9_get_modes,
	.destroy = boe_kd079d5_31nb_a9_destroy,
	.dump_regs = boe_kd079d5_31nb_a9_dump_regs,
	.enable = boe_kd079d5_31nb_a9_enable,
	.disable = boe_kd079d5_31nb_a9_disable,
	.send_otp_cmds = boe_kd079d5_31nb_a9_send_otp_cmds,
};
