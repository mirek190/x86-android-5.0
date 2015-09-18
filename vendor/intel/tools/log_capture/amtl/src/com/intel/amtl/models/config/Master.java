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
 * Author: Erwan Bracq <erwan.bracq@intel.com>
 * Author: Morgane Butscher <morganex.butscher@intel.com>
 */

package com.intel.amtl.models.config;

import com.intel.amtl.AMTLApplication;

import android.util.Log;

public class Master {

    private final String TAG = AMTLApplication.getAMTLApp().getLogTag();
    private final String MODULE = "Master";
    private String name = "";
    private String defaultPort = "";
    private String defaultConf = "";

    public Master() {
    }

    public Master(String name, String defaultPort, String defaultConf) {
        this.setName(name);
        this.setDefaultPort(defaultPort);
        this.setDefaultConf(defaultConf);
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setDefaultPort(String defaultPort) {
        this.defaultPort = defaultPort;
    }

    public void setDefaultConf(String defaultConf) {
        this.defaultConf = defaultConf;
    }

    public String getName() {
        return this.name;
    }

    public String getDefaultPort() {
        return this.defaultPort;
    }

    public String getDefaultConf() {
        return this.defaultConf;
    }

    public void printToLog() {
        Log.d(TAG, MODULE + ": name = " + name + ", defaultPort = " + defaultPort
                + ", defaultConf = " + defaultConf + ".");
    }
}
