/*
 *  xpwr_vib.c - xpwr CHGLED vibrator
 *
 *  Copyright (C) 2011-13 Intel Corp
 *  Author: Zhang Ning <ning.a.zhang@intel.com>
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt


#include <linux/platform_device.h>
#include <linux/mfd/intel_mid_pmic.h>
#include <linux/stat.h>
#include <linux/slab.h>
#include <linux/module.h>

#define XPWR_CHGLED	0x32
#define XPWR_CHGLED_CTRL (0x3<<4)
#define CHGMODE_0	(0x0<<4) /*Hi-z*/
#define CHGMODE_1	(0x1<<4) /*25% 0.5HZ*/
#define CHGMODE_2	(0x2<<4) /*25% 2HZ*/
#define CHGMODE_3	(0x3<<4) /*low*/

struct vibra_info {
	bool   enabled;
	int	duty;
	struct mutex	lock;
	const char	*name;
	struct device	*dev;
};

/* Enable's vibra driver */
static void vibra_enable(struct vibra_info *info)
{
	int mode;
	switch (info->duty) {
	case 0:
		mode = CHGMODE_1;
		break;
	case 1:
		mode = CHGMODE_2;
		break;
	case 3:
		mode = CHGMODE_3;
		break;
	default:
		mode = CHGMODE_3;
	}
	pr_debug("Enable vib\n");
	mutex_lock(&info->lock);

	intel_mid_pmic_writeb(XPWR_CHGLED, intel_mid_pmic_readb(XPWR_CHGLED)|mode);

	info->enabled = true;
	mutex_unlock(&info->lock);
}

static void vibra_disable(struct vibra_info *info)
{
	pr_debug("Disable vib\n");
	mutex_lock(&info->lock);

	intel_mid_pmic_writeb(XPWR_CHGLED, intel_mid_pmic_readb(XPWR_CHGLED)&(~XPWR_CHGLED_CTRL));

	info->enabled = false;
	mutex_unlock(&info->lock);
}

/*******************************************************************************
 * SYSFS                                                                       *
 ******************************************************************************/

static ssize_t vibra_show_vibrator(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vibra_info *info = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", info->enabled);

}

static ssize_t vibra_set_vibrator(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	long vibrator_enable;
	struct vibra_info *info = dev_get_drvdata(dev);

	if (kstrtol(buf, 0, &vibrator_enable))
		return -EINVAL;
	if (vibrator_enable == info->enabled)
		return len;
	else if (vibrator_enable == 0)
		vibra_disable(info);
	else if (vibrator_enable == 1)
		vibra_enable(info);
	else
		return -EINVAL;
	return len;
}

static ssize_t vibra_show_duty(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vibra_info *info = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", info->duty);

}

static ssize_t vibra_set_duty(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	long vibrator_duty;
	struct vibra_info *info = dev_get_drvdata(dev);

	if (kstrtol(buf, 0, &vibrator_duty))
		return -EINVAL;
	if (vibrator_duty == info->duty)
		return len;
	else
		 info->duty = vibrator_duty;
	return len;
}

static struct device_attribute vibra_attrs[] = {
	__ATTR(vibrator, S_IRUGO | S_IWUSR,
	       vibra_show_vibrator, vibra_set_vibrator),
	__ATTR(duty, S_IRUGO | S_IWUSR,
	       vibra_show_duty, vibra_set_duty),
};

static int vibra_register_sysfs(struct vibra_info *info)
{
	int r, i;

	for (i = 0; i < ARRAY_SIZE(vibra_attrs); i++) {
		r = device_create_file(info->dev, &vibra_attrs[i]);
		if (r)
			goto fail;
	}
	return 0;
fail:
	while (i--)
		device_remove_file(info->dev, &vibra_attrs[i]);

	return r;
}

static void vibra_unregister_sysfs(struct vibra_info *info)
{
	int i;

	for (i = ARRAY_SIZE(vibra_attrs) - 1; i >= 0; i--)
		device_remove_file(info->dev, &vibra_attrs[i]);
}

static int xpwr_vib_platform_probe(struct platform_device *pdev)
{
	struct vibra_info *info;
	int ret = 0;

	info =  devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->dev = &pdev->dev;
	info->name = "intel_mid:vibrator";
	info->duty = 3; /* default to low */
	mutex_init(&info->lock);
	ret = vibra_register_sysfs(info);
	if (ret < 0) {
		pr_err("could not register sysfs files\n");
		kfree(info);
		return ret;
	}
	dev_set_drvdata(&pdev->dev, info);
	return ret;

}

static int xpwr_vib_platform_remove(struct platform_device *pdev)
{
	struct vibra_info *info = dev_get_drvdata(&pdev->dev);
	vibra_unregister_sysfs(info);
	kfree(info);
	return 0;
}

static struct platform_driver xpwr_vib_platform_driver = {
	.driver		= {
		.name		= "xpwr_vib",
		.owner		= THIS_MODULE,
	},
	.probe		= xpwr_vib_platform_probe,
	.remove		= xpwr_vib_platform_remove,
};

module_platform_driver(xpwr_vib_platform_driver);


MODULE_DESCRIPTION("XPWR Vibra driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Zhang Ning<ning.a.zhang@intel.com>");

