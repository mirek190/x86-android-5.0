/*
 * INTEL CONFIDENTIAL
 *
 * Copyright (C) 2010 - 2013 Intel Corporation.
 * All Rights Reserved.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or licensors. Title to the Material remains with Intel
 * Corporation or its licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No License under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or
 * delivery of the Materials, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights
 * must be express and approved by Intel in writing.
 */

#ifndef _MK_FIRMWARE_H_
#define _MK_FIRMWARE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#if defined(TARGET_crun)
#define C_RUN
#endif

#define MK_FIRMWARE

#define _STR(x) #x
#define STR(x) _STR(x)

#define _CAT(x, y) x##y
#define CAT(x, y) _CAT(x, y)

#define NO_HOIST

#ifdef CELL
#include STR(CAT(CELL, _params.h))
#endif

#define __register

#include "system_types.h"
#include <hrt/api.h>

#include "ia_css_binary.h"
#include "sh_css_firmware.h"
#include "sh_css_defs.h"
#include "ia_css_acc_types.h"


#if defined(TARGET_crun) || defined(TARGET_unsched)
#define DUMMY_BLOBS 1
#else
#define DUMMY_BLOBS 0
#endif


/* Headers for all binaries */
#if DUMMY_BLOBS

#define BLOB_HEADER(n) \
{ \
    .size		= 0, \
    .padding_size	= 0, \
    .icache_source	= 0, \
    .icache_size	= 0, \
    .icache_padding	= 0, \
    .text_source	= 0, \
    .text_size		= 0, \
    .text_padding	= 0, \
    .data_source	= 0, \
    .data_target	= 0, \
    .data_size		= 0, \
    .data_padding	= 0, \
    .bss_target		= 0, \
    .bss_size		= 0, \
}

#else

#define BLOB_HEADER(n) \
{ \
    .size		= CAT(_hrt_size_of_, n), \
    .padding_size	= 0, \
    .icache_source	= CAT(_hrt_icache_source_of_, n), \
    .icache_size	= CAT(_hrt_icache_size_of_, n), \
    .icache_padding	= 0, \
    .text_source	= CAT(_hrt_text_source_of_, n), \
    .text_size		= CAT(_hrt_text_size_of_, n), \
    .text_padding	= 0, \
    .data_source	= CAT(_hrt_data_source_of_, n), \
    .data_target	= CAT(_hrt_data_target_of_, n), \
    .data_size		= CAT(_hrt_data_size_of_, n), \
    .data_padding	= 0, \
    .bss_target		= CAT(_hrt_bss_target_of_, n), \
    .bss_size		= CAT(_hrt_bss_size_of_, n), \
}

#endif /* DUMMY_BLOBS */

#ifndef INPUT_NUM_CHUNKS
#define INPUT_NUM_CHUNKS OUTPUT_NUM_CHUNKS
#endif

extern bool verbose;
extern const char *blob;
extern struct ia_css_fw_info firmware_header;
extern struct ia_css_memory_offsets *mem_offsets;
extern size_t mem_offsets_size;
extern struct ia_css_config_memory_offsets *conf_mem_offsets;
extern size_t conf_mem_offsets_size;
extern struct ia_css_state_memory_offsets *state_mem_offsets;
extern size_t state_mem_offsets_size;

extern struct ia_css_blob_info fill_header(void);

#endif  /* _MK_FIRMWARE_H_ */

