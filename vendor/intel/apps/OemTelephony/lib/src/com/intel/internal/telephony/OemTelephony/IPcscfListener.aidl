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

import com.intel.internal.telephony.OemTelephony.PcscfInfo;

/**
 * Callback methods for PCSCF response from the network
 *
 * {@hide}
 */
oneway interface IPcscfListener {
    /**
     * Callback called when the modem has received PCSCF info.
     * @param pcscf
     */
    void onPcscfResponse(in PcscfInfo pcscf);
}
