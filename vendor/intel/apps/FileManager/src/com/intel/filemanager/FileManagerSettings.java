package com.intel.filemanager;

import java.math.RoundingMode;
import java.text.DecimalFormat;

import android.app.ActionBar;
import android.content.Context;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.PreferenceActivity;
import android.widget.Toast;

import com.intel.filemanager.R;

public class FileManagerSettings extends PreferenceActivity {
    public static final String ENABLE_SHOW_HIDDENFILE = "enable_show_hiddenfile";
    
    private Toast mToast;
    
    private Context mContext;

    CheckBoxPreference mShowHiddenfileEnable;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.filemanager_settings);
        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setDisplayShowTitleEnabled(true);
            setTitle(R.string.fm_options_settings);
        }

        mContext = this;
    }
}
