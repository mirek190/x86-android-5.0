/*
 * Support for Intel Camera Imaging ISP subsystem.
 *
 * Copyright (c) 2010 - 2014 Intel Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <stdio.h>
#include "ia_css_isp_param.mk_fw.h"

void
ia_css_isp_param_print_mem_initializers(const struct ia_css_binary_info *info)
{
	unsigned c, m;
	for (c = 0; c < IA_CSS_NUM_PARAM_CLASSES; c++) {
		for (m = 0; m < IA_CSS_NUM_MEMORIES; m++) {
			fprintf(stdout, "mem[%d][%d] = {a=%d,  s=%d}\n",
				c, m,
				info->mem_initializers.params[c][m].address,
				info->mem_initializers.params[c][m].size);
		}
	}
}
