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
#include "dsi_mod_rayken_rk785x32a1ci.h"
#include "linux/lnw_gpio.h"
#include "linux/gpio.h"

static void rk785x32a1ci_destroy(struct intel_dsi_device *dsi)
{
}

static void rk785x32a1ci_dump_regs(struct intel_dsi_device *dsi)
{
}

static void rk785x32a1ci_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *rk785x32a1ci_get_modes(
	struct intel_dsi_device *dsi)
{
	struct drm_display_mode *mode = NULL;
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	/* Allocate */
	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode) {
		DRM_DEBUG_KMS("Cpt panel: No memory\n");
		return NULL;
	}
	mode->hdisplay = 768;
	mode->hsync_start = 828;
	mode->hsync_end = 892;
	mode->htotal = 948;

	mode->vdisplay = 1024;
	mode->vsync_start = 1060;
	mode->vsync_end = 1110;
	mode->vtotal = 1140;

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


static void  rk785x32a1ci_get_panel_info(int pipe,
					struct drm_connector *connector)
{
	if (!connector) {
		DRM_DEBUG_KMS("RAYKEN: Invalid input to get_info\n");
		return;
	}

	if (pipe == 0) {
		connector->display_info.width_mm = 160;
		connector->display_info.height_mm = 120;
	}

	return;
}

static bool rk785x32a1ci_get_hw_state(struct intel_dsi_device *dev)
{
	return true;
}

static enum drm_connector_status rk785x32a1ci_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	dev_priv->is_mipi = true;

	return connector_status_connected;
}

static bool rk785x32a1ci_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}

void rk785x32a1ci_panel_reset(struct intel_dsi_device *dsi)
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

static int rk785x32a1ci_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	return MODE_OK;
}

static void rk785x32a1ci_dpms(struct intel_dsi_device *dsi, bool enable)
{
	DRM_DEBUG_KMS("\n");
}

static void rk785x32a1ci_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");

	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(10);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	msleep(100);
}
static void rk785x32a1ci_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(81);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(35);
}

bool rk785x32a1ci_init(struct intel_dsi_device *dsi)
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

	DRM_DEBUG_KMS("Init: RAYKEN rk785x32a1ci panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to rk785x32a1ci_init\n");
		return false;
	}
	intel_dsi->eotp_pkt = 1;
	/*intel_dsi->dsi_clock_freq = 513;*/
	intel_dsi->operation_mode = DSI_VIDEO_MODE;
	intel_dsi->video_mode_type = DSI_VIDEO_BURST;
	intel_dsi->pixel_format = VID_MODE_FORMAT_RGB666_LOOSE;
	intel_dsi->escape_clk_div = ESCAPE_CLOCK_DIVIDER_1;
	intel_dsi->lp_rx_timeout = 0xffff;
	intel_dsi->turn_arnd_val = 0x3f;
	intel_dsi->rst_timer_val = 0xff;
	intel_dsi->init_count = 0x7d0;
	intel_dsi->hs_to_lp_count = 0x46;
	intel_dsi->lp_byte_clk = 4;
	intel_dsi->bw_timer = 0;
	intel_dsi->clk_lp_to_hs_count = 0x24;
	intel_dsi->clk_hs_to_lp_count = 0x0F;
	intel_dsi->video_frmt_cfg_bits = DISABLE_VIDEO_BTA;
	intel_dsi->dphy_reg = 0x3F10430D;
	intel_dsi->backlight_off_delay = 20;
	intel_dsi->send_shutdown = true;
	intel_dsi->shutdown_pkt_delay = 20;
	intel_dsi->lane_count = 4;
	intel_dsi->port = 0; /* PORT_A by default */
	intel_dsi->burst_mode_ratio = 100;
	return true;
}

void rk785x32a1ci_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");

	intel_dsi->hs = 0;
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(100);

	intel_dsi->hs = 1;
}

void rk785x32a1ci_enable_panel_power(struct intel_dsi_device *dsi)
{
	DRM_DEBUG_KMS("\n");
}

void rk785x32a1ci_disable_panel_power(struct intel_dsi_device *dsi)
{
	DRM_DEBUG_KMS("\n");
}

/* Callbacks. We might not need them all. */
struct intel_dsi_dev_ops rayken_rk785x32a1ci_dsi_display_ops = {
	.init = rk785x32a1ci_init,
	.get_info = rk785x32a1ci_get_panel_info,
	.create_resources = rk785x32a1ci_create_resources,
	.dpms = rk785x32a1ci_dpms,
	.mode_valid = rk785x32a1ci_mode_valid,
	.mode_fixup = rk785x32a1ci_mode_fixup,
	.panel_reset = rk785x32a1ci_panel_reset,
	.detect = rk785x32a1ci_detect,
	.get_hw_state = rk785x32a1ci_get_hw_state,
	.get_modes = rk785x32a1ci_get_modes,
	.destroy = rk785x32a1ci_destroy,
	.dump_regs = rk785x32a1ci_dump_regs,
	.enable = rk785x32a1ci_enable,
	.disable = rk785x32a1ci_disable,
	.send_otp_cmds = rk785x32a1ci_send_otp_cmds,
	.disable_panel_power = rk785x32a1ci_disable_panel_power,
};
