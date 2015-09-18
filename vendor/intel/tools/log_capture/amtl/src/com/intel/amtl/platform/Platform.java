/* Android AMTL
 *
 * Copyright (C) Intel 2013
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
 * Author: Morgane Butscher <morganeX.butscher@intel.com>
 * Author: Erwan Bracq <erwan.bracq@intel.com>
 */
package com.intel.amtl.platform;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.SystemProperties;
import android.util.Log;

import com.intel.amtl.AMTLApplication;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.FileInputStream;

public class Platform {

    private final String TAG = AMTLApplication.getAMTLApp().getLogTag();
    private String catalogPath = "/system/etc/telephony/";

    public String getPlatformConf() {
        String property = AMTLApplication.getAMTLApp().getProperty();
        return catalogPath + "amtl_" + SystemProperties.get(property, "") + ".cfg";
    }
}
