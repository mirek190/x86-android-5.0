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

public class OemQosInfo implements Parcelable {
    public int mCid;
    public int mPCid;
    public String mIface;
    public int mQci;
    public int mDlGbr;
    public int mUlGbr;
    public int mDlMbr;
    public int mUlMbr;

    public OemQosInfo(Parcel p) {
        this.mCid = p.readInt();
        this.mPCid = p.readInt();
        this.mIface = p.readString();
        this.mQci = p.readInt();
        this.mDlGbr = p.readInt();
        this.mUlGbr = p.readInt();
        this.mDlMbr = p.readInt();
        this.mUlMbr = p.readInt();
    }

    public OemQosInfo() {
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mCid);
        dest.writeInt(mPCid);
        dest.writeString(mIface);
        dest.writeInt(mQci);
        dest.writeInt(mDlGbr);
        dest.writeInt(mUlGbr);
        dest.writeInt(mDlMbr);
        dest.writeInt(mUlMbr);
    }

    /**
     * Parcelable creator
     */
    public static final Parcelable.Creator<OemQosInfo> CREATOR =
            new Parcelable.Creator<OemQosInfo>() {
        public OemQosInfo createFromParcel(Parcel source) {
            return new OemQosInfo(source);
        }

        public OemQosInfo[] newArray(int size) {
            return new OemQosInfo[size];
        }
    };
}
