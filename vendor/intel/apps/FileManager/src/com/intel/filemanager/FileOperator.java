package com.intel.filemanager;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Point;
import android.media.AudioManager;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Parcelable;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.text.TextUtils;
import android.widget.Toast;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FileDBOP;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.LogHelper;
import com.intel.filemanager.util.MediaFile;
import com.intel.filemanager.FileManagerSettings;

public class FileOperator {

    private static LogHelper Log = LogHelper.getLogger();

    private final static String TAG = "FileOperator";

    private static FileOperator mInstance = null;

    private static final String fileNameFilter = ".";

    public static boolean isDeleteFiles = false;

    public static String srcParentPath = "";

    public static boolean isCut = false;

    public static boolean isCanPaste = false;

    public static String copyPath = "";

    public FileOperator() {
    }

    public static FileOperator instance() {
        if (null == mInstance) {
            mInstance = new FileOperator();
        }
        return mInstance;
    }

    public static File[] getSubFileArray(String path, final Context context) {
        File file = new File(path);
        if (file == null || !file.exists()) {
            return null;
        }
        Log.v(TAG, "file = " + file.getAbsolutePath());
        File[] files = file.listFiles(new FilenameFilter() {
            public boolean accept(File arg0, String fileName) {
                if (fileName.startsWith(fileNameFilter)) {
                    return PreferenceManager.getDefaultSharedPreferences(context)
                            .getBoolean("showHiddenfile", false);
                }
                return true;
            }
        });
        return files;
    }

    public static String[] getSubFileStringArray(String path, final Context context) {
        File file = new File(path);
        if (file == null || !file.exists()) {
            return null;
        }
        Log.v(TAG, "file = " + file.getAbsolutePath());
        String[] files = file.list(new FilenameFilter() {
            public boolean accept(File arg0, String fileName) {
                if (fileName.startsWith(fileNameFilter)) {
                    return PreferenceManager.getDefaultSharedPreferences(context)
                            .getBoolean("showHiddenfile", false);
                }
                return true;
            }
        });
        return files;
    }

    public void preCopy(String path) {
        isDeleteFiles = false;
        srcParentPath = path;
        isCut = false;
        isCanPaste = false;
    }

    public static void copy(Context context, FileNode node) {
        FileOperator.isCut = false;
        FileOperator.isCanPaste = true;
        FileOperator.copyPath = node.path;
        Log.v(TAG, "copyPath = " + node.path);
        File temp = new File(FileOperator.copyPath);
        FileOperator.srcParentPath = temp.getParent();
        Toast.makeText(context, R.string.fm_options_copy, Toast.LENGTH_SHORT).show();
    }

    public static void copy(String path, Context context) {
        Log.v(TAG, "copy file " + path);
        if (TextUtils.isEmpty(path))
            return;
        File file = new File(path);
        FileOperator.isCut = false;
        FileOperator.isCanPaste = true;
        FileOperator.copyPath = path;
        if (file.exists()) {
            FileOperator.srcParentPath = file.getParent();
        } else {
            FileOperator.srcParentPath = null;
        }

        Log.v(TAG, "copyPath = " + path);

        sendPasteIntent(context);
    }

    public static void cut(Context context, FileNode node) {
        Log.v(TAG, "cut file " + node.path);
        FileOperator.isCut = true;
        FileOperator.isCanPaste = true;
        FileOperator.copyPath = node.path;
        File temp1 = new File(FileOperator.copyPath);
        FileOperator.srcParentPath = temp1.getParent();

        sendPasteIntent(context);
    }

    public static void cut(String path, Context context) {
        Log.v(TAG, "cut file " + path);
        if (TextUtils.isEmpty(path))
            return;
        File file = new File(path);
        FileOperator.isCut = true;
        FileOperator.isCanPaste = true;
        FileOperator.copyPath = path;
        if (file.exists()) {
            FileOperator.srcParentPath = file.getParent();
        } else {
            FileOperator.srcParentPath = null;
        }

        sendPasteIntent(context);
    }

    public static void sendPasteIntent(Context context) {
        if (context instanceof SubCategoryActivity) {
            ((SubCategoryActivity) context).finish();
        }

        Intent intent = new Intent(FileMainActivity.ACTION_PASTE_FILE);
        intent.addCategory(Intent.CATEGORY_DEFAULT);
        intent.setClass(context, FileMainActivity.class);

        intent.putExtra(FileMainActivity.EXTRA_FILE_PATH, FileOperator.srcParentPath);
        context.startActivity(intent);
    }

    public static void doShare(Context context, FileNode node) {
        Log.v(TAG, "share file " + node.path);
        try {
            File f = new File(node.path);
            String mime = null;
            mime = node.type;// MediaFile.getMimeTypeForFile(f);
            Intent shareIntent = new Intent(Intent.ACTION_SEND);
            Log.v(TAG, "mime = " + mime);
            if (mime == null || mime.equals("") || mime.equals("unknown")) {
                mime = "*/*";
            }
            shareIntent.setType(mime);
            shareIntent.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(f));
            context.startActivity(Intent.createChooser(shareIntent, null));
        } catch (android.content.ActivityNotFoundException e) {
            Toast.makeText(context, R.string.fm_share_no_way_to_share, Toast.LENGTH_SHORT).show();
        }
    }

    public static void doShare(String path, Context context) {
        Log.v(TAG, "share file " + path);
        if (TextUtils.isEmpty(path))
            return;
        if (path.contains(File.pathSeparator)) {
            // do multi share
            ArrayList<Parcelable> pathList = new ArrayList<Parcelable>();
            for (String subPath : path.split(File.pathSeparator)) {
                pathList.add(Uri.fromFile(new File(subPath)));
            }
            Intent intent = new Intent(Intent.ACTION_SEND_MULTIPLE);
            intent.setType("*/*");
            intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, pathList);
            context.startActivity(intent);
        } else {
            // do share
            try {
                File f = new File(path);
                String mime = null;
                mime = MediaFile.getMimeType(path);
                Intent shareIntent = new Intent(Intent.ACTION_SEND);
                Log.v(TAG, "mime = " + mime);
                if (mime == null || mime.equals("") || mime.equals("unknown")) {
                    mime = "*/*";
                }
                shareIntent.setType(mime);
                shareIntent.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(f));
                context.startActivity(Intent.createChooser(shareIntent, null));
            } catch (android.content.ActivityNotFoundException e) {
                Toast.makeText(context, R.string.fm_share_no_way_to_share, Toast.LENGTH_SHORT)
                        .show();
            }
        }
    }

    public static void setRingTone(final String path, final Context context) {
        Log.v(TAG, "setRingTone path = " + path);
        if (TextUtils.isEmpty(path))
            return;

        new AlertDialog.Builder(context).setTitle(R.string.title_set_ringtone)
                .setItems(R.array.ringtone_type, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Uri ringUri = FileDBOP.updateMediaAudioDB(context, path);
                        if (null == ringUri) {
                            String message = context.getString(
                                    R.string.fm_ringtone_setting_error,
                                    path.substring(path.lastIndexOf('/') + 1));
                            Toast.makeText(context, message, Toast.LENGTH_SHORT).show();
                            return;
                        }

                        final int RINGTONE_TYPE_ALARM = 0;
                        final int RINGTONE_TYPE_PHONE = 1;
                        final int RINGTONE_TYPE_NOTIFICATION = 2;

                        switch (which) {
                            case RINGTONE_TYPE_ALARM:
                                RingtoneManager.setActualDefaultRingtoneUri(context,
                                        RingtoneManager.TYPE_ALARM, ringUri);
                                Toast.makeText(context, R.string.set_ringtone_success,
                                        Toast.LENGTH_SHORT).show();
                                break;
                            case RINGTONE_TYPE_PHONE:
                                Intent targetedIntent = new Intent(
                                        "art.calling.SET_CUSTOM_RINGTONE");
                                targetedIntent.putExtra("ringtone_uri", ringUri.toString());
                                context.sendBroadcast(targetedIntent, null);
                                RingtoneManager.setActualDefaultRingtoneUri(context,
                                        RingtoneManager.TYPE_RINGTONE, ringUri);
                                Toast.makeText(context, R.string.set_ringtone_success,
                                        Toast.LENGTH_SHORT).show();
                                break;
                            case RINGTONE_TYPE_NOTIFICATION:
                                RingtoneManager.setActualDefaultRingtoneUri(context,
                                        RingtoneManager.TYPE_NOTIFICATION, ringUri);
                                Toast.makeText(context, R.string.set_ringtone_success,
                                        Toast.LENGTH_SHORT).show();
                                break;
                            default:
                                break;
                        }
                    }
                }).show();
    }

    public static void setWallPaper(String path, Context context) {
        if (TextUtils.isEmpty(path))
            return;
        try {
            String mimeType = "image/*";
            int width = context.getWallpaperDesiredMinimumWidth();
            int height = context.getWallpaperDesiredMinimumHeight();
            Point size = new Point();
            ((Activity) context).getWindowManager().getDefaultDisplay().getSize(size);
            float spotlightX = (float) size.x / width;
            float spotlightY = (float) size.y / height;

            Intent intent = new Intent("com.android.camera.action.CROP");
            intent.setDataAndType(Uri.fromFile(new File(path)), mimeType);
            intent.putExtra("outputX", width);
            intent.putExtra("outputY", height);
            intent.putExtra("aspectX", width);
            intent.putExtra("aspectY", height);
            intent.putExtra("spotlightX", spotlightX);
            intent.putExtra("spotlightY", spotlightY);
            intent.putExtra("scale", true);
            intent.putExtra("scaleUpIfNeeded", true);
            intent.putExtra("noFaceDetection", true);
            intent.putExtra("set-as-wallpaper", true);
            intent.putExtra("setWallpaper", true);
            context.startActivity(intent);
        } catch (Exception e) {
            String message = context.getString(R.string.wallpaper_set_error, path);
            Toast.makeText(context, message, Toast.LENGTH_SHORT).show();
            e.printStackTrace();
            Log.v(TAG, "SET WALLPAPER ERROR");
        }
    }

    public static void showProperty(String path, Context context) {
        Log.v(TAG, "in showProperty srcPath = " + path);
        if (TextUtils.isEmpty(path))
            return;
        try {
            Intent result = new Intent(context, FileDetailInfor.class);
            result.putExtra(FileDetailInfor.PATHTAG, path);
            File file = new File(path);
            if (file.isDirectory()) {
                result.putExtra(FileDetailInfor.TYPETAG, "Folder");
            } else {
                result.putExtra(FileDetailInfor.TYPETAG, MediaFile.getMimeType(path));
            }
            context.startActivity(result);
        } catch (Exception ex) {
            Log.v(TAG, "in showProperty(): " + ex.toString());
        }
    }

    public static String getSQLFiltedString(String filterString) {
        return filterString.replaceAll("'", "''");
    }

    public static void addFavorite(String path, int category, Context context) {
        if (TextUtils.isEmpty(path) || isFavorite(path, context))
            return;
        ContentValues values = new ContentValues();
        values.put(FavoriteProvider.FavoriteColumns.PATH, path);
        values.put(FavoriteProvider.FavoriteColumns.CATEGORY, category);
        context.getContentResolver().insert(FavoriteProvider.FavoriteColumns.CONTENT_URI, values);
    }

    public static void deleteFavorite(String path, Context context) {
        if (TextUtils.isEmpty(path) || !isFavorite(path, context))
            return;
        context.getContentResolver().delete(FavoriteProvider.FavoriteColumns.CONTENT_URI,
                FavoriteProvider.FavoriteColumns.PATH + " = '" + getSQLFiltedString(path) + "'",
                null);
    }

    public static boolean isFavorite(String path, Context context) {
        if (TextUtils.isEmpty(path))
            return false;
        Cursor cursor = context.getContentResolver().query(
                FavoriteProvider.FavoriteColumns.CONTENT_URI, null,
                FavoriteProvider.FavoriteColumns.PATH + " = '" + getSQLFiltedString(path) + "'",
                null, null);
        if (cursor != null) {
            int count = cursor.getCount();
            cursor.close();
            return count > 0;
        } else {
            return false;
        }
    }

}
