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
#include "dsi_mod_p088pw.h"
#include "linux/mfd/intel_mid_pmic.h"
typedef struct {
    unsigned char count;
    unsigned char para_list[100];
} LCM_setting_table;

static LCM_setting_table init_sequence[] =
{

	{1, {0x11}},					// Normal Display
	{1, {0x29}},
};

//static u8 cpt_enable_ic_power[]      = {0xC3, 0x40, 0x00, 0x28};
//static u8 cpt_disable_ic_power[]      = {0xC3, 0x40, 0x00, 0x20};

void p088pw_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	int sequence_line, i;
	DRM_DEBUG_KMS(" p088pw_send_otp_cmds \n");
#if 1
	sequence_line = sizeof(init_sequence) / sizeof(init_sequence[0]);
	for(i = 0; i < sequence_line; i++)
	{
		dsi_vc_dcs_write(intel_dsi, 0, &init_sequence[i].para_list[0], init_sequence[i].count);
		if(init_sequence[i].para_list[0] == 0x11)
			msleep(120);
		if(init_sequence[i].para_list[0] == 0x29)
			msleep(20);
	}
#endif
}

static void p088pw_get_panel_info(int pipe,
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

static void p088pw_destroy(struct intel_dsi_device *dsi)
{
	
}

static void p088pw_dump_regs(struct intel_dsi_device *dsi)
{
	
}

static void p088pw_create_resources(struct intel_dsi_device *dsi)
{
	
}

static struct drm_display_mode *p088pw_get_modes(
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
	mode->hdisplay = 1920;
	mode->vdisplay = 1200;

	mode->hsync_start = mode->hdisplay + 20; //HFP
	mode->hsync_end = mode->hsync_start + 16; //HSW
	mode->htotal = mode->hsync_end + 15; //HBP

	mode->vsync_start = mode->vdisplay + 15; //VFP
	mode->vsync_end = mode->vsync_start + 2;//VSW
	mode->vtotal = mode->vsync_end + 3;//VBP
	mode->vrefresh = 60; 
	mode->clock =  mode->vrefresh * mode->vtotal *mode->htotal / 1000;//VCLK
	mode->type |= DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_set_crtcinfo(mode, 0);

	return mode;
}


static bool p088pw_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	
	return true;
}

static enum drm_connector_status p088pw_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
 	dev_priv->is_mipi = true;
	
	DRM_DEBUG_KMS("\n");
	return connector_status_connected;
}

static bool p088pw_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	
	return true;
}

void p088pw_reset(struct intel_dsi_device *dsi)
{
    int val;
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	DRM_DEBUG_KMS("\n");
	//printk("p088pw_reset\n");
	val = intel_mid_pmic_readb(0x12);
    val |= 1<<5;
	intel_mid_pmic_writeb(0x12, val);
    usleep_range(100000, 120000);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PCONF0, 0x2000CC00);//reset
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PAD, 0x00000004);
	usleep_range(100000, 120000);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_10_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_10_PAD, 0x00000005);
	usleep_range(100000, 120000);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PAD, 0x00000005);
	msleep(10);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PCONF0, 0x2000CC00);
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PAD, 0x00000005);
}

static int p088pw_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
    DRM_DEBUG_KMS("\n");
	//printk("p088pw_mode_valid\n");
	return MODE_OK;
}

static void p088pw_dpms(struct intel_dsi_device *dsi, bool enable)
{
//	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	//printk("p088pw_dpms\n");
	DRM_DEBUG_KMS("\n");
}
static void p088pw_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");
	//printk("p088pw_enable\n");
	
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	
	msleep(120);  
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	msleep(20);
}
static void p088pw_disable(struct intel_dsi_device *dsi)
{
    int val = 0;
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	DRM_DEBUG_KMS("\n");
	printk("p088pw_disable\n");
	msleep(200);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(80);

	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(34);
	val = intel_mid_pmic_readb(0x12);    	
	val &= ~(1<<5);		
	intel_mid_pmic_writeb(0x12, val);		
	usleep_range(100000, 120000);	
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PCONF0, 0x2000CC00);	
	vlv_gpio_nc_write(dev_priv, GPIO_NC_9_PAD, 0x00000004);	
	usleep_range(100000, 120000);	
	vlv_gpio_nc_write(dev_priv, GPIO_NC_10_PCONF0, 0x2000CC00);	
	vlv_gpio_nc_write(dev_priv, GPIO_NC_10_PAD, 0x00000004);	
	usleep_range(100000, 120000);	
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PCONF0, 0x2000CC00);	
	vlv_gpio_nc_write(dev_priv, GPIO_NC_11_PAD, 0x00000004);
}

bool p088pw_init(struct intel_dsi_device *dsi)
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

	DRM_DEBUG_KMS("Init: p088pw panel\n");
	//printk("p088pw_init\n");
	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to p088pw\n");
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
	intel_dsi->hs_to_lp_count = 0x2C;
	/*b060*/
	intel_dsi->lp_byte_clk = 0x06;
	/*b080*/
	intel_dsi->dphy_reg = 0x301A6D16;
	/* b088 high 16bits */
	intel_dsi->clk_lp_to_hs_count = 0x3A;
	/* b088 low 16bits */
	intel_dsi->clk_hs_to_lp_count = 0x16;
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
struct intel_dsi_dev_ops p088pw_dsi_display_ops = {
	.init = p088pw_init,
	.get_info = p088pw_get_panel_info,
	.create_resources = p088pw_create_resources,
	.dpms = p088pw_dpms,
	.mode_valid = p088pw_mode_valid,
	.mode_fixup = p088pw_mode_fixup,
	.panel_reset = p088pw_reset,
	.detect = p088pw_detect,
	.get_hw_state = p088pw_get_hw_state,
	.get_modes = p088pw_get_modes,
	.destroy = p088pw_destroy,
	.dump_regs = p088pw_dump_regs,
	.enable = p088pw_enable,
	.disable = p088pw_disable,
	.send_otp_cmds = p088pw_send_otp_cmds,
};
