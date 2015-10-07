/*******************************************
 * INTEL CONFIDENTIAL
 *
 * Cellular Modem Core Dump Reader
 *
 * Copyright 2011 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material contains trade secrets and proprietary
 * and confidential information of Intel or its suppliers and licensors. The
 * Material is protected by worldwide copyright and trade secret laws and treaty
 * provisions. No part of the Material may be used, copied, reproduced, modified
 * published, uploaded, posted, transmitted, distributed, or disclosed in any
 * way without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */

#ifndef __DUMPREADER_H__
#define __DUMPREADER_H__

#include <linux/limits.h>
#include "tcs_mmgr.h"
#include <cutils/properties.h>

#define MCDR_STATES \
    /* normal cases */ \
    X(SUCCEED), \
    X(INIT), \
    X(IN_PROGRESS), \
    X(READ_IN_PROGRESS), \
    X(READ_SUCCEED), \
    X(ARCHIVE_IN_PROGRESS), \
    X(ARCHIVE_SUCCEED), \
    X(CLEANING_UP), \
    /* error cases */ \
    X(INIT_ERROR), \
    X(START_PROT_ERR), \
    X(IO_ERROR), \
    X(FS_ERROR), \
    X(PROT_ERROR)

/* Status of the Core dump reader */
typedef enum mcdr_status {
#undef X
#define X(a) MCDR_ ## a
    MCDR_STATES
} mcdr_status_t;

typedef struct mcdr_data {
    /* INPUT: */
    char mdm_version[PROPERTY_VALUE_MAX];
    mcdr_info_t mcdr_info;
    /* OUTPUT: */
    mcdr_status_t state;
    char coredump_file[PATH_MAX];
} mcdr_data_t;

/**
 * This function is the main library function. it process the get core dump
 * operation.
 *
 * @param [in,out] data
 * @return cd_status_t, CD_DONE if successful
 */
void mcdr_get_core_dump(mcdr_data_t *data);

/**
 * This function can be called at any time to stop and cleanup MCDR
 */
void mcdr_cleanup(void);

/**
 * This function provides MCDR status
 */
mcdr_status_t mcdr_get_state(void);

char *mcdr_get_reason(void);

#endif //__DUMPREADER_H__
