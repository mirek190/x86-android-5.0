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

import android.app.ActivityManager;
import android.content.Context;
import android.util.Log;

import java.util.List;

/**
 * The ThermalAppAgent class implements methods to throttle the
 * performance of Apps during platform thermal conditions.
 *
 * @hide
 */
public class ThermalAppAgent {

    private static final String TAG = "ThermalAppAgent";

    private static void killApp(Context context, String appName) {
        boolean isKilled = false;
        ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        // Search to see if this 'app' is in the list of Apps running.
        // If found, kill this app using its PID.
        List<ActivityManager.RunningAppProcessInfo> listOfProcesses = am.getRunningAppProcesses();
        for (int i = 0; i < listOfProcesses.size(); i++) {
            ActivityManager.RunningAppProcessInfo proc = listOfProcesses.get(i);
            if (proc.processName.equalsIgnoreCase(appName)) {
                Log.i(TAG, "Killing App: " + appName + " with PID: " + proc.pid);
                android.os.Process.killProcess(proc.pid);
                isKilled = true;
                break;
            }
        }
        if (!isKilled) {
            Log.i(TAG, "Could not find app: " + appName);
        }
    }

    public static void throttleApp(Context context, String appName, String action) {
        if (action.equals(ThermalManager.ACTION_KILL)) {
            killApp(context, appName);
        } else {
            // TODO: Implement other actions if any (like pause, move to background)
            Log.i(TAG, "Invalid action tag");
        }
    }
}
