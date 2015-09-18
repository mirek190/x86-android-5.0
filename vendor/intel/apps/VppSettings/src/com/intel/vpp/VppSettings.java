/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.intel.vpp;

import android.app.Activity;

import android.content.SharedPreferences;
import android.content.Intent;

import android.os.SystemProperties;

import android.os.Bundle;

import android.util.Log;

import android.view.MenuItem;
import android.view.View;

import android.widget.CompoundButton;
import android.widget.RelativeLayout;
import android.widget.Switch;
import android.widget.TextView;
//import com.intel.multidisplay.DisplaySetting;

public class VppSettings extends Activity implements CompoundButton.OnCheckedChangeListener {

    private static final String TAG = "VppSettings";

    private static final String VPP_SHARED_PREF = "vpp_settings";
    private static final String FRC_STATUS = "frc_status";
    private static final String[] FRC_STATUS_VALUE = {"0frc", "1frc"};
    private static final String VPP_STATUS = "vpp_status";
    private static final String[] VPP_STATUS_VALUE = {"0vpp", "1vpp"};

    private int mStatus;
    private SharedPreferences mSharedPref;
    private Switch mFrcSwitch;
    private Switch mVppSwitch;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        getActionBar().setDisplayHomeAsUpEnabled(true);

        setContentView(R.layout.act_settings);

        int showFRC = 0;
        try {
            showFRC = SystemProperties.getInt("vppsettings.frc", 0);
        } catch (Exception e) {
            Log.e(TAG, "invalid vppsettings.frc value");
            showFRC = 0;
        }
        if (showFRC == 0) {
            RelativeLayout frcLayout = (RelativeLayout)findViewById(R.id.frc_layout);
            if (frcLayout != null)
                frcLayout.setVisibility(View.GONE);

            TextView vppDesc = (TextView)findViewById(R.id.vpp_desc);
            if (vppDesc != null)
                vppDesc.setText(getString(R.string.vpp_desc_gen));
        }

        mFrcSwitch = (Switch)findViewById(R.id.frc_switcher);
        mVppSwitch = (Switch)findViewById(R.id.vpp_switcher);
        mSharedPref = getSharedPreferences(VPP_SHARED_PREF, Activity.MODE_WORLD_READABLE);
    }


    @Override
    public void onResume() {
        super.onResume();
        try {
            mStatus = Integer.valueOf(mSharedPref.getString(FRC_STATUS, FRC_STATUS_VALUE[0]).substring(0, 1)).intValue();
        } catch (Exception e) {
            mStatus = 0;
        }
        if (mFrcSwitch != null) {
            mFrcSwitch.setOnCheckedChangeListener(this);
            mFrcSwitch.setChecked(mStatus > 0);
        }

        try {
            mStatus = Integer.valueOf(mSharedPref.getString(VPP_STATUS, VPP_STATUS_VALUE[0]).substring(0, 1)).intValue();
        } catch (Exception e) {
            mStatus = 0;
        }
        if (mVppSwitch != null) {
            mVppSwitch.setOnCheckedChangeListener(this);
            mVppSwitch.setChecked(mStatus > 0);
        }

    }

    @Override
    public void onPause() {
        super.onPause();
        if (mFrcSwitch != null)
            mFrcSwitch.setOnCheckedChangeListener(null);

        if (mVppSwitch != null)
            mVppSwitch.setOnCheckedChangeListener(null);
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        mStatus = isChecked ? 1 : 0;
        if (buttonView == mFrcSwitch) {
            SharedPreferences.Editor editor = mSharedPref.edit();
            editor.putString(FRC_STATUS, FRC_STATUS_VALUE[mStatus]).commit();
        } else if (buttonView == mVppSwitch) {
            SharedPreferences.Editor editor = mSharedPref.edit();
            editor.putString(VPP_STATUS, VPP_STATUS_VALUE[mStatus]).commit();

            /*
            Bundle bundle = new Bundle();
            bundle.putInt("status", mStatus);
            Intent intent = new Intent(DisplaySetting.MDS_SET_VPP_STATUS);
            intent.putExtras(bundle);
            sendBroadcast(intent);
            */
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        switch (id) {
            case android.R.id.home:
                finish();
                break;
        }
        return true;
    }
}
