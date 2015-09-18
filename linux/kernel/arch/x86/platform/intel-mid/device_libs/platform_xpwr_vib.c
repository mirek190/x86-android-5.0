#include <linux/platform_device.h>
static struct platform_device xpwr_vib_device = {
	.name = "xpwr_vib",
	.id = 0,
	.dev = {},
};
static struct platform_device *xpwr_vib_devices[] __initdata = {
	&xpwr_vib_device,
};

static int __init xpwr_vib_platform_init(void)
{
	platform_add_devices(xpwr_vib_devices, ARRAY_SIZE(xpwr_vib_devices));
	return 0;
}
device_initcall(xpwr_vib_platform_init);
