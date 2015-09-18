/*
 * INTEL CONFIDENTIAL
 *
 * Copyright (C) 2010 - 2014 Intel Corporation.
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

#include "mk_firmware.h"

#include "assert_support.h"

#ifdef __ACC
#include "isp_const.h"
#include "isp_types.h"
#include "isp_defs_for_hive.h"
#endif /* __ACC */

#include "sh_css_defs.h"

#if !defined(TARGET_crun)
#if !DUMMY_BLOBS
#include STR(APPL.blob.h)
#endif
#include STR(APPL.map.h)
#endif

#ifdef __SP1
#define BINARY_ID -2
#else  /* SP */
#define BINARY_ID -1
#endif /* __SP1 */

#if defined(TARGET_crun)

#ifndef HIVE_ADDR_sp_start_isp_entry
#define HIVE_ADDR_sp_start_isp_entry	0x0
#endif

#ifdef __SP1
#ifndef HIVE_ADDR_sp1_start_entry
#define HIVE_ADDR_sp1_start_entry	0x0
#endif
#endif

#endif /* defined(TARGET_crun) */

#ifndef HIVE_ADDR_sp_init_dmem_data
#define HIVE_ADDR_sp_init_dmem_data	0x0
#endif

#ifndef HIVE_ADDR_ia_css_ispctrl_sp_isp_started
#define HIVE_ADDR_ia_css_ispctrl_sp_isp_started	0x0
#endif

#ifndef HIVE_ADDR_sp_sw_state
#define HIVE_ADDR_sp_sw_state	0x0
#endif

#ifndef HIVE_ADDR_sp_per_frame_data
#define HIVE_ADDR_sp_per_frame_data	0x0
#endif

#ifndef HIVE_ADDR_sp_group
#define HIVE_ADDR_sp_group	0x0
#endif

#ifndef HIVE_ADDR_sp_output
#define HIVE_ADDR_sp_output	0x0
#endif

#ifndef HIVE_ADDR_ia_css_bufq_host_sp_queue
#define HIVE_ADDR_ia_css_bufq_host_sp_queue	0x0
#endif

#ifndef HIVE_ADDR_host_sp_com
#define HIVE_ADDR_host_sp_com	0x0
#endif

#ifdef __SP1
#ifndef HIVE_ADDR_host_sp1_com
#define HIVE_ADDR_host_sp1_com	0x0
#endif
#endif

#ifndef HIVE_ADDR_host_sp_queues_initialized
#define HIVE_ADDR_host_sp_queues_initialized	0x0
#endif

#ifndef HIVE_ADDR_sp_sleep_mode
#define HIVE_ADDR_sp_sleep_mode	0x0
#endif

#ifndef HIVE_ADDR_sp_stop_copy_preview
#define HIVE_ADDR_sp_stop_copy_preview	0x0
#endif

#ifndef HIVE_ADDR_ia_css_dmaproxy_sp_invalidate_tlb
#define HIVE_ADDR_ia_css_dmaproxy_sp_invalidate_tlb	0x0
#endif

#ifndef HIVE_ADDR_debug_buffer_ddr_address
#define HIVE_ADDR_debug_buffer_ddr_address	0x0
#endif

#ifndef HIVE_ADDR_raw_copy_line_count
#define HIVE_ADDR_raw_copy_line_count 0x0
#endif

#ifndef HIVE_ADDR_ia_css_isys_sp_error_cnt
#define HIVE_ADDR_ia_css_isys_sp_error_cnt 0x0
#endif

#ifdef HAS_WATCHDOG_SP_THREAD_DEBUG

#ifndef HIVE_ADDR_sp_thread_wait
#define HIVE_ADDR_sp_thread_wait 0x0
#endif

#ifndef HIVE_ADDR_sp_pipe_stage
#define HIVE_ADDR_sp_pipe_stage 0x0
#endif

#ifndef HIVE_ADDR_sp_pipe_stripe
#define HIVE_ADDR_sp_pipe_stripe 0x0
#endif

#endif /* HAS_WATCHDOG_SP_THREAD_DEBUG */

#ifndef HIVE_ADDR_pipeline_sp_curr_binary_id
#define HIVE_ADDR_pipeline_sp_curr_binary_id 0x0
#endif

/* SR: Workaround for a tools fix that is available in the SDK for newer
 * systems (merrifield-plus, skycam-B0). The SP makefiles use -map-section to
 * rename the "text" section to "critical". After the tools fix, the linker
 * won't generate any _hrt_text_... defines in the transfer.h file because the
 * text section does not exist anymore after the rename. Instead, it will
 * generate _hrt_critical_... defines.
 */
#ifndef _hrt_text_source_of_sp
#define _hrt_text_source_of_sp  _hrt_critical_source_of_sp
#endif
#ifndef _hrt_text_size_of_sp
#define _hrt_text_size_of_sp    _hrt_critical_size_of_sp
#endif

#ifdef __SP1
#ifndef _hrt_text_source_of_sp1
#define _hrt_text_source_of_sp1 _hrt_critical_source_of_sp1
#endif
#ifndef _hrt_text_size_of_sp1
#define _hrt_text_size_of_sp1   _hrt_critical_size_of_sp1
#endif
#endif


/* A conditional initializer support macro */
#ifdef  HAS_WATCHDOG_SP_THREAD_DEBUG
#define OPTION_HAS_WATCHDOG_SP_THREAD_DEBUG(s1) s1,
#else
#define OPTION_HAS_WATCHDOG_SP_THREAD_DEBUG(s1)
#endif /* HAS_WATCHDOG_SP_THREAD_DEBUG */

#define INIT_SP_INFO() \
{ \
	.init_dmem_data		= HIVE_ADDR_sp_init_dmem_data, \
	.isp_started		= HIVE_ADDR_ia_css_ispctrl_sp_isp_started, \
	.sw_state			= HIVE_ADDR_sp_sw_state, \
	.host_sp_queues_initialized	= HIVE_ADDR_host_sp_queues_initialized, \
	.per_frame_data		= HIVE_ADDR_sp_per_frame_data, \
	.group			= HIVE_ADDR_sp_group, \
	.output			= HIVE_ADDR_sp_output, \
	.host_sp_queue		= HIVE_ADDR_ia_css_bufq_host_sp_queue, \
	.host_sp_com		= HIVE_ADDR_host_sp_com, \
	.sp_entry			= HIVE_ADDR_sp_start_isp_entry, \
	.sleep_mode			= HIVE_ADDR_sp_sleep_mode, \
	.stop_copy_preview		= HIVE_ADDR_sp_stop_copy_preview, \
	.invalidate_tlb		= HIVE_ADDR_ia_css_dmaproxy_sp_invalidate_tlb, \
	.debug_buffer_ddr_address	= HIVE_ADDR_debug_buffer_ddr_address, \
	.perf_counter_input_system_error = HIVE_ADDR_ia_css_isys_sp_error_cnt, \
	OPTION_HAS_WATCHDOG_SP_THREAD_DEBUG(.debug_wait = HIVE_ADDR_sp_thread_wait) \
	OPTION_HAS_WATCHDOG_SP_THREAD_DEBUG(.debug_stage = HIVE_ADDR_sp_pipe_stage) \
	OPTION_HAS_WATCHDOG_SP_THREAD_DEBUG(.debug_stripe = HIVE_ADDR_sp_pipe_stripe) \
	.curr_binary_id		= HIVE_ADDR_pipeline_sp_curr_binary_id, \
	.raw_copy_line_count	= HIVE_ADDR_raw_copy_line_count \
}
/* Init info for SP1 */
#ifdef __SP1
#define INIT_SP1_INFO() \
{ \
    .sw_state			= HIVE_ADDR_sp_sw_state, \
    .host_sp_com		= HIVE_ADDR_host_sp1_com, \
    .sp_entry			= HIVE_ADDR_sp1_start_entry, \
    .init_dmem_data             = HIVE_ADDR_sp_init_dmem_data, \
}

#endif

#ifdef __SP1
#undef  BINARY_HEADER
#define BINARY_HEADER(n, bin_id) \
{ \
    .type     = ia_css_sp1_firmware, \
    .info.sp1 = INIT_SP1_INFO(), \
    .blob     = BLOB_HEADER(n), \
}
#else /* SP */
#define BINARY_HEADER(n, bin_id) \
{ \
    .type    = ia_css_sp_firmware, \
    .info.sp = INIT_SP_INFO(), \
    .blob    = BLOB_HEADER(n), \
}
#endif /* __SP1 */

#if DUMMY_BLOBS
const char *blob = NULL;
#else
const char *blob = (const char *)&CAT(_hrt_blob_, APPL);
#endif

struct ia_css_fw_info firmware_header = BINARY_HEADER(APPL, BINARY_ID);

struct ia_css_memory_offsets *mem_offsets = NULL;
size_t mem_offsets_size = 0;

struct ia_css_config_memory_offsets *conf_mem_offsets = NULL;
size_t conf_mem_offsets_size = 0;

struct ia_css_state_memory_offsets *state_mem_offsets = NULL;
size_t state_mem_offsets_size = 0;

/*
 * For SP, re-align all program sections
 */
struct ia_css_blob_info fill_header(void)
{
    struct ia_css_blob_info	blob_original = firmware_header.blob;
    struct ia_css_blob_info	blob_aligned  = firmware_header.blob;
    /*
     * Re-format the blob layout to make sure that all section source addresses
     * are DDR word aligned. The #of padding bytes is computed separately to
     * make sure that the total size matches the sum of the section sizes
     *
     * Assume the following order in the blob file:
     *	- icache
     *	- text (pmem)
     *	- data
     *	- bss (no source address required)
     *
     * But it can easily be made recursive without knowledge of the section
     * order see "enum ia_css_blob_section_type" & "struct ia_css_blob_info_new"
     */
    if (blob_aligned.icache_size != 0) {
	uint32_t base = ceil_div(blob_aligned.icache_source,
				 HIVE_ISP_DDR_WORD_BYTES);
	uint32_t aligned_base = base * HIVE_ISP_DDR_WORD_BYTES;
	uint32_t delta = aligned_base - blob_aligned.icache_source;

	if (blob_aligned.text_size != 0) {
	    assert(blob_aligned.icache_source < blob_aligned.text_source);
	}
	if (blob_aligned.data_size != 0) {
	    assert(blob_aligned.icache_source < blob_aligned.data_source);
	}

	if (aligned_base != blob_aligned.icache_source) {
	    blob_aligned.icache_padding = delta;
	    blob_aligned.size += delta;
	    blob_aligned.padding_size += delta;
	    /* Move the sections */
	    blob_aligned.icache_source += delta;
	    if (blob_aligned.text_size != 0) {
		blob_aligned.text_source += delta;
	    }
	    if (blob_aligned.data_size != 0) {
		blob_aligned.data_source += delta;
	    }
	}
    }
    if (blob_aligned.text_size != 0) {
	uint32_t base = ceil_div(blob_aligned.text_source,
				 HIVE_ISP_DDR_WORD_BYTES);
	uint32_t aligned_base = base * HIVE_ISP_DDR_WORD_BYTES;
	uint32_t delta = aligned_base - blob_aligned.text_source;

	if (blob_aligned.data_size != 0) {
	    assert(blob_aligned.text_source < blob_aligned.data_source);
	}

	if (aligned_base != blob_aligned.text_source) {
	    blob_aligned.text_padding = delta;
	    blob_aligned.size += delta;
	    blob_aligned.padding_size += delta;
	    /* Move the sections */
	    blob_aligned.text_source += delta;
	    if (blob_aligned.data_size != 0) {
		blob_aligned.data_source += delta;
	    }
	}
    }
    if (blob_aligned.data_size != 0) {
	uint32_t base = ceil_div(blob_aligned.data_source,
				 HIVE_ISP_DDR_WORD_BYTES);
	uint32_t aligned_base = base * HIVE_ISP_DDR_WORD_BYTES;
	uint32_t delta = aligned_base - blob_aligned.data_source;
	if (aligned_base != blob_aligned.data_source) {
	    blob_aligned.data_padding = delta;
	    blob_aligned.size += delta;
	    blob_aligned.padding_size += delta;
	    /* Move the sections */
	    blob_aligned.data_source += delta;
	}
    }

    if (verbose) {
	fprintf(stdout, "blob.size:          %d -> %d\n",
		firmware_header.blob.size,
		blob_aligned.size);
	fprintf(stdout, "blob.padding_size:  %d -> %d\n",
		firmware_header.blob.padding_size,
		blob_aligned.padding_size);
	fprintf(stdout, "blob.icache_source: %d -> %d\n",
		firmware_header.blob.icache_source,
		blob_aligned.icache_source);
	fprintf(stdout, "blob.icache_size:   %d -> %d + %d\n",
		firmware_header.blob.icache_size,
		blob_aligned.icache_size,
		blob_aligned.icache_padding);
	fprintf(stdout, "blob.text_source:   %d -> %d\n",
		firmware_header.blob.text_source,
		blob_aligned.text_source);
	fprintf(stdout, "blob.text_size:     %d -> %d + %d\n",
		firmware_header.blob.text_size,
		blob_aligned.text_size,
		blob_aligned.text_padding);
	fprintf(stdout, "blob.data_source:   %d -> %d\n",
		firmware_header.blob.data_source,
		blob_aligned.data_source);
	fprintf(stdout, "blob.data_size:     %d -> %d + %d\n",
		firmware_header.blob.data_size,
		blob_aligned.data_size,
		blob_aligned.data_padding);
    }
    firmware_header.blob = blob_aligned;

return blob_original;
}
