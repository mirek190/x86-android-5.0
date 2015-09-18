/*
 * Copyright (c) 2013-2014, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "uefivar.h"
#include "inc/libuefivar.h"

#define LOG_TAG "libuefivar"
#include <utils/Log.h>

static bool verbose = 0;

#define UEFIVAR_MAX_STR_SIZE 1024

static struct efi_var *uefi_create_var(struct arguments *args) {
	struct efi_var *var;

	var = calloc(1, sizeof(*var));
	if (!var)
		return NULL;

	size_t len = strlen(args->varname);
	do {
		var->name[len * 2] = args->varname[len];
	} while (len--);


	var->guid = args->guid;
	var->attributes = EFI_DEFAULT_ATTRIBUTES;

	return var;
}

static int uefivar_allocate_buffer(struct arguments args, off_t *entry_size, char **path, struct efi_var **buffer) {
	struct stat st;
	int ret;

	ret = asprintf(path, EFI_VAR_PATH, args.varname, args.guid_str);
	if (ret == -1) {
		error("Failed to build EFI variable path\n");
		return -ENOMEM;
	}

	ret = stat(*path, &st);
	if (ret != 0) {
		if (errno == ENOENT && args.value) {
			*buffer = uefi_create_var(&args);
			if (!*buffer){
				free(*path);
				return -ENOMEM;
			}
			return ENOENT;
		} else {
			error("Failed to stat file: %s\n", strerror(errno));
			free(*path);
			return -EFAULT;
		}
	} else {
		*entry_size = st.st_size;
		debug("File size is 0x%llx bytes\n", st.st_size);
	}

	*buffer = malloc(st.st_size);
	if (!*buffer) {
		error("Failed to allocate buffer: %s\n", strerror(errno));
		free(*path);
		return -ENOMEM;
	}

	return 0;
}

void uefivar_free_buffer(struct efi_var **buffer) {
	free(*buffer);
	*buffer = NULL;
}

int uefi_set_var(int fd, struct efi_var *var) {
	int ret;

	ret = write(fd, (char *)var, sizeof(*var));

	if (ret < 0) {
		error("Write failed: %s\n", strerror(errno));
		return ret;
	}

	if (ret != sizeof(*var)) {
		error("Wrote only 0x%x/0x%x bytes: %s\n", ret, sizeof(*var), strerror(errno));
		return -EBADE;
	}

	return 0;
}

int uefi_get_value(char *buffer, char *type) {
	struct efi_var *var = (struct efi_var *)buffer;

	if (!strcmp(type, "int")) {
		unsigned long long int *i;
		if ((unsigned)var->size > sizeof(*i)) {
#ifdef KERNEL_X86_64
			error("Variable size (%llu bytes) is larger than long long integer size (%d bytes)\n", var->size, sizeof(*i));
#else
			error("Variable size (%lu bytes) is larger than long long integer size (%d bytes)\n", var->size, sizeof(*i));
#endif
			return -EINVAL;
		}

		i = (unsigned long long int *)var->data;
		info("%llu\n", *i);
		return 0;
	} else if (!strcmp(type, "string")) {
		var->data[var->size - 1] = '\0';
		info("%s\n", var->data);
		return 0;
	}

	error("Value type %s is not supported.\n", type);
	return -EINVAL;
}

static int uefi_set_value(struct efi_var *var, char *value, unsigned long value_size, bool keep_size) {
	if (keep_size && (value_size > var->size)) {
#ifdef KERNEL_X86_64
		error("New value is larger than original size (%lu > %llu) and keep_size=1\n", value_size, var->size);
#else
		error("New value is larger than original size (%lu > %lu) and keep_size=1\n", value_size, var->size);
#endif
		return -EINVAL;
	}

	if (!keep_size && value_size > (int)sizeof(var->data))
		return -ENOMEM;

	if (!keep_size)
		var->size = value_size;

	memcpy(var->data, value, value_size);
	memset(var->data + value_size, 0, sizeof(var->data) - value_size);

	return 0;
}

int parse_guid(const char *guidstr, struct guid *guid) {
	/* Use an oversized guid buffer in order to simplify the code
	 * by using the sscanf() function.  */
	char buffer[sizeof(struct guid) + sizeof(unsigned int)];
	struct guid *tmp = (struct guid *)buffer;
	int ret;

	ret = sscanf(guidstr, GUID_FORMAT,
			&tmp->part1, (unsigned int *)&tmp->part2,
			(unsigned int *)&tmp->part3, (unsigned int *)&tmp->part4[0],
			(unsigned int *)&tmp->part4[1], (unsigned int *)&tmp->part4[2],
			(unsigned int *)&tmp->part4[3], (unsigned int *)&tmp->part4[4],
			(unsigned int *)&tmp->part4[5], (unsigned int *)&tmp->part4[6],
			(unsigned int *)&tmp->part4[7]);

	if (ret != GUID_FORMAT_ELT_NB) {
		error("Invalid GUID format: %s\n", optarg);
		return -EINVAL;
	}

	*guid = *tmp;

	return 0;
}

int uefivar_process_entry(const struct arguments args, struct efi_var **buffer) {
	int ret;
	int i, fd;
	int var_size = sizeof(struct efi_var);
	char *path;
	off_t entry_size;

	*buffer = NULL;
	ret = uefivar_allocate_buffer(args, &entry_size, &path, buffer);

	if (ret < 0)
		goto out;

	if (ret == 0) {
		fd = open(path, O_RDWR);
		if (fd < 0) {
			error("Failed to open file: %s\n", strerror(errno));
			ret = -EIO;
			goto free_path;
		}

		var_size = 0;
		do {
			i = read(fd, *buffer + var_size, entry_size - var_size);
			if (i < 0) {
				error("Failed to read file: %s\n", strerror(errno));
				goto close;
			}
			var_size += i;
		} while (i != 0 && var_size < entry_size);
		debug("Read 0x%x bytes\n", var_size);
	} else if (ret == ENOENT) {
		if (!args.value) {
			ret = -ENOENT;
			goto free_path;
		}

		fd = open(EFI_NEW_VAR_PATH, O_WRONLY);
		if (fd < 0) {
			error("Failed to open file: %s\n", strerror(errno));
			ret = -EIO;
			goto free_path;
		}
	}

	struct efi_var *var = (struct efi_var *) *buffer;
	if (var_size != sizeof(*var)) {
		error("EFI variable size is inconsistent with internal definition\n");
		goto close;
	}

	if (args.value) {
		ret = uefi_set_value(var, args.value, args.value_size, args.keep_size);
		if (ret) {
			error("Failed to set value\n");
			goto close;
		}
		ret = uefi_set_var(fd, var);
		if (ret < 0) {
			error("Failed to set variable %s-%s\n", args.varname, args.guid_str);
			goto close;
		}
	}

	ret = 0;
close:
	close(fd);
free_path:
	free(path);
out:
	if (ret < 0)
		uefivar_free_buffer(buffer);

	return ret;
}

int libuefivar_read_string(const char *name, const char *guid, char *result, unsigned int maxlen) {
	int ret;
	struct efi_var *buffer = NULL;
	struct arguments args;

	if (!name || !guid || !result)
		return -EINVAL;

	args.varname = (char *)name;
	args.guid_str = (char *)guid;
	args.value = NULL;

	if ((ret = uefivar_process_entry(args, &buffer)) < 0)
		return ret;

	maxlen = (maxlen > buffer->size) ? buffer->size : maxlen;
	memcpy(result, buffer->data, maxlen);
	uefivar_free_buffer(&buffer);

	return maxlen;
}

int libuefivar_set_string(const char *name, const char *guid, char *input, unsigned int length) {
	int ret = -EINVAL;
	struct efi_var *buffer = NULL;
	struct arguments args;

	if (!name || !guid || !input || (length > UEFIVAR_MAX_STR_SIZE))
		return ret;

	args.varname = (char *)name;
	args.guid_str = (char *)guid;
	args.value = input;
	args.value_size = length;
	args.keep_size = 0;

	if (!parse_guid(guid, &args.guid)) {
		ret = uefivar_process_entry(args, &buffer);
		uefivar_free_buffer(&buffer);
	}

	return ret;
}

int libuefivar_read_int(const char *name, const char *guid, unsigned long long int *result) {
	int ret;
	struct efi_var *buffer = NULL;
	struct arguments args;

	if (!name || !guid || !result)
		return -EINVAL;

	args.varname = (char *)name;
	args.guid_str = (char *)guid;
	args.value = NULL;

	if ((ret = uefivar_process_entry(args, &buffer)) < 0)
		return ret;

	if ((unsigned)buffer->size > sizeof(*result)) {
		uefivar_free_buffer(&buffer);
		return -EINVAL;
	}

	memcpy(result, buffer->data, sizeof(*result));
	uefivar_free_buffer(&buffer);

	return ret;
}

int libuefivar_set_int(const char *name, const char *guid, unsigned long long int value) {
	int ret = -EINVAL;
	struct efi_var *buffer = NULL;
	struct arguments args;

	if (!name || !guid)
		return ret;

	args.varname = (char *)name;
	args.guid_str = (char *)guid;
	args.value = (char*)&value;
	args.value_size = sizeof(value);
	args.keep_size = 0;

	if (!parse_guid(guid, &args.guid)) {
		ret = uefivar_process_entry(args, &buffer);
		uefivar_free_buffer(&buffer);
	}

	return ret;
}
