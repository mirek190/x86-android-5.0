/*
 * Copyright (C) 2014 Intel Corporation, All Rights Reserved
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

public class OemHooksConstants {

    /**
     * These enumerations should be in sync with what is used in
     * RIL adaptation.
     *
     * These enums details the additional requests (OEM) to pass to the RIL
     * via the RIL_REQUEST_OEM_HOOK_RAW API
     * The command id allocation is the following:
     * 0x00000001 -> 0x0000009F : Product Specific
     * 0x000000A0 -> 0x000000CF : Platform Requests
     * 0x000000D0 -> 0x000000FF : Platform Unsolicited
     */

    /** OEM hook to get the value of sensor id */
    public static final int RIL_OEM_HOOK_STRING_THERMAL_GET_SENSOR = 0x000000A2;
    /** OEM hook to set the min and max threhold of the sensor id */
    public static final int RIL_OEM_HOOK_STRING_ACTIVATE_THERMAL_SENSOR_NOTIFICATION = 0x000000A3;
    /** OEM HOOK to set the Autonomous Fast Dormancy Mode */
    public static final int RIL_OEM_HOOK_STRING_SET_MODEM_AUTO_FAST_DORMANCY = 0x000000A4;
    /** OEM hook to Get the Answer to reset of the current SIM card */
    public static final int RIL_OEM_HOOK_STRING_GET_ATR = 0x000000A5;
    /** OEM hook to get GPRS cell environment */
    public static final int RIL_OEM_HOOK_STRING_GET_GPRS_CELL_ENV = 0x000000A6;
    /** OEM hook to dump the cell environment */
    public static final int RIL_OEM_HOOK_STRING_DEBUG_SCREEN_COMMAND = 0x000000A7;
    /** OEM hook to release all the calls */
    public static final int RIL_OEM_HOOK_STRING_RELEASE_ALL_CALLS = 0x000000A8;
    /** OEM hook to get the SMS Transport Mode */
    public static final int RIL_OEM_HOOK_STRING_GET_SMS_TRANSPORT_MODE = 0x000000A9;
    /** OEM hook to set the SMS Transport Mode */
    public static final int RIL_OEM_HOOK_STRING_SET_SMS_TRANSPORT_MODE = 0x000000AA;
    /** OEM hook to set the RF Power table */
    public static final int RIL_OEM_HOOK_STRING_GET_RF_POWER_CUTBACK_TABLE = 0x000000AB;
    /** OEM hook to set the RF Power table */
    public static final int RIL_OEM_HOOK_STRING_SET_RF_POWER_CUTBACK_TABLE = 0x000000AC;
    /** OEM hook to register or unregister on IMS network */
    public static final int RIL_OEM_HOOK_STRING_IMS_REGISTRATION = 0x000000AD;
    /** OEM hook to set a new IMS apn */
    public static final int RIL_OEM_HOOK_STRING_IMS_CONFIG = 0x000000AE;
    /** OEM hook specific to set the default APN and type. Valid only in LTE modem. */
    public static final int RIL_OEM_HOOK_STRING_SET_DEFAULT_APN = 0x00000AF;
    /** OEM hook to inform that modem can be released */
    public static final int RIL_OEM_HOOK_STRING_NOTIFY_RELEASE_MODEM = 0x000000B0;
    /** OEM hook specific to DSDS for swapping protocol stacks configs */
    public static final int RIL_OEM_HOOK_STRING_SWAP_PS = 0x000000B2;
    /** OEM hook used to send direct AT commands to modem */
    public static final int RIL_OEM_HOOK_STRING_SEND_AT = 0x000000B3;
    /** OEM hook specific to indicate call available with IMS */
    public static final int RIL_OEM_HOOK_STRING_IMS_CALL_STATUS = 0x000000B4;
    /** OEM hook specific to indicate sms available with IMS */
    public static final int RIL_OEM_HOOK_STRING_IMS_SMS_STATUS = 0x000000B5;
    /** OEM hook specific to get PC-CSCF information with IMS */
    public static final int RIL_OEM_HOOK_STRING_IMS_GET_PCSCF = 0x000000B6;
    /** OEM hook specific to IMS to pass SRVCC call infos */
    public static final int RIL_OEM_HOOK_STRING_IMS_SRVCC_PARAM = 0x000000B7;
    /** OEM hook specific to DSDS to reset the SIM */
    public static final int RIL_OEM_HOOK_STRING_SIM_RESET = 0x000000B8;
    /** OEM hook specific to DSDS to get the DVP Config */
    public static final int RIL_OEM_HOOK_STRING_GET_DVP_STATE = 0x000000B9;
    /** OEM hook specific to DSDS to set the DVP Config */
    public static final int RIL_OEM_HOOK_STRING_SET_DVP_ENABLED = 0x000000BA;
    /** OEM hook to get the version of thermal sensor APIs to be used */
    public static final int RIL_OEM_HOOK_STRING_GET_OEM_VERSION = 0x000000BB;
    /** OEM hook to get the value of sensor id using V2 API*/
    public static final int RIL_OEM_HOOK_STRING_THERMAL_GET_SENSOR_V2 = 0x000000BC;
    /** OEM hook to set the tripPointNumber and hysteresis using the V2 API */
    public static final int RIL_OEM_HOOK_STRING_ACTIVATE_THERMAL_SENSOR_NOTIFICATION_V2
            = 0x000000BD;
    /** OEM hook to activate or deactivate the registration status and band information */
    public static final int RIL_OEM_HOOK_STRING_SET_REG_STATUS_AND_BAND_IND = 0x000000BE;
     /** OEM hook to have an automatic selection of CSG cell */
    public static final int RIL_OEM_HOOK_STRING_CSG_SET_AUTOMATIC_SELECTION = 0x000000BF;
    /** OEM hook to get current CSG state */
    public static final int RIL_OEM_HOOK_STRING_CSG_GET_CURRENT_CSG_STATE = 0x000000C0;
    /** OEM hook to get current CNAP state */
    public static final int RIL_OEM_HOOK_STRING_CNAP_GET_CURRENT_STATE = 0x000000C1;
    /** OEM hook to activate or deactivate the reception of frequency info from modem */
    public static final int RIL_OEM_HOOK_STRING_ADPCLK_ACTIVATE = 0x000000C2;
    /** OEM hook to get the current frequency info from the modem */
    public static final int RIL_OEM_HOOK_STRING_ADPCLK_GET_FREQ_INFO = 0x000000C3;
    /** OEM hook to activate or deactivate the registration state and band information reporting */
    public static final int RIL_OEM_HOOK_SET_REGISTRATION_STATUS_AND_BAND_REPORT = 0x000000C4;
    /** OEM hook to activate or deactivate the COEX unsolicited report */
    public static final int RIL_OEM_HOOK_STRING_SET_COEX_REPORT = 0x000000C5;
    /** OEM hook to set the wlan parameters to the modem IDC part */
    public static final int RIL_OEM_HOOK_STRING_SET_COEX_WLAN_PARAMS = 0x000000C6;
    /** OEM hook to set the BT parameters to the modem IDC part */
    public static final int RIL_OEM_HOOK_STRING_SET_COEX_BT_PARAMS = 0x000000C7;
    /** OEM hook to get the thermal alarm indication */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND = 0x000000D0;
    /** OEM hook specific to DSDS for catching out of service URC */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_FAST_OOS_IND = 0x000000D1;
    /** OEM hook specific to DSDS for catching in service URC */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_IN_SERVICE_IND = 0x000000D2;
    /** OEM hook specific to indicate data suspended/resume status */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_DATA_STATUS_IND = 0x000000D3;
    /** OEM hook specific to indicate MT class */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_MT_CLASS_IND = 0x000000D4;
    /** OEM hook specific to report events to crashtool */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_CRASHTOOL_EVENT_IND = 0x000000D5;
    /** OEM hook specific to indicate IMS registration status */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_IMS_REG_STATUS = 0x000000D6;
    /** OEM hook specific to indicate network IMS support status */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_IMS_SUPPORT_STATUS = 0x000000D7;
    /** OEM hook to indicate device diagnostic metrics and IDC CWS info, for RF Coexistence */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_COEX_INFO = 0x000000D8;
    /** OEM hook specific to indicate network APN information */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_NETWORK_APN_IND = 0x000000D9;
    /** OEM hook specific to indicate SIM appliation error */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_SIM_APP_ERR_IND = 0x000000DA;
    /** OEM hook specific to indicate call disconnect */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_CALL_DISCONNECTED = 0x000000DB;
    /** OEM hook specific to indicate new bearer TFT information */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_BEARER_TFT_PARAMS = 0x000000DC;
    /** OEM hook specific to indicate new bearer QOS information */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_BEARER_QOS_PARAMS = 0x000000DD;
    /** OEM hook specific to indicate bearer deactivation */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_BEARER_DEACT = 0x000000DE;
    /** OEM hook specific to ciphering indication */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_CIPHERING_IND = 0x000000DF;
    /** OEM hook specific to indicate SRVCC procedure started or completed*/
    public static final int RIL_OEM_HOOK_RAW_UNSOL_SRVCCH_STATUS = 0x000000E0;
    /** OEM hook specific to indicate SRVCC synchronization needed */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_SRVCC_HO_STATUS = 0x000000E1;
    /** OEM hook specific to indicate bearer activation */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_BEARER_ACT = 0x000000E2;
    /** OEM hook to get the thermal alarm indication with V2 apis */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND_V2 = 0x000000E3;
    /** OEM hook to get the registration status and band information indication */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND = 0x000000E4;
    /** OEM hook specific to CSG indication */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_CSG_IND = 0x000000E5;
    /** OEM hook specific to indicate frequency info change */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_ADPCLK_FREQ_INFO_NOTIF = 0x000000E6;
    /** OEM hook to indicate the registration state and band information reporting */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_REPORT = 0x000000E7;
    /** OEM hook to indicate the COEX unsolicited report */
    public static final int RIL_OEM_HOOK_RAW_UNSOL_COEX_REPORT = 0x000000E8;
}
