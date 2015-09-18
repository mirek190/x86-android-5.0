/*
 * intel_crystalcove_chgr.c - Intel Crystal Cove Charging Driver
 *
 * Copyright (C) 2013 Intel Corporation
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author: Li, Hui <hui.li1@intel.com>
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/usb/phy.h>
#include <linux/notifier.h>
#include <linux/extcon.h>
#include <linux/mfd/intel_mid_pmic.h>
#include <asm/intel_crystalcove_pwrsrc.h>
#include <linux/power/intel_crystalcove_chgr.h>

#define CRYSTALCOVE_CHGRIRQ_REG		0x0A
#define CRYSTALCOVE_MCHGRIRQS0_REG	0x17
#define CRYSTALCOVE_MCHGRIRQSX_REG	0x18
#define CRYSTALCOVE_VBUSCNTL_REG	0x6C

#define CHGR_IRQ_DET			(1 << 0)

#define CHGR_DRV_NAME			"crystal_cove_chgr"


#ifndef DEBUG
#define dev_dbg(dev, format, arg...)		\
	dev_printk(KERN_DEBUG, dev, format, ##arg)
#endif


	struct chgr_info {
	struct platform_device *pdev;
	struct crystalcove_chgr_pdata	*pdata;
	int irq;
	struct power_supply	psy_usb;
	struct mutex		lock;
	struct usb_phy		*otg;
	struct work_struct	otg_work;
	struct notifier_block	id_nb;
	bool			id_short;

	int chgr_health;
	int chgr_status;
	int cc;
	int cv;
	int inlmt;
	int max_cc;
	int max_cv;
	int iterm;
	int cable_type;
	int cntl_state;
	int max_temp;
	int min_temp;
	bool online;
	bool present;
	bool is_charging_enabled;
	bool is_charger_enabled;
};

static enum power_supply_property crystalcove_chrg_usb_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_MAX_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_MAX_CHARGE_VOLTAGE,
	POWER_SUPPLY_PROP_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CHARGE_VOLTAGE,
	POWER_SUPPLY_PROP_INLMT,
	POWER_SUPPLY_PROP_ENABLE_CHARGING,
	POWER_SUPPLY_PROP_ENABLE_CHARGER,
	POWER_SUPPLY_PROP_CHARGE_TERM_CUR,
	POWER_SUPPLY_PROP_CABLE_TYPE,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
	POWER_SUPPLY_PROP_MAX_TEMP,
	POWER_SUPPLY_PROP_MIN_TEMP,
};


static enum power_supply_type get_power_supply_type(
		enum power_supply_charger_cable_type cable)
{

	switch (cable) {

	case POWER_SUPPLY_CHARGER_TYPE_USB_DCP:
		return POWER_SUPPLY_TYPE_USB_DCP;
	case POWER_SUPPLY_CHARGER_TYPE_USB_CDP:
		return POWER_SUPPLY_TYPE_USB_CDP;
	case POWER_SUPPLY_CHARGER_TYPE_USB_ACA:
	case POWER_SUPPLY_CHARGER_TYPE_ACA_DOCK:
		return POWER_SUPPLY_TYPE_USB_ACA;
	case POWER_SUPPLY_CHARGER_TYPE_AC:
		return POWER_SUPPLY_TYPE_MAINS;
	case POWER_SUPPLY_CHARGER_TYPE_NONE:
	case POWER_SUPPLY_CHARGER_TYPE_USB_SDP:
	default:
		return POWER_SUPPLY_TYPE_USB;
	}

	return POWER_SUPPLY_TYPE_USB;
}
static int crystalcove_chgr_enable_charger(struct chgr_info *info, bool enable)
{
	int ret;

	dev_info(&info->pdev->dev, "crystalcove enable charger\n");

	if (enable)
		ret = crystal_cove_enable_vbus();
	else
		ret = crystal_cove_disable_vbus();
	return ret;
}

static int crystalcove_chgr_enable_charging(struct chgr_info *info, bool enable)
{
	int ret;

	dev_info(&info->pdev->dev, "crystalcove enable chargering\n");

	ret = crystalcove_chgr_enable_charger(info, true);
	if (ret < 0)
		dev_warn(&info->pdev->dev, "vbus disable failed\n");

	if (enable) {
		ret = intel_mid_pmic_writeb(CRYSTALCOVE_MCHGRIRQS0_REG, 0x00);
		if (ret < 0)
			dev_warn(&info->pdev->dev, "chgr mask disable failed\n");
		else
			ret = intel_mid_pmic_writeb(CRYSTALCOVE_MCHGRIRQSX_REG, 0x00);
	} else {
		ret = intel_mid_pmic_writeb(CRYSTALCOVE_MCHGRIRQS0_REG, 0x01);
		if (ret < 0)
			dev_warn(&info->pdev->dev, "chgr mask enable failed\n");
		else
			ret = intel_mid_pmic_writeb(CRYSTALCOVE_MCHGRIRQSX_REG, 0x01);
	}
	return ret;
}

static int get_charger_health(struct chgr_info *info)
{
	int ret, pwr_stat, chgr_stat, pwr_irq;
	int health = POWER_SUPPLY_HEALTH_UNKNOWN;

	dev_info(&info->pdev->dev, "crystalcove charger health\n");

	ret = crystal_cove_vbus_on_status();
	if ((ret < 0) || (ret == 0))
		goto health_read_fail;
	else
		pwr_stat = ret;

	ret = intel_mid_pmic_readb(CRYSTALCOVE_MCHGRIRQS0_REG);
	if (ret < 0)
		goto health_read_fail;
	else
		ret = intel_mid_pmic_readb(CRYSTALCOVE_MCHGRIRQSX_REG);
		if (ret < 0)
			goto health_read_fail;
		else
			pwr_irq = ret;

	if (pwr_stat == 1)
		health = POWER_SUPPLY_HEALTH_GOOD;

health_read_fail:
	return health;
}

static int get_charging_status(struct chgr_info *info)
{
	int stat = POWER_SUPPLY_STATUS_UNKNOWN;
	int vbus_stat, chgr_stat, chgr_irq_stat;

	dev_info(&info->pdev->dev, "crystalcove chargering status\n");

	vbus_stat = crystal_cove_vbus_on_status();
	if (vbus_stat < 0)
		goto chgr_stat_read_fail;

	chgr_stat = intel_mid_pmic_readb(CRYSTALCOVE_MCHGRIRQS0_REG);
	if (chgr_stat < 0)
		goto chgr_stat_read_fail;
	else
		chgr_stat = intel_mid_pmic_readb(CRYSTALCOVE_MCHGRIRQSX_REG);
	if (chgr_stat < 0)
		goto chgr_stat_read_fail;

	chgr_irq_stat = intel_mid_pmic_readb(CRYSTALCOVE_CHGRIRQ_REG);
	if (chgr_irq_stat < 0)
		goto chgr_stat_read_fail;

	if (vbus_stat != 1)
		stat = POWER_SUPPLY_STATUS_DISCHARGING;
	else if (vbus_stat == 1 && chgr_stat == 0)
		stat = POWER_SUPPLY_STATUS_CHARGING;
	else if (chgr_irq_stat == 1)
		stat = POWER_SUPPLY_STATUS_FULL;
	else
		stat = POWER_SUPPLY_STATUS_NOT_CHARGING;

chgr_stat_read_fail:
	return stat;
}

static int crystalcove_chgr_usb_set_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    const union power_supply_propval *val)
{
	struct chgr_info *info = container_of(psy,
						    struct chgr_info,
						    psy_usb);
	int ret = 0;

	mutex_lock(&info->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
		info->present = val->intval;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		info->online = val->intval;
		break;
	case POWER_SUPPLY_PROP_ENABLE_CHARGING:
		ret = crystalcove_chgr_enable_charging(info, val->intval);
		if (ret < 0)
			dev_warn(&info->pdev->dev, "crystalcove enable charging failed\n");

		info->is_charging_enabled = val->intval;
		break;
	case POWER_SUPPLY_PROP_ENABLE_CHARGER:

		ret = crystalcove_chgr_enable_charging(info, val->intval);
		if (ret < 0)
			dev_warn(&info->pdev->dev, "crystalcove enable charger failed\n");

		 info->is_charger_enabled = val->intval;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CURRENT:
		info->cc = val->intval;
		break;
	case POWER_SUPPLY_PROP_INLMT:
		info->inlmt = val->intval;
		break;
	case POWER_SUPPLY_PROP_CHARGE_VOLTAGE:
		info->cv = val->intval;
		break;
	case POWER_SUPPLY_PROP_MAX_CHARGE_CURRENT:
		info->max_cc = val->intval;
		break;
	case POWER_SUPPLY_PROP_MAX_CHARGE_VOLTAGE:
		info->max_cv = val->intval;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TERM_CUR:
		info->iterm = val->intval;
		break;
	case POWER_SUPPLY_PROP_CABLE_TYPE:
		info->cable_type = val->intval;
		info->psy_usb.type = get_power_supply_type(info->cable_type);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		info->cntl_state = val->intval;
		break;
	case POWER_SUPPLY_PROP_MAX_TEMP:
		info->max_temp = val->intval;
		break;
	case POWER_SUPPLY_PROP_MIN_TEMP:
		info->min_temp = val->intval;
		break;
	default:
		ret = -EINVAL;
	}

	mutex_unlock(&info->lock);
	return ret;
}

static int crystalcove_chgr_usb_get_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	struct chgr_info *info = container_of(psy,
				struct chgr_info, psy_usb);
	int ret = 0;

	mutex_lock(&info->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
		ret = crystal_cove_vbus_on_status();
		if (ret < 0)
			goto psy_get_prop_fail;
		val->intval = ret;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = info->online;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = get_charger_health(info);
		break;
	case POWER_SUPPLY_PROP_MAX_CHARGE_CURRENT:
		val->intval = info->max_cc;
		break;
	case POWER_SUPPLY_PROP_MAX_CHARGE_VOLTAGE:
		val->intval = info->max_cv;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CURRENT:
		val->intval = info->cc;
		break;
	case POWER_SUPPLY_PROP_CHARGE_VOLTAGE:
		val->intval = info->cv;
		break;
	case POWER_SUPPLY_PROP_INLMT:
		val->intval = info->inlmt;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TERM_CUR:
		val->intval = info->iterm;
		break;
	case POWER_SUPPLY_PROP_CABLE_TYPE:
		val->intval = info->cable_type;
		break;
	case POWER_SUPPLY_PROP_ENABLE_CHARGING:
		val->intval = info->is_charging_enabled;
		break;
	case POWER_SUPPLY_PROP_ENABLE_CHARGER:
		val->intval = info->is_charger_enabled;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		val->intval = info->cntl_state;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		val->intval = info->pdata->num_throttle_states;
		break;
	case POWER_SUPPLY_PROP_MAX_TEMP:
		val->intval = info->max_temp;
		break;
	case POWER_SUPPLY_PROP_MIN_TEMP:
		val->intval = info->min_temp;
		break;
	default:
		mutex_unlock(&info->lock);
		return -EINVAL;
	}

psy_get_prop_fail:
	mutex_unlock(&info->lock);
	return ret;
}

static irqreturn_t crystalcove_chgr_isr(int irq, void *data)
{
	struct chgr_info *info = data;
	int chgrirq;

	chgrirq = intel_mid_pmic_readb(CRYSTALCOVE_CHGRIRQ_REG);

	if (chgrirq < 0) {
		dev_err(&info->pdev->dev, "CHGRIRQ read failed\n");
		goto pmic_irq_fail;
	}

	dev_dbg(&info->pdev->dev, "crystalcove charger irq happens, chgrirq=%x\n", chgrirq);

/*--------------CHGRIRQ HANDLER--------------*/
	power_supply_changed(&info->psy_usb);


pmic_irq_fail:
	intel_mid_pmic_writeb(CRYSTALCOVE_CHGRIRQ_REG, chgrirq);

	return IRQ_HANDLED;
}

static void crystalcove_otg_event_worker(struct work_struct *work)
{
	struct chrg_info *info = container_of(work, struct chgr_info, otg_work);
/*	int ret;*/

	info = info;
}
static int crystalcove_handle_otg_event(struct notifier_block *nb,
				   unsigned long event, void *param)
{
	struct chgr_info *info =
	    container_of(nb, struct chgr_info, id_nb);
	int *val = (int *)param;

	if (!val || (event != USB_EVENT_ID))
		return NOTIFY_DONE;

	dev_info(&info->pdev->dev,
		"[OTG notification]evt:%lu val:%d\n", event, *val);

	switch (event) {
	case USB_EVENT_ID:
		/*
		 * in case of ID short(*id = 0)
		 * enable vbus else disable vbus.
		 */
		if (*val)
			info->id_short = false;
		else
			info->id_short = true;
		schedule_work(&info->otg_work);
		break;
	default:
		dev_warn(&info->pdev->dev, "invalid OTG event\n");
	}

	return NOTIFY_OK;
}

extern void *crystalcove_chgr_pdata(void *info);

static int crystalcove_chgr_probe(struct platform_device *pdev)
{
	struct chgr_info *info;
	int ret;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "mem alloc failed\n");
		return -ENOMEM;
	}

	dev_info(&pdev->dev, "crystalcove charger probe begin\n");

	info->pdev = pdev;
#ifdef CONFIG_ACPI
	info->pdata = crystalcove_chgr_pdata(NULL);
#else
	info->pdata = pdev->dev.platform_data;
#endif

	platform_set_drvdata(pdev, info);

	mutex_init(&info->lock);
	INIT_WORK(&info->otg_work, crystalcove_otg_event_worker);

	info->irq = platform_get_irq(pdev, 0);

	ret = request_threaded_irq(info->irq, NULL, crystalcove_chgr_isr,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT, CHGR_DRV_NAME, info);
	if (ret) {
		dev_err(&pdev->dev, "unable to register irq %d\n", info->irq);
		goto psy_reg_failed;
	}

	info->otg = usb_get_phy(USB_PHY_TYPE_USB2);

	if (!info->otg) {
		dev_warn(&pdev->dev, "Failed to get otg transceiver!!\n");
	} else {
		info->id_nb.notifier_call = crystalcove_handle_otg_event;
		ret = usb_register_notifier(info->otg, &info->id_nb);
		if (ret)
			dev_err(&pdev->dev, "failed to register otg notifier\n");
	}

	info->max_cc = info->pdata->max_cc;
	info->max_cv = info->pdata->max_cv;

	info->psy_usb.name = CHGR_DRV_NAME;
	info->psy_usb.type = POWER_SUPPLY_TYPE_USB;
	info->psy_usb.properties = crystalcove_chrg_usb_props;
	info->psy_usb.num_properties = ARRAY_SIZE(crystalcove_chrg_usb_props);
	info->psy_usb.get_property = crystalcove_chgr_usb_get_property;
	info->psy_usb.set_property = crystalcove_chgr_usb_set_property;
	info->psy_usb.supplied_to = info->pdata->supplied_to;
	info->psy_usb.num_supplicants = info->pdata->num_supplicants;
	info->psy_usb.throttle_states = info->pdata->throttle_states;
	info->psy_usb.num_throttle_states = info->pdata->num_throttle_states;
	info->psy_usb.supported_cables = info->pdata->supported_cables;

	ret = power_supply_register(&pdev->dev, &info->psy_usb);

	if (ret) {
		dev_err(&pdev->dev, "Failed: power supply register (%d)\n",
			ret);
		goto psy_reg_failed;
	}

	/* unmask the CHGR interrupts */
	intel_mid_pmic_writeb(CRYSTALCOVE_MCHGRIRQS0_REG, 0x00);
	intel_mid_pmic_writeb(CRYSTALCOVE_MCHGRIRQSX_REG, 0x00);
	return 0;
psy_reg_failed:
	return ret;
}

static int crystalcove_chgr_remove(struct platform_device *pdev)
{
	struct chgr_info *info =  dev_get_drvdata(&pdev->dev);
	int i;

	free_irq(info->irq, info);
	if (info->otg)
		usb_put_phy(info->otg);
	power_supply_unregister(&info->psy_usb);
	return 0;
}

static int crystalcove_chgr_suspend(struct device *dev)
{
	return 0;
}

static int crystalcove_chgr_resume(struct device *dev)
{
	return 0;
}

static int crystalcove_chgr_runtime_suspend(struct device *dev)
{

	dev_dbg(dev, "%s called\n", __func__);
	return 0;
}

static int crystalcove_chgr_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "%s called\n", __func__);
	return 0;
}

static int crystalcove_chgr_runtime_idle(struct device *dev)
{

	dev_dbg(dev, "%s called\n", __func__);
	return 0;
}


static const struct dev_pm_ops crystalcove_chgr_driver_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(crystalcove_chgr_suspend,
					crystalcove_chgr_resume)
			SET_RUNTIME_PM_OPS(crystalcove_chgr_runtime_suspend,
					crystalcove_chgr_runtime_resume,
					crystalcove_chgr_runtime_idle)

};

static struct platform_driver crystalcove_chgr_driver = {
	.probe = crystalcove_chgr_probe,
	.remove = crystalcove_chgr_remove,
	.driver = {
		.name = CHGR_DRV_NAME,
		.owner = THIS_MODULE,
		.pm = &crystalcove_chgr_driver_pm_ops,
	},
};

static int __init crystalcove_chgr_init(void)
{
	return platform_driver_register(&crystalcove_chgr_driver);
}
module_init(crystalcove_chgr_init);

static void __exit crystalcove_chgr_exit(void)
{
	platform_driver_unregister(&crystalcove_chgr_driver);
}
module_exit(crystalcove_chgr_exit);

MODULE_AUTHOR("Kannappan R <r.kannappan@intel.com>");
MODULE_AUTHOR("Ramakrishna Pallala <ramakrishna.pallala@intel.com>");
MODULE_DESCRIPTION("CrystalCove Charger Driver");
MODULE_LICENSE("GPL");

