package com.intel.filemanager.util;

import android.text.InputFilter;
import android.text.Spanned;

public class EditTextLengthFilter implements InputFilter {
    public EditTextLengthFilter(int max) {
        mMax = max;
    }

    public CharSequence filter(CharSequence source, int start, int end,
                               Spanned dest, int dstart, int dend) {
        int keep = mMax - (dest.length() - (dend - dstart));

        if (keep <= 0) {
        	if(triger!=null)
        		triger.onGetMax();
            return "";
        } else if (keep >= end - start) {
            return null; // keep original
        } else {
            return source.subSequence(start, start + keep);
        }
    }
    public void setTriger(Triger t)
    {
    	triger = t;
    }
    private int mMax;
    private Triger triger = null;
    public interface Triger{
    	public void onGetMax();
    }
}
