/*
 * platform_crystalcove_charger.c: crystalcove charger platform data initilization file
 */

#include <linux/gpio.h>
#include <asm/intel-mid.h>
#include <linux/lnw_gpio.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/power/intel_crystalcove_chgr.h>
#include <asm/intel_em_config.h>

static struct crystalcove_chgr_pdata pdata;

#define CC_CHRG_CHRG_CUR_NOLIMIT	1800
#define CC_CHRG_CHRG_CUR_MEDIUM		1400
#define CC_CHRG_CHRG_CUR_LOW		1000

static struct ps_batt_chg_prof ps_cc_batt_chgr_prof;
static struct ps_pse_mod_prof cc_batt_chgr_profile;
static struct power_supply_throttle cc_chgr_throttle_states[] = {
	{
		.throttle_action = PSY_THROTTLE_CC_LIMIT,
		.throttle_val = CC_CHRG_CHRG_CUR_NOLIMIT,
	},
	{
		.throttle_action = PSY_THROTTLE_CC_LIMIT,
		.throttle_val = CC_CHRG_CHRG_CUR_MEDIUM,
	},
	{
		.throttle_action = PSY_THROTTLE_CC_LIMIT,
		.throttle_val = CC_CHRG_CHRG_CUR_LOW,
	},
	{
		.throttle_action = PSY_THROTTLE_DISABLE_CHARGING,
	},
};

char *cc_supplied_to[] = {
	"max17047_battery",
};


static void *platform_get_batt_charge_profile(void)
{
	if (!em_config_get_charge_profile(&cc_batt_chgr_profile))
		ps_cc_batt_chgr_prof.chrg_prof_type = CHRG_PROF_NONE;
	else
		ps_cc_batt_chgr_prof.chrg_prof_type = PSE_MOD_CHRG_PROF;

	ps_cc_batt_chgr_prof.batt_prof = &cc_batt_chgr_profile;
	battery_prop_changed(POWER_SUPPLY_BATTERY_INSERTED,
			&ps_cc_batt_chgr_prof);

	return &ps_cc_batt_chgr_prof;
}

static void *platform_init_chrg_params(void)
{
/*no use currently*/
	pdata.max_cc = 2000;
	pdata.max_cv = 4350;
	pdata.def_cc = 500;
	pdata.def_cv = 4350;
	pdata.def_ilim = 900;
	pdata.def_iterm = 300;
	pdata.def_max_temp = 55;
	pdata.def_min_temp = 0;
	pdata.throttle_states = cc_chgr_throttle_states;
	pdata.supplied_to = cc_supplied_to;
	pdata.num_throttle_states = ARRAY_SIZE(cc_chgr_throttle_states);
	pdata.num_supplicants = ARRAY_SIZE(cc_supplied_to);
	pdata.supported_cables = POWER_SUPPLY_CHARGER_TYPE_USB;
	pdata.chg_profile = (struct ps_batt_chg_prof *)
			platform_get_batt_charge_profile();
	return &pdata;
}

void *crystalcove_chgr_pdata(void *info)
{

	return platform_init_chrg_params();
}
