/* Android Modem Traces and Logs
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
 */

package com.intel.amtl.modem;

import com.intel.amtl.AMTLApplication;

import android.util.Log;

public class Gsmtty {

    private final String TAG = AMTLApplication.getAMTLApp().getLogTag();
    private final String MODULE = "Gsmtty";
    private int ttySerialFd = -1;
    private int baudrate = 115200;
    private String ttyName = "";
    private String mNativeTag = TAG;

    /* Load Amtl JNI library */
    static {
        System.loadLibrary("amtl_jni");
    }

    private native int OpenSerial(String jtty_name, int baudrate);

    private native int CloseSerial(int fd);

    public Gsmtty(String ttyName, int baudrate) {
        this.ttyName = ttyName;
        this.baudrate = baudrate;
    }

    public void openTty() {
        /* Check if ttyName is already open */
        if (this.ttySerialFd < 0) {
            /* Not open -> open it */
            this.ttySerialFd = this.OpenSerial(this.ttyName, this.baudrate);
            if (this.ttySerialFd < 0) {
                Log.e(TAG, MODULE + " : openTty() failed " + this.ttyName);
            } else {
                Log.d(TAG, MODULE + " : openTty() succeeded " + this.ttyName);
            }
        } else {
            Log.d(TAG, MODULE +  " : openTty() port already open " + this.ttyName);
        }
    }

    public void closeTty() {
        Log.d(TAG, MODULE + " : closeTty() ongoing " + this.ttyName);
        this.CloseSerial(this.ttySerialFd);
        this.ttySerialFd = -1;
    }
}
