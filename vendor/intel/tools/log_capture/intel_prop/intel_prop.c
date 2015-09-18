/* Copyright (C) Intel 2014
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <cutils/properties.h>

#ifdef ENABLE_DMI
#include <libdmi.h>
#endif

#define LOG_TAG "intel_props"
#include <utils/Log.h>

#define DEFAULT_CFG_FILE "/intel_prop.cfg"
#define MIN_LINE_LENGTH 3
#define FIELD_SEPARATOR ':'

#define EPOK    0
#define EPINVAL 1
#define EPNSUPP 2
#define EPNOTFOUND 3
#define EPFAIL 4

/**
 * Setup prop execution function
 * @param src - source of the property
 * @param dst - destination property
 * @return < 0 if it fails
 */
typedef int (*set_param)(const char *src, const char *dst);

struct type_setup {
	const char *signature;
	set_param fnc;
	const char *desc;
};

static int fs_to_prop(const char *src, const char *dst);

#ifdef ENABLE_DMI
static int dmi_to_prop(const char *src, const char *dst, enum smbios_type type);
static int dmi_b_to_prop(const char *src, const char *dst);
static int dmi_s_to_prop(const char *src, const char *dst);
#endif

/**
 * Type execution table
 */
struct type_setup type_tbl[] = {
		{ "fs", fs_to_prop,	"Copy the content of a fs node" },
#ifdef ENABLE_DMI
		{ "dmi_bi", dmi_b_to_prop, "DMI BIOS_INFORMATION parameter" },
		{ "dmi_is", dmi_s_to_prop, "DMI INTEL_SMBIOS parameter" },
#endif
		{ 0, }, /*null ended */
};

#ifdef ENABLE_DMI
static int dmi_to_prop(const char *src, const char *dst, enum smbios_type type) {
	int ret;
	char *dmi_value;
	dmi_value = libdmi_getfield(type, src);
	if (!dmi_value) {
		return -EPNOTFOUND;
	}

	ret = property_set(dst, dmi_value);

	free(dmi_value);
	if (ret)
		return -EPFAIL;
	return EPOK;
}

static int dmi_b_to_prop(const char *src, const char *dst) {
	return dmi_to_prop(src, dst, BIOS_INFORMATION);
}

static int dmi_s_to_prop(const char *src, const char *dst) {
	return dmi_to_prop(src, dst, INTEL_SMBIOS);
}
#endif

static int fs_to_prop(const char *src, const char *dst) {

	char prop_val[PROP_VALUE_MAX];
	FILE *f;
	size_t size;

	f = fopen(src, "r");

	if (!f)
		return -EPNOTFOUND;

	size = fread(prop_val, 1, PROP_VALUE_MAX - 1, f);

	if (size) {
		prop_val[size - 1] = 0;
		property_set(dst, prop_val);
	}

	fclose(f);
	return EPOK;
}

static int parse(const char *line) {
	char *dst, *src, *type, *tmp;
	int ret;
	struct type_setup *entry;

	type = strdup(line);
	if (!type) {
		return -EPINVAL;
	}
	src = strchr(type, FIELD_SEPARATOR);
	if (!src) {
		free(type);
		return -EPINVAL;
	}
	src[0] = 0;
	src++;

	dst = strchr(src, FIELD_SEPARATOR);
	if (!dst) {
		free(type);
		return -EPINVAL;
	}
	dst[0] = 0;
	dst++;
	/*remove \n */

	tmp = strchr(dst, '\n');
	if (tmp)
		tmp[0] = 0;

	if (!strlen(dst) || !strlen(src) || !strlen(type)) {
		free(type);
		return -EPINVAL;
	}

	ret = 0;
	entry = type_tbl;
	while (entry->signature) {
		if (strcmp(type, entry->signature) == 0) {
			ret = entry->fnc(src, dst);
			break;
		}
		entry++;
	}
	if (ret) {
		ALOGW("Cannot set %s from %s of type %s ", dst, src,
				type);
		free(type);
		return -EPNSUPP;
	}

	if (!entry->signature) {
		ALOGW("Unknown type %s for %s  ", type, dst);
		free(type);
		return -EPINVAL;
	}

	free(type);
	return EPOK;
}

static const char *cfg_file = NULL;

static void print_types() {
	struct type_setup *entry = type_tbl;
	printf("Supported parameter types:\n");
	while (entry->signature) {
		printf("\t%s\t-> %s\n", entry->signature, entry->desc);
		entry++;
	}
}

static void usage(const char *app) {
	char *base = basename(app);
	printf("Usage: %s [-l | -f <path> ]\n", base);
	printf("\t-l\t-list available types\n");
	printf("\t-f <path>\t-load config from <path>\n");
	free(base);
}

static int get_args(int argc, char **argv) {
	int option;

	while ((option = getopt(argc, argv, "lf:")) != -1)
		switch (option) {
		case 'l':
			print_types();
			return -1;
			break;
		case 'f':
			cfg_file = optarg;
			break;
		default:
			cfg_file = NULL;
		}

	if (argc >= 2 && (argc - optind)) {
		usage(argv[0]);
		return -1;
	}

	return 0;
}

int main(int argc, char **argv) {
	FILE *f;
	char *line = NULL;
	size_t len = 0;

	if (get_args(argc, argv))
		return -1;

	if (!cfg_file)
		cfg_file = DEFAULT_CFG_FILE;

	f = fopen(cfg_file, "r");
	if (!f)
		return -1;

	while (getline(&line, &len, f) > 0) {
		if (line[0] == '#' || strlen(line) < MIN_LINE_LENGTH)
			continue;
		parse(line);
	}

	free(line);
	fclose(f);
	return 0;
}
