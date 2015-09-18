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

import android.content.ContentResolver;
import android.content.Context;

import android.os.SystemProperties;
import android.os.Vibrator;

import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL;

import android.util.Log;

import android.widget.Toast;

import java.util.ArrayList;

/**
 * The BrightnessControl class implements throttling of
 * display brightness at various levels.
 *
 * @hide
 */
public class BrightnessControl {
    private static final String TAG = "Thermal:BrightnessControl";
    private static final String MSG = "Display brightness throttled since the device is warm";
    // Brightness range is 0 to 255 as per official android documentation
    private static final int MAX_BRIGHTNESS = 255;
    private static boolean sThrottled = false;
    private static int sMode = 0xDEADBEEF;
    private static int sBrightness;
    private static Context sContext = null;
    private static ArrayList<Integer> sValues = new ArrayList<Integer>();

    public static void throttleDevice(int thermalState) {
        int brightness, curBrightness, curMode;
        if (sContext == null) return;
        ContentResolver cr = sContext.getContentResolver();
        try {
            curMode = Settings.System.getInt(cr, Settings.System.SCREEN_BRIGHTNESS_MODE);
            if (thermalState > 0) {
                // Disable adaptive brightness if it is not disabled before
                if (curMode != SCREEN_BRIGHTNESS_MODE_MANUAL) {
                    Settings.System.putInt(cr,
                        SCREEN_BRIGHTNESS_MODE, SCREEN_BRIGHTNESS_MODE_MANUAL);
                    // Save current mode to revert to it when normal
                    sMode = curMode;
                }
                // Get current brightness setting
                curBrightness = Settings.System.getInt(cr, Settings.System.SCREEN_BRIGHTNESS);
                // Get the brightness throttle value wrt current state
                brightness = sValues.get(thermalState) * MAX_BRIGHTNESS / 100;
                // Throttle brightness only if throttle value is lesser than current value
                if (brightness < curBrightness) {
                    // Save the user set brightness value before throttling for the first time
                    if (!sThrottled) {
                        sBrightness = curBrightness;
                        sThrottled = true;
                    }
                    Settings.System.putInt(cr, Settings.System.SCREEN_BRIGHTNESS, brightness);
                        // Vibrate for half a second if configured to vibrate
                    if (SystemProperties.get("persist.thermal.display.vibra", "0").equals("1")) {
                        Vibrator vibrator = (Vibrator)
                                sContext.getSystemService(Context.VIBRATOR_SERVICE);
                        vibrator.vibrate(500);
                    }
                    // Show toast message if configured to show message
                    if (SystemProperties.get("persist.thermal.display.msg", "0").equals("1")) {
                        Toast.makeText(sContext, MSG, Toast.LENGTH_LONG).show();
                    }
                }
            } else {
                // On normal state, revert to the original settings
                if (sMode != 0xDEADBEEF && sMode != curMode) {
                    Settings.System.putInt(cr, Settings.System.SCREEN_BRIGHTNESS_MODE, sMode);
                }
                if (sThrottled) {
                    Settings.System.putInt(cr, Settings.System.SCREEN_BRIGHTNESS, sBrightness);
                }
                // Reset the saved settings on normal state
                sMode = 0xDEADBEEF;
                sThrottled = false;
            }
        } catch(SettingNotFoundException e) {
            Log.e(TAG, "Caught " + e);
        }
    }

    public static void init(Context context, String path, ArrayList<Integer> values) {
        sContext = context;
        sValues = values;
    }
}
