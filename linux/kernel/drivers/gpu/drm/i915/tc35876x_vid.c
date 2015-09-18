/*
 * Copyright © 2010 Intel Corporation
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
 * Authors:
 * Xiujun Geng <xiujun.geng@intel.com>
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
#include "tc35876x_vid.h"
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include "linux/mfd/intel_mid_pmic.h"

/*	TC358764XBG	*/
/*GPIO Pins */
#define GPIO_MIPI_BRIDGE_RESET	153		/*	GPIO_S5_23 changed to PANEL0_BKLTCTL/GPIONC_5	*/
#define GPIO_PANEL_RESET	94			/*	GPIO_S5_9 MDSI2LVDS_RST_N	*/
#define GPIO_PANEL_STANDBY	154		/*	GPIO_S5_8 SOC_LCD_PWDN	*/
#define GPIO_PANEL_EN		58			/*	GPIO_S0_58 //Do not require , in AIC , PANEL Power connected to 3P3S +V3P3S_LCD	*/

/* bridge registers */
/* DSI PPI Layer Registers */
#define PPI_STARTPPI		0x0104
#define PPI_BUSYPPI		0x0108
#define PPI_LINEINITCNT		0x0110
#define PPI_LPTXTIMECNT		0x0114
#define PPI_LANEENABLE		0x0134
#define PPI_TX_RX_TA		0x013C
#define PPI_CLS_ATMR		0x0140
#define PPI_D0S_ATMR		0x0144
#define PPI_D1S_ATMR		0x0148
#define PPI_D2S_ATMR		0x014C
#define PPI_D3S_ATMR		0x0150
#define PPI_D0S_CLRSIPOCOUNT	0x0164
#define PPI_D1S_CLRSIPOCOUNT	0x0168
#define PPI_D2S_CLRSIPOCOUNT	0x016C
#define PPI_D3S_CLRSIPOCOUNT	0x0170
#define CLS_PRE			0x0180
#define D0S_PRE			0x0184
#define D1S_PRE			0x0188
#define D2S_PRE			0x018C
#define D3S_PRE			0x0190
#define CLS_PREP		0x01A0
#define D0S_PREP		0x01A4
#define D1S_PREP		0x01A8
#define D2S_PREP		0x01AC
#define D3S_PREP		0x01B0
#define CLS_ZERO		0x01C0
#define D0S_ZERO		0x01C4
#define D1S_ZERO		0x01C8
#define D2S_ZERO		0x01CC
#define D3S_ZERO		0x01D0
#define PPI_CLRFLG		0x01E0
#define PPI_CLRSIPO		0x01E4
#define HSTIMEOUT		0x01F0
#define HSTIMEOUTENABLE		0x01F4

/* DSI Protocol Layer Registers */
#define DSI_STARTDSI		0x0204
#define DSI_BUSYDSI		0x0208
#define DSI_LANEENABLE		0x0210
#define DSI_LANESTATUS0		0x0214
#define DSI_LANESTATUS1		0x0218
#define DSI_INTSTATUS		0x0220
#define DSI_INTMASK		0x0224
#define DSI_INTCLR		0x0228
#define DSI_LPTXTO		0x0230

/* DSI General Registers */
#define DSIERRCNT		0x0300

/* DSI Application Layer Registers */
#define APLCTRL			0x0400
#define RDPKTLN			0x0404

/* Video Path Registers */
#define VPCTRL			0x0450
#define HTIM1			0x0454
#define HTIM2			0x0458
#define VTIM1			0x045C
#define VTIM2			0x0460
#define VFUEN			0x0464

/* LVDS Registers */
#define LVMX0003		0x0480
#define LVMX0407		0x0484
#define LVMX0811		0x0488
#define LVMX1215		0x048C
#define LVMX1619		0x0490
#define LVMX2023		0x0494
#define LVMX2427		0x0498
#define LVCFG			0x049C
#define LVPHY0			0x04A0
#define LVPHY1			0x04A4

/* System Registers */
#define SYSSTAT			0x0500
#define SYSRST			0x0504

/* GPIO Registers */
/*
#define GPIOC			0x0520
#define GPIOO			0x0524
#define GPIOI			0x0528
*/
/* Chip/Rev Registers */
#define IDREG			0x0580
/* Input muxing for registers LVMX0003...LVMX2427 */
enum {
	INPUT_R0,	/* 0 */
	INPUT_R1,
	INPUT_R2,
	INPUT_R3,
	INPUT_R4,
	INPUT_R5,
	INPUT_R6,
	INPUT_R7,
	INPUT_G0,	/* 8 */
	INPUT_G1,
	INPUT_G2,
	INPUT_G3,
	INPUT_G4,
	INPUT_G5,
	INPUT_G6,
	INPUT_G7,
	INPUT_B0,	/* 16 */
	INPUT_B1,
	INPUT_B2,
	INPUT_B3,
	INPUT_B4,
	INPUT_B5,
	INPUT_B6,
	INPUT_B7,
	INPUT_HSYNC,	/* 24 */
	INPUT_VSYNC,
	INPUT_DE,
	LOGIC_0,
	/* 28...31 undefined */
};

#define FLD_MASK(start, end)    (((1<<((start) - (end) + 1)) - 1) << (end))
#define FLD_VAL(val, start, end) (((val)<<(end))&FLD_MASK(start, end))
#define INPUT_MUX(lvmx03, lvmx02, lvmx01, lvmx00)		\
	(FLD_VAL(lvmx03, 29, 24) | FLD_VAL(lvmx02, 20, 16) |  \
	FLD_VAL(lvmx01, 12, 8) | FLD_VAL(lvmx00, 4, 0))

static struct i2c_client *tc35876x_client;
static struct intel_dsi *intel_dsi_clone;
/*	#define BRIDGE_INT_CMD_WITH_I2C	*/
#ifdef BRIDGE_INT_CMD_WITH_I2C
static
int tc35876x_bridge_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	DRM_DEBUG_KMS("\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		DRM_ERROR("i2c_check_functionality() failed\n");
		return -ENODEV;
	}

	/*	PANEL0_BKLTCTL/GPIONC_5	*/



	tc35876x_client = client;

	return 0;
}

static int tc35876x_bridge_remove(struct i2c_client *client)
{
	DRM_DEBUG_KMS("\n");
	tc35876x_client = NULL;

	return 0;
}

static
const struct i2c_device_id tc35876x_bridge_id[] = {
	{ "i2c_disp_brig", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tc35876x_bridge_id);

static
struct i2c_driver tc35876x_bridge_i2c_driver = {
	.driver = {
		.name = "i2c_disp_brig",
	},
	.id_table = tc35876x_bridge_id,
	.probe = tc35876x_bridge_probe,
	.remove = tc35876x_bridge_remove,
};

static
int tc35876x_regw(struct i2c_client *client, u16 reg, u32 value)
{
	int r;
	u8 tx_data[] = {
		/* NOTE: Register address big-endian, data little-endian. */
	(reg >> 8) & 0xff,
	reg & 0xff,
	value & 0xff,
	(value >> 8) & 0xff,
	(value >> 16) & 0xff,
	(value >> 24) & 0xff,
	};
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = tx_data,
			.len = ARRAY_SIZE(tx_data),
		},
	};

	r = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (r < 0) {
		dev_err(&client->dev, "%s: reg 0x%04x val 0x%08x error %d\n",
			__func__, reg, value, r);
		return r;
	}

	if (r < ARRAY_SIZE(msgs)) {
		dev_err(&client->dev, "%s: reg 0x%04x val 0x%08x msgs %d\n",
			__func__, reg, value, r);
		return -EAGAIN;
	}

	return 0;
}
static
int tc35876x_regr(struct i2c_client *client, u16 reg, u32 *value)
{
	int r;
	u8 tx_data[] = {
		(reg >> 8) & 0xff,
		reg & 0xff,
	};
	u8 rx_data[4];
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = tx_data,
			.len = ARRAY_SIZE(tx_data),
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.buf = rx_data,
			.len = ARRAY_SIZE(rx_data),
		 },
	};

	r = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (r < 0) {
		dev_err(&client->dev, "%s: reg 0x%04x error %d\n", __func__,
			reg, r);
		return r;
	}

	if (r < ARRAY_SIZE(msgs)) {
		dev_err(&client->dev, "%s: reg 0x%04x msgs %d\n", __func__,
			reg, r);
		return -EAGAIN;
	}

	*value = rx_data[0] << 24 | rx_data[1] << 16 |
		rx_data[2] << 8 | rx_data[3];

	dev_dbg(&client->dev, "%s: reg 0x%04x value 0x%08x\n", __func__,
		reg, *value);

	return 0;
}

#else
static
int tc35876x_regw(struct i2c_client *client, u16 reg, u32 value)
{
	/* int r; */
	u8 tx_data[] = {
		/* NOTE: Register address big-endian, data little-endian. */
		reg & 0xff,
		(reg >> 8) & 0xff,
		value & 0xff,
		(value >> 8) & 0xff,
		(value >> 16) & 0xff,
		(value >> 24) & 0xff,
	};

	dsi_vc_generic_write(intel_dsi_clone, 0, tx_data, 6);

	return 0;
}

int tc35876x_regr(struct i2c_client *client, u16 reg, u32 *value)
{
/*	int r; */
	u32 data1;
	u32 data2;
	u8 tx_data[] = {
		reg & 0xff,
		(reg >> 8) & 0xff,
	};
	u8 rx_data[8];
	dsi_vc_generic_read(intel_dsi_clone, 0, tx_data, 2, rx_data, 4);
	data1 = rx_data[0] << 24 | rx_data[1] << 16 |
		rx_data[2] << 8 | rx_data[3];
	data2 = rx_data[4] << 24 | rx_data[5] << 16 |
		rx_data[6] << 8 | rx_data[7];

	DRM_DEBUG_KMS("%s: reg 0x%04x value 0x%08x, 0x%08x\n", __func__,
		reg, data1, data2);
	*value = data2;
	return 0;
}

#endif


void tc35876x_configure_lvds_bridge(void)
{
	struct i2c_client *i2c = tc35876x_client;
	u32 id = 0;
	u32 ppi_lptxtimecnt;
	u32 txtagocnt;
	u32 txtasurecnt;

	DRM_DEBUG_KMS("\n");

	tc35876x_regr(i2c, IDREG, &id);
	DRM_INFO("tc35876x ID 0x%08x\n", id);

	ppi_lptxtimecnt = 4;
	txtagocnt = (5 * ppi_lptxtimecnt - 3) / 4;
	txtasurecnt = 3 * ppi_lptxtimecnt / 2;

	tc35876x_regw(i2c, PPI_TX_RX_TA, FLD_VAL(txtagocnt, 26, 16) | FLD_VAL(txtasurecnt, 10, 0));
	tc35876x_regw(i2c, PPI_LPTXTIMECNT, FLD_VAL(ppi_lptxtimecnt, 10, 0));
	tc35876x_regw(i2c, PPI_D0S_CLRSIPOCOUNT, FLD_VAL(1, 5, 0));
	tc35876x_regw(i2c, PPI_D1S_CLRSIPOCOUNT, FLD_VAL(1, 5, 0));
	tc35876x_regw(i2c, PPI_D2S_CLRSIPOCOUNT, FLD_VAL(1, 5, 0));
	tc35876x_regw(i2c, PPI_D3S_CLRSIPOCOUNT, FLD_VAL(1, 5, 0));

	/* Enabling MIPI & PPI lanes, Enable 4 lanes */
	tc35876x_regw(i2c, PPI_LANEENABLE,
		BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0));
	tc35876x_regw(i2c, DSI_LANEENABLE,
		BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0));
	tc35876x_regw(i2c, PPI_STARTPPI, BIT(0));
	tc35876x_regw(i2c, DSI_STARTDSI, BIT(0));
	/* Setting LVDS output frequency */
	tc35876x_regw(i2c, LVPHY0, FLD_VAL(1, 20, 16) |
		FLD_VAL(2, 15, 14) | FLD_VAL(6, 4, 0)); /* 0x00048006 */

	/* Setting video panel control register,0x00000120 VTGen=ON ?!?!? */
	/* tc35876x_regw(i2c, VPCTRL, BIT(5)|BIT(0)); */
	if ((LVDS_DSI_TC35876X_AUO_B101EAN01_2 == i915_mipi_panel_id) || (LVDS_DSI_TC35876X_BOE_BP101WX4_300 == i915_mipi_panel_id))
		tc35876x_regw(i2c, VPCTRL, BIT(5) | BIT(8));
	else
	/* tc35876x_regw(i2c, VPCTRL, BIT(5)|BIT(0)); */
		tc35876x_regw(i2c, VPCTRL, 0x00a00021);
	/* Horizontal back porch and horizontal pulse width. 0x00280028 */
	tc35876x_regw(i2c, HTIM1, FLD_VAL(40, 24, 16) | FLD_VAL(40, 8, 0));

	/* Horizontal front porch and horizontal active video size. 0x00500500*/
	tc35876x_regw(i2c, HTIM2, FLD_VAL(80, 24, 16) | FLD_VAL(1280, 10, 0));

	/* Vertical back porch and vertical sync pulse width. 0x000e000a */
	tc35876x_regw(i2c, VTIM1, FLD_VAL(14, 23, 16) | FLD_VAL(10, 7, 0));

	/* Vertical front porch and vertical display size. 0x000e0320 */
	tc35876x_regw(i2c, VTIM2, FLD_VAL(14, 23, 16) | FLD_VAL(800, 10, 0));

	/* Set above HTIM1, HTIM2, VTIM1, and VTIM2 at next VSYNC. */
	tc35876x_regw(i2c, VFUEN, BIT(0));

	/* Soft reset LCD controller. */
	tc35876x_regw(i2c, SYSRST, BIT(2));
	if ((LVDS_DSI_TC35876X_AUO_B101EAN01_2 == i915_mipi_panel_id) || (LVDS_DSI_TC35876X_BOE_BP101WX4_300 == i915_mipi_panel_id)) {
		/*modify by PJ7 for grandhill&flaghill LCD,2013-07-01*/
		tc35876x_regw(i2c, LVMX0003,
			INPUT_MUX(INPUT_R3, INPUT_R2, INPUT_R1, INPUT_R0));
		tc35876x_regw(i2c, LVMX0407,
			INPUT_MUX(INPUT_G0, INPUT_R5, INPUT_R7, INPUT_R4));
		tc35876x_regw(i2c, LVMX0811,
			INPUT_MUX(INPUT_G7, INPUT_G6, INPUT_G2, INPUT_G1));
		tc35876x_regw(i2c, LVMX1215,
			INPUT_MUX(INPUT_B0, INPUT_G5, INPUT_G4, INPUT_G3));
		tc35876x_regw(i2c, LVMX1619,
			INPUT_MUX(INPUT_B2, INPUT_B1, INPUT_B7, INPUT_B6));
		tc35876x_regw(i2c, LVMX2023,
			INPUT_MUX(LOGIC_0,  INPUT_B5, INPUT_B4, INPUT_B3));
		tc35876x_regw(i2c, LVMX2427,
			INPUT_MUX(INPUT_R6, INPUT_DE, INPUT_VSYNC, INPUT_HSYNC));
		/*modify end*/
		} else {
			/*modify by lucheng megered from MFLD */
		tc35876x_regw(i2c, LVMX0003,
			INPUT_MUX(INPUT_R5, INPUT_R4, INPUT_R3, INPUT_R2));
		tc35876x_regw(i2c, LVMX0407,
			INPUT_MUX(INPUT_G2, INPUT_R7, INPUT_R1, INPUT_R6));
		tc35876x_regw(i2c, LVMX0811,
			INPUT_MUX(INPUT_G1, INPUT_G0, INPUT_G4, INPUT_G3));
		tc35876x_regw(i2c, LVMX1215,
			INPUT_MUX(INPUT_B2, INPUT_G7, INPUT_G6, INPUT_G5));
		tc35876x_regw(i2c, LVMX1619,
			INPUT_MUX(INPUT_B4, INPUT_B3, INPUT_B1, INPUT_B0));
		tc35876x_regw(i2c, LVMX2023,
			INPUT_MUX(LOGIC_0,  INPUT_B7, INPUT_B6, INPUT_B5));
		tc35876x_regw(i2c, LVMX2427,
			INPUT_MUX(INPUT_R0, INPUT_DE, INPUT_VSYNC, INPUT_HSYNC));
		/*modify end*/
		}
	/* Enable LVDS transmitter. */
	tc35876x_regw(i2c, LVCFG, BIT(0));
	/* Clear notifications. Don't write reserved bits. Was write 0xffffffff
	 * to 0x0288, must be in error?! */
	tc35876x_regw(i2c, DSI_INTCLR, FLD_MASK(31, 30) | FLD_MASK(22, 0));

}

static void  tc35876x_get_panel_info(int pipe,
					struct drm_connector *connector)
{
	if (!connector) {
		DRM_DEBUG_KMS("Cpt: Invalid input to get_info\n");
		return;
	}

	if (pipe == 0) {
		if (LVDS_DSI_TC35876X_CPT_CLAA070WP03 == i915_mipi_panel_id) {
			connector->display_info.width_mm = 150;
			connector->display_info.height_mm = 94;
			}
		else if (LVDS_DSI_TC35876X_AUO_B101XTN01_1 == i915_mipi_panel_id) {
			connector->display_info.width_mm = 243;
			connector->display_info.height_mm = 147;
			}
		else if (LVDS_DSI_TC35876X_CMI_N101ICG_L21 == i915_mipi_panel_id) {
			connector->display_info.width_mm = 262;
			connector->display_info.height_mm = 144;
			}
		else if (LVDS_DSI_TC35876X_CDY_BI097XN02 == i915_mipi_panel_id) {
			connector->display_info.width_mm = 196;
			connector->display_info.height_mm = 147;
			}
		else if (LVDS_DSI_TC35876X_AUO_B101EAN01_2 == i915_mipi_panel_id) {
			connector->display_info.width_mm = 227;
			connector->display_info.height_mm = 147;
			}
		else if (LVDS_DSI_TC35876X_BOE_BP101WX4_300 == i915_mipi_panel_id) {
			connector->display_info.width_mm = 216;
			connector->display_info.height_mm = 135;
			}
		else{
			DRM_DEBUG_KMS("tc35876x_get_panel_info enter oops !!!");
			}

	}

	return;
}

static void tc35876x_destroy(struct intel_dsi_device *dsi)
{
}

static void tc35876x_dump_regs(struct intel_dsi_device *dsi)
{
}

static void tc35876x_create_resources(struct intel_dsi_device *dsi)
{
}

static struct drm_display_mode *tc35876x_get_modes(
	struct intel_dsi_device *dsi)
{
	struct drm_display_mode *mode = NULL;
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	DRM_DEBUG_KMS("tc35876x_get_modes enter");
	/* Allocate */
	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode) {
		DRM_DEBUG_KMS("panel: No memory\n");
		return NULL;
	}
	if (LVDS_DSI_TC35876X_CPT_CLAA070WP03 == i915_mipi_panel_id) {
		DRM_DEBUG_KMS("tc35876x_get_modes enter CLA_A070WP03");
		mode->hdisplay = 800;
		mode->vdisplay = 1280;
		mode->hsync_start = mode->hdisplay + 20;
		mode->hsync_end = mode->hsync_start + 24;
		mode->htotal = mode->hsync_end + 20;
		mode->vsync_start = mode->vdisplay + 3;
		mode->vsync_end = mode->vsync_start + 2;
		mode->vtotal = mode->vsync_end + 3;
		}
	else if (LVDS_DSI_TC35876X_AUO_B101XTN01_1 == i915_mipi_panel_id) {
		DRM_DEBUG_KMS("tc35876x_get_modes enter AUO_B101XTN01_1");
		mode->hdisplay = 1364;
		mode->vdisplay = 768;
		mode->hsync_start = mode->hdisplay + 30;
		mode->hsync_end = mode->hsync_start + 30;
		mode->htotal = mode->hsync_end + 30;
		mode->vsync_start = mode->vdisplay + 10;
		mode->vsync_end = mode->vsync_start + 10;
		mode->vtotal = mode->vsync_end + 36;
		}
	else if (LVDS_DSI_TC35876X_CMI_N101ICG_L21 == i915_mipi_panel_id) {
		DRM_DEBUG_KMS("tc35876x_get_modes enter CMI_N101ICG");
		mode->hdisplay = 1280;
		mode->vdisplay = 800;
		mode->hsync_start = mode->hdisplay + 48;
		mode->hsync_end = mode->hsync_start + 32;
		mode->htotal = mode->hsync_end + 80;
		mode->vsync_start = mode->vdisplay + 8;
		mode->vsync_end = mode->vsync_start + 7;
		mode->vtotal = mode->vsync_end + 8;
		}
	else if (LVDS_DSI_TC35876X_CDY_BI097XN02 == i915_mipi_panel_id) {
		DRM_DEBUG_KMS("tc35876x_get_modes enter CENTURY_BI097XN02");
		mode->hdisplay = 1024;
		mode->vdisplay = 768;
		mode->hsync_start = mode->hdisplay + 30;
		mode->hsync_end = mode->hsync_start + 20;
		mode->htotal = mode->hsync_end + 30;
		mode->vsync_start = mode->vdisplay + 16;
		mode->vsync_end = mode->vsync_start + 10;
		mode->vtotal = mode->vsync_end + 16;
		}
	else if ((LVDS_DSI_TC35876X_AUO_B101EAN01_2 == i915_mipi_panel_id) || (LVDS_DSI_TC35876X_BOE_BP101WX4_300 == i915_mipi_panel_id)) {
		DRM_DEBUG_KMS("tc35876x_get_modes enter AUO_B101EAN01_2");
		mode->hdisplay = 1280;
		mode->vdisplay = 800;
		mode->hsync_start = mode->hdisplay + 30;
		mode->hsync_end = mode->hsync_start + 20;
		mode->htotal = mode->hsync_end + 30;
		mode->vsync_start = mode->vdisplay + 4;
		mode->vsync_end = mode->vsync_start + 2;
		mode->vtotal = mode->vsync_end + 4;
		}
	else {
		DRM_DEBUG_KMS("tc35876x_get_modes enter oops !!!");
		}

	mode->vrefresh = 60;
	mode->clock = mode->vrefresh * mode->htotal * mode->vtotal / 1000;
	intel_dsi->pclk = mode->clock;
	DRM_DEBUG_KMS("hdisplay is %d\n", mode->hdisplay);
	DRM_DEBUG_KMS("vdisplay is %d\n", mode->vdisplay);
	DRM_DEBUG_KMS("HSS is %d\n", mode->hsync_start);
	DRM_DEBUG_KMS("HSE is %d\n", mode->hsync_end);
	DRM_DEBUG_KMS("htotal is %d\n", mode->htotal);
	DRM_DEBUG_KMS("VSS is %d\n", mode->vsync_start);
	DRM_DEBUG_KMS("VSE is %d\n", mode->vsync_end);
	DRM_DEBUG_KMS("vtotal is %d\n", mode->vtotal);
	DRM_DEBUG_KMS("clock is %d\n", mode->clock);
	/* mode->type |= DRM_MODE_TYPE_PREFERRED; */
	drm_mode_set_name(mode);
	drm_mode_set_crtcinfo(mode, 0);
	mode->type |= DRM_MODE_TYPE_PREFERRED;
	return mode;
}


static bool tc35876x_get_hw_state(struct intel_dsi_device *dev)
{
	return true;
}

static enum drm_connector_status tc35876x_detect(
					struct intel_dsi_device *dsi)
{
/*	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
		struct drm_device *dev = intel_dsi->base.base.dev;
		struct drm_i915_private *dev_priv = dev->dev_private;
		*/
		return connector_status_connected;
}

static bool tc35876x_mode_fixup(struct intel_dsi_device *dsi,
		    const struct drm_display_mode *mode,
		    struct drm_display_mode *adjusted_mode) {
	return true;
}

void tc35876x_panel_reset(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int val, ret;
	dev_priv = dev_priv;
	DRM_DEBUG_KMS("\n");

	if (BYT_CR_CONFIG) {
		/* enable DLDO3 output */
		val = intel_mid_pmic_readb(0x12);
		if (0 == (val & (1 << 5))) {
			val |= (1<<5);
			ret = intel_mid_pmic_writeb(0x12, val);

			if (LVDS_DSI_TC35876X_BOE_BP101WX4_300 == i915_mipi_panel_id)
				msleep(250);
			else
				msleep(100); 
		}
	}

#if 0
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
		usleep_range(100000, 120000);
#endif
}

static int tc35876x_mode_valid(struct intel_dsi_device *dsi,
		   struct drm_display_mode *mode)
{
	return MODE_OK;
}

static void tc35876x_dpms(struct intel_dsi_device *dsi, bool enable)
{

	DRM_DEBUG_KMS("\n");
}
static void tc35876x_enable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	struct intel_dsi *intel_dsi_clone = container_of(dsi, struct intel_dsi, dev);
	struct drm_device *dev = intel_dsi->base.base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	bool old_value;
	DRM_DEBUG_KMS("Config bridge chip in enable function\n");
	old_value = 0;
	dev_priv = dev_priv;
	old_value = intel_dsi_clone->hs;
	intel_dsi_clone->hs = 0;
	tc35876x_configure_lvds_bridge();
	intel_dsi_clone->hs = old_value;
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x11);
	msleep(20);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x29);
	msleep(20);
}
static void tc35876x_disable(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);
	int val, ret;
	DRM_DEBUG_KMS("\n");
	intel_dsi = intel_dsi;
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x28);
	msleep(20);
	dsi_vc_dcs_write_0(intel_dsi, 0, 0x10);
	msleep(20);

	if (BYT_CR_CONFIG) {
		/*Disable DLDO3 output*/
		val = intel_mid_pmic_readb(0x12);
		val &= ~(1<<5);
		ret = intel_mid_pmic_writeb(0x12, val);
		mdelay(20);
	}

}

bool tc35876x_init(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi = container_of(dsi, struct intel_dsi, dev);

#ifdef BRIDGE_INT_CMD_WITH_I2C

	/* schematic's i2c bus start at 0, but ACPI start at 1 */
	int i2c_busnum = 11; /* test on Baytrail RVP */
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	int ret;
	struct tc35876x_platform_data {
		int gpio_bridge_reset;
		int gpio_panel_bl_en;
		int gpio_panel_vadd;
	};

	struct i2c_board_info tc35876x_i2c_info = {
		.type = "i2c_disp_brig",
		.addr = 0x0f,
	};

	/* static struct tc35876x_platform_data pdata;
		u32 hdmic_reg;
		hdmic_reg = I915_READ(0x61160);
		printk("tc35876x_init hdimc_reg:0x%x",hdmic_reg); */
#endif

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

	DRM_DEBUG_KMS("Init: tc35876x panel\n");

	if (!dsi) {
		DRM_DEBUG_KMS("Init: Invalid input to tc35876x_init\n");
		return false;
	}
	if (LVDS_DSI_TC35876X_CPT_CLAA070WP03 == i915_mipi_panel_id) {
		intel_dsi->dphy_reg = 0x160d3610;
		intel_dsi->hs_to_lp_count = 0x18;
		intel_dsi->lp_byte_clk = 0x03;
		intel_dsi->clk_lp_to_hs_count = 0x18;
		intel_dsi->clk_hs_to_lp_count = 0x0b;
		}
	else if (LVDS_DSI_TC35876X_AUO_B101XTN01_1 == i915_mipi_panel_id) {
		/*b044*/
		intel_dsi->hs_to_lp_count = 0x2B;
		/*b060*/
		intel_dsi->lp_byte_clk = 0x06;
		/*b080*/
		intel_dsi->dphy_reg = 0x2A18681F;
		/* b088 high 16bits */
		intel_dsi->clk_lp_to_hs_count = 0x2B;
		/* b088 low 16bits */
		intel_dsi->clk_hs_to_lp_count = 0x14;
		}
	else if (LVDS_DSI_TC35876X_CMI_N101ICG_L21 == i915_mipi_panel_id) {
		/*b044*/
		intel_dsi->hs_to_lp_count = 0x2B;
		/*b060*/
		intel_dsi->lp_byte_clk = 0x06;
		/*b080*/
		intel_dsi->dphy_reg = 0x2A18681F;
		/* b088 high 16bits */
		intel_dsi->clk_lp_to_hs_count = 0x2B;
		/* b088 low 16bits */
		intel_dsi->clk_hs_to_lp_count = 0x14;
		}
	else if (LVDS_DSI_TC35876X_CDY_BI097XN02 == i915_mipi_panel_id) {
		intel_dsi->hs_to_lp_count = 0x14;
		/*b060*/
		intel_dsi->lp_byte_clk = 0x03;
		/*b080*/
		intel_dsi->dphy_reg = 0x120A2909;
		/* b088 high 16bits */
		intel_dsi->clk_lp_to_hs_count = 0x18;
		/* b088 low 16bits */
		intel_dsi->clk_hs_to_lp_count = 0x0B;
		}
	else if ((LVDS_DSI_TC35876X_AUO_B101EAN01_2 == i915_mipi_panel_id) || (LVDS_DSI_TC35876X_BOE_BP101WX4_300 == i915_mipi_panel_id)) {
		/*b044*/
		intel_dsi->hs_to_lp_count = 0x27;
		/*b060*/
		intel_dsi->lp_byte_clk = 0x05;
		/*b080*/
		intel_dsi->dphy_reg = 0x2A18681F;
		/* b088 high 16bits */
		intel_dsi->clk_lp_to_hs_count = 0x27;
		/* b088 low 16bits */
		intel_dsi->clk_hs_to_lp_count = 0x12;
		}
	else{
		DRM_DEBUG_KMS("tc35876x_init enter oops !!!");
		}
	intel_dsi->eotp_pkt = 1;
	intel_dsi->operation_mode = DSI_VIDEO_MODE;
	intel_dsi->video_mode_type = DSI_VIDEO_BURST;	/*DSI_VIDEO_NBURST_SEVENT;*/
	intel_dsi->pixel_format = VID_MODE_FORMAT_RGB888;
	intel_dsi->port_bits = 0;
	intel_dsi->turn_arnd_val = 0x14;
	intel_dsi->rst_timer_val = 0xffff;
	intel_dsi->bw_timer = 0x820;
	/* BTA sending at the last blanking line of VFP is disabled */
	intel_dsi->video_frmt_cfg_bits = 1<<3;	/*(2)|(1<<3) = 0x6 */
	intel_dsi->init_count = 0x7d0;
	intel_dsi->lane_count = 4;
	intel_dsi->lp_rx_timeout = 0xffff;
	intel_dsi->port = 0; /* PORT_A by default */
	intel_dsi->burst_mode_ratio = 100;
	intel_dsi->backlight_off_delay = 20;
	intel_dsi->send_shutdown = true;
	intel_dsi->shutdown_pkt_delay = 20;
	/*	intel_dsi->clock_stop;	*/
#ifdef BRIDGE_INT_CMD_WITH_I2C
	DRM_DEBUG_KMS("i2c init: tc35876x panel, busnum:%d\n", i2c_busnum);

	adapter = i2c_get_adapter(i2c_busnum);
	if (!adapter) {
		DRM_DEBUG_KMS("i2c_get_adapter(%d) failed\n", i2c_busnum);
		return -EINVAL;
	}
	client = i2c_new_device(adapter, &tc35876x_i2c_info);
	if (!client) {
		DRM_DEBUG_KMS("i2c_new_device() failed\n");
		i2c_put_adapter(adapter);
		return -EINVAL;
	}
	ret = i2c_add_driver(&tc35876x_bridge_i2c_driver);
	if (ret) {
		DRM_ERROR("add bridge I2C driver faild\n");
		return -EINVAL;
	}
#endif
	return true;
}

void tc35876x_send_otp_cmds(struct intel_dsi_device *dsi)
{
	struct intel_dsi *intel_dsi;
	bool old_value;
	intel_dsi = container_of(dsi, struct intel_dsi, dev);
	intel_dsi_clone = container_of(dsi, struct intel_dsi, dev);
	old_value = 0;
	return;
	DRM_DEBUG_KMS("\n");
	old_value = intel_dsi_clone->hs;
	intel_dsi_clone->hs = 0;
	tc35876x_configure_lvds_bridge();
	intel_dsi_clone->hs = old_value;
}


/* Callbacks. We might not need them all. */
struct intel_dsi_dev_ops tc35876x_dsi_display_ops = {
	.init = tc35876x_init,
	.get_info = tc35876x_get_panel_info,
	.create_resources = tc35876x_create_resources,
	.dpms = tc35876x_dpms,
	.mode_valid = tc35876x_mode_valid,
	.mode_fixup = tc35876x_mode_fixup,
	.panel_reset = tc35876x_panel_reset,
	.detect = tc35876x_detect,
	.get_hw_state = tc35876x_get_hw_state,
	.get_modes = tc35876x_get_modes,
	.destroy = tc35876x_destroy,
	.dump_regs = tc35876x_dump_regs,
	.enable = tc35876x_enable,
	.disable = tc35876x_disable,
	.send_otp_cmds = tc35876x_send_otp_cmds,
};
