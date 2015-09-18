/* Android AMTL
 *
 * Copyright (C) Intel 2014
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
 * Author: Damien Charpentier <damienx.charpentier@intel.com>
 */

package com.intel.amtl;

import com.intel.amtl.R;
import com.intel.internal.telephony.ModemStatusManager;

import android.app.Application;
import android.content.Context;
import android.util.Log;

public class AMTLApplication extends Application {
    private static Context ctx;
    private static boolean isCloseTtyEnable = true;
    private static boolean isPauseState = true;
    private static AMTLApplication sAMTLApp = null;

    private int mAppId = 1;
    private String mProperty = "";
    private String mModemEvent = "";
    private String mTtyFileName = "";
    private ModemStatusManager mdmStatusManager = null;
    private String mAppName = "";
    private String mdmConfClassName = "";
    private String mdmctlClassName = "";
    private String mLogTag = "";
    private String mShowAppLabel = "";

    @Override
    public void onCreate() {
        super.onCreate();
        try {
            init(this);
        } catch (InstantiationException e) {
            Log.e(mLogTag, "AMTLApplication: init error + " + e.toString());
        }
    }

    public static AMTLApplication getAMTLApp() {
        return sAMTLApp;
    }

    private void init(AMTLApplication app) throws InstantiationException {
        sAMTLApp = app;
        mAppName = app.getString(R.string.app_name);
        mAppId = app.getResources().getInteger(R.integer.application_id);
        mProperty = app.getString(R.string.property);
        mModemEvent = app.getString(R.string.modem_event);
        mTtyFileName = app.getString(R.string.tty_file_name);
        mdmConfClassName = app.getString(R.string.modem_conf_class_name);
        mdmctlClassName = app.getString(R.string.modem_control_class_name);
        mLogTag = app.getString(R.string.log_tag);
        mShowAppLabel = app.getString(R.string.show_app_label);
        mdmStatusManager = ModemStatusManager.getInstance(mAppId);
    }

    public String getAppName() {
        return mAppName;
    }

    public String getModemConClassName() {
        return mdmConfClassName;
    }

    public String getModemCtlClassName() {
        return mdmctlClassName;
    }

    public String getProperty() {
        return mProperty;
    }

    public String getModemEvent() {
        return mModemEvent;
    }

    public String getLogTag() {
        return mLogTag;
    }

    public String getShowAppLabel() {
        return mShowAppLabel;
    }

    public ModemStatusManager getModemStatusManager() throws InstantiationException {
        return mdmStatusManager;
    }

    public String getTtyFileName() {
        return mTtyFileName;
    }

    public static Context getContext() {
        return ctx;
    }

    public static void setContext(Context context) {
        ctx = context;
    }

    public static boolean getCloseTtyEnable() {
        return isCloseTtyEnable;
    }

    public static void setCloseTtyEnable(boolean isEnable) {
        isCloseTtyEnable = isEnable;
    }

    public static boolean getPauseState() {
        return isPauseState;
    }

    public static void setPauseState(boolean bState) {
        isPauseState = bState;
    }
}
