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
#include "dsi_mod_mjk_m080wx01b.h"

static u8 boe_init_sequence_01[]      = {0xF0, 0x5A, 0x5A};
static u8 boe_init_sequence_02[]      = {0xF1, 0x5A, 0x5A};
static u8 boe_init_sequence_03[]      = {0xFC, 0xA5, 0xA5};
static u8 boe_init_sequence_04[]      = {0xB1, 0x10};
static u8 boe_init_sequence_05[]      = {0xB2, 0x14, 0x22, 0x2F, 0x04};
static u8 boe_init_sequence_06[]      = {0xF2, 0x02, 0x08, 0x08, 0x40, 0x10};
static u8 boe_init_sequence_07[]      = {0xB0, 0x03};
static u8 boe_init_sequence_08[]      = {0xFD, 0x23, 0x09};
static u8 boe_init_sequence_09[]      = {0xF3, 0x01, 0xd7, 0xe2, 0x62, 0xf4, 0xf7, 0x77, 0x3c, 0x26, 0x00};
static u8 boe_init_sequence_10[]      = {0xF4, 0x00, 0x02, 0x03, 0x26, 0x03, 0x02, 0x09, 0x00, 0x07, 0x16, 0x16, 0x03, 0x00, 0x08, 0x08, 0x03, 0x0E, 0x0F, 0x12, 0x1C, 0x1D, 0x1E, 0x0C, 0x09, 0x01, 0x04, 0x02, 0x61, 0x74, 0x75, 0x72, 0x83, 0x80, 0x80, 0xB0, 0x00, 0x01, 0x01, 0x28, 0x04, 0x03, 0x28, 0x01, 0xD1, 0x32};
static u8 boe_init_sequence_11[]      = {0xF5, 0x84, 0x2F, 0x2F, 0x5F, 0xAB, 0x98, 0x52, 0x0F, 0x33, 0x43, 0x04, 0x59, 0x54, 0x52, 0x05, 0x40, 0x60, 0x4E, 0x60, 0x40, 0x27, 0x26, 0x52, 0x25, 0x6D, 0x18};
static u8 boe_init_sequence_12[]			= {0xEE, 0x25, 0x00, 0x25, 0x00, 0x25, 0x00, 0x25, 0x00};
static u8 boe_init_sequence_13[]			= {0xEF, 0x34, 0x12, 0x98, 0xBA, 0x20, 0x00, 0x24, 0x80};
static u8 boe_init_sequence_14[]			= {0xF7, 0x0E, 0x0E, 0x0A, 0x0A, 0x0F, 0x0F, 0x0B, 0x0B, 0x05, 0x07, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x0C, 0x0C, 0x08, 0x08, 0x0D, 0x0D, 0x09, 0x09, 0x04, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
static u8 boe_init_sequence_15[]      = {0xBC, 0x01, 0x4E, 0x0A};
static u8 boe_init_sequence_16[]      = {0xE1, 0x03, 0x10, 0x1C, 0xA0, 0x10};
static u8 boe_init_sequence_17[]      = {0xF6, 0x60, 0x21, 0xA6, 0x00, 0x00, 0x00};
static u8 boe_init_sequence_18[]      = {0xFE, 0x00, 0x0D, 0x03, 0x21, 0x00, 0x48};
static u8 boe_init_sequence_19[]      = {0xB0, 0x22};
static u8 boe_init_sequence_20[]      = {0xFA, 0x02, 0x34, 0x09, 0x13, 0x0B, 0x0F, 0x16, 0x16, 0x17, 0x1E, 0x1D, 0x1C, 0x1E, 0x1D, 0x1D, 0x1F, 0x24};
static u8 boe_init_sequence_21[]      = {0xB0, 0x22};
static u8 boe_init_sequence_22[]      = {0xFB, 0x00, 0x34, 0x07, 0x11, 0x09, 0x0D, 0x14, 0x14, 0x15, 0x1C, 0x1F, 0x1C, 0x1D, 0x1D, 0x1D, 0x20, 0x26};
static u8 boe_init_sequence_23[]      = {0xB0, 0x11};
static u8 boe_init_sequence_24[]      = {0xFA, 0x20, 0x34, 0x24, 0x27, 0x19, 0x1B, 0x1F, 0x1E, 0x1B, 0x1F, 0x21, 0x1F, 0x1E, 0x20, 0x1E, 0x1E, 0x21};
static u8 boe_init_sequence_25[]      = {0xB0, 0x11};
static u8 boe_init_sequence_26[]      = {0xFB, 0x1E, 0x34, 0x22, 0x25, 0x17, 0x19, 0x1D, 0x1A, 0x19, 0x20, 0x1F, 0x1E, 0x20, 0x1E, 0x1E, 0x1F, 0x22};
static u8 boe_init_sequence_27[]      = {0xFA, 0x1C, 0x34, 0x1C, 0x1F, 0x13, 0x17, 0x1A, 0x18, 0x18, 0x1E, 0x20, 0x21, 0x21, 0x21, 0x23, 0x22, 0x2A};
static u8 boe_init_sequence_28[]      = {0xFB, 0x1A, 0x34, 0x1A, 0x1D, 0x11, 0x15, 0x18, 0x16, 0x16, 0x1C, 0x20, 0x20, 0x20, 0x1F, 0x23, 0x23, 0x2B};
static u8 boe_init_sequence_c3[]      = {0xC3, 0x40, 0x00, 0x28};

void m080wx01b_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_01, sizeof(boe_init_sequence_01));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_02, sizeof(boe_init_sequence_02));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_03, sizeof(boe_init_sequence_03));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_04, sizeof(boe_init_sequence_04));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_05, sizeof(boe_init_sequence_05));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_06, sizeof(boe_init_sequence_06));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_07, sizeof(boe_init_sequence_07));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_08, sizeof(boe_init_sequence_08));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_09, sizeof(boe_init_sequence_09));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_10, sizeof(boe_init_sequence_10));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_11, sizeof(boe_init_sequence_11));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_12, sizeof(boe_init_sequence_12));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_13, sizeof(boe_init_sequence_13));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_14, sizeof(boe_init_sequence_14));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_15, sizeof(boe_init_sequence_15));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_16, sizeof(boe_init_sequence_16));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_17, sizeof(boe_init_sequence_17));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_18, sizeof(boe_init_sequence_18));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_19, sizeof(boe_init_sequence_19));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_20, sizeof(boe_init_sequence_20));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_21, sizeof(boe_init_sequence_21));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_22, sizeof(boe_init_sequence_22));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_23, sizeof(boe_init_sequence_23));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_24, sizeof(boe_init_sequence_24));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_25, sizeof(boe_init_sequence_25));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_26, sizeof(boe_init_sequence_26));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_27, sizeof(boe_init_sequence_27));
	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_28, sizeof(boe_init_sequence_28));
}

static void  m080wx01b_get_panel_info(int pipe,
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

static void m080wx01b_destroy(struct intel_dsi_device *dsi)
{
}

static void m080wx01b_dump_regs(struct intel_dsi_device *dsi)
{
}

static void m080wx01b_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *m080wx01b_get_modes(
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
	/*HFP = 16, HSYNC = 5, HBP = 59 */
	/*VFP = 8, VSYNC = 5, VBP = 3 */
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

static bool m080wx01b_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	return true;
}

static enum drm_connector_status m080wx01b_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	DRM_DEBUG_KMS("\n");
	dev_priv->is_mipi = true;

	return connector_status_connected;
}

static bool m080wx01b_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}
#define DISP_RST_N 107
void m080wx01b_panel_reset(struct intel_dsi_device *dsi)
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

static int m080wx01b_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("\n");
	return MODE_OK;
}

static void m080wx01b_dpms(struct intel_dsi_device *dsi, bool enable)
{
	DRM_DEBUG_KMS("\n");
}

static void m080wx01b_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");

	dsi_vc_dcs_write(intel_dsi, 0, boe_init_sequence_c3, sizeof(boe_init_sequence_c3));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x35);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(5);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
}

static void m080wx01b_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(30);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(30);
}

bool m080wx01b_init(struct intel_dsi_device *dsi)
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

	DRM_DEBUG_KMS("Init: m080wx01b panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to m080wx01b\n");
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
struct intel_dsi_dev_ops mjk_m080wx01b_dsi_display_ops = {
	.init = m080wx01b_init,
	.get_info = m080wx01b_get_panel_info,
	.create_resources = m080wx01b_create_resources,
	.dpms = m080wx01b_dpms,
	.mode_valid = m080wx01b_mode_valid,
	.mode_fixup = m080wx01b_mode_fixup,
	.panel_reset = m080wx01b_panel_reset,
	.detect = m080wx01b_detect,
	.get_hw_state = m080wx01b_get_hw_state,
	.get_modes = m080wx01b_get_modes,
	.destroy = m080wx01b_destroy,
	.dump_regs = m080wx01b_dump_regs,
	.enable = m080wx01b_enable,
	.disable = m080wx01b_disable,
	.send_otp_cmds = m080wx01b_send_otp_cmds,
};
