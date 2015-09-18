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

#include "PropertyIntent.hpp"
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sstream>
#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include "cutils/properties.h"

using std::stringstream;
using std::string;
using android::IBinder;
using android::String16;
using android::sp;
using android::IServiceManager;
using android::defaultServiceManager;
using android::Parcel;

const char *const PropertyIntent::_rootName = "com.intel.property_changed.";

const char *const PropertyIntent::_activityServiceName = "activity";

const char *const PropertyIntent::_activityManagerName = "android.app.IActivityManager";

const int PropertyIntent::_broadcastIntentTransaction =
    IBinder::FIRST_CALL_TRANSACTION + PropertyIntent::_broadcastIntentTransactionOffset;

PropertyIntent::PropertyIntent(const string &propertyName)
    : _name(propertyName)
{
}

bool PropertyIntent::broadcast(string &error)
{
    String16 intent(_rootName);
    intent += String16(_name.c_str());

    sp<IServiceManager> sm = defaultServiceManager();
    if (sm == NULL) {

        error = "could not find service manager";
        return false;
    }
    // Retrieve an existing service called "activity" from service manager (non blocking)
    sp<IBinder> am = sm->checkService(String16(_activityServiceName));
    if (am == NULL) {

        error = "getService could not find activity service";
        return false;
    }

    char buf[PROPERTY_VALUE_MAX];
    int len = property_get("sys.boot_completed", buf, "0");
    if (!len || strncmp(buf, "1", sizeof(buf))) {

        error = "should not send vtsv.routed before boot_completed";
        return false;
    }

    if (property_set("AudioComms.vtsv.routed", "true") < 0) {

        error = "could not set property";
        return false;
    }

    Parcel data;
    Parcel reply;

    writeParcel(data, intent);

    if (am->transact(_broadcastIntentTransaction, data, &reply) != 0) {

        return false;
    }
    int exceptionCode = reply.readExceptionCode();
    if (exceptionCode) {

        stringstream errorStream;
        errorStream << "send broadcast for property " << _name <<
            " message caught exception " << exceptionCode;
        error = errorStream.str();
        return false;
    }
    return true;
}

void PropertyIntent::writeParcel(Parcel &data, const String16 &action)
{
    /**
     * Sending an intent from native layer requires to format manually the data to send
     * through parcel (serialization purpose)
     */
    data.writeInterfaceToken(String16(_activityManagerName));

    data.writeStrongBinder(NULL);
    data.writeString16(action);

    data.writeInt32(0); /**< Type Id (null). */
    data.writeInt32(-1); /**< resultat data:: no string provided. */
    data.writeInt32(0); /**< resultdata. */
    data.writeInt32(-1); /**< bundle map. */
    data.writeInt32(-1); /**< Permission. */
    data.writeInt32(0); /**< app op. */
    data.writeInt32(0); /**< serialized: no. */
    data.writeInt32(0); /**< sticky: no. */
    data.writeInt32(0); /**< userId. */
    data.writeInt32(-1); /**< Extra. */
    data.writeInt32(-1); /**< no bundle specified. */

    data.writeStrongBinder(NULL); /**< No intent receiver specified. */
    data.writeInt32(0); /**< result code. */
    data.writeInt32(-1); /**< resultat data: -1 : no string provided, convention for none. */
    data.writeInt32(-1); /**< bundle: -1 : no bundle provided, convention for none. */
    data.writeInt32(-1); /**< Permission: -1, no permission provided, convention for none. */
    data.writeInt32(0); /**< app op. */
    data.writeInt32(0); /**< serialized: no. */
    data.writeInt32(0); /**< sticky: no. */
}
