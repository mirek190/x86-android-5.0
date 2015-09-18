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

#include <efi.h>
#include <efilib.h>
#include <log.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <uefi_utils.h>
#include <bootimg.h>
#include <tco_reset.h>
#include <gpt.h>

#include "fastboot_usb.h"
#include "flash.h"

#define MAGIC_LENGTH 64
#define MAX_DOWNLOAD_SIZE 500*1024*1024
#define MAX_VARIABLE_LENGTH 128

struct fastboot_cmd {
	struct fastboot_cmd *next;
	const char *prefix;
	unsigned prefix_len;
	void (*handle) (char *arg, void **addr, unsigned *sz);
};

struct fastboot_var {
	struct fastboot_var *next;
	char name[MAX_VARIABLE_LENGTH];
	char value[MAX_VARIABLE_LENGTH];
};

enum fastboot_states {
	STATE_OFFLINE,
	STATE_COMMAND,
	STATE_COMPLETE,
	STATE_DOWNLOAD,
	STATE_GETVAR,
	STATE_ERROR,
};

EFI_GUID guid_linux_data = {0xebd0a0a2, 0xb9e5, 0x4433, {0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7}};

static struct fastboot_cmd *cmdlist;
static char command_buffer[MAGIC_LENGTH];
static struct fastboot_var *varlist;
static enum fastboot_states fastboot_state = STATE_OFFLINE;

void fastboot_register(const char *prefix,
		       void (*handle) (char *arg, void **addr, unsigned *sz))
{
	struct fastboot_cmd *cmd;
	cmd = AllocatePool(sizeof(*cmd));
	if (!cmd) {
		error(L"Failed to allocate fastboot command %a\n", prefix);
		return;
	}
	cmd->prefix = prefix;
	cmd->prefix_len = strlen(prefix);
	cmd->handle = handle;
	cmd->next = cmdlist;
	cmdlist = cmd;
}

void fastboot_publish(const char *name, const char *value)
{
	struct fastboot_var *var;
	UINTN namelen = strlena((CHAR8 *) name) + 1;
	UINTN valuelen = strlena((CHAR8 *) value) + 1;

	if (namelen > sizeof(var->name) ||
		valuelen > sizeof(var->value)) {
		error(L"name or value too long\n");
		return;
	}
	var = AllocateZeroPool(sizeof(*var));
	if (!var) {
		error(L"Failed to allocate variable %a\n", name);
		return;
	}
	CopyMem(var->name, name, namelen);
	CopyMem(var->value, value, valuelen);
	var->next = varlist;
	varlist = var;
}

const char *fastboot_getvar(const char *name)
{
	struct fastboot_var *var;
	const char *ret = NULL;

	for (var = varlist; var; var = var->next) {
		if (!strcmp(name, var->name)) {
			ret = var->value;
			break;
		}
	}

	return ret;
}

static void fastboot_ack(const char *code, const char *format, va_list ap)
{
	char response[MAGIC_LENGTH];
	char reason[MAGIC_LENGTH];
	int i;

	if (vsnprintf(reason, MAGIC_LENGTH, format, ap) < 0) {
		error(L"Failed to build reason string\n");
		return;
	}
	ZeroMem(response, sizeof(response));

	/* Nip off trailing newlines */
	for (i = strlen(reason); (i > 0) && reason[i - 1] == '\n'; i--)
		reason[i - 1] = '\0';
	snprintf(response, MAGIC_LENGTH, "%a%a", code, reason);
	debug(L"ack %a %a\n", code, reason);
	if (usb_write(response, MAGIC_LENGTH) < 0)
		fastboot_state = STATE_ERROR;
}

void fastboot_info(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fastboot_ack("INFO", fmt, ap);
	va_end(ap);
}

void fastboot_fail(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fastboot_ack("FAIL", fmt, ap);
	va_end(ap);

	fastboot_state = STATE_COMPLETE;
}

void fastboot_okay(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fastboot_ack("OKAY", fmt, ap);
	va_end(ap);

	fastboot_state = STATE_COMPLETE;
}

static void cmd_flash(char *arg, void **addr, unsigned *sz)
{
	EFI_STATUS ret;
	CHAR16 *label = stra_to_str((CHAR8*)arg);
	if (!label) {
		error(L"Failed to get label %a\n", arg);
		fastboot_fail("Allocation error");
		return;
	}

	ret = flash(*addr, *sz, label);
	FreePool(label);
	if (EFI_ERROR(ret))
		fastboot_fail("Flash failure: %r", ret);
	else
		fastboot_okay("");
}

static void cmd_erase(char *arg, void **addr, unsigned *sz)
{
	EFI_STATUS ret;
	CHAR16 *label = stra_to_str((CHAR8*)arg);
	if (!label) {
		error(L"Failed to get label %a\n", arg);
		fastboot_fail("Allocation error");
		return;
	}

	ret = erase_by_label(label);
	FreePool(label);
	if (EFI_ERROR(ret))
		fastboot_fail("Flash failure: %r", ret);
	else
		fastboot_okay("");
}
static void boot_ok(void)
{
	fastboot_okay("");
}

static void cmd_boot(char *arg, void **addr, unsigned *sz)
{
	struct bootimg_hooks hooks;
	EFI_STATUS ret;

	hooks.before_exit = boot_ok;
	hooks.watchdog = tco_start_watchdog;
	hooks.before_jump = NULL;

	ret = android_image_start_buffer(*addr, NULL, &hooks);

	fastboot_fail("boot failure: %r", ret);
}

static void worker_getvar_all(struct fastboot_var *start)
{
	static struct fastboot_var *var;
	if (start)
		var = start;

	if (var) {
		fastboot_info("%a: %a", var->name, var->value);
		var = var->next;
	} else
		fastboot_okay("");
}

static void cmd_getvar(char *arg, void **addr, unsigned *sz)
{
	if (!strcmp(arg, "all")) {
		fastboot_state = STATE_GETVAR;
		worker_getvar_all(varlist);
	} else {
		const char *value;
		value = fastboot_getvar(arg);
		if (value) {
			fastboot_okay("%a", value);
		} else {
			fastboot_okay("");
		}
	}
}

static void cmd_reboot(char *arg, void **addr, unsigned *sz)
{
	uefi_reset_system(EfiResetCold);
}

static void fastboot_read_command(void)
{
	usb_read(command_buffer, sizeof(command_buffer));
}

static void cmd_download(char *arg, void **addr, unsigned *sz)
{
	char response[MAGIC_LENGTH];

	*sz = strtoul(arg, NULL, 16);
	debug(L"Receiving %d bytes\n", *sz);

	if (*sz > MAX_DOWNLOAD_SIZE) {
		fastboot_fail("data too large");
		return;
	}

	sprintf(response, "DATA%08x", *sz);
	if (usb_write(response, strlen(response)) < 0) {
		fastboot_state = STATE_ERROR;
		return;
	}

	*addr = AllocatePool(*sz);
	if (!*addr) {
		error(L"Failed to allocate download buffer (0x%x bytes)\n", *sz);
		fastboot_fail("Memory allocation failure");
		return;
	}

	if (usb_read(*addr, *sz)) {
		error(L"Failed to receive %d bytes\n", *sz);
		fastboot_fail("Usb receive failed");
		return;
	}
	fastboot_state = STATE_DOWNLOAD;
}

static void fastboot_process_tx(void *buf, unsigned len)
{
	switch (fastboot_state) {
	case STATE_GETVAR:
		worker_getvar_all(NULL);
		break;
	case STATE_COMPLETE:
		fastboot_read_command();
		break;
	default:
		/* Nothing to do */
		break;
	}
}

static void fastboot_process_rx(void *buf, unsigned len)
{
	struct fastboot_cmd *cmd;
	static void *addr = NULL;
	static unsigned download_size = 0;

	switch (fastboot_state) {
	case STATE_DOWNLOAD:
		fastboot_state = STATE_COMMAND;
		if (len == download_size)
			fastboot_okay("");
		else {
			fastboot_fail("Received 0x%x bytes on 0x%x\n", len, download_size);
			download_size = 0;
			FreePool(addr);
		}
		break;
	case STATE_COMPLETE:
		((char *)buf)[len] = 0;
		debug(L"fastboot got command: %a\n", (char *)buf);

		fastboot_state = STATE_COMMAND;
		for (cmd = cmdlist; cmd; cmd = cmd->next) {
			if (memcmp(buf, cmd->prefix, cmd->prefix_len))
				continue;

			cmd->handle((char *)buf + cmd->prefix_len,
				    &addr, &download_size);

			if (fastboot_state == STATE_COMMAND)
				fastboot_fail("unknown reason");
			break;
		}
		if (!cmd) {
			error(L"unknown command '%a'\n", buf);
			fastboot_fail("unknown command");
		}
		break;
	default:
		error(L"Inconsistent fastboot state: 0x%x\n", fastboot_state);
	}
}

static void fastboot_start_callback(void)
{
	fastboot_state = STATE_COMPLETE;
	fastboot_read_command();
}

static void publish_partsize(void)
{
	struct gpt_partition_interface *gparti;
	UINTN part_count;
	UINTN i;

	gpt_list_partition(&gparti, &part_count);

	for (i = 0; i < part_count; i++) {
		char fastboot_var[MAX_VARIABLE_LENGTH];
		char partsize[MAX_VARIABLE_LENGTH];
		UINT64 size;

		size = gparti[i].bio->Media->BlockSize * (gparti[i].part.ending_lba + 1 - gparti[i].part.starting_lba);

		if (snprintf(fastboot_var, sizeof(fastboot_var), "partition-size:%s", gparti[i].part.name) < 0)
			continue;
		if (snprintf(partsize, sizeof(partsize), "0x%lX", size) < 0)
			continue;

		fastboot_publish(fastboot_var, partsize);

		if (snprintf(fastboot_var, sizeof(fastboot_var), "partition-type:%s", gparti[i].part.name) < 0)
			continue;

		if (!CompareGuid(&gparti[i].part.type, &guid_linux_data))
			fastboot_publish(fastboot_var, "ext4");
		else
			fastboot_publish(fastboot_var, "none");
	}
}

int fastboot_start()
{
	char download_max_str[30];

	if (snprintf(download_max_str, sizeof(download_max_str), "0x%lX", MAX_DOWNLOAD_SIZE))
		warning(L"Failed to set download_max_str string\n");
	else
		fastboot_publish("max-download-size", download_max_str);

	fastboot_register("reboot", cmd_reboot);
	fastboot_register("flash:", cmd_flash);
	fastboot_register("getvar:", cmd_getvar);
	fastboot_register("download:", cmd_download);
	fastboot_register("boot", cmd_boot);
	fastboot_register("erase:", cmd_erase);
	publish_partsize();

	fastboot_usb_start(fastboot_start_callback, fastboot_process_rx, fastboot_process_tx);

	return 0;
}
