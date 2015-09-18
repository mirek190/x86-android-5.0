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

import com.android.systemui.recent.TaskDescription;

public final class ExtendTaskDescription extends TaskDescription {

    public ExtendTaskDescription(int _taskId, int _persistentTaskId,
            ResolveInfo _resolveInfo, Intent _intent,
            String _packageName, CharSequence _description, ContainerInfo _containerInfo) {
        super(_taskId, _persistentTaskId, _resolveInfo, _intent, _packageName, _description);
    }

    public ExtendTaskDescription() {
        super();
    }
}
