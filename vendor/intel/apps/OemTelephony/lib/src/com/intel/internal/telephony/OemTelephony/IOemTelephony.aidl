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

import android.os.Message;

import com.intel.internal.telephony.OemTelephony.IDedicatedBearerListener;
import com.intel.internal.telephony.OemTelephony.ISrvccListener;
import com.intel.internal.telephony.OemTelephony.IPcscfListener;
import com.intel.internal.telephony.OemTelephony.OemSrvccSyncParam;
import com.intel.internal.telephony.OemTelephony.AdaptiveClockingFrequencyInfo;

/**
 * Interface used to interact with the OEM Hook.
 */
interface IOemTelephony {


    /**
     * On Success returns the Answer to Reset of the current active SIM card.
     * On Failure, returns an empty string.
     */
    String getATR();

    /**
     * Returns the Oem version.
     * @return On Success Contains the version information.
     *         On Failure, returns -1.
     */
    int getOemVersion();

    /**
     * Retrieves the temperature of the selected modem sensor.
     * @param sensorId: Sensor id
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_RF},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_BASEBAND_CHIP},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_PCB}
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_PMU}
     *          or {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_PA}
     *
     * @return On Success, String contains the temperatures separated by space
     *         "Filtered temperature Raw Temperature". Both temperature
     *          are formated as a 4 digits number: "2300" = 23.00 celcius
     *          degrees  (two digits after dot).
     *         On Failure, returns an empty string.
     */
    String getThermalSensorValue(int sensorId);

    /**
     * Retrieves the temperature of the selected modem sensor.
     * @param sensorId: Sensor id
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_RF},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_BASEBAND_CHIP},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_PCB}
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_PMU}
     *          or {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_PA}
     *
     * @return On Success, String contains the temperature in MilliDegC.
     *         On Failure, returns an empty string.
     */
    String getThermalSensorValueV2(String sensorId);

    /**
     * Gets a dump of the GPRS cell environment.
     *
     * @return On Success, returns a string containing a dump of GPRS Cell Environment
     *         On Failure, returns an empty string.
     */
    String getGprsCell();

    /**
     * Retrieves Cell Environment
     * @param page: page_nr
     * @return On Success, string containing a dump of the cell environment
     *         On Failure, returns an empty string.
     */
    String getDumpScreen(int page);

    /**
     * Activates the modem sensor alarm notification when the selected
     * sensor temperature is below the minimal threshold or above the
     * maximal threshold value.
     *
     * @param activate: activate the threshold notification when true,
     *                  else deactivates the notification.
     * @param sensorId: Sensor id
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_RF},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_BASEBAND_CHIP},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_PCB}
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_PMU}
     *          or {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_ID_PA}
     *
     * @param minThresholdValue: temperature are formated as a 4 digits number:
     *                  "2300" = 23.00 celcius degrees  (two digits after dot)
     * @param maxThresholdValue: temperature are formated as a 4 digits number:
     *                  "2300" = 23.00 celcius degrees  (two digits after dot)
     */
    oneway void activateThermalSensorNotification(boolean activate, int sensorId,
                            int minThresholdValue, int maxThresholdValue);

    /**
     * Activates the modem sensor alarm notification when the selected
     * sensor temperature is below the tripPointNumber-hysteresis or above the
     * tripPointNumber.
     *
     * @param sensorId: Sensor id
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_RF},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_BASEBAND_CHIP},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_PCB}
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_PMU}
     *          or {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#MODEM_SENSOR_PA}
     *
     * @param alarmId: Alarm id, allowed values 0-3.
     * @param tripPointNumber: temperature is in MilliDegC.
     * @param hysteresis: temperature is in MilliDegC and should be positive.
     */
    oneway void activateThermalSensorNotificationV2(String sensorId, int alarmId,
                            int tripPointNumber, int hysteresis);
    /**
     * Retrieves the SMS Transport Mode.
     * @return On Success, int representing the SMS Transport Mode,
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#SMS_TRANSPORT_MODE_PACKET_DOMAIN},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#SMS_TRANSPORT_MODE_CIRCUIT_SWITCHED},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#SMS_TRANSPORT_MODE_PACKET_DOMAIN_PREFERRED},
     *          or {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#SMS_TRANSPORT_MODE_CIRCUIT_SWITCHED_PREFERRED}
     *         On Failure, returns {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#SMS_TRANSPORT_MODE_INVALID}
     */
    int getSMSTransportMode();

    /**
     * Sets the SMS Transport Mode.
     * @param transportMode: SMS Transport Mode,
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#SMS_TRANSPORT_MODE_PACKET_DOMAIN},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#SMS_TRANSPORT_MODE_CIRCUIT_SWITCHED},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#SMS_TRANSPORT_MODE_PACKET_DOMAIN_PREFERRED},
     *          or {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#SMS_TRANSPORT_MODE_CIRCUIT_SWITCHED_PREFERRED}
     */
    oneway void setSMSTransportMode(int transportMode);

    /**
     * Get the currently applied rf power cutback table.
     * @return On Success, int representing the applied table.
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#OFFSET_TABLE_INDEX_0},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#OFFSET_TABLE_INDEX_1},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#OFFSET_TABLE_INDEX_2},
     */
    int getRFPowerCutbackTable();

    /**
     * Set the currently applied rf power cutback table.
     * @param table the table to apply.
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#OFFSET_TABLE_INDEX_0},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#OFFSET_TABLE_INDEX_1},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#OFFSET_TABLE_INDEX_2},
     */
    oneway void setRFPowerCutbackTable(int table);

    /**
     * Set the current status for call available with IMS
     * @param table the table to apply.
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#IMS_CALL_NOT_AVAILABLE},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#IMS_CALL_AVAILABLE},
     */
    oneway void setImsCallAvailable(boolean status);

    /**
     * Set the current status for sms available with IMS
     * @param table the table to apply.
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#IMS_SMS_NOT_AVAILABLE},
     *          {@link com.intel.internal.telephony.OemTelephony.
     *                  OemTelephonyConstants#IMS_SMS_AVAILABLE},
     */
    oneway void setImsSmsAvailable(boolean status);

    /**
     * Sends AT command to the modem
     *
     * <p class="note">
     * This API should be used only for testing purpose. Any specific requests should
     * be addressed using new APIs. Requires the caller to be either running in system
     * or phone process.
     *
     * @deprecated
     */
    String sendAtCommand(String command);

    /**
     * Set Femtocell CSG Auto Selection Mode
     *
     * return value: TRUE: set without errors. FALSE: set failed
     */
    boolean setCSGAutoSelection();

    /**
     * Get current CSG state
     */
    String getCSGState();

    /**
     * Get the current status of IMS network.
     *
     * @param iface the interface name for which we want to retrieve
     *              pcscf data from.
     * @param m the message on which the result will be sent. obj field
     *          of message will contain a string array with the following
     *          content: <ul>
     *          <li> [0] the cid of the PDN </li>
     *          <li> [1] the first pcscf server ip</li>
     *          <li> [2] the second pcscf server ip</li>
     *          </ul>
     */
    void getPcscf(in String iface, in IPcscfListener listener);

    /*
     * Sends the SRVCC synchronization params to the modem.
     * @param params the list of params to send.
     */
    void setSrvccSyncParams(in List<OemSrvccSyncParam> params, in Message result);

    /**
     * Activates or deactivates the registration state and band information reporting.
     *
     * @param reportOn: Activates(true) or deactivates(false) the registration state
     *                and band information reporting.
     *
     * <p class="note">
     * Requires the WRITE_SETTINGS permission.
     *
     * @deprecated Not to be used anymore. Replaced by
     *     {@link #setRegistrationStatusAndBandReporting(long, boolean)}
     */
    oneway void setRegistrationStatusAndBandInd(boolean reportOn);

    /**
     * Activate or deactivate adaptive clocking in the modem for thermal
     * control. When activated the modem will send notifications to host
     * driver whenever there is a frequency change
     *
     * @see ACTION_MODEM_ADPCLK_FREQ_CHANGED
     * @param enable: true to activate
     *                false to deactivate the adaptive clocking
     * @return true if the operation succeed; false otherwise
     */
    boolean enableAdaptiveClocking(boolean enable);

    /**
     * Retrieves adaptive clocking frequency information from the modem
     * Attention, adaptive clocking must have been enabled before.
     *
     * @return list of AdaptiveClockingFrequencyInfo received (empty list
     * in case of issues when retrieving the information).
     */
    List<AdaptiveClockingFrequencyInfo> getAdaptiveClockingFrequencyInfo();

    /**
     * Interface to register for new SRVCC listener
     * @param listener listener object
     */
    void registerSrvccListener(in ISrvccListener listener);

    /**
     * Interface to de-register SRVCC listener
     * @param listener listener object
     */
    void unregisterSrvccListener(in ISrvccListener listener);

    /**
     * Interface to register Dedicated Bearer info listener
     * @param listener Dedicated Bearer info listener object
     */
     void registerDedicatedBearerListener(in IDedicatedBearerListener listener);

    /**
     * Interface to de-register Dedicated Bearer listener
     * @param listener Dedicated Bearer listener object
     */
     void unregisterDedicatedBearerListener(in IDedicatedBearerListener listener);


    /**
     * Activates or deactivates the registration state and band information
     * reporting using intent @see ACTION_IDC_REG_STATUS_AND_BAND_IND
     *
     * @param subId: Subscription Id of the target.
     * @param enableReport: Activates(true) or deactivates(false) the
     *          registration state and band information reporting.
     * @return  On success, returns 1
     *          On failure, returns 0
     *          Unsupported, returns -1
     */
    int setRegistrationStatusAndBandReporting(long subId, boolean enableReport);

    /**
     * Activates or deactivates the coexistence information reporting using
     * intents:
     *    @see ACTION_IDC_WLAN_COEX_IND
     *    @see ACTION_IDC_BT_COEX_IND
     *    @see ACTION_IDC_LTE_FRAME_IND
     *    @see ACTION_IDC_LTE_SPS_IND
     *
     * @param subId: Subscription Id of the target.
     * @param enableReport: Activates (true) or deactivates(false) the
     *          coexistence information reporting.
     * @return  On success, returns 1
     *          On failure, returns 0
     *          Unsupported, returns -1
     */
    int setCoexReporting(long subId, boolean enableReport);

    /**
     * Set the Wlan parameters to the modem IDC part.
     *
     * @param subId: Subscription Id of the target.
     * @param enableWlanReport: Wlan status:
     *          activated (true)
     *          deactivated (false)
     * @param wlanBandwidth: NotSpecified / 20MHZ / 40MHz / 80MHz
     * @see WLAN_BANDWIDTH_UNSUPPORTED
     * @see WLAN_BANDWIDTH_20_MHZ
     * @see WLAN_BANDWIDTH_40_MHZ
     * @see WLAN_BANDWIDTH_80_MHZ
     * @return  On success, returns 1
     *          On failure, returns 0
     *          Unsupported, returns -1
     */
    int setWlanParams(long subId, boolean enableWlanReport, int wlanBandwidth);

    /**
     * Set the Bt parameters to the modem IDC part.
     *
     * @param subId: Subscription Id of the target.
     * @param btEnabled: Bluetooth status:
     *          activated (true)
     *          deactivated
     * @return  On success, returns 1
     *          On failure, returns 0
     *          Unsupported, returns -1
     */
    int setBtParams(long subId, boolean btEnabled);
}
