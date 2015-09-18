/*****************************************************************
 * INTEL CONFIDENTIAL
 * Copyright 2011 Intel Corporation All Rights Reserved.
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or its suppliers or licensors. Title to the Material remains with
 * Intel Corporation or its suppliers and licensors. The Material contains
 * trade secrets and proprietary and confidential information of Intel or
 * its suppliers and licensors. The Material is protected by worldwide
 * copyright and trade secret laws and treaty provisions. No part of the
 * Material may be used, copied, reproduced, modified, published, uploaded,
 * posted, transmitted, distributed, or disclosed in any way without Intel’s
 * prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */

#ifndef _ATCMD_H_
#define _ATCMD_H_

typedef enum {
    ATE = 0,
    ATS0,
    /**
     *Audio related
     */
    ATA,
    ATD,
    ATH,
    AT_CHLD,
    AT_CHUP,
    AT_CVHU,
    /**
     *others
     */
    AT_CBC,
    AT_CLAC,
    AT_CFUN,
    ATS7,
    AT_AT,
    AT_AND,

    AT_MAX = 100
} atcmd_table;

const  char *const atcmd_name[] = {
    "ATE",
    "ATS0",
    "ATA",
    "ATD",
    "ATH",
    "AT+CHLD",
    "AT+CHUP",
    "AT+CVHU",
    "AT+CBC",
    "AT+CLAC",
    "AT+CFUN",
    /* FIXME "ATS7=60M1+ES=3,0,2;+DS=3;+DS44=3;+IFC=2,2;BX4", */
    "ATS7",
    "AT@",
    "AT&",

};

enum {
    CONFIG_GMI,
    CONFIG_GMM,
    CONFIG_GMR,
    CONFIG_GSN,
    CONFIG_MAX
};

typedef enum {
    RESULT_CODE_ERROR = 0,
    RESULT_CODE_NO_CARRIER,
    RESULT_CODE_BUSY,
    RESULT_CODE_NO_ANSWER,
    RESULT_CODE_OK,
    RESULT_CODE_RING,
    RESULT_CODE_CONNECT,
} atcmd_response_table;

const  char *const atcmd_response_name[] = {
    "ERROR",
    "NO CARRIER",
    "BUSY",
    "NO ANSWER",
    "OK",
    "RING",
    "CONNECT",
};
#endif

