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
import android.content.pm.UserInfo;
import android.widget.FrameLayout;
import com.android.keyguard.KeyguardViewMediator;
import com.android.keyguard.KeyguardHostView.OnDismissAction;
import com.android.internal.widget.LockPatternUtils;
import java.util.List;

public class ContainerPolicyCommons {

    public ContainerPolicyCommons(Context context, LockPatternUtils lockPatternUtils) {
    }

    public int setContainerKeyguardFeatures(int mDisabledFeatures) {
        return mDisabledFeatures;
    }

    public void editKeyguardForContainer(FrameLayout mFrameLayout) {
    }

    public void removeContainerUser(List<UserInfo> users) {
    }

    public OnDismissAction handleContainerKeyguardDismiss(KeyguardViewMediator.ViewMediatorCallback
            mViewMediatorCallback, OnDismissAction mDismissAction) {
        return mDismissAction;
    }

    public void launchHome() {
    }

    public static CharSequence updateCarrierText(Context mContext,
            LockPatternUtils mLockPatternUtils, CharSequence text) {
        return text;
    }

    public boolean allowKeyguardDisable() {
        return true;
    }
}
