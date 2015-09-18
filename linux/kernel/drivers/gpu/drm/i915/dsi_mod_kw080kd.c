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
#include "dsi_mod_kw080kd.h"
#include "linux/mfd/intel_mid_pmic.h"
typedef struct {
    unsigned char count;
    unsigned char para_list[100];
} LCM_setting_table;

static LCM_setting_table init_sequence[] =
{
#if 0
	{5, {0xFF,0xAA,0x55,0xA5,0x80}},		// MIPI related Timing Setting
	{3, {0x6F,0x11,0x00}},

	{2, {0x6F,0x06}},					//  Improve ESD option
	{2, {0xF7,0xA0}},
	{2, {0x6F,0x19}},
	{2, {0xF7,0x12}},
	
#if 0
	{2, {0x6F,0x08}},					// Vcom floating
	{2, {0xFA,0x40}},
	{2, {0x6F,0x11}},
	{2, {0xF3,0x01}},
#endif

	{6, {0xF0,0x55,0xAA,0x52,0x08,0x00}},//========== page0 relative ==========
	{2, {0xC8, 0x80}},

	{3, {0xB1,0x6C,0x01}},				// Set WXGA resolution
	{2, {0xB6,0x08}},					// Set source output hold time

	{2, {0x6F,0x02}},					//EQ control function
	{2, {0xB8,0x08}},

	{3, {0xBB,0x54,0x54}},				// Set bias current for GOP and SOP
	{3, {0xBC,0x05,0x05}},				// Inversion setting 
	{2, {0xC7,0x01}},					// zigzag setting
	{6, {0xBD,0x02,0xB0,0x0C,0x0A,0x00}},// DSP Timing Settings update for BIST
	{6, {0xF0,0x55,0xAA,0x52,0x08,0x01}},//========== page1 relative ==========

	{3, {0xB0,0x05,0x05}},				// Setting AVDD, AVEE clamp
	{3, {0xB1,0x05,0x05}},

	{3, {0xBC,0x3A,0x01}},				// VGMP, VGMN, VGSP, VGSN setting
	{3, {0xBD,0x3E,0x01}},

	{2, {0xCA,0x00}},					// gate signal control
	{2, {0xC0,0x04}},					// power IC control
	{3, {0xB2,0x00,0x00}},				// VCL SET -2.5V
	{2, {0xBE,0x80}},					// VCOM = -1.888V

	{3, {0xB3,0x19,0x19}},				// Setting VGH=15V, VGL=-11V
	{3, {0xB4,0x12,0x12}},

	{3, {0xB9,0x24,0x24}},				// power control for VGH, VGL
	{3, {0xBA,0x14,0x14}},

	{6, {0xF0,0x55,0xAA,0x52,0x08,0x02}},//========== page2 relative ==========
	{2, {0xEE,0x01}},					//gamma setting
	{5, {0xEF,0x09,0x06,0x15,0x18}},		//Gradient Control for Gamma Voltage

	{7, {0xB0,0x00,0x00,0x00,0x08,0x00,0x17}},
	{2, {0x6F,0x06}},
	{7, {0xB0,0x00,0x25,0x00,0x30,0x00,0x45}},
	{2, {0x6F,0x0C}},
	{5, {0xB0,0x00,0x56,0x00,0x7A}},
	{7, {0xB1,0x00,0xA3,0x00,0xE7,0x01,0x20}},
	{2, {0x6F,0x06}},
	{7, {0xB1,0x01,0x7A,0x01,0xC2,0x01,0xC5}},
	{2, {0x6F,0x0C}},
	{5, {0xB1,0x02,0x06,0x02,0x5F}},
	{7, {0xB2,0x02,0x92,0x02,0xD0,0x02,0xFC}},
	{2, {0x6F,0x06}},
	{7, {0xB2,0x03,0x35,0x03,0x5D,0x03,0x8B}},
	{2, {0x6F,0x0C}},
	{5, {0xB2,0x03,0xA2,0x03,0xBF}},
	{5, {0xB3,0x03,0xD2,0x03,0xFF}},

	{6, {0xF0, 0x55,0xAA,0x52,0x08,0x06}},		// PAGE6 : GOUT Mapping, VGLO select
	{3, {0xB0, 0x00,0x17}},
	{3, {0xB1, 0x16,0x15}},
	{3, {0xB2, 0x14,0x13}},
	{3, {0xB3, 0x12,0x11}},
	{3, {0xB4, 0x10,0x2D}},
	{3, {0xB5, 0x01,0x08}},
	{3, {0xB6, 0x09,0x31}},
	{3, {0xB7, 0x31,0x31}},
	{3, {0xB8, 0x31,0x31}},
	{3, {0xB9, 0x31,0x31}},
	{3, {0xBA, 0x31,0x31}},
	{3, {0xBB, 0x31,0x31}},
	{3, {0xBC, 0x31,0x31}},
	{3, {0xBD, 0x31,0x09}},
	{3, {0xBE, 0x08,0x01}},
	{3, {0xBF, 0x2D,0x10}},
	{3, {0xC0, 0x11,0x12}},
	{3, {0xC1, 0x13,0x14}},
	{3, {0xC2, 0x15,0x16}},
	{3, {0xC3, 0x17,0x00}},
	{3, {0xE5, 0x31,0x31}},
	{3, {0xC4, 0x00,0x17}},
	{3, {0xC5, 0x16,0x15}},
	{3, {0xC6, 0x14,0x13}},
	{3, {0xC7, 0x12,0x11}},
	{3, {0xC8, 0x10,0x2D}},
	{3, {0xC9, 0x01,0x08}},
	{3, {0xCA, 0x09,0x31}},
	{3, {0xCB, 0x31,0x31}},
	{3, {0xCC, 0x31,0x31}},
	{3, {0xCD, 0x31,0x31}},
	{3, {0xCE, 0x31,0x31}},
	{3, {0xCF, 0x31,0x31}},
	{3, {0xD0, 0x31,0x31}},
	{3, {0xD1, 0x31,0x09}},
	{3, {0xD2, 0x08,0x01}},
	{3, {0xD3, 0x2D,0x10}},
	{3, {0xD4, 0x11,0x12}},
	{3, {0xD5, 0x13,0x14}},
	{3, {0xD6, 0x15,0x16}},
	{3, {0xD7, 0x17,0x00}},
	{3, {0xE6, 0x31,0x31}},

	{6, {0xD8, 0x00,0x00,0x00,0x00,0x00}},		//VGL level select
	{6, {0xD9, 0x00,0x00,0x00,0x00,0x00}},
	{2, {0xE7, 0x00}},

	{6, {0xF0, 0x55,0xAA,0x52,0x08,0x03}},		//gate timing control
	{3, {0xB0, 0x20,0x00}},
	{3, {0xB1, 0x20,0x00}},
	{6, {0xB2, 0x05,0x00,0x42,0x00,0x00}},
	{6, {0xB6, 0x05,0x00,0x42,0x00,0x00}},
	{6, {0xBA, 0x53,0x00,0x42,0x00,0x00}},
	{6, {0xBB, 0x53,0x00,0x42,0x00,0x00}},
	{2, {0xC4, 0x40}},

	{6, {0xF0, 0x55,0xAA,0x52,0x08,0x05}},		// PAGE5 :
	{3, {0xB0, 0x17,0x06}},
	{2, {0xB8, 0x00}},
	{6, {0xBD, 0x03,0x01,0x01,0x00,0x01}},
	{3, {0xB1, 0x17,0x06}},
	{3, {0xB9, 0x00,0x01}},
	{3, {0xB2, 0x17,0x06}},
	{3, {0xBA, 0x00,0x01}},
	{3, {0xB3, 0x17,0x06}},
	{3, {0xBB, 0x0A,0x00}},
	{3, {0xB4, 0x17,0x06}},
	{3, {0xB5, 0x17,0x06}},
	{3, {0xB6, 0x14,0x03}},
	{3, {0xB7, 0x00,0x00}},
	{3, {0xBC, 0x02,0x01}},
	{2, {0xC0, 0x05}},
	{2, {0xC4, 0xA5}},
	{3, {0xC8, 0x03,0x30}},
	{3, {0xC9, 0x03,0x51}},
	{6, {0xD1, 0x00,0x05,0x03,0x00,0x00}},
	{6, {0xD2, 0x00,0x05,0x09,0x00,0x00}},
	{2, {0xE5, 0x02}},
	{2, {0xE6, 0x02}},
	{2, {0xE7, 0x02}},
	{2, {0xE9, 0x02}},
	{2, {0xED, 0x33}},
#endif
	{1, {0x11}},					// Normal Display
	{1, {0x29}},
};

//static u8 cpt_enable_ic_power[]      = {0xC3, 0x40, 0x00, 0x28};
//static u8 cpt_disable_ic_power[]      = {0xC3, 0x40, 0x00, 0x20};

void kw080kd_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	int sequence_line, i;
	DRM_DEBUG_KMS("\n");
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

static void kw080kd_get_panel_info(int pipe,
					struct drm_connector *connector)
{
	DRM_DEBUG_KMS("\n");
	printk("kw080kd_get_panel_info\n");
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

static void kw080kd_destroy(struct intel_dsi_device *dsi)
{
	printk("kw080kd_destroy\n");
}

static void kw080kd_dump_regs(struct intel_dsi_device *dsi)
{
	printk("kw080kd_dump_regs\n");
}

static void kw080kd_create_resources(struct intel_dsi_device *dsi)
{
	printk("kw080kd_create_resources\n");
}

static struct drm_display_mode *kw080kd_get_modes(
	struct intel_dsi_device *dsi)
{
	struct drm_display_mode *mode = NULL;
	printk("kw080kd_get_modes\n");
	DRM_DEBUG_KMS("\n");
	/* Allocate */
	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode) {
		DRM_DEBUG_KMS("Cpt panel: No memory\n");
		return NULL;
	}
	mode->hdisplay = 1200;
	mode->vdisplay = 1920;

	mode->hsync_start = mode->hdisplay + 20; //HFP
	mode->hsync_end = mode->hsync_start + 5; //HSW
	mode->htotal = mode->hsync_end + 20; //HBP

	mode->vsync_start = mode->vdisplay + 20; //VFP
	mode->vsync_end = mode->vsync_start + 5;//VSW
	mode->vtotal = mode->vsync_end + 20;//VBP
	mode->vrefresh = 60; 
	mode->clock =  mode->vrefresh * mode->vtotal *mode->htotal / 1000;
	mode->type |= DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_set_crtcinfo(mode, 0);

	return mode;
}


static bool kw080kd_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	printk("kw080kd_get_hw_state\n");
	return true;
}

static enum drm_connector_status kw080kd_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
 	dev_priv->is_mipi = true;
	printk("kw080kd_detect\n");
	DRM_DEBUG_KMS("\n");
	return connector_status_connected;
}

static bool kw080kd_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	printk("kw080kd_mode_fixup\n");
	return true;
}

void kw080kd_reset(struct intel_dsi_device *dsi)
{int val;
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	DRM_DEBUG_KMS("\n");
	printk("kw080kd_reset\n");
val = intel_mid_pmic_readb(0x12);
val |= 1<<5;
  intel_mid_pmic_writeb(0x12, val);
msleep(50);
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

static int kw080kd_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
    DRM_DEBUG_KMS("\n");
	printk("kw080kd_mode_valid\n");
	return MODE_OK;
}

static void kw080kd_dpms(struct intel_dsi_device *dsi, bool enable)
{
//	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	printk("kw080kd_dpms\n");
	DRM_DEBUG_KMS("\n");
}
static void kw080kd_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");
	printk("kw080kd_enable\n");
	
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(30);  //wait for 30ms
	
	msleep(150);  //wait for 150ms
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	msleep(200);
}
static void kw080kd_disable(struct intel_dsi_device *dsi)
{int val;
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	printk("kw080kd_disable\n");
	msleep(200);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(80);

	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(34);
	printk("kw080kd_disable_panel_power\n");
  val = intel_mid_pmic_readb(0x12);
 val &= ~(1<<5);
 intel_mid_pmic_writeb(0x12, val);  
}

bool kw080kd_init(struct intel_dsi_device *dsi)
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

	DRM_DEBUG_KMS("Init: kw080kd panel\n");
	printk("kw080kd_init\n");
	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to kw080kd\n");
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
	intel_dsi->hs_to_lp_count = 0x30;
	/*b060*/
	intel_dsi->lp_byte_clk = 0x06;
	/*b080*/
	intel_dsi->dphy_reg = 0x351D7818;
	/* b088 high 16bits */
	intel_dsi->clk_lp_to_hs_count = 0x3F;
	/* b088 low 16bits */
	intel_dsi->clk_hs_to_lp_count = 0x18;
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
struct intel_dsi_dev_ops qc_kw080kd_dsi_display_ops = {
	.init = kw080kd_init,
	.get_info = kw080kd_get_panel_info,
	.create_resources = kw080kd_create_resources,
	.dpms = kw080kd_dpms,
	.mode_valid = kw080kd_mode_valid,
	.mode_fixup = kw080kd_mode_fixup,
	.panel_reset = kw080kd_reset,
	.detect = kw080kd_detect,
	.get_hw_state = kw080kd_get_hw_state,
	.get_modes = kw080kd_get_modes,
	.destroy = kw080kd_destroy,
	.dump_regs = kw080kd_dump_regs,
	.enable = kw080kd_enable,
	.disable = kw080kd_disable,
	.send_otp_cmds = kw080kd_send_otp_cmds,
};
