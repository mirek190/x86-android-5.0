/*
 * Copyright (c) 2014, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
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

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <libgen.h>
#include <inc/libdmi.h>
#include <cutils/properties.h>

#define LOG_TAG "intel_firmware_props"
#include <utils/Log.h>

static bool verbose = 1;

#define debug(...)				\
	if (verbose) {				\
		ALOGD(__VA_ARGS__);		\
		printf(__VA_ARGS__);		\
	}
#define info(...) {				\
		ALOGI(__VA_ARGS__);		\
		printf(__VA_ARGS__);		\
	}
#define warn(...) {				\
		ALOGW(__VA_ARGS__);		\
		fprintf(stderr, __VA_ARGS__);	\
	}
#define error(...) {				\
		ALOGE(__VA_ARGS__);		\
		fprintf(stderr, __VA_ARGS__);	\
	}


static void usage(char *name)
{
	static const char *help =
		"\nUsage: %s [OPTIONS]\n"
		" -h, --help         Print this help\n"
		" -v, --verbose      Print debug informations\n";

	error(help, basename(name), basename(name));
	exit(EXIT_FAILURE);
}

static void get_args(int argc, char **argv)
{
	int option;
	int ret = 0;
	const char *optstring = "hv";
	struct option longopts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "verbose", no_argument, NULL, 'v' },
		{ 0, 0, 0, 0 }
	};

	while ((option = getopt_long(argc, argv, optstring, longopts, NULL)) != -1)
		switch (option) {
		case 'v':
			verbose = 1;
			break;
		default:
			usage(argv[0]);
		}
}

struct dmi_props {
	char *property;
	char *field_name;
	enum smbios_type type;
} intel_fw_props[] = {
	{"sys.ia32.version", "BiosVersion", INTEL_SMBIOS},
	{"sys.ifwi.version", "bios_version", BIOS_INFORMATION},
	{"sys.chaabi.version", "SECVersion", INTEL_SMBIOS},
	{"sys.pmic.version", "PmicVersion", INTEL_SMBIOS},
	{"sys.pmc.version", "PMCVersion", INTEL_SMBIOS},
	{"sys.punit.version", "PUnitVersion", INTEL_SMBIOS},
	{"sys.gop.version", "GopVersion", INTEL_SMBIOS},
};

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	unsigned i;
	char *dmi_value;

	get_args(argc, argv);

	for (i = 0 ; i < sizeof(intel_fw_props) / sizeof(*intel_fw_props) ; i++) {
		debug("Setting %s with value of %s\n", intel_fw_props[i].property, intel_fw_props[i].field_name);
		dmi_value = libdmi_getfield(intel_fw_props[i].type, intel_fw_props[i].field_name);
		if (!dmi_value) {
			error("Failed to set %s : could not get dmi data\n", intel_fw_props[i].property);
			continue;
		}

		ret = property_set(intel_fw_props[i].property, dmi_value);
		if (ret)
			error("Failed to set %s\n", intel_fw_props[i].property);
		free(dmi_value);
	}
out:
	exit(ret);
}
