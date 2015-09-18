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

/**
 * Oem Telephony Intents & Keys.
 */
public class OemTelephonyIntents {

    /**
     * Broadcast Action: Modem sensor has reached the set threshold temperature.
     * The intent will have the following extra values:</p>
     * <ul>
     *   <li><em>sensor Id</em> -  - An int with one of the following values:
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_RF},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_BASEBAND_CHIP},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_PCB}
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_PMU}
     *          or {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_PA}</li>
     *   <li><em>temperature</em> - Integer containing the temperature formatted as a
     *              4 digits number: 2300 = 23.00 celcius degrees (two digits after dot).</li>
     * </ul>
     *
     * <p class="note">
     * Requires the READ_PHONE_STATE permission.
     *
     * <p class="note">This is a protected intent that can only be sent
     * by the system.
     */
    public static final String ACTION_MODEM_SENSOR_THRESHOLD_REACHED =
            "intel.intent.action.MODEM_SENSOR_THRESHOLD_REACHED";

    /**
     * Broadcast Action: Modem sensor has reached the set threshold temperature.
     * The intent will have the following extra values:</p>
     * <ul>
     *   <li><em>sensor Id</em> -  - An int with one of the following values:
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_RF},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_BB},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_PCB}
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_PMU}
     *          or {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_PA}</li>
     *   <li><em>temperature</em> - Integer containing the temperature in MilliDegC.</li>
     * </ul>
     *
     * <p class="note">
     * Requires the READ_PHONE_STATE permission.
     *
     * <p class="note">This is a protected intent that can only be sent
     * by the system.
     */
    public static final String ACTION_MODEM_SENSOR_THRESHOLD_REACHED_V2 =
            "intel.intent.action.MODEM_SENSOR_THRESHOLD_REACHED_V2";

    public static final String MODEM_SENSOR_ID_KEY = "sensorId";
    public static final String MODEM_SENSOR_TEMPERATURE_KEY = "temperature";

    public static final String IMS_STATUS_KEY = "IMSState";
    public static final String ACTION_IMS_REGISTRATION_STATE_CHANGED =
            "intel.intent.action.IMS_REGISTRATION_STATE_CHANGED";

    public static final String ACTION_IMS_NW_SUPPORT_STATE_CHANGED =
            "intel.intent.action.IMS_NW_SUPPORT_STATE_CHANGED";

    public static final String COEX_INFO_KEY = "CoexInfo";
    public static final String ACTION_COEX_INFO =
            "intel.intent.action.ACTION_COEX_INFO";

    public static final String ACTION_EMERGENCY_CALL_STATUS_CHANGED =
            "intel.intent.action.EMERGENCY_CALL_STATUS";
    public static final String EMERGENCY_CALL_STATUS_KEY = "emergencyCallOngoing";

    /**
     * Broadcast Action: Registration status and band information.
     * The intent will have the following extra values:</p>
     * <ul>
     *   <li><em>reg status</em> - An int with one of the following values:
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#REG_STATUS_NOT_REGISTERED},
     *          or {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#REG_STATUS_REGISTERED}</li>
     *   <li><em>band</em> - String containing band information of the serving cell.</li>
     * </ul>
     *
     * <p class="note">
     * Requires the READ_PHONE_STATE permission.
     *
     * <p class="note">This is a protected intent that can only be sent
     * by the system.
     */
    public static final String ACTION_REG_STATUS_AND_BAND_IND =
            "intel.intent.action.ACTION_REG_STATUS_AND_BAND_IND";

    /** Ciphering Indication Intents. */
    public static final String ACTION_CIPHERING_STATE_CHANGED =
            "intel.intent.action.CIPHERING_STATE_CHANGED";
    /** The key for ciphering status (ON/OFF). */
    public static final String CIPHERING_STATUS_KEY = "CipheringStatus";

    /**
     * Adaptive Clocking (ADPCLK) Intents.
     * The intent will have the following extra values:
     *          a AdaptiveClockingFrequencyInfo (Parcelable) object
     */
    public static final String ACTION_MODEM_ADPCLK_FREQUENCY_CHANGED =
            "intel.intent.action.APDCLK_FREQUENCY_CHANGED";
    /** The key for ADPCLK frequency information. */
    public static final String MODEM_ADPCLK_FREQUENCY_CHANGED_KEY = "FrequencyChanged";

    /** OemTelephony availability. */
    public static final String OEM_TELEPHONY_READY =
            "intel.intent.action.OEM_TELEPHONY_READY";

    /** CSG Indication Intents. */
    public static final String ACTION_CSG_STATE_CHANGED =
            "intel.intent.action.CSG_STATE_CHANGED";
    /** The keys for csg status. */
    /** CsgSelCause - 0/1/2 */
    public static final String CSG_CAUSE_KEY    = "CsgSelCause";
    /** CsgCellId - numeric */
    public static final String CSG_CELLID_KEY   = "CsgCellId";
    /** CsgRecNum - numeric 0 / >0 */
    public static final String CSG_REC_NUM_KEY  = "CsgRecNum";
    /** CsgHnbNum - numeric 0 / >0 */
    public static final String CSG_HNB_NUM_KEY  = "CsgHnbNum";
    /** CsgHnbName - string */
    public static final String CSG_HNB_NAME_KEY = "CsgHnbName";
    /** CsgOperId - mcc+mnc */
    public static final String CSG_OPER_KEY     = "CsgOperId";
    /** CsgAcT - radio access technology. 0:GSM/2:UMTS/7:LTE */
    public static final String CSG_ACT_KEY      = "CsgAcT";
    /** CsgIdList - 0/1/2 */
    public static final String CSG_ID_LIST_KEY  = "CsgIdList";

    /**
     * This intent is sent when modem registration status and/or band
     * are changing.
     */
    public static final String ACTION_IDC_REG_STATUS_AND_BAND_IND =
            "intel.intent.action.ACTION_IDC_REG_STATUS_AND_BAND_IND";

    /** (long) subscription Id of the emitter of this intent */
    public static final String SUBID_KEY = "subId";
    /** (int) registration status: 0 => Not registered : 1 => Registered */
    public static final String REG_STATUS_KEY = "regStatus";
    /** (String) registered band (Eg: "BAND_LTE_7") */
    public static final String BAND_KEY = "band";

    /**
     * This intent is sent when modem URC dedicated to LTE Cell info reporting
     * is received through +XNRTCWS URC registered using setCoexReporting().
     */
    public static final String ACTION_IDC_LTE_FRAME_IND =
            "intel.intent.action.ACTION_IDC_LTE_FRAME_IND";

    /** LteCoexInfo bundle key */
    public static final String LTE_FRAME_INFO_KEY = "LteCoexInfo";

    /** (long) subscription Id of the emitter of this intent. */
    public static final String LTE_FRAME_INFO_SUBID_KEY = "subId";
    /** (int) current packed LTE cell frame bitmap. */
    public static final String LTE_FRAME_INFO_BITMAP_KEY = "LteBitmap";
    /** (int) current LTE cell SubFrame config no. */
    public static final String LTE_FRAME_INFO_TDDSPECIALSUBFRAMENO_KEY =
            "TDDSpecialSubFrameNumber";

    /**
    * This intent is sent when modem URC dedicated to Wlan Coex handling
    * is received through +XNRTCWS URC registered using setCoexReporting().
    * This requires also that WLAN activation be declared via setWlanParams().
    */
    public static final String ACTION_IDC_WLAN_COEX_IND =
            "intel.intent.action.ACTION_IDC_WLAN_COEX_IND";

    /** WlanCoexInfo bundle key. */
    public static final String WLAN_COEX_INFO_KEY = "WlanCoexInfo";

    /** (long) subscription Id of the emitter of this intent. */
    public static final String WLAN_COEX_INFO_SUBID_KEY = "subId";
    /** (int) wlan lower safe Rx frequency in MHz. */
    public static final String WLAN_COEX_INFO_SAFERXMIN_KEY = "WlanSafeRxMin";
    /** (int) wlan upper safe Rx frequency in MHz. */
    public static final String WLAN_COEX_INFO_SAFERXMAX_KEY = "WlanSafeRxMax";
    /** (int) wlan lower safe Tx frequency in MHz. */
    public static final String WLAN_COEX_INFO_SAFETXMIN_KEY = "WlanSafeTxMin";
    /** (int) wlan upper safe Tx frequency in MHz. */
    public static final String WLAN_COEX_INFO_SAFETXMAX_KEY = "WlanSafeTxMax";
    /** (int) number of Wlan Safe Tx Power table values. */
    public static final String WLAN_COEX_INFO_SAFETXPOWERNUM_KEY = "WlanSafeTxPowerNum";
    /** (int[]) wlan Safe Tx Power Table. */
    public static final String WLAN_COEX_INFO_SAFETXPOWERTABLE_KEY =
            "WlanSafeTxPowerTable";

    /**
     * This intent is sent when modem URC dedicated to Bluetooth Coex handling
     * is received through +XNRTCWS URC registered using setCoexReporting().
     * This requires also that BT activation be declared via setBtParams().
     */
    public static final String ACTION_IDC_BT_COEX_IND =
            "intel.intent.action.ACTION_IDC_BT_COEX_IND";

    /** BtCoexInfo bundle key. */
    public static final String BT_COEX_INFO_KEY = "BtCoexInfo";

    /** (long) subscription Id of the emitter of this intent. */
    public static final String BT_COEX_INFO_SUBID_KEY = "subId";
    /** (int) bt lower safe Rx frequency in MHz. */
    public static final String BT_COEX_INFO_SAFERXMIN_KEY = "BtSafeRxMin";
    /** (int) bt upper safe Rx frequency in MHz. */
    public static final String BT_COEX_INFO_SAFERXMAX_KEY = "BtSafeRxMax";
    /** (int) bt lower safe Tx frequency in MHz. */
    public static final String BT_COEX_INFO_SAFETXMIN_KEY = "BtSafeTxMin";
    /** (int) bt upper safe Tx frequency in MHz. */
    public static final String BT_COEX_INFO_SAFETXMAX_KEY = "BtSafeTxMax";
    /** (int) number of Bt Safe Tx Power table values. */
    public static final String BT_COEX_INFO_SAFETXPOWERNUM_KEY = "BtSafeTxPowerNum";
    /** (int[]) bt Safe Tx Power Table. */
    public static final String BT_COEX_INFO_SAFETXPOWERTABLE_KEY = "BtSafeTxPowerTable";
}
