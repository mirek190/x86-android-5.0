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

package com.intel.android.nvmconfig.controllers.medfield;

import java.io.IOException;
import java.io.RandomAccessFile;

import android.util.Log;

import com.intel.android.nvmconfig.io.GsmTty;
import com.intel.android.nvmconfig.controllers.ModemControlPipe;
import com.intel.android.nvmconfig.exceptions.ModemControlException;
import com.intel.android.nvmconfig.models.modem.at.ATControlCommand;
import com.intel.android.nvmconfig.models.modem.at.ATControlResponse;

public class MedfieldModemControlPipe implements ModemControlPipe<ATControlCommand, ATControlResponse> {

    protected RandomAccessFile file = null;
    protected GsmTty ttyManager = null;

    public MedfieldModemControlPipe() throws ModemControlException {

        try {
            this.ttyManager = new GsmTty(this.getTtyFileName(), 115200);
            this.ttyManager.openTty();
            this.file = new RandomAccessFile(this.getTtyFileName(), "rw");
        }
        catch (ExceptionInInitializerError ex) {
            throw new ModemControlException("The libnvmconfig_jni library was not found.", ex);
        }
        catch (Exception ex) {
            throw new ModemControlException(String.format("Error while opening %s", this.getTtyFileName()), ex);
        }
    }

    protected String getTtyFileName() {
        return "/dev/gsmtty19";
    }

    public ATControlResponse writeToModemControl(ATControlCommand command) throws ModemControlException {

        ATControlResponse ret = null;
        String atCommand = (String)command.getCommand();
        String atResponse = "";
        byte[] responseBuffer = new byte[1024];

        try {
            this.file.writeBytes(atCommand);
            Log.i("MedfieldModemControlPipe", atCommand);
        }
        catch (IOException ex) {
            throw new ModemControlException("Unable to send to AT command to the modem.", ex);
        }

        try {
            // leave a short time to the modem for giving us a response
            try { Thread.sleep(500); } catch (InterruptedException ex) { Log.e("MedfieldModemControlPipe", ex.toString()); }
            int readCount = this.file.read(responseBuffer);
            atResponse = new String(responseBuffer, 0, readCount);
            ret = new ATControlResponse(atResponse);
            Log.i("MedfieldModemControlPipe", atResponse);
        }
        catch (IOException ex) {
            throw new ModemControlException("Unable to read an AT response from the modem.", ex);
        }
        return ret;
    }

    public void close() {
        if (this.file != null) {
            try {
                this.file.close();
            }
            catch (IOException ex) {
                Log.e("MeldfieldModemControlPipe", ex.toString());
            }
        }
    }
}
