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

import android.content.Context;
import android.util.Log;

import java.util.ArrayList;

/**
 * The CameraFlashControl class implements Camera flash throttling using the
 * BCU subsystem's sysfs interfaces.
 *
 * @hide
 */
public class CameraFlashControl {
    private static final String TAG = "Thermal:CameraFlashControl";

    private static final String sCameraFlashThrottleSysfsPath = "/dev/bcu/camflash_ctrl";

    private static int sMaxThrottleValues;

    private static ArrayList<Integer> sValues = null;

    public static void throttleDevice(int tstate) {
        int val;
        Log.d(TAG, "throttleDevice called with" + tstate);
        // check out of bound condition
        if (tstate < 0 || tstate > sMaxThrottleValues - 1) {
            Log.i(TAG, "Camera Flash Control plugin cannot handle state:" + tstate);
            return;
        }

        if (ThermalUtils.isFileExists(sCameraFlashThrottleSysfsPath)) {
            val = (sValues == null) ? tstate : sValues.get(tstate);
            ThermalUtils.writeSysfs(sCameraFlashThrottleSysfsPath, tstate);
        }
    }

    public static void init(Context context, String path, ArrayList<Integer> values) {
        if (values == null) {
            sMaxThrottleValues = ThermalManager.DEFAULT_NUM_THROTTLE_VALUES;
        } else {
            if (values.size() < ThermalManager.DEFAULT_NUM_THROTTLE_VALUES) {
                Log.i(TAG, "Number of values provided for throttling is < : "
                        + ThermalManager.DEFAULT_NUM_THROTTLE_VALUES);
                return;
            }
            sMaxThrottleValues = values.size();
            sValues = new ArrayList<Integer>(values);
        }
    }
}
