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
#include "dsi_mod_kd080d14.h"

//HX8394-C+BOE_8
static unsigned char init_sequence1[]={0xB9,0xFF,0x83,0x94};
//
static unsigned char init_sequence2[]={0xBA,0x23};
//set CABC control
static unsigned char init_sequence3[]={0xC9,0x1F,0x2E,0x1E,0x1E,0x10};
//set power
static unsigned char init_sequence4[]={0xB1,0x64,0x10,0x30,0x43,0x34,0x11,0xF1,0x81,0x70,0xD9,0x34,0x80,0xC0,0xD2,0x41};  
//set display
static unsigned char init_sequence5[]={0xB2,0x45,0x64,0x0F,0x09,0x40,0x1C,0x08,0x08,0x1C,0x4D,0x00,0x00};
//set CYC
static unsigned char init_sequence6[]={0xB4,0x00,0xFF,0x6F,0x60,0x6F,0x60,0x6F,0x60,0x01,0x6E,0x0F,0x6E,0x6F,0x60,0x6F,
                                0x60,0x6F,0x60,0x01,0x6E,0x0F,0x6E};
//set VCOM
static unsigned char init_sequence7[]={0xB6,0x6F,0x6F};
//set panel
static unsigned char init_sequence8[]={0xCC,0x09};
//set GIP_0
static unsigned char init_sequence9[]={0xD3,0x00,0x08,0x00,0x01,0x07, // 5 
                            	0x00,0x08,0x32,0x10,0x0A, // 10
                            	0x00,0x05,0x00,0x20,0x0A, // 15
                            	0x05,0x09,0x00,0x32,0x10, // 20
                            	0x08,0x00,0x35,0x33,0x0D, // 25
                            	0x07,0x47,0x0D,0x07,0x47,// 30
                            	0x0F,0x08}; 
//set GIP_1
static unsigned char init_sequence10[]={0xD5,0x03,0x02,0x03,0x02,0x01,0x00,0x01,0x00,0x07,0x06,0x07,0x06,0x05,0x04,0x05,
                                 0x04,0x21,0x20,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x23,
                                 0x22,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18};
//set gamma
static unsigned char init_sequence11[]={0xE0,0x03,0x17,0x1C,0x2D,0x30,0x3B,0x27,0x40,0x08,0x0B,0x0D,0x18,0x0F,0x12,0x15,
                                 0x13,0x14,0x07,0x12,0x14,0x17,0x03,0x17,0x1C,0x2D,0x30,0x3B,0x27,0x40,0x08,0x0B,
                                 0x0D,0x18,0x0F,0x12,0x15,0x13,0x14,0x07,0x12,0x14,0x17};
//set gamma
static unsigned char init_sequence12[]={0xC7,0x00,0x00,0x20};

static u8 cpt_enable_ic_power[]      = {0xC3, 0x40, 0x00, 0x28};
void kd080d14_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
    printk("--->%s\n",__func__);	
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence1, sizeof(init_sequence1));
	mdelay(10);
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence2, sizeof(init_sequence2));
	mdelay(10);
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence3, sizeof(init_sequence3));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence4, sizeof(init_sequence4));
	mdelay(10);
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence5, sizeof(init_sequence5));
	mdelay(10);
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence6, sizeof(init_sequence6));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence7, sizeof(init_sequence7));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence8, sizeof(init_sequence8));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence9, sizeof(init_sequence9));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence10, sizeof(init_sequence10));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence11, sizeof(init_sequence11));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence12, sizeof(init_sequence12));

}

static void  kd080d14_get_panel_info(int pipe,
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

static void kd080d14_destroy(struct intel_dsi_device *dsi)
{
}

static void kd080d14_dump_regs(struct intel_dsi_device *dsi)
{
}

static void kd080d14_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *kd080d14_get_modes(
	struct intel_dsi_device *dsi)
{
	struct drm_display_mode *mode = NULL;
	DRM_DEBUG_KMS("\n");
	/* Allocate */
	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode) {
		DRM_DEBUG_KMS("Cpt panel: No memory\n");
		return NULL;
	}
	mode->hdisplay = 800;
	mode->vdisplay = 1280;

	mode->hsync_start = mode->hdisplay + 20;
	mode->hsync_end = mode->hsync_start + 5;
	mode->htotal = mode->hsync_end + 20;

	mode->vsync_start = mode->vdisplay + 20;
	mode->vsync_end = mode->vsync_start + 5;
	mode->vtotal = mode->vsync_end + 20;
	mode->vrefresh = 60; 
	mode->clock =  mode->vrefresh * mode->vtotal *mode->htotal / 1000;
	mode->type |= DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_set_crtcinfo(mode, 0);

	return mode;
}


static bool kd080d14_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	return true;
}

static enum drm_connector_status kd080d14_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
    printk("--->%s\n",__func__);

	DRM_DEBUG_KMS("\n");
	dev_priv->is_mipi = true;

	return connector_status_connected;
}

static bool kd080d14_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}

void kd080d14_panel_reset(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	DRM_DEBUG_KMS("\n");
    printk("--->%s\n",__func__);

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
	msleep(50);
}

static int kd080d14_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("\n");
	return MODE_OK;
}

static void kd080d14_dpms(struct intel_dsi_device *dsi, bool enable)
{
//	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
}

static void kd080d14_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");
	printk("--->%s\n",__func__);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(300);  //wait for 30ms
	dsi_vc_dcs_write(intel_dsi, 0, cpt_enable_ic_power, sizeof(cpt_enable_ic_power));
	msleep(150);  //wait for 150ms
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	msleep(50);
}
static void kd080d14_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	DRM_DEBUG_KMS("\n");
	printk("--->%s\n",__func__);
	msleep(200);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(80);
	//dsi_vc_dcs_write(intel_dsi, 0, cpt_disable_ic_power, sizeof(cpt_disable_ic_power));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(34);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PCONF0, 0x2000CC00);	
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PAD, 0x00000004);	
	usleep_range(100000, 120000);	
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PCONF0, 0x2000CC00);	
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PAD, 0x00000004);
}

bool kd080d14_init(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
//	struct drm_device *dev = intel_dsi->base.base.dev;
//	struct drm_i915_private *dev_priv = dev->dev_private;

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
    printk("--->%s\n",__func__);
	DRM_DEBUG_KMS("Init: kd080d14 panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to kd080d14\n");
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

	intel_dsi->backlight_off_delay = 20;
	intel_dsi->send_shutdown = true;
	intel_dsi->shutdown_pkt_delay = 20;
	//dev_priv->mipi.panel_bpp = 24;

	return true;
}


/* Callbacks. We might not need them all. */
struct intel_dsi_dev_ops kd_kd080d14_dsi_display_ops = {
	.init = kd080d14_init,
	.get_info = kd080d14_get_panel_info,
	.create_resources = kd080d14_create_resources,
	.dpms = kd080d14_dpms,
	.mode_valid = kd080d14_mode_valid,
	.mode_fixup = kd080d14_mode_fixup,
	.panel_reset = kd080d14_panel_reset,
	.detect = kd080d14_detect,
	.get_hw_state = kd080d14_get_hw_state,
	.get_modes = kd080d14_get_modes,
	.destroy = kd080d14_destroy,
	.dump_regs = kd080d14_dump_regs,
	.enable = kd080d14_enable,
	.disable = kd080d14_disable,
	.send_otp_cmds = kd080d14_send_otp_cmds,

};
