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

import java.net.InetAddress;

public class PcscfInfo implements Parcelable {
    public int mCid;
    public InetAddress[] mPcscfAddresses;

    public PcscfInfo() {
        mCid = -1;
        mPcscfAddresses = new InetAddress[0];
    }

    public PcscfInfo(Parcel p) {
        mCid = p.readInt();
        int addrCount = p.readInt();
        mPcscfAddresses = new InetAddress[addrCount];
        for (int i = 0; i < addrCount; i++) {
            mPcscfAddresses[i] = InetAddress.parseNumericAddress(p.readString());
        }
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mCid);
        dest.writeInt(mPcscfAddresses.length);
        for (int i = 0; i < mPcscfAddresses.length; i++) {
            dest.writeString(mPcscfAddresses[i].getHostAddress());
        }
    }

    /**
     * Parcelable creator
     */
    public static final Parcelable.Creator<PcscfInfo> CREATOR =
            new Parcelable.Creator<PcscfInfo>() {
                public PcscfInfo createFromParcel(Parcel source) {
                    return new PcscfInfo(source);
                }

                public PcscfInfo[] newArray(int size) {
                    return new PcscfInfo[size];
                }
            };

    public String toString() {
        StringBuilder sb = new StringBuilder(64).append("mCid= ").append(mCid);
        for (InetAddress addr : mPcscfAddresses) {
            sb.append(addr).append("\n");
        }
        return sb.toString();
    }
}
