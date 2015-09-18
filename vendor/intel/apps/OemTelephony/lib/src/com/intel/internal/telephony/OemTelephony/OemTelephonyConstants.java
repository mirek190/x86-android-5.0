/*
 * Copyright (C) 2012 Intel Corporation, All Rights Reserved
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

public interface OemTelephonyConstants {

    /** SMS Transport Mode - See 3GPP 27.007 */
    public static final int SMS_TRANSPORT_MODE_INVALID = -1;
    public static final int SMS_TRANSPORT_MODE_PACKET_DOMAIN = 0;
    public static final int SMS_TRANSPORT_MODE_CIRCUIT_SWITCHED = 1;
    public static final int SMS_TRANSPORT_MODE_PACKET_DOMAIN_PREFERRED = 2;
    public static final int SMS_TRANSPORT_MODE_CIRCUIT_SWITCHED_PREFERRED = 3;

    /** TX Power Offset Table Index */
    public static final int OFFSET_TABLE_INDEX_0 = 0;
    public static final int OFFSET_TABLE_INDEX_1 = 1;
    public static final int OFFSET_TABLE_INDEX_2 = 2;

    public static final int MODEM_SENSOR_ID_RF              = 0;
    public static final int MODEM_SENSOR_ID_BASEBAND_CHIP   = 1;
    public static final int MODEM_SENSOR_ID_PCB             = 3;
    public static final int MODEM_SENSOR_ID_PMU             = 4;
    public static final int MODEM_SENSOR_ID_PA              = 5;

    public static final String MODEM_SENSOR_RF = "TRF";
    public static final String MODEM_SENSOR_BB = "TBBIC";
    public static final String MODEM_SENSOR_PCB = "TPCB";
    public static final String MODEM_SENSOR_PMU = "TPMU";
    public static final String MODEM_SENSOR_PA = "TPA";

    public static final int REG_STATUS_NOT_REGISTERED = 0;
    public static final int REG_STATUS_REGISTERED = 1;

    /** IMS network status */
    public static final int IMS_NW_STATUS_NOT_SUPPORTED = 0;
    public static final int IMS_NW_STATUS_SUPPORTED = 1;
    /** IMS registration state request */
    public static final int IMS_STATE_REQUEST_UNREGISTER = 0;
    public static final int IMS_STATE_REQUEST_REGISTER = 1;
    /** IMS registration status */
    public static final int IMS_STATUS_UNREGISTERED = 0;
    public static final int IMS_STATUS_REGISTERED = 1;
    /** IMS call status */
    public static final int IMS_CALL_NOT_AVAILABLE = 0;
    public static final int IMS_CALL_AVAILABLE = 1;
    /** IMS sms status */
    public static final int IMS_SMS_NOT_AVAILABLE = 0;
    public static final int IMS_SMS_AVAILABLE = 1;

    /** Return codes */
    public static final int UNSUPPORTED = -1;
    public static final int FAIL = 0;
    public static final int SUCCESS = 1;

    /** RRIL command states */
    public static final int DEACTIVATE = 0;
    public static final int ACTIVATE = 1;

    /** Coexistence WLAN constants as used by RRIL */
    public static final int WLAN_BANDWIDTH_UNSUPPORTED = -1;
    public static final int WLAN_BANDWIDTH_20_MHZ = 0;
    public static final int WLAN_BANDWIDTH_40_MHZ = 1;
    public static final int WLAN_BANDWIDTH_80_MHZ = 2;
}
