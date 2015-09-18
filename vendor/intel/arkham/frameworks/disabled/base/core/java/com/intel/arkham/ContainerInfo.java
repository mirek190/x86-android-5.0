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
import android.os.Parcelable;

public class ContainerInfo implements Parcelable {

    public ContainerInfo(Parcel source) {

    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public String getContainerName() {
        return null;
    }

    public int getContainerId() {
        return -4;
    }

    public String getAdminPackageName() {
        return null;
    }

    public static final Parcelable.Creator<ContainerInfo> CREATOR =
            new Parcelable.Creator<ContainerInfo>() {

                @Override
                public ContainerInfo createFromParcel(Parcel source) {
                    return new ContainerInfo(source);
                }

                @Override
                public ContainerInfo[] newArray(int size) {
                    return new ContainerInfo[size];
                }
            };
}
