/*
 * Copyright (C) 2013 Intel Corporation, All Rights Reserved
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

package com.intel.internal.telephony.OemTelephony;

import android.os.Parcel;
import android.os.Parcelable;

public class OemSrvccSyncResult implements Parcelable {
    public int mCallId;
    public int mTransferResult;

    public OemSrvccSyncResult(int callId, int transferResult) {
        this.mCallId = callId;
        this.mTransferResult = transferResult;
    }

    public OemSrvccSyncResult(Parcel p) {
        mCallId = p.readInt();
        mTransferResult = p.readInt();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mCallId);
        dest.writeInt(mTransferResult);
    }

    /**
     * Parcelable creator
     */
    public static final Parcelable.Creator<OemSrvccSyncResult> CREATOR =
            new Parcelable.Creator<OemSrvccSyncResult>() {
        public OemSrvccSyncResult createFromParcel(Parcel source) {
            return new OemSrvccSyncResult(source);
        }

        public OemSrvccSyncResult[] newArray(int size) {
            return new OemSrvccSyncResult[size];
        }
    };
}
