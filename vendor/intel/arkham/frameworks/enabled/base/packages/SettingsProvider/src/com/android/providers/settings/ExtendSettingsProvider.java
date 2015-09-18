/*
 * Copyright (C) 2013 Intel Corporation
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

package com.android.providers.settings;


import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.UserInfo;

import android.os.FileObserver;
import android.os.UserHandle;
import android.os.UserManager;
import android.util.Slog;

import java.util.concurrent.atomic.AtomicInteger;


public class ExtendSettingsProvider extends SettingsProvider {

    private static final String TAG = "ExtendSettingsProvider";
    public class ExtendSettingsFileObserver extends SettingsFileObserver {

        public ExtendSettingsFileObserver(int userHandle, String path) {
            super(userHandle, path);
        }

        // ARKHAM-733: START
        @Override
        public void onEvent(int event, String path) {
            // synchronize this with onUserStopping
            synchronized (ExtendSettingsProvider.this) {
                // check for null value (onUserStopping might delete it)
                AtomicInteger mutationCount = sKnownMutationsInFlight.get(mUserHandle);
                if (mutationCount == null)
                    return;
                super.onEvent(event, path);
            }
        }
    }

    @Override
    public boolean onCreate() {
        boolean ret = super.onCreate();

        // ARKHAM-773: close settings.db when user is stopped
        IntentFilter userStopFilter = new IntentFilter();
        userStopFilter.addAction(Intent.ACTION_USER_STOPPING);
        userStopFilter.setPriority(IntentFilter.SYSTEM_LOW_PRIORITY);
        getContext().registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (intent.getAction().equals(Intent.ACTION_USER_STOPPING)) {
                    final int userHandle = intent.getIntExtra(Intent.EXTRA_USER_HANDLE,
                            UserHandle.USER_OWNER);
                    if (userHandle != UserHandle.USER_OWNER) {
                        onUserStopping(userHandle);
                    }
                }
            }
        }, userStopFilter);

        return ret;
    }


    @Override
    protected SettingsFileObserver newSettingsFileObserver(int userHandle, String path) {
        return new ExtendSettingsFileObserver(userHandle, path);
    }


    /*
     * ARKHAM-773: close settings.db when container user is stopped
     */
    void onUserStopping(int userHandle) {
        // close settings.db only for container users
        UserInfo userInfo = mUserManager.getUserInfo(userHandle);
        if (userInfo == null || !userInfo.isContainer())
            return;

        synchronized (this) {
            FileObserver observer = sObserverInstances.get(userHandle);
            if (observer != null) {
                observer.stopWatching();
                sObserverInstances.delete(userHandle);
            }

            DatabaseHelper dbhelper =  mOpenHelpers.get(userHandle);
            Slog.w(TAG, "Inside Setting Providers: delete db helper and caches for user "
                    + userHandle);
            if (dbhelper != null) {
                dbhelper.close();
            }
            mOpenHelpers.delete(userHandle);
            sSystemCaches.delete(userHandle);
            sSecureCaches.delete(userHandle);
            sKnownMutationsInFlight.delete(userHandle);
        }
    }

}
