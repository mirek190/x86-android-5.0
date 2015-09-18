/*
 * Copyright © 2012 Intel Corporation
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Lin A Jia <lin.a.jia@intel.com>
 *
 */
package com.intel.hdmi;

import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.content.Intent;
import android.preference.CheckBoxPreference;
import android.preference.DialogPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.util.Log;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.media.AudioManager;
import android.content.ContentResolver;
import android.database.ContentObserver;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.telephony.TelephonyManager;

import com.intel.multidisplay.DisplaySetting;
import com.intel.hdmi.OverscanPreference;
/**
 * Settings activity for HDMI settings and status information.
 * This is also the main entry point of HDMI in the application grid.
 */
public class HDMISettings extends PreferenceActivity
    implements OnPreferenceChangeListener
{
    private static final String TAG = "HdmiSettings";

    /** Keys for the settings */
    private static final String KEY_MODE = "hdmi_mode";
    private static final String KEY_HDMI_STATUS = "hdmi_status";
    private static final String KEY_SCALE = "hdmi_scale";
    private static final String KEY_OVERSCAN = "hdmi_overscan";

    /** define information type: width, height, refresh, mArrInterlace, mArrRatio */
    private int[] mArrWidth = null;
    private int[] mArrHeight = null;
    private int[] mArrRefresh = null;
    private int[] mArrInterlace = null;
    private int[] mArrRatio = null;
    private String[] mInfoString = null;

    /** record hdmi connected status */
    // 0: not external display, DisplaySetting.EDP_HDMI, DisplaySetting.EDP_DVI
    private int mEdpStatus = 0;
    private int mScaleType = 3;
    private int mWidth = 0;
    private int mHeight = 0;
    private int mRefresh = 0;
    private int mHDMIModeNum = 5;
    private final BroadcastReceiver mReceiver = new intentHandler();
    private boolean mHasInComingCall = false;
    private boolean mInComingCallFinished = true;
    private boolean mAllowModeSet = true;

    /** preference */
    private Preference mEdpStatusPref;
    private ListPreference mScalePref;
    private ListPreference mModePref;
    private OverscanPreference mOsPref;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        IntentFilter intentFilter = new IntentFilter(DisplaySetting.MDS_EDP_HOTPLUG);
        intentFilter.addAction(DisplaySetting.MDS_HDMI_INFO);
        intentFilter.addAction(DisplaySetting.MDS_HDMI_INFO);
        intentFilter.addAction(TelephonyManager.ACTION_PHONE_STATE_CHANGED);
        intentFilter.addAction(DisplaySetting.MDS_ALLOW_MODE_SET);

        registerReceiver(mReceiver, intentFilter);
        addPreferencesFromResource(R.xml.hdmi_settings);

        mEdpStatusPref = (Preference)findPreference(KEY_HDMI_STATUS);
        if (mEdpStatusPref == null) return;
        mEdpStatusPref.setEnabled(false);
        mEdpStatusPref.setSummary(R.string.hdmi_status_summary_off);

        mScalePref = (ListPreference)findPreference(KEY_SCALE);
        if (mScalePref == null) return;
        mScalePref.setOnPreferenceChangeListener(this);
        mScalePref.setDependency(KEY_HDMI_STATUS);

        mOsPref = (OverscanPreference)findPreference(KEY_OVERSCAN);
        if (mOsPref == null) return;
        mOsPref.setDependency(KEY_HDMI_STATUS);
        mModePref = (ListPreference)findPreference(KEY_MODE);
        if (mModePref == null) return;
        /** init mode control*/
        CharSequence[] mEntries = null;
        CharSequence[] mValue = null;
        mEntries = new CharSequence[1];
        mEntries[0] = "0*0:0";
        mValue= new CharSequence[]{"0"};
        mModePref.setEntries(mEntries);
        mModePref.setEntryValues(mValue);
        restoreHDMISettingInfo();
        mModePref.setDependency(KEY_HDMI_STATUS);
        mModePref.setOnPreferenceChangeListener(this);
        Intent outIntent = new Intent(DisplaySetting.MDS_GET_HDMI_INFO);
        sendBroadcast(outIntent);
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mReceiver);
   }

    /**
     * From OnPreferenceChangeListener
     */
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        // TODO Auto-generated method stub
        final String key = preference.getKey();
        mOsPref.setEnabled(true);
        if (KEY_SCALE.equals(key)) {
        /** Scale control be click */
            int type = Integer.parseInt(newValue.toString());
            mScaleType = type;
            Bundle bundle = new Bundle();
            bundle.putInt("Type", type);
            Intent intent = new Intent(DisplaySetting.MDS_SET_HDMI_SCALE);
            intent.putExtras(bundle);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            sendBroadcast(intent);

            Log.i(TAG,"Scale Type: " + type);
            /** Disable center setp adjust */
            if (type == 2)
            {
                mOsPref.setEnabled(false);
            }
        } else {
            if (mArrWidth == null || mArrHeight == null ||
                    mArrRefresh == null || mArrInterlace == null || mArrRatio == null)
                return false;
            /** Mode contrl click */
            int index = Integer.parseInt(newValue.toString());
            if (index >= mArrWidth.length)
                return false;
            mWidth = mArrWidth[index];
            mHeight = mArrHeight[index];
            mRefresh = mArrRefresh[index];
            int mInterlace = mArrInterlace[index];
            int mRatio = mArrRatio[index];
            Bundle bundle = new Bundle();
            bundle.putInt("width", mWidth);
            bundle.putInt("height", mHeight);
            bundle.putInt("refresh", mRefresh);
            bundle.putInt("interlace", mInterlace);
            bundle.putInt("ratio", mRatio);
            Intent intent = new Intent(DisplaySetting.MDS_SET_HDMI_MODE);
            intent.putExtras(bundle);
            sendBroadcast(intent);

            /**Reset scale mode and overscan compensation value*/
            mModePref.setValueIndex(0);
            mOsPref.SetChange(true);
            mOsPref.setEnabled(true);
            mScalePref.setValueIndex(0);
            mScaleType = 3;
        }
        return true;
    }

    /**
     * Init mode infomation from hdmiObserver
    **/
    private void UpdateInfo(String[] info, ListPreference target) {
        // TODO Auto-generated method stub
        if (mEdpStatus > 0) {
            CharSequence[] mEntries = new CharSequence[info.length];
            CharSequence[] mValue = new CharSequence[info.length];
            for(int i=0; i < info.length; i++)
            {
                mEntries[i] = info[i];
                mValue[i] = Integer.toString(i);
                Log.i(TAG, "mode["+ i+ "]:"+ info[i]);
            }
            target.setEntries(mEntries);
            target.setEntryValues(mValue);
        }
    }

    private void clearStatus() {
        Log.i(TAG, "clear Status");
        mScalePref.setValueIndex(0);
        mOsPref.SetChange(true);
        mModePref.setValueIndex(-1);
    }

    private void storeHDMISettingInfo(int count) {
        SharedPreferences preferences =
            mModePref.getContext().getSharedPreferences(
                    "com.intel.hdmi_preferences", Context.MODE_MULTI_PROCESS);
        Editor editor = preferences.edit();
        editor.putInt("count", count);
        for (int j = 0; j < count; j++) {
            String str = "mInfoString" + Integer.toString(j);
            editor.putString(str, mInfoString[j]);
        }
        editor.commit();
    }

    private void restoreHDMISettingInfo() {
        String[] infoStr;
        SharedPreferences preferences =
            mModePref.getContext().getSharedPreferences(
                    "com.intel.hdmi_preferences", Context.MODE_MULTI_PROCESS);
        int count = 0;
        if (preferences != null) {
            count = preferences.getInt("count", mHDMIModeNum);
            infoStr = new String[count];
            CharSequence[] mmEntries = new CharSequence[count];
            CharSequence[] mmValue = new CharSequence[count];
            for (int i = 0; i < count; i++) {
                 String str = "mInfoString" + Integer.toString(i);
                 infoStr[i] = preferences.getString(str, "0");
                 mmEntries[i] = infoStr[i];
                 mmValue[i] = Integer.toString(i);
            }
            mModePref.setEntries(mmEntries);
            mModePref.setEntryValues(mmValue);
        }
    }

    private class intentHandler extends BroadcastReceiver {
            @Override
            public void onReceive(Context context, Intent intent) {
            // TODO Auto-generated method stub
            Log.i(TAG, "Receive intent: " + intent.toString());
            String action = intent.getAction();
            if (action.equals(DisplaySetting.MDS_EDP_HOTPLUG)) {
                int type = intent.getIntExtra("type", 0);
                int state = intent.getIntExtra("state", 0);
                Log.i(TAG,"Display " + type + " connection " + state + "," + mEdpStatus);
                if(state == 0) {
                    mEdpStatus = 0;
                    Dialog scalePreDialog = mScalePref.getDialog();
                    Dialog modePreDialog = mModePref.getDialog();
                    Dialog osPreDialog = mOsPref.getDialog();
                    if (scalePreDialog != null) {
                        scalePreDialog.dismiss();
                    }
                    if (modePreDialog != null) {
                        modePreDialog.dismiss();
                    }
                    if (osPreDialog != null) {
                        osPreDialog.dismiss();
                    }
                    mEdpStatusPref.setEnabled(false);
                    mEdpStatusPref.setSummary(R.string.hdmi_status_summary_off);
                } else {
                    mEdpStatus = type;
                    Intent outIntent = new Intent(DisplaySetting.MDS_GET_HDMI_INFO);
                    context.sendBroadcast(outIntent);
                    if (mAllowModeSet)
                        mEdpStatusPref.setEnabled(true);
                    else
                        mEdpStatusPref.setEnabled(false);
                    mEdpStatusPref.setSummary(R.string.hdmi_status_summary_on);
                }
            } else if (action.equals(TelephonyManager.ACTION_PHONE_STATE_CHANGED)) {
                if (TelephonyManager.EXTRA_STATE == null ||
                            TelephonyManager.EXTRA_STATE_RINGING == null)
                    return;
                String extras = intent.getStringExtra(TelephonyManager.EXTRA_STATE);
                if (extras == null)
                    return;
                if (extras.equals(TelephonyManager.EXTRA_STATE_RINGING)) {
                    mInComingCallFinished = false;
                    mHasInComingCall = true;
                } else if (extras.equals(TelephonyManager.EXTRA_STATE_IDLE)) {
                    mInComingCallFinished = true;
                    mHasInComingCall = false;
                } else
                    return;
                Log.i(TAG, "incoming call is " +
                        (mHasInComingCall == true ? "initiated" : "terminated"));

                if (mHasInComingCall) {
                    mEdpStatusPref.setEnabled(false);
                    mEdpStatus = 0;
                    mEdpStatusPref.setSummary(R.string.hdmi_status_summary_off);
                } else {
                    if ((mEdpStatus > 0)
                            && ((!mHasInComingCall) && mInComingCallFinished)) {
                        if (mAllowModeSet)
                            mEdpStatusPref.setEnabled(true);
                        else
                            mEdpStatusPref.setEnabled(false);
                        mEdpStatusPref.setSummary(R.string.hdmi_status_summary_on);
                    }
                }
            } else if (action.equals(DisplaySetting.MDS_HDMI_INFO)) {
                // release previous allocated source
                mArrWidth = null;
                mArrHeight = null;
                mArrRefresh = null;
                mArrInterlace = null;
                mArrRatio = null;
                mInfoString = null;
                /** Get mode infomation from MDS*/
                Bundle extras = intent.getExtras();
                if (extras == null)
                    return;
                int count = extras.getInt("count");
                if (count <= 0)
                    return;
                int eDPlugIn = extras.getInt("eDPlugin");// HDMI or DVI
                // TODO: bootStatus is a strange variable
                int bootStatus = extras.getInt("bootStatus");
                mHasInComingCall = extras.getBoolean("hasIncomingCall");
                mAllowModeSet = extras.getBoolean("allowModeSet");
                Log.i(TAG, "Hdmi mode count:" + count +
                        ",Hdmi status:"+ mEdpStatus + " ,incoming call:" +
                        mHasInComingCall + " ,Boot status " +
                        bootStatus + ", plugin:" + eDPlugIn + ", allow mode set: " + mAllowModeSet);
                // A corner case for EDP hotplug in boot process
                if (mEdpStatus == 0 &&
                        !mHasInComingCall && mInComingCallFinished) {
                    Log.i(TAG, "Find an ignored HDMI plugin event");
                    mEdpStatus = eDPlugIn;
                    if (mAllowModeSet)
                        mEdpStatusPref.setEnabled(true);
                    else
                        mEdpStatusPref.setEnabled(false);
                    mEdpStatusPref.setSummary(R.string.hdmi_status_summary_on);
                }

                mInfoString = new String[count];
                mArrWidth = new int[count];
                mArrWidth = (int[]) extras.getSerializable("width");
                mArrHeight = new int[count];
                mArrHeight = (int[]) extras.getSerializable("height");
                mArrRefresh = new int[count];
                mArrRefresh = (int[]) extras.getSerializable("refresh");
                mArrInterlace = new int[count];
                mArrInterlace = (int[]) extras.getSerializable("interlace");
                mArrRatio = new int[count];
                mArrRatio = (int[]) extras.getSerializable("ratio");
                if (mArrWidth == null || mArrHeight == null ||
                        mArrRefresh == null || mArrInterlace == null || mArrRatio == null)
                    return;

                for (int i = 0; i < count; i++) {
                    mInfoString[i] = mArrWidth[i] + "*" + mArrHeight[i];
                    if (mArrInterlace[i] != 0)
                        mInfoString[i] += "I";
                    else
                        mInfoString[i] += "P";

                    mInfoString[i] += "@" + mArrRefresh[i]+ "Hz";
                    if (mArrRatio[i] == 1)
                        mInfoString[i] += " [4:3]";
                    else if (mArrRatio[i] == 2)
                        mInfoString[i] += " [16:9]";
                }

                if (mHasInComingCall) {
                    mEdpStatus = 0;
                    mEdpStatusPref.setEnabled(false);
                } else {
                    if (mEdpStatus > 0) {
                        if (mAllowModeSet)
                            mEdpStatusPref.setEnabled(true);
                        else
                            mEdpStatusPref.setEnabled(false);
                        mEdpStatusPref.setSummary(R.string.hdmi_status_summary_on);
                    }
                }

                UpdateInfo(mInfoString, mModePref);
                if (bootStatus == 1 || eDPlugIn > 0) {
                    mModePref.setValueIndex(0);
                    mOsPref.SetChange(true);
                    mOsPref.setEnabled(true);
                    mScalePref.setValueIndex(0);
                    mScaleType = 3;
                } else {
                    mScaleType = Integer.parseInt(mScalePref.getValue());
                }
                Log.i(TAG, "Scale type: " + mScaleType);
                if (mScaleType == 2)
                    mOsPref.setEnabled(false);
                else
                    mOsPref.setEnabled(true);
                String value = mModePref.getValue();
                if(value != null) {
                    int index = Integer.parseInt(value);
                    if (index >= count) {
                        Log.w(TAG, "Out of array boundary, " + value + ", index " + index);
                        return;
                    }
                    mWidth = mArrWidth[index];
                    mHeight = mArrHeight[index];
                    mRefresh = mArrRefresh[index];
                }
                Log.i(TAG, "Now Mode is: " + mWidth + "x" + mHeight + "@" + mRefresh + "Hz");
                storeHDMISettingInfo(count);
            } else if (action.equals(DisplaySetting.MDS_ALLOW_MODE_SET)) {
                Bundle extras = intent.getExtras();
                if(extras == null) {
                    Log.e(TAG, "Try to dereference null");
                    return;
                }
                mAllowModeSet = extras.getBoolean("allowModeSet");
                Log.i(TAG, "allow Mode Set : "+ mAllowModeSet + ", edp: " + mEdpStatus);
                if (mAllowModeSet && mEdpStatus > 0 && !mHasInComingCall)
                    mEdpStatusPref.setEnabled(true);
                else
                    mEdpStatusPref.setEnabled(false);
            } // MDS_HDMI_INFO
        }//onRecive
    }//intentHandler
}

