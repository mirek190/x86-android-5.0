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


import com.intel.android.nvmconfig.controllers.ModemControlPipe;
import com.intel.android.nvmconfig.controllers.ModemController;
import com.intel.android.nvmconfig.exceptions.ModemControlException;
import com.intel.android.nvmconfig.models.modem.at.ATControlCommand;
import com.intel.android.nvmconfig.models.modem.at.ATControlResponse;
import com.intel.internal.telephony.MmgrClientException;
import com.intel.internal.telephony.ModemEventListener;
import com.intel.internal.telephony.ModemNotification;
import com.intel.internal.telephony.ModemNotificationArgs;
import com.intel.internal.telephony.ModemStatus;
import com.intel.internal.telephony.ModemStatusManager;

/**
 * @author emarmouX
 *
 */
public class MedfieldModemController implements
        ModemController<ATControlCommand, ATControlResponse>,
        ModemEventListener {

    public static final int MSG_ERROR = 1;

    protected ModemControlPipe<ATControlCommand, ATControlResponse> pipe = null;
    protected ModemStatusManager modemStatusManager = null;
    protected ModemStatus currentModemStatus = ModemStatus.NONE;

    public MedfieldModemController()
            throws ModemControlException {

        try {
            this.modemStatusManager = ModemStatusManager.getInstance();
        } catch (InstantiationException ex) {
            throw new ModemControlException(
                    "Cannot instantiate Modem Status Manager", ex);
        }
        try {
            this.modemStatusManager.subscribeToEvent(this, ModemStatus.ALL,
                    ModemNotification.ALL);
        } catch (MmgrClientException ex) {
            throw new ModemControlException(
                    "Cannot subscribe to Modem Status Manager", ex);
        }
        try {
            this.modemStatusManager.connect("NVM_Config");
        } catch (MmgrClientException ex) {
            throw new ModemControlException(
                    "Cannot connect to Modem Status Manager", ex);
        }
        this.pipe = new MedfieldModemControlPipe();
    }

    public ATControlResponse writeRawCommand(ATControlCommand command)
            throws ModemControlException {

        if (command != null) {

            if (this.currentModemStatus != ModemStatus.UP) {
                throw new ModemControlException(String.format(
                        "The modem is not ready. Status = %s",
                        this.currentModemStatus));
            }
            return this.pipe.writeToModemControl(command);
        }
        return null;
    }

    public void close() throws IOException {
        if (this.pipe != null) {
            this.pipe.close();
            this.pipe = null;
        }
        if (this.modemStatusManager != null) {
            this.modemStatusManager.disconnect();
            this.modemStatusManager = null;
        }
    }

    @Override
    public void onModemColdReset(ModemNotificationArgs args) {
    }

    @Override
    public void onModemShutdown(ModemNotificationArgs args) {
    }

    @Override
    public void onPlatformReboot(ModemNotificationArgs args) {
    }

    @Override
    public void onModemCoreDump(ModemNotificationArgs args) {
    }

    @Override
    public void onModemUp() {
        this.currentModemStatus = ModemStatus.UP;
    }

    @Override
    public void onModemDown() {
        this.currentModemStatus = ModemStatus.DOWN;
    }

    @Override
    public void onModemDead() {
        this.currentModemStatus = ModemStatus.DEAD;
    }
}
