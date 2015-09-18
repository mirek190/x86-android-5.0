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

package com.intel.internal.telephony.OemTelephonyApp;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.AsyncResult;
import android.os.Binder;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.provider.Settings;
import android.util.Log;

import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneBase;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.PhoneProxy;
import com.android.internal.telephony.TelephonyIntents;

import com.intel.internal.telephony.OemTelephony.AdaptiveClockingFrequencyInfo;
import com.intel.internal.telephony.OemTelephony.IDedicatedBearerListener;
import com.intel.internal.telephony.OemTelephony.IOemTelephony;
import com.intel.internal.telephony.OemTelephony.IPcscfListener;
import com.intel.internal.telephony.OemTelephony.ISrvccListener;
import com.intel.internal.telephony.OemTelephony.OemHooksConstants;
import com.intel.internal.telephony.OemTelephony.OemSrvccSyncParam;
import com.intel.internal.telephony.OemTelephony.OemSrvccSyncResult;
import com.intel.internal.telephony.OemTelephony.OemTelephonyConstants;
import com.intel.internal.telephony.OemTelephony.OemTelephonyIntents;
import com.intel.internal.telephony.OemTelephony.PcscfInfo;

import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;

/**
 * Implementation of the IOemTelephony interface.
 */
public class OemTelephonyInterfaceManager extends IOemTelephony.Stub {
    private static final String LOG_TAG = "OemTelephonyInterfaceManager";
    private static final boolean DBG = true;

    // Message codes used with mMainThreadHandler
    private static final int CMD_GET_ATR = 1;
    private static final int EVENT_GET_ATR_DONE = 2;
    private static final int CMD_GET_THERMAL_SENSOR_VALUE = 3;
    private static final int EVENT_GET_THERMAL_SENSOR_VALUE_DONE = 4;
    private static final int CMD_GET_SMS_TRANSPORT_MODE = 5;
    private static final int EVENT_GET_SMS_TRANSPORT_MODE_DONE = 6;
    private static final int CMD_GET_GPRS_CELL_ENV = 7;
    private static final int EVENT_GET_GPRS_CELL_ENV_DONE = 8;
    private static final int CMD_DEBUG_SCREEN_COMMAND = 9;
    private static final int EVENT_DEBUG_SCREEN_COMMAND_DONE = 10;
    private static final int CMD_GET_RF_POWER_CUTBACK_TABLE_COMMAND = 11;
    private static final int EVENT_GET_RF_POWER_CUTBACK_TABLE_COMMAND_DONE = 12;
    private static final int CMD_NOTIFY_RELEASE_MODEM = 13;
    private static final int EVENT_NOTIFY_RELEASE_MODEM_DONE = 14;
    private static final int CMD_SEND_AT = 15;
    private static final int EVENT_SEND_AT_DONE = 16;
    private static final int CMD_GET_PCSCF = 17;
    private static final int EVENT_GET_PCSCF = 18;
    private static final int CMD_SRVCC_PARAM = 19;
    private static final int EVENT_SRVCC_PARAM_DONE = 20;
    private static final int CMD_GET_OEM_VERSION = 21;
    private static final int EVENT_GET_OEM_VERSION_DONE = 22;
    private static final int CMD_GET_THERMAL_SENSOR_VALUE_V2 = 23;
    private static final int EVENT_GET_THERMAL_SENSOR_VALUE_V2_DONE = 24;
    private static final int CMD_SET_CSG_AUTO_MODE = 25;
    private static final int EVENT_SET_CSG_AUTO_MODE_DONE = 26;
    private static final int CMD_GET_CSG_STATE = 27;
    private static final int EVENT_GET_CSG_STATE_DONE = 28;
    private static final int CMD_GET_ADPCLK_FREQ_INFO = 29;
    private static final int EVENT_GET_ADPCLK_FREQ_INFO_DONE = 30;
    private static final int CMD_SET_REGISTRATION_STATUS_AND_BAND_REPORT = 31;
    private static final int EVENT_SET_REGISTRATION_STATUS_AND_BAND_REPORT_DONE = 32;
    private static final int CMD_SET_COEX_REPORT = 33;
    private static final int EVENT_SET_COEX_REPORT_DONE = 34;
    private static final int CMD_SET_COEX_WLAN_PARAMS = 35;
    private static final int EVENT_SET_COEX_WLAN_PARAMS_DONE = 36;
    private static final int CMD_SET_COEX_BT_PARAMS = 37;
    private static final int EVENT_SET_COEX_BT_PARAMS_DONE = 38;

    // Permissions
    private static final String READ_PHONE_STATE =
                        android.Manifest.permission.READ_PHONE_STATE;
    private static final String ACCESS_FINE_LOCATION =
                    android.Manifest.permission.ACCESS_FINE_LOCATION;
    private static final String MODIFY_PHONE_STATE =
                    android.Manifest.permission.MODIFY_PHONE_STATE;
    private static final String WRITE_SETTINGS =
                    android.Manifest.permission.WRITE_SETTINGS;
    private static final String SET_THERMAL =
                    "com.intel.telephony.permission.SET_THERMAL";
    private static final String FIELD_TEST_INFO =
                    "com.intel.telephony.permission.FIELD_TEST_INFO";

    private static final int RELEASE_MODEM_REASON_AIRPLANE_MODE = 2;

    /** The singleton instance. */
    private static OemTelephonyInterfaceManager sInstance;

    //OEM Telephony Handler for events
    private OemTelephonyHandler mOemTelephonyHandler;

    private CommandsInterface mCM;
    Phone mPhone;
    MainThreadHandler mMainThreadHandler;
    private boolean mIsSimRecordsLoaded;
    private boolean mIsSimAbsent = true;
    private boolean mIsAirplaneModeOn = false;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (Intent.ACTION_AIRPLANE_MODE_CHANGED.equals(action)) {
                mIsAirplaneModeOn = intent.getBooleanExtra("state", false);
            } else if (TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(action)) {
                mIsSimAbsent = false;
                String state = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                if (IccCardConstants.INTENT_VALUE_ICC_LOADED.equals(state)) {
                    mIsSimRecordsLoaded = true;
                } else if (IccCardConstants.INTENT_VALUE_ICC_UNKNOWN.equals(state)
                        || IccCardConstants.INTENT_VALUE_ICC_NOT_READY.equals(state)) {
                    mIsSimRecordsLoaded = false;
                } else if (IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(state)) {
                    mIsSimRecordsLoaded = false;
                    mIsSimAbsent = true;
                }
            }

            if (mIsAirplaneModeOn && (mIsSimRecordsLoaded || mIsSimAbsent)) {
                notifyReleaseModem(RELEASE_MODEM_REASON_AIRPLANE_MODE);
            }
        }
    };

    /*
     * A request object for use with {@link MainThreadHandler}. Requesters
     * should wait() on the request after sending. The main thread will notify
     * the request when it is complete.
     */
    private static final class MainThreadRequest {
        /** The argument to use for the request */
        public Object argument;
        /** The result of the request that is run on the main thread */
        public Object result;

        public MainThreadRequest(Object argument) {
            this.argument = argument;
        }
    }

    /*
     * A handler that processes messages on the main thread in the phone
     * process. Since many of the Phone calls are not thread safe this is needed
     * to shuttle the requests from the inbound binder threads to the main
     * thread in the phone process. The Binder thread may provide a
     * {@link MainThreadRequest} object in the msg.obj field that they are
     * waiting on, which will be notified when the operation completes and will
     * contain the result of the request.
     *
     * <p>
     * If a MainThreadRequest object is provided in the msg.obj field, note that
     * request.result must be set to something non-null for the calling thread
     * to unblock.
     */
    private final class MainThreadHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            MainThreadRequest request;
            String[] requestStr;
            Message onCompleted;
            AsyncResult ar;

            switch (msg.what) {
                case CMD_GET_ATR:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(EVENT_GET_ATR_DONE, request);
                    requestStr = new String[1];
                    requestStr[0] = Integer.toString(
                           OemHooksConstants.RIL_OEM_HOOK_STRING_GET_ATR);
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_GET_ATR_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    if (ar.exception == null && ar.result != null) {
                        String response[] = (String[])ar.result;
                        if (response.length > 0) {
                            request.result = response[0];
                        } else {
                            request.result = "";
                        }
                    } else {
                        request.result = "";
                    }
                    synchronized (request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_GET_THERMAL_SENSOR_VALUE:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(EVENT_GET_THERMAL_SENSOR_VALUE_DONE,
                                            request);
                    requestStr = new String[2];
                    requestStr[0] = Integer.toString(
                               OemHooksConstants.RIL_OEM_HOOK_STRING_THERMAL_GET_SENSOR);
                    requestStr[1] = Integer.toString(
                               ((Integer) request.argument).intValue());
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_GET_THERMAL_SENSOR_VALUE_DONE: {
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    String[] resArray = (String[]) ar.result;
                    if (ar.exception == null && resArray != null
                              && resArray.length > 0) {
                        request.result = resArray[0];
                    } else {
                        request.result = "";
                    }
                    synchronized (request) {
                        request.notifyAll();
                    }
                    break;
                }

                case CMD_GET_OEM_VERSION:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(EVENT_GET_OEM_VERSION_DONE, request);
                    requestStr = new String[1];
                    requestStr[0] = Integer.toString(OemHooksConstants
                            .RIL_OEM_HOOK_STRING_GET_OEM_VERSION);
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_GET_OEM_VERSION_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    if (ar.exception == null && ar.result != null
                            && ((String[]) ar.result).length > 0) {
                        request.result = ((String[]) ar.result)[0];
                    } else {
                        request.result = "";
                    }
                    synchronized (request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_GET_THERMAL_SENSOR_VALUE_V2:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(EVENT_GET_THERMAL_SENSOR_VALUE_V2_DONE,
                            request);
                    requestStr = new String[2];
                    requestStr[0] = Integer.toString(
                               OemHooksConstants.RIL_OEM_HOOK_STRING_THERMAL_GET_SENSOR_V2);
                    requestStr[1] = (String)request.argument;
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_GET_THERMAL_SENSOR_VALUE_V2_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    if (ar.exception == null && ar.result != null
                              && ((String[]) ar.result).length > 0) {
                        request.result = ((String[]) ar.result)[0];
                    } else {
                        request.result = "";
                    }

                    synchronized (request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_GET_ADPCLK_FREQ_INFO:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(
                            EVENT_GET_ADPCLK_FREQ_INFO_DONE, request);

                    requestStr = new String[1];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_STRING_ADPCLK_GET_FREQ_INFO);
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_GET_ADPCLK_FREQ_INFO_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    if (ar.exception == null && ar.result != null
                            && ((String[]) ar.result).length > 0) {
                        request.result = ((String[]) ar.result)[0];
                    } else {
                        request.result = "";
                    }

                    synchronized (request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_GET_SMS_TRANSPORT_MODE:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(
                                EVENT_GET_SMS_TRANSPORT_MODE_DONE, request);
                    requestStr = new String[1];
                    requestStr[0] = Integer.toString(
                              OemHooksConstants.RIL_OEM_HOOK_STRING_GET_SMS_TRANSPORT_MODE);
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_GET_SMS_TRANSPORT_MODE_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    if (ar.exception == null && ar.result != null
                              && ((String[]) ar.result).length > 0) {
                        request.result = ((String[]) ar.result)[0];
                    } else {
                        request.result = "";
                    }
                    synchronized (request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_GET_GPRS_CELL_ENV:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(
                            EVENT_GET_GPRS_CELL_ENV_DONE, request);
                    requestStr = new String[1];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_STRING_GET_GPRS_CELL_ENV);
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_GET_GPRS_CELL_ENV_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    if (ar.exception == null && ar.result != null
                                       && ((String[]) ar.result).length > 0) {
                        request.result = ((String[]) ar.result)[0];
                    } else {
                        request.result = "";
                    }
                    synchronized (request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_DEBUG_SCREEN_COMMAND:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(
                            EVENT_DEBUG_SCREEN_COMMAND_DONE, request);
                    requestStr = new String[3];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_STRING_DEBUG_SCREEN_COMMAND);
                    requestStr[1] = "0";
                    requestStr[2] = Integer.toString(
                                ((Integer) request.argument).intValue());
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_DEBUG_SCREEN_COMMAND_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    if (ar.exception == null && ar.result != null
                                && ((String[]) ar.result).length > 0) {
                        request.result = ((String[]) ar.result)[0];
                    } else {
                        request.result = "";
                    }
                    synchronized (request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_GET_RF_POWER_CUTBACK_TABLE_COMMAND:
                    request = (MainThreadRequest)msg.obj;
                    onCompleted = obtainMessage(
                            EVENT_GET_RF_POWER_CUTBACK_TABLE_COMMAND_DONE, request);
                    requestStr = new String[1];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_STRING_GET_RF_POWER_CUTBACK_TABLE);
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_GET_RF_POWER_CUTBACK_TABLE_COMMAND_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    if (ar.exception == null && ar.result != null
                            && ((String[])ar.result).length > 0) {
                        request.result = ((String[])ar.result)[0];
                    } else {
                        request.result = "";
                    }
                    synchronized(request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_NOTIFY_RELEASE_MODEM:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(EVENT_NOTIFY_RELEASE_MODEM_DONE, request);
                    requestStr = new String[2];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_STRING_NOTIFY_RELEASE_MODEM);
                    requestStr[1] = Integer.toString(
                            ((Integer) request.argument).intValue());
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_NOTIFY_RELEASE_MODEM_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    // In order to complete the request, result should not be NULL.
                    request.result = new Integer(0);
                    if (ar.exception != null) {
                        Log.w(LOG_TAG, "Notify release modem failed");
                    }

                    synchronized(request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_SEND_AT:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(
                            EVENT_SEND_AT_DONE, request);
                    requestStr = new String[2];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_STRING_SEND_AT);
                    requestStr[1] = (String)request.argument;
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_SEND_AT_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    if (ar.exception == null && ar.result != null
                            && ((String[]) ar.result).length > 0) {
                        request.result = ((String[]) ar.result)[0];
                    } else {
                        request.result = "";
                    }
                    synchronized (request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_GET_PCSCF:
                    request = (MainThreadRequest) msg.obj;
                    if (DBG) Log.d(LOG_TAG, "getPcscf invokeOemRilRequestStrings CMD_GET_PCSCF " +
                            request.result.toString());
                    onCompleted = obtainMessage(
                            EVENT_GET_PCSCF, request);
                    requestStr = new String[2];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_STRING_IMS_GET_PCSCF);
                    //We use request.result as argument passing for the forward path.
                    requestStr[1] = (String)request.argument;
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_GET_PCSCF:
                    ar = (AsyncResult) msg.obj;
                    if (DBG) Log.d(LOG_TAG, "getPcscf receive EVENT_GET_PCSCF " + ar.result);
                    request = (MainThreadRequest) ar.userObj;
                    IPcscfListener listener = (IPcscfListener) request.result;
                    PcscfInfo pcscf = new PcscfInfo();
                    if (ar.exception == null && ar.result != null
                            && ((String[]) ar.result).length > 0) {
                        String response = ((String[]) ar.result)[0];
                        String[] pcscfParams = response.split("\\s");
                        pcscf.mCid = Integer.parseInt(pcscfParams[0]);
                        pcscf.mPcscfAddresses = new InetAddress[pcscfParams.length - 1];
                        for (int i = 1; i < pcscfParams.length; i++) {
                            pcscf.mPcscfAddresses[i - 1] =
                                    InetAddress.parseNumericAddress(pcscfParams[i]);
                        }
                    }
                    if (DBG) Log.d(LOG_TAG, "getPcscf notifying listener with " + pcscf);
                    try {
                        listener.onPcscfResponse(pcscf);
                    } catch (RemoteException e) {
                        Log.d(LOG_TAG, e.toString());
                    }
                    break;

                case CMD_SRVCC_PARAM:
                    request = (MainThreadRequest) msg.obj;
                    if (DBG) Log.d(LOG_TAG, "invokeOemRilRequestStrings CMD_SRVCC_PARAM "
                            + request.argument.toString());
                    onCompleted = obtainMessage(
                            EVENT_SRVCC_PARAM_DONE, request);
                    requestStr = new String[2];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_STRING_IMS_SRVCC_PARAM);
                    //We use request.result as argument passing for the forward path.
                    requestStr[1] = (String)request.argument;
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_SRVCC_PARAM_DONE:
                    ar = (AsyncResult) msg.obj;
                    if (DBG) Log.d(LOG_TAG, "receive EVENT_SRVCC_PARAM_DONE " + ar.result);
                    request = (MainThreadRequest) ar.userObj;
                    Message rsp = (Message) request.result;
                    if (ar.exception == null && ar.result != null
                            && ((String[]) ar.result).length > 0) {
                        String response = ((String[]) ar.result)[0];

                        ArrayList<OemSrvccSyncResult> parsedResult =
                                new ArrayList<OemSrvccSyncResult>();
                        String[] split = response.split(",");
                        // we expect pairs of callid / result. if count not even then we
                        // have an issue
                        if (split.length % 2 != 0) {
                            Log.e(LOG_TAG, "Wrong result format: " + response);
                            rsp.obj = null;
                            break;
                        }
                        for (int i = 0; i < split.length; i += 2) {
                            try {
                                int callId = Integer.parseInt(split[i].trim());
                                int transferResult = Integer.parseInt(split[i + 1].trim());
                                parsedResult.add(new OemSrvccSyncResult(callId, transferResult));
                            } catch (NumberFormatException ex) {
                                Log.e(LOG_TAG, "Format error in response returned by RIL: "
                                        + ex.toString());
                                break;
                            }
                        }
                        rsp.obj = parsedResult;
                        if (DBG) Log.d(LOG_TAG, "setSrvccSyncParams rsp sendToTarget " + ar.result);
                    } else {
                        rsp.obj = null;
                        Log.e(LOG_TAG, "Error received from RIL");
                        if (ar.exception != null) {
                            Log.e(LOG_TAG, ar.exception.toString());
                        }
                    }
                    rsp.sendToTarget();
                    break;

                case CMD_SET_CSG_AUTO_MODE:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(EVENT_SET_CSG_AUTO_MODE_DONE, request);
                    requestStr = new String[1];
                    requestStr[0] = Integer.toString(
                           OemHooksConstants.RIL_OEM_HOOK_STRING_CSG_SET_AUTOMATIC_SELECTION);
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_SET_CSG_AUTO_MODE_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    if (ar.exception != null) {
                        Log.w(LOG_TAG, "setCSGAutoSelection FAILED");
                    }
                    request.result = (ar.exception == null) ? true : false;

                    synchronized(request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_GET_CSG_STATE:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(EVENT_GET_CSG_STATE_DONE, request);
                    requestStr = new String[1];
                    requestStr[0] = Integer.toString(
                           OemHooksConstants.RIL_OEM_HOOK_STRING_CSG_GET_CURRENT_CSG_STATE);
                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_GET_CSG_STATE_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;
                    if (ar.exception == null && ar.result != null
                            && ((String[]) ar.result).length > 0) {
                        request.result = ((String[]) ar.result)[0];
                    } else {
                        request.result = new String("");
                    }

                    synchronized(request) {
                        request.notifyAll();
                    }
                    break;

                case CMD_SET_REGISTRATION_STATUS_AND_BAND_REPORT:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(EVENT_SET_REGISTRATION_STATUS_AND_BAND_REPORT_DONE,
                            request);

                    requestStr = new String[2];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_SET_REGISTRATION_STATUS_AND_BAND_REPORT);
                    requestStr[1] = ((Integer) request.argument).toString();

                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case CMD_SET_COEX_REPORT:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(EVENT_SET_COEX_REPORT_DONE, request);

                    requestStr = new String[2];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_STRING_SET_COEX_REPORT);
                    requestStr[1] = ((Integer) request.argument).toString();

                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case CMD_SET_COEX_WLAN_PARAMS:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(EVENT_SET_COEX_WLAN_PARAMS_DONE, request);
                    String arg[] = ((String) request.argument).split(" ");

                    requestStr = new String[3];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_STRING_SET_COEX_WLAN_PARAMS);
                    requestStr[1] = arg[0];

                    if (OemTelephonyConstants.WLAN_BANDWIDTH_UNSUPPORTED
                            != Integer.parseInt(arg[1])) {
                        requestStr[2] = arg[1];
                    } else {
                        requestStr[2] = "0";// to comply with RRIL side
                    }

                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case CMD_SET_COEX_BT_PARAMS:
                    request = (MainThreadRequest) msg.obj;
                    onCompleted = obtainMessage(EVENT_SET_COEX_BT_PARAMS_DONE, request);

                    requestStr = new String[2];
                    requestStr[0] = Integer.toString(
                            OemHooksConstants.RIL_OEM_HOOK_STRING_SET_COEX_BT_PARAMS);
                    requestStr[1] = ((Integer) request.argument).toString();

                    mPhone.invokeOemRilRequestStrings(requestStr, onCompleted);
                    break;

                case EVENT_SET_REGISTRATION_STATUS_AND_BAND_REPORT_DONE:
                case EVENT_SET_COEX_REPORT_DONE:
                case EVENT_SET_COEX_WLAN_PARAMS_DONE:
                case EVENT_SET_COEX_BT_PARAMS_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (MainThreadRequest) ar.userObj;

                    if (ar.exception == null) {
                        request.result = String.valueOf(OemTelephonyConstants.SUCCESS);
                    } else if (ar.exception instanceof CommandException) {
                        if (CommandException.Error.REQUEST_NOT_SUPPORTED
                                == ((CommandException) ar.exception).getCommandError()) {
                            request.result = String.valueOf(OemTelephonyConstants.UNSUPPORTED);
                        } else {
                            request.result = String.valueOf(OemTelephonyConstants.FAIL);
                        }
                    }

                    synchronized (request) {
                        request.notifyAll();
                    }
                    break;

                default:
                    Log.w(LOG_TAG, "MainThreadHandler: unexpected message code: "+ msg.what);
                    break;
            }
        }
    }

    /*
     * Posts the specified command to be executed on the main thread, waits for
     * the request to complete, and returns the result.
     *
     * @see sendRequestAsync
     */
    private Object sendRequest(int command, Object argument) {
        if (Looper.myLooper() == mMainThreadHandler.getLooper()) {
            throw new RuntimeException("This method will deadlock if called from the main thread.");
        }
        MainThreadRequest request = new MainThreadRequest(argument);
        Message msg = mMainThreadHandler.obtainMessage(command, request);
        msg.sendToTarget();
        // Wait for the request to complete
        synchronized (request) {
            while (request.result == null) {
                try {
                    request.wait();
                } catch (InterruptedException e) {
                    // Do nothing, go back and wait until the request is
                    // complete
                }
            }
        }
        return request.result;
    }

    /*
     * Asynchronous ("fire and forget") version of sendRequest(): Posts the
     * specified command to be executed on the main thread, and returns
     * immediately.
     *
     * @see sendRequest
     */
    private void sendRequestAsync(int command) {
        mMainThreadHandler.sendEmptyMessage(command);
    }

    /* Private constructor; @see init() */
    private OemTelephonyInterfaceManager(Context context) {
        if (DBG) Log.d(LOG_TAG, "OemTelephonyInterfaceManager()");
        mPhone = ((PhoneProxy)PhoneFactory.getDefaultPhone()).getActivePhone();
        mCM = ((PhoneBase)mPhone).mCi;
        mMainThreadHandler = new MainThreadHandler();
        mOemTelephonyHandler = new OemTelephonyHandler(context);
        publish();
    }

    private void publish() {
        ServiceManager.addService("oemtelephony", this);
        mIsAirplaneModeOn = Settings.System.getInt(
            mPhone.getContext().getContentResolver(), Settings.System.AIRPLANE_MODE_ON, 0) != 0;

        Intent intent = new Intent(OemTelephonyIntents.OEM_TELEPHONY_READY);
        mPhone.getContext().sendBroadcast(intent);
        if (DBG) Log.d(LOG_TAG, "Broadcast : OEM_TELEPHONY_READY");

        IntentFilter filter = new IntentFilter(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        filter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        mPhone.getContext().registerReceiver(mReceiver, filter);
        if (DBG) Log.d(LOG_TAG, "Register for receivers in OemTelephony");
    }

    private void notifyReleaseModem(int reason) {
        MainThreadRequest request = new MainThreadRequest(null);
        request.argument = reason;
        Message msg = mMainThreadHandler.obtainMessage(CMD_NOTIFY_RELEASE_MODEM, request);
        msg.sendToTarget();
    }

    /**
     * Initialize the singleton PhoneInterfaceManager instance. This is only
     * done once, at startup, from PhoneApp.onCreate().
     */
    public static OemTelephonyInterfaceManager init(Context context) {
        synchronized (OemTelephonyInterfaceManager.class) {
            if (sInstance == null) {
                sInstance = new OemTelephonyInterfaceManager(context);
            } else {
                Log.wtf(LOG_TAG, "init() called multiple times!  sInstance = "+ sInstance);
            }
            return sInstance;
        }
    }

    /**
     * Implementation of the following IOemTelephony interfaces.
     * getATR()
     * getOemVersion()
     * getThermalSensorValue(int)
     * getThermalSensorValueV2(String)
     * getGprsCell()
     * getDumpScreen()
     * ActivateThermalSensorNotification(boolean, int, int, int)
     * ActivateThermalSensorNotificationV2(String, int, int, int)
     * setSMSTransportMode(int)
     * getSMSTransportMode()
     * setRegistrationStatusAndBandInd(boolean)
     *
     */
    public String getATR() {
        String atr = "";
        if (mPhone.getContext().checkCallingPermission(
                READ_PHONE_STATE) == PackageManager.PERMISSION_GRANTED) {
            atr = (String) sendRequest(CMD_GET_ATR, null);
            if (DBG) Log.d(LOG_TAG, "getATR " + atr);
        } else {
            Log.e(LOG_TAG, "Permission denied - getATR");
        }

        return atr;
    }

    public int getOemVersion() {
        String version = "";

        if (mPhone.getContext().checkCallingPermission(READ_PHONE_STATE)
                == PackageManager.PERMISSION_GRANTED) {
            try {
                version = (String) sendRequest(CMD_GET_OEM_VERSION, null);
            } catch (RuntimeException e) {
                Log.e(LOG_TAG, "getOemVersion exception: " + e.toString());
            }
        } else {
            Log.e(LOG_TAG, "Permission denied - getOemVersion");
        }

        return version.isEmpty() ? -1 : Integer.parseInt(version);
    }

    public String getThermalSensorValue(int sensorId) {
        String thermalSensorVal = "";

        if (!mCM.getRadioState().isOn()) {
            Log.e(LOG_TAG, "getThermalSensorValue - Radio not on");
            return thermalSensorVal;
        }

        if (mPhone.getContext().checkCallingPermission(
                READ_PHONE_STATE) == PackageManager.PERMISSION_GRANTED) {
            try {
                thermalSensorVal = (String) sendRequest(CMD_GET_THERMAL_SENSOR_VALUE, sensorId);
            } catch (RuntimeException e) {
                Log.e(LOG_TAG, "getThermalSensorValue exception: " + e.toString());
            }
        } else {
            Log.e(LOG_TAG, "Permission denied - getThermalSensorValue");
        }

        return thermalSensorVal;
    }

    public String getThermalSensorValueV2(String sensorId) {
        String thermalSensorVal = "";

        if (mPhone.getContext().checkCallingPermission(READ_PHONE_STATE)
                == PackageManager.PERMISSION_GRANTED) {
            try {
                thermalSensorVal = (String) sendRequest(CMD_GET_THERMAL_SENSOR_VALUE_V2, sensorId);
            } catch (RuntimeException e) {
                Log.e(LOG_TAG, "getThermalSensorValueV2 exception: " + e.toString());
            }
        } else {
            Log.e(LOG_TAG, "Permission denied - getThermalSensorValueV2");
        }

        return thermalSensorVal;
    }


    public String getGprsCell() {
        String gprsCell = "";
        if (mPhone.getContext().checkCallingPermission(
                FIELD_TEST_INFO) == PackageManager.PERMISSION_GRANTED) {
            try {
                gprsCell = (String) sendRequest(CMD_GET_GPRS_CELL_ENV, null);
            } catch (RuntimeException e) {
                Log.e(LOG_TAG, "" + e.toString());
            }
        } else {
            Log.e(LOG_TAG, "Permission denied - getGprsCell");
        }

        return gprsCell;
    }

    @Deprecated
    public String sendAtCommand(String command) {
        Log.w(LOG_TAG, "Deprecated API that will be removed soon. Please use another API.");

        String response = "";
        int uid = Binder.getCallingUid();

        if (uid == Process.SYSTEM_UID || uid == Process.PHONE_UID) {
            try {
                response = (String) sendRequest(CMD_SEND_AT, command + "\r\n");
            } catch (RuntimeException e) {
                Log.e(LOG_TAG, "" + e.toString());
            }
        } else {
            Log.e(LOG_TAG, "Permission denied - sendAtCommand");
        }

        return response;
    }

    public void getPcscf(String ifname, IPcscfListener listener) {
        if (DBG) Log.d(LOG_TAG, "getPcscf sendRequest CMD_GET_PCSCF for " + ifname);
        if (ifname == null) {
            Log.e(LOG_TAG, "Invalid interface name " + ifname);
            return;
        }
        if (!mCM.getRadioState().isOn()) {
            Log.e(LOG_TAG, "ERROR : getPdnInfo - Radio not on");
            return;
        }
        MainThreadRequest request = new MainThreadRequest(listener);
        request.argument = ifname;
        request.result = listener;

        Message msg = mMainThreadHandler.obtainMessage(CMD_GET_PCSCF, request);
        msg.sendToTarget();
    }

    public String getDumpScreen(int page) {
        String dumpScreen = "";
        if (mPhone.getContext().checkCallingPermission(
                FIELD_TEST_INFO) == PackageManager.PERMISSION_GRANTED) {
            try {
                dumpScreen = (String) sendRequest(CMD_DEBUG_SCREEN_COMMAND, page);
            } catch (RuntimeException e) {
                Log.e(LOG_TAG, "" + e.toString());
            }
        } else {
            Log.e(LOG_TAG, "Permission denied - getDumpScreen");
        }

        return dumpScreen;
    }

    public void activateThermalSensorNotification(boolean activate, int sensorId,
                                                  int minThresholdValue,
                                                  int maxThresholdValue) {
        if (!mCM.getRadioState().isOn()) {
            Log.e(LOG_TAG, "activateThermalSensorNotification - Radio not on");
            return;
        }

        if (mPhone.getContext().checkCallingPermission(
                SET_THERMAL) == PackageManager.PERMISSION_GRANTED) {
            String[] request = new String[2];
            request[0] = Integer.toString(
                    OemHooksConstants.RIL_OEM_HOOK_STRING_ACTIVATE_THERMAL_SENSOR_NOTIFICATION);
            request[1] =
                    activate + " " + sensorId + " " + minThresholdValue + " "+ maxThresholdValue;
            mPhone.invokeOemRilRequestStrings(request, null);
            return;
        }
        Log.e(LOG_TAG, "Permission denied - activateThermalSensorNotification");
    }

    public void activateThermalSensorNotificationV2(String sensorId, int alarmId,
            int tripPointNumber, int hysteresis) {
        if (!mCM.getRadioState().isOn()) {
            Log.e(LOG_TAG, "activateThermalSensorNotificationV2 - Radio not on");
            return;
        }

        if (mPhone.getContext().checkCallingPermission(SET_THERMAL)
                == PackageManager.PERMISSION_GRANTED) {
            String[] request = new String[2];
            request[0] = Integer.toString(OemHooksConstants
                    .RIL_OEM_HOOK_STRING_ACTIVATE_THERMAL_SENSOR_NOTIFICATION_V2);
            request[1] =
                    sensorId + " " + alarmId + " " + tripPointNumber + " " + hysteresis;
            mPhone.invokeOemRilRequestStrings(request, null);
            return;
        }
        Log.e(LOG_TAG, "Permission denied - activateThermalSensorNotificationV2");
    }

    public int getSMSTransportMode() {
        int transportMode = OemTelephonyConstants.SMS_TRANSPORT_MODE_INVALID;
        if (mPhone.getContext().checkCallingPermission(
                READ_PHONE_STATE) == PackageManager.PERMISSION_GRANTED) {
            try {
                String mode = (String) sendRequest(CMD_GET_SMS_TRANSPORT_MODE, null);
                transportMode = Integer.parseInt(mode);
            } catch (RuntimeException e) {
                Log.e(LOG_TAG, "getSMSTransportMode exception: " + e.toString());
            }
        } else {
            Log.e(LOG_TAG, "Permission denied - getSMSTransportMode");
        }

        return transportMode;
    }

    public void setSMSTransportMode(int transportMode) {
        if (mPhone.getContext().checkCallingPermission(
                WRITE_SETTINGS) == PackageManager.PERMISSION_GRANTED) {
            String[] request = new String[2];
            request[0] = Integer.toString(
                              OemHooksConstants.RIL_OEM_HOOK_STRING_SET_SMS_TRANSPORT_MODE);
            request[1] = Integer.toString(transportMode);
            mPhone.invokeOemRilRequestStrings(request, null);
            return;
        }
        Log.e(LOG_TAG, "Permission denied - setSMSTransportMode");
    }

    public int getRFPowerCutbackTable() {
        int table = -1;
        if (mPhone.getContext().checkCallingPermission(
                READ_PHONE_STATE) == PackageManager.PERMISSION_GRANTED) {
            try {
                String ret = (String) sendRequest(CMD_GET_RF_POWER_CUTBACK_TABLE_COMMAND, null);
                table = Integer.parseInt(ret);
            } catch (RuntimeException e) {
                Log.e(LOG_TAG, "getRFPowerCutbackTable exception: " + e.toString());
            }
        } else {
            Log.e(LOG_TAG, "Permission denied - getRFPowerCutbackTable");
        }

        return table;
    }

    public void setRFPowerCutbackTable(int table) {
        Log.d(LOG_TAG, "Permissions : "  + mPhone.getContext().checkCallingOrSelfPermission(
                WRITE_SETTINGS));
        if (mPhone.getContext().checkCallingOrSelfPermission(
                WRITE_SETTINGS) == PackageManager.PERMISSION_GRANTED) {
            String[] request = new String[2];
            request[0] = Integer.toString(
                              OemHooksConstants.RIL_OEM_HOOK_STRING_SET_RF_POWER_CUTBACK_TABLE);
            request[1] = Integer.toString(table);
            mPhone.invokeOemRilRequestStrings(request, null);
            return;
        }
        Log.e(LOG_TAG, "Permission denied - setRFPowerCutbackTable");
    }

    public void setImsCallAvailable(boolean status) {
        if (DBG) Log.d(LOG_TAG, "Permissions : "  +
                mPhone.getContext().checkCallingOrSelfPermission(WRITE_SETTINGS));
        if (mPhone.getContext().checkCallingOrSelfPermission(
                WRITE_SETTINGS) == PackageManager.PERMISSION_GRANTED) {
            String[] request = new String[2];
            request[0] = Integer.toString(
                              OemHooksConstants.RIL_OEM_HOOK_STRING_IMS_CALL_STATUS);
            request[1] = Integer.toString(status ?
                    OemTelephonyConstants.IMS_CALL_AVAILABLE :
                    OemTelephonyConstants.IMS_CALL_NOT_AVAILABLE);
            mPhone.invokeOemRilRequestStrings(request, null);
            return;
        }
        Log.e(LOG_TAG, "Permission denied - setImsCallAvailable");
    }

    public void setImsSmsAvailable(boolean status) {
        if (DBG) Log.d(LOG_TAG, "Permissions : "  +
                mPhone.getContext().checkCallingOrSelfPermission(WRITE_SETTINGS));
        if (mPhone.getContext().checkCallingOrSelfPermission(
                WRITE_SETTINGS) == PackageManager.PERMISSION_GRANTED) {
            String[] request = new String[2];
            request[0] = Integer.toString(
                              OemHooksConstants.RIL_OEM_HOOK_STRING_IMS_SMS_STATUS);
            request[1] = Integer.toString(status ?
                    OemTelephonyConstants.IMS_SMS_AVAILABLE :
                    OemTelephonyConstants.IMS_SMS_NOT_AVAILABLE);
            mPhone.invokeOemRilRequestStrings(request, null);
            return;
        }
        Log.e(LOG_TAG, "Permission denied - setImsSmsAvailable");
    }

    public void setSrvccSyncParams(List<OemSrvccSyncParam> params, Message result) {
        if (DBG) Log.d(LOG_TAG, "srvccSyncParams sendRequest CMD_SRVCC_PARAM for " + params);
        if (params == null) {
            Log.e(LOG_TAG, "Invalid SRVCC params " + params);
            return;
        }
        if (mPhone.getContext().checkCallingOrSelfPermission(
                WRITE_SETTINGS) == PackageManager.PERMISSION_GRANTED) {
            StringBuilder sb = new StringBuilder();
            sb.append(params.size()).append(", "); // <noOfCallSessions>
            for (int i = 0; i < params.size(); i++) {
                OemSrvccSyncParam param = params.get(i);
                sb.append(param.mCallId).append(", ");
                sb.append(param.mTid).append(", ");
                sb.append(param.mTidFlag).append(", ");
                sb.append(param.mIsEmergencyCall ? 1 : 0).append(", ");
                sb.append(param.mCallState).append(", ");
                sb.append(param.mAuxState).append(", ");
                sb.append(param.mMptyAuxState).append(", \"");
                sb.append(param.mPhoneNo).append("\", ");
                sb.append(param.mTonNpi).append(", ");
                sb.append(param.mPiSi);
                if (i < params.size() - 1) {
                    sb.append(", ");
                }
            }
            MainThreadRequest request = new MainThreadRequest(result);
            request.argument = sb.toString();
            request.result = result;

            Message msg = mMainThreadHandler.obtainMessage(CMD_SRVCC_PARAM, request);
            msg.sendToTarget();
        } else {
            Log.e(LOG_TAG, "Permission denied - srvccSyncParams");
        }
    }

    @Deprecated
    public void setRegistrationStatusAndBandInd(boolean reportOn) {
        if (mPhone.getContext().checkCallingOrSelfPermission(
                WRITE_SETTINGS) == PackageManager.PERMISSION_GRANTED) {
            String[] request = new String[2];
            request[0] = Integer.toString(
                    OemHooksConstants.RIL_OEM_HOOK_STRING_SET_REG_STATUS_AND_BAND_IND);
            request[1] = Boolean.toString(reportOn);
            mPhone.invokeOemRilRequestStrings(request, null);
            return;
        }
        Log.e(LOG_TAG, "Permission denied - setRegistrationStatusAndBandInd");
    }

    public boolean enableAdaptiveClocking(boolean enable) {
        final String METHOD_NAME = "enableAdaptiveClocking";

        if (!mCM.getRadioState().isOn()) {
            Log.e(LOG_TAG, METHOD_NAME + " - Radio not on");
            return false;
        }

        if (mPhone.getContext().checkCallingPermission(SET_THERMAL)
                == PackageManager.PERMISSION_GRANTED) {
            String[] request = new String[2];
            request[0] = Integer.toString(
                    OemHooksConstants.RIL_OEM_HOOK_STRING_ADPCLK_ACTIVATE);
            request[1] = Integer.toString(enable ? 1 : 0);
            mPhone.invokeOemRilRequestStrings(request, null);

            return true;
        }

        Log.e(LOG_TAG, "Permission denied - " + METHOD_NAME);
        return false;
    }

    public List<AdaptiveClockingFrequencyInfo> getAdaptiveClockingFrequencyInfo() {
        List<AdaptiveClockingFrequencyInfo> retList =
                new ArrayList<AdaptiveClockingFrequencyInfo>();

        final String METHOD_NAME = "getAdaptiveClockingFrequencyInfo";

        if (!mCM.getRadioState().isOn()) {
            Log.e(LOG_TAG, METHOD_NAME + " - Radio not on");
            return retList;
        }

        if (mPhone.getContext().checkCallingPermission(READ_PHONE_STATE)
                == PackageManager.PERMISSION_GRANTED) {
            try {
                // resp will be in the form "x1,y1,z1[,x2,y2,z2][,x3,y3,z3...]"
                String resp = (String) sendRequest(CMD_GET_ADPCLK_FREQ_INFO, null);
                String[] params = resp.split(",");

                // the params list must be a multiple of 3
                if (params.length % 3 != 0) {
                    Log.e(LOG_TAG, METHOD_NAME
                            + " - Invalid ADPCLK response format " + params);

                    return retList;
                }

                for (int k = 0; k < params.length; k = k + 3) {
                    retList.add(new AdaptiveClockingFrequencyInfo(
                            Long.valueOf(params[k].trim()),
                            Integer.valueOf(params[k + 1].trim()),
                            Integer.valueOf(params[k + 2].trim())));
                }
            } catch (RuntimeException e) {
                Log.e(LOG_TAG, METHOD_NAME + e.toString());
            }
        } else {
            Log.e(LOG_TAG, "Permission denied - " + METHOD_NAME);
        }

        return retList;
    }

    @Override
    public void registerSrvccListener(ISrvccListener listener) throws RemoteException {
        mOemTelephonyHandler.registerSrvccListener(listener);
    }

    @Override
    public void unregisterSrvccListener(ISrvccListener listener) throws RemoteException {
        mOemTelephonyHandler.unregisterSrvccListener(listener);
    }

    @Override
    public void registerDedicatedBearerListener(IDedicatedBearerListener listener)
            throws RemoteException {
        mOemTelephonyHandler.registerDedicatedBearerListener(listener);
    }

    @Override
    public void unregisterDedicatedBearerListener(IDedicatedBearerListener listener)
            throws RemoteException {
        mOemTelephonyHandler.unregisterDedicatedBearerListener(listener);
    }

    public boolean setCSGAutoSelection() {
        boolean result = false;
        try {
            result = (Boolean) sendRequest(CMD_SET_CSG_AUTO_MODE, null);
        } catch (Exception e) {
            Log.e(LOG_TAG, "setCSGAutoSelection exception: " + e.toString());
        }
        return result;
    }

    public String getCSGState() {
        String state = "";
        try {
            state = (String) sendRequest(CMD_GET_CSG_STATE, null);
        } catch (Exception e) {
            Log.e(LOG_TAG, "getCSGState exception: " + e.toString());
        }
        return state;
    }

    public int setRegistrationStatusAndBandReporting(long subId, boolean enableReport) {
        final String METHOD_NAME = "setRegistrationStatusAndBandReporting()";
        int ret = OemTelephonyConstants.FAIL;

        if (mPhone.getContext().checkCallingPermission(WRITE_SETTINGS)
                != PackageManager.PERMISSION_GRANTED) {
            Log.e(LOG_TAG, METHOD_NAME + " - Permission denied");
            return ret;
        }

        if (!mCM.getRadioState().isOn()) {
            Log.e(LOG_TAG, METHOD_NAME + " - Radio not on");
            return ret;
        }

        try {
            String response = (String) sendRequest(CMD_SET_REGISTRATION_STATUS_AND_BAND_REPORT,
                    enableReport ? OemTelephonyConstants.ACTIVATE
                            : OemTelephonyConstants.DEACTIVATE);
            ret = Integer.parseInt(response);
        } catch (RuntimeException e) {
            Log.e(LOG_TAG, METHOD_NAME + " - Runtime Exception: " + e.toString());
        }

        return ret;
    }

    public int setCoexReporting(long subId, boolean enableReport) {
        final String METHOD_NAME = "setCoexReporting()";
        int ret = OemTelephonyConstants.FAIL;

        if (mPhone.getContext().checkCallingPermission(WRITE_SETTINGS)
                != PackageManager.PERMISSION_GRANTED) {
            Log.e(LOG_TAG, METHOD_NAME + " - Permission denied");
            return ret;
        }

        if (!mCM.getRadioState().isOn()) {
            Log.e(LOG_TAG, METHOD_NAME + " - Radio not on");
            return ret;
        }

        try {
            String response = (String) sendRequest(CMD_SET_COEX_REPORT, enableReport
                    ? OemTelephonyConstants.ACTIVATE : OemTelephonyConstants.DEACTIVATE);
            ret = Integer.parseInt(response);
        } catch (RuntimeException e) {
            Log.e(LOG_TAG, METHOD_NAME + " - Runtime Exception: " + e.toString());
        }

        return ret;
    }

    public int setWlanParams(long subId, boolean enableWlanReport, int wlanBandwidth) {
        final String METHOD_NAME = "setWlanParams()";
        int ret = OemTelephonyConstants.FAIL;

        if (mPhone.getContext().checkCallingPermission(WRITE_SETTINGS)
                != PackageManager.PERMISSION_GRANTED) {
            Log.e(LOG_TAG, METHOD_NAME + " - Permission denied");
            return ret;
        }

        if (!mCM.getRadioState().isOn()) {
            Log.e(LOG_TAG, METHOD_NAME + " - Radio not on");
            return ret;
        }

        try {
            String response = (String) sendRequest(CMD_SET_COEX_WLAN_PARAMS,
                    new String(
                            Integer.toString(enableWlanReport
                                    ? OemTelephonyConstants.ACTIVATE
                                    : OemTelephonyConstants.DEACTIVATE)
                            + " "
                            + Integer.toString(wlanBandwidth)));
            ret = Integer.parseInt(response);
        } catch (RuntimeException e) {
            Log.e(LOG_TAG, METHOD_NAME + " - Runtime Exception: " + e.toString());
        }

        return ret;
    }

    public int setBtParams(long subId, boolean btEnabled) {
        final String METHOD_NAME = "setBtParams()";
        int ret = OemTelephonyConstants.FAIL;

        if (mPhone.getContext().checkCallingPermission(WRITE_SETTINGS)
                != PackageManager.PERMISSION_GRANTED) {
            Log.e(LOG_TAG, METHOD_NAME + " - Permission denied");
            return ret;
        }

        if (!mCM.getRadioState().isOn()) {
            Log.e(LOG_TAG, METHOD_NAME + " - Radio not on");
            return ret;
        }

        try {
            String response = (String) sendRequest(CMD_SET_COEX_BT_PARAMS, btEnabled
                    ? OemTelephonyConstants.ACTIVATE : OemTelephonyConstants.DEACTIVATE);
            ret = Integer.parseInt(response);
        } catch (RuntimeException e) {
            Log.e(LOG_TAG, METHOD_NAME + " - Runtime Exception: " + e.toString());
        }

        return ret;
    }
}
