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
#include "dsi_mod_ps_lsl080al02.h"

//
static unsigned char init_sequence1[]={0xF0,0x5A,0x5A};
//
static unsigned char init_sequence2[]={0xF1,0x5A,0x5A};
//
static unsigned char init_sequence3[]={0xFC,0xA5,0xA5};
//
static unsigned char init_sequence4[]={0xD0,0x00,0x10};  
//
static unsigned char init_sequence5[]={0xC3,0x40,0x00,0x28};
//DELAY 20MS
static unsigned char init_sequence6[]={0x36,0x04};
//
static unsigned char init_sequence7[]={0xF6,0x63,0x20,0x86,0x00,0x00,0x10};
//
//static unsigned char init_sequence8[]={0x11,0x00};
//MDEALY 120MS
static unsigned char init_sequence9[]={0x36,0x00};
//set 
static unsigned char init_sequence10[]={0xF0,0xA5,0xA5}; 
//set 
static unsigned char init_sequence11[]={0xF1,0xA5,0xA5};
//set 
static unsigned char init_sequence12[]={0xFC,0x5A,0x5A};
//
//static unsigned char init_sequence13[]={0x29,0x00};
//MDEALY 200MS
//BACKLIGHT ON

//***********     SLEEP OUT      ***************///
static u8 ps_enable_ic_power1[]      = {0xF0, 0x5A, 0x5A};
static u8 ps_enable_ic_power2[]      = {0xF1, 0x5A, 0x5A};
static u8 ps_enable_ic_power3[]      = {0xC3, 0x40, 0x00, 0x28};
//DELAY 20MS
//static u8 ps_enable_ic_power4[]      = {0x11};
//DELAY 120MS
static u8 ps_enable_ic_power5[]      = {0xF0, 0xA5, 0xA5};
static u8 ps_enable_ic_power6[]      = {0xF1, 0xA5, 0xA5};
//static u8 ps_enable_ic_power7[]      = {0x29};
//DEALY 200MS


//***********     SLEEP IN      ***************///
//backligth turn off  
//delay 200ms
//static u8 ps_disable_ic_power1[]      = {0x28};

static u8 ps_disable_ic_power2[]      = {0xF0, 0x5A, 0x5A};
static u8 ps_disable_ic_power3[]      = {0xF1, 0x5A, 0x5A};
static u8 ps_disable_ic_power4[]      = {0xC3, 0x40, 0x00, 0x20};

//static u8 ps_disable_ic_power5[]      = {0x10};
//delay 34ms
#if 0
//***********     power off     ***************///
//backligth turn off  
//delay 200ms
static u8 ps_poweroff_ic_power1[]      = {0x28};
//delay 80ms
static u8 ps_poweroff_ic_power2[]      = {0xF0, 0x5A, 0x5A};
static u8 ps_poweroff_ic_power3[]      = {0xF1, 0x5A, 0x5A};
static u8 ps_poweroff_ic_power4[]      = {0xC3, 0x40, 0x00, 0x20};
//delay 20ms
static u8 ps_poweroff_ic_power5[]      = {0x10};
//delay 34ms
#endif
void lsl080al02_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
    printk("--->%s\n",__func__);	
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence1, sizeof(init_sequence1));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence2, sizeof(init_sequence2));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence3, sizeof(init_sequence3));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence4, sizeof(init_sequence4));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence5, sizeof(init_sequence5));
	mdelay(50);
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence6, sizeof(init_sequence6));
	
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence7, sizeof(init_sequence7));
	//dsi_vc_dcs_write(intel_dsi, 0, init_sequence8, sizeof(init_sequence8));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	mdelay(150);
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence9, sizeof(init_sequence9));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence10, sizeof(init_sequence10));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence11, sizeof(init_sequence11));
	dsi_vc_dcs_write(intel_dsi, 0, init_sequence12, sizeof(init_sequence12));
	mdelay(10);
	//dsi_vc_dcs_write(intel_dsi, 0, init_sequence13, sizeof(init_sequence13));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	mdelay(100);

}

static void  lsl080al02_get_panel_info(int pipe,
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

static void lsl080al02_destroy(struct intel_dsi_device *dsi)
{
}

static void lsl080al02_dump_regs(struct intel_dsi_device *dsi)
{
}

static void lsl080al02_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *lsl080al02_get_modes(
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

	mode->hsync_start = mode->hdisplay + 16;    //hfp
	mode->hsync_end = mode->hsync_start + 14;	//hsync
	mode->htotal = mode->hsync_end + 140;		//hbp

	mode->vsync_start = mode->vdisplay + 8;		//vfp
	mode->vsync_end = mode->vsync_start + 4;	//vsync
	mode->vtotal = mode->vsync_end + 4;			//vbp
	mode->vrefresh = 70; 
	mode->clock =  mode->vrefresh * mode->vtotal *mode->htotal / 1000;
	mode->type |= DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_set_crtcinfo(mode, 0);

	return mode;
}


static bool lsl080al02_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	return true;
}

static enum drm_connector_status lsl080al02_detect(
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

static bool lsl080al02_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}

void lsl080al02_panel_reset(struct intel_dsi_device *dsi)
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

static int lsl080al02_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("\n");
	return MODE_OK;
}

static void lsl080al02_dpms(struct intel_dsi_device *dsi, bool enable)
{
//	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
}

static void lsl080al02_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");
	printk("--->%s\n",__func__);
	
	dsi_vc_dcs_write(intel_dsi, 0, ps_enable_ic_power1, sizeof(ps_enable_ic_power1));
	dsi_vc_dcs_write(intel_dsi, 0, ps_enable_ic_power2, sizeof(ps_enable_ic_power2));
	dsi_vc_dcs_write(intel_dsi, 0, ps_enable_ic_power3, sizeof(ps_enable_ic_power3));
	msleep(50);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(200);  //wait for 120ms
	//dsi_vc_dcs_write(intel_dsi, 0, ps_enable_ic_power4, sizeof(ps_enable_ic_power4));
	dsi_vc_dcs_write(intel_dsi, 0, ps_enable_ic_power5, sizeof(ps_enable_ic_power5));
	dsi_vc_dcs_write(intel_dsi, 0, ps_enable_ic_power6, sizeof(ps_enable_ic_power6));
	//dsi_vc_dcs_write(intel_dsi, 0, ps_enable_ic_power7, sizeof(ps_enable_ic_power7));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);


	msleep(200);
}
static void lsl080al02_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	printk("--->%s\n",__func__);
	msleep(200);
	//dsi_vc_dcs_write(intel_dsi, 0, ps_disable_ic_power1, sizeof(ps_disable_ic_power1));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(100);
	dsi_vc_dcs_write(intel_dsi, 0, ps_disable_ic_power2, sizeof(ps_disable_ic_power2));
	dsi_vc_dcs_write(intel_dsi, 0, ps_disable_ic_power3, sizeof(ps_disable_ic_power3));
	dsi_vc_dcs_write(intel_dsi, 0, ps_disable_ic_power4, sizeof(ps_disable_ic_power4));
	//dsi_vc_dcs_write(intel_dsi, 0, ps_disable_ic_power5, sizeof(ps_disable_ic_power5));	
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(80);

}

bool lsl080al02_init(struct intel_dsi_device *dsi)
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
	DRM_DEBUG_KMS("Init: lsl080al02 panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to lsl080al02\n");
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
	intel_dsi->hs_to_lp_count = 0x19;
	/*b060*/
	intel_dsi->lp_byte_clk = 0x03;
	/*b080*/
	intel_dsi->dphy_reg = 0x190E390C;
	/* b088 high 16bits */
	intel_dsi->clk_lp_to_hs_count = 0x20;
	/* b088 low 16bits */
	intel_dsi->clk_hs_to_lp_count = 0x0E;
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
struct intel_dsi_dev_ops kd_lsl080al02_dsi_display_ops = {
	.init = lsl080al02_init,
	.get_info = lsl080al02_get_panel_info,
	.create_resources = lsl080al02_create_resources,
	.dpms = lsl080al02_dpms,
	.mode_valid = lsl080al02_mode_valid,
	.mode_fixup = lsl080al02_mode_fixup,
	.panel_reset = lsl080al02_panel_reset,
	.detect = lsl080al02_detect,
	.get_hw_state = lsl080al02_get_hw_state,
	.get_modes = lsl080al02_get_modes,
	.destroy = lsl080al02_destroy,
	.dump_regs = lsl080al02_dump_regs,
	.enable = lsl080al02_enable,
	.disable = lsl080al02_disable,
	.send_otp_cmds = lsl080al02_send_otp_cmds,

};
