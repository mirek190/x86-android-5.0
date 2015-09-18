/*
 * Copyright 2011 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <droidboot.h>
#include <droidboot_plugin.h>
#include <droidboot_util.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <cutils/properties.h>
#include <cutils/android_reboot.h>
#include <unistd.h>
#include <charger/charger.h>
#include <linux/ioctl.h>
#include <sys/mount.h>

#include "volumeutils/ufdisk.h"
#include "update_osip.h"
#include "util.h"
#include "fw_version_check.h"
#include "fastboot.h"
#include "droidboot_ui.h"
#include "oem_partition.h"
#include "flash.h"
#include "ulpmc.h"

#ifdef TEE_FRAMEWORK
#include "tee_connector.h"
#endif	/* TEE_FRAMEWORK */

#ifndef EXTERNAL
#include "pmdb.h"
#include "token.h"
#endif

#define PARAMETER_VALUE_SIZE			20
#define OPTION_SIZE				4

enum {
	TIMEOUT_INFINITE = 0x3F,
	TIMEOUT_TEN = 0X2D,
	TIMEOUT_THREE = 0x1B,
	TIMEOUT_ZERO = 0X00
};

static int oem_custom_boot(int argc, char **argv)
{
	int retval = -1;
	int size;
	unsigned int value;
	char custom_boot_value[PARAMETER_VALUE_SIZE] = "";

	if (argc != 2) {
		fastboot_fail("oem custom_boot requires one argument");
		goto out;
	}

	size = snprintf(custom_boot_value, PARAMETER_VALUE_SIZE, "%s", argv[1]);
	if (size < 0 || size > PARAMETER_VALUE_SIZE - 1) {
		fastboot_fail("Parameter size exceeds limit");
		goto out;
	}

	value = strtoul(custom_boot_value, NULL, 0);
	if (EINVAL == value || ERANGE == value) {
		fastboot_fail("Unable to read parameter");
		goto out;
	}

	retval = flash_custom_boot((void *)&value, sizeof(value));
	if (retval != 0) {
		fastboot_fail("Fail write_umip");
		goto out;
	}
	fastboot_okay("");

out:
	return retval;
}

static int oem_erase_token(int argc, char **argv)
{
	int retval = -1;

	if (argc > 1) {
		fastboot_fail("oem_erase_token requires no argument");
		goto out;
	}

	retval = erase_token_umip();
	if (retval != 0) {
		fastboot_fail("Fail erase_token_umip");
		goto out;
	}
	fastboot_okay("");

out:
	return retval;
}

static int oem_dnx_timeout(int argc, char **argv)
{
	int retval = -1;
	int size;
	unsigned int value;
	char dnx_timeout_value[PARAMETER_VALUE_SIZE] = "";
	char option[OPTION_SIZE] = "";

	if (argc < 2 || argc > 3) {
		fastboot_fail("oem_dnx_timeout requires one or two arguments");
		goto out;
	}

	size = snprintf(option, OPTION_SIZE, "%s", argv[1]);
	if (size < 0 || size > OPTION_SIZE - 1) {
		fastboot_fail("Option size exceeds limit");
		goto out;
	}

	if (!strcmp(option, "get")) {
		/* Get current timeout */
		value = read_dnx_timeout();

		switch (value)
		{
			case TIMEOUT_INFINITE:
				ui_print("infinite\n");
				break;
			case TIMEOUT_TEN:
				ui_print("tenseconds\n");
				break;
			case TIMEOUT_THREE:
				ui_print("threeseconds\n");
				break;
			case TIMEOUT_ZERO:
				ui_print("zeroseconds\n");
				break;
			default:
				fastboot_fail("invalid timeout value");
				goto out;
		}
		retval = 0;
	} else if (!strcmp(option, "set")) {
		/* Set new timeout */
		if (argc != 3) {
			fastboot_fail("oem_dnx_timeout set not enough arguments");
			goto out;
		}

		size = snprintf(dnx_timeout_value, PARAMETER_VALUE_SIZE, "%s", argv[2]);
		if (size < 0 || size > PARAMETER_VALUE_SIZE - 1) {
			fastboot_fail("Parameter size exceeds limit");
			goto out;
		}

		if (!strcmp("infinite", dnx_timeout_value))
			value = TIMEOUT_INFINITE;
		else if (!strcmp("tenseconds", dnx_timeout_value))
			value = TIMEOUT_TEN;
		else if (!strcmp("threeseconds", dnx_timeout_value))
			value = TIMEOUT_THREE;
		else if (!strcmp("zeroseconds", dnx_timeout_value))
			value = TIMEOUT_ZERO;
		else {
			fastboot_fail("invalid value, shall be infinite,"
				" tenseconds, threeseconds or zeroseconds");
			goto out;
		}

		retval = flash_dnx_timeout((void *)&value, sizeof(value));
		if (retval != 0) {
			fastboot_fail("Fail write_umip");
			goto out;
		}
	} else {
		fastboot_fail("Option shall be get or set");
		goto out;
	}
	fastboot_okay("");

out:
	return retval;
}

#define K_MAX_LINE_LEN 8192
#define K_MAX_ARGS 256
#define K_MAX_ARG_LEN 256

#ifndef EXTERNAL
static int wait_property(char *prop, char *value, int timeout_sec)
{
	int i;
	char v[PROPERTY_VALUE_MAX];

	for (i = 0; i < timeout_sec; i++) {
		property_get(prop, v, NULL);
		if (!strcmp(v, value))
			return 0;
		sleep(1);
	}
	return -1;
}

static int oem_backup_factory(int argc, char **argv)
{
	int len;
	char value[PROPERTY_VALUE_MAX];

	len = property_get("sys.backup_factory", value, NULL);
	if (strcmp(value, "done") && len) {
		fastboot_fail("Factory partition backing up failed!\n");
		return -1;
	}

	property_set("sys.backup_factory", "backup");
	ui_print("Backing up factory partition...\n");
	if (wait_property("sys.backup_factory", "done", 60)) {
		fastboot_fail("Factory partition backing up timeout!\n");
		return -1;
	}

	return 0;
}

static int oem_restore_factory(int argc, char **argv)
{
	char value[PROPERTY_VALUE_MAX];

	property_get("sys.backup_factory", value, NULL);
	if (strcmp(value, "done")) {
		fastboot_fail("Factory partition restoration failed!\n");
		return -1;
	}

	property_set("sys.backup_factory", "restore");
	ui_print("Restoring factory partition...\n");
	if (wait_property("sys.backup_factory", "done", 60)) {
		fastboot_fail("Factory partition restore timeout!\n");
		return -1;
	}

	return 0;
}
#endif	/* EXTERNAL */

static int oem_get_batt_info_handler(int argc, char **argv)
{
	char msg_buf[] = " level: 000";
	int batt_level = 0;

	batt_level = get_battery_level();
	if (batt_level == -1) {
		fastboot_fail("Could not get battery level");
		return -1;
	}
	// Prepare the message sent to the host
	snprintf(msg_buf, sizeof(msg_buf), "\nlevel: %d", batt_level);
	// Push the value to the host
	fastboot_info(msg_buf);
	// Display the result on the UI
	ui_print("Battery level at %d%%\n", batt_level);

	return 0;
}

#ifndef EXTERNAL
static int oem_fru_handler(int argc, char **argv)
{
	int ret = -1;
	char *str;
	char tmp[3];
	char fru[PMDB_FRU_SIZE];
	int i;

	if (argc != 3) {
		fastboot_fail("oem fru must be called with \"set\" subcommand\n");
		goto out;
	}

	if (strcmp(argv[1], "set")) {
		fastboot_fail("unknown oem fru subcommand\n");
		goto out;
	}

	str = argv[2];
	if (strlen(str) != (PMDB_FRU_SIZE * 2)) {
		fastboot_fail("fru value must be 20 4-bits nibbles in hexa format. Ex: 123456...\n");
		goto out;
	}

	tmp[2] = 0;
	for (i = 0; i < PMDB_FRU_SIZE; i++) {
		/* FRU is passed by 4bits nibbles. Need to reorder them into hex values. */
		tmp[0] = str[2 * i + 1];
		tmp[1] = str[2 * i];
		if (!is_hex(tmp[0]) || !is_hex(tmp[1]))
			fastboot_fail("fru value have non hexadecimal characters\n");
		sscanf(tmp, "%2hhx", &fru[i]);
	}
	ret = pmdb_write_fru(fru, PMDB_FRU_SIZE);

out:
	return ret;
}
#endif	/* EXTERNAL */

#define CONFIG_FILE "/mnt/config/local_config"

/* argv[1] : config name */
static int oem_config(int argc, char **argv)
{
	int ret = 0;
	int written;
	int len;
	FILE *f;

	if (argc != 2) {
		fastboot_fail("oem config must be called with 1 parameter\n");
		return -1;
	}

	if ((f = fopen(CONFIG_FILE, "w")) == NULL) {
		LOGE("open %s error!\n", CONFIG_FILE);
		return -1;
	}

	len = strlen(argv[1]);
	written = fwrite(argv[1], 1, len, f);
	if (written != len) {
		LOGE("write %s error: %d\n", CONFIG_FILE, ferror(f));
		ret = -1;
	}

	fclose(f);

	return ret;
}

#ifndef EXTERNAL
static int oem_fastboot2adb(int argc, char **argv)
{
	char value[PROPERTY_VALUE_MAX];
	int len = 0;
	int ret = -1;

	len = property_get("ro.debuggable", value, NULL);
	if ((len != 0) && (strcmp(value, "1") == 0)) {
		fastboot_okay("");
		ret = property_set("sys.adb.config", "fastboot2adb");
	} else {
		fastboot_fail("property ro.debuggable must be set to activate adb.");
	}
	return ret;
}
#endif	/* EXTERNAL */

static int oem_reboot(int argc, char **argv)
{
	char *target_os;

	switch (argc) {
	case 1:
		target_os = "android";
		break;
	case 2:
		target_os = argv[1];
		break;
	default:
		LOGE("reboot command take zero on one argument only\n");
		fastboot_fail("Usage: reboot [target_os]");
		return -EINVAL;
	}

	fastboot_okay("");
	sync();

	ui_print("REBOOT in %s...\n", target_os);
	pr_info("Rebooting in %s !\n", target_os);
	return android_reboot(ANDROID_RB_RESTART2, 0, target_os);
}

static int oem_mount(int argc, char **argv)
{
	int ret = 0;
	char *dev_path = NULL;
	char *mountpoint = NULL;

	/* Check parameters */
	if (argc != 3) {
		LOGE("fastboot mount command takes two parameters\n");
		fastboot_fail("Usage: mount <partition_name> <fs_type>");
		return -EINVAL;
	}

	ret = get_device_path(&dev_path, argv[1]);
	if (ret) {
		fastboot_fail("Unable to find the appropriate device path\n");
		goto end;
	}

	/* mount in a sub-folder of /mnt */
	ret = asprintf(&mountpoint, "/mnt/%s", argv[1]);
	if (ret < 0) {
		fastboot_fail("asprintf mountpoint failed");
		goto end;
	}
	LOGI("Mounting partition %s in %s (type %s)\n", dev_path, mountpoint, argv[2]);

	ret = mkdir(mountpoint, S_IRWXU | S_IRWXG | S_IRWXO);
	if (ret == -1 && errno != EEXIST) {
		fastboot_fail("mkdir failed");
		LOGE("mkdir failed : %s\n", strerror(errno));
		ret = -errno;
		goto end;
	}

	ret = mount(dev_path, mountpoint, argv[2], MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
	if (ret == -1) {
		fastboot_fail("mount failed");
		LOGE("mount failed : %s\n", strerror(errno));
		ret = -errno;
		goto end;
	}

end:
	free(dev_path);
	free(mountpoint);

	return ret;
}

#ifdef USE_GUI
#define PROP_FILE					"/default.prop"
#define SERIAL_NUM_FILE			"/sys/class/android_usb/android0/iSerial"
#define MAX_NAME_SIZE			128
#define BUF_SIZE					256

static char *strupr(char *str)
{
	char *p = str;
	while (*p != '\0') {
		*p = toupper(*p);
		p++;
	}
	return str;
}

static int read_from_file(char *file, char *attr, char *value)
{
	char *p;
	char buf[BUF_SIZE];
	FILE *f;

	if ((f = fopen(file, "r")) == NULL) {
		LOGE("open %s error!\n", file);
		return -1;
	}
	while (fgets(buf, BUF_SIZE, f)) {
		if ((p = strstr(buf, attr)) != NULL) {
			p += strlen(attr) + 1;
			strncpy(value, p, MAX_NAME_SIZE);
			value[MAX_NAME_SIZE - 1] = '\0';
			strupr(value);
			break;
		}
	}

	fclose(f);
	return 0;
}

static int get_system_info(int type, char *info, unsigned sz)
{
	int ret = -1;
	FILE *f;
	char value[PROPERTY_VALUE_MAX];

	switch (type) {
	case IFWI_VERSION:
		property_get("sys.ifwi.version", value, "");
		snprintf(info, sz, "%s", value);
		ret = 0;
		break;
	case PRODUCT_NAME:
		property_get("ro.product.device", value, "");
		snprintf(info, sz, "%s", value);
		ret = 0;
		break;
	case SERIAL_NUM:
		if ((f = fopen(SERIAL_NUM_FILE, "r")) == NULL)
			break;
		if (fgets(info, sz, f) == NULL) {
			fclose(f);
			break;
		}
		fclose(f);
		ret = 0;
		break;
	case VARIANT:
		property_get("ro.product.model", value, "");
		snprintf(info, sz, "%s", value);
		ret = 0;
		break;
	case HW_VERSION:
		property_get("sys.hw.version", value, "");
		snprintf(info, sz, "%s", value);
		ret = 0;
		break;
	case BOOTLOADER_VERSION:
		property_get("ro.bootloader", value, "");
		snprintf(info, sz, "%s", value);
		ret = 0;
		break;
	default:
		break;
	}

	return ret;
}
#endif

static void cmd_intel_reboot(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	// This will cause a property trigger in init.rc to cold boot
	property_set("sys.forcecoldboot", "yes");
	sync();
	ui_print("REBOOT...\n");
	pr_info("Rebooting!\n");
	android_reboot(ANDROID_RB_RESTART2, 0, "android");
	pr_error("Reboot failed");
}

static void cmd_intel_reboot_bootloader(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	// No cold boot as it would not allow to reboot in bootloader
	sync();
	ui_print("REBOOT in BOOTLOADER...\n");
	pr_info("Rebooting in BOOTLOADER !\n");
	android_reboot(ANDROID_RB_RESTART2, 0, "bootloader");
	pr_error("Reboot failed");
}

static void cmd_intel_boot(const char *arg, void *data, unsigned sz)
{
	ui_print("boot command stubbed on this platform!\n");
	pr_info("boot command stubbed on this platform!\n");
	fastboot_okay("");
}

struct property_format {
	const char *name;
	const char *msg;
	const char *error_msg;
};

static struct property_format properties_format[] = {
	{"sys.ifwi.version", "         ifwi:", NULL},
	{NULL, "---- components ----", NULL},
	{"sys.scu.version", "          scu:", NULL},
	{"sys.punit.version", "        punit:", NULL},
	{"sys.valhooks.version", "    hooks/oem:", NULL},
	{"sys.ia32.version", "         ia32:", NULL},
	{"sys.suppia32.version", "     suppia32:", NULL},
	{"sys.mia.version", "          mIA:", NULL},
	{"sys.chaabi.version", "       chaabi:", "CHAABI versions unreadable at runtime"},
};

static void dump_system_versions()
{
	char property[PROPERTY_VALUE_MAX];
	unsigned int i;

	for (i = 0; i < sizeof(properties_format) / sizeof((properties_format)[0]); i++) {
		struct property_format *fmt = &properties_format[i];

		if (!fmt->name) {
			printf("%s\n", fmt->msg);
			continue;
		}

		property_get(fmt->name, property, "");
		if (strcmp(property, ""))
			printf("%s %s\n", fmt->msg, property);
		else if (fmt->error_msg)
			printf("%s\n", fmt->error_msg);
	}
}

#ifdef TEE_FRAMEWORK
static int aboot_write_token(void *data, unsigned size)
{
	return write_token(data, (size_t)size);
}
#endif

void libintel_droidboot_init(void)
{
	int ret = 0;
	char build_type_prop[PROPERTY_VALUE_MAX] = { '\0', };
	char platform_prop[PROPERTY_VALUE_MAX] = { '\0', };
	struct ufdisk ufdisk = {
		.umount_all = ufdisk_umount_all,
		.create_partition = ufdisk_create_partition
	};

	property_get("ro.board.platform", platform_prop, '\0');
	property_get("ro.build.type", build_type_prop, '\0');

	oem_partition_init(&ufdisk);
	util_init(fastboot_fail, fastboot_info);
	ret |= aboot_register_flash_cmd(TEST_OS_NAME, flash_testos);
	ret |= aboot_register_flash_cmd(ANDROID_OS_NAME, flash_android_kernel);
	ret |= aboot_register_flash_cmd(RECOVERY_OS_NAME, flash_recovery_kernel);
	ret |= aboot_register_flash_cmd(FASTBOOT_OS_NAME, flash_fastboot_kernel);
	ret |= aboot_register_flash_cmd(ESP_PART_NAME, flash_esp);
	ret |= aboot_register_flash_cmd(SPLASHSCREEN_NAME, flash_splashscreen_image1);
	ret |= aboot_register_flash_cmd(SPLASHSCREEN_NAME1, flash_splashscreen_image1);
	ret |= aboot_register_flash_cmd(SPLASHSCREEN_NAME2, flash_splashscreen_image2);
	ret |= aboot_register_flash_cmd(SPLASHSCREEN_NAME3, flash_splashscreen_image3);
	ret |= aboot_register_flash_cmd(SPLASHSCREEN_NAME4, flash_splashscreen_image4);
	ret |= aboot_register_flash_cmd("dnx", flash_dnx);
	ret |= aboot_register_flash_cmd("ifwi", flash_ifwi);
	ret |= aboot_register_flash_cmd("token_umip", flash_token_umip);
	ret |= aboot_register_flash_cmd("capsule", flash_capsule);
	ret |= aboot_register_flash_cmd("ulpmc", flash_ulpmc);
	ret |= aboot_register_flash_cmd("esp_update", flash_esp_update);
	ret |= aboot_register_flash_cmd(SILENT_BINARY_NAME, flash_silent_binary);

	if (strcmp(build_type_prop, "user")) {
		ret |= aboot_register_oem_cmd("dnx_timeout", oem_dnx_timeout);
		ret |= aboot_register_oem_cmd("custom_boot" , oem_custom_boot);
		ret |= aboot_register_oem_cmd("erase_token", oem_erase_token);
	}
	ret |= aboot_register_oem_cmd("erase", oem_erase_partition);
	ret |= aboot_register_oem_cmd("repart", oem_repart_partition);

	ret |= aboot_register_oem_cmd("write_osip_header", oem_write_osip_header);
	ret |= aboot_register_oem_cmd("erase_osip_header", oem_erase_osip_header);
	ret |= aboot_register_oem_cmd("start_partitioning", oem_partition_start_handler);
	ret |= aboot_register_oem_cmd("partition", oem_partition_cmd_handler);
	ret |= aboot_register_oem_cmd("retrieve_partitions", oem_retrieve_partitions);
	ret |= aboot_register_oem_cmd("stop_partitioning", oem_partition_stop_handler);
	ret |= aboot_register_oem_cmd("get_batt_info", oem_get_batt_info_handler);
	ret |= aboot_register_oem_cmd("reboot", oem_reboot);
	ret |= aboot_register_oem_cmd("wipe", oem_wipe_partition);
	ret |= aboot_register_oem_cmd("config", oem_config);
	ret |= aboot_register_oem_cmd("mount", oem_mount);

#ifdef TEE_FRAMEWORK
	print_fun = fastboot_info;
	error_fun = fastboot_fail;

	ret |= aboot_register_oem_cmd("get-spid", get_spid);
	ret |= aboot_register_oem_cmd("get-fru", get_fru);
	ret |= aboot_register_oem_cmd("get-part-id", get_part_id);
	ret |= aboot_register_oem_cmd("get-lifetime", get_lifetime);
	ret |= aboot_register_oem_cmd("start-update", start_update);
	ret |= aboot_register_oem_cmd("cancel-update", cancel_update);
	ret |= aboot_register_oem_cmd("finalize-update", finalize_update);
	ret |= aboot_register_oem_cmd("remove-token", remove_token);
	ret |= aboot_register_flash_cmd("token", aboot_write_token);
#endif

#ifndef EXTERNAL
	ret |= aboot_register_oem_cmd("fru", oem_fru_handler);
	ret |= libintel_droidboot_token_init();

	ret |= aboot_register_flash_cmd(RAMDUMP_OS_NAME, flash_ramdump);

	ret |= aboot_register_oem_cmd("backup_factory", oem_backup_factory);
	ret |= aboot_register_oem_cmd("restore_factory", oem_restore_factory);
	ret |= aboot_register_oem_cmd("fastboot2adb", oem_fastboot2adb);
#endif

	fastboot_register("continue", cmd_intel_reboot);
	fastboot_register("reboot", cmd_intel_reboot);
	fastboot_register("reboot-bootloader", cmd_intel_reboot_bootloader);
	fastboot_register("boot", cmd_intel_boot);

#ifdef USE_GUI
	ret |= aboot_register_ui_cmd(UI_GET_SYSTEM_INFO, get_system_info);
#endif

	if (ret)
		die();

	dump_system_versions();
}
