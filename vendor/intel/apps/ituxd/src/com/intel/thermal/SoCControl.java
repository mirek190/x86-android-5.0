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

import java.io.File;
import java.util.ArrayList;
/**
 * The SoC Control modifies the Power Limit for Turbo
 *
 * @hide
 */
public class SoCControl {
    private static final String TAG = "Thermal:SoCControl";
    private static String sSoCThrottlePath = null;

    private static void setThrottlePath() {
        int indx = ThermalUtils.getCoolingDeviceIndexContains("SoC");

        if (indx != -1) {
            sSoCThrottlePath = ThermalManager.sCoolingDeviceBasePath + indx
                    + ThermalManager.sCoolingDeviceState;
        } else {
            // look up failed.
            sSoCThrottlePath = null;
        }
    }

    public static void throttleDevice(int thermalState) {
        // Override the thermalState variable to indicate Dynamic Turbo is disabled.
        if (!ThermalManager.sIsDynamicTurboEnabled) {
            thermalState = ThermalManager.DISABLE_DYNAMIC_TURBO;
        }

        if (sSoCThrottlePath != null) {
            ThermalUtils.writeSysfs(sSoCThrottlePath, thermalState);
        }
    }

    public static void init(Context context, String path, ArrayList<Integer> values) {
        // If 'path' is 'auto' enumerate from Sysfs
        if (path.equalsIgnoreCase("auto")) {
            setThrottlePath();
        // If 'path' is neither 'auto' nor a null, the given path _is_ the Sysfs path
        } else if (path != null) {
            sSoCThrottlePath = path;
        // None of the above cases. Set the throttle path to null
        } else {
            sSoCThrottlePath = null;
        }
    }
}
