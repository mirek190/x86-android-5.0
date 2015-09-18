/*
 * Copyright © 2012 Intel Corporation
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Lin A Jia <lin.a.jia@intel.com>
 *
 */
package com.intel.hdmi;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.preference.*;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ZoomControls;
import android.widget.ProgressBar;
import android.widget.RadioGroup;
import android.widget.RadioButton;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

import java.io.File;
import com.intel.multidisplay.DisplaySetting;


public class OverscanPreference extends SeekBarDialogPreference {

    private static final String TAG = "HdmiSettings";
    private  ZoomControls mZoomControls;
    private  ProgressBar  mProgressBar;
    private  RadioGroup   mRadioGroup;
    private  RadioButton  mRadioHori;
    private  RadioButton  mRadioVert;

    /** Scale ratio scope */
    private static final int MINIMUM_RATIO = 0;
    private static final int MAXIMUM_RATIO = 5;
    private static final int INIT_STATES = MAXIMUM_RATIO;

    //record current scale ratio
    private int mScaleRatio = 5;
    private int mHoriRatio = 5;
    private int mVertRatio = 5;
    private int mOrientation = 0;
    private boolean mInit = true;
    private boolean mChange = false;

    @Override
    public void createActionButtons() {
        setPositiveButtonText(null);
        setNegativeButtonText(null);
    }

    public OverscanPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setDialogLayoutResource(R.layout.preference_dialog_overscan);

        SharedPreferences preferences =
            this.getContext().getSharedPreferences(
                    "com.intel.hdmi_preferences", Context.MODE_MULTI_PROCESS);
        //Log.i(TAG, "preferences" + this.getContext().getSharedPrefsFile("com.android.hdmi_preferences"));
        //Log.i(TAG,"scaleRatio"+scaleRatio);
        if (preferences != null) {
            mScaleRatio = preferences.getInt("scaleRatio", MAXIMUM_RATIO);
            mHoriRatio = preferences.getInt("horiRatio", MAXIMUM_RATIO);
            mVertRatio = preferences.getInt("vertRatio", MAXIMUM_RATIO);
            mOrientation = preferences.getInt("mOrientation", MAXIMUM_RATIO);
        }
        Log.i(TAG, "scaleRatio: " + mScaleRatio);
    }

    public void SetChange(boolean change) {
        mChange = change;
        mScaleRatio = mHoriRatio = mVertRatio = MAXIMUM_RATIO;
        StoreOverscanInfo();
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);
        mZoomControls = (ZoomControls)view.findViewById(com.intel.hdmi.R.id.zoomControls);
        mProgressBar = (ProgressBar)view.findViewById(com.intel.hdmi.R.id.progressBar);
        mRadioGroup = (RadioGroup)view.findViewById(com.intel.hdmi.R.id.orientation);
        mRadioHori = (RadioButton)view.findViewById(com.intel.hdmi.R.id.horizontal);
        mRadioVert = (RadioButton)view.findViewById(com.intel.hdmi.R.id.vertical);

        if (mZoomControls == null || mProgressBar == null || mRadioGroup == null
            || mRadioHori == null || mRadioVert == null)
            return;

        mProgressBar.setMax(MAXIMUM_RATIO - MINIMUM_RATIO);
        mRadioGroup.setOnCheckedChangeListener(mChangeRadio);

        if (mChange) {
            mInit = true;
            mScaleRatio = mHoriRatio = mVertRatio = MAXIMUM_RATIO;
            mOrientation = 0;
            mChange = false;
        } else
            mInit = false;

        if (mInit) {
            mInit = false;
            mOrientation = 0;
            SetStatus(mHoriRatio);
            mRadioGroup.check(com.intel.hdmi.R.id.horizontal);
        }
        else {
            if (mOrientation == 0) {
                SetStatus(mHoriRatio);
                mRadioGroup.check(com.intel.hdmi.R.id.horizontal);
            }
            else {
                SetStatus(mVertRatio);
                mRadioGroup.check(com.intel.hdmi.R.id.vertical);
            }
        }
        mZoomControls.setOnZoomInClickListener(new OnClickListener() {
            public void onClick(View v) {
                // TODO Auto-generated method stub
                if (mScaleRatio == MINIMUM_RATIO)
                    mZoomControls.setIsZoomOutEnabled(true);
                mScaleRatio++;
                //Log.e(TAG, "vale:" + scaleRatio);
                if (mScaleRatio == MAXIMUM_RATIO)
                    mZoomControls.setIsZoomInEnabled(false);
                mProgressBar.incrementProgressBy(1);
                SendIntent(mScaleRatio);

                if (mOrientation == 0) {
                    mHoriRatio = mScaleRatio;
                    Log.i(TAG, "hori: " + mHoriRatio + "mOrientation: " + mOrientation);
                }
                else {
                    mVertRatio = mScaleRatio;
                    Log.i(TAG, "vert: " + mVertRatio);
                }
                StoreOverscanInfo();
            }
        });

        // zoom out event
        mZoomControls.setOnZoomOutClickListener(new OnClickListener() {
            public void onClick(View v) {
                // TODO Auto-generated method stub
                if (mScaleRatio == MAXIMUM_RATIO)
                    mZoomControls.setIsZoomInEnabled(true);
                mScaleRatio--;
                if (mScaleRatio == MINIMUM_RATIO)
                    mZoomControls.setIsZoomOutEnabled(false);
                mProgressBar.incrementProgressBy(-1);
                SendIntent(mScaleRatio);
                if (mOrientation == 0) {
                    mHoriRatio = mScaleRatio;
                //    Log.i(TAG, "hori:" + horiRatio + "mOrientation:" + mOrientation);
                }
                else {
                    mVertRatio = mScaleRatio;
                    //Log.i(TAG, "vert:" + vertRatio);
                }
                StoreOverscanInfo();
            }
        });
    }

    private RadioGroup.OnCheckedChangeListener mChangeRadio = new
            RadioGroup.OnCheckedChangeListener() {

                public void onCheckedChanged(RadioGroup group, int checkedId) {
                    // TODO Auto-generated method stub
                    if (checkedId == mRadioHori.getId()) {
                        mOrientation = 0;
                        Log.i(TAG, "set mOrientation :" + mOrientation);
                        mScaleRatio = mHoriRatio;
                    }
                    else if (checkedId == mRadioVert.getId()) {
                        mOrientation = 1;
                        Log.i(TAG, "set mOrientation :" + mOrientation);
                        mScaleRatio = mVertRatio;
                    }
                    StoreOverscanInfo();
                    SetStatus(mScaleRatio);
                }
            };

    // send intent to hdmiobersver
    private void SendIntent(int value) {
        //Log.e(TAG, "tao value:" + scaleRatio + "ori:" + mOrientation);
        Bundle bundle = new Bundle();
        bundle.putInt("Step", value);
        bundle.putInt("Orientation", mOrientation);
        Intent intent = new Intent(DisplaySetting.MDS_SET_HDMI_STEP_SCALE);
        Log.i(TAG,"orientation: " + mOrientation);
        Log.i(TAG,"Step: " + value);
        intent.putExtras(bundle);
        this.getContext().sendBroadcast(intent);
    }

    private void SetStatus(int ratio)
    {
        mProgressBar.setProgress(ratio);
        mZoomControls.setIsZoomInEnabled(true);
        mZoomControls.setIsZoomOutEnabled(true);
        if (mProgressBar.getProgress() == MAXIMUM_RATIO) {
            mZoomControls.setIsZoomInEnabled(false);
            Log.i(TAG, "big");
        }
        else if (mProgressBar.getProgress() == MINIMUM_RATIO)
            mZoomControls.setIsZoomOutEnabled(false);
    }

    private void StoreOverscanInfo() {
        SharedPreferences preferences =
            getContext().getSharedPreferences(
                    "com.intel.hdmi_preferences", Context.MODE_MULTI_PROCESS);
        Editor editor = preferences.edit();
        if (preferences != null) {
            editor.putInt("scaleRatio", mScaleRatio);
            editor.putInt("horiRatio", mHoriRatio);
            editor.putInt("vertRatio", mVertRatio);
            editor.putInt("mOrientation", mOrientation);
            editor.commit();
        }
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);
        StoreOverscanInfo();
    }
};
