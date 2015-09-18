/*
 * Copyright (C) 2014 Intel Corporation, All Rights Reserved
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

public class AdaptiveClockingFrequencyInfo implements Parcelable {
    /**
     * The center frequency of the channel number in Hz. This is host
     * receiver channel frequency.
     */
    public long mCenterFrequency;

    /**
     * The frequency spread of the channel in Hz. This is host receiver
     * channel frequency spread.
     */
    public int mFrequencySpread;

    /**
     * The noise power referred at antenna in dBm, at the reported center
     * frequency, and integrated over bandwidth equal to reported frequency
     * spread.
     */
    public int mNoisePower;

    public AdaptiveClockingFrequencyInfo(long centerFrequency,
                int frequencySpread, int noisePower) {
        this.mCenterFrequency = centerFrequency;
        this.mFrequencySpread = frequencySpread;
        this.mNoisePower = noisePower;
    }

    public AdaptiveClockingFrequencyInfo(Parcel p) {
        this.mCenterFrequency = p.readLong();
        this.mFrequencySpread = p.readInt();
        this.mNoisePower = p.readInt();
    }

    public AdaptiveClockingFrequencyInfo() {
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeLong(mCenterFrequency);
        dest.writeInt(mFrequencySpread);
        dest.writeInt(mNoisePower);
    }

    /**
     * Parcelable creator
     */
    public static final Parcelable.Creator<AdaptiveClockingFrequencyInfo> CREATOR =
                new Parcelable.Creator<AdaptiveClockingFrequencyInfo>() {
        public AdaptiveClockingFrequencyInfo createFromParcel(Parcel source) {
            return new AdaptiveClockingFrequencyInfo(source);
        }

        public AdaptiveClockingFrequencyInfo[] newArray(int size) {
            return new AdaptiveClockingFrequencyInfo[size];
        }
    };
}
