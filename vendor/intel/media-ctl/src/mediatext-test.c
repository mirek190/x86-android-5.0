/*
 * libmediatext test program
 *
 * Copyright (C) 2013 Intel Corporation
 *
 * Contact: Sakari Ailus <sakari.ailus@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mediactl.h"
#include "mediatext.h"

int main(int argc, char *argv[])
{
	struct media_device *device;
	char buf[1024];
	unsigned int i;
	int rval;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <media device> <string>\n\n", argv[0]);
		fprintf(stderr, "\tstring := [ v4l2-ctrl | v4l2-mbus | link-reset | link-conf]\n\n");
		fprintf(stderr, "\tv4l2-ctrl := \"entity\" ctrl_type ctrl_id ctrl_value\n");
		fprintf(stderr, "\tctrl_type := [ int | int64 | bitmask ]\n");
		fprintf(stderr, "\tctrl_value := [ %%d | %%PRId64 | bitmask_value ]\n");
		fprintf(stderr, "\tbitmask_value := b<binary_number>\n\n");
		fprintf(stderr, "\tv4l2-mbus := \n");
		fprintf(stderr, "\tlink-conf := \"entity\":pad -> \"entity\":pad[link-flags]\n");
		return EXIT_FAILURE;
	}

	device = media_device_new(argv[1]);
	if (!device)
		return EXIT_FAILURE;

	media_debug_set_handler(device, (void (*)(void *, ...))fprintf, stdout);

	rval = media_device_enumerate(device);
	if (rval)
		return EXIT_FAILURE;

	rval = mediatext_parse(device, argv[2]);
	if (rval) {
		fprintf(stderr, "bad string %s (%s)\n", argv[2], strerror(-rval));
		return EXIT_FAILURE;
	}

	media_device_unref(device);

	return EXIT_SUCCESS;
}
