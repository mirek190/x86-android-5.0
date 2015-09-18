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
#include "dsi_mod_auo_b080ean011.h"


static u8 auo_enable_ic_power[]      = {0xC3, 0x40, 0x00, 0x28};
static u8 auo_disable_ic_power[]      = {0xC3, 0x40, 0x00, 0x20};
static u8 auo_password[]      = {0xF0, 0x5A, 0x5A};

static u8 auo_init_sequence1[]      = {0xF1, 0x5A, 0x5A};
static u8 auo_init_sequence2[]      = {0xFC, 0xA5, 0xA5};
static u8 auo_init_sequence3[]      = {0xD0, 0x00, 0x10};
static u8 auo_init_sequence4[]      = {0xBC, 0x1, 0x4E, 0xA0};
static u8 auo_init_sequence5[]      = {0xE1, 0x3, 0x10, 0x1C, 0x82, 0x07};
static u8 auo_init_sequence6[]      = {0xB1, 0x10};
static u8 auo_init_sequence7[]      = {0xB2, 0x14, 0x22, 0x2F, 0x04};
static u8 auo_init_sequence8[]      = {0xF2, 0x02, 0xC, 0x8, 0x88, 0x18};
static u8 auo_init_sequence9[]      = {0xB5, 0x1E};
static u8 auo_init_sequence10[]      = {0xB0, 0x04};
static u8 auo_init_sequence11[]      = {0xFD, 0x9};
static u8 auo_init_sequence12[]      = {0xF6, 0x63, 0x21, 0x86, 0x00, 0x00, 0x00};
static u8 auo_init_sequence13[]      = {0xD8, 0x5E, 0x4C, 0x10};
static u8 auo_init_sequence14[]      = {0xF3, 0x1, 0xC0, 0xE0, 0x62, 0xD0, 0x81, 0x35, 0xF3, 0x30, 0x24, 0x00};
static u8 auo_init_sequence15[]      = {0xF4, 0x00, 0x02, 0x03, 0x26, 0x03, 0x02, 0x09, 0x00, 0x07, 0x16, 0x16, 0x03, 0x00
, 0x08, 0x08, 0x03, 0x00, 0x00, 0x12, 0x1C, 0x1D, 0x1E, 0x01, 0x09, 0x01, 0x04, 0x02, 0x61, 0x74, 0x75
, 0x72, 0x83, 0x80, 0x80, 0x00, 0x00, 0x01, 0x01, 0x28, 0x04, 0x03, 0x28, 0x01, 0xD1, 0x32};
static u8 auo_init_sequence16[]      = {0xF5, 0x9D, 0x42, 0x42, 0x5F, 0xAB, 0x98, 0x4F, 0x0F
, 0x33, 0x43, 0x04, 0x59, 0x54, 0x52, 0x05, 0x40, 0x60, 0x40, 0x60, 0x40, 0x27, 0x26, 0x52, 0x25
, 0x6D, 0x18};

static u8 auo_init_sequence17[] = {0xEE, 0x3F, 0x3, 0x3F, 0x00, 0x3F, 0x3F, 0x3F, 0x00, 0x11, 0x22};

static u8 auo_init_sequence18[] = {0xEF, 0x12, 0x12, 0x43
, 0x43, 0x90, 0x84, 0x24
, 0x81, 0x00, 0x21, 0x21
, 0x03, 0x03, 0x40, 0x80
, 0x82, 0x00};
static u8 auo_init_sequence19[] = {0xFA, 0x00, 0x35, 0x06, 0x0A, 0x14, 0x0D, 0x13, 0x19
	, 0x1C, 0x25, 0x2B, 0x32, 0x3B, 0x39, 0x3D, 0x38, 0x3D, 0x25, 0x30, 0x26, 0x2A, 0x2B, 0x1E, 0x22
	, 0x23, 0x22, 0x28, 0x2D, 0x33, 0x3B, 0x38, 0x2D, 0x2D, 0x2A, 0x0C, 0x35, 0x10, 0x14
	, 0x1C, 0x14, 0x1A, 0x1E, 0x1F, 0x27, 0x2C, 0x33, 0x3B, 0x38, 0x30, 0x30, 0x30};

static u8 auo_init_sequence20[] = {0xFA, 0x00, 0x35, 0x06, 0x0A, 0x14, 0x0D, 0x13, 0x19
	, 0x1C, 0x25, 0x2B, 0x32, 0x3B, 0x39, 0x3D, 0x38, 0x3D, 0x25, 0x30, 0x26, 0x2A, 0x2B, 0x1E, 0x22
	, 0x23, 0x22, 0x28, 0x2D, 0x33, 0x3B, 0x38, 0x2D, 0x2D, 0x2A, 0x0C, 0x35, 0x10, 0x14
	, 0x1C, 0x14, 0x1A, 0x1E, 0x1F, 0x27, 0x2C, 0x33, 0x3B, 0x38, 0x30, 0x30, 0x30};

static u8 auo_init_sequence21[] = {0xF7, 0x0B, 0x0B, 0x09
, 0x09, 0x0A, 0x0A, 0x08
, 0x08, 0x01, 0x16, 0x16
, 0x17, 0x17, 0x07, 0x01
, 0x01, 0x0B, 0x0B, 0x09
, 0x09, 0x0A, 0x0A, 0x08
, 0x08, 0x01, 0x16, 0x16
, 0x17, 0x17, 0x07, 0x01
, 0x01};

void b080ean011_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	dsi_vc_dcs_write(intel_dsi, 0, auo_password, sizeof(auo_password));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence1, sizeof(auo_init_sequence1));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence2, sizeof(auo_init_sequence2));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence3, sizeof(auo_init_sequence3));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence4, sizeof(auo_init_sequence4));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence5, sizeof(auo_init_sequence5));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence6, sizeof(auo_init_sequence6));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence7, sizeof(auo_init_sequence7));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence8, sizeof(auo_init_sequence8));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence9, sizeof(auo_init_sequence9));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence10, sizeof(auo_init_sequence10));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence11, sizeof(auo_init_sequence11));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence12, sizeof(auo_init_sequence12));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence13, sizeof(auo_init_sequence13));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence14, sizeof(auo_init_sequence14));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence15, sizeof(auo_init_sequence15));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence16, sizeof(auo_init_sequence16));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence17, sizeof(auo_init_sequence17));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence18, sizeof(auo_init_sequence18));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence19, sizeof(auo_init_sequence19));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence20, sizeof(auo_init_sequence20));
	dsi_vc_dcs_write(intel_dsi, 0, auo_init_sequence21, sizeof(auo_init_sequence21));
}

static void  b080ean011_get_panel_info(int pipe,
					struct drm_connector *connector)
{
	DRM_DEBUG_KMS("\n");
	if (!connector) {
		DRM_DEBUG_KMS("Cpt: Invalid input to get_info\n");
		return;
	}

	if (pipe == 0) {
		connector->display_info.width_mm = 192;
		connector->display_info.height_mm = 120;
	}
	return;
}

static void b080ean011_destroy(struct intel_dsi_device *dsi)
{
}

static void b080ean011_dump_regs(struct intel_dsi_device *dsi)
{
}

static void b080ean011_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *b080ean011_get_modes(
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

	/* Hardcode 800*1280 */
	mode->hdisplay = 800;
	mode->hsync_start = mode->hdisplay + 24;
	mode->hsync_end = mode->hsync_start + 8;
	mode->htotal = mode->hsync_end  + 128;

	mode->vdisplay = 1280;
	mode->vsync_start = mode->vdisplay + 12;
	mode->vsync_end = mode->vsync_start + 8;
	mode->vtotal = mode->vsync_end + 12;

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

static bool b080ean011_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	return true;
}

static enum drm_connector_status b080ean011_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	dev_priv->is_mipi = true;
	DRM_DEBUG_KMS("\n");
	return connector_status_connected;
}

static bool b080ean011_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}
#define DISP_RST_N 107
void b080ean011_panel_reset(struct intel_dsi_device *dsi)
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

static int b080ean011_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("\n");
	return MODE_OK;
}

static void b080ean011_dpms(struct intel_dsi_device *dsi, bool enable)
{
	DRM_DEBUG_KMS("\n");
}
static void b080ean011_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");
	dsi_vc_dcs_write(intel_dsi, 0, auo_password, sizeof(auo_password));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(30);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	msleep(30);
	dsi_vc_dcs_write(intel_dsi, 0, auo_enable_ic_power, sizeof(auo_enable_ic_power));
}
static void b080ean011_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(30);
	dsi_vc_dcs_write(intel_dsi, 0, auo_disable_ic_power, sizeof(auo_disable_ic_power));
	msleep(15);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(30);
}

bool b080ean011_init(struct intel_dsi_device *dsi)
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

	DRM_DEBUG_KMS("Init: b080ean011 panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to b080ean011\n");
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
	/*dev_priv->mipi.panel_bpp = 24;*/
	return true;
}


/* Callbacks. We might not need them all. */
struct intel_dsi_dev_ops auo_b080ean011_dsi_display_ops = {
	.init = b080ean011_init,
	.get_info = b080ean011_get_panel_info,
	.create_resources = b080ean011_create_resources,
	.dpms = b080ean011_dpms,
	.mode_valid = b080ean011_mode_valid,
	.mode_fixup = b080ean011_mode_fixup,
	.panel_reset = b080ean011_panel_reset,
	.detect = b080ean011_detect,
	.get_hw_state = b080ean011_get_hw_state,
	.get_modes = b080ean011_get_modes,
	.destroy = b080ean011_destroy,
	.dump_regs = b080ean011_dump_regs,
	.enable = b080ean011_enable,
	.disable = b080ean011_disable,
	.send_otp_cmds = b080ean011_send_otp_cmds,
};

