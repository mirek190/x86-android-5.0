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

package com.intel.arkham;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import android.util.Slog;

import com.android.systemui.recent.RecentTasksLoader;
import com.android.systemui.recent.TaskDescription;

public class ExtendRecentTasksLoader extends RecentTasksLoader {
    private static final String TAG = "ExtendRecentTasksLoader";

    public ExtendRecentTasksLoader(Context context) {
        super(context);
    }

    protected ContainerInfo getContainer(int userId) {
        long token = Binder.clearCallingIdentity();
        ContainerInfo container = null;
        try {
            IBinder b = ServiceManager.getService(ContainerConstants.CONTAINER_MANAGER_SERVICE);
            IContainerManager containerService = IContainerManager.Stub.asInterface(b);
            // Only add calling user's recent tasks
            // Also add existing containers' recent tasks, if userId is the container owner
            if (containerService != null)
                container = containerService.getContainerFromCid(userId);
        } catch (RemoteException e) {
            Slog.e(TAG, "getContainer: failed talking with ContainerManagerService: ", e);
        } finally {
            Binder.restoreCallingIdentity(token);
        }
        return container;
    }

    @Override
    protected void loadThumbnailAndIcon(TaskDescription _td) {
        ExtendTaskDescription td = (ExtendTaskDescription) _td;
        final ActivityManager am = (ActivityManager)
                mContext.getSystemService(Context.ACTIVITY_SERVICE);
        final PackageManager pm = mContext.getPackageManager();
        Bitmap thumbnail = am.getTaskTopThumbnail(td.getPersistentTaskId());
        Drawable icon = getFullResIcon(td.getResolveInfo().activityInfo.applicationInfo, pm);
        Drawable containerIcon = null;
        String containerPackageName = td.getContainerPackageName();

        /* ARKHAM-276 - Use icon to identify container activities
         * If this activity is running inside a container, use the container default
         * package name (returned by ContainerManagerService), to display the container
         * icon on top of the application icon (this icon will be smaller so that it won't
         * cover the application icon)
         */
        if (containerPackageName != null) {
            Log.d(TAG, "Loading container icon from: " + containerPackageName);
            ApplicationInfo info = null;
            try {
                info = pm.getApplicationInfo(containerPackageName, 0);
            } catch (NameNotFoundException ex) {
                Log.e(TAG, "PackageName for container icon not found: " + containerPackageName, ex);
            }
            if (info == null) {
                Log.e(TAG, "Could not get ApplicationInfo!");
            } else {
                containerIcon = getFullResIcon(info, pm);
            }
        }

        if (DEBUG) Log.v(TAG, "Loaded bitmap for task "
                + td + ": " + thumbnail);
        synchronized (td) {
            if (thumbnail != null) {
                td.setThumbnail(new BitmapDrawable(mContext.getResources(), thumbnail));
            } else {
                td.setThumbnail(mDefaultThumbnailBackground);
            }
            if (icon != null) {
                td.setIcon(icon);
            }
            if (containerIcon != null) {
                td.setContainerIcon(containerIcon);
            }
            td.setLoaded(true);
        }
    }

    private Drawable getFullResIcon(ApplicationInfo info, PackageManager packageManager) {
        Resources resources;
        try {
            resources = packageManager.getResourcesForApplication(info);
        } catch (PackageManager.NameNotFoundException e) {
            resources = null;
        }
        if (resources != null && info.icon != 0) {
            return getFullResIcon(resources, info.icon);
        }
        return getFullResDefaultActivityIcon();
    }

}
