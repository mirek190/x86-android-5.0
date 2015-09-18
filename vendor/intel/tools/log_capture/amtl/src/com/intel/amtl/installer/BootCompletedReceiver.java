/* Android AMTL
 *
 * Copyright (C) 2014 Intel Corporation, All Rights Reserved.
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
 *
 * Author: Damien Charpentier <damienx.charpentier@intel.com>
 */

package com.android.amtl.installer;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.SystemProperties;
import android.util.Log;
import com.intel.amtl.AMTLApplication;
import com.intel.amtl.R;
/**
 * Boot completed receiver. used to reset the app install state every time the
 * device boots.
 *
 */
public class BootCompletedReceiver extends BroadcastReceiver {

    private String mLogTag = null;
    private final static String key = "persist.radio.multisim.config";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        mLogTag = AMTLApplication.getAMTLApp().getLogTag();
        Log.d(mLogTag, " BootCompletedReceiver");
        String show = AMTLApplication.getAMTLApp().getShowAppLabel();

        // make sure the app icon is removed every time the device boots.
        if (action.equals(Intent.ACTION_BOOT_COMPLETED)) {
            String value = SystemProperties.get(key);
            Log.d(mLogTag, "multi sim config: " + value);
            if (show.equals("0")) {
                unInstall(context);
            } else if (show.equals("1")) {
                install(context);
            } else {
                if(value.equals("dsda")) {
                    install(context);
                } else {
                    unInstall(context);
                }
            }
        }
    }

    private void install(Context context) {
        setAppState(context, true);
    }

    private void unInstall(Context context) {
        setAppState(context, false);
    }

    private void setAppState(Context context, boolean install) {
        if (context == null) {
            return;
        }
        PackageManager pm = context.getPackageManager();
        if (pm == null) {
            return;
        }
        // check that AMTL app package is known to the PackageManager
        ComponentName cName = new ComponentName(context.getPackageName(),
                "com.intel.amtl.gui.AMTLTabLayout");
        int state = install ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED
                : PackageManager.COMPONENT_ENABLED_STATE_DISABLED;

        try {
            pm.setComponentEnabledSetting(cName, state, PackageManager.DONT_KILL_APP);
        } catch (Exception e) {
            Log.d(mLogTag, "Could not change " + AMTLApplication.getAMTLApp().getAppName()
                    + " app state: " + e.getMessage());
        }
    }
}
