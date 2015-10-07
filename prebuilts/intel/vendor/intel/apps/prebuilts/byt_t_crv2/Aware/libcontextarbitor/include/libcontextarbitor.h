/*
  INTEL CONFIDENTIAL

  Copyright 2013 Intel Corporation All Rights Reserved.

  This source code and all documentation related to the source code
  ("Material") contains trade secrets and proprietary and confidential
  information of Intel and its suppliers and licensors. The Material is
  deemed highly confidential, and is protected by worldwide copyright and
  trade secret laws and treaty provisions. No part of the Material may be
  used, copied, reproduced, modified, published, uploaded, posted,
  transmitted, distributed, or disclosed in any way without Intel's prior
  express written permission.

  No license under any patent, copyright, trade secret or other
  intellectual property right is granted to or conferred upon you by
  disclosure or delivery of the Materials, either expressly, by
  implication, inducement, estoppel or otherwise. Any license under such
  intellectual property rights must be express and approved by Intel in
  writing.

  Author: Sun, Taiyi (taiyi.sun@intel.com)
*/

#ifndef LIBCONTEXTARBITOR_H
#define LIBCONTEXTARBITOR_H

#include <stddef.h>

typedef struct
{
    int prop;
    int size;
    void *value;
} ctx_option_item_t;

typedef struct
{
    int len;
    ctx_option_item_t items[0];
} ctx_option_t;


#ifdef __cplusplus
extern "C" {
#endif

/*
 * function:		opens session of a context sensor.
 * sensor_type:		psh sensor type
 * returns:		session handle; NULL for failure.
 */
void * ctx_open_session(const char * sensor_type);

/*
 * function:		sets the options to an opened session. merges with options of
 *			other sessions and returns the merged options wihch should be set in psh firmware.
 * handle:		session handle
 * option_property:	property type which represents the property set of a sensor.
 *			its value should contain the complete setting of the sensor.
 * in_option:		the input option string in json format.
 * out_option:		out parameter. To be set as the pointer of a ctx_option_t structure.
 *			it contains the merged options.
 * returns:		0 for success; -1 for failure.
 */
int ctx_set_option(void *handle, int option_property, const char *in_option, ctx_option_t **out_option);

/*
 * function:		post process the data to report
 * handle:		session handle
 * in_data:		pointer of the input data which contains raw data.
 * in_size:		size of input buffer in bytes.
 * out_data:		pointer of the pointer of the output data buffer. it should be filled. may be the set to in_data
 * out_size:		pointer of the buffer to be filled with the size of out_data in bytes.
 * returns:		0 for success but no publish; 1 for success and publish; -1 for failure.
 */
int ctx_dispatch_data(void *handle, void *in_data, int in_size, void **out_data, int *out_size);

/*
 * function:		close the session
 * handle:		session handle
 * out_option:		out parameter. To be set as the pointer of a ctx_option_t structure.
 *			it contains the merged options.
 * returns:		0 for success and noting to do; 1 success and reset the property; -1 for failure
 */
int ctx_close_session(void *handle, ctx_option_t **out_option);

/*
 * function:		free the allocated memory in ctx_option_t
 * option:		option previously returned by ctx_set_option and ctx_close_option
 */
int ctx_option_release(ctx_option_t *option);

/*
 * function:	release the initial-allocated memory. only for valgrind check.
 */
void ctx_uninit();

#ifdef __cplusplus
};
#endif

#endif
