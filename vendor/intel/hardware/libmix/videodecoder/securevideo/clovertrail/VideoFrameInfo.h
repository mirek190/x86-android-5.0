/* INTEL CONFIDENTIAL
* Copyright (c) 2014 Intel Corporation.  All rights reserved.
*
* The source code contained or described herein and all documents
* related to the source code ("Material") are owned by Intel
* Corporation or its suppliers or licensors.  Title to the
* Material remains with Intel Corporation or its suppliers and
* licensors.  The Material contains trade secrets and proprietary
* and confidential information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright and
* trade secret laws and treaty provisions.  No part of the Material
* may be used, copied, reproduced, modified, published, uploaded,
* posted, transmitted, distributed, or disclosed in any way without
* Intel's prior express written permission.
*
* No license under any patent, copyright, trade secret or other
* intellectual property right is granted to or conferred upon you
* by disclosure or delivery of the Materials, either expressly, by
* implication, inducement, estoppel or otherwise. Any license
* under such intellectual property rights must be express and
* approved by Intel in writing.
*
*/


#ifndef VIDEO_FRAME_INFO_H_
#define VIDEO_FRAME_INFO_H_

#define MAX_NUM_NALUS 16

typedef struct {
    uint8_t  type;       // nalu type + nal_ref_idc
                         // If type is SPS/PPS/SEI, then 'data' points to clear SPS/PPS/SEI nalu data 
                         //   and imr_offset should be ignored
                         // If type is slice/IDR, then imr_offset points to the data in IMR,
                         //   and 'data' should be ignored
    uint32_t imr_offset; // offset in IMR to the beginning of the data
    uint8_t* data;       // if current NALU is SPS/PPS/SEI, data is the pointer to clear SPS/PPS/SEI data
    uint32_t length;     // nalu length
} nalu_info_t;

typedef struct {
    uint32_t num_nalus;  // number of NALU
    nalu_info_t nalus[MAX_NUM_NALUS];
} frame_info_t;

#endif
