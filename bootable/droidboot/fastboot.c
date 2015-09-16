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
#define LOG_TAG "fastboot"

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <cutils/properties.h>
#include <netinet/in.h>
#include <poll.h>
#include <linux/usb/ch9.h>
#include <linux/usb/functionfs.h>

#include "volumeutils/roots.h"
#include "droidboot.h"
#include "droidboot_ui.h"
#include "fastboot.h"
#include "droidboot_util.h"

#define FILEMODE  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define MAGIC_LENGTH 64

#define USB_ADB_PATH      "/dev/android_adb"
#define USB_FFS_ADB_PATH  "/dev/usb-ffs/adb/"
#define USB_FFS_ADB_EP(x) USB_FFS_ADB_PATH#x
#define USB_FFS_ADB_EP0   USB_FFS_ADB_EP(ep0)
#define USB_FFS_ADB_OUT   USB_FFS_ADB_EP(ep1)
#define USB_FFS_ADB_IN    USB_FFS_ADB_EP(ep2)

#define MAX_USB_FFS_ADB_EP0_TRIES 30
#define DELAY_USB_FFS_ADB_EP0_TRIES 100000 /* in us */

#define ADB_CLASS              0xff
#define ADB_SUBCLASS           0x42
#define FASTBOOT_PROTOCOL      0x3

#define MAX_PACKET_SIZE_FS	64
#define MAX_PACKET_SIZE_HS	512
#define MAX_PACKET_SIZE_SS	1024
#define MAX_FASTBOOT_RESPONSE_SIZE	65

#define cpu_to_le16(x)  htole16(x)
#define cpu_to_le32(x)  htole32(x)

struct usb_fp
{
	int read_fp;
	int write_fp;
};
static struct usb_fp usb;

static const struct {
	struct usb_functionfs_descs_head header;
	struct {
		struct usb_interface_descriptor intf;
		struct usb_endpoint_descriptor_no_audio source;
		struct usb_endpoint_descriptor_no_audio sink;
	} __attribute__((packed)) fs_descs, hs_descs;
	struct {
		struct usb_interface_descriptor intf;
		struct usb_endpoint_descriptor_no_audio source;
		struct usb_ss_ep_comp_descriptor source_comp;
		struct usb_endpoint_descriptor_no_audio sink;
		struct usb_ss_ep_comp_descriptor sink_comp;
	} __attribute__((packed)) ss_descs;
} __attribute__((packed)) descriptors = {
	.header = {
		.magic = cpu_to_le32(FUNCTIONFS_DESCRIPTORS_MAGIC),
		.length = cpu_to_le32(sizeof(descriptors)),
		.fs_count = 3,
		.hs_count = 3,
		.ss_count = 5,
	},
	.fs_descs = {
		.intf = {
			.bLength = sizeof(descriptors.fs_descs.intf),
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0,
			.bNumEndpoints = 2,
			.bInterfaceClass = ADB_CLASS,
			.bInterfaceSubClass = ADB_SUBCLASS,
			.bInterfaceProtocol = FASTBOOT_PROTOCOL,
			.iInterface = 1, /* first string from the provided table */
		},
		.source = {
			.bLength = sizeof(descriptors.fs_descs.source),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 1 | USB_DIR_OUT,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = MAX_PACKET_SIZE_FS,
		},
		.sink = {
			.bLength = sizeof(descriptors.fs_descs.sink),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 2 | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = MAX_PACKET_SIZE_FS,
		},
	},
	.hs_descs = {
		.intf = {
			.bLength = sizeof(descriptors.hs_descs.intf),
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0,
			.bNumEndpoints = 2,
			.bInterfaceClass = ADB_CLASS,
			.bInterfaceSubClass = ADB_SUBCLASS,
			.bInterfaceProtocol = FASTBOOT_PROTOCOL,
			.iInterface = 1, /* first string from the provided table */
		},
		.source = {
			.bLength = sizeof(descriptors.hs_descs.source),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 1 | USB_DIR_OUT,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = MAX_PACKET_SIZE_HS,
		},
		.sink = {
			.bLength = sizeof(descriptors.hs_descs.sink),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 2 | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = MAX_PACKET_SIZE_HS,
		},
	},
	.ss_descs = {
		.intf = {
			.bLength = sizeof(descriptors.ss_descs.intf),
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0,
			.bNumEndpoints = 2,
			.bInterfaceClass = ADB_CLASS,
			.bInterfaceSubClass = ADB_SUBCLASS,
			.bInterfaceProtocol = FASTBOOT_PROTOCOL,
			.iInterface = 1, /* first string from the provided table */
		},
		.source = {
			.bLength = sizeof(descriptors.ss_descs.source),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 1 | USB_DIR_OUT,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = MAX_PACKET_SIZE_SS,
		},
		.source_comp = {
			.bLength = sizeof(descriptors.ss_descs.source_comp),
			.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,
		},
		.sink = {
			.bLength = sizeof(descriptors.ss_descs.sink),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 2 | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = MAX_PACKET_SIZE_SS,
		},
		.sink_comp = {
			.bLength = sizeof(descriptors.ss_descs.sink_comp),
			.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,
		},
	},
};

#define STR_INTERFACE_ "FASTBOOT Interface"

static const struct {
	struct usb_functionfs_strings_head header;
	struct {
		__le16 code;
		const char str1[sizeof(STR_INTERFACE_)];
	} __attribute__((packed)) lang0;
} __attribute__((packed)) strings = {
	.header = {
		.magic = cpu_to_le32(FUNCTIONFS_STRINGS_MAGIC),
		.length = cpu_to_le32(sizeof(strings)),
		.str_count = cpu_to_le32(1),
		.lang_count = cpu_to_le32(1),
	},
	.lang0 = {
		cpu_to_le16(0x0409), /* en-us */
		STR_INTERFACE_,
	},
};

extern int g_disable_fboot_ui;

void fastboot_register(const char *prefix,
		       void (*handle) (const char *arg, void *data,
				       unsigned sz))
{
	struct fastboot_cmd *cmd;
	cmd = malloc(sizeof(*cmd));
	if (cmd) {
		cmd->prefix = prefix;
		cmd->prefix_len = strlen(prefix);
		cmd->handle = handle;
		cmd->next = cmdlist;
		cmdlist = cmd;
	}
}

static struct fastboot_var *varlist;

void fastboot_publish(const char *name, const char *value,
		      const char *(*fun)(const char *))
{
	struct fastboot_var *var;
	var = malloc(sizeof(*var));
	if (var) {
		var->name = name;
		var->value = value;
		var->fun = fun;
		var->next = varlist;
		varlist = var;
	}
}

const char *fastboot_getvar(const char *name)
{
	struct fastboot_var *var;
	const char *arg;
	size_t len;

	arg = strstr(name, ":");
	len = arg ? (size_t)(arg - name) : strlen(name);

	for (var = varlist; var; var = var->next)
		if (!strncmp(var->name, name, len))
			return var->fun ? var->fun(arg + 1) : var->value;

	return NULL;
}

static unsigned char buffer[4096];

static void *download_base;
static unsigned download_max;
static unsigned download_size;

#define STATE_OFFLINE	0
#define STATE_COMMAND	1
#define STATE_COMPLETE	2
#define STATE_ERROR	3

static unsigned fastboot_state = STATE_OFFLINE;
static int tmp_fp = -1;

#define USB_CHUNK_SIZE (1024 * 1024)
static int usb_read(void *_buf, unsigned len)
{
	int r = 0;
	unsigned xfer;
	unsigned char *buf = _buf;
	int count = 0;
	unsigned const len_orig = len;

	if (fastboot_state == STATE_ERROR)
		goto oops;

	pr_verbose("usb_read %d\n", len);
	while (len > 0) {
		xfer = (len > USB_CHUNK_SIZE) ? USB_CHUNK_SIZE : len;

		r = read(usb.read_fp, buf, xfer);
		if (r < 0) {
			pr_warning("read");
			goto oops;
		} else if (r == 0) {
			pr_info("Connection closed\n");
			goto oops;
		}
		if (tmp_fp < 0) {
			buf += r;
		} else {
			/* we dont have enough memory in scratch
			   write directly in /cache */
			int w = 0, rv;
			while (w < r) {
				rv = write(tmp_fp, buf+w, r-w);
				if (rv == -1) {
					pr_perror("write to tmpfile");
					goto oops;
				}
				w += rv;
			}
		}
		count += r;
		len -= r;

		/* Fastboot proto is badly specified. Corner case where it will fail:
		 * file download is MAGIC_LENGTH. Transport has MTU < MAGIC_LENGTH.
		 * Will be necessary to retry after short read, but break below will
		 * prevent it.
		 */
		if (len_orig == MAGIC_LENGTH)
			break;
	}
	pr_verbose("usb_read complete\n");
	return count;

oops:
	fastboot_state = STATE_ERROR;
	return -1;
}

static int usb_write(void *_buf, unsigned len)
{
	int r;
	size_t count = 0;
	unsigned char *buf = _buf;

	pr_verbose("usb_write %d\n", len);
	if (fastboot_state == STATE_ERROR)
		goto oops;

	do {
		r = write(usb.write_fp, buf + count, len - count);
		if (r < 0) {
			pr_perror("write");
			goto oops;
		}
		 count += r;
	} while (count < len);

	return r;

oops:
	fastboot_state = STATE_ERROR;
	return -1;
}

void fastboot_ack(const char *code, const char *reason)
{
	char response[MAGIC_LENGTH];

	if (fastboot_state != STATE_COMMAND)
		return;

	if (reason == 0)
		reason = "";

	snprintf(response, MAGIC_LENGTH, "%s%s", code, reason);
	fastboot_state = STATE_COMPLETE;

	usb_write(response, strlen(response));

}

void fastboot_info(const char *info)
{
	char response[MAX_FASTBOOT_RESPONSE_SIZE];

	if (fastboot_state != STATE_COMMAND)
		return;

	snprintf(response, sizeof(response), "%s%s", "INFO", info);
	usb_write(response, strlen(response));
}

#define TEMP_BUFFER_SIZE		512
#define RESULT_FAIL_STRING		"RESULT: FAIL("
void fastboot_fail(const char *reason)
{
	char buf[TEMP_BUFFER_SIZE];

	sprintf(buf, RESULT_FAIL_STRING);
	strncat(buf, reason, TEMP_BUFFER_SIZE - 2 - strlen(RESULT_FAIL_STRING));
	strcat(buf, ")");

	if (!g_disable_fboot_ui) {
		ui_msg(ALERT, "%s", buf);
	}
	fastboot_ack("FAIL", reason);
}

void fastboot_okay(const char *info)
{
	if (!g_disable_fboot_ui) {
		ui_msg(TIPS, "RESULT: OKAY");
	}
	fastboot_ack("OKAY", info);
}

static void cmd_getvar(const char *arg, void *data, unsigned sz)
{
	struct Volume_list *vol_list;

	pr_debug("fastboot: cmd_getvar %s\n", arg);
	if (strcmp(arg, "all")) {
		const char *value = fastboot_getvar(arg);
		fastboot_okay(value ? value : "");
	} else {
		char response[MAX_FASTBOOT_RESPONSE_SIZE];
		char varname[MAX_FASTBOOT_RESPONSE_SIZE];
		const char *varvalue;
		struct fastboot_var *var;
		int num_volumes, i;
		Volume* device_volumes;

		get_volume_table(&num_volumes, &device_volumes);
		/* Start from 1 as first volume reported is the tmp fs */
		for (i=1; i < num_volumes; i++) {
			if ((snprintf(varname, sizeof(varname), "partition-type:%s",
				(device_volumes[i].mount_point) + 1)) > MAX_FASTBOOT_RESPONSE_SIZE)
					goto error;
			varvalue = fastboot_getvar(varname);
			if (varvalue == NULL)
					goto error_var;
			if ((snprintf(response, sizeof(response), "%s: %s", varname, varvalue)) >
				MAX_FASTBOOT_RESPONSE_SIZE)
					goto error;
			fastboot_info(response);
			if ((snprintf(varname, sizeof(varname), "partition-size:%s",
				(device_volumes[i].mount_point) + 1)) > MAX_FASTBOOT_RESPONSE_SIZE)
					goto error;
			varvalue = fastboot_getvar(varname);
			if (varvalue == NULL)
					goto error_var;
			if ((snprintf(response, sizeof(response), "%s: %s", varname, varvalue)) >
				MAX_FASTBOOT_RESPONSE_SIZE)
					goto error;
			fastboot_info(response);
		}
		for (var = varlist; var; var = var->next) {
			if (var->fun == NULL) {
				snprintf(response, sizeof(response), "%s: %s", var->name, var->value);
				fastboot_info(response);
			}
		}
		fastboot_okay("");
	}
	return;
error:
	fastboot_fail("Unable to get partition info, partition name too long");
	return;
error_var:
	fastboot_fail("Unknown variable");
	return;
}

static void cmd_download(const char *arg, void *data, unsigned sz)
{
	char response[MAGIC_LENGTH];
	unsigned len;
	int r;

	len = strtoul(arg, NULL, 16);
	if (!g_disable_fboot_ui)
		ui_print("RECEIVE DATA...\n");
	pr_debug("fastboot: cmd_download %d bytes\n", len);

	download_size = 0;
	if (len > download_max) {
		if (tmp_fp >=0 )
			close(tmp_fp);
		ensure_path_mounted(FASTBOOT_DOWNLOAD_TMP_FILE);
		tmp_fp = open(FASTBOOT_DOWNLOAD_TMP_FILE, O_RDWR | O_CREAT | O_TRUNC, FILEMODE);
		if (tmp_fp < 0) {
			fastboot_fail("unable to create download file");
			return;
		}
	}

	sprintf(response, "DATA%08x", len);
	if (usb_write(response, strlen(response)) < 0)
		return;

	r = usb_read(download_base, len);
	if ((r < 0) || ((unsigned int)r != len)) {
		pr_error("fastboot: cmd_download error only got %d bytes\n", r);
		fastboot_state = STATE_ERROR;
		return;
	}
	if (tmp_fp >=0) {
		close(tmp_fp);
		tmp_fp = -1;
		strcpy(download_base, FASTBOOT_DOWNLOAD_TMP_FILE);
		len = strlen(FASTBOOT_DOWNLOAD_TMP_FILE);
	}
	download_size = len;
	fastboot_okay("");
}

int fastboot_in_process = 0;
static void fastboot_command_loop(void)
{
	struct fastboot_cmd *cmd;
	int r;
	if (!g_disable_fboot_ui)
		ui_print("FASTBOOT CMD WAITING...\n");
	pr_debug("fastboot: processing commands\n");

again:
	while (fastboot_state != STATE_ERROR) {
		memset(buffer, 0, MAGIC_LENGTH);
		r = usb_read(buffer, MAGIC_LENGTH);
		if (r < 0)
			break;
		buffer[r] = 0;
		pr_debug("fastboot got command: %s\n", buffer);

		for (cmd = cmdlist; cmd; cmd = cmd->next) {
			if (memcmp(buffer, cmd->prefix, cmd->prefix_len))
				continue;
			fastboot_state = STATE_COMMAND;
			fastboot_in_process = 1;
			if (!g_disable_fboot_ui) {
				ui_set_screen_state(1);
				ui_msg(TIPS, "CMD(%s)...", buffer);
			}
			cmd->handle((const char *)buffer + cmd->prefix_len,
				    (void *)download_base, download_size);
			fastboot_in_process = 0;
			if (fastboot_state == STATE_COMMAND)
				fastboot_fail("unknown reason");
			goto again;
		}
		pr_error("unknown command '%s'\n", buffer);
		fastboot_fail("unknown command");

	}
	fastboot_state = STATE_OFFLINE;
	if (!g_disable_fboot_ui)
		ui_print("FASTBOOT OFFLINE!\n");
	pr_warning("fastboot: oops!\n");
}

static int open_tcp(void)
{
	pr_verbose("Beginning TCP init\n");
	int tcp_fd = -1;
	int portno = 1234;
	struct sockaddr_in serv_addr;

	pr_verbose("Allocating socket\n");
	tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_fd < 0) {
		pr_error("Socket creation failed: %s\n", strerror(errno));
		return -1;
	}

	memset(&serv_addr, sizeof(serv_addr), 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(portno);
	pr_verbose("Binding socket\n");
	if (bind(tcp_fd, (struct sockaddr *) &serv_addr,
		 sizeof(serv_addr)) < 0) {
		pr_error("Bind failure: %s\n", strerror(errno));
		close(tcp_fd);
		return -1;
	}

	pr_verbose("Listening socket\n");
	if (listen(tcp_fd,5)) {
		pr_error("Listen failure: %s\n", strerror(errno));
		close(tcp_fd);
		return -1;
	}

	pr_info("Listening on TCP port %d\n", portno);
	return tcp_fd;
}

static int enable_ffs = 0;

static int open_usb_fd(void)
{
	usb.read_fp = open(USB_ADB_PATH, O_RDWR);
	/* tip to reuse same usb_read() and usb_write() than ffs */
	usb.write_fp = usb.read_fp;

	return usb.read_fp;
}

static int open_usb_ffs(void)
{
	ssize_t ret;
	int control_fp = -1;
	int ntries = 0;

	/* Retry to connect to avoid race condition with stop adbd */
	while ((control_fp = open(USB_FFS_ADB_EP0, O_RDWR)) == -EBUSY && ntries++ <= MAX_USB_FFS_ADB_EP0_TRIES)
		usleep(DELAY_USB_FFS_ADB_EP0_TRIES);
	if (control_fp < 0) {
		pr_info("[ %s: cannot open control endpoint: errno=%d]\n", USB_FFS_ADB_EP0, errno);
		goto err;
	}

	ret = write(control_fp, &descriptors, sizeof(descriptors));
	if (ret < 0) {
		pr_info("[ %s: write descriptors failed: errno=%d ]\n", USB_FFS_ADB_EP0, errno);
		goto err;
	}

	ret = write(control_fp, &strings, sizeof(strings));
	if (ret < 0) {
		pr_info("[ %s: writing strings failed: errno=%d]\n", USB_FFS_ADB_EP0, errno);
		goto err;
	}

	usb.read_fp = open(USB_FFS_ADB_OUT, O_RDWR);
	if (usb.read_fp < 0) {
		pr_info("[ %s: cannot open bulk-out ep: errno=%d ]\n", USB_FFS_ADB_OUT, errno);
		goto err;
	}

	usb.write_fp = open(USB_FFS_ADB_IN, O_RDWR);
	if (usb.write_fp < 0) {
		pr_info("[ %s: cannot open bulk-in ep: errno=%d ]\n", USB_FFS_ADB_IN, errno);
		goto err;
	}

	pr_info("Fastboot opened on %s\n", USB_FFS_ADB_PATH);

	close(control_fp);
	control_fp = -1;
	return usb.read_fp;

err:
	if (usb.write_fp > 0) {
		close(usb.write_fp);
		usb.write_fp = -1;
	}
	if (usb.read_fp > 0) {
		close(usb.read_fp);
		usb.read_fp = -1;
	}
	if (control_fp > 0) {
		close(control_fp);
		control_fp = -1;
	}

	return -1;
}

/**
 * Opens the file descriptor either using first /dev/android_adb if exists
 * otherwise the ffs one /dev/usb-ffs/adb/
 * */
static int open_usb(void)
{
	int ret = 0;
	static int printed = 0;

	enable_ffs = 0;
	/* first try /dev/android_adb */
	ret = open_usb_fd();

	if (ret < 1) {
		/* next /dev/usb-ffs/adb */
		enable_ffs = 1;
		ret = open_usb_ffs();
	}

	if (!printed) {
		if (ret < 1) {
			pr_info("Can't open ADB device node (%s),"
				" Listening on TCP only.\n",
				strerror(errno));
		} else {
			if (enable_ffs)
				pr_info("Listening on /dev/usb-ffs/adb/...\n");
			else
				pr_info("Listening on /dev/android_adb\n");
		}
		printed = 1;
	}

	return ret;
}

/**
 * Force to close file descriptor used at open_usb()
 * */
static void close_usb(void)
{
	if (usb.write_fp > 0) {
		close(usb.write_fp);
		usb.write_fp = -1;
	}
	if (usb.read_fp > 0) {
		if (enable_ffs)
			close(usb.read_fp);
		usb.read_fp = -1;
	}
}

static int fastboot_handler(void *arg)
{
	int usb_fd_idx = 0;
	int tcp_fd_idx = 1;
	int const nfds = 2;
	struct pollfd fds[nfds];

	memset(&fds, sizeof fds, 0);

	fds[usb_fd_idx].fd = -1;
	fds[tcp_fd_idx].fd = -1;

	for (;;) {
		if (fds[usb_fd_idx].fd == -1)
			fds[usb_fd_idx].fd = open_usb();
		if (fds[tcp_fd_idx].fd == -1)
			fds[tcp_fd_idx].fd = open_tcp();

		if (fds[usb_fd_idx].fd >= 0)
			fds[usb_fd_idx].events |= POLLIN;
		if (fds[tcp_fd_idx].fd >= 0)
			fds[tcp_fd_idx].events |= POLLIN;

		while (poll(fds, nfds, -1) == -1) {
			if (errno == EINTR)
				continue;
			pr_error("Poll failed: %s\n", strerror(errno));
			return -1;
		}

		if (fds[usb_fd_idx].revents & POLLIN) {
			fastboot_command_loop();
			close_usb();
			fds[usb_fd_idx].fd = -1;
		}

		if (fds[tcp_fd_idx].revents & POLLIN) {
			usb.read_fp = accept(fds[tcp_fd_idx].fd, NULL, NULL);
			usb.write_fp = usb.read_fp;
			if (usb.read_fp < 0)
				pr_error("Accept failure: %s\n", strerror(errno));
			else
				fastboot_command_loop();
			close_usb();
		}
	}
	return 0;
}

#define MAX_SCRATCH_SIZE 512*1024*1024

int fastboot_init(unsigned size)
{
	unsigned freememsize = getfreememsize()*1024;
	char* maxdownloadsize;

	pr_verbose("fastboot_init()\n");
	pr_info("Free Mem size : %d \n", freememsize);

	if (size == 0) {
		size = MIN(MAX_SCRATCH_SIZE, freememsize*2/3);
		pr_info("No sratch size specified in cmdline will use %u "
			"(minimum between 2/3 of free memory and MAX_SCRATCH_SIZE=%u",
			size, MAX_SCRATCH_SIZE);
	} else {
		if (size > freememsize) {
			pr_error("scratch size of %u bigger than free memory %u "
				" Unable to continue.\n\n", size, freememsize);
			fastboot_fail("Requested scratch buffer size is bigger than free memory");
			die();
		}
		if (size > (freememsize*2/3)) {
			pr_warning("scratch size is %u bigger than 2/3 of free memory %u "
				"\n", size, freememsize);
		}
	}
	download_max = size;
	download_base = malloc(size);
	if (download_base == NULL) {
		pr_error("scratch malloc of %u failed in fastboot."
			" Unable to continue.\n\n", size);
		fastboot_fail("Scratch buffer malloc failed");
		die();
	}

	fastboot_register("getvar:", cmd_getvar);
	fastboot_register("download:", cmd_download);
	fastboot_publish("version-bootloader", DROIDBOOT_VERSION, NULL);

	if (asprintf(&maxdownloadsize,"%d", size) == -1) {
		fastboot_fail("asprintf return error. Unable to continue.");
		die();
	}
	fastboot_publish("max-download-size", maxdownloadsize, NULL);

	/* We setup functional settings on USB to declare Fastboot Device */
	property_set("sys.usb.config", "adb");
	fastboot_handler(NULL);

	return 0;
}
