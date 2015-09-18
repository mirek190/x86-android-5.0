/*
 * Copyright 2014 Intel Corporation All Rights Reserved.
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

package com.intel.thermal;

import android.app.Notification;
import android.app.NotificationManager;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import android.os.AsyncTask;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;

import android.provider.Settings;

import android.telephony.ServiceState;

import android.util.Log;

import android.widget.Toast;

import com.android.internal.telephony.TelephonyIntents;

import com.intel.internal.telephony.OemTelephony.IOemTelephony;
import com.intel.internal.telephony.OemTelephony.OemTelephonyConstants;
import com.intel.internal.telephony.OemTelephony.OemTelephonyIntents;

import java.lang.Math;

import java.util.ArrayList;
import java.util.Hashtable;

/**
 * ModemZone class
 *  mCurrThermalState is the sensor with max state,
 *  ZoneTemp is temperature of the sensor with max state
 *  These variable needs to be updated synchronously from the
 *  intent receiver function
 *  sensors - PCB,BB,RF.
 *  @hide
 */

public class ModemZone extends ThermalZone {
    private static final String TAG = "Thermal:ModemZone";
    private static boolean sModemReady = false;
    private boolean mPendingIntentRegister = false;
    private static boolean sThermalAirplaneMode = false;
    private IOemTelephony mPhoneService = null;
    private static final int INVALID_VERSION = -1;
    private static final int VERSION_1 = 1;
    private static final int VERSION_2 = 2;
    private static int sApiVersion = INVALID_VERSION;
    private boolean exit;
    private Context mContext;
    private NotificationManager mNotificationManager;
    // oem string related constants
    private static final int FILTERED_TEMP_INDEX = 0;
    private static final int RAW_TEMP_INDEX = 1;
    private static final int MAX_TEMP_VALUES = 2;
    // lock to update and read zone attributes, zonestate and temp
    private static final Object sZoneAttribLock = new Object();
    // lock to update service state and zone monitoring status variables
    private static final Object sMonitorStateLock = new Object();
    // this variable indicates telephony service state - in service,
    // out of service, emrgency call, poweroff
    private static int sServiceState = ServiceState.STATE_POWER_OFF;
    // this flag indicates if sensor thresholds are set for monitoring
    private static boolean sIsMonitoring = false;
    private int mSensorIDwithMaxTemp;
    private static final int MAX_NUM_SENSORS = 3;
    // indices in the sensor attribute arrays
    private static final int SENSOR_INDEX_PCB = 0;
    private static final int SENSOR_INDEX_RF = 1;
    private static final int SENSOR_INDEX_BB = 2;
    private static final int DEFAULT_WAIT_POLL_TIME = 30 * 1000;  // 30 seconds
    // Emergency call related locks and flags
    private static final Object sEmergencyCallLock = new Object();
    private static boolean sOnGoingEmergencyCall = false;
    private static boolean sCriticalMonitorPending = false;
    private static boolean sShutDownInProgress = false;
    // a class variable to ensure only one monitor is active at a time
    private static final int STATUS_MONITOR_RUNNING = -1;
    private static final int STATUS_SUCCESS = 0;
    // this lock is to synchronize critical monitor async task
    private static final Object sCriticalMonitorLock = new Object();
    // this flag suggests if a critical monitor is already started
    private static boolean sIsCriticalMonitorStarted = false;
    // this variable stores the last known zone state.
    // it is used to determine if user needs to be notified via a toast
    // that critical monitor is being started again i.e if zone is still
    // in CRITICAL state, when one poliing interval for critical monitor
    // ends, user should not be notified that monitor is starting again.
    private int mLastKnownZoneState = ThermalManager.THERMAL_STATE_OFF;
    // read from thermal throttle config, critical shutdown flag
    // if shutdown flag is true, donot switch to AIRPLANE mode
    private boolean mIsCriticalShutdownEnable = false;
    private boolean mIsCriticalShutdownflagUpdated = false;
    private boolean mIsModemInfoUpdated = false;
    private ModemStateBroadcastReceiver mIntentReceiver = null;
    private ModemThresholdIntentBroadcastReceiver mThresholdIntentReceiver = null;
    private ShutDownReceiver mShutDownReceiver = null;
    private int mModemOffState = ThermalManager.THERMAL_STATE_OFF;
    private ThermalManager.ZoneCoolerBindingInfo mZoneCoolerBindInfo = null;

    // irrespective of what flag is set in XML, emul temp flag is false for Modem thermal zone
    // over ride function. so that even if flag is 1 in XML, 0 will be written
    public void setEmulTempFlag(int flag) {
        mSupportsEmulTemp = false;
        if (flag != 0) {
            Log.i(TAG, "zone:" + getZoneName()
                    + "is a Modem zone, doesnt support emulated temperature");
        }
    }

    private final class BootCompletedReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            mContext.unregisterReceiver(this);
            sModemReady = true;
            if (mPendingIntentRegister) {
                registerReceiver();
                mPendingIntentRegister = false;
            }
        }
    }


    public ModemZone(Context context) {
        super();

        // register with the intent
        mContext = context;

        IntentFilter filter = new IntentFilter();
        filter = new IntentFilter();
        filter.addAction(OemTelephonyIntents.OEM_TELEPHONY_READY);
        mContext.registerReceiver(new BootCompletedReceiver(), filter);
    }

    private void connect() {

        mPhoneService = IOemTelephony.Stub.asInterface(ServiceManager.getService("oemtelephony"));
        if (mPhoneService == null) {
            Log.i(TAG, "failed to acquire IOemTelephony interface handle\n");
            return;
        }

        // handle to notificaiton manager
        mNotificationManager = (NotificationManager) mContext.getSystemService(
                Context.NOTIFICATION_SERVICE);

        // initialize zone attributes
        updateZoneAttributes(-1, ThermalManager.THERMAL_STATE_OFF, ThermalManager.INVALID_TEMP);

    }

    private void registerModemThresholdIntentReceiver(String intentName) {
        if (intentName != null && mThresholdIntentReceiver == null) {
            mThresholdIntentReceiver = new ModemThresholdIntentBroadcastReceiver();
            IntentFilter filter = new IntentFilter();
            filter.addAction(intentName);
            mContext.registerReceiver(mThresholdIntentReceiver, filter);
        }
    }

    private boolean updateModemInfo() {
        if (mIsModemInfoUpdated == false) {
            if (getOemAPIVersion() == INVALID_VERSION) {
                return false;
            }
            for (ThermalSensor ts : mThermalSensors) {
                if (sApiVersion == VERSION_1) {
                    updateSensorID(ts);
                } else if (sApiVersion == VERSION_2) {
                    updateSensorName(ts);
                    updateSensorID(ts);
                }
            }
            if (sApiVersion == VERSION_1) {
                registerModemThresholdIntentReceiver(OemTelephonyIntents.
                        ACTION_MODEM_SENSOR_THRESHOLD_REACHED);
            } else if (sApiVersion == VERSION_2) {
                registerModemThresholdIntentReceiver(OemTelephonyIntents.
                        ACTION_MODEM_SENSOR_THRESHOLD_REACHED_V2);
            }
            mIsModemInfoUpdated = true;
        }
        return true;
    }

    private void updateModemOffState() {
        int finalState = ThermalManager.THERMAL_STATE_OFF;
        ThermalCoolingDevice device = null;

        if (mZoneCoolerBindInfo != null &&
                mZoneCoolerBindInfo.getCoolingDeviceInfoList() != null) {
            for (ThermalManager.ZoneCoolerBindingInfo.CoolingDeviceInfo cDeviceInfo :
                    mZoneCoolerBindInfo.getCoolingDeviceInfoList()) {
                device = ThermalManager.sCDevMap.get(cDeviceInfo.getCoolingDeviceId());
                if (device != null && device.getDeviceName() != null &&
                        device.getDeviceName().equalsIgnoreCase("ModemAirplane")) {
                    ArrayList<Integer> list = cDeviceInfo.getThrottleMaskList();
                    if (list == null) break;
                    // iterate the list and take first enabled state
                    // We do not throttle in normal state and so don't check that
                    for (int i = 1; i < getMaxStates() - 1; i++) {
                       if (list.get(i) == 1) {
                           finalState = i;
                           break;
                       }
                    }
                }
            }
        }

        if (finalState == ThermalManager.THERMAL_STATE_OFF ||
                (finalState ==  ThermalManager.THERMAL_STATE_CRITICAL &&
                mIsCriticalShutdownEnable == true)) {
            mModemOffState = ThermalManager.THERMAL_STATE_OFF;
        } else {
            mModemOffState = finalState;
        }
        Log.i(TAG, "ModemOff State=" + mModemOffState);
    }

    public void unregisterReceiver() {
        Log.i(TAG, "Modem zone unregister called");
        if (mContext == null) return;

        //Turn off airplane mode, if it had been turned on by us
        if (sThermalAirplaneMode && getAirplaneMode()) setAirplaneMode(false);
        if (mIntentReceiver != null) {
            mContext.unregisterReceiver(mIntentReceiver);
            mIntentReceiver = null;
        }
        if (mThresholdIntentReceiver != null) {
            mContext.unregisterReceiver(mThresholdIntentReceiver);
            mThresholdIntentReceiver = null;
        }

        for (ThermalSensor t : mThermalSensors) {
            t.setCurrTemp(0);
            t.setSensorThermalState(ThermalManager.THERMAL_STATE_OFF);
        }
        // reset zone states
        synchronized (sZoneAttribLock) {
            mSensorIDwithMaxTemp = -1;
            mCurrThermalState = ThermalManager.THERMAL_STATE_OFF;
            mZoneTemp = 0;
            mLastKnownZoneState = ThermalManager.THERMAL_STATE_OFF;
        }
        mIsCriticalShutdownflagUpdated = false;
        setZoneMonitorStatus(false);
    }

    private boolean isDebounceConditionSatisfied(int temp, int debounceInterval, int oldState) {
        int lowTemp = ThermalUtils.getLowerThresholdTemp(oldState, getZoneTempThreshold());
        return (((lowTemp - temp) >= debounceInterval) ? true : false);
    }

    void updateCriticalShutdownFlag() {
        // one time update
        if (mIsCriticalShutdownflagUpdated == false) {
            Hashtable<Integer, ThermalManager.ZoneCoolerBindingInfo> mBindMap =
                    ThermalManager.getCurBindMap();
            if (mBindMap == null) {
                Log.i(TAG, "mBindMap null in updateCriticalShutdownFlag for ModemZone");
                return;
            }

            mZoneCoolerBindInfo = mBindMap.get(getZoneId());
            if (mZoneCoolerBindInfo != null) {
                mIsCriticalShutdownEnable = mZoneCoolerBindInfo.getCriticalActionShutdown() == 1;
            }
            updateModemOffState();
            mIsCriticalShutdownflagUpdated = true;
        }
    }

    @Override
    public void registerReceiver() {
        if (!sModemReady) {
            Log.e(TAG, "Modem is still waiting for ready intent from OemTelephony");
            mPendingIntentRegister = true;
            return;
        }
        connect();
        // register for Modem Intents
        if (mPhoneService == null) return;
        Log.i(TAG, "Modem thermal zone registered successfully");
        if (mIntentReceiver == null) {
            mIntentReceiver = new ModemStateBroadcastReceiver();
            IntentFilter filter = new IntentFilter();
            filter.addAction(TelephonyIntents.ACTION_SERVICE_STATE_CHANGED);
            filter.addAction(OemTelephonyIntents.ACTION_EMERGENCY_CALL_STATUS_CHANGED);
            mContext.registerReceiver(mIntentReceiver, filter);
        }
    }

    // this method is triggered in two conditions:
    // a) service state chnages from OFF to active state
    // b) there is a change is service state within 0,1,2
    // and no sensor thresholds are set.
    // startmonitoring updates all active sensor states and
    // resets thresholds.
    @Override
    public void startMonitoring() {
        int minTemp = 0, maxTemp = 0, temp = 0;
        int currMaxSensorState, sensorState = -1;
        int finalMaxTemp = ThermalManager.INVALID_TEMP;
        int debounceInterval = 0;
        int oldState = ThermalManager.THERMAL_STATE_OFF;

        // register for Shutdown intent
        if (mShutDownReceiver == null) {
            mShutDownReceiver = new ShutDownReceiver();
            IntentFilter shutDownFilter = new IntentFilter();
            shutDownFilter.addAction(Intent.ACTION_SHUTDOWN);
            mContext.registerReceiver(mShutDownReceiver, shutDownFilter);
        }

        if (getServiceState() == ServiceState.STATE_POWER_OFF) {
            Log.i(TAG, "radio not yet available");
            return;
        }

        if (!updateModemInfo()) return;
        updateCriticalShutdownFlag();
        debounceInterval = getDBInterval();
        for (ThermalSensor t : mThermalSensors) {
            temp = readModemSensorTemp(t);
            finalMaxTemp = Math.max(finalMaxTemp,temp);
            if (temp != ThermalManager.INVALID_TEMP) {
                t.setCurrTemp(temp);
                oldState = t.getSensorThermalState();
                sensorState = ThermalUtils.calculateThermalState(temp, getZoneTempThreshold());
                if (oldState == sensorState) continue;
                if ((sensorState < oldState) &&
                        (!isDebounceConditionSatisfied(temp, debounceInterval, oldState)))
                    // update sensor state only if debounce condition statisfied
                    // else retain old state
                    continue;
                Log.i(TAG, "updating sensor state:<old,new>=" + oldState + "," + sensorState);
                t.setSensorThermalState(sensorState);
            }
        }

        if (finalMaxTemp == ThermalManager.INVALID_TEMP) {
            Log.i(TAG, "all modem sensor temp invalid!!!exiting...");
            setZoneMonitorStatus(false);
            return;
        }

        updateLastKnownZoneState();
        currMaxSensorState = getMaxSensorState();
        if (isZoneStateChanged(currMaxSensorState)) {
            sendThermalEvent(mCurrEventType, mCurrThermalState, mZoneTemp);
        }

        if (currMaxSensorState == ThermalManager.THERMAL_STATE_CRITICAL &&
                mIsCriticalShutdownEnable == true) {
            // if shutdown flag is enabled for critical state, The intent sent to
            // Thermal Cooling takes care of platform shutfdown. so just return
            return;
        } else if (currMaxSensorState >= mModemOffState) {
            if (triggerCriticalMonitor()) {
                setZoneMonitorStatus(false);
            }
        } else {
            if (sApiVersion == VERSION_2) {
                programSensorTripPoints();
            } else if (sApiVersion == VERSION_1) {
                // set the thresholds after zone attributes are updated once,
                // to prevent race condition
                Integer[] tempThresholds = getZoneTempThreshold();
                for (ThermalSensor t : mThermalSensors) {
                    sensorState = t.getSensorThermalState();
                    if (sensorState != ThermalManager.THERMAL_STATE_OFF) {
                        minTemp = ThermalUtils.getLowerThresholdTemp(sensorState, tempThresholds);
                        maxTemp = ThermalUtils.getUpperThresholdTemp(sensorState, tempThresholds);
                        minTemp -= debounceInterval;
                        setModemSensorThreshold(true, t, minTemp, maxTemp);
                    }
                }
            }
            setZoneMonitorStatus(true);
        }
    }

    private void programSensorTripPoints() {
        int alarmID = 0;
        for (ThermalSensor t : mThermalSensors) {
            alarmID = 0;
            for (int i = ThermalManager.THERMAL_STATE_NORMAL;
                    i < ThermalManager.THERMAL_STATE_CRITICAL; i++) {
                try {
                    Log.i(TAG, "activating 3 trip point alrms, sensor:"
                            + t.getSensorName() + " temp:"
                            + ThermalUtils.getUpperThresholdTemp(i, getZoneTempThreshold()));
                    mPhoneService.activateThermalSensorNotificationV2(t.getSensorName(), alarmID++,
                            ThermalUtils.getUpperThresholdTemp(i, getZoneTempThreshold()),
                            getDBInterval());
                } catch (RemoteException e) {
                    Log.i(TAG, "caught exception while programming"
                            + " trip points for sensor:" + t.getSensorName());
                    continue;
                }
            }
        }
    }

    private final class ModemStateBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent)
        {
            String action = intent.getAction();
            Log.i(TAG, "got intent with action: " + action);

            if (action.equals(TelephonyIntents.ACTION_SERVICE_STATE_CHANGED)) {
                handleServiceStateChange(intent);
            } else if (action.equals(OemTelephonyIntents.ACTION_EMERGENCY_CALL_STATUS_CHANGED)) {
                handleEmergencyCallIntent(intent);
            }
        }
    }

    private final class ModemThresholdIntentBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.i(TAG, "got intent with action: " + action);

            if (action.equals(OemTelephonyIntents.ACTION_MODEM_SENSOR_THRESHOLD_REACHED)
                    || action.equals(OemTelephonyIntents.
                            ACTION_MODEM_SENSOR_THRESHOLD_REACHED_V2)) {
                if (sApiVersion == VERSION_1) {
                    handleSensorThesholdReachedV1(intent);
                } else if (sApiVersion == VERSION_2) {
                    handleSensorThesholdReachedV2(intent);
                }
            }
        }
    }

    private static void setServiceState(int val) {
        synchronized(sMonitorStateLock) {
            sServiceState = val;
        }
    }

    private static int getServiceState() {
        synchronized(sMonitorStateLock) {
            return sServiceState;
        }
    }

    private static boolean getZoneMonitorStatus() {
        synchronized(sMonitorStateLock) {
            return sIsMonitoring;
        }
    }

    private static void setZoneMonitorStatus(boolean flag) {
        synchronized(sMonitorStateLock) {
            sIsMonitoring = flag;
        }
    }

    private boolean isZoneStateChanged(int currSensorstate) {
        boolean retVal = false;
        if (currSensorstate != mCurrThermalState) {
            mCurrEventType = (currSensorstate < mCurrThermalState) ?
                    ThermalManager.THERMAL_LOW_EVENT : ThermalManager.THERMAL_HIGH_EVENT;
            int currMaxSensorState = getMaxSensorState();
            if ((mCurrEventType == ThermalManager.THERMAL_HIGH_EVENT) ||
                    ((mCurrEventType == ThermalManager.THERMAL_LOW_EVENT) &&
                            (currMaxSensorState < mCurrThermalState))) {
                int sensorID = getSensorIdWithMaxState();
                updateZoneAttributes(sensorID,
                        currMaxSensorState, getSensorTempFromID(sensorID));
                retVal = true;
            }
        }
        return retVal;
    }

    public void updateLastKnownZoneState() {
        synchronized (sZoneAttribLock) {
            mLastKnownZoneState = mCurrThermalState;
            Log.i(TAG, "updateLastKnownZoneState: laststate:" + mLastKnownZoneState);
        }
    }

    public int getLastKnownZoneState() {
        synchronized (sZoneAttribLock) {
            Log.i(TAG, "getLastKnownZoneState: laststate:" + mLastKnownZoneState);
            return mLastKnownZoneState;
        }
    }

    public int getZoneCurrThermalState() {
        synchronized (sZoneAttribLock) {
            return mCurrThermalState;
        }
    }

    public void setZoneCurrThermalState(int state) {
        synchronized (sZoneAttribLock) {
            mCurrThermalState = state;
        }
    }

    public int getCurrZoneTemp() {
        synchronized (sZoneAttribLock) {
            return mZoneTemp;
        }
    }

    private void updateZoneAttributes(int sensorID, int state, int temp) {
        synchronized (sZoneAttribLock) {
            mLastKnownZoneState = mCurrThermalState;
            mSensorIDwithMaxTemp = sensorID;
            mCurrThermalState = state;
            mZoneTemp = temp;
            Log.i(TAG, "updateZoneAttrib: lastState:" + mLastKnownZoneState +
                    "currstate:" + mCurrThermalState);
        }
    }

    // TODO: Make this API take no arguments like the ones in ThermalZone.java
    private void sendThermalEvent (int eventType, int thermalState, int temp) {
        ThermalEvent event = new ThermalEvent(mZoneID, eventType, thermalState,
                temp, mZoneName, ThermalManager.getCurProfileName());
        ThermalManager.addThermalEvent(event);
    }

    private void setModemSensorThreshold(boolean flag, ThermalSensor t, int minTemp, int maxTemp) {
        String str;
        if (mPhoneService == null) return;
        try {
            Log.i(TAG,"Setting Thresholds for Modem Sensor: " +
                    t.getSensorName() + "--Min: " + minTemp + "Max: " + maxTemp);
            // convert temp format from millidegrees to format expected by oemTelephony class
            minTemp = minTemp / 10;
            maxTemp = maxTemp / 10;
            mPhoneService.activateThermalSensorNotification(flag,
                    t.getSensorID(), minTemp, maxTemp);
        } catch (RemoteException e) {
            Log.i(TAG, "remote exception while Setting Thresholds");
        }
    }

    // this fucntion returns the temparature read for a given sensor ID.
    // the temparature returned can be either a filtered value or raw value,
    // depending on the tempindex. the temp returned is in the format 2300
    // i.e last two digits are after decimal places. it should be interpreted
    // as 23.00 degrees
    public int readModemSensorTemp(ThermalSensor t) {
        if (mPhoneService == null || t == null) return ThermalManager.INVALID_TEMP;
        String value;
        ArrayList<Integer> tempList = new ArrayList<Integer>();
        int finalval = ThermalManager.INVALID_TEMP;

        if (mPhoneService == null) return -1;

        try {
            if (sApiVersion == VERSION_1) {
                value = mPhoneService.getThermalSensorValue(t.getSensorID());
                if (value != null && value.length() > 0) {
                    for (String token : value.split(" ")) {
                        try {
                            tempList.add(Integer.parseInt(token));
                        } catch (NumberFormatException e) {
                            Log.i(TAG, token + "is not a number");
                            return ThermalManager.INVALID_TEMP;
                        }
                    }
                    finalval = tempList.get(FILTERED_TEMP_INDEX);
                    finalval *= 10;
                }
            } else if (sApiVersion == VERSION_2) {
                value = mPhoneService.getThermalSensorValueV2(t.getSensorName());
                if (value != null && value.length() > 0) {
                    finalval = Integer.parseInt(value);
                }
            } else {
                Log.i(TAG, "unknown API version for getThermalSensorValueV2()");
                return ThermalManager.INVALID_TEMP;
            }
        } catch (RemoteException e) {
            Log.i(TAG, "Remote Exception while reading temp for sensor:" + t.getSensorName());
            return ThermalManager.INVALID_TEMP;
        }

        if (finalval == ThermalManager.INVALID_TEMP) {
            Log.i(TAG, "readSensorTemp():finalval for sensor:"+ t.getSensorName() + " is invalid");
        } else {
            Log.i(TAG, "readSensorTemp():finalval for sensor:"+ t.getSensorName() + " is " +
                    finalval);
        }
        return finalval;
    }

    private ThermalSensor getThermalSensorObject(String name) {
        if (name == null) return null;
        for (ThermalSensor t : mThermalSensors) {
            if (t.getSensorName() != null && t.getSensorName().equalsIgnoreCase(name)) {
                return t;
            }
        }
        return null;
    }

    // ZoneState is the max of three modem sensors states: PCB,BB,RF
    private int getMaxSensorState() {
        int maxState = -1;
        int currState;
        for (ThermalSensor t : mThermalSensors) {
            currState = t.getSensorThermalState();
            if (maxState < currState) maxState = currState;
        }
        return maxState;
    }

    private int getSensorIdWithMaxState() {
        int maxIndex = 0;
        int maxState = ThermalManager.THERMAL_STATE_OFF, currState;
        int sensorID = -1;

        for (ThermalSensor t : mThermalSensors) {
            currState = t.getSensorThermalState();
            if (maxState < currState) {
                maxState = currState;
                sensorID = t.getSensorID();
            }
        }
        return sensorID;
    }

    private int getSensorTempFromID(int sensorID) {
        ThermalSensor t = getThermalSensorObject(sensorID);
        if (t == null) return ThermalManager.INVALID_TEMP;
        return t.getCurrTemp();
    }

    private ThermalSensor getThermalSensorObject(int sensorID) {
        for (ThermalSensor t : mThermalSensors) {
            if (t.getSensorID() == sensorID) {
                return t;
            }
        }
        return null;
    }

    private void setAirplaneMode(boolean enable) {
        if (getAirplaneMode() == enable) return;
        int state = enable ? 1 : 0;

        // Change the system setting
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON,
                state);
        if (enable) sThermalAirplaneMode = true;
        // Post the intent
        Log.i(TAG, "sending AIRPLANE_MODE_CHANGED INTENT with enable:" + enable);
        Intent intent = new Intent(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        intent.putExtra("state", enable);
        mContext.sendBroadcast(intent);
    }

    private boolean getAirplaneMode() {
        return (Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, -1) == 1) ?
                        true : false;
    }

    // this fucntion is called in context of UI thread from a Async Task.
    // Modem zone switched platform to AIRPLANE mode and sleeps for specified
    // time. before turning AIRPLANE mode OFF
    private void sleep(int millisec) {
        try {
            Thread.sleep(millisec);
        } catch (InterruptedException iex){}
    }

    private void vibrate() {
        Notification n = new Notification();

        // after a 100ms delay, vibrate for 200ms then pause for another
        // 100ms and then vibrate for 500ms
        n.vibrate = new long[]{0, 200, 100, 500};
        if (mNotificationManager != null) {
            mNotificationManager.notify(0, n);
        }
    }

    private class CriticalStateMonitor extends AsyncTask<Void, String, Integer>{
        @Override
        protected Integer doInBackground(Void... arg0) {
            if (getCriticalMonitorStatus() != true) {
                return STATUS_MONITOR_RUNNING;
            }
            // if last known state of zone was already CRITICAL,
            // user need not be notified again, while the polling begins again.
            if (getLastKnownZoneState() < mModemOffState) {
                vibrate();
                publishProgress("Modem heating up!" +
                        "Switching to Airplane mode for sometime");
            }
            Log.i(TAG, "setting airplaneMode ON...");
            if (!sShutDownInProgress) setAirplaneMode(true);
            sleep(DEFAULT_WAIT_POLL_TIME);
            Log.i(TAG, "setting airplaneMode OFF...");
            setAirplaneMode(false);
            setCriticalMonitorStatus(false);
            return STATUS_SUCCESS;
        }

        @Override
        protected void onProgressUpdate(String ...arg){
            String str = arg[0];
            super.onProgressUpdate(arg);
            Toast.makeText(mContext, str, Toast.LENGTH_LONG).show();
        }

        @Override
        protected void onPostExecute(Integer result){
            super.onPostExecute(result);
            Log.i(TAG, "monitor return status = " + result);
            if (result == STATUS_SUCCESS && mIntentReceiver != null) {
                synchronized(sMonitorStateLock) {
                    if (getServiceState() != ServiceState.STATE_POWER_OFF &&
                            getZoneMonitorStatus() == false) {
                        // this handles a scenario where critical monitor turns ON airplane mode,
                        // but user forcefully turns it OFF, but modem temp is still critical.
                        // start monitoring doesn't set thresholds and doesn't call critical monitor
                        // since one instance of the async task is already running. In that case
                        // once the critical monitor finishes, no ACTION_SERVICE_STATE_CHANGE intent
                        // will be sent, as system is already out of AIRPLANE mode.
                        // to handle this, we should check if service state is not OFF,
                        // and monitorStatus is false .
                        // This check is synchronized with sMonitorStateLock.
                        startMonitoring();
                    }
                }
            }
        }
    }

    private void handleSensorThesholdReachedV1(Intent intent) {

        int currSensorstate, currZoneState;
        int minTemp, maxTemp;
        ThermalSensor t;

        int sensorID = intent.getIntExtra(OemTelephonyIntents.MODEM_SENSOR_ID_KEY, 0);
        int temperature = intent.getIntExtra(
                OemTelephonyIntents.MODEM_SENSOR_TEMPERATURE_KEY, 0);
        // convert to millidegree celcius
        temperature *= 10;
        t = getThermalSensorObject(sensorID);
        if (t == null) return;
        Log.i(TAG, "Got notification for Sensor:" + t.getSensorName()
                + " with Current Temperature " + temperature);

        synchronized (sZoneAttribLock) {
            // this method is triggered asynchonously for any sensor that trips the
            // threshold; The zone state, is the max of all the sensor states; If a
            // sensor moves to a state, that the zone is already in, no action is taken.
            // Only the upper and lower thresholds for the given state are reactivated
            // for the respective sensor.
            // if the sensor moves to a different state, this gets updated synchronously
            // in zone state variables

            currSensorstate = ThermalUtils.calculateThermalState(
                    temperature, getZoneTempThreshold());
            t.setSensorThermalState(currSensorstate);
            t.setCurrTemp(temperature);

            if (isZoneStateChanged(currSensorstate)) {
                sendThermalEvent(mCurrEventType, mCurrThermalState, mZoneTemp);
            }

            if (currSensorstate == ThermalManager.THERMAL_STATE_CRITICAL &&
                    mIsCriticalShutdownEnable == true) {
                // if shutdown flag is enabled for critical state, The intent sent to
                // Thermal Cooling takes care of platform shutfdown. so just return
                return;
            } else if (currSensorstate == mModemOffState) {
                if (triggerCriticalMonitor()) {
                    setZoneMonitorStatus(false);
                }
            } else {
                // if temp below critical, reset thresholds
                // reactivate thresholds
                Integer[] tempThresholds = getZoneTempThreshold();
                minTemp = ThermalUtils.getLowerThresholdTemp(currSensorstate, tempThresholds);
                maxTemp = ThermalUtils.getUpperThresholdTemp(currSensorstate, tempThresholds);
                int debounceInterval = getDBInterval();
                minTemp -= debounceInterval;
                setModemSensorThreshold(true, t, minTemp, maxTemp);
                setZoneMonitorStatus(true);
            }
        }
    }

    private void handleSensorThesholdReachedV2(Intent intent) {
        int currSensorstate;
        int currZoneState;
        ThermalSensor t;

        String sensorName = intent.getStringExtra(OemTelephonyIntents.MODEM_SENSOR_ID_KEY);
        int temperature = intent.getIntExtra(
                OemTelephonyIntents.MODEM_SENSOR_TEMPERATURE_KEY, 0);
        t = getThermalSensorObject(sensorName);
        if (t == null) return;

        Log.i(TAG, "Got notification for Sensor:" + sensorName
                + " with Current Temperature " + temperature);

        synchronized (sZoneAttribLock) {
            // this method is triggered asynchonously for any sensor that trips the
            // threshold; The zone state, is the max of all the sensor states; If a
            // sensor moves to a state, that the zone is already in, no action is taken.
            // Only the upper and lower thresholds for the given state are reactivated
            // for the respective sensor.
            // if the sensor moves to a different state, this gets updated synchronously
            // in zone state variables

            currSensorstate = ThermalUtils.calculateThermalState(
                    temperature, getZoneTempThreshold());
            t.setCurrTemp(temperature);
            // if old state  equal to new state ignore and return
            if (currSensorstate == t.getSensorThermalState()) return;
            t.setSensorThermalState(currSensorstate);

            if (isZoneStateChanged(currSensorstate)) {
                sendThermalEvent(mCurrEventType, mCurrThermalState, mZoneTemp);
            }
            if (currSensorstate == ThermalManager.THERMAL_STATE_CRITICAL
                    && mIsCriticalShutdownEnable == true) {
                // if shutdown flag is enabled for critical state, The intent sent to
                // Thermal Cooling takes care of platform shutfdown. so just return
                return;
            } else if (currSensorstate == mModemOffState) {
                if (triggerCriticalMonitor()) {
                    setZoneMonitorStatus(false);
                }
            }
        }
    }

    private void handleServiceStateChange(Intent intent ) {
        Bundle extras = intent.getExtras();
        if (extras == null) {
            return;
        }

        ServiceState ss = ServiceState.newFromBundle(extras);
        if (ss == null) return;
        boolean monitorStatus;
        int newServiceState;
        int oldServiceState;
        synchronized(sMonitorStateLock) {
            newServiceState = ss.getState();
            oldServiceState = getServiceState();
            setServiceState(newServiceState);
            monitorStatus = getZoneMonitorStatus();
            Log.i(TAG, "<old, new> servicestate = " + oldServiceState + ", " + newServiceState);
            Log.i(TAG, "<monitor status> = " + monitorStatus);
            // if ongoing critical monitor, donot call startmonitoring
            if (getCriticalMonitorStatus()) return;
            if (newServiceState == ServiceState.STATE_POWER_OFF) {
                setZoneMonitorStatus(false);
            } else if (oldServiceState ==  ServiceState.STATE_POWER_OFF ||
                    monitorStatus == false) {
                setZoneMonitorStatus(true);
                startMonitoring();
            }
        }
    }

    private void handleEmergencyCallIntent(Intent intent) {
        boolean callStatus = intent.getBooleanExtra("emergencyCallOngoing", false);
        Log.i(TAG, "emergency call intent received, callStatus = " + callStatus);
        synchronized(sEmergencyCallLock) {
            sOnGoingEmergencyCall = callStatus;
            // if emergency call has ended, check if critical state monitor is pending
            if (callStatus == false) {
                if (sCriticalMonitorPending == false) {
                    return;
                }
                sCriticalMonitorPending = false;
                // if critical shutdown enable , just exit. since an intent is sent
                // to ThermalCooling to shutdown the platform
                if (getLastKnownZoneState() == ThermalManager.THERMAL_STATE_CRITICAL &&
                        mIsCriticalShutdownEnable == true) {
                    return;
                }
                // if critical monitor is pending start a async task.
                if (triggerCriticalMonitor()) {
                    setZoneMonitorStatus(false);
                }
            }
        }
    }

    public static boolean isEmergencyCallOnGoing() {
        synchronized(sEmergencyCallLock) {
            return sOnGoingEmergencyCall;
        }
    }

    private boolean getCriticalMonitorPendingStatus(){
        synchronized(sEmergencyCallLock) {
            return sCriticalMonitorPending;
        }
    }

    private void setCriticalMonitorPendingStatus(boolean flag){
        synchronized(sEmergencyCallLock) {
            sCriticalMonitorPending = flag;
        }
    }

    private static boolean getCriticalMonitorStatus() {
        synchronized(sCriticalMonitorLock) {
            return sIsCriticalMonitorStarted;
        }
    }

    private static void setCriticalMonitorStatus(boolean flag) {
        synchronized(sCriticalMonitorLock) {
            sIsCriticalMonitorStarted = flag;
        }
    }

    /**
      * triggerCriticalMonitor method triggers an async task to toggle airplane mode
      * if there is already an ongoing async task CriticalMonitor, return value is false
      * if an ongoing emrgency call, return value is false
      * if async task sucessfully fired, method returns true
      */
    private boolean triggerCriticalMonitor() {
        // check for ongoing emergency call, if no call in progress start a new critical
        // monitor if not already started. if call in progress set the pending critical
        // monitor flag and exit. When emergency call exits, the intent handler checks
        // for this flag and starts a new monitor if needed.
        if (isEmergencyCallOnGoing() == true) {
            setCriticalMonitorPendingStatus(true);
            return false;
        }
        if (getCriticalMonitorStatus() ==  false) {
            setCriticalMonitorStatus(true);
            CriticalStateMonitor monitor = new CriticalStateMonitor();
            monitor.execute();
            return true;
        }
        return false;
    }

    private int getOemAPIVersion() {
        if (mPhoneService == null) return INVALID_VERSION;

        try {
            sApiVersion = mPhoneService.getOemVersion();
            printOemAPIVersion(sApiVersion);
        } catch (RemoteException e) {
            Log.i(TAG, "remote exception while reading API version");
            sApiVersion = INVALID_VERSION;
        }
        return sApiVersion;
    }

    private static void printOemAPIVersion(int ver) {
        String version = null;
        switch (ver) {
            case VERSION_1:
                version = "v1";
                break;
            case VERSION_2:
                version = "v2";
                break;
            case INVALID_VERSION:
            default:
                version = "unknown";
                break;
        }
        Log.i(TAG, "OEM API verison:" + version);
    }
    // Override method
    public void setMaxStates(int state) {
        if (state > ThermalManager.DEFAULT_NUM_ZONE_STATES) {
        int minNumState = ThermalManager.DEFAULT_NUM_ZONE_STATES - 1;
            Log.i(TAG, "capping max states for modem zone to :"
                    + minNumState);
        }
        mMaxStates = ThermalManager.DEFAULT_NUM_ZONE_STATES;
    }

    private final class ShutDownReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            // Turn off airplane mode before shutdown if it was on because of thermal condition.
            // This is to prevent booting with airplane mode against user's wish.
            // If thermal condition did not improve on next boot, thermal service will
            // anyway turn on airplane mode again.
            if (getCriticalMonitorStatus()) {
                setAirplaneMode(false);
                sShutDownInProgress = true;
                Log.i(TAG, "Turning off airplane mode before shutdown...");
            }
        }
    }

    // Modem specific sensor IDs
    public void updateSensorID(ThermalSensor ts) {
        int sensorID = -1;
        String sensorName = ts.getSensorName();
        if (sensorName == null) return;

        if (sensorName.contains("PCB")) {
            sensorID = OemTelephonyConstants.MODEM_SENSOR_ID_PCB;
        } else if (sensorName.contains("RF")) {
            sensorID = OemTelephonyConstants.MODEM_SENSOR_ID_RF;
        } else if (sensorName.contains("BB")) {
            sensorID = OemTelephonyConstants.MODEM_SENSOR_ID_BASEBAND_CHIP;
        } else if (sensorName.contains("PMU")) {
            sensorID = OemTelephonyConstants.MODEM_SENSOR_ID_PMU;
        } else if (sensorName.contains("PA")) {
            sensorID = OemTelephonyConstants.MODEM_SENSOR_ID_PA;
        }
        ts.setSensorID(sensorID);
    }

    public void updateSensorName(ThermalSensor ts) {
        String sensorName = ts.getSensorName();
        if (sensorName == null) return;
        if (sensorName.contains("PCB")) {
            sensorName = OemTelephonyConstants.MODEM_SENSOR_PCB;
        } else if (sensorName.contains("RF")) {
            sensorName = OemTelephonyConstants.MODEM_SENSOR_RF;
        } else if (sensorName.contains("BB")) {
            sensorName = OemTelephonyConstants.MODEM_SENSOR_BB;
        } else if (sensorName.contains("PMU")) {
            sensorName = OemTelephonyConstants.MODEM_SENSOR_PMU;
        } else if (sensorName.contains("PA")) {
            sensorName = OemTelephonyConstants.MODEM_SENSOR_PA;
        }
        ts.setSensorName(sensorName);
    }
}
