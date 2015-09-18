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

#include "libc.h"

static inline /* Pending fish spill bug, inline should be removed */
unsigned int _divu_icache(unsigned int num, unsigned int den)
{
  unsigned int bit = 1;
  unsigned int res = 0;
  int c;

  while (den < num && bit != 0 && (int) den >= 0) {
    den <<= 1;
    bit <<= 1;
  }

  while(bit) {
    c = num >= den;
    num = c ? num-den : num;
    res = c ? res|bit : res;
    bit >>= 1;
    den >>= 1;
  }

  return res;
#pragma hivecc section="icache"
}


