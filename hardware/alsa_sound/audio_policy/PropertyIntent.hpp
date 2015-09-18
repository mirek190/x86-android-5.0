/*
 * INTEL CONFIDENTIAL
 *
 * Copyright (c) 2013 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors.
 *
 * Title to the Material remains with Intel Corporation or its suppliers and
 * licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The
 * Material is protected by worldwide copyright and trade secret laws and treaty
 * provisions. No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or disclosed
 * in any way without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */

#pragma once

#include <binder/Parcel.h>
#include <utils/String8.h>
#include <string>

class PropertyIntent
{
public:
    PropertyIntent(const std::string &propertyName);

    /**
     * Broadcast an intent
     *
     * @param[out] error human readable error.
     *
     * @return true if success, false if error, in this case, error string is set accordingly.
     */
    bool broadcast(std::string &error);
private:
    /**
     * Format a Parcel with the requested action.
     *
     * @param[in] data Parcel to write into.
     * @param[in] action associated action to write in the parcel data.
     */
    void writeParcel(android::Parcel &data, const android::String16 &action);

    std::string _name; /**< Intent Name. */

    static const char *const _rootName; /**< formalized root name of the property intent. */

    /**
     * Broadcast intent transaction code.
     */
    static const int _broadcastIntentTransaction;
    /**
     * Transaction code offset for broadcast, must match the offset of IActivityManager.java.
     */
    static const int _broadcastIntentTransactionOffset = 13;

    static const char *const _activityServiceName; /**< activity service name. */

    static const char *const _activityManagerName; /**< activity manager name. */
};
