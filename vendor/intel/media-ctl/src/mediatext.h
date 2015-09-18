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

#ifndef __MEDIATEXT_H__
#define __MEDIATEXT_H__

struct media_device;

int mediatext_parse(struct media_device *device, char *string);

#endif /* __MEDIATEXT_H__ */
