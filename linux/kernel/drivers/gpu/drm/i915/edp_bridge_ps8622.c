/*
 * Copyright ? 2008 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Lingyan Guo <lingyan.guo@intel.com>
 *
 */


#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/input/mt.h>
#include <linux/lnw_gpio.h>
#include <linux/acpi.h>
#include <linux/acpi_gpio.h>
#include <drm/drmP.h>
#include "intel_drv.h"
#include <drm/i915_drm.h>
#include "i915_drv.h"
#include "edp_bridge_ps8622.h"



#ifdef CONFIG_PS8622_BRIDGE

int ps8622_i2c_Read(struct i2c_client *client, u8 addr,
	char *writebuf, int writelen,
	char *readbuf, int readlen)
{
	int ret = 0;

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
			 .addr = addr>>1,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = addr>>1,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};

		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			DRM_DEBUG_KMS(": i2c read error.\n");
	  }
	return ret;
}



int ps8622_read_reg(struct i2c_client *client, u8 addr, u8 regaddr, u8 *regvalue)
{
	int ret =  ps8622_i2c_Read(client, addr, &regaddr, 1, regvalue, 1);
	DRM_DEBUG_KMS("(0x%02x,0x%02x)=0x%02x ret:%d\n", addr, regaddr, *regvalue, ret);
	return ret;
}

static int ps8622_i2c_write(struct i2c_client *client, u8 addr, u8 reg, u8 val)
{
	int ret;
	int retry = 0;

	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	u8 data[] = {reg, val};

	msg.addr = addr>>1;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	for (retry = 0; retry < 10; retry++) {
		ret = i2c_transfer(adap, &msg, 1);
		if (1 == ret)
			break;
		usleep_range(500, 2000);
	}
	if (ret < 0) {
		DRM_DEBUG_KMS("(0x%02x,0x%02x,0x%02x) I2C failed: %d\n",
			addr , reg, val, ret);
		return 1;
	}
	return 0;
}

static bool ps8622_send_config(struct i2c_client *client)
{
	int err = 0;
	bool ret = false;
	DRM_DEBUG_KMS(" Begin\n");
	if (client == NULL) {
		DRM_DEBUG_KMS("not i2c client\n");
		return false;
	}
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		DRM_DEBUG_KMS("i2c_check_functionality() failed\n");
		return false;
	} else {
		DRM_DEBUG_KMS("i2c_check_functionality() ok\n");
	}
	usleep_range(2000, 5000);
	err |= ps8622_i2c_write(client, 0x14, 0xa1, 0x01);
	/* SW setting */
	err |= ps8622_i2c_write(client, 0x18, 0x14, 0x01);
	/* RCO SS setting */
	err |= ps8622_i2c_write(client, 0x18, 0xe3, 0x20);
	err |= ps8622_i2c_write(client, 0x18, 0xe2, 0x80);
	/* RPHY Setting */
	err |= ps8622_i2c_write(client, 0x18, 0x8a, 0x0c);
	err |= ps8622_i2c_write(client, 0x18, 0x89, 0x08);
	err |= ps8622_i2c_write(client, 0x18, 0x71, 0x2d);
	/* 2.7G CDR settings */
	err |= ps8622_i2c_write(client, 0x18, 0x7d, 0x07);
	err |= ps8622_i2c_write(client, 0x18, 0x7b, 0x00);
	err |= ps8622_i2c_write(client, 0x18, 0x7a, 0xfd);
	/* 1.62G CDR settings */
	err |= ps8622_i2c_write(client, 0x18, 0xc0, 0x12);
	err |= ps8622_i2c_write(client, 0x18, 0xc1, 0x92);
	err |= ps8622_i2c_write(client, 0x18, 0xc2, 0x1c);
	err |= ps8622_i2c_write(client, 0x18, 0x32, 0x80);
	err |= ps8622_i2c_write(client, 0x18, 0x00, 0xb0);
	err |= ps8622_i2c_write(client, 0x18, 0x15, 0x40);
	err |= ps8622_i2c_write(client, 0x18, 0x54, 0x10);
	err |= ps8622_i2c_write(client, 0x12, 0x02, 0x81);
	err |= ps8622_i2c_write(client, 0x12, 0x21, 0x81);
	err |= ps8622_i2c_write(client, 0x10, 0x52, 0x20);
	err |= ps8622_i2c_write(client, 0x10, 0xf1, 0x03);
	err |= ps8622_i2c_write(client, 0x10, 0x62, 0x41);
	err |= ps8622_i2c_write(client, 0x10, 0xf6, 0x01);
	err |= ps8622_i2c_write(client, 0x14, 0xa1, 0x10);
	err |= ps8622_i2c_write(client, 0x10, 0x77, 0x06);
	err |= ps8622_i2c_write(client, 0x10, 0x4c, 0x04);
	err |= ps8622_i2c_write(client, 0x12, 0xc0, 0x00);
	err |= ps8622_i2c_write(client, 0x12, 0xc1, 0x1c);
	err |= ps8622_i2c_write(client, 0x12, 0xc2, 0xf8);
	err |= ps8622_i2c_write(client, 0x12, 0xc3, 0x44);
	err |= ps8622_i2c_write(client, 0x12, 0xc4, 0x32);
	err |= ps8622_i2c_write(client, 0x12, 0xc5, 0x53);
	err |= ps8622_i2c_write(client, 0x12, 0xc6, 0x4c);
	err |= ps8622_i2c_write(client, 0x12, 0xc7, 0x56);
	err |= ps8622_i2c_write(client, 0x12, 0xc8, 0x35);
	err |= ps8622_i2c_write(client, 0x12, 0xca, 0x01);
	err |= ps8622_i2c_write(client, 0x12, 0xcb, 0x07);
	err |= ps8622_i2c_write(client, 0x12, 0xa5, 0xa0);
	err |= ps8622_i2c_write(client, 0x12, 0xa7, 0xff);
	err |= ps8622_i2c_write(client, 0x12, 0xcc, 0x00);
	err |= ps8622_i2c_write(client, 0x18, 0x10, 0x16);
	err |= ps8622_i2c_write(client, 0x18, 0x59, 0x60);
	err |= ps8622_i2c_write(client, 0x18, 0x54, 0x14);
	err |= ps8622_i2c_write(client, 0x10, 0x3C, 0x40);
	err |= ps8622_i2c_write(client, 0x10, 0x05, 0x0C);
	err |= ps8622_i2c_write(client, 0x14, 0xa1, 0x91);
	usleep_range(2000, 5000);

	ret = err ? false : true;
	DRM_DEBUG_KMS(" End. ret:%d\n", ret);
	return ret;
}



static
const struct i2c_device_id ps8622_bridge_id[] = {
	{"ps8622", 0},
	{"VBUS8622:00", 0},
	{"VBUS8622", 0},
	{}

};

static struct ps8622_priv ps8622 =  {NULL};

static
int ps8622_bridge_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{

	DRM_DEBUG_KMS("\n");
	ps8622.client = client;
	return 0;
}

static int ps8622_bridge_remove(struct i2c_client *client)
{
	DRM_DEBUG_KMS("\n");
	return 0;
}

struct i2c_driver ps8622_bridge_i2c_driver = {
	.driver = {
		.name = "VBUS8622",
	},
	.id_table = ps8622_bridge_id,
	.probe = ps8622_bridge_probe,
	.remove = ps8622_bridge_remove,
};


void ps8622_send_init_cmd(struct intel_dp *intel_dp)
{
	if (intel_dp->bridge_setup_done) {
		DRM_DEBUG_KMS("Skip bridge setup\n");
		return;
	}

	if (ps8622_send_config(ps8622.client)) {
		DRM_DEBUG_KMS("bridge setup done\n");
		intel_dp->bridge_setup_done = true;
	}
}

MODULE_DEVICE_TABLE(i2c, ps8622_bridge_id);




static int __init ps8622_modinit(void)
{
	return i2c_add_driver(&ps8622_bridge_i2c_driver);
}

module_init(ps8622_modinit);

static void __exit ps8622_modexit(void)
{
	i2c_del_driver(&ps8622_bridge_i2c_driver);
}

module_exit(ps8622_modexit);

MODULE_DESCRIPTION("EDP to LVDS Brdige PS8622 driver");
MODULE_AUTHOR("Lingyan Guo <lingyan.guo@intel.com>");
MODULE_LICENSE("GPL");





#endif
