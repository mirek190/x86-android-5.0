/*-------------------------------------------------------------------------
 * INTEL CONFIDENTIAL
 *
 * Copyright 2012 Intel Corporation All Rights Reserved.
 *
 * This source code and all documentation related to the source code
 * ("Material") contains trade secrets and proprietary and confidential
 * information of Intel and its suppliers and licensors. The Material is
 * deemed highly confidential, and is protected by worldwide copyright and
 * trade secret laws and treaty provisions. No part of the Material may be
 * used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise. Any license under such
 * intellectual property rights must be express and approved by Intel in
 * writing.
 *-------------------------------------------------------------------------
 */

/*
 * Author: Han Ke
 * Group: PSI, MCG
 */

#ifndef __GESTURE_H__
#define __GESTURE_H__

#include <iostream>
#include <stdio.h>
#include <set>
#include <string>
#include <string.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;

extern "C" {
/**
 * stop gesture recognition algorithm
 *   CAUTION: this function is not multi-thread safe
 */
void gesture_close();

/**
 * initial gesture recognition algorithm
 *   input:
 *     if (argc == 6) {
 *       hmm_model_folder = argv[0];
 *       templ_match_all_model = argv[1];
 *       templ_match_spec_model = argv[2];
 *       config_params = argv[3];
 *       mov_detect_forward = argv[4];
 *       mov_detect_backward = argv[5];
 *     }
 *     else
 *       use default models
 *   output:
 *     return true if success, false if failed
 *   CAUTION: this function is not multi-thread safe
 */
bool gesture_initial(int argc, char* argv[]);

/**
 * process sensor data, and report recognized gesture
 *   input:
 *     short* data      length = 6, includes 3 axis accel and gyro data
 *     bool segmented    if input segmented movement data, set it to true
 *                          otherwise set it to false
 *     bool last        if segmented = true, set to true for last data
 *                          otherwise set it to false
 *   output:
 *     if gesture is recognized, return "<gesture name> ", otherwise return NULL
 *   CAUTION: this function is not multi-thread safe
 */
char * gesture_process_single_data(short *data, bool segmented, bool last);
}
#endif
