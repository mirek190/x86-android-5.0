/*
 * Media controller text-based configuration library
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

#include <sys/ioctl.h>

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <linux/types.h>

#include "mediactl.h"
#include "mediactl-priv.h"
#include "tools.h"
#include "v4l2subdev.h"

struct parser {
	char *prefix;
	int (*parse)(struct media_device *media, const struct parser *p,
		     char *string);
	struct parser *next;
	bool no_args;
};

static int parse(struct media_device *media, const struct parser *p, char *string)
{
	int rval;

	for (; p->prefix; p++) {
		size_t len = strlen(p->prefix);

		if (strncmp(p->prefix, string, len))
			continue;

		string += len;

		for (; isspace(*string); string++);

		if (p->no_args)
			return p->parse(media, p->next, NULL);

		if (strlen(string) == 0)
			return -ENOEXEC;

		return p->parse(media, p->next, string);
	}

	return -ENOENT;
}

struct ctrl_type {
	uint32_t type;
	char *str;
} ctrltypes[] = {
	{ V4L2_CTRL_TYPE_INTEGER, "int" },
	{ V4L2_CTRL_TYPE_MENU, "menu" },
	{ V4L2_CTRL_TYPE_INTEGER_MENU, "intmenu" },
	{ V4L2_CTRL_TYPE_BITMASK, "bitmask" },
	{ V4L2_CTRL_TYPE_INTEGER64, "int64" },
};

/* adapted from yavta.c */
static int parse_v4l2_ctrl(struct media_device *media, const struct parser *p,
			   char *string)
{
	struct v4l2_ext_control ctrl = { 0 };
	struct v4l2_ext_controls ctrls = { .count = 1,
					   .controls = &ctrl };
	int64_t val;
	int rval;
	char *end = string + strlen(string);
	struct media_entity *entity;
	struct ctrl_type *ctype;
	unsigned int i;

	entity = media_parse_entity(media, string, &string);
	if (!entity)
		return -ENOENT;

	for (i = 0; i < ARRAY_SIZE(ctrltypes); i++)
		if (!strncmp(string, ctrltypes[i].str,
			     strlen(ctrltypes[i].str)))
			break;

	if (i == ARRAY_SIZE(ctrltypes))
		return -ENOENT;

	ctype = &ctrltypes[i];

	string += strlen(ctrltypes[i].str);
	for (; isspace(*string); string++);
	rval = sscanf(string, "0x%" PRIx32, &ctrl.id);
	if (rval <= 0)
		return -EINVAL;

	for (; !isspace(*string) && *string; string++);
	for (; isspace(*string); string++);

	ctrls.ctrl_class = V4L2_CTRL_ID2CLASS(ctrl.id);

	switch (ctype->type) {
	case V4L2_CTRL_TYPE_BITMASK:
		if (*string++ != 'b')
			return -EINVAL;
		while (*string == '1' || *string == '0') {
			val <<= 1;
			if (*string == '1')
				val++;
			string++;
		}
		break;
	default:
		rval = sscanf(string, "%" PRId64, &val);
		break;
	}
	if (rval <= 0)
		return -EINVAL;

	media_dbg(media, "Setting control 0x%8.8x (type %s), value %" PRId64 "\n",
		  ctrl.id, ctype->str, val);

	if (ctype->type == V4L2_CTRL_TYPE_INTEGER64)
		ctrl.value64 = val;
	else
		ctrl.value = val;

	rval = v4l2_subdev_open(entity);
	if (rval < 0)
		return rval;

	rval = ioctl(entity->fd, VIDIOC_S_EXT_CTRLS, &ctrls);
	if (ctype->type != V4L2_CTRL_TYPE_INTEGER64) {
		if (rval != -1) {
			ctrl.value64 = ctrl.value;
		} else if (ctype->type != V4L2_CTRL_TYPE_STRING &&
			   (errno == EINVAL || errno == ENOTTY)) {
			struct v4l2_control old = { .id = ctrl.id,
						    .value = val };

			rval = ioctl(entity->fd, VIDIOC_S_CTRL, &old);
			if (rval != -1) {
				ctrl.value64 = old.value;
			}
		}
	}
	if (rval == -1) {
		media_dbg(media,
			  "Failed setting control 0x8.8x: %s (%d) to value %"
			  PRId64 "\n", ctrl.id, strerror(errno), errno, val);;
		return -errno;
	}

	if (val != ctrl.value64)
		media_dbg(media, "Asking for %" PRId64 ", got %" PRId64 "\n",
			  val, ctrl.value64);

	return 0;
}

static int parse_v4l2_mbus(struct media_device *media, const struct parser *p,
			   char *string)
{
	media_dbg(media, "Media bus format setup: %s\n", string);
	return v4l2_subdev_parse_setup_formats(media, string);
}

static int parse_link_reset(struct media_device *media, const struct parser *p,
			    char *string)
{
	media_dbg(media, "Resetting links\n");
	return media_reset_links(media);
}

static int parse_link_conf(struct media_device *media, const struct parser *p,
			   char *string)
{
	media_dbg(media, "Configuring links: %s\n", string);
	return media_parse_setup_links(media, string);
}

static const struct parser parsers[] = {
	{ "v4l2-ctrl", parse_v4l2_ctrl },
	{ "v4l2-mbus", parse_v4l2_mbus },
	{ "link-reset", parse_link_reset, NULL, true },
	{ "link-conf", parse_link_conf },
	{ 0 }
};

int mediatext_parse(struct media_device *media, char *string)
{
	return parse(media, parsers, string);
}
