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

import android.os.Parcel;

/** @hide */
public abstract class ParentUserInfo {

    public static final int FLAG_CONTAINER   = 0x80000000;

    public int containerOwner = -4;

    public boolean isContainer() {
        return false;
    }

    public ParentUserInfo() {
    }

    public ParentUserInfo(ParentUserInfo orig) {
    }

    public void writeToParcel(Parcel dest, int parcelableFlags) {
    }

    protected ParentUserInfo(Parcel source) {
    }
}
