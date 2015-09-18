/**
 * linux/drivers/modem_control/mcd_mdm.c
 *
 * Version 1.0
 *
 * This code includes power sequences for IMC 7060 modems and its derivative.
 * That includes :
 *	- XMM6360
 *	- XMM7160
 *	- XMM7260
 * There is no guarantee for other modems.
 *
 * Intel Mobile driver for modem powering.
 *
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * Contact: Ranquet Guillaune <guillaumex.ranquet@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "mdm_util.h"
#include "mcd_mdm.h"

#include <linux/mdm_ctrl.h>

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

/*****************************************************************************
 *
 * Modem Power/Reset functions
 *
 ****************************************************************************/

int mcd_mdm_init(void *data)
{
	return 0;
}

/**
 *  mcd_mdm_cold_boot - Perform a modem cold boot sequence
 *  @drv: Reference to the driver structure
 *
 *  - Set to HIGH the PWRDWN_N to switch ON the modem
 *  - Set to HIGH the RESET_BB_N
 *  - Do a pulse on ON1
 *  - Do a pulse on ON KEY for Modem 2230
 */
int mcd_mdm_cold_boot(void *data, int rst, int pwr_on, int on_key)
{
	struct mdm_ctrl_mdm_data *mdm_data = data;

    pr_info("kz enter function: %s", __func__);

	/* Toggle the RESET_BB_N */
	gpio_set_value(rst, 1);

	/* Wait before doing the pulse on ON1 */
	usleep_range(mdm_data->pre_on_delay, mdm_data->pre_on_delay);

	/* Do a pulse on ON1 */
	gpio_set_value(pwr_on, 1);
	usleep_range(mdm_data->on_duration, mdm_data->on_duration);
	gpio_set_value(pwr_on, 0);

	/* currently on_key is only used by Modem2230 */
	UNREFERENCED_PARAMETER(on_key);

	return 0;
}

/**
 *  mdm_ctrl_silent_warm_reset_7x6x - Perform a silent modem warm reset
 *				      sequence
 *  @drv: Reference to the driver structure
 *
 *  - Do a pulse on the RESET_BB_N
 *  - No struct modification
 *  - debug purpose only
 */
int mcd_mdm_warm_reset(void *data, int rst)
{
	struct mdm_ctrl_mdm_data *mdm_data = data;

	gpio_set_value(rst, 0);
	usleep_range(mdm_data->warm_rst_duration, mdm_data->warm_rst_duration);
	gpio_set_value(rst, 1);

	return 0;
}

/**
 *  mcd_mdm_power_off - Perform the modem switch OFF sequence
 *  @drv: Reference to the driver structure
 *
 *  - Set to low the ON1
 *  - Write the PMIC reg
 */
int mcd_mdm_power_off(void *data, int rst, int pwr_on)
{
	struct mdm_ctrl_mdm_data *mdm_data = data;

	/* Set the RESET_BB_N to 0 */
	gpio_set_value(rst, 0);

	/* Wait before doing the pulse on ON1 */
	usleep_range(mdm_data->pre_pwr_down_delay,
		     mdm_data->pre_pwr_down_delay);

	/* Here POWER ON is not used except for Modem2230 */
	UNREFERENCED_PARAMETER(pwr_on);
	return 0;
}

int mcd_mdm_get_cflash_delay(void *data)
{
	struct mdm_ctrl_mdm_data *mdm_data = data;
	return mdm_data->pre_cflash_delay;
}

int mcd_mdm_get_wflash_delay(void *data)
{
	struct mdm_ctrl_mdm_data *mdm_data = data;
	return mdm_data->pre_wflash_delay;
}

int mcd_mdm_cleanup(void *data)
{
	return 0;
}

/**
 *  mcd_mdm_cold_boot_ngff - Perform a NGFF modem cold boot sequence
 *  @drv: Reference to the driver structure
 *
 *  - Set to HIGH the RESET_BB_N
 *  - reset USB hub
 */
int mcd_mdm_cold_boot_ngff(void *data, int rst, int pwr_on)
{

	/* Toggle the RESET_BB_N */
	gpio_set_value(rst, 1);

	/* reset the USB hub here*/
	usleep_range(1000,1001);
	gpio_set_value(GPIO_RST_USBHUB,0);
	usleep_range(1000,1001);
	gpio_set_value(GPIO_RST_USBHUB,1);

	return 0;
}

/**
 *  mcd_mdm_cold_boot_2230 - Perform a cold boot boot for modem 2230
 *  @drv: Reference to the driver structure
 *
 *  - TODO: update time between toggling ON_KEY and POWER_ON
 */
int mcd_mdm_cold_boot_2230(void *data, int rst, int pwr_on, int on_key)
{
	struct mdm_ctrl_mdm_data *mdm_data = data;

	/* Toggle the RESET_BB_N */
	gpio_set_value(rst, 0);

	/* Toggle the POWER_ON */
	usleep_range(mdm_data->pre_on_delay, mdm_data->pre_on_delay);
	gpio_set_value_cansleep(pwr_on, 0);

	/* Toggle RESET_BB_N */
	usleep_range(mdm_data->on_duration, mdm_data->on_duration);
	gpio_set_value(rst, 1);

	/* Toggle POWER_ON */
	usleep_range(100000, 100000);
	gpio_set_value_cansleep(pwr_on, 1);

	/* Toggle ON_KEY */
	usleep_range(1000*1000, 1000*1000);
	gpio_set_value(on_key, 1);
	usleep_range(150000, 150000);
	gpio_set_value(on_key, 0);

	return 0;
}

/**
 *  mcd_mdm_power_off_2230 - Perform a power off for modem 2230
 *  @drv: Reference to the driver structure
 *
 *  - Set to LOW the PWRDWN_N
 *  - Set to LOW the PWR ON pin
 */
int mcd_mdm_power_off_2230(void *data, int rst, int pwr_on)
{
	struct mdm_ctrl_mdm_data *mdm_data = data;

	/* Set the RESET_BB_N to 0 */
	gpio_set_value(rst, 0);

	/* Wait before doing the pull down battery on */
	usleep_range(mdm_data->pre_pwr_down_delay,
		mdm_data->pre_pwr_down_delay);

	gpio_set_value_cansleep(pwr_on, 0);

	return 0;
}
