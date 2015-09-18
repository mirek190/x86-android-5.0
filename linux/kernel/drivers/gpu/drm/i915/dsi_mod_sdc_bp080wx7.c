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
#include "dsi_mod_sdc_bp080wx7.h"

static u8 sdc_init_sequence_01[]      = {0xF0, 0x5A, 0x5A};
static u8 sdc_init_sequence_02[]      = {0xF1, 0x5A, 0x5A};
static u8 sdc_init_sequence_03[]      = {0xFC, 0xA5, 0xA5};
static u8 sdc_init_sequence_04[]      = {0xD0, 0x00, 0x10};
static u8 sdc_init_sequence_05[]      = {0x36, 0x04};
static u8 sdc_init_sequence_06[]      = {0xF6, 0x63, 0x20, 0x86, 0x00, 0x00, 0x10};
static u8 sdc_init_sequence_07[]      = {0x36, 0x00};
static u8 sdc_init_sequence_08[]      = {0xF0, 0xA5, 0xA5};
static u8 sdc_init_sequence_09[]      = {0xF1, 0xA5, 0xA5};
static u8 sdc_init_sequence_10[]      = {0xFC, 0x5A, 0x5A};

/* disable ic power */
static u8 sdc_init_sequence_c30[]      = {0xC3, 0x40, 0x00, 0x20};
/* enable ic power */
static u8 sdc_init_sequence_c31[]      = {0xC3, 0x40, 0x00, 0x28};

void sdc_bp080wx7_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	msleep(100);
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_01, sizeof(sdc_init_sequence_01));
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_02, sizeof(sdc_init_sequence_02));
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_03, sizeof(sdc_init_sequence_03));
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_04, sizeof(sdc_init_sequence_04));
	msleep(20);
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_c30, sizeof(sdc_init_sequence_c30));
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_c31, sizeof(sdc_init_sequence_c31));

	msleep(20);
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_05, sizeof(sdc_init_sequence_05));
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_06, sizeof(sdc_init_sequence_06));
}

static void sdc_bp080wx7_get_panel_info(int pipe,
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

static void sdc_bp080wx7_destroy(struct intel_dsi_device *dsi)
{
}

static void sdc_bp080wx7_dump_regs(struct intel_dsi_device *dsi)
{
}

static void sdc_bp080wx7_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *sdc_bp080wx7_get_modes(
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
	mode->hsync_start = mode->hdisplay + 16;
	mode->hsync_end = mode->hsync_start + 140;
	mode->htotal = mode->hsync_end  + 4;

	mode->vdisplay = 1280;
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

static bool sdc_bp080wx7_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	return true;
}

static enum drm_connector_status sdc_bp080wx7_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	DRM_DEBUG_KMS("\n");
	dev_priv->is_mipi = true;

	return connector_status_connected;
}

static bool sdc_bp080wx7_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}
#define DISP_RST_N 107
void sdc_bp080wx7_panel_reset(struct intel_dsi_device *dsi)
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

static int sdc_bp080wx7_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("\n");
	return MODE_OK;
}

static void sdc_bp080wx7_dpms(struct intel_dsi_device *dsi, bool enable)
{
	DRM_DEBUG_KMS("\n");
}

static void sdc_bp080wx7_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");

	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);

	msleep(120);
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_07, sizeof(sdc_init_sequence_07));
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_08, sizeof(sdc_init_sequence_08));
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_09, sizeof(sdc_init_sequence_09));
	dsi_vc_dcs_write(intel_dsi, 0, sdc_init_sequence_10, sizeof(sdc_init_sequence_10));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
}

static void sdc_bp080wx7_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(30);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(30);
}

bool sdc_bp080wx7_init(struct intel_dsi_device *dsi)
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

	DRM_DEBUG_KMS("Init: bp080wx7 panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to bp080wx7\n");
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
struct intel_dsi_dev_ops sdc_bp080wx7_dsi_display_ops = {
	.init = sdc_bp080wx7_init,
	.get_info = sdc_bp080wx7_get_panel_info,
	.create_resources = sdc_bp080wx7_create_resources,
	.dpms = sdc_bp080wx7_dpms,
	.mode_valid = sdc_bp080wx7_mode_valid,
	.mode_fixup = sdc_bp080wx7_mode_fixup,
	.panel_reset = sdc_bp080wx7_panel_reset,
	.detect = sdc_bp080wx7_detect,
	.get_hw_state = sdc_bp080wx7_get_hw_state,
	.get_modes = sdc_bp080wx7_get_modes,
	.destroy = sdc_bp080wx7_destroy,
	.dump_regs = sdc_bp080wx7_dump_regs,
	.enable = sdc_bp080wx7_enable,
	.disable = sdc_bp080wx7_disable,
	.send_otp_cmds = sdc_bp080wx7_send_otp_cmds,
};
