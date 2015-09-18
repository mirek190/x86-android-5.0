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

import android.app.admin.DevicePolicyManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.UserInfo;
import android.graphics.drawable.Drawable;
import android.os.UserHandle;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout.LayoutParams;
import com.android.internal.R;
import com.android.keyguard.KeyguardViewMediator;
import com.android.keyguard.KeyguardHostView.OnDismissAction;
import com.android.internal.widget.LockPatternUtils;
import java.util.Iterator;
import java.util.List;

public class ContainerPolicyCommons {

    LockPatternUtils mLockPatternUtils;
    Context mContext;

    public ContainerPolicyCommons(Context context, LockPatternUtils lockPatternUtils) {
        mContext = context;
        mLockPatternUtils = lockPatternUtils;
    }

    public int setContainerKeyguardFeatures(int mDisabledFeatures) {
        // ARKHAM-544, do not show widgets for container unlock screen
        if (mLockPatternUtils.isContainerUserMode()) {
            mDisabledFeatures = mDisabledFeatures |
                DevicePolicyManager.KEYGUARD_DISABLE_WIDGETS_ALL |
                DevicePolicyManager.KEYGUARD_DISABLE_SECURE_CAMERA;
        }
        return mDisabledFeatures;
    }

    public void editKeyguardForContainer(FrameLayout mFrameLayout) {
        // ARKHAM-676 START: If isContainerUserMode, add container's icon
        // on top/left of the screen
        if (mLockPatternUtils.isContainerUserMode()) {
            final PackageManager pm = mContext.getPackageManager();
            ContainerManager cm = new ContainerManager(mContext);
            ContainerInfo containerInfo = cm
                    .getContainerFromCid(mLockPatternUtils.getCurrentUser());
            if (containerInfo != null) {
                String containerPackage = containerInfo.getAdminPackageName();
                try {
                    Drawable containerIcon = pm
                            .getApplicationIcon(containerPackage);
                    ImageView logo = new ImageView(mContext);
                    logo.setImageDrawable(containerIcon);
                    logo.setLayoutParams(new FrameLayout.LayoutParams(
                            LayoutParams.WRAP_CONTENT,
                            LayoutParams.WRAP_CONTENT));
                    mFrameLayout.addView(logo, 0);
                } catch (NameNotFoundException e) {
                    e.printStackTrace();
                }
            }
        }
        // ARKHAM-676 END
    }

    public void removeContainerUser(List<UserInfo> users) {
        // ARKHAM-89 Hide container users in the lock screen
        // Don't display container users in the user switcher.
        Iterator<UserInfo> it = users.iterator();
        while (it.hasNext())
            if (it.next().isContainer())
                it.remove();
    }

    public boolean allowKeyguardDisable() {
        if (mLockPatternUtils!=null && mLockPatternUtils.isContainerUserMode()) {
            DevicePolicyManager dpm = (DevicePolicyManager) mContext.getSystemService(
                    Context.DEVICE_POLICY_SERVICE);
            if (dpm != null) {
                return (dpm.getPasswordQuality(null, mLockPatternUtils.getCurrentUser())
                        == DevicePolicyManager.PASSWORD_QUALITY_UNSPECIFIED);
            }
        }
        return true;
    }

    public OnDismissAction handleContainerKeyguardDismiss(KeyguardViewMediator.ViewMediatorCallback
            mViewMediatorCallback, OnDismissAction mDismissAction) {
        // ARKHAM-596, allow container keyguard to be cancelled.
        if (mLockPatternUtils.isContainerUserMode()) {
            boolean deferKeyguardDone = false;
            if (mDismissAction != null) {
                deferKeyguardDone = mDismissAction.onDismiss();
                mDismissAction = null;
            }
            if (mViewMediatorCallback != null) {
                if (deferKeyguardDone) {
                    mViewMediatorCallback.keyguardDonePending();
                } else {
                    mViewMediatorCallback.keyguardDone(false);
                }
            }
        }
        return mDismissAction;
        // ARKHAM-596 Ends.
    }

    public void launchHome() {
        // ARKHAM-596, when back pressed start the launcher.
        // TODO: Instead fix Activity Stack and end up with previous activity.
        if(mLockPatternUtils.isContainerUserMode()) {
            Intent mHomeIntent =  new Intent(Intent.ACTION_MAIN, null);
            mHomeIntent.addCategory(Intent.CATEGORY_HOME);
            mHomeIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                    | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
            mContext.startActivityAsUser(mHomeIntent, UserHandle.CURRENT);
        }
        // ARKHAM-596 Ends.
    }

    public static CharSequence updateCarrierText(Context mContext,
            LockPatternUtils mLockPatternUtils, CharSequence text) {
        // ARKHAM-676 START: If isContainerUserMode, change carrier text to an
        // unlock custom message
        if (mLockPatternUtils.isContainerUserMode()) {
            final PackageManager pm = mContext.getPackageManager();
            ContainerManager cm = new ContainerManager(mContext);
            ContainerInfo containerInfo = cm
                    .getContainerFromCid(mLockPatternUtils.getCurrentUser());
            if (containerInfo != null) {
                text = mContext.getString(
                        R.string.container_unlock_message,
                        containerInfo.getContainerName());
            }
        }
        return text;
        // ARKHAM-676 END
    }
}
