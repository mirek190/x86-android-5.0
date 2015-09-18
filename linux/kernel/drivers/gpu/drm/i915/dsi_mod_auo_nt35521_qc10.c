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
#include "dsi_mod_auo_nt35521_qc10.h"

typedef struct {
    unsigned char count;
    unsigned char para_list[128];
} LCM_setting_table;


static LCM_setting_table init_sequence[] =
{
	{5, {0xFF,0xAA,0x55,0xA5,0x80}},

	{3, {0x6F,0x11,0x00}},
	{3, {0xF7,0x20,0x00}},
	{2, {0x6F,0x06}},
	{2, {0xF7,0xA0}},
	{2, {0x6F,0x19}},
	{2, {0xF7,0x12}},
	{2, {0xF4,0x03}}, 

	{2, {0x6F,0x08}},                  			// new Vcom floating 
	{2, {0xFA,0x40}},
	{2, {0x6F,0x11}},
	{2, {0xF3,0x01}},

	{2, {0x6F,0x06}},							// for abnormal power off
	{2, {0xFC,0x03}},

	{6, {0xF0,0x55,0xAA,0x52,0x08,0x00}},

	{3, {0xB1,0x68,0x01}},

	{2, {0xB6,0x08}},

	{2, {0x6F,0x02}},
	{2, {0xB8,0x08}},

	{3, {0xBB,0x54,0x44}},

	{3, {0xBC,0x05,0x05}},

	{2, {0xC7,0x01}},
	{6, {0xBD, 0x02,0xB0,0x1E,0x1E,0x00}},

	{3, {0xC5, 0x01,0x07}},

	{2, {0xC8, 0x80}},

	{6, {0xF0,0x55,0xAA,0x52,0x08,0x01}},

	{3, {0xB0,0x05,0x05}},
	{3, {0xB1,0x05,0x05}},

	{3, {0xB2,0x00,0x00}},			//new

	{3, {0xBC,0x90,0x01}},
	{3, {0xBD,0x90,0x01}},

	{2, {0xCA,0x00}},

	{2, {0xC0,0x04}},

	{2, {0xBE,0x29}},

	{3, {0xB3,0x28,0x28}},
	{3, {0xB4,0x12,0x12}},

	{3, {0xB9,0x35,0x35}},
	{3, {0xBA,0x25,0x25}},

	{6, {0xF0,0x55,0xAA,0x52,0x08,0x02}},

	{2, {0xEE,0x01}},
	{5, {0xEF,0x09,0x06,0x15,0x18}},

	{7, {0xB0,0x00,0x00,0x00,0x24,0x00,0x55}},
	{2, {0x6F,0x06}},
	{7, {0xB0,0x00,0x77,0x00,0x94,0x00,0xC0}},
	{2, {0x6F,0x0C}},
	{5, {0xB0,0x00,0xE3,0x01,0x1A}},                                                                                                                                         
	{7, {0xB1,0x01,0x46,0x01,0x88,0x01,0xBC}},
	{2, {0x6F,0x06}},
	{7, {0xB1,0x02,0x0B,0x02,0x4B,0x02,0x4D}},
	{2, {0x6F,0x0C}},
	{5, {0xB1,0x02,0x88,0x02,0xC9}},
	{7, {0xB2,0x02,0xF3,0x03,0x29,0x03,0x4E}},
	{2, {0x6F,0x06}},                         
	{7, {0xB2,0x03,0x7D,0x03,0x9B,0x03,0xBE}},
	{2, {0x6F,0x0C}},
	{5, {0xB2,0x03,0xD3,0x03,0xE9}},
	{5, {0xB3,0x03,0xFB,0x03,0xFF}},   

	{6, {0xF0, 0x55,0xAA,0x52,0x08,0x06}},
	{3, {0xB0, 0x0B,0x2E}},
	{3, {0xB1, 0x2E,0x2E}},
	{3, {0xB2, 0x2E,0x09}},
	{3, {0xB3, 0x2A,0x29}},
	{3, {0xB4, 0x1B,0x19}},
	{3, {0xB5, 0x17,0x15}},
	{3, {0xB6, 0x13,0x11}},
	{3, {0xB7, 0x01,0x2E}},
	{3, {0xB8, 0x2E,0x2E}},
	{3, {0xB9, 0x2E,0x2E}},
	{3, {0xBA, 0x2E,0x2E}},
	{3, {0xBB, 0x2E,0x2E}},
	{3, {0xBC, 0x2E,0x00}},
	{3, {0xBD, 0x10,0x12}},
	{3, {0xBE, 0x14,0x16}},
	{3, {0xBF, 0x18,0x1A}},
	{3, {0xC0, 0x29,0x2A}},
	{3, {0xC1, 0x08,0x2E}},
	{3, {0xC2, 0x2E,0x2E}},
	{3, {0xC3, 0x2E,0x0A}},
	{3, {0xE5, 0x2E,0x2E}},
	{3, {0xC4, 0x0A,0x2E}},
	{3, {0xC5, 0x2E,0x2E}},
	{3, {0xC6, 0x2E,0x00}},
	{3, {0xC7, 0x2A,0x29}},
	{3, {0xC8, 0x10,0x12}},
	{3, {0xC9, 0x14,0x16}},
	{3, {0xCA, 0x18,0x1A}},
	{3, {0xCB, 0x08,0x2E}},
	{3, {0xCC, 0x2E,0x2E}},
	{3, {0xCD, 0x2E,0x2E}},
	{3, {0xCE, 0x2E,0x2E}},
	{3, {0xCF, 0x2E,0x2E}},
	{3, {0xD0, 0x2E,0x09}},
	{3, {0xD1, 0x1B,0x19}},
	{3, {0xD2, 0x17,0x15}},
	{3, {0xD3, 0x13,0x11}},
	{3, {0xD4, 0x29,0x2A}},
	{3, {0xD5, 0x01,0x2E}},
	{3, {0xD6, 0x2E,0x2E}},
	{3, {0xD7, 0x2E,0x0B}},
	{3, {0xE6, 0x2E,0x2E}},
	{6, {0xD8, 0x00,0x00,0x00,0x00,0x00}},
	{6, {0xD9, 0x00,0x00,0x00,0x00,0x00}},
	{2, {0xE7, 0x00}},
	
	{6, {0xF0, 0x55,0xAA,0x52,0x08,0x03}},
	{2, {0xB0, 0x20,0x00}},
	{2, {0xB1, 0x20,0x00}},
	{6, {0xB2, 0x05,0x00,0x00,0x00,0x00}},

	{6, {0xB6, 0x05,0x00,0x00,0x00,0x00}},
	{6, {0xB7, 0x05,0x00,0x00,0x00,0x00}},

	{6, {0xBA, 0x57,0x00,0x00,0x00,0x00}},
	{6, {0xBB, 0x57,0x00,0x00,0x00,0x00}},

	{5, {0xC0, 0x00,0x00,0x00,0x00}},
	{5, {0xC1, 0x00,0x00,0x00,0x00}},

	{2, {0xC4, 0x60}},
	{2, {0xC5, 0x40}},
	
	{6, {0xF0, 0x55,0xAA,0x52,0x08,0x05}},
	{6, {0xBD, 0x03,0x01,0x03,0x03,0x03}},
	{3, {0xB0, 0x17,0x06}},
	{3, {0xB1, 0x17,0x06}},
	{3, {0xB2, 0x17,0x06}},
	{3, {0xB3, 0x17,0x06}},
	{3, {0xB4, 0x17,0x06}},
	{3, {0xB5, 0x17,0x06}},
		 
	{2, {0xB8, 0x00}},
	{2, {0xB9, 0x00}},
	{2, {0xBA, 0x00}},
	{2, {0xBB, 0x02}},
	{2, {0xBC, 0x00}},

	{2, {0xC0, 0x07}},

	{2, {0xC4, 0x80}},
	{2, {0xC5, 0xA4}},

	{3, {0xC8, 0x05,0x30}},
	{3, {0xC9, 0x01,0x31}},

	{4, {0xCC, 0x00,0x00,0x3C}},
	{4, {0xCD, 0x00,0x00,0x3C}},

	{6, {0xD1, 0x00,0x05,0x09,0x07,0x10}},
	{6, {0xD2, 0x00,0x05,0x0E,0x07,0x10}},
	
	{2, {0xE5, 0x06}},
	{2, {0xE6, 0x06}},
	{2, {0xE7, 0x06}},
	{2, {0xE8, 0x06}},
	{2, {0xE9, 0x06}},
	{2, {0xEA, 0x06}},

	{2, {0xED, 0x30}},

	{2, {0x6F,0x11}},
	{2, {0xF3,0x01}},

	{2, {0x35,0x00}},
	{1, {0x11}},

	{1, {0x29}}
};



void nt35521_qc10_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	int sequence_line, i;
	DRM_DEBUG_KMS("nt35521_qc10_send_otp_cmds \n");

	sequence_line = sizeof(init_sequence) / sizeof(init_sequence[0]);
	for(i = 0; i < sequence_line; i++)
	{
		dsi_vc_dcs_write(intel_dsi, 0, &init_sequence[i].para_list[0], init_sequence[i].count);
		if(init_sequence[i].para_list[0] == 0x11)
			msleep(120);
		if(init_sequence[i].para_list[0] == 0x29)
			msleep(20);
	}

}

static void nt35521_qc10_get_panel_info(int pipe,
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

static void nt35521_qc10_destroy(struct intel_dsi_device *dsi)
{
}

static void nt35521_qc10_dump_regs(struct intel_dsi_device *dsi)
{
}

static void nt35521_qc10_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *nt35521_qc10_get_modes(
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

	mode->hsync_start = mode->hdisplay + 16; //HFP
	mode->hsync_end = mode->hsync_start + 40;//HSYNC
	mode->htotal = mode->hsync_end + 48;//HBP

	mode->vsync_start = mode->vdisplay + 5; //VFP
	mode->vsync_end = mode->vsync_start + 20;//VSYNC
	mode->vtotal = mode->vsync_end + 3;//VBP
	mode->vrefresh = 60; 
	mode->clock =  mode->vrefresh * mode->vtotal *mode->htotal / 1000;
	mode->type |= DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_set_crtcinfo(mode, 0);

	return mode;
}


static bool nt35521_qc10_get_hw_state(struct intel_dsi_device *dev)
{
	DRM_DEBUG_KMS("\n");
	return true;
}

static enum drm_connector_status nt35521_qc10_detect(
					struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
 	dev_priv->is_mipi = true;
	DRM_DEBUG_KMS("\n");
	return connector_status_connected;
}

static bool nt35521_qc10_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}
#define DISP_RST_N 107
void nt35521_qc10_reset(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	DRM_DEBUG_KMS("\n");
	printk("nt35521_qc10_reset\n");

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

static int nt35521_qc10_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
    DRM_DEBUG_KMS("\n");
	return MODE_OK;
}

static void nt35521_qc10_dpms(struct intel_dsi_device *dsi, bool enable)
{
//	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("\n");
}
static void nt35521_qc10_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS(" nt35521_qc10_enable\n");
	//dsi_vc_dcs_write(intel_dsi, 0, cpt_init_sequence5, sizeof(cpt_init_sequence5));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(150);  //wait for 150ms
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	msleep(200);
}
static void nt35521_qc10_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

	DRM_DEBUG_KMS("\n");
	msleep(200);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(80);
	//dsi_vc_dcs_write(intel_dsi, 0, cpt_disable_ic_power, sizeof(cpt_disable_ic_power));
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(34);
}

bool nt35521_qc10_init(struct intel_dsi_device *dsi)
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

	DRM_DEBUG_KMS("Init: nt35521_qc10 panel\n");
	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to nt35521_qc10\n");
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
	intel_dsi->dphy_reg = 0x180D360B;
	/* b088 high 16bits */
	intel_dsi->clk_lp_to_hs_count = 0x1F;
	/* b088 low 16bits */
	intel_dsi->clk_hs_to_lp_count = 0x0D;
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
struct intel_dsi_dev_ops auo_nt35521_qc10_dsi_display_ops = {
	.init = nt35521_qc10_init,
	.get_info = nt35521_qc10_get_panel_info,
	.create_resources = nt35521_qc10_create_resources,
	.dpms = nt35521_qc10_dpms,
	.mode_valid = nt35521_qc10_mode_valid,
	.mode_fixup = nt35521_qc10_mode_fixup,
	.panel_reset = nt35521_qc10_reset,
	.detect = nt35521_qc10_detect,
	.get_hw_state = nt35521_qc10_get_hw_state,
	.get_modes = nt35521_qc10_get_modes,
	.destroy = nt35521_qc10_destroy,
	.dump_regs = nt35521_qc10_dump_regs,
	.enable = nt35521_qc10_enable,
	.disable = nt35521_qc10_disable,
	.send_otp_cmds = nt35521_qc10_send_otp_cmds,
};
