#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/usb/otg.h>
#include <linux/notifier.h>
#include <linux/power_supply.h>
#include <linux/idr.h>
#include <linux/slab.h>
#include <asm/unaligned.h>
#include <linux/notifier.h>
#include <linux/gpio.h>
#include <linux/acpi.h>
#include <linux/mfd/intel_mid_pmic.h>
#include <linux/power/dc_xpwr_charger.h>
#include <linux/usb/dwc3-intel-mid.h>
#define DEV_NAME "vbus_warn"

struct vbus_warn_info {
	struct platform_device *pdev;
	struct notifier_block id_nb;
	struct usb_phy *otg;
	struct work_struct otg_work;
	int id_short;
};

static struct platform_device vbus_warn_device = {
	.name = DEV_NAME,
	.id = -1,
};

static struct platform_device *vbus_warn_devices[] __initdata = {
	&vbus_warn_device,
};

static void vbus_warn_otg_event_worker(struct work_struct *work)
{
	struct vbus_warn_info *info =
	    container_of(work, struct vbus_warn_info, otg_work);
	char *plug_in[] = { "otg_plug=1", NULL };
	char *plug_out[] = { "otg_plug=0", NULL };
	if (info->id_short)
		kobject_uevent_env(&info->pdev->dev.kobj, KOBJ_CHANGE, plug_in);
	else
		kobject_uevent_env(&info->pdev->dev.kobj,
				   KOBJ_CHANGE, plug_out);
}

static int vbus_warn_handle_otg_event(struct notifier_block *nb,
				      unsigned long event, void *param)
{
	struct vbus_warn_info *info =
	    container_of(nb, struct vbus_warn_info, id_nb);
	int *val = (int *)param;

	if (!val || (event != USB_EVENT_ID))
		return NOTIFY_DONE;
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
		break;
	}

	return NOTIFY_OK;

}

static int vbus_warn_probe(struct platform_device *pdev)
{
	struct vbus_warn_info *info;
	int ret;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "mem alloc failed\n");
		return -ENOMEM;
	}
	info->pdev = pdev;
	INIT_WORK(&info->otg_work, vbus_warn_otg_event_worker);
	info->otg = usb_get_phy(USB_PHY_TYPE_USB2);
	if (!info->otg) {
		dev_warn(&pdev->dev, "Failed to get otg transceiver!!\n");
	} else {
		info->id_nb.notifier_call = vbus_warn_handle_otg_event;
		ret = usb_register_notifier(info->otg, &info->id_nb);
		if (ret)
			dev_err(&pdev->dev,
				"failed to register otg notifier\n");
	}
	dev_info(&pdev->dev, "vbus warn probe finished\n");
	return 0;
}

static int vbus_warn_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver vbus_warn_driver = {
	.driver = {
		   .name = DEV_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = vbus_warn_probe,
	.remove = vbus_warn_remove,
};

static int __init vbus_warn_init(void)
{
	platform_add_devices(vbus_warn_devices, ARRAY_SIZE(vbus_warn_devices));
	return platform_driver_register(&vbus_warn_driver);

}

late_initcall(vbus_warn_init);

static void __exit vbus_warn_exit(void)
{
	platform_driver_unregister(&vbus_warn_driver);
}

module_exit(vbus_warn_exit);
