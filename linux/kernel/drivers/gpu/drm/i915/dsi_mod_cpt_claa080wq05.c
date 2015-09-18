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
#include "dsi_mod_cpt_claa080wq05.h"

static u8 cpt_enable_ic_power[]		= {0xC3, 0x40, 0x00, 0x28};
static u8 cpt_disable_ic_power[]	= {0xC3, 0x40, 0x00, 0x20};
/* Test key for level 2 commands */
static u8 cpt_init_sequence5[]		= {0xF0, 0x5A, 0x5A};
/* Test key for level 2 commands */
static u8 cpt_init_sequence6[]		= {0xF1, 0x5A, 0x5A};
/* Test key for level 3 commands */
static u8 cpt_init_sequence7[]		= {0xFC, 0xA5, 0xA5};
/* LV2 OTP Reload selection */
static u8 cpt_init_sequence8[]		= {0xD0, 0x00, 0x10};
/* Resolution selection */
static u8 cpt_init_sequence9[]		= {0xB1, 0x10};
/* Source Direction selection */
static u8 cpt_init_sequence10[]		= {0xB2, 0x14, 0x22, 0x2F, 0x04};
static u8 cpt_init_sequence11_1[]	= {0xB5, 0x01};
/* ASG Timing selection */
static u8 cpt_init_sequence11_2[]	= {0xEE, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x1F, 0x00};
/* ASG Timing selection */
static u8 cpt_init_sequence12[]		= {0xEF, 0x56, 0x34, 0x43, 0x65, 0x90, 0x80, 0x24, 0x80, 0x00, 0x91, 0x11, 0x11, 0x11};
/* ASG Pin Assignment */
static u8 cpt_init_sequence13[]		= {0xF7, 0x04, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x16, 0x17, 0x10, 0x01, 0x01,  0x01, 0x01, 0x04, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x16, 0x17, 0x10, 0x01, 0x01, 0x01, 0x01};
/* Vertical&Horizontal Porch control */
static u8 cpt_init_sequence14[]		= {0xF2, 0x02, 0x08, 0x08, 0x40, 0x10};
/* Source output control */
static u8 cpt_init_sequence15[]		= {0xF6, 0x60, 0x25, 0x26, 0x00, 0x00, 0x00};
static u8 cpt_init_sequence16_1[]	= {0xFA, 0x04, 0x35, 0x07, 0x0B, 0x12, 0x0B, 0x10, 0x16, 0x1A, 0x24, 0x2C, 0x33, 0x3B, 0x3B, 0x33, 0x34, 0x33};
/* Gamma */
static u8 cpt_init_sequence16_2[]	= {0xFB, 0x04, 0x35, 0x07, 0x0B, 0x12, 0x0B, 0x10, 0x16, 0x1A, 0x24, 0x2C, 0x33, 0x3B, 0x3B, 0x33, 0x34, 0x33};
/* DDI internal power control */
static u8 cpt_init_sequence17[]		= {0xF3, 0x01, 0xC4, 0xE0, 0x62, 0xD4, 0x83, 0x37, 0x3C, 0x24, 0x00};
/* DDI internal power sequence control */
static u8 cpt_init_sequence18[]		= {0xF4, 0x00, 0x02, 0x03, 0x26, 0x03, 0x02, 0x09, 0x00, 0x07, 0x16, 0x16, 0x03, 0x00, 0x08, 0x08, 0x03, 0x19, 0x1C, 0x12, 0x1C, 0x1D, 0x1E, 0x1A, 0x09, 0x01, 0x04, 0x02, 0x61, 0x74, 0x75, 0x72, 0x83, 0x80, 0x80, 0xF0};
static u8 cpt_init_sequence19_1[]	= {0xB0, 0x01};
/* Output Voltage setting & internal power sequence */
static u8 cpt_init_sequence19_2[]	= {0xF5, 0x2F, 0x2F, 0x5F, 0xAB, 0x98, 0x52, 0x0F, 0x33, 0x43, 0x04, 0x59, 0x54, 0x52, 0x05, 0x40, 0x40, 0x5D, 0x59, 0x40};
static u8 cpt_init_sequence20_1[]	= {0xBC, 0x01, 0x4E, 0x08};
/* Watch Dog */
static u8 cpt_init_sequence20_2[]	= {0xE1, 0x03, 0x10, 0x1C, 0xA0, 0x10};
/* DDI Analog interface Setting */
static u8 cpt_init_sequence21[]		= {0xFD, 0x16, 0x10, 0x11, 0x20, 0x09};
/* BC_CTRL Enable(Power IC Enable) */
static u8 cpt_init_sequence22[]		= {0xC3, 0x40, 0x00, 0x28};

void claa080wq05_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");

	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5, sizeof(cpt_init_sequence5));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence6, sizeof(cpt_init_sequence6));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence7, sizeof(cpt_init_sequence7));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence8, sizeof(cpt_init_sequence8));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence9, sizeof(cpt_init_sequence9));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence10, sizeof(cpt_init_sequence10));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence11_1, sizeof(cpt_init_sequence11_1));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence11_2, sizeof(cpt_init_sequence11_2));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence12, sizeof(cpt_init_sequence12));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence13, sizeof(cpt_init_sequence13));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence14, sizeof(cpt_init_sequence14));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence15, sizeof(cpt_init_sequence15));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence16_1, sizeof(cpt_init_sequence16_1));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence16_2, sizeof(cpt_init_sequence16_2));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence17, sizeof(cpt_init_sequence17));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence18, sizeof(cpt_init_sequence18));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence19_1, sizeof(cpt_init_sequence19_1));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence19_2, sizeof(cpt_init_sequence19_2));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence20_1, sizeof(cpt_init_sequence20_1));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence20_2, sizeof(cpt_init_sequence20_2));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence21, sizeof(cpt_init_sequence21));
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence22, sizeof(cpt_init_sequence22));
}

static void  claa080wq05_get_panel_info(int pipe,
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

static void claa080wq05_destroy(struct intel_dsi_device *dsi)
{
}

static void claa080wq05_dump_regs(struct intel_dsi_device *dsi)
{
}

static void claa080wq05_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *claa080wq05_get_modes(
	struct intel_dsi_device *dsi)
{
	struct drm_display_mode *mode;
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	/* Allocate */
	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode) {
		DRM_DEBUG_KMS("Cpt panel: No memory\n");
		return NULL;
	}

	/* Hardcode 800*1280 */
	/*HFP = 16, HSYNC = 5, HBP = 59, HP = 880*/
	/*VFP = 8, VSYNC = 5, VBP = 3, VP = 1296*/
	mode->hdisplay = 800;
	mode->hsync_start = mode->hdisplay + 16;
	mode->hsync_end = mode->hsync_start + 5;
	mode->htotal = mode->hsync_end  + 59;

	mode->vdisplay = 1280;
	mode->vsync_start = mode->vdisplay + 8;
	mode->vsync_end = mode->vsync_start + 5;
	mode->vtotal = mode->vsync_end + 3;

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

static bool claa080wq05_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	return true;
}

static enum drm_connector_status claa080wq05_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	dev_priv->is_mipi = true;
	DRM_DEBUG_KMS("\n");
	return connector_status_connected;
}

static bool claa080wq05_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}
#define DISP_RST_N 107
void claa080wq05_reset(struct intel_dsi_device *dsi)
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

static int claa080wq05_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("\n");
	return MODE_OK;
}

static void claa080wq05_dpms(struct intel_dsi_device *dsi, bool enable)
{
	DRM_DEBUG_KMS("\n");
}

static void claa080wq05_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");
	dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5, sizeof(cpt_init_sequence5));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(30);
	dsi_vc_dcs_write(intel_dsi, 0, cpt_enable_ic_power, sizeof(cpt_enable_ic_power));
	msleep(150);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	msleep(200);
}

static void claa080wq05_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	msleep(200);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(80);
	dsi_vc_dcs_write(intel_dsi, 0, cpt_disable_ic_power, sizeof(cpt_disable_ic_power));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(34);
}

bool claa080wq05_init(struct intel_dsi_device *dsi)
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

	DRM_DEBUG_KMS("Init: claa080wq05 panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to claa080wq05\n");
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
struct intel_dsi_dev_ops cpt_claa080wq05_dsi_display_ops = {
	.init = claa080wq05_init,
	.get_info = claa080wq05_get_panel_info,
	.create_resources = claa080wq05_create_resources,
	.dpms = claa080wq05_dpms,
	.mode_valid = claa080wq05_mode_valid,
	.mode_fixup = claa080wq05_mode_fixup,
	.panel_reset = claa080wq05_reset,
	.detect = claa080wq05_detect,
	.get_hw_state = claa080wq05_get_hw_state,
	.get_modes = claa080wq05_get_modes,
	.destroy = claa080wq05_destroy,
	.dump_regs = claa080wq05_dump_regs,
	.enable = claa080wq05_enable,
	.disable = claa080wq05_disable,
	.send_otp_cmds = claa080wq05_send_otp_cmds,
};
