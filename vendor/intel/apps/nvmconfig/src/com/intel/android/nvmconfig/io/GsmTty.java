/* Android Modem NVM Configuration Tool
 *
 * Copyright (C) Intel 2012
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
 * Author: Edward Marmounier <edwardx.marmounier@intel.com>
 */

package com.intel.android.nvmconfig.io;

import android.util.Log;

// This class is needed to be able to call the native library built by libnvmconfig_jni
// This lib create the gsmTTy port to talk to the modem
// DO NOT REFACTOR THE PACKAGE NAME, or you'll break the JNI call mechanism.
public class GsmTty {

    protected int ttySerialFd = -1;
    protected int baudrate = 115200;
    protected String ttyName = "";

    static {
        System.loadLibrary("nvmconfig_jni");
    }

    protected native int OpenSerial(String jtty_name, int baudrate);

    protected native int CloseSerial(int fd);

    public GsmTty(String ttyName, int baudrate) {

        this.ttyName = ttyName;
        this.baudrate = baudrate;
    }

    public void openTty() {
        /* Check if ttyName is already open */
        if (this.ttySerialFd < 0) {
            /* Not open -> open it */
            this.ttySerialFd = this.OpenSerial(this.ttyName, this.baudrate);
            if (this.ttySerialFd < 0) {
                Log.e("GsmTty", "openTty() failed");
            } else {
                Log.d("GsmTty", "openTty() succeeded");
            }
        } else {
            Log.d("GsmTty", "openTty() port already open");
        }
    }

    public void closeTty() {
        this.CloseSerial(this.ttySerialFd);
        this.ttySerialFd = -1;
    }
}
