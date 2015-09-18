/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This is a simplified subset of the ui code in bootable/recovery/
 */

#ifndef _DROIDBOOT_UI_H_
#define _DROIDBOOT_UI_H_
#include <log/log.h>

/* logcat support */
#ifndef LOG_TAG
#define LOG_TAG "droidboot"
#endif

#define VERBOSE_DEBUG 0

#define pr_perror(x)	pr_error("%s failed: %s\n", x, strerror(errno))

/* while LOGE workaround is still in log.h, avoid duplicate definition */
#undef LOGE

#define LOGW ALOGW
#define LOGI ALOGI
#define LOGV ALOGV
#define LOGD ALOGD
#define pr_warning ALOGW
#define pr_info ALOGI
#define pr_debug ALOGD
#if VERBOSE_DEBUG
#define pr_verbose ALOGV
#else
#define pr_verbose(format, ...)				do { } while (0)
#endif

//UI messge types
enum {
	ALERT,
	TIPS
};

//Show or hidden UI block
enum {
	HIDDEN,
	VISIBLE
};

#ifdef USE_GUI
#define LOGE(format, ...) \
    do { \
        ui_print("E:" format, ##__VA_ARGS__); \
	ALOGE(format, ##__VA_ARGS__); \
    } while (0)
#define pr_error(format, ...) \
    do { \
        ui_print("E:" format, ##__VA_ARGS__); \
	ALOGE(format, ##__VA_ARGS__); \
    } while (0)


//default const UI values.
#define MAX_COLS		60
#define MAX_ROWS		34
#define BLANK_SIZE		1
#define INFO_TOP		0
#define INFO_MAX		11
#define MSG_TOP			(MAX_ROWS - MSG_MAX)
#define MSG_MAX			1
#define CHAR_WIDTH		10
#define CHAR_HEIGHT		30
#define SMALL_SCREEN_CHAR_HEIGHT	18

#define UI_GET_SYSTEM_INFO		"get_system_info"

//icons
enum {
  BACKGROUND_ICON_NONE,
  BACKGROUND_ICON_BACKGROUND,
  NUM_BACKGROUND_ICONS
};

//UI block types
enum {
	INFO,
	MSG,
	BLOCK_NUM
};

//system info type for get_system_info ui_cmd.
enum {
	IFWI_VERSION,
	SERIAL_NUM,
	PRODUCT_NAME,
	VARIANT,
	HW_VERSION,
	BOOTLOADER_VERSION,
};

enum targets {
	TARGET_START,
	TARGET_POWER_OFF,
	TARGET_RECOVERY,
	TARGET_BOOTLOADER,
	NUM_TARGETS
};

struct color {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

struct ui_block {
	int type;
	int top;
	int rows;
	int show;
	struct color** clr_table;
	char (*text_table) [MAX_COLS];
};

void ui_set_screen_state(int state);
int ui_get_screen_state(void);
void ui_set_background(int icon);
void ui_block_init(int type, char **titles, struct color **clrs);
void ui_block_show(int type);
void ui_block_hide(int type);
int ui_block_visible(int type);
void ui_start_process_bar();
void ui_stop_process_bar();
void ui_show_process(int show);
void ui_print(const char *fmt, ...);
void ui_msg(int type, const char *fmt, ...);
void ui_start_menu(char** items, int initial_selection);
void ui_menu_select(enum targets sel);
int ui_wait_key();
int ui_key_pressed(int key);
void ui_clear_key_queue();
void ui_event_init(void);
void ui_init();

void droidboot_ui_init();
void droidboot_init_table();
void droidboot_ui_show_process();
void droidboot_run_ui();

#define droidboot_run()					droidboot_run_ui()

#else /* !USE_GUI */

#define LOGE(format, ...) \
	do { \
	__libc_format_log(ANDROID_LOG_ERROR, LOG_TAG, (format), ##__VA_ARGS__ ); \
	} while (0)
#define pr_error(format, ...) \
	do { \
	__libc_format_log(ANDROID_LOG_ERROR, LOG_TAG, (format), ##__VA_ARGS__ ); \
	} while (0)

#define ui_start_process_bar()			do { } while (0)
#define ui_stop_process_bar()			do { } while (0)
#define ui_set_screen_state(x)			do { } while (0)
#define ui_print							pr_info
#define ui_msg(x, format, ...)			pr_info(format, ##__VA_ARGS__);

#define droidboot_ui_init()				do { } while (0)
#define droidboot_init_table()			do { } while (0)
#define droidboot_ui_show_process()		do { } while (0)
#define droidboot_run()					droidboot_run_noui()

#endif /* USE_GUI */
#endif /* _DROIDBOOT_UI_H_ */
