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
#include "dsi_mod_ivo_kr079ia1t.h"
#include "linux/lnw_gpio.h"
#include "linux/gpio.h"

static void kr079ia1t_destroy(struct intel_dsi_device *dsi)
{
}

static void kr079ia1t_dump_regs(struct intel_dsi_device *dsi)
{
}

static void kr079ia1t_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *kr079ia1t_get_modes(
	struct intel_dsi_device *dsi)
{
	struct drm_display_mode *mode = NULL;
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	/* Allocate */
	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode) {
		DRM_DEBUG_KMS("Ivo panel: No memory\n");
		return NULL;
	}
	mode->hdisplay = 768;
	mode->hsync_start = mode->hdisplay + 80;	/*mode->hdisplay + HFP;*/
	mode->hsync_end = mode->hsync_start + 21;		/*mode->hsync_start + HSYNC;*/
	mode->htotal = mode->hsync_end + 59;				/*mode->hsync_end + HBP;*/

	mode->vdisplay = 1024;
	mode->vsync_start = mode->vdisplay + 18;	/*mode->vdisplay + VFP;*/
	mode->vsync_end = mode->vsync_start + 4;		/*mode->vsync_start + VSYNC;*/
	mode->vtotal = mode->vsync_end + 19;			/*mode->vsync_end + VBP;*/

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


static void  kr079ia1t_get_panel_info(int pipe,
					struct drm_connector *connector)
{
	if (!connector) {
		DRM_DEBUG_KMS("RAYKEN: Invalid input to get_info\n");
		return;
	}

	if (pipe == 0) {
	connector->display_info.width_mm = 120;
	connector->display_info.height_mm = 160;
	}

	return;
}

static bool kr079ia1t_get_hw_state(struct intel_dsi_device *dev)
{
	return true;
}

static enum drm_connector_status kr079ia1t_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	dev_priv->is_mipi = true;

	return connector_status_connected;
}

static bool kr079ia1t_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}

void kr079ia1t_panel_reset(struct intel_dsi_device *dsi)
{
#if 1
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	DRM_DEBUG_KMS("\n");

	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PAD, 0x00000004);

	/* panel disable */
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PAD, 0x00000004);
	usleep_range(10000, 120000);

	/* panel enable */
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PAD, 0x00000005);
	usleep_range(100000, 120000);

	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PAD, 0x00000005);
#endif
}

static int kr079ia1t_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	return MODE_OK;
}

static void kr079ia1t_dpms(struct intel_dsi_device *dsi, bool enable)
{
#if 0
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	if (enable) {
		dsi_vc_dcs_write_0(intel_dsi, 0, MIPI_DCS_EXIT_SLEEP_MODE);

		dsi_vc_dcs_write_1(intel_dsi, 0, MIPI_DCS_SET_TEAR_ON, 0x00);

		dsi_vc_dcs_write_0(intel_dsi, 0, MIPI_DCS_SET_DISPLAY_ON);
		dsi_vc_dcs_write_1(intel_dsi, 0, 0x14, 0x55);

	} else {
		dsi_vc_dcs_write_0(intel_dsi, 0, MIPI_DCS_SET_DISPLAY_OFF);
		dsi_vc_dcs_write_0(intel_dsi, 0, MIPI_DCS_ENTER_SLEEP_MODE);
	}
#endif
}
static void kr079ia1t_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");

	intel_dsi->hs = 0;
	msleep(10);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);

	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	msleep(100);

	intel_dsi->hs = 1;

}
static void kr079ia1t_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");

	intel_dsi->hs = 0;

	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(100);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(40);

	intel_dsi->hs = 1;

}

bool kr079ia1t_init(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
/*	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int err = 0; */
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

	DRM_DEBUG_KMS("Init: kr079ia1t panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to kr079ia1t_init\n");
		return false;
	}

	intel_dsi->dsi_clock_freq = 300;
	intel_dsi->eotp_pkt = 1;
	intel_dsi->operation_mode = DSI_VIDEO_MODE;
	intel_dsi->video_mode_type = DSI_VIDEO_NBURST_SEVENT;
	intel_dsi->pixel_format = VID_MODE_FORMAT_RGB888;
	intel_dsi->port_bits = 0;
	intel_dsi->turn_arnd_val = 0x14;
	intel_dsi->rst_timer_val = 0xffff;
	intel_dsi->bw_timer = 0x820;
	/*b044*/
	intel_dsi->hs_to_lp_count = 0x15;
	/*b060*/
	intel_dsi->lp_byte_clk = 0x03;
	/*b080*/
	intel_dsi->dphy_reg = 0x140b2d09;
	/* b088 high 16bits */
	intel_dsi->clk_lp_to_hs_count = 0x1a;
	/* b088 low 16bits */
	intel_dsi->clk_hs_to_lp_count = 0x0c;
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

void kr079ia1t_send_otp_cmds(struct intel_dsi_device *dsi)
{
	DRM_DEBUG_KMS("\n");
}

void kr079ia1t_enable_panel_power(struct intel_dsi_device *dsi)
{
/*	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
*/
	DRM_DEBUG_KMS("\n");
}

void kr079ia1t_disable_panel_power(struct intel_dsi_device *dsi)
{
/*	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
*/
	DRM_DEBUG_KMS("\n");
}

/* Callbacks. We might not need them all. */
struct intel_dsi_dev_ops ivo_kr079ia1t_dsi_display_ops = {
	.init = kr079ia1t_init,
	.get_info = kr079ia1t_get_panel_info,
	.create_resources = kr079ia1t_create_resources,
	.dpms = kr079ia1t_dpms,
	.mode_valid = kr079ia1t_mode_valid,
	.mode_fixup = kr079ia1t_mode_fixup,
	.panel_reset = kr079ia1t_panel_reset,
	.detect = kr079ia1t_detect,
	.get_hw_state = kr079ia1t_get_hw_state,
	.get_modes = kr079ia1t_get_modes,
	.destroy = kr079ia1t_destroy,
	.dump_regs = kr079ia1t_dump_regs,
	.enable = kr079ia1t_enable,
	.disable = kr079ia1t_disable,
	.send_otp_cmds = kr079ia1t_send_otp_cmds,
	.disable_panel_power = kr079ia1t_disable_panel_power,
};
