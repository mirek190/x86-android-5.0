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

package com.intel.android.nvmconfig.controllers;

import com.intel.android.nvmconfig.models.modem.ModemControlCommand;
import com.intel.android.nvmconfig.models.modem.ModemControlCommandList;
import com.intel.android.nvmconfig.models.modem.ModemControlResponse;
import com.intel.android.nvmconfig.models.modem.ModemControlResponseList;

import android.os.AsyncTask;

public class AsyncCommandExecutor<C extends ModemControlCommand, R extends ModemControlResponse> extends AsyncTask<ModemControlCommandList<C>, String, ModemControlResponseList<R>> {

    protected ModemController<C, R> modemController = null;
    protected Exception lastException = null;
    protected AsyncTaskEventListener<R> eventListener = null;

    public AsyncCommandExecutor(AsyncTaskEventListener<R> eventListener, ModemController<C, R> modemController) {
        if (modemController == null) {
            throw new IllegalArgumentException("modemController");
        }
        this.modemController = modemController;
        if (eventListener == null) {
            throw new IllegalArgumentException("eventListener");
        }
        this.eventListener = eventListener;
    }

    @Override
    protected ModemControlResponseList<R> doInBackground(ModemControlCommandList<C>... params) {

        ModemControlResponseList<R> ret = new ModemControlResponseList<R>();

        try {
            if (params != null && params.length > 0) {
                for (int i = 0; i < params[0].getCommands().size(); i++) {

                    C command = params[0].getCommands().get(i);
                    super.publishProgress("AP SAYS: " + command.getCommand());

                    R response = this.modemController.writeRawCommand(command);
                    if (response != null) {
                        super.publishProgress("BP SAYS: " + response.getResponse());

                        response.validate();
                        ret.getResponses().add(response);
                    }
                    else {
                        super.publishProgress("BP SAYS NOTHING");
                    }
                }
            }
        }
        catch(Exception ex) {
            this.lastException = ex;
        }
        return ret;
    }

    @Override
    protected void onPostExecute(ModemControlResponseList<R> result) {
        super.onPostExecute(result);

        if(this.lastException != null) {
            this.eventListener.onError(this.lastException);
        }
        else {
            this.eventListener.onFinished(result);
        }
    }

    @Override
    protected void onProgressUpdate(String... values) {
        super.onProgressUpdate(values);

        this.eventListener.onProgress(values[0]);
    }

    public interface AsyncTaskEventListener<R extends ModemControlResponse> {

        public void onError(Exception ex);
        public void onFinished(ModemControlResponseList<R> result);
        public void onCancel();
        public void onProgress(String pendingOperation);
    }
}
