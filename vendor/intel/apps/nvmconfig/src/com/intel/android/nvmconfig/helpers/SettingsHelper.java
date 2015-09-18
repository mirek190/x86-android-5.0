package com.intel.android.nvmconfig.helpers;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;

public class SettingsHelper {

    private static final String PREFS_NAME = "nvmconfig_settings";

    public static String getVariantScanPath(Context context) {

        SharedPreferences settings = context.getSharedPreferences(PREFS_NAME,
                Activity.MODE_PRIVATE);
        return settings.getString("scan_path_key", "/");
    }

    public static void setVariantScanPath(Context context, String path) {

        SharedPreferences settings = context.getSharedPreferences(PREFS_NAME,
                Activity.MODE_PRIVATE);
        SharedPreferences.Editor editor = settings.edit();

        editor.putString("scan_path_key", path).commit();
    }
}
