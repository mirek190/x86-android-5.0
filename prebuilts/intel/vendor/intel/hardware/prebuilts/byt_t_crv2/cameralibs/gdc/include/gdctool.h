/*
** Copyright 2013 Intel Corporation
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef __GDCTOOL_H__
#define __GDCTOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MORPH_TABLE_NUM_PLANES  6

struct morph_table {
    unsigned int height;
    unsigned int width; /* number of valid elements per line */
    unsigned short *coordinates_x[MORPH_TABLE_NUM_PLANES];
    unsigned short *coordinates_y[MORPH_TABLE_NUM_PLANES];
};

   void validateGdcValue(int *value);
   struct morph_table *allocateGdcTable(unsigned int width, unsigned int height);
   void freeGdcTable(struct morph_table *table);
   struct morph_table *getGdcTable(int image_width, int image_height);

#ifdef __cplusplus
}
#endif

#endif // __GDCTOOL_H__
