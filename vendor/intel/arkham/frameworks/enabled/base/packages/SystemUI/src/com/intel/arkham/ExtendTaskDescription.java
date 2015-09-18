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

import android.content.Intent;
import android.content.pm.ResolveInfo;
import android.graphics.drawable.Drawable;

import com.android.systemui.recent.TaskDescription;

public final class ExtendTaskDescription extends TaskDescription {

    // ARKHAM-276 - Use icon to identify container activities
    private Drawable mContainerIcon; // container icon

    /* ARKHAM-326 - Restart stopped container activities as container user
     * If the task is a container activity, hold a copy of it's ContainerInfo
     * so we can later use the information to restart the activity as it's container user
     */
    private ContainerInfo mContainerInfo;

    public ExtendTaskDescription(int _taskId, int _persistentTaskId,
            ResolveInfo _resolveInfo, Intent _intent,
            String _packageName, CharSequence _description, ContainerInfo _containerInfo) {
        super(_taskId, _persistentTaskId, _resolveInfo, _intent, _packageName, _description);

        // ARKHAM-326 - Restart stopped container activities as container user
        mContainerInfo = _containerInfo;
    }

    public ExtendTaskDescription() {
        super();
        mContainerInfo = null;
    }


    // ARKHAM-276 - Use icon to identify container activities
    @Override
    public String getContainerLabel() {
        return (mContainerInfo != null) ? mContainerInfo.getContainerName() : null;
    }

    // ARKHAM-276 - Use icon to identify container activities
    @Override
    public Drawable getContainerIcon() {
        return mContainerIcon;
    }

    // ARKHAM-326 - Restart stopped container activities as container user
    // We need the container id when restarting a container activity
    @Override
    public int getContainerId() {
        return (mContainerInfo != null) ? mContainerInfo.getContainerId() : 0;
    }

    // ARKHAM-276 - Use icon to identify container activities
    public String getContainerPackageName() {
        return (mContainerInfo != null) ? mContainerInfo.getAdminPackageName() : null;
    }

    // ARKHAM-276 - Use icon to identify container activities
    public void setContainerIcon(Drawable icon) {
        mContainerIcon = icon;
    }

    int getPersistentTaskId() {
        return persistentTaskId;
    }

    ResolveInfo getResolveInfo() {
        return resolveInfo;
    }
}
