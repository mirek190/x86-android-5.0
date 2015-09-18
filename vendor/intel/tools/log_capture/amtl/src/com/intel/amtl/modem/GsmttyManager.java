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
import com.intel.amtl.exceptions.ModemControlException;

import java.io.Closeable;
import java.io.IOException;
import java.io.RandomAccessFile;

import android.util.Log;

public class GsmttyManager implements Closeable {

    private final String TAG = AMTLApplication.getAMTLApp().getLogTag();
    private final String MODULE = "GsmttyManager";
    private RandomAccessFile file = null;
    private Gsmtty gsmtty = null;

    public GsmttyManager() throws ModemControlException {

        try {
            this.gsmtty = new Gsmtty(this.getTtyFileName(), 115200);
            this.gsmtty.openTty();
            this.file = new RandomAccessFile(this.getTtyFileName(), "rw");
        } catch (ExceptionInInitializerError ex) {
            throw new ModemControlException("libamtl_jni library was not found " + ex);
        } catch (IOException ex) {
            throw new ModemControlException("Error while opening gsmtty " + ex);
        } catch (IllegalArgumentException ex) {
            throw new ModemControlException("Error while opening gsmtty" + ex);
        }
    }

    private String getTtyFileName() {
        return AMTLApplication.getAMTLApp().getTtyFileName();
    }

    public void writeToModemControl(String atCommand) throws ModemControlException {

        try {
            this.file.writeBytes(atCommand);
            Log.i(TAG, MODULE + ": sending to modem " + atCommand);
        }
        catch (IOException ex) {
            throw new ModemControlException("Unable to send to AT command to the modem.");
        }
    }

    public String readFromModemControl() throws ModemControlException {

        String atResponse = "";
        byte[] responseBuffer = new byte[1024];

        try {
            int readCount = this.file.read(responseBuffer);
            if (readCount >= 0) {
                atResponse = new String(responseBuffer, 0, readCount);
                Log.i(TAG, MODULE + " : response from modem " + atResponse);
            } else {
                throw new ModemControlException("Unable to read response from the modem.");
            }
        }
        catch (IOException ex) {
            throw new ModemControlException("Unable to read response from the modem.");
        }
        return atResponse;
    }

    public void close() {
        if (this.file != null) {
            try {
                this.file.close();
                this.file = null;
            }
            catch (IOException ex) {
                Log.e(TAG, MODULE + ex.toString());
            } finally {
                if (this.gsmtty != null) {
                    this.gsmtty.closeTty();
                    this.gsmtty = null;
                }
            }
        }
    }
}
