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

public class OemSrvccSyncParam implements Parcelable {
    public int mCallId;
    public int mTid;
    public int mTidFlag;
    public boolean mIsEmergencyCall;
    public int mCallState;
    public int mAuxState;
    public int mMptyAuxState;
    public String mPhoneNo;
    public int mTonNpi;
    public int mPiSi;

    public OemSrvccSyncParam() {
    }

    public OemSrvccSyncParam(Parcel p) {
        this.mCallId = p.readInt();
        this.mTid = p.readInt();
        this.mTidFlag = p.readInt();
        this.mIsEmergencyCall = (p.readInt() != 0);
        this.mCallState = p.readInt();
        this.mAuxState = p.readInt();
        this.mMptyAuxState = p.readInt();
        this.mPhoneNo = p.readString();
        this.mTonNpi = p.readInt();
        this.mPiSi = p.readInt();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mCallId);
        dest.writeInt(mTid);
        dest.writeInt(mTidFlag);
        dest.writeInt(mIsEmergencyCall ? 1 : 0);
        dest.writeInt(mCallState);
        dest.writeInt(mAuxState);
        dest.writeInt(mMptyAuxState);
        dest.writeString(mPhoneNo);
        dest.writeInt(mTonNpi);
        dest.writeInt(mPiSi);
    }

    /**
     * Parcelable creator
     */
    public static final Parcelable.Creator<OemSrvccSyncParam> CREATOR =
            new Parcelable.Creator<OemSrvccSyncParam>() {
        public OemSrvccSyncParam createFromParcel(Parcel source) {
            return new OemSrvccSyncParam(source);
        }

        public OemSrvccSyncParam[] newArray(int size) {
            return new OemSrvccSyncParam[size];
        }
    };
}
