/*
 * Copyright 2011-2013 Intel Corporation
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <libgen.h>
#include <edify/expr.h>
#include <updater/updater.h>
#include <common.h>
#include <cutils/properties.h>
#include <sys/mman.h>

#include "update_osip.h"
#include "util.h"
#include "fw_version_check.h"
#ifdef TEE_FRAMEWORK
#include "tee_connector.h"
#endif
#include "oem_partition.h"

#include "flash.h"

Value *ExtractImageFn(const char *name, State * state, int argc, Expr * argv[])
{
	Value *ret = NULL;
	char *filename = NULL;
	char *source = NULL;
	void *data = NULL;
	int size;

	if (ReadArgs(state, argv, 2, &filename, &source) < 0) {
		return NULL;
	}

	if (filename == NULL || strlen(filename) == 0) {
		ErrorAbort(state, "filename argument to %s can't be empty", name);
		goto done;
	}

	if (source == NULL || strlen(source) == 0) {
		ErrorAbort(state, "source argument to %s can't be empty", name);
		goto done;
	}

	if ((size = read_image(source, &data)) < 0) {
		ErrorAbort(state, "Couldn't read image %s", source);
		goto done;
	}

	if (file_write(filename, data, size) < 0) {
		ErrorAbort(state, "Couldn't write %s data to %s", source, filename);
		goto done;
	}

	ret = StringValue(strdup("t"));
done:
	if (source)
		free(source);
	if (filename)
		free(filename);
	if (data)
		free(data);

	return ret;
}

Value *ExecuteOsipFunction(const char *name, State * state, int argc, Expr * argv[], int (*action) (char *))
{
	Value *ret = NULL;
	char *destination = NULL;

	if (ReadArgs(state, argv, 1, &destination) < 0) {
		return NULL;
	}

	if (destination == NULL || strlen(destination) == 0) {
		ErrorAbort(state, "destination argument to %s can't be empty", name);
		goto done;
	}

	if (action(destination) == -1) {
		ErrorAbort(state, "Error writing %s to OSIP", destination);
		goto done;
	}

	ret = StringValue(strdup("t"));

done:
	if (destination)
		free(destination);

	return ret;
}

Value *InvalidateOsFn(const char *name, State * state, int argc, Expr * argv[])
{
	return ExecuteOsipFunction(name, state, argc, argv, invalidate_osii);
}

Value *RestoreOsFn(const char *name, State * state, int argc, Expr * argv[])
{
	return ExecuteOsipFunction(name, state, argc, argv, restore_osii);
}

#define IFWI_BIN_PATH "/tmp/ifwi.bin"
#define IFWI_NAME     "ifwi"
#define FILEMODE      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

enum flash_option_type {
	FLASH_IFWI_BINARY,
	FLASH_BOM_TOKEN_BINARY,
};

#ifdef TEE_FRAMEWORK
#define BOM_TOKEN_NAME "bom-token"
#endif

Value *FlashIfwiOrBomFn(enum flash_option_type flash_option, const char *name, State * state, int argc,
			Expr * argv[])
{
	Value *ret = NULL;
	char *filename = NULL;
	int err, i, num, buffsize;
	char ifwi_name[128];
	ZipArchive ifwi_za;
	const ZipEntry *ifwi_entry;
	unsigned char *buffer;
	unsigned char *file_buf = NULL;
	size_t file_size;
#ifdef TEE_FRAMEWORK
	char bom_token_name[128];
	const ZipEntry *bom_token_entry;
	int bom_token_buffsize;
	unsigned char *bom_token_buffer;
#endif

	if (ReadArgs(state, argv, 1, &filename) < 0) {
		return NULL;
	}

	if (filename == NULL || strlen(filename) == 0) {
		ErrorAbort(state, "filename argument to %s can't be empty", name);
		goto done;
	}

	err = file_read(filename, (void **)&file_buf, &file_size);
	if (err) {
		ErrorAbort(state, "Failed to open zip archive file %s\n", filename);
		goto done;
	}

	err = mzOpenZipArchive(file_buf, file_size, &ifwi_za);
	if (err) {
		ErrorAbort(state, "Failed to open zip archive\n");
		goto done;
	}

	num = mzZipEntryCount(&ifwi_za);
	for (i = 0; i < num; i++) {
		ifwi_entry = mzGetZipEntryAt(&ifwi_za, i);
		if ((ifwi_entry->fileNameLen + 1) < sizeof(ifwi_name)) {
			strncpy(ifwi_name, ifwi_entry->fileName, ifwi_entry->fileNameLen);
			ifwi_name[ifwi_entry->fileNameLen] = '\0';
		} else {
			ErrorAbort(state, "ifwi file name is too big. Size max is:%zd.\n", sizeof(ifwi_name));
			goto error;
		}
		if (strncmp(ifwi_name, IFWI_NAME, strlen(IFWI_NAME)))
			continue;
		buffsize = mzGetZipEntryUncompLen(ifwi_entry);
		if (buffsize <= 0) {
			ErrorAbort(state, "Bad ifwi_entry size : %d.\n", buffsize);
			goto error;
		}
		buffer = (unsigned char *)malloc(sizeof(unsigned char) * buffsize);
		if (buffer == NULL) {
			ErrorAbort(state, "Unable to alloc ifwi buffer of %d bytes.\n", buffsize);
			goto error;
		}
		err = mzExtractZipEntryToBuffer(&ifwi_za, ifwi_entry, buffer);
		if (!err) {
			ErrorAbort(state, "Failed to unzip %s\n", IFWI_BIN_PATH);
			free(buffer);
			goto error;
		}

		if (check_ifwi_file(buffer, buffsize) < 1) {
			free(buffer);
			continue;
		}

		if (flash_option == FLASH_BOM_TOKEN_BINARY) {
#ifdef TEE_FRAMEWORK
			strcpy(bom_token_name, BOM_TOKEN_NAME);
			strncat(bom_token_name, &(ifwi_name[strlen(IFWI_NAME)]),
				sizeof(bom_token_name) - strlen(BOM_TOKEN_NAME) - 1);
			bom_token_entry = mzFindZipEntry(&ifwi_za, bom_token_name);

			if (bom_token_entry != NULL) {
				bom_token_buffsize = mzGetZipEntryUncompLen(bom_token_entry);
				if (bom_token_buffsize <= 0) {
					ErrorAbort(state, "Bad bom_token_entry size : %d.\n",
						   bom_token_buffsize);
					free(buffer);
					goto error;
				}
				bom_token_buffer =
				    (unsigned char *)malloc(sizeof(unsigned char) * bom_token_buffsize);
				if (bom_token_buffer == NULL) {
					ErrorAbort(state, "Unable to alloc bom token buffer of %d bytes.\n",
						   bom_token_buffsize);
					free(buffer);
					goto error;
				}
				err = mzExtractZipEntryToBuffer(&ifwi_za, bom_token_entry, bom_token_buffer);
				if (!err) {
					ErrorAbort(state, "Failed to unzip %s.\n", IFWI_BIN_PATH);
					free(bom_token_buffer);
					free(buffer);
					goto error;
				}
				if (write_token(bom_token_buffer, bom_token_buffsize) == 0) {
					printf("BOM token written\n");
				} else {
					printf("Unable to write BOM token.\n");
					cancel_update(0, NULL);
					free(bom_token_buffer);
					free(buffer);
					ret = StringValue(strdup("fail"));
					goto error;
				}
				free(bom_token_buffer);
			}
#else
			printf("BOM token flashing not supported\n");
#endif
		} else if (flash_option == FLASH_IFWI_BINARY) {
			printf("Flashing IFWI\n");
			update_ifwi_file(buffer, buffsize);
		} else {
			ErrorAbort(state, "Don't know what to do with option %d\n", flash_option);
			free(buffer);
			goto error;
		}
		free(buffer);
	}

	ret = StringValue(strdup("t"));

error:
	mzCloseZipArchive(&ifwi_za);

done:
	free(file_buf);
	if (filename)
		free(filename);

	return ret;
}

#ifndef CLVT

Value *FlashIfwiFn(const char *name, State * state, int argc, Expr * argv[])
{
	return FlashIfwiOrBomFn(FLASH_IFWI_BINARY, name, state, argc, argv);
}

#endif

#ifdef TEE_FRAMEWORK
Value *FlashBomFn(const char *name, State * state, int argc, Expr * argv[])
{
	return FlashIfwiOrBomFn(FLASH_BOM_TOKEN_BINARY, name, state, argc, argv);
}
#endif

/* This should never be enabled anymore.
 *
 * Clovertrail program has been removed from mainline and we are
 * waiting for some months to remove this code
 */
#ifdef CLVT

#define DNX_BIN_PATH "/tmp/dnx.bin"
#define DNX_NAME     "dnx"

Value *FlashIfwiFn(const char *name, State * state, int argc, Expr * argv[])
{
	Value *ret = NULL;
	char *filename = NULL;
	int err, ifwi_bin_fd, dnx_bin_fd, i, num;
	char ifwi_name[128], dnx_name[128];
	ZipArchive ifwi_za;
	const ZipEntry *dnx_entry, *ifwi_entry;
	unsigned char *file_buf = NULL;
	size_t file_size;

	if (ReadArgs(state, argv, 1, &filename) < 0) {
		return NULL;
	}

	if (filename == NULL || strlen(filename) == 0) {
		ErrorAbort(state, "filename argument to %s can't be empty", name);
		goto done;
	}

	err = file_read(filename, (void **)&file_buf, &file_size);
	if (err) {
		ErrorAbort(state, "Failed to open zip archive file %s\n", filename);
		goto done;
	}

	err = mzOpenZipArchive(file_buf, file_size, &ifwi_za);
	if (err) {
		ErrorAbort(state, "Failed to open zip archive\n");
		goto done;
	}

	num = mzZipEntryCount(&ifwi_za);
	for (i = 0; i < num; i++) {
		ifwi_entry = mzGetZipEntryAt(&ifwi_za, i);
		if ((ifwi_entry->fileNameLen + 1) < sizeof(ifwi_name)) {
			strncpy(ifwi_name, ifwi_entry->fileName, ifwi_entry->fileNameLen);
			ifwi_name[ifwi_entry->fileNameLen] = '\0';
		} else {
			ErrorAbort(state, "ifwi file name is too big. Size max is:%d.\n", sizeof(ifwi_name));
			goto error;
		}
		if (strncmp(ifwi_name, IFWI_NAME, strlen(IFWI_NAME)))
			continue;

		if ((ifwi_bin_fd = open(IFWI_BIN_PATH, O_RDWR | O_TRUNC | O_CREAT, FILEMODE)) < 0) {
			ErrorAbort(state, "unable to create Extracted file:%s.\n", IFWI_BIN_PATH);
			goto error;
		}
		if ((dnx_bin_fd = open(DNX_BIN_PATH, O_RDWR | O_TRUNC | O_CREAT, FILEMODE)) < 0) {
			ErrorAbort(state, "unable to create Extracted file:%s.\n", IFWI_BIN_PATH);
			close(ifwi_bin_fd);
			goto error;
		}
		strcpy(dnx_name, "dnx_fwr");
		strncat(dnx_name, &(ifwi_name[strlen(IFWI_NAME)]), sizeof(dnx_name) - strlen("dnx_fwr") - 1);
		dnx_entry = mzFindZipEntry(&ifwi_za, dnx_name);

		if (dnx_entry == NULL) {
			ErrorAbort(state, "Could not find DNX entry");
			close(ifwi_bin_fd);
			close(dnx_bin_fd);
			goto error;
		}

		err = mzExtractZipEntryToFile(&ifwi_za, dnx_entry, dnx_bin_fd);
		if (!err) {
			ErrorAbort(state, "Failed to unzip %s\n", DNX_BIN_PATH);
			close(ifwi_bin_fd);
			close(dnx_bin_fd);
			goto error;
		}
		close(dnx_bin_fd);
		err = mzExtractZipEntryToFile(&ifwi_za, ifwi_entry, ifwi_bin_fd);
		if (!err) {
			ErrorAbort(state, "Failed to unzip %s\n", DNX_BIN_PATH);
			close(ifwi_bin_fd);
			goto error;
		}
		close(ifwi_bin_fd);
		int update_ifwi_file_scu_ipc(const char *dnx, const char *ifwi);
		update_ifwi_file_scu_ipc(DNX_BIN_PATH, IFWI_BIN_PATH);
	}

	ret = StringValue(strdup("t"));

error:
	mzCloseZipArchive(&ifwi_za);

done:
	free(file_buf);
	if (filename)
		free(filename);

	return ret;
}
#endif	/* CLVT */

Value *FlashCapsuleFn(const char *name, State * state, int argc, Expr * argv[])
{
	Value *ret = NULL;
	char *filename = NULL;
	void *data = NULL;
	size_t size;

	if (ReadArgs(state, argv, 1, &filename) < 0) {
		ErrorAbort(state, "ReadArgs() failed");
		goto done;
	}

	if (filename == NULL || strlen(filename) == 0) {
		ErrorAbort(state, "filename argument to %s can't be empty", name);
		goto done;
	}

	if (file_read(filename, &data, &size)) {
		ErrorAbort(state, "file_read %s failed", filename);
		goto done;
	}

	if (flash_capsule(data, size) != 0) {
		ErrorAbort(state, "flash_capsule failed");
		goto done;
	}

	/* no error */
	ret = StringValue(strdup("t"));
done:
	if (filename)
		free(filename);
	if (data)
		free(data);

	return ret;
}

Value *FlashEspUpdateFn(const char *name, State * state, int argc, Expr * argv[])
{
	Value *ret = NULL;
	char *filename = NULL;
	void *data = NULL;
	size_t size;

	if (ReadArgs(state, argv, 1, &filename) < 0) {
		ErrorAbort(state, "ReadArgs() failed");
		goto done;
	}

	if (filename == NULL || strlen(filename) == 0) {
		ErrorAbort(state, "filename argument to %s can't be empty", name);
		goto done;
	}

	if (file_read(filename, &data, &size)) {
		ErrorAbort(state, "file_read %s failed", filename);
		goto done;
	}

	if (flash_esp_update(data, size) != 0) {
		ErrorAbort(state, "flash_esp_update failed");
		goto done;
	}

	/* no error */
	ret = StringValue(strdup("t"));
done:
	if (filename)
		free(filename);
	if (data)
		free(data);

	return ret;
}

Value *FlashUlpmcFn(const char *name, State * state, int argc, Expr * argv[])
{
	Value *ret = NULL;
	char *filename = NULL;
	void *data = NULL;
	size_t size;

	if (ReadArgs(state, argv, 1, &filename) < 0) {
		ErrorAbort(state, "ReadArgs() failed");
		goto done;
	}

	if (filename == NULL || strlen(filename) == 0) {
		ErrorAbort(state, "filename argument to %s can't be empty", name);
		goto done;
	}

	if (file_read(filename, &data, &size)) {
		ErrorAbort(state, "file_read failed %s failed", filename);
		goto done;
	}

	if (flash_ulpmc(data, size) != 0) {
		ErrorAbort(state, "flash_ulpmc failed");
		goto done;
	}

	/* no error */
	ret = StringValue(strdup("t"));
done:
	if (filename)
		free(filename);
	if (data)
		free(data);

	return ret;
}

static void recovery_error(const char *msg)
{
	fprintf(stderr, "%s", msg);
}

Value *FlashCallFunction(int (*fun) (char *), const char *name, State * state, int argc, Expr * argv[])
{
	Value *ret = NULL;
	char *filename = NULL;

	if (ReadArgs(state, argv, 1, &filename) < 0)
		goto done;

	if (fun(filename) != EXIT_SUCCESS) {
		ErrorAbort(state, "%s failed.", name);
		goto done;
	}

	ret = StringValue(strdup("t"));

done:
	if (filename)
		free(filename);
	return ret;
}

Value *CommandFunction(int (*fun) (int, char **), const char *name, State * state, int argc, Expr * argv[])
{
	Value *ret = NULL;
	char *argv_str[argc + 1];
	int i;

	char **argv_read = ReadVarArgs(state, argc, argv);
	if (argv_read == NULL) {
		ErrorAbort(state, "%s parameter parsing failed.", name);
		goto done;
	}

	argv_str[0] = (char *)name;
	for (i = 0; i < argc; i++)
		argv_str[i + 1] = argv_read[i];

	if (fun(sizeof(argv_str) / sizeof(char *), argv_str) != EXIT_SUCCESS) {
		ErrorAbort(state, "%s failed.", name);
		goto done;
	}

	for (i = 0; i < argc; i++)
		free(argv_read[i]);
	free(argv_read);

	ret = StringValue(strdup("t"));

done:
	return ret;
}

Value *FlashOSImage(const char *name, State * state, int argc, Expr * argv[])
{
	char* result = NULL;

	Value *funret = NULL;
	char *image_type;
	int ret;

	Value* partition_value;
	Value* contents;
	if (ReadValueArgs(state, argv, 2, &contents, &partition_value) < 0) {
		return NULL;
	}

	char* partition = NULL;
	if (partition_value->type != VAL_STRING) {
		ErrorAbort(state, "partition argument to %s must be string", name);
		goto exit;
	}

	partition = partition_value->data;
	if (strlen(partition) == 0) {
		ErrorAbort(state, "partition argument to %s can't be empty", name);
		goto exit;
	}

	if (contents->type == VAL_STRING && strlen((char*) contents->data) == 0) {
		ErrorAbort(state, "file argument to %s can't be empty", name);
		goto exit;
	}

	image_type = basename(partition);

	ret = flash_image(contents->data, contents->size, image_type);
	if (ret != 0) {
		ErrorAbort(state, "%s: Failed to flash image %s, %s.",
			   name, image_type, strerror(errno));
		goto free;
	}

	funret = StringValue(strdup("t"));

free:
	free(image_type);
exit:
	return funret;
}

Value *FlashImageAtOffset(const char *name, State * state, int argc, Expr * argv[])
{
	Value *funret = NULL;
	char *filename, *offset_str;
	void *data;
	off_t offset;
	int fd;
	int ret;

	if (argc != 2) {
		ErrorAbort(state, "%s: Invalid parameters.", name);
		goto exit;
	}

	if (ReadArgs(state, argv, 2, &filename, &offset_str) < 0) {
		ErrorAbort(state, "%s: ReadArgs failed.", name);
		goto exit;
	}

	char *end;
	offset = strtoul(offset_str, &end, 10);
	if (*end != '\0' || errno == ERANGE) {
		ErrorAbort(state, "%s: offset argument parsing failed.", name);
		goto free;
	}

	int length = file_size(filename);
	if (length == -1)
		goto free;

	data = file_mmap(filename, length, false);
	if (data == MAP_FAILED) {
		goto free;
	}

	fd = open(MMC_DEV_POS, O_WRONLY);
	if (fd == -1) {
		ErrorAbort(state, "%s: Failed to open %s device block, %s.",
			   name, MMC_DEV_POS, strerror(errno));
		goto unmmap_file;
	}

	ret = lseek(fd, offset, SEEK_SET);
	if (ret == -1) {
		ErrorAbort(state, "%s: Failed to seek into %s device block, %s.",
			   name, MMC_DEV_POS, strerror(errno));
		goto close;
	}

	char *ptr = (char *)data;
	ret = 0;
	for (; length; length -= ret, ptr += ret) {
		ret = write(fd, ptr, length);
		if (ret == -1) {
			ErrorAbort(state, "%s: Failed to write into %s device block, %s.",
				   name, MMC_DEV_POS, strerror(errno));
			goto close;
		}
	}

	ret = fsync(fd);
	if (ret == -1) {
		ErrorAbort(state, "%s: Failed to sync %s, %s.", name, MMC_DEV_POS, strerror(errno));
		goto close;
	}

	funret = StringValue(strdup("t"));

close:
	close(fd);
unmmap_file:
	munmap(data, length);
free:
	free(filename);
	free(offset_str);
exit:
	return funret;
}

/* Warning: USE THIS FUNCTION VERY CAUTIOUSLY. It only has been added
 * for an OTA update which REALLY needs to modify the partition
 * scheme. Before Android KitKat, we have some running services that
 * needs some partition to be mounted and thus we cannot reload the
 * partition scheme and the /dev/block/ nodes won't be accurately
 * representation of the actual partition scheme after this function
 * is called. Think about this before calling this function.  */
Value *FlashPartition(const char *name, State * state, int argc, Expr * argv[])
{
	Value *ret = NULL;

	/* Do not reload partition table during OTA since some partition
	 * are still mounted, reload would failed.  */
	oem_partition_disable_cmd_reload();

	property_set("sys.partitioning", "1");
	ret = CommandFunction(oem_partition_cmd_handler, name, state, argc, argv);
	property_set("sys.partitioning", "0");

	return ret;
}

#define PARTITION_UPDATE "/tmp/partition_update.tbl"

Value *FlashOsipToGPTPartition(const char *name, State * state, int argc, Expr * argv[])
{
	FILE *fp, *fp_update;
	char *filename;
	char buffer[K_MAX_ARG_LEN];
	char partition_type[K_MAX_ARG_LEN];
	char **gpt_argv = NULL;
	unsigned int j;
	int i, gpt_argc = 0;
	Value *ret = NULL;
	struct OSIP_header osip;


	/* Do not reload partition table during OTA since some partition
	 * are still mounted, reload would failed.  */
	oem_partition_disable_cmd_reload();

	if (argc != 1) {
		ErrorAbort(state, "%s: Invalid parameters.", name);
		return StringValue(strdup(""));
	}
	if (ReadArgs(state, argv, argc, &filename) < 0) {
		ErrorAbort(state, "%s: ReadArgs failed.", name);
		return StringValue(strdup(""));
	}

	/* Dump OSIP to ease debug and get LBA of OSIP entries */
	if (read_OSIP(&osip)) {
		fprintf(stderr, "read_OSIP fails\n");
	}
	dump_osip_header(&osip);

	/* Open partition and partition update files */
	fp = fopen(filename, "r");
	if (!fp) {
		ErrorAbort(state, "%s: Can't open %s partition file.", name, filename);
		free(filename);
		ret = StringValue(strdup(""));
		goto exit;
	}
	fp_update = fopen(PARTITION_UPDATE, "w+");
	if (!fp_update) {
		ErrorAbort(state, "%s: Can't create %s partition file\n", name, PARTITION_UPDATE);
		fclose(fp);
		free(filename);
		ret = StringValue(strdup(""));
		goto exit;
	}

	/* Check first line of partition file */
	memset(buffer, 0, sizeof(buffer));
	if (!fgets(buffer, sizeof(buffer), fp)) {
		ErrorAbort(state, "partition file is empty");
		ret = StringValue(strdup(""));
		goto error;
	}
	buffer[strlen(buffer) - 1] = '\0';
	if (sscanf(buffer, "%*[^=]=%255s", partition_type) != 1) {
		ErrorAbort(state, "partition file is invalid");
		ret = StringValue(strdup(""));
		goto error;
	}

	/* Write first line in update file */
	fprintf(fp_update, "%s\n", buffer);

	/* parse partitition files line to catch update_partitions */
	while (fgets(buffer, sizeof(buffer), fp)) {
		if (buffer[strlen(buffer) - 1] == '\n')
			buffer[strlen(buffer) - 1] = '\0';
		gpt_argv = str_to_array(buffer, &gpt_argc);
		if (gpt_argv == NULL)
			continue;
		char *command = gpt_argv[0];
		uint64_t  size, lba_start;
		int64_t osii_lba;
		bool update_need =  false;
		bool update_write = true;
		char *e;
		const char* update_partitions[] = {ANDROID_OS_NAME, RECOVERY_OS_NAME, FASTBOOT_OS_NAME};

		/* catch add commands */
		if (0 == strncmp("add", command, strlen(command))) {
			/* catch partitions to update */
			for (i = 0; i < gpt_argc; i++) {
				/* Do not write back "reserved" */
				if (0 == strncmp("reserved", gpt_argv[i], strlen(gpt_argv[i])))
					update_write = false;
				for (j = 0; j < ARRAY_SIZE(update_partitions); j++) {
					if (0 == strncmp(update_partitions[j], gpt_argv[i], strlen(gpt_argv[i]))) {
						if ((osii_lba = get_named_osii_logical_start_block(gpt_argv[i])) == -1) {
							ErrorAbort(state, "Unable to get LBA of %s partition", gpt_argv[i]);
							ret = StringValue(strdup(""));
							goto error;
						}
						printf("Found %s partition at osii_lba %"PRId64"\n", gpt_argv[i], osii_lba);
						update_need = true;
						break;
					}
				}
				if (update_need || !update_write)
					break;
			}

			/* update size and LBA */
			if (update_need) {
				for (i = 0; i < gpt_argc; i++) {
					if (0 == strncmp("-s", gpt_argv[i], strlen(gpt_argv[i]))) {
						size = strtoull(gpt_argv[i+1], &e, 0);
						if (e && *e) {
							ErrorAbort(state, "Unable to get size of partition %s", buffer);
							ret = StringValue(strdup(""));
							goto error;
						}
						printf("Size was %"PRIu64" new is %u \n", size, OS_MAX_LBA);
						sprintf(gpt_argv[i+1],"%d", OS_MAX_LBA);
					}
					if (0 == strncmp("-b", gpt_argv[i], strlen(gpt_argv[i]))) {
						lba_start = strtoull(gpt_argv[i+1], &e, 0);
						if (e && *e) {
							ErrorAbort(state, "Unable to get LBA of partition %s", buffer);
							ret = StringValue(strdup(""));
							goto error;
						}
						printf("LBA was %"PRIu64" new is %"PRId64" \n", lba_start, osii_lba);
						sprintf(gpt_argv[i+1],"%"PRId64"", osii_lba);
					}
				}
			}
		}

		/* write lines in partition update */
		for (i = 0; i < gpt_argc; i++) {
			if (update_write)
				fprintf(fp_update,"%s ",gpt_argv[i]);
			free(gpt_argv[i]);
			gpt_argv[i] = NULL;
		}
		if (update_write)
			fprintf(fp_update,"\n");
		free(gpt_argv);
		gpt_argv = NULL;
	}

	/* Ensure to close fp_update as it will be re-open by oem_partition_cmd_handler */
	free(filename);
	fclose(fp);
	fclose(fp_update);

	/* partition with new partition file */
	property_set("sys.partitioning", "1");

	char *path[] = {0,PARTITION_UPDATE};
	int partret = -1;

	if ((partret = oem_partition_cmd_handler(2,(char **)path)) == -1) {
		ErrorAbort(state, "%s: re-partitionning fails with error %d", name, partret);
		ret = StringValue(strdup(""));
	}
	else
		ret = StringValue(strdup("t"));
	property_set("sys.partitioning", "0");

	goto exit;

error:
	free(filename);
	fclose(fp);
	fclose(fp_update);
exit:
	return ret;
}


Value *FlashImageAtPartition(const char *name, State * state, int argc, Expr * argv[])
{
	Value *funret = NULL;
	char *osname, *filename, *parttable;
	void *data;
	off_t offset = 0;
	int fd;
	FILE *fp;
	char buffer[K_MAX_ARG_LEN];
	char partition_type[K_MAX_ARG_LEN];
	char **gpt_argv = NULL;
	int i, gpt_argc = 0;
	Value *ret = NULL;
	int fret;

	if (argc != 3) {
		ErrorAbort(state, "%s: Invalid parameters.", name);
		goto exit;
	}

	if (ReadArgs(state, argv, 3, &osname, &filename, &parttable) < 0) {
		ErrorAbort(state, "%s: ReadArgs failed.", name);
		goto exit;
	}

	/* Open partition file */
	fp = fopen(parttable, "r");
	if (!fp) {
		ErrorAbort(state, "%s: Can't open %s partition file.", name, parttable);
		ret = StringValue(strdup(""));
		goto free;
	}

	/* Check first line of partition file */
	memset(buffer, 0, sizeof(buffer));
	if (!fgets(buffer, sizeof(buffer), fp)) {
		ErrorAbort(state, "partition file is empty");
		ret = StringValue(strdup(""));
		goto free;
	}
	buffer[strlen(buffer) - 1] = '\0';
	if (sscanf(buffer, "%*[^=]=%255s", partition_type) != 1) {
		ErrorAbort(state, "partition file is invalid");
		ret = StringValue(strdup(""));
		goto free;
	}

	bool found = false;
	/* parse partitition files line to catch osname */
	while (fgets(buffer, sizeof(buffer), fp)) {
		if (buffer[strlen(buffer) - 1] == '\n')
			buffer[strlen(buffer) - 1] = '\0';
		gpt_argv = str_to_array(buffer, &gpt_argc);
		if (gpt_argv == NULL)
			continue;
		char *command = gpt_argv[0];
		char* e;
		/* catch add commands */
		if (0 == strncmp("add", command, strlen(command))) {
			/* catch partitions to update */
			for (i=0; i < gpt_argc; i++) {
				if (0 == strncmp(osname, gpt_argv[i], strlen(gpt_argv[i]))) {
					found = true;
					printf("found %s ", osname);
				}
			}
			/* get offset of osname */
			if (found) {
				for (i=0; i < gpt_argc; i++) {
					if (0 == strncmp("-b", gpt_argv[i], strlen(gpt_argv[i]))) {
						offset = strtoull(gpt_argv[i+1], &e, 0) * 512;
						if (e && *e) {
							ErrorAbort(state, "Unable to get LBA of partition %s", buffer);
							ret = StringValue(strdup(""));
							goto free;
						}
						printf("at LBA %s offset % ld \n", gpt_argv[i+1], offset);
					}
				}
			}
		}
		/* free gpt_argv */
		for (i = 0; i < gpt_argc; i++) {
			free(gpt_argv[i]);
			gpt_argv[i] = NULL;
		}
		free(gpt_argv);
		gpt_argv = NULL;
		if (found)
			break;
	}
	fclose(fp);

	if (!found) {
		ErrorAbort(state, "partition %s not found in %s", name, parttable);
		ret = StringValue(strdup(""));
		goto free;
	} else {
		printf("%s: found partition %s at offset %ld\n", name, osname, offset);
	}

	/* Flash image at offset */
	int length = file_size(filename);
	if (length == -1)
		goto free;

	data = file_mmap(filename, length, false);
	if (data == MAP_FAILED) {
		goto free;
	}

	fd = open(MMC_DEV_POS, O_WRONLY);
	if (fd == -1) {
		ErrorAbort(state, "%s: Failed to open %s device block, %s.",
			   name, MMC_DEV_POS, strerror(errno));
		goto unmmap_file;
	}

	fret = lseek(fd, offset, SEEK_SET);
	if (fret == -1) {
		ErrorAbort(state, "%s: Failed to seek into %s device block, %s.",
			   name, MMC_DEV_POS, strerror(errno));
		ret = StringValue(strdup(""));
		goto close;
	}

	char *ptr = (char *)data;
	fret = 0;
	for (; length; length -= fret, ptr += fret) {
		fret = write(fd, ptr, length);
		if (fret == -1) {
			ErrorAbort(state, "%s: Failed to write into %s device block, %s.",
				   name, MMC_DEV_POS, strerror(errno));
			ret = StringValue(strdup(""));
			goto close;
		}
	}

	fret = fsync(fd);
	if (fret == -1) {
		ErrorAbort(state, "%s: Failed to sync %s, %s.", name, MMC_DEV_POS, strerror(errno));
		ret = StringValue(strdup(""));
		goto close;
	}

	ret = StringValue(strdup("t"));

close:
	close(fd);
unmmap_file:
	munmap(data, length);
free:
	free(osname);
	free(filename);
	free(parttable);
exit:
	return ret;
}


Value *EraseOsipHeader(const char *name, State * state, int argc, Expr * argv[])
{
	Value *ret = NULL;
	printf("Erase OSIP header \n");
	ret = CommandFunction(oem_erase_osip_header, name, state, argc, argv);
	return ret;
}

void Register_libintel_updater(void)
{
	RegisterFunction("flash_ifwi", FlashIfwiFn);
#ifdef TEE_FRAMEWORK
	RegisterFunction("flash_bom_token", FlashBomFn);
#endif	/* TEE_FRAMEWORK */

	RegisterFunction("extract_image", ExtractImageFn);
	RegisterFunction("invalidate_os", InvalidateOsFn);
	RegisterFunction("restore_os", RestoreOsFn);

	RegisterFunction("flash_capsule", FlashCapsuleFn);
	RegisterFunction("flash_esp_update", FlashEspUpdateFn);
	RegisterFunction("flash_ulpmc", FlashUlpmcFn);
	RegisterFunction("flash_partition", FlashPartition);
	RegisterFunction("flash_osiptogpt_partition", FlashOsipToGPTPartition);
	RegisterFunction("flash_image_at_partition", FlashImageAtPartition);
	RegisterFunction("flash_image_at_offset", FlashImageAtOffset);
	RegisterFunction("flash_os_image", FlashOSImage);
	RegisterFunction("write_osip_image", FlashOSImage);
	RegisterFunction("erase_osip", EraseOsipHeader);
	RegisterFunction("restore_os", RestoreOsFn);

	util_init(recovery_error, NULL);
}
