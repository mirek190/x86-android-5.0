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


import java.util.ArrayList;
import java.util.List;

import android.os.Parcel;
import android.os.Parcelable;

public class OemTftInfo implements Parcelable {
    public int mCid;
    public int mPCid;
    public String mIface;

    public List<TftInfoItem> mTftInfoItems;

    public OemTftInfo() {
        mTftInfoItems = new ArrayList<TftInfoItem>();
    }

    public OemTftInfo(Parcel p) {
        mCid = p.readInt();
        mPCid = p.readInt();
        mIface = p.readString();
        mTftInfoItems = new ArrayList<TftInfoItem>();
        p.readList(mTftInfoItems, TftInfoItem.class.getClassLoader());
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
        dest.writeList(mTftInfoItems);
    }

    /**
     * Parcelable creator
     */
    public static final Parcelable.Creator<OemTftInfo> CREATOR =
            new Parcelable.Creator<OemTftInfo>() {
        public OemTftInfo createFromParcel(Parcel source) {
            return new OemTftInfo(source);
        }
        public OemTftInfo[] newArray(int size) {
            return new OemTftInfo[size];
        }
    };
}
