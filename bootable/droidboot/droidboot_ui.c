/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#define LOG_TAG "droidboot_ui"

#include <pthread.h>
#include <sys/reboot.h>
#include <minui/minui.h>
#include <cutils/android_reboot.h>
#include <cutils/klog.h>
#include <cutils/hashmap.h>
#include <charger/charger.h>

#include "aboot.h"
#include "droidboot_util.h"
#include "droidboot_plugin.h"
#include "droidboot_installer.h"
#include "droidboot.h"
#include "fastboot.h"
#include "droidboot_ui.h"

#define NO_ACTION           -1
#define HIGHLIGHT_UP        -2
#define HIGHLIGHT_DOWN      -3
#define SELECT_ITEM         -4

enum {
	ITEM_BOOTLOADER,
	ITEM_REBOOT,
	ITEM_RECOVERY,
	ITEM_POWEROFF
};

extern struct color white;
extern struct color green;
extern struct color brown;
extern struct color red;

static char **title, **info;
static struct color* info_clr[INFO_MAX];

static char* menu[] = {
	"REBOOT DROIDBOOT",
	"REBOOT",
	"RECOVERY",
	"POWER OFF", NULL};

static int table_init(char*** table, int width, int height)
{
	int i;

	if ((*table = malloc(height * sizeof(char*))) == NULL)
		return -1;
	for (i = 0; i < height; i++) {
		if (((*table)[i] = malloc(width * sizeof(char))) == NULL)
			return -1;
		memset((*table)[i], 0, width);
	}
	return 0;
}

static void table_exit(char ** table, int height)
{
	int i;

	if (table) {
		for (i = 0; i < height; i++)
			if (table[i])   free(table[i]);
	}
}

#define SYSFS_FORCE_SHUTDOWN	"/sys/module/intel_mid_osip/parameters/force_shutdown_occured"
void force_shutdown()
{
	int fd;
	char c = '1';

	pr_info("[SHTDWN] %s, force shutdown", __func__);
	if ((fd = open(SYSFS_FORCE_SHUTDOWN, O_WRONLY)) < 0) {
		pr_error("[SHUTDOWN] Open %s error!\n", SYSFS_FORCE_SHUTDOWN);
	} else {
                if (write(fd, &c, 1) < 0)
                        pr_error("[SHUTDOWN] Write %s error!\n", SYSFS_FORCE_SHUTDOWN);
		close(fd);
	}

	sync();
	reboot(LINUX_REBOOT_CMD_POWER_OFF);
}

static void goto_recovery()
{
	if (ui_block_visible(INFO) == VISIBLE)
		table_exit(info, INFO_MAX);
	sleep(1);
	android_reboot(ANDROID_RB_RESTART2, 0, "recovery");
	ui_msg(ALERT, "SWITCH TO RECOVERY FAILED!");
}

#define BUF_IFWI_SZ		80
#define BUF_PRODUCT_SZ		80
#define BUF_VARIANT_SZ		10
#define BUF_HW_VERSION_SZ	10
#define BUF_BL_VERSION_SZ	20
#define BUF_SERIALNUM_SZ	20
extern Hashmap *ui_cmds;

#define fill_info(clr, ...) \
	{			 \
	info_clr[i] = clr;	 \
	snprintf(info[i++], MAX_COLS, __VA_ARGS__); \
	}

static int get_info()
{
	int i = 0;
	char ifwi[BUF_IFWI_SZ], product[BUF_PRODUCT_SZ], serialnum[BUF_SERIALNUM_SZ];
	char variant[BUF_VARIANT_SZ], hw_version[BUF_HW_VERSION_SZ];
	char bl_version[BUF_BL_VERSION_SZ];
	ui_func cb;

	cb = hashmapGet(ui_cmds, UI_GET_SYSTEM_INFO);
	if (cb == NULL) {
		pr_error("Get ui_cmd: %s error!\n", UI_GET_SYSTEM_INFO);
		return -1;
	}
	cb(IFWI_VERSION, ifwi, BUF_IFWI_SZ);
	cb(PRODUCT_NAME, product, BUF_PRODUCT_SZ);
	cb(SERIAL_NUM, serialnum, BUF_SERIALNUM_SZ);
	cb(VARIANT, variant, BUF_VARIANT_SZ);
	cb(HW_VERSION, hw_version, BUF_HW_VERSION_SZ);
	cb(BOOTLOADER_VERSION, bl_version, BUF_BL_VERSION_SZ);

	fill_info(&red, "FASTBOOT MODE");
	fill_info(&white, "PRODUCT_NAME - %s", product);
	fill_info(&white, "VARIANT - %s", variant);
	fill_info(&white, "HW_VERSION - %s", hw_version);
	fill_info(&white, "BOOTLOADER VERSION - %s", bl_version);
	fill_info(&white, "IFWI VERSION - %s", ifwi);
	fill_info(&white, "SERIAL NUMBER - %s", serialnum);
	fill_info(&white, "SIGNING - ?");
	fill_info(&white, "SECURE_BOOT - ?");

	return 0;
}

static int device_handle_key(int key_code, int visible)
{
	if (visible) {
		switch (key_code) {
			case KEY_VOLUMEDOWN:
				return HIGHLIGHT_DOWN;

			case KEY_VOLUMEUP:
				return HIGHLIGHT_UP;

			case KEY_POWER:
			case KEY_CAMERA:
				return SELECT_ITEM;
		}
	}
	return NO_ACTION;
}

static enum targets get_menu_selection(enum targets initial_selection) {
	ui_clear_key_queue();
	enum targets selected = initial_selection;
	enum targets chosen_item = NUM_TARGETS;

	while (chosen_item == NUM_TARGETS) {
		int key = ui_wait_key();
		int action = device_handle_key(key, 1);

		if (ui_get_screen_state() == 0)
			ui_set_screen_state(1);
		else
			switch (action) {
				case HIGHLIGHT_UP:
					selected--;
					if (selected < 0)
						selected = NUM_TARGETS - 1;
					ui_menu_select(selected);
					break;
				case HIGHLIGHT_DOWN:
					selected = (selected + 1) % NUM_TARGETS;
					ui_menu_select(selected);
					break;
				case SELECT_ITEM:
					chosen_item = selected;
					break;
				case NO_ACTION:
					break;
			}
	}
	return chosen_item;
}

static void prompt_and_wait()
{
	for (;;) {
		enum targets chosen_item = get_menu_selection(TARGET_START);
		switch (chosen_item) {
			case TARGET_BOOTLOADER:
				sync();
				android_reboot(ANDROID_RB_RESTART2, 0, "bootloader");
				break;
			case TARGET_RECOVERY:
				sync();
				goto_recovery();
				break;
			case TARGET_START:
				sync();
				android_reboot(ANDROID_RB_RESTART2, 0, "android");
				break;
			case TARGET_POWER_OFF:
				force_shutdown();
				break;
			case NUM_TARGETS:
				pr_error("This choice should not be possible!\n");
				break;
		}
	}
}

void droidboot_ui_init()
{
	ui_init();
}

void droidboot_init_table()
{
	ui_event_init();
	ui_set_background(BACKGROUND_ICON_BACKGROUND);

	//Init info table
	if (table_init(&info, MAX_COLS, INFO_MAX) == 0) {
		if(get_info() < 0)
			pr_error("get_info error!\n");
		ui_block_init(INFO, (char**)info, info_clr);
	} else {
		pr_error("Init info table error!\n");
	}

	ui_block_show(MSG);
}

static void *fastboot_thread(void *arg)
{
	extern int g_scratch_size;

	pr_info("Listening for the fastboot protocol over USB.");
	ui_print("FASTBOOT INIT...\n");
	fastboot_init(g_scratch_size * MEGABYTE);
	return NULL;
}

void droidboot_run_ui()
{
	ui_block_show(INFO);
	ui_block_show(MSG);
	pthread_t th_fastboot;

	pthread_create(&th_fastboot, NULL, fastboot_thread, NULL);
#ifdef USE_INSTALLER
	pthread_t th_installer;
	if (g_use_installer)
		pthread_create(&th_installer, NULL, installer_thread, NULL);
#endif

	//wait for user's choice
	prompt_and_wait();
}
