/*
 * Copyright 2012 Intel Corporation, All Rights Reserved.
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

import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.AsyncResult;
import android.os.BatteryManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Parcel;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.telephony.PhoneNumberUtils;
import android.telephony.ServiceState;
import android.util.Log;
import android.view.WindowManager;

import com.android.internal.telephony.BaseCommands;
import com.android.internal.telephony.CallManager;
import com.android.internal.telephony.CallTracker;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneBase;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.PhoneProxy;
import com.android.internal.telephony.dataconnection.DcTracker;
import com.android.internal.telephony.gsm.GSMPhone;

import com.intel.internal.telephony.OemTelephony.AdaptiveClockingFrequencyInfo;
import com.intel.internal.telephony.OemTelephony.IDedicatedBearerListener;
import com.intel.internal.telephony.OemTelephony.ISrvccListener;
import com.intel.internal.telephony.OemTelephony.OemHooksConstants;
import com.intel.internal.telephony.OemTelephony.OemQosInfo;
import com.intel.internal.telephony.OemTelephony.OemTelephonyIntents;
import com.intel.internal.telephony.OemTelephony.OemTftInfo;
import com.intel.internal.telephony.OemTelephony.TftInfoItem;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;

public class OemTelephonyHandler extends Handler {
    static final String LOG_TAG = "OemTelephonyHandler";
    private static final boolean DBG = false;

    private static final int MT_CLASS_A   = 1;
    private static final int MT_CLASS_B   = 2;
    private static final int MT_CLASS_CG  = 3;
    private static final int MT_CLASS_CC  = 4;

    private static final int DATA_SUSPENDED = 0;
    private static final int DATA_RESUMED   = 1;

    /* Crashtool reporting type */
    private static final int CRASHTOOL_INFO  = 0;
    private static final int CRASHTOOL_ERROR = 1;
    private static final int CRASHTOOL_STATS = 3;

    /* Crashtool reporting string size */
    private static final int CRASHTOOL_TYPE_SIZE = 64;
    private static final int CRASHTOOL_NB_DATA = 6;
    private static final int CRASHTOOL_BUFFER_SIZE = 255;
    private static final int CRASHTOOL_LARGE_BUFFER_SIZE = 512;

    /* SIM application error size */
    private static final int SIM_APP_ERROR_SIZE = 4;
    /* SIM application error notification id */
    private static final int SIM_APP_ERROR_NOTIFICATION_ID = 987;
    /* Ciphering Indication notification id */
    private static final int CIPHERING_INDICATION_ID = 988;
    /* ims settings preferences */
    private static final String IMS_SHARED_PREF = "ims_shared_pref";
    private static final String IMS_ENABLED_PARAM = "ims_enabled";
    private static final String IMS_SERVICES_PACKAGE = "com.android.phone";

    private static final int EVENT_OEM_HOOK_RAW = 1;
    private static final int EVENT_OEM_HOOK_IMS_REGISTRATION = 2;
    private static final int EVENT_RADIO_STATE_CHANGED = 3;
    private static final int EVENT_SERVICE_STATE_CHANGED = 4;
    private static final int EVENT_VOICE_CALL_STATE_CHANGED = 5;
    private static final int EVENT_VOICE_CALL_ENDED = 6;
    private static final int EVENT_VOICE_REGISTRATION_DENIED_REASON_CHANGED = 7;

    // Voice registration denied reasons
    private static final int VOICE_REGISTRATION_DENIED_REASON_SIM_NOT_PROVISIONED = 2;
    private static final int VOICE_REGISTRATION_DENIED_REASON_SIM_NOT_ALLOWED = 3;
    private static final int VOICE_REGISTRATION_DENIED_REASON_ILLEGAL_ME = 6;

    private static final int DIALOG_TIMEOUT = 90000;
    private static final int MSG_DISMISS_DIALOG = 0;

    private static final int IMS_REGISTERED = 1;

    private static final int MAX_APN_SIZE = 101;
    private static final int MAX_IPADDR_SIZE = 65;
    private static final int MAX_RANGE_SIZE = 12;
    private static final int MAX_TFT_PARAMS = 7;
    private static final int MAX_IFNAME_SIZE = 50;
    private static final int MAX_SENSOR_NAME_SIZE = 10;
    private static final int MAX_BAND_SIZE = 20;

    /** Maximum size of the coex response. */
    private static final int MAX_COEX_RESPONSE_SIZE = 256;

    private CommandsInterface mCM;
    private Phone mPhone;
    private boolean mIsDataSuspended = false;
    private boolean mIsConcurrentVoiceAndDataAllowed = false;
    private DcTracker mDataConnectionTracker;
    private CallManager mCallMgr = null;
    private boolean mIsEmergencyCallOngoing = false;
    private String mEmergencyCallNumber = "";

    private AlertDialog mVoiceRegistrationDeniedReasonDialog;
    private boolean mIsShowRegDeniedReason = false;
    private Context mContext;

    // this flag should be set to true if the notifications from OemTelephony are needed
    // The icon used by both OemTelephony and the recipient of the intent is the same
    // so only one of them should be used to avoid duplicate icons.
    // This boolean is kept in OemTelephony only for debug/test purpose
    private static boolean mCipheringNotificationEnabled = true;
    // boolean to avoid multiple cancelling of the notification on tel events.
    private static boolean mCipheringIndicationCancelled = false;

    private final ArrayList<ISrvccListener> mSrvccListeners = new ArrayList<ISrvccListener>();
    private final ArrayList<IDedicatedBearerListener> mDedicatedBearerListeners =
            new ArrayList<IDedicatedBearerListener>();

    // Constructor
    public OemTelephonyHandler(Context context) {
        mContext = context;
        mPhone = ((PhoneProxy)PhoneFactory.getDefaultPhone()).getActivePhone();
        mCM = ((PhoneBase)mPhone).mCi;
        mDataConnectionTracker = (DcTracker)(((PhoneBase)mPhone).mDcTracker);
        ((BaseCommands)mCM).setOnUnsolOemHookRaw(this, EVENT_OEM_HOOK_RAW, null);
        if (mCipheringNotificationEnabled) {
            mPhone.registerForServiceStateChanged(this, EVENT_SERVICE_STATE_CHANGED, null);
            mCM.registerForRadioStateChanged(this, EVENT_RADIO_STATE_CHANGED, null);
        }
        mCallMgr = CallManager.getInstance();
        mCallMgr.registerForPreciseCallStateChanged(this, EVENT_VOICE_CALL_STATE_CHANGED, null);
        mIsShowRegDeniedReason = mContext.getResources().getBoolean(
                R.bool.config_show_registration_denied_reason);
        if (mIsShowRegDeniedReason) {
            try {
                Method registerForVoiceRegistrationDeniedReasonChanged = Phone.class.
                        getMethod("registerForVoiceRegistrationDeniedReasonChanged",
                        new Class[] {Handler.class, int.class, Object.class});
                registerForVoiceRegistrationDeniedReasonChanged.invoke(mPhone,
                        new Object[] {this, EVENT_VOICE_REGISTRATION_DENIED_REASON_CHANGED, null});
            } catch (NoSuchMethodException | NullPointerException | SecurityException ex) {
                Log.e(LOG_TAG, "Exception in getting method: " + ex);
            } catch (IllegalAccessException | IllegalArgumentException
                    | InvocationTargetException ex) {
                Log.e(LOG_TAG, "Exception while invoking method: " + ex);
            }
        }
    }

    private void cancelNotification(int id) {
        if (id == CIPHERING_INDICATION_ID
            && mCipheringIndicationCancelled) {
            return;
        }
        NotificationManager notifMgr = (NotificationManager) mPhone.getContext()
                .getSystemService(Context.NOTIFICATION_SERVICE);

        if (notifMgr != null) {
            notifMgr.cancel(id);
            if (id == CIPHERING_INDICATION_ID) {
                mCipheringIndicationCancelled = true;
            }
        }
    }

    public void dispose() {
        mCallMgr.unregisterForPreciseCallStateChanged(this);
        if (mCipheringNotificationEnabled) {
            mPhone.unregisterForServiceStateChanged(this);
            mCM.unregisterForRadioStateChanged(this);
            mCipheringIndicationCancelled = false;
        }
        ((BaseCommands)mCM).unSetOnUnsolOemHookRaw(this);
        if (mIsShowRegDeniedReason) {
            try {
                Method unregisterForVoiceRegistrationDeniedReasonChanged = Phone.class.
                        getMethod("unregisterForVoiceRegistrationDeniedReasonChanged",
                        new Class[] {Handler.class});
                unregisterForVoiceRegistrationDeniedReasonChanged.invoke(mPhone,
                        new Object[] {this});
            } catch (NoSuchMethodException | NullPointerException | SecurityException ex) {
                Log.e(LOG_TAG, "Exception in getting method: " + ex);
            } catch (IllegalAccessException | IllegalArgumentException
                    | InvocationTargetException ex) {
                Log.e(LOG_TAG, "Exception while invoking method: " + ex);
            }
        }
    }

    private void broadcastEmergencyCallIntent(boolean value) {
        Intent intent = new Intent
                (OemTelephonyIntents.ACTION_EMERGENCY_CALL_STATUS_CHANGED);
        if (intent != null) {
            intent.putExtra(OemTelephonyIntents.EMERGENCY_CALL_STATUS_KEY, value);
            mPhone.getContext().sendBroadcast(intent);
            Log.i(LOG_TAG, "intent sent "
                    + OemTelephonyIntents.ACTION_EMERGENCY_CALL_STATUS_CHANGED);
        }
    }

    private void onVoiceRegistrationDeniedReasonChanged(AsyncResult r) {
        int cause = (Integer) r.result;
        Log.i(LOG_TAG, "Registration denied reason:" + cause);

        // Dismiss old dialog if exist
        if (mVoiceRegistrationDeniedReasonDialog != null) {
            mVoiceRegistrationDeniedReasonDialog.dismiss();
            mVoiceRegistrationDeniedReasonDialog = null;
        }

        int errorMessageResId = 0;
        switch (cause) {
            case VOICE_REGISTRATION_DENIED_REASON_SIM_NOT_PROVISIONED:
                errorMessageResId = R.string.sim_not_provisioned;
                break;
            case VOICE_REGISTRATION_DENIED_REASON_SIM_NOT_ALLOWED:
                errorMessageResId = R.string.sim_not_allowed;
                break;
            case VOICE_REGISTRATION_DENIED_REASON_ILLEGAL_ME:
                errorMessageResId = R.string.illegal_me;
                break;
        }

        if (errorMessageResId != 0) {
            mVoiceRegistrationDeniedReasonDialog = new AlertDialog.Builder(mContext)
                    .setMessage(errorMessageResId)
                    .setPositiveButton(R.string.ok, null)
                    .create();
            mVoiceRegistrationDeniedReasonDialog.getWindow().setType(
                    WindowManager.LayoutParams.TYPE_SYSTEM_DIALOG);
            mVoiceRegistrationDeniedReasonDialog.getWindow().addFlags(
                    WindowManager.LayoutParams.FLAG_DIM_BEHIND);
            mVoiceRegistrationDeniedReasonDialog.show();

            mVoiceRegistrationDeniedReasonHandler.sendEmptyMessageDelayed(MSG_DISMISS_DIALOG,
                    DIALOG_TIMEOUT);
        }
    }

    private Handler mVoiceRegistrationDeniedReasonHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_DISMISS_DIALOG:
                    if (mVoiceRegistrationDeniedReasonDialog != null) {
                        Log.v(LOG_TAG, "- dismissing registration denied reason dialog");
                        mVoiceRegistrationDeniedReasonDialog.dismiss();
                        mVoiceRegistrationDeniedReasonDialog = null;
                    }
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    public void handleMessage (Message msg) {
        AsyncResult ar;
        Message onComplete;
        switch (msg.what) {
            case EVENT_OEM_HOOK_RAW:
                ar = (AsyncResult)msg.obj;
                handleUnsolicitedOemHookRaw((byte[]) ar.result);
                break;

            case EVENT_OEM_HOOK_IMS_REGISTRATION:
                Log.i(LOG_TAG, "Command complete");
                break;

            case EVENT_RADIO_STATE_CHANGED:
                if (!mCM.getRadioState().isOn()) {
                    cancelNotification(CIPHERING_INDICATION_ID);
                } else {
                    if (SystemProperties.getInt("persist.ims_support", 0) == 1) {
                        Log.i(LOG_TAG, "radio state is on and ims bp centric is enabled"
                                + ", sending ims registration config to the modem");
                        initImsregistration();
                    }
                }
                break;

            case EVENT_SERVICE_STATE_CHANGED:
                ServiceState state = (ServiceState)((AsyncResult) msg.obj).result;
                if (state.getVoiceRegState() != ServiceState.STATE_IN_SERVICE
                    && state.getDataRegState() != ServiceState.STATE_IN_SERVICE) {
                    cancelNotification(CIPHERING_INDICATION_ID);
                }
                break;

             case EVENT_VOICE_CALL_STATE_CHANGED:
                 Log.i(LOG_TAG, " EVENT_VOICE_CALL_STATE_CHANGED " + mIsEmergencyCallOngoing);
                 if (mIsEmergencyCallOngoing != true) {
                     boolean emCall = false;
                     for (Connection fgConn : mCallMgr.getFgCallConnections()) {
                         String number = fgConn.getAddress();
                         Log.i(LOG_TAG, "Call to " + number + " is " + fgConn.isAlive());
                         if (number != null && fgConn.isAlive()
                                 && PhoneNumberUtils.isPotentialEmergencyNumber(number)) {
                             mEmergencyCallNumber = number;
                             emCall = true;
                             break;
                         }
                     }

                     if (emCall == true && mIsEmergencyCallOngoing == false) {
                        broadcastEmergencyCallIntent(true);
                        mIsEmergencyCallOngoing = true;
                        ((PhoneBase)mPhone).getCallTracker().registerForVoiceCallEnded(this,
                                EVENT_VOICE_CALL_ENDED, null);
                    }
                }
                break;

            case EVENT_VOICE_CALL_ENDED:
                Log.i(LOG_TAG, "EVENT_VOICE_CALL_ENDED " + mIsEmergencyCallOngoing);
                if (mIsEmergencyCallOngoing == true) {
                    boolean emCall = false;
                    // If there are no FG calls
                    List<Connection> connList = mCallMgr.getFgCallConnections();
                    if (connList.isEmpty()) {
                        Log.i(LOG_TAG, "No More FG calls");
                        emCall = true;
                    } else {
                        // if there are FG calls, then, check the number
                        for (Connection fgConn : connList) {
                            String number = fgConn.getAddress();
                            Log.i(LOG_TAG, "Call to " + number + " is " + fgConn.isAlive());
                            if (number != null && !fgConn.isAlive()
                                    && mEmergencyCallNumber.equals(number)) {
                                emCall = true;
                                break;
                            }
                        }
                    }

                    if (emCall == true) {
                        broadcastEmergencyCallIntent(false);
                        mIsEmergencyCallOngoing = false;
                        mEmergencyCallNumber = "";
                        ((PhoneBase)mPhone).getCallTracker().unregisterForVoiceCallEnded(this);
                    }
                }
                break;

            case EVENT_VOICE_REGISTRATION_DENIED_REASON_CHANGED:
                onVoiceRegistrationDeniedReasonChanged((AsyncResult) msg.obj);
                break;

            default:
                Log.e(LOG_TAG, "Unhandled message with number: " + msg.what);
                break;
        }
    }

    public boolean isConcurrentVoiceAndDataAllowed() {
        if (DBG) {
            Log.i(LOG_TAG, "mIsConcurrentVoiceAndDataAllowed: " +
                    mIsConcurrentVoiceAndDataAllowed);
        }

        return mIsConcurrentVoiceAndDataAllowed;
    }

    private void handleUnsolicitedOemHookRaw(byte[] rawData) {
        if (DBG) Log.i(LOG_TAG, "handleUnsolicitedOemHookRaw");
        if (rawData.length <= 0) return;

        Parcel p = Parcel.obtain();
        p.unmarshall(rawData, 0, rawData.length);
        p.setDataPosition(0);

        final int msgId = p.readInt();

        switch (msgId) {
            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND:
            {
                Intent intent =
                    new Intent(OemTelephonyIntents.ACTION_MODEM_SENSOR_THRESHOLD_REACHED);
                intent.putExtra(
                        OemTelephonyIntents.MODEM_SENSOR_ID_KEY,
                        p.readInt());
                intent.putExtra(OemTelephonyIntents.MODEM_SENSOR_TEMPERATURE_KEY,
                        p.readInt());
                mPhone.getContext().sendBroadcast(intent);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND_V2:
            {
                int sensorNameLength = p.readInt();
                String sensorName = new String(rawData, p.dataPosition(), sensorNameLength);
                p.setDataPosition(p.dataPosition() + MAX_SENSOR_NAME_SIZE);

                Intent intent =
                        new Intent(OemTelephonyIntents.ACTION_MODEM_SENSOR_THRESHOLD_REACHED_V2);
                intent.putExtra(
                        OemTelephonyIntents.MODEM_SENSOR_ID_KEY,
                        sensorName);
                intent.putExtra(OemTelephonyIntents.MODEM_SENSOR_TEMPERATURE_KEY,
                        p.readInt());
                mPhone.getContext().sendBroadcast(intent);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_DATA_STATUS_IND:
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_DATA_STATUS_IND");
                boolean isDataSuspended =
                        (p.readInt() == DATA_SUSPENDED) ? true : false;
                if (isDataSuspended != mIsDataSuspended) {
                    mIsDataSuspended = isDataSuspended;
                    if (mIsDataSuspended) {
                        SystemProperties.set("gsm.internal.oem.datastate", "suspended");
                        // mDataConnectionTracker.onDataSuspended();
                    } else {
                        SystemProperties.set("gsm.internal.oem.datastate", "resumed");
                        // mDataConnectionTracker.onDataResumed();
                    }
                }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_MT_CLASS_IND:
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_MY_CLASS_IND");
                int mtClass = p.readInt();
                mIsConcurrentVoiceAndDataAllowed = (MT_CLASS_A == mtClass) ? true : false;
                if (mIsConcurrentVoiceAndDataAllowed) {
                    SystemProperties.set("gsm.internal.oem.concurrency", "allowed");
                    if (mPhone.getState() != PhoneConstants.State.IDLE) {
                        // mDataConnectionTracker.onDataResumed();
                    }
                } else {
                    SystemProperties.set("gsm.internal.oem.concurrency", "notallowed");
                    if (mPhone.getState() != PhoneConstants.State.IDLE) {
                        // mDataConnectionTracker.onDataSuspended();
                    }
                }
                break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_IMS_REG_STATUS:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_IMS_REG_STATUS");
                Intent intent =
                      new Intent(OemTelephonyIntents.ACTION_IMS_REGISTRATION_STATE_CHANGED);
                intent.putExtra(
                        OemTelephonyIntents.IMS_STATUS_KEY,
                        (IMS_REGISTERED == p.readInt()));
                mPhone.getContext().sendBroadcast(intent);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_IMS_SUPPORT_STATUS:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_IMS_SUPPORT_STATUS");
                int curr_status = p.readInt();

                Intent intent =
                        new Intent(OemTelephonyIntents.ACTION_IMS_NW_SUPPORT_STATE_CHANGED);
                intent.putExtra(
                        OemTelephonyIntents.IMS_STATUS_KEY,
                        curr_status);
                mPhone.getContext().sendBroadcast(intent);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_CRASHTOOL_EVENT_IND:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_CRASHTOOL_EVENT_IND");

                int type = p.readInt();

                Intent msg = new Intent();
                switch (type) {
                    case CRASHTOOL_INFO:
                        msg.setAction("intel.intent.action.phonedoctor.REPORT_INFO");
                        break;
                    case CRASHTOOL_ERROR:
                        msg.setAction("intel.intent.action.phonedoctor.REPORT_ERROR");
                        break;
                    case CRASHTOOL_STATS:
                        msg.setAction("intel.intent.action.phonedoctor.REPORT_STATS");
                        break;
                    default:
                        Log.i(LOG_TAG, "Unhandled crashtool event type: " + type);
                        p.recycle();
                        return;
                }

                int nameSize = p.readInt();
                String name = new String(rawData, p.dataPosition(), nameSize);
                p.setDataPosition(p.dataPosition() + CRASHTOOL_TYPE_SIZE);

                int[] sizeData = new int[CRASHTOOL_NB_DATA];
                for (int i = 0; i < CRASHTOOL_NB_DATA; i++) {
                    sizeData[i] = p.readInt();
                }

                for (int i = 0; i < CRASHTOOL_NB_DATA; i++) {
                    if (sizeData[i] != 0) {
                        String data = new String(rawData, p.dataPosition(), sizeData[i]);
                        msg.putExtra("intel.intent.extra.phonedoctor.DATA"+i, data);
                    }

                    if (i == 2 || i == 4 || i == 5) {
                        p.setDataPosition(p.dataPosition() + CRASHTOOL_LARGE_BUFFER_SIZE);
                    } else {
                        p.setDataPosition(p.dataPosition() + CRASHTOOL_BUFFER_SIZE);
                    }
                }

                // For call drop reporting, the data5 must contain the battery status
                if (name.equals("TFT_STAT_CDROP")) {
                    // ACTION_BATTERY_CHANGED is a sticky intent. Calling registerReceiver
                    // passing in null as the receiver, the current battery status intent
                    // is returned immediatly
                    Intent batteryIntent = mPhone.getContext().registerReceiver(null,
                            new IntentFilter(Intent.ACTION_BATTERY_CHANGED));

                    if (batteryIntent != null) {
                        int level = batteryIntent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
                        int voltage = batteryIntent.getIntExtra(BatteryManager.EXTRA_VOLTAGE, -1);
                        int temperature = batteryIntent.getIntExtra(
                                BatteryManager.EXTRA_TEMPERATURE, -1);
                        int plugged = batteryIntent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);

                        String batteryData = new String(level + ", " + voltage + ", "
                                + temperature + ", " + plugged);
                        msg.putExtra("intel.intent.extra.phonedoctor.DATA5", batteryData);
                    }
                }


                msg.putExtra("intel.intent.extra.phonedoctor.TYPE", name);
                mPhone.getContext().sendBroadcast(msg);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_COEX_INFO:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_COEX_INFO");

                int responseSize = p.readInt();
                String response = new String(rawData, p.dataPosition(), responseSize);

                Intent intent =
                        new Intent(OemTelephonyIntents.ACTION_COEX_INFO);
                intent.putExtra(
                        OemTelephonyIntents.COEX_INFO_KEY,
                        response);
                mPhone.getContext().sendBroadcast(intent);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_NETWORK_APN_IND:
            {
                int apnSize = p.readInt();
                String apn = new String(rawData, p.dataPosition(), apnSize);
                p.setDataPosition(p.dataPosition() + MAX_APN_SIZE);

                int pdpTypeSize = p.readInt();
                String pdpType = new String(rawData, p.dataPosition(), pdpTypeSize);

                Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_NETWORK_APN_IND" +
                        " - APN: " + apn + " PDP Type: " + pdpType);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_SIM_APP_ERR_IND:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_SIM_APP_ERR_IND");
                String error = new String(rawData, p.dataPosition(), SIM_APP_ERROR_SIZE);
                String errorMsg = "";

                if (error.startsWith("6F")) {
                    errorMsg = "SIM Technical Problem";
                } else if (error.equals("9240")) {
                    errorMsg = "Memory Problem";
                } else if (error.equals("9804")) {
                    errorMsg = "Access security policy not fulfilled or secret code rejected";
                } else if (error.equals("9840")) {
                    errorMsg = "Secret code locked";
                } else {
                    errorMsg = "Unknown error code: " + error;
                }

                Notification notification = new Notification.Builder(mPhone.getContext())
                         .setSmallIcon(android.R.drawable.stat_notify_error)
                         .setContentTitle("SIM Error")
                         .setContentText(errorMsg)
                         .setAutoCancel(true)
                         .setOnlyAlertOnce(true).build();
                NotificationManager mNotificationManager =
                        (NotificationManager) mPhone.getContext().getSystemService(
                                Context.NOTIFICATION_SERVICE);
                mNotificationManager.notify(SIM_APP_ERROR_NOTIFICATION_ID, notification);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_CIPHERING_IND:
            {
                Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_CIPHERING_IND");
                int status = p.readInt();
                String statusMsg = "";
                int icon = R.drawable.stat_sys_ciphering_disable;

                if (status == 0) {
                    statusMsg = "OFF";
                } else if (status == 1) {
                    statusMsg = "ON";
                }

                Log.i(LOG_TAG, "Ciphering Status " + statusMsg);

                // If Notifications enabled. This is only for CS domain
                if (mCipheringNotificationEnabled) {
                    NotificationManager notifMgr = (NotificationManager) mPhone.getContext()
                            .getSystemService(Context.NOTIFICATION_SERVICE);

                    if (notifMgr != null) {
                        if (status == 0) {
                            Notification notification = new Notification
                                    .Builder(mPhone.getContext())
                                    .setSmallIcon(icon)
                                    .setContentTitle("Ciphering: " + statusMsg)
                                    .setAutoCancel(true)
                                    .build();

                            notifMgr.notify(CIPHERING_INDICATION_ID, notification);
                            mCipheringIndicationCancelled = false;
                        } else {
                            cancelNotification(CIPHERING_INDICATION_ID);
                        }
                    }
                }

                // The intent gives the status of the ciphering
                Intent intent =
                        new Intent(OemTelephonyIntents.ACTION_CIPHERING_STATE_CHANGED);
                intent.putExtra(
                        OemTelephonyIntents.CIPHERING_STATUS_KEY, statusMsg);
                mPhone.getContext().sendBroadcast(intent);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_CALL_DISCONNECTED:
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_CALL_DISCONNECTED");
                CallTracker callTracker = ((PhoneBase)mPhone).getCallTracker();

                if (callTracker != null) {
                    // callTracker.onCallDisconnected(p.readInt());
                } else {
                    if (DBG) Log.i(LOG_TAG, "Call Tracker is null!");
                }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_BEARER_TFT_PARAMS:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_BEARER_TFT_PARAMS");
                OemTftInfo info = new OemTftInfo();
                info.mTftInfoItems = new ArrayList<TftInfoItem>();
                info.mIface = new String(rawData, p.dataPosition(),
                        MAX_IFNAME_SIZE).trim();
                p.setDataPosition(p.dataPosition() + MAX_IFNAME_SIZE);
                info.mPCid = p.readInt();
                int count = p.readInt();
                for (int i=0; (i < count) && (i < MAX_TFT_PARAMS); i++) {
                    TftInfoItem infoItem = new TftInfoItem();
                    info.mCid = p.readInt();
                    infoItem.mPfId = p.readInt();
                    infoItem.mEpIdx = p.readInt();
                    infoItem.mSourceAddr = null;
                    infoItem.mSourceMask = null;
                    String addressv4 = new String(rawData, p.dataPosition(),
                            MAX_IPADDR_SIZE);
                    addressv4 = addressv4.trim();
                    p.setDataPosition(p.dataPosition() + MAX_IPADDR_SIZE);
                    String addressv6 = new String(rawData, p.dataPosition(),
                            MAX_IPADDR_SIZE);
                    addressv6 = addressv6.trim();
                    p.setDataPosition(p.dataPosition() + MAX_IPADDR_SIZE);
                    String maskv4 = new String(rawData, p.dataPosition(),
                            MAX_IPADDR_SIZE);
                    maskv4 = maskv4.trim();
                    p.setDataPosition(p.dataPosition() + MAX_IPADDR_SIZE);
                    String maskv6 = new String(rawData, p.dataPosition(),
                            MAX_IPADDR_SIZE);
                    maskv6 = maskv6.trim();
                    p.setDataPosition(p.dataPosition() + MAX_IPADDR_SIZE);
                    if (InetAddress.isNumeric(addressv4)
                            && InetAddress.isNumeric(maskv4)) {
                        infoItem.mSourceAddr = InetAddress.parseNumericAddress(addressv4);
                        infoItem.mSourceMask = InetAddress.parseNumericAddress(maskv4);
                    }
                    if (InetAddress.isNumeric(addressv6)
                            && InetAddress.isNumeric(maskv6)) {
                        infoItem.mSourceAddr = InetAddress.parseNumericAddress(addressv6);
                        infoItem.mSourceMask = InetAddress.parseNumericAddress(maskv6);
                    }
                    if (InetAddress.isNumeric(addressv4) == false
                            && InetAddress.isNumeric(addressv6) == false) {
                        Log.w(LOG_TAG, "Address not parsed properly!");
                        infoItem.mSourceAddr = InetAddress
                                .parseNumericAddress("0.0.0.0");
                        infoItem.mSourceMask = InetAddress
                                .parseNumericAddress("0.0.0.0");
                    }
                    infoItem.mPNumber = p.readInt();
                    String dstRng = new String(rawData, p.dataPosition(), MAX_RANGE_SIZE);
                    infoItem.mDstRng = dstRng.trim();
                    p.setDataPosition(p.dataPosition() + MAX_RANGE_SIZE);
                    String srcRng = new String(rawData, p.dataPosition(), MAX_RANGE_SIZE);
                    infoItem.mSrcRng =srcRng.trim();
                    p.setDataPosition(p.dataPosition() + MAX_RANGE_SIZE);
                    infoItem.mSpi = p.readInt();
                    String tosAndMask = new String(rawData, p.dataPosition(), MAX_RANGE_SIZE);
                    infoItem.mTosAndMask = tosAndMask.trim();
                    p.setDataPosition(p.dataPosition() + MAX_RANGE_SIZE);
                    infoItem.mFlowLabel = p.readInt();
                    infoItem.mDirection = p.readInt();
                    infoItem.mNwId = p.readInt();
                    info.mTftInfoItems.add(infoItem);
                    if (DBG) {
                        StringBuilder sb = new StringBuilder();
                        sb.append("Tft info ").append(i);
                        sb.append(" : Intf ").append(info.mIface).append(", ");
                        sb.append("PCid ").append(info.mPCid).append(", ");
                        sb.append("Cid ").append(info.mCid).append(", ");
                        sb.append("Pfid ").append(infoItem.mPfId).append(", ");
                        sb.append("EpIdx ").append(infoItem.mEpIdx).append(", ");
                        sb.append("SourceAddr ").append(infoItem.mSourceAddr).append(", ");
                        sb.append("SourceMask ").append(infoItem.mSourceMask).append(", ");
                        sb.append("PNumber ").append(infoItem.mPNumber).append(", ");
                        sb.append("DstRng ").append(infoItem.mDstRng).append(", ");
                        sb.append("SrcRng ").append(infoItem.mSrcRng).append(", ");
                        sb.append("Spi ").append(infoItem.mSpi).append(", ");
                        sb.append("TosAndMask ").append(infoItem.mTosAndMask).append(", ");
                        sb.append("FlowLabel ").append(infoItem.mFlowLabel).append(", ");
                        sb.append("Direction ").append(infoItem.mDirection).append(", ");
                        sb.append("NwId ").append(infoItem.mNwId);
                        Log.i(LOG_TAG, sb.toString());
                    }
                }
                synchronized (mDedicatedBearerListeners) {
                    for (IDedicatedBearerListener l : mDedicatedBearerListeners) {
                        try {
                            if (DBG) Log.i(LOG_TAG, "onTftChanged event");
                            l.onTftChanged(info);
                        } catch (RemoteException ex) {
                            Log.e(LOG_TAG, ex.toString());
                        }
                    }
                }
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_BEARER_QOS_PARAMS:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_BEARER_QOS_PARAMS");
                OemQosInfo qosInfo = new OemQosInfo();
                qosInfo.mIface = new String(rawData, p.dataPosition(),
                        MAX_IFNAME_SIZE).trim();
                p.setDataPosition(p.dataPosition() + MAX_IFNAME_SIZE);
                qosInfo.mPCid = p.readInt();
                qosInfo.mCid = p.readInt();
                qosInfo.mQci = p.readInt();
                qosInfo.mDlGbr = p.readInt();
                qosInfo.mUlGbr = p.readInt();
                qosInfo.mDlMbr = p.readInt();
                qosInfo.mUlMbr = p.readInt();
                synchronized (mDedicatedBearerListeners) {
                    for (IDedicatedBearerListener l : mDedicatedBearerListeners) {
                        try {
                            if (DBG) Log.i(LOG_TAG, "onQosChanged event");
                            l.onQosChanged(qosInfo);
                        } catch (RemoteException ex) {
                            Log.e(LOG_TAG, ex.toString());
                        }
                    }
                }
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_BEARER_DEACT:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_BEARER_DEACT");
                String iface = new String(rawData, p.dataPosition(),
                        MAX_IFNAME_SIZE).trim();
                p.setDataPosition(p.dataPosition() + MAX_IFNAME_SIZE);
                int pCid = p.readInt();
                int cid = p.readInt();
                if (DBG) Log.i(LOG_TAG, "Intf " + iface + ", pcid " + pCid + ", cid " + cid);
                synchronized(mDedicatedBearerListeners) {
                    for (IDedicatedBearerListener l : mDedicatedBearerListeners) {
                        try {
                            if (DBG) Log.i(LOG_TAG, "onDedicated Bearer closed event");
                            l.onDedicatedBearerClosed(iface, cid);
                        } catch (RemoteException ex) {
                            Log.e(LOG_TAG, ex.toString());
                        }
                    }
                }
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_BEARER_ACT:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_BEARER_ACT");
                String iface = new String(rawData, p.dataPosition(),
                        MAX_IFNAME_SIZE).trim();
                p.setDataPosition(p.dataPosition() + MAX_IFNAME_SIZE);
                int pCid = p.readInt();
                int cid = p.readInt();
                if (DBG) Log.i(LOG_TAG, "Intf " + iface + ", pcid " + pCid + ", cid " + cid);
                synchronized(mDedicatedBearerListeners) {
                    if (mDedicatedBearerListeners.isEmpty()) {
                        Log.e(LOG_TAG, "no dedicated bearer listener registered");
                    }
                    for (IDedicatedBearerListener l : mDedicatedBearerListeners) {
                        try {
                            if (DBG) Log.i(LOG_TAG, "onDedicated Bearer opened event");
                            l.onDedicatedBearerOpen(iface, cid);
                        } catch (RemoteException ex) {
                            Log.e(LOG_TAG, ex.toString());
                        }
                    }
                }
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_SRVCCH_STATUS: {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_SRVCCH_STATUS");
                int srvcch = p.readInt();
                synchronized (mSrvccListeners) {
                    for (ISrvccListener l : mSrvccListeners) {
                        try {
                            l.onSrvccHandoverStateChanged(srvcch);
                        } catch (RemoteException ex) {
                            Log.e(LOG_TAG, ex.toString());
                        }
                    }
                }
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_SRVCC_HO_STATUS: {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_SRVCC_HO_STATUS");
                final int srvcc_ho_status = p.readInt();
                // srvcc_ho_status = 0 means srvcc sync is needed
                if (srvcc_ho_status == 0) {
                    synchronized (mSrvccListeners) {
                        for (ISrvccListener l : mSrvccListeners) {
                            try {
                                l.onSrvccSyncNeeded();
                            } catch (RemoteException ex) {
                                Log.e(LOG_TAG, ex.toString());
                            }
                        }
                    }
                } else {
                    Log.w(LOG_TAG, "Unexpected value for srvcc_ho_status " + srvcc_ho_status);
                }
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND");
                int regStatus = p.readInt();
                int bandLength = p.readInt();
                String band = new String(rawData, p.dataPosition(), bandLength);
                p.setDataPosition(p.dataPosition() + MAX_BAND_SIZE);

                Intent intent =
                        new Intent(OemTelephonyIntents.ACTION_REG_STATUS_AND_BAND_IND);
                intent.putExtra(OemTelephonyIntents.REG_STATUS_KEY, regStatus);
                intent.putExtra(OemTelephonyIntents.BAND_KEY, band);
                mPhone.getContext().sendBroadcast(intent);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_CSG_IND:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_CSG_IND");

                int csgSelCause = p.readInt();
                int csgId = p.readInt();

                Intent intent =
                        new Intent(OemTelephonyIntents.ACTION_CSG_STATE_CHANGED);
                intent.putExtra(OemTelephonyIntents.CSG_CAUSE_KEY, csgSelCause);

                if (csgId != -1) {
                    intent.putExtra(OemTelephonyIntents.CSG_CELLID_KEY, csgId);
                    intent.putExtra(OemTelephonyIntents.CSG_REC_NUM_KEY, p.readInt());
                    intent.putExtra(OemTelephonyIntents.CSG_HNB_NUM_KEY, p.readInt());
                    intent.putExtra(OemTelephonyIntents.CSG_HNB_NAME_KEY,p.readString());
                    intent.putExtra(OemTelephonyIntents.CSG_OPER_KEY, p.readString());
                    intent.putExtra(OemTelephonyIntents.CSG_ACT_KEY, p.readInt());
                    intent.putExtra(OemTelephonyIntents.CSG_ID_LIST_KEY, p.readInt());
                }
                mPhone.getContext().sendBroadcast(intent);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_ADPCLK_FREQ_INFO_NOTIF:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_ADPCLK_FREQ_INFO_NOTIF");

                Intent intent =
                        new Intent(OemTelephonyIntents.ACTION_MODEM_ADPCLK_FREQUENCY_CHANGED);
                intent.putExtra(OemTelephonyIntents.MODEM_ADPCLK_FREQUENCY_CHANGED_KEY,
                        new AdaptiveClockingFrequencyInfo(p));

                mPhone.getContext().sendBroadcast(intent);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_REPORT:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_REPORT");

                Intent intent =
                        new Intent(OemTelephonyIntents.ACTION_IDC_REG_STATUS_AND_BAND_IND);
                intent.putExtra(OemTelephonyIntents.SUBID_KEY, 0L);
                intent.putExtra(OemTelephonyIntents.REG_STATUS_KEY, p.readInt());

                int bandLength = p.readInt();
                String band = new String(rawData, p.dataPosition(), bandLength);
                intent.putExtra(OemTelephonyIntents.BAND_KEY, band);

                mPhone.getContext().sendBroadcast(intent);
            }
            break;

            case OemHooksConstants.RIL_OEM_HOOK_RAW_UNSOL_COEX_REPORT:
            {
                if (DBG) Log.i(LOG_TAG, "RIL_OEM_HOOK_RAW_UNSOL_COEX_REPORT");

                /*
                 * URC arguments list for LTE / WLAN / BT:
                 *
                 * +XNRTCWSI:
                 * <lte_active>,
                 * <wlan_safe_rx_min>,
                 * <wlan_safe_rx_max>,
                 * <bt_safe_rx_min>,
                 * <bt_safe_rx_max>,
                 * <sps_start>,
                 * <<lte_sps_periodicity>,<lte_sps_duration>,<lte_sps_initial_offset>>,
                 * <lte_bitmap>,
                 * <wlan_safe_tx_min>,
                 * <wlan_safe_tx_max>,
                 * <bt_safe_tx_min>,
                 * <bt_safe_tx_max>,
                 * <lte_tdd_spl_sf_num>,
                 * <num_of_wlan_nrt_safe_tx_power>,
                 * <num_of_bt_nrt_safe_tx_power>,
                 * <<wlan_nrt_safe_tx_power_list1...N>>, // 13 usually
                 * <<bt_nrt_safe_tx_power_list1...N>> // 15 usually
                 */

                /* Used for non populated arguments */
                final int EMPTY_ARG = -1;

                /* Response prefix */
                final String URC_PREFIX = "+XNRTCWSI: ";

                int msgSize = p.readInt();
                String msg = new String(rawData, p.dataPosition(), msgSize);
                p.setDataPosition(p.dataPosition() + MAX_COEX_RESPONSE_SIZE);

                if (!msg.startsWith(URC_PREFIX)) {
                    if (DBG) Log.e(LOG_TAG,
                            "Non [" + URC_PREFIX + "] message received. Nothing to be done.");
                    break;
                }

                /* URC prefix is skipped */
                msg = msg.substring(URC_PREFIX.length(), msg.length());

                String args[] = msg.split(",");
                if (args.length != 45) { // this is the total number of arguments
                    if (DBG) Log.e(LOG_TAG, "Malformed message received from RRIL.");
                    break;
                }

                int lte_active = Integer.parseInt(args[0]);
                int wlan_safe_rx_min = Integer.parseInt(args[1]);
                int wlan_safe_rx_max = Integer.parseInt(args[2]);
                int bt_safe_rx_min = Integer.parseInt(args[3]);
                int bt_safe_rx_max = Integer.parseInt(args[4]);
                int sps_start = Integer.parseInt(args[5]);
                int lte_sps_periodicity = Integer.parseInt(args[6]);
                int lte_sps_duration = Integer.parseInt(args[7]);
                int lte_sps_initial_offset = Integer.parseInt(args[8]);
                int lte_bitmap = Integer.parseInt(args[9]);
                int wlan_safe_tx_min = Integer.parseInt(args[10]);
                int wlan_safe_tx_max = Integer.parseInt(args[11]);
                int bt_safe_tx_min = Integer.parseInt(args[12]);
                int bt_safe_tx_max = Integer.parseInt(args[13]);
                int lte_tdd_spl_sf_num = Integer.parseInt(args[14]);
                int num_of_wlan_nrt_safe_tx_power = Integer.parseInt(args[15]);
                int num_of_bt_nrt_safe_tx_power = Integer.parseInt(args[16]);

                if (EMPTY_ARG != lte_bitmap) {
                    Intent intent =
                            new Intent(OemTelephonyIntents.ACTION_IDC_LTE_FRAME_IND);

                    Bundle extras = new Bundle();
                    extras.putLong(OemTelephonyIntents.LTE_FRAME_INFO_SUBID_KEY, 0L);
                    extras.putInt(OemTelephonyIntents.LTE_FRAME_INFO_BITMAP_KEY,
                            lte_bitmap);
                    extras.putInt(
                            OemTelephonyIntents.LTE_FRAME_INFO_TDDSPECIALSUBFRAMENO_KEY,
                            lte_tdd_spl_sf_num);

                    intent.putExtra(OemTelephonyIntents.LTE_FRAME_INFO_KEY, extras);
                    mPhone.getContext().sendBroadcast(intent);

                } else if (EMPTY_ARG != num_of_wlan_nrt_safe_tx_power) {
                    Intent intent =
                            new Intent(OemTelephonyIntents.ACTION_IDC_WLAN_COEX_IND);

                    Bundle extras = new Bundle();
                    extras.putLong(OemTelephonyIntents.WLAN_COEX_INFO_SUBID_KEY, 0L);
                    extras.putInt(OemTelephonyIntents.WLAN_COEX_INFO_SAFERXMIN_KEY,
                            wlan_safe_rx_min);
                    extras.putInt(OemTelephonyIntents.WLAN_COEX_INFO_SAFERXMAX_KEY,
                            wlan_safe_rx_max);
                    extras.putInt(OemTelephonyIntents.WLAN_COEX_INFO_SAFETXMIN_KEY,
                            wlan_safe_tx_min);
                    extras.putInt(OemTelephonyIntents.WLAN_COEX_INFO_SAFETXMAX_KEY,
                            wlan_safe_tx_max);
                    extras.putInt(
                            OemTelephonyIntents.WLAN_COEX_INFO_SAFETXPOWERNUM_KEY,
                            num_of_wlan_nrt_safe_tx_power);

                    int[] wlan_nrt_safe_tx_power_list = new int[num_of_wlan_nrt_safe_tx_power];

                    // Read the wlan nrt safe tx power list params.
                    for (int k = 0; k < num_of_wlan_nrt_safe_tx_power; k++) {
                        wlan_nrt_safe_tx_power_list[k] = Integer.parseInt(args[17 + k]);
                    }
                    extras.putIntArray(
                            OemTelephonyIntents.WLAN_COEX_INFO_SAFETXPOWERTABLE_KEY,
                            wlan_nrt_safe_tx_power_list);

                    intent.putExtra(OemTelephonyIntents.WLAN_COEX_INFO_KEY, extras);
                    mPhone.getContext().sendBroadcast(intent);

                } else if (EMPTY_ARG != num_of_bt_nrt_safe_tx_power) {
                    Intent intent =
                            new Intent(OemTelephonyIntents.ACTION_IDC_BT_COEX_IND);

                    Bundle extras = new Bundle();
                    extras.putLong(OemTelephonyIntents.BT_COEX_INFO_SUBID_KEY, 0L);
                    extras.putInt(OemTelephonyIntents.BT_COEX_INFO_SAFERXMIN_KEY,
                            bt_safe_rx_min);
                    extras.putInt(OemTelephonyIntents.BT_COEX_INFO_SAFERXMAX_KEY,
                            bt_safe_rx_max);
                    extras.putInt(OemTelephonyIntents.BT_COEX_INFO_SAFETXMIN_KEY,
                            bt_safe_tx_min);
                    extras.putInt(OemTelephonyIntents.BT_COEX_INFO_SAFETXMAX_KEY,
                            bt_safe_tx_max);
                    extras.putInt(
                            OemTelephonyIntents.BT_COEX_INFO_SAFETXPOWERNUM_KEY,
                            num_of_bt_nrt_safe_tx_power);

                    int[] bt_nrt_safe_tx_power_list = new int[num_of_bt_nrt_safe_tx_power];

                    // Read the bt nrt safe tx power list params.
                    for (int k = 0; k < num_of_bt_nrt_safe_tx_power; k++) {
                        bt_nrt_safe_tx_power_list[k] = Integer.parseInt(args[17 + 13 + k]);
                    }
                    extras.putIntArray(
                            OemTelephonyIntents.BT_COEX_INFO_SAFETXPOWERTABLE_KEY,
                            bt_nrt_safe_tx_power_list);

                    intent.putExtra(OemTelephonyIntents.BT_COEX_INFO_KEY, extras);
                    mPhone.getContext().sendBroadcast(intent);

                }
            }
            break;

            default:
                Log.i(LOG_TAG, "Unhandled oem hook raw event, msgId: " + msgId);
        }
        p.recycle();
    }

    private void initImsregistration() {
        /* Initializing mSharedPreferences (IMS Config UI) */
        try {
            Context localContext = mPhone.getContext().createPackageContext(
                    IMS_SERVICES_PACKAGE, Context.CONTEXT_IGNORE_SECURITY);
            SharedPreferences mSharedPreferences = localContext.getSharedPreferences(
                    IMS_SHARED_PREF, Context.MODE_WORLD_READABLE);

            if (!mSharedPreferences.contains(IMS_ENABLED_PARAM)) {
                Editor editor = mSharedPreferences.edit();
                editor.putBoolean(IMS_ENABLED_PARAM, true);
                editor.commit();
                if (DBG) {
                    Log.d(LOG_TAG, "SharedPreference is empty. Setting ims to true since"
                            + " ims should be enabled with BP centric solution");
                }
            }
            boolean imsFeatureEnableState = mSharedPreferences.getBoolean(IMS_ENABLED_PARAM, true);
            if (DBG) Log.d(LOG_TAG, "initImsregistration: ims state on UI "
                    + imsFeatureEnableState);
            int temp = imsFeatureEnableState ? 1 : 0;
            String[] request = new String[2];
            request[0] = Integer
                    .toString(OemHooksConstants.RIL_OEM_HOOK_STRING_IMS_REGISTRATION);
            request[1] = Integer.toString(temp);

            Message msg = this.obtainMessage(EVENT_OEM_HOOK_IMS_REGISTRATION);
            mPhone.invokeOemRilRequestStrings(request, msg);
        } catch (NameNotFoundException e) {
            throw new IllegalStateException("Unable to create package context on "
                    + IMS_SERVICES_PACKAGE);
        } catch (NullPointerException e) {
            throw new IllegalStateException("Unable to access the " + IMS_SERVICES_PACKAGE
                    + " SharedPref");
        }

    }

    public void registerSrvccListener(ISrvccListener listener) throws RemoteException {
        synchronized (mSrvccListeners) {
            mSrvccListeners.add(listener);
        }
    }

    public void unregisterSrvccListener(ISrvccListener listener) throws RemoteException {
        synchronized (mSrvccListeners) {
            mSrvccListeners.remove(listener);
        }
    }

    public void registerDedicatedBearerListener(IDedicatedBearerListener listener) {
        synchronized (mDedicatedBearerListeners) {
            mDedicatedBearerListeners.add(listener);
        }
    }

    public void unregisterDedicatedBearerListener(IDedicatedBearerListener listener) {
        synchronized (mDedicatedBearerListeners) {
            mDedicatedBearerListeners.remove(listener);
        }
    }
}
