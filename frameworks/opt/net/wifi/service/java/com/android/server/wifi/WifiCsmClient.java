/*
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
 */

package com.android.server.wifi;

import android.content.Context;
import android.os.RemoteException;
import android.util.Slog;
import android.util.Log;

import com.intel.cws.cwsservicemanagerclient.CsmClient;
import com.intel.cws.cwsservicemanager.CsmException;

public class WifiCsmClient extends CsmClient {
    private static final String TAG = "WifiCsmClient";
    private final boolean DBG;

    private final WifiServiceImpl mWifiService;

    private int mUsageCount;

    public WifiCsmClient(Context context, WifiServiceImpl service) throws CsmException {
        super(context, CSM_ID_WIFI, CSM_CLIENT_BIND);
        mUsageCount = 0;
        mWifiService = service;
        DBG = Log.isLoggable(TAG, Log.DEBUG);
        csmActivateSimStatusReceiver();
    }

    /**
     * @hide
     */
    public synchronized void getModem() {
        if (mUsageCount++ == 0) {
            if (DBG) Slog.d(TAG, "Modem lock");
            try {
                this.csmStartModem();
            } catch (CsmException e) {
                Slog.d(TAG, "startAsync error " + e);
            }
        }
    }

    /**
     * @hide
     */
    public synchronized void putModem() {
        if (--mUsageCount == 0) {
            if (DBG) Slog.d(TAG, "Modem unlock");
            this.csmStop();
        } else if (mUsageCount < 0) {
            Slog.e(TAG, "putModem: Unbalanced call");
            mUsageCount = 0;
        }
    }

    @Override
    public void csmClientModemAvailable() {
        // do wifi stuff
        return;
    }

    @Override
    public void csmClientModemUnavailable() {
        // do wifi stuff
        return;
    }

    @Override
    public void onSimLoaded() {
        // do wifi stuff (country code, SIM AKA AKA', coex...)
        return;
    }

    @Override
    public void onSimAbsent() {
        // do wifi stuff
        return;
    }

    public int getWifiSafeChannelBitmap() {
        int bitmap = 0;
        if (mCwsServiceMgr == null) {
            Slog.e(TAG, "getWifiSafeChannelBitmap: mCwsServiceMgr is null");
        } else {
            try {
                bitmap = mCwsServiceMgr.getWifiSafeChannelBitmap();
            } catch (CsmException e) {
                Slog.e(TAG, "getWifiSafeChannelBitmap() error " + e);
            } catch (RemoteException e) {
                Slog.e(TAG, "getWifiSafeChannelBitmap() error " + e);
            }
        }
        return bitmap;
    }
}
