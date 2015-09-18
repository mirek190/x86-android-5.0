/*
 * Copyright (c) 2013-2014, Intel Corporation
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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>

#include "uefivar.h"

#define LOG_TAG "uefivar"
#include <utils/Log.h>

static bool verbose = 0;

static void usage(char *name)
{
	static const char *help =
		"\nUsage: %s [OPTIONS]\n"
		" Without -s option, variable's value is printed. Guid, name and value type are always required.\n"
		" -h, --help         Print this help\n"
		" -v, --verbose      Print debug informations\n"
		" -g, --guid GUID    GUID of the variable. Format must be the same as in /sys/firmware/efi/vars\n"
		" -n, --varname      Name of the variable. Must not exceed 511 characters length.\n"
		" -s, --set VALUE    Set value to the specified variable\n"
		" -k, --keep-size    Only with --set. Do not change current variable size.\n"
		"                    New value must fit in the current size.\n"
		" -t, --value-type   Specify the type of value to set or get\n"
		"                    If not -k, the variable size will be changed to the type's size\n"
		"                    Available types are:\n"
		"                      - \"int\" : sizeof(int)\n"
		"                      - \"int16\" : sizeof(int16)\n"
		"                      - \"string\" : sizeof(char) * strlen(string)\n"
		" Note: if the variable does not exist %s will create it as a non-volatile variable.\n"
		" Examples:\n"
		"  %s -g 8be4df61-93ca-11d2-aa0d-00e098032b8c -n BootOrder -t int\n"
		"  %s -g 8be4df61-93ca-11d2-aa0d-00e098032b8c -n BootOrder -t int -s 0x2 -k\n" ;
	char *cmdname = basename(name);

	error(help, cmdname, cmdname, cmdname, cmdname);
	exit(EXIT_FAILURE);
}

static int get_args(int argc, char **argv, struct arguments *args)
{
	int option;
	int ret = 0;
	char *value = NULL;
	const char *optstring = "g:hn:s:kt:v";
	struct option longopts[] = {
		{ "guid", required_argument, NULL, 'g' },
		{ "help", no_argument, NULL, 'h' },
		{ "varname", required_argument, NULL, 'n' },
		{ "set", required_argument, NULL, 's' },
		{ "keep-size", no_argument, NULL, 'k' },
		{ "value-type", required_argument, NULL, 't' },
		{ "verbose", no_argument, NULL, 'v' },
		{ 0, 0, 0, 0 }
	};

	memset(args, 0, sizeof(*args));

	while ((option = getopt_long(argc, argv, optstring, longopts, NULL)) != -1)
		switch (option) {
		case 'g':
			args->guid_str = optarg;
			if (parse_guid(optarg, &args->guid))
				usage(argv[0]);
			break;
		case 'n':
			if (strlen(optarg) > EFI_VARNAME_LEN)
				usage(argv[0]);
			args->varname = optarg;
			break;
		case 'k':
			args->keep_size = true;
			break;
		case 's':
			value = optarg;
			break;
		case 't':
			args->value_type = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage(argv[0]);
		}
	if (!args->guid_str || !args->varname || !args->value_type) {
		error("Mandatory parameters are: Guid, Varname and Value-type\n");
		usage(argv[0]);
	}

	if (value) {
		if (!args->value_type) {
			error("value-type option is required with --set\n");
			usage(argv[0]);
		}
		if (!strcmp(args->value_type, "int") || !strcmp(args->value_type, "int16")) {
			char *endptr;
			unsigned long int l = strtoul(value, &endptr, 0);
			if ((value[0] == '\0' || *endptr != '\0') || l == ULONG_MAX) {
				error("Failed to read integer argument %s: %s\n", optarg, strerror(errno));
				ret = -EINVAL;
				goto out;
			}


			if ((!strcmp(args->value_type, "int16") && l > USHRT_MAX)
			    || l > UINT_MAX) {
				error("Specified value is too large for type \"%s\"\n", args->value_type);
				ret = -EINVAL;
				goto out;
			}

			unsigned int i = (int)l;
			size_t size = !strcmp(args->value_type, "int16") ? sizeof(int16_t) : sizeof(i);

			args->value = malloc(size);
			if (!args->value) {
				error("Failed to allocate argument: %s\n", strerror(errno));
				ret = -ENOMEM;
				goto out;
			}

			memcpy(args->value, &i, size);
			if (!args->keep_size)
				args->value_size = size;
			else {
				/* Determine minimum size to fit the value
				 * This will be compared to variable's size later
				 */
				args->value_size = 0;
				int tmp = i;
				do {
					tmp = tmp >> 8;
					args->value_size++;
				} while (tmp);
			}
			goto out;
		} else if (!strcmp(args->value_type, "string")) {
			args->value = strdup(value);
			if (!args->value) {
				error("Failed to allocate string value variable, %s\n", strerror(errno));
				ret = -ENOMEM;
				goto out;
			}
			args->value_size = strlen(args->value) + 1;
			goto out;
		}
		error("value-type %s is not supported\n", args->value_type);
		usage(argv[0]);
	}
out:
	return ret;
}

static void free_args(struct arguments *args)
{
	if (args->value)
		free(args->value);
}

int main(int argc, char **argv)
{
	int ret = EXIT_FAILURE;
	struct efi_var *buffer = NULL;
	struct arguments args;

	ret = get_args(argc, argv, &args);
	if (ret)
		goto out;

	if ((ret = uefivar_process_entry(args, &buffer)) == 0) {
		if (!args.value)
			uefi_get_value((char*)buffer, args.value_type);

		uefivar_free_buffer(&buffer);
	}
out:
	free_args(&args);
	return ret;
}
