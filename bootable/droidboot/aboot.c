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

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cutils/android_reboot.h>
#include <cutils/hashmap.h>

/* from ext4_utils for sparse ext4 images */
#include <sparse_format.h>
#include <sparse/sparse.h>

#include "volumeutils/roots.h"
#include "fastboot.h"
#include "droidboot.h"
#include "droidboot_util.h"
#include "droidboot_plugin.h"
#include "droidboot_ui.h"

#define CMD_SYSTEM		"system"
#define CMD_SHOWTEXT		"showtext"
#define SYSTEM_BUF_SIZ     512    /* For system() and popen() calls. */
#define FILE_NAME_SIZ    50     /* /dev/<whatever> */

#define FILEMODE  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

Hashmap *flash_cmds;
Hashmap *oem_cmds;

#ifdef USE_GUI
Hashmap *ui_cmds;
#endif
static int isCatCommand;

static bool strcompare(void *keyA, void *keyB)
{
	return !strcmp(keyA, keyB);
}

//mei add
static int cmd_flash_txefileset(void *data, unsigned sz) {
	Volume *v;

        if ((v = volume_for_path("/logs")) == NULL) {
                pr_error("Cannot find logs volume!\n");
                return -1;
        }
	char * buf = (char*)data;
	FILE* pFile;  
    	pFile = fopen("/logs/tmp" , "w+");
	if(!pFile) {
		
		ui_print("Failed to open /logs/tmp\n");
                return -1;

	}  
    	fwrite(buf , 1 , sz, pFile);  
    	fclose(pFile);
	return 0;  
  

}

static int cmd_oem_txe(int argc, char **argv) {
	FILE   *stream;
	char   buf[102400];
	if(argc < 2 || !argv[1]) {
		ui_print("Command parameters are required\n");
		return -1;
	}
	memset(buf, '\0', sizeof(buf));
	char tmpStr[200];
	memset(tmpStr, '\0', sizeof(tmpStr));
	int i;
	strcpy(tmpStr, argv[1]);
	isCatCommand = 0;
	if(!memcmp(tmpStr, "cat", 3)) {
		isCatCommand = 1;
	}
	for(i = 3; i <= argc; i++) {
		strcat(tmpStr, " ");
		char tmpStr2[100];
	       	memset(tmpStr2, '\0', sizeof(tmpStr2));
  		strcpy(tmpStr2, argv[i-1]);
		strcat(tmpStr, tmpStr2);
	}
	stream = popen(tmpStr , "r");
	if(!stream) return -1;
	//fread(buf, sizeof(char), sizeof(buf), stream);
	while(fgets(buf, 200, stream)!=NULL) {
		ui_print("%s\n", buf);	
	}
	int rc = pclose(stream);  
	ui_print("shell return result:\n%s",buf);
	ui_print("argc:%d  tmpStr:%s\n", argc, tmpStr);
	int status_child = WEXITSTATUS(rc);
	if(status_child == 0) {
        	if(strstr(buf, "Passed") || strstr(buf, "Successful")) {
			ui_print("Passed Successful\n");
			if(isCatCommand) {
				fastboot_info(buf);
			}
                	return 0;
        	}
                if(isCatCommand) {
                     fastboot_info(buf);
                }
		return 0;
	} else {
		if(strstr(buf, "Failed") || strstr(buf, "Error") || strstr(buf, "cannot") || strstr(buf, "No such")) {
        	        ui_print("Failed Error\n");
			isCatCommand = 0;
			return -1;
	        }
 		isCatCommand = 0;
	        return -1;
	}
/*	if(strstr(buf, "Passed") || strstr(buf, "Successful")) {
		return 0;
	} else if(strstr(buf, "Failed") || strstr(buf, "Error") || strstr(buf, "cannot") || strstr(buf, "No such")) {
		return -1;
	}*/
	//return 0;


}

static int strhash(void *key)
{
	return hashmapHash(key, strlen((char *)key));
}

static int aboot_register_cmd(Hashmap *map, char *key, void *callback)
{
	char *k;

	k = strdup(key);
	if (!k) {
		pr_perror("strdup");
		return -1;
	}
	if (hashmapGet(map, k)) {
		pr_error("key collision '%s'\n", k);
		free(k);
		return -1;
	}
	hashmapPut(map, k, callback);
	pr_verbose("Registered plugin function %p (%s) with table %p\n",
			callback, k, map);
	return 0;
}

int aboot_register_flash_cmd(char *key, flash_func callback)
{
	return aboot_register_cmd(flash_cmds, key, callback);
}

int aboot_register_oem_cmd(char *key, oem_func callback)
{
	return aboot_register_cmd(oem_cmds, key, callback);
}

#ifdef USE_GUI
int aboot_register_ui_cmd(char *key, ui_func callback)
{
	return aboot_register_cmd(ui_cmds, key, callback);
}
#endif

static Volume *get_part_volume(const char *part_name)
{
	char mnt_point[FILE_NAME_SIZ];

	if (!part_name)
		return NULL;

	/* supports fastboot -w who wants to erase userdata */
	if (!strcmp(part_name, "userdata"))
		snprintf(mnt_point, FILE_NAME_SIZ, "/%s", "data");
	else
		snprintf(mnt_point, FILE_NAME_SIZ, "/%s", part_name);

	return volume_for_path(mnt_point);
}

static int erase_partition(const char *part_name)
{
	int fd = 0;
	unsigned long long size;
	Volume *v;
	int ret = -1;

	v = get_part_volume(part_name);
	if (!v) {
		pr_error("Unable to get device for %s\n", part_name);
		goto out;
	}


	fd = open(v->device, O_WRONLY | O_TRUNC, 0644);
	if (fd == -1) {
		pr_error("%s open failed\n", v->device);
		goto out;
	}

	if (ioctl(fd, BLKGETSIZE64, &size) == -1) {
		pr_error("Unable to get %s block device\n", v->device);
		goto close;
	}

	unsigned long long range[] = { 0, size };
	if (ioctl(fd, BLKDISCARD, range) == -1) {
		pr_error("Unable to erase %s\n", part_name);
		goto close;
	}
	ret = 0;
close:
	close(fd);
out:
	return ret;
}

void cmd_erase(const char *part_name, void *data, unsigned sz)
{
	char mnt_point[FILE_NAME_SIZ];
	int ret;
	snprintf(mnt_point,FILE_NAME_SIZ, "/%s",part_name);

	/* supports fastboot -w who wants to erase userdata */
	ui_print("ERASE %s...\n", part_name);
	ret = erase_partition(part_name);
	ui_print("ERASE %s\n", ret==0 ? "COMPLETE." : "FAILED!");

	if (ret==0)
		fastboot_okay("");
	else
		fastboot_fail("unable to format");
}

static int cmd_flash_update(void *data, unsigned sz)
{
	int fd;
	char command[512];

	ui_print("OTA_UPDATE...\n");
	sprintf(command, "--update_package=" OTA_UPDATE_FILE);
	if (ensure_path_mounted("/cache") != 0) {
		pr_error("Unable to mount /cache!\n");
		goto err;
	}
	if (ensure_path_mounted(OTA_UPDATE_FILE) != 0) {
		pr_error("Unable to mount update file storage filesystem!\n");
		goto err;
	}

	unlink(OTA_UPDATE_FILE);
	if (named_file_write(OTA_UPDATE_FILE, data, sz) < 0) {
		pr_error("Unable to write file: " OTA_UPDATE_FILE "\n");
		goto err;
	}
	if (access("/cache/recovery", F_OK) != 0)
		if (mkdir("/cache/recovery", 777) != 0) {
			pr_error("Unable to create /cache/recovery directory!\n");
			goto err;
	}
	if ((fd = open("/cache/recovery/command", O_CREAT | O_WRONLY, FILEMODE)) < 0) {
		pr_error("Unable to open /cache/recovery/command file!\n");
		goto err;
	}
	if (write(fd, command, strlen(command)) < 0) {
		pr_error("Unable to write /cache/recovery/command file!\n");
		goto err;
	}
	sync();

	ui_print("Rebooting to recovery to apply update.\n");
	pr_info("Rebooting into recovery console to apply update\n");
	fastboot_okay("");
	android_reboot(ANDROID_RB_RESTART2, 0, "recovery");

	return 0;

err:
	ui_msg(ALERT, "OTA_UPDATE FAILED!\n");
	fastboot_fail("problem with creating ota update file!");
	return -1;
}


static const char *var_partition_size(const char *part_name)
{
	static char var_value[19];
	unsigned long long size;
	int fd = 0;

	Volume *v = get_part_volume(part_name);
	if (!v) {
		pr_error("Unable to get volume for %s\n", part_name);
		goto fail;
	}

	fd = open(v->device, O_WRONLY | O_TRUNC, 0644);
	if (fd == -1) {
		pr_error("%s open failed\n", v->device);
		goto fail;
	}

	if (ioctl(fd, BLKGETSIZE64, &size) == -1) {
		pr_error("Unable to get %s block device\n", v->device);
		goto fail;
	}

	if (sprintf(var_value, "0x%016llx", size) < 0)
		goto fail;

	close(fd);
	return var_value;

fail:
	if (fd)
		close(fd);
	return NULL;
}

static const char *var_partition_type(const char *part_name)
{
	Volume *v = get_part_volume(part_name);
	if (!v)
		pr_error("Unable to get volume for %s\n", part_name);

	return v ? v->fs_type : NULL;
}

static int cmd_flash_system(void *data, unsigned sz)
{
	Volume *v;
	int ret;
	FILE * src = NULL;

	if ((v = volume_for_path("/system")) == NULL) {
		pr_error("Cannot find system volume!\n");
		return -1;
	}
	/* Check if data is a buffer or a file */
	if (strncmp((const char*)data, FASTBOOT_DOWNLOAD_TMP_FILE, sz) == 0) {
		char startoffile[sizeof(sparse_header_t)];

		src = fopen(FASTBOOT_DOWNLOAD_TMP_FILE, "r");
		if (src == NULL) {
			pr_error("could not open download tmp file\n");
			return -1;
		}
		ret = fread(startoffile, 1, sizeof(sparse_header_t), src);
		fclose(src);
		if (ret != sizeof(sparse_header_t)){
			pr_error("could not read enough bytes from download tmp file\n");
			return -1;
		}

		if (((sparse_header_t*)startoffile)->magic == SPARSE_HEADER_MAGIC) {
			/* If there is enough data to hold the header,
			* and MAGIC appears in header,
			* then it is a sparse ext4 image */
			pr_info("sparse image file detected flash sparse package\n");
			ret = named_file_write_ext4_sparse(v->device, data, sz);
		} else {
			pr_info("try to flash gzip\n");
			ret = named_file_write_decompress_gzip(v->device, data, sz);
		}
		if (unlink(FASTBOOT_DOWNLOAD_TMP_FILE) != 0) {
			pr_error("Cannot remove %s\n", FASTBOOT_DOWNLOAD_TMP_FILE);
		}

	} else {
		if (((sparse_header_t*)data)->magic == SPARSE_HEADER_MAGIC) {
			pr_info("sparse image data detected flash sparse package\n");
			ret = write_ext4_sparse(v->device, data, sz);
		}
		else {
			pr_error("data not sparsed unable to flash \n");
			return -1;
		}
	}

	return ret;
}

static void cmd_flash(const char *part_name, void *data, unsigned sz)
{
	char path[FILE_NAME_SIZ];
	int ret = -1;
	flash_func cb;
	Volume *v = NULL;

	ui_print("FLASH %s...\n", part_name);

	if ( (cb = hashmapGet(flash_cmds, (char *)part_name)) ) {
		/* Use our table of flash functions registered by platform
		 * specific plugin libraries */
		if ((ret = cb(data, sz)) != 0)
			pr_error("%s flash failed!\n", part_name);
	} else {
		if (part_name[0] == '/') {
			snprintf(path, FILE_NAME_SIZ, "%s", part_name);
		} else {
			snprintf(path, FILE_NAME_SIZ, "/%s", part_name);
			if ((v = volume_for_path(path)) == NULL) {
				pr_error("unknown volume %s to flash!\n", path);
				goto out;
			}
			snprintf(path, FILE_NAME_SIZ, "%s", v->device);
		}
		if (v && ((sparse_header_t*)data)->magic == SPARSE_HEADER_MAGIC) {
			pr_info("sparse image data detected flash sparse package\n");
			ret = write_ext4_sparse(v->device, data, sz);
		} else if ((ret = named_file_write(path, data, sz)) < 0) {
			pr_error("Can't write data to target %s!\n", path);
			goto out;
		}
		sync();
	}

out:
	if (ret == 0) {
		ui_print("FLASH COMPLETE!\n");
		fastboot_okay("");
	} else {
		ui_print("FLASH FAILED!\n");
		fastboot_fail("flash_cmds error!\n");
	}
	return;
}

static void cmd_oem(const char *arg, void *data, unsigned sz)
{
	char *command, *saveptr, *str1;
	char *argv[MAX_OEM_ARGS];
	int argc = 0;
	oem_func cb;

	pr_verbose("%s: <%s>\n", __FUNCTION__, arg);

	while (*arg == ' ')
		arg++;
	command = strdup(arg); /* Can't use strtok() on const strings */
	if (!command) {
		pr_perror("strdup");
		fastboot_fail("memory allocation error");
		return;
	}

	for (str1 = command; argc < MAX_OEM_ARGS; str1 = NULL) {
		argv[argc] = strtok_r(str1, " \t", &saveptr);
		if (!argv[argc])
			break;
		argc++;
	}
	if (argc == 0) {
		fastboot_fail("empty OEM command");
		goto out;
	}

	if ( (cb = hashmapGet(oem_cmds, argv[0])) ) {
		int ret;

		ui_print("CMD '%s'...\n", argv[0]);
		ret = cb(argc, argv);
		if (ret) {
			pr_error("oem %s command failed, retval = %d\n",
					argv[0], ret);
			fastboot_fail(argv[0]);
		} else {
			ui_print("CMD '%s' COMPLETE.\n", argv[0]);
			fastboot_okay("");
		}
	} else if (strcmp(argv[0], CMD_SYSTEM) == 0) {
                fastboot_fail("OEM system command are not supported anymore");
	} else if (strcmp(argv[0], CMD_SHOWTEXT) == 0) {
		fastboot_okay("");
	} else {
		fastboot_fail("unknown OEM command");
	}
out:
	if (command)
		free(command);
	return;
}

static void cmd_boot(const char *arg, void *data, unsigned sz)
{
	fastboot_fail("boot command stubbed on this platform!");
}

static void cmd_reboot(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	sync();
	ui_print("REBOOT...\n");
	pr_info("Rebooting!\n");
	android_reboot(ANDROID_RB_RESTART2, 0, "android");
	pr_error("Reboot failed");
}

static void cmd_reboot_bl(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	sync();
	ui_print("REBOOT...\n");
	pr_info("Restarting Droidboot...\n");
	android_reboot(ANDROID_RB_RESTART2, 0, "fastboot");
	pr_error("Reboot failed");
}

void aboot_register_commands(void)
{
	fastboot_register("oem", cmd_oem);
	fastboot_register("boot", cmd_boot);
	fastboot_register("reboot", cmd_reboot);
	fastboot_register("reboot-bootloader", cmd_reboot_bl);
	fastboot_register("erase:", cmd_erase);
	fastboot_register("flash:", cmd_flash);
	fastboot_register("continue", cmd_reboot);

	fastboot_publish("product", DEVICE_NAME, NULL);
	fastboot_publish("kernel", "droidboot", NULL);

	fastboot_publish("partition-size", NULL, var_partition_size);
	fastboot_publish("partition-type", NULL, var_partition_type);

	flash_cmds = hashmapCreate(30, strhash, strcompare);
	oem_cmds = hashmapCreate(30, strhash, strcompare);
	if (!flash_cmds || !oem_cmds) {
		pr_error("Memory allocation error\n");
		die();
	}
#ifdef USE_GUI
	ui_cmds = hashmapCreate(8, strhash, strcompare);
	if (!ui_cmds) {
		pr_error("Memory allocation error\n");
		die();
	}
#endif

	aboot_register_flash_cmd("update", cmd_flash_update);
	aboot_register_flash_cmd("system", cmd_flash_system);
	//mei add
	aboot_register_flash_cmd("txefileset", cmd_flash_txefileset);
	aboot_register_oem_cmd("txe", cmd_oem_txe);
	//mei add end

}
