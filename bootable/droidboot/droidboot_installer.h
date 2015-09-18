/*
 * Copyright 2013 Intel Corporation
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


#ifndef _DROIDBOOT_INSTALLER_H
#define _DROIDBOOT_INSTALLER_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/reboot.h>
#include <unistd.h>
#include <sys/mount.h>
#include <cutils/android_reboot.h>
#include <cutils/properties.h>
#include <cutils/klog.h>
#include <cutils/hashmap.h>
#include <sys/mman.h>

#include "droidboot.h"
#include "droidboot_ui.h"
#include "fastboot.h"

/* installer will try to mount each devices given on cmdline based on priority
 * (remote, usb, sdcard, internal). If one is being mounted, it will parse the
 * installer_file and execute commands in sequence*/

/* Flag to enable/disable usage of installer from kernel command line */
int g_use_installer;

/* usb device base name for installer, i.e /dev/block/sde2 */
char g_installer_usb_dev[BUFSIZ];

/* sdcard device base name for installer, i.e /dev/block/mmcblk1p2 */
char g_installer_sdcard_dev[BUFSIZ];

/* internal device base name for installer, i.e /dev/block/mmcblk0p2 */
char g_installer_internal_dev[BUFSIZ];

/* remote device base name for installer, i.e nfs, tftp */
char g_installer_remote_dev[BUFSIZ];

/* Default filename for installer file */
#define DEFAULT_INSTALLER_FILENAME "installer.cmd"

/* filename for installer, this file contains all the commands
 * to be executed
 */
char g_installer_file[BUFSIZ];


int install_from_device(const char *device, const char *fs_type,
		char *installer_file,
		int (*device_init) (const char *, int));

int installer_handle_cmd(struct fastboot_cmd *cmd, char *buffer);

void *installer_thread(void *arg);

#endif
