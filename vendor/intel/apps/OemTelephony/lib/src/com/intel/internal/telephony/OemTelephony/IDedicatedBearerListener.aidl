/*
 * Copyright (C) 2012 Intel Corporation, All rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.intel.internal.telephony.OemTelephony;

import com.intel.internal.telephony.OemTelephony.OemQosInfo;
import com.intel.internal.telephony.OemTelephony.OemTftInfo;

/**
 * Callback methods for dedicated bearer events
 */
oneway interface IDedicatedBearerListener {
    /**
     * Callback called when the qos for dedicated bearer is received
     * @param qosInfo
     */
    void onQosChanged(in OemQosInfo qosInfo);

    /**
     * Callback called when tft for dedicated bearer is received
     * @param tftInfo
     */
    void onTftChanged(in OemTftInfo tftInfo);

     /**
     * Callback called when a new dedicated bearer is closed
     * @param iface
     * @param cid
     */
    void onDedicatedBearerClosed(in String iface, in int cid);

     /**
     * Callback called when a new dedicated bearer is opened
     * @param iface
     * @param cid
     */
    void onDedicatedBearerOpen(in String iface, in int cid);
}
