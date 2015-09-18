
package com.intel.internal.telephony.OemTelephony;

import android.os.Parcel;
import android.os.Parcelable;
import java.net.InetAddress;

public class TftInfoItem implements Parcelable {
    /** packet filter Id */
    public int mPfId;

    /** evaluation precedence Index */
    public int mEpIdx;

    /** source address and mask */
    public InetAddress mSourceAddr;
    public InetAddress mSourceMask;

    /** protocol number / next header */
    public int mPNumber;

    /**
     * destination port range f.t
     */
    public String mDstRng;

    /**
     * source port range f.t
     */
    public String mSrcRng;

    /** ipsec security param index (spi) */
    public int mSpi;

    /** tos and mask */
    public String mTosAndMask;

    /** Flow label */
    public int mFlowLabel;

    /**
     * direction 1 Uplink 2 Downlink 3 both
     */
    public int mDirection;

    /** Network packet filter identifier */
    public int mNwId;

    public TftInfoItem() {
    }

    public TftInfoItem(Parcel p) {
        this.mPfId = p.readInt();
        this.mEpIdx = p.readInt();
        this.mSourceAddr = InetAddress.parseNumericAddress(p.readString());
        this.mSourceMask = InetAddress.parseNumericAddress(p.readString());
        this.mPNumber = p.readInt();
        this.mDstRng = p.readString();
        this.mSrcRng = p.readString();
        this.mSpi = p.readInt();
        this.mTosAndMask = p.readString();
        this.mFlowLabel = p.readInt();
        this.mDirection = p.readInt();
        this.mNwId = p.readInt();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mPfId);
        dest.writeInt(mEpIdx);
        dest.writeString(mSourceAddr.getHostAddress());
        dest.writeString(mSourceMask.getHostAddress());
        dest.writeInt(mPNumber);
        dest.writeString(mDstRng);
        dest.writeString(mSrcRng);
        dest.writeInt(mSpi);
        dest.writeString(mTosAndMask);
        dest.writeInt(mFlowLabel);
        dest.writeInt(mDirection);
        dest.writeInt(mNwId);
    }

    /**
     * Parcelable creator
     */
    public static final Parcelable.Creator<TftInfoItem> CREATOR =
            new Parcelable.Creator<TftInfoItem>() {
        public TftInfoItem createFromParcel(Parcel source) {
            return new TftInfoItem(source);
        }

        public TftInfoItem[] newArray(int size) {
            return new TftInfoItem[size];
        }
    };
}
