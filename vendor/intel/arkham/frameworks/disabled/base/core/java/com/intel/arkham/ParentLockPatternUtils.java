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

import android.content.Context;
import android.os.UserHandle;

import com.android.internal.widget.LockPatternView;

import java.util.List;

/** @hide */
public abstract class ParentLockPatternUtils {
    // ARKHAM - 596, isContainerUserMode - setting false will show primary user's keyguard.
    protected static volatile boolean isContainerUserMode = false;
    // ARKHAM - 596, sContainerUserId - is set by PhoneWindowManager.
    protected static volatile int sContainerUserId = UserHandle.USER_NULL;

    protected ParentLockPatternUtils (Context context) {
    }

    protected boolean isContainerUser(int userId) {
        return false;
    }
    protected boolean isContainerSystemDataMounted() {
        return false;
    }

    public boolean checkPassword(String password, int userId) {
        return false;
    }

    public boolean checkPattern(List<LockPatternView.Cell> pattern, int userId) {
        return false;
    }

    protected boolean changeContainerLockPassword(int cid, boolean isFallback, Object password,
            int quality, boolean isPattern) {
        return true;
    }

    protected long getLong(String secureSettingKey, long defaultValue, int userHandle) {
        return defaultValue;
    }

    public boolean isContainerUserMode() {
        return false;
    }

    protected int getsContainerUserId() {
        return -1;
    }
}
