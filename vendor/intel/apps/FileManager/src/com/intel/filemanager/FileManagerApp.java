/**
 * Copyright (c) 2007, Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *
 *     http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */

package com.intel.filemanager;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

import android.app.Application;

public class FileManagerApp extends Application {
    // indicate whether paste is available.
    protected boolean isPasteAvailable = false;
    // indicate paste source path.
    protected List<String> mPasteSource = new ArrayList<String>();
    // indicate the current position in mPasteSource
    protected int iPos = 0;
    // indicate whether delete original files/direcoties.
    protected boolean mCutState = false;

    private static AtomicBoolean mIsInit = new AtomicBoolean(false);

    public boolean getPasteState()
    {
        return isPasteAvailable;
    }

    /**
     * Set paste available or unavailable
     * 
     * @Param mPasteAvailable
     * @return boolean the pre state of paste.
     */
    public boolean setPasteState(boolean mPasteAvailable)
    {
        boolean available = isPasteAvailable;
        isPasteAvailable = mPasteAvailable;
        // if(!mPasteAvailable)
        // mPasteSource.clear();
        return available;
    }

    public void clearPasteSource()
    {
        iPos = 0;
        mPasteSource.clear();
    }

    public int getPasteSourceSize()
    {
        return mPasteSource.size();
    }

    public void addPasteSource(String source)
    {
        mPasteSource.add(source);
    }

    public String getPasteSource(int position)
    {
        if ((position < 0) || (position >= mPasteSource.size()))
            return null;
        else
            return mPasteSource.get(position);
    }

    public boolean hasNextPasteSource()
    {
        if ((iPos >= 0) && (iPos < mPasteSource.size()))
            return true;
        return false;
    }

    public String getNextPasteSource()
    {
        if ((iPos >= 0) && (iPos < mPasteSource.size()))
            return mPasteSource.get(iPos++);
        return null;
    }

    public void rewindPasteSource()
    {
        iPos = 0;
    }

    public void setCutState(boolean state)
    {
        mCutState = state;
    }

    public boolean getCutState()
    {
        return mCutState;
    }

    public FileManagerApp() {
    }

    public void onCreate() {
        super.onCreate();
        System.out.println("============init=================");

        if (mIsInit.getAndSet(true)) {
            return;
        }
    }

    public void onTerminate() {
    }

}
