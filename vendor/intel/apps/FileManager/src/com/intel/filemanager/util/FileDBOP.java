package com.intel.filemanager.util;

import java.io.File;
import java.util.HashMap;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.media.MediaScannerConnection;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Environment;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.widget.Toast;

import com.intel.filemanager.FileOperator;
import com.intel.filemanager.R;

/*
 * use this class  to
 * update Media DB when do operations on the files
 */
public class FileDBOP {

    private static LogHelper Log = LogHelper.getLogger();
    private static final String TAG = "FileDBOP";
    private static final String SD = Environment.getExternalStorageDirectory()
            .getAbsolutePath();
    private static final String PHONE = "/local";

    public final static String SEPARATOR = File.pathSeparator;
    public final static String INITSD = Environment
            .getExternalStorageDirectory().getAbsolutePath();
    public final static String INITPHONE = "/local";
    public static final String ACTION_MEDIA_SCANNER_SCAN_DIR = "android.intent.action.MEDIA_SCANNER_SCAN_DIR";

    private static Uri sThumbURI = Images.Media.EXTERNAL_CONTENT_URI;
    public static File THUMBFILE = null;

    private static String[][] COLS = new String[][] {
            { MediaStore.Audio.Media.MIME_TYPE, MediaStore.Audio.Media.DATA,
                    MediaStore.Audio.Media._ID,
                    MediaStore.Audio.Media.DISPLAY_NAME,
                    MediaStore.Audio.Media.TITLE },
            { MediaStore.Video.Media.MIME_TYPE, MediaStore.Video.Media.DATA,
                    MediaStore.Video.Media._ID,
                    MediaStore.Video.Media.DISPLAY_NAME,
                    MediaStore.Video.Media.TITLE },
            { MediaStore.Images.Media.MIME_TYPE, MediaStore.Images.Media.DATA,
                    MediaStore.Images.Media._ID,
                    MediaStore.Images.Media.DISPLAY_NAME,
                    MediaStore.Images.Media.TITLE },
            { MediaStore.Audio.Media.MIME_TYPE, MediaStore.Audio.Media.DATA,
                    MediaStore.Audio.Media._ID,
                    MediaStore.Audio.Media.DISPLAY_NAME,
                    MediaStore.Audio.Media.TITLE },
            { MediaStore.Video.Media.MIME_TYPE, MediaStore.Video.Media.DATA,
                    MediaStore.Video.Media._ID,
                    MediaStore.Video.Media.DISPLAY_NAME,
                    MediaStore.Video.Media.TITLE },
            { MediaStore.Images.Media.MIME_TYPE, MediaStore.Images.Media.DATA,
                    MediaStore.Images.Media._ID,
                    MediaStore.Images.Media.DISPLAY_NAME,
                    MediaStore.Images.Media.TITLE }, };
    private static Uri[] URICOLS = new Uri[] {
            MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
            MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
            MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
            MediaStore.Audio.Media.INTERNAL_CONTENT_URI,
            MediaStore.Video.Media.INTERNAL_CONTENT_URI,
            MediaStore.Images.Media.INTERNAL_CONTENT_URI };

    // return what kind of file type is must test image, video, audio DB
    public static String getFileType(ContentResolver cr, String path) {
        try {
            if (!Environment.getExternalStorageState().equals(
                    Environment.MEDIA_MOUNTED)) {
                // no sdcard can't delete in DB
                return "";
            }

            String[] cols;
            Uri uri;
            path = path.replaceAll("'", "''");
            for (int i = 0; i < COLS.length; i++) {
                cols = COLS[i];
                uri = URICOLS[i];
                Cursor mCursor = cr.query(uri, cols, "_data=" + "'" + path
                        + "'", null, null);
                if (mCursor != null) {
                    if (mCursor.getCount() == 0) {
                        mCursor.close();
                        continue;
                    }
                    mCursor.moveToFirst();
                    String mimetype = mCursor.getString(mCursor
                            .getColumnIndexOrThrow(cols[0]));
                    // Log.v(TAG,"path == "+path+" mimetype = "+mimetype);
                    mCursor.close();
                    return mimetype;
                } else {
                    continue;
                }
            }
        } catch (Exception ex) {
            Log.e(TAG, "Exception : " + ex.toString());
        }
        return "";
    }

    public static boolean startMediaScan(final Context context, final String path) {
        if (null == context || null == path) {
            Log.d(TAG, "the params for updateInDB() is invalid!");
            return false;
        }

         MediaScannerConnection.scanFile(context, new String[] { path.toString()
            }, null, new MediaScannerConnection.OnScanCompletedListener() {
                public void onScanCompleted(String path, Uri uri) {
                    Log.i(TAG, "Scanned " + path + ":");
                    Log.i(TAG, "-> uri=" + uri);
                }
            });
        return true;
    }
    
    public static void deleteFileInDB(Context context, String path){
        ContentResolver cr = context.getContentResolver();
        String wherestr = MediaStore.MediaColumns.DATA + " = '" + FileOperator.getSQLFiltedString(path) + "'";
        try{
            cr.delete(Uri.parse("content://media/external/file") ,wherestr, null);
            Log.d(TAG, "delete from media db, file: " + path);
        }catch(Exception e){
            Log.e(TAG,e.getMessage());
        }
    }
    
    public static void updateFileInDB(Context context, String newPath, String oldPath){
        Uri uri = Uri.parse("content://media/external/file");
        if (MediaFile.isAudioFileType(newPath)) {
            uri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        } else if (MediaFile.isImageFileType(newPath)) {
            uri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
        } else if (MediaFile.isVideoFileType(newPath)) {
            uri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
        }

        Log.d(TAG, "update common file, update meida db directly,uri: "
                + uri.toString() + " newPath: " + newPath + ", oldPath: "
                + oldPath);

        ContentResolver cr = context.getContentResolver();
        ContentValues values = new ContentValues();

        values.put(MediaStore.Files.FileColumns.DATA, newPath);
        String wherestr = MediaStore.MediaColumns.DATA + " = '"
                + FileOperator.getSQLFiltedString(oldPath) + "'";
        try{
            cr.update(uri, values, wherestr, null);
        }catch(Exception e){
            Log.e(TAG,e.getMessage());
        }
    }

    // need delete in Thumbnail
    public static boolean deleteInThumbnail(String _id, String path,
            ContentResolver cr) {
        String[] cols = new String[] { Images.Thumbnails._ID,
                Images.Thumbnails.IMAGE_ID };
        Uri _uri = Images.Thumbnails.EXTERNAL_CONTENT_URI;
        Log.v(TAG, "DELETE image id = " + _id);
        
        Cursor mCursor = null;
        try{
            mCursor = cr.query(_uri, cols, "image_id=" + _id, null, null);
            if (mCursor == null) {
                Log.d(TAG, "mCursor = null ");
                return false;
            }
            if (mCursor.getCount() != 0) {
                mCursor.moveToFirst();
                int where = mCursor.getInt(mCursor.getColumnIndexOrThrow(cols[0]));
                deleteImageThumb(path, where);
                return true;
            } else {
                return false;
            }
        }catch(Exception e){
            Log.e(TAG,e.getMessage());
            return false;
        }finally{
            if(mCursor!=null){
                mCursor.close();
            }
        }
    }

    public static boolean deleteImageThumb(String path, int where) {
        if (THUMBFILE == null) {
            THUMBFILE = new File("/sdcard/Pictures/.thumbnails/.thumbdata3-"
                    + sThumbURI.hashCode());
        }
        return false;
    }

    public static Uri updateMediaAudioDB(Context context, String path) {
        ContentResolver resolver = context.getContentResolver();

        String[] cols = new String[] { MediaStore.Audio.Media._ID,
                MediaStore.Audio.Media.DATA, MediaStore.Audio.Media.TITLE };

        path = path.replaceAll("'", "''");
        Uri uri;
        if (path.startsWith(SD)) {
            uri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        } else {
            uri = MediaStore.Audio.Media.INTERNAL_CONTENT_URI;
        }

        String where = MediaStore.Audio.Media.DATA + "=" + "'" + path + "'";
        // Log.v(TAG,"PATH = "+path+" where = "+where);
        Cursor cursor = null;
        String title = null;
        try {
            cursor = resolver.query(uri, cols, where, null, null);
            if (cursor != null && cursor.getCount() == 1) {

                cursor.moveToFirst();
                long _id = cursor
                        .getLong(cursor.getColumnIndexOrThrow(cols[0]));

                Uri ringUri = Uri.withAppendedPath(uri, String.valueOf(_id));
                title = cursor.getString(cursor.getColumnIndexOrThrow(cols[2]));

                ContentValues values = new ContentValues(3);
                values.put(MediaStore.Audio.Media.IS_RINGTONE, "1");
                values.put(MediaStore.Audio.Media.IS_ALARM, "1");
                values.put(MediaStore.Audio.Media.IS_NOTIFICATION, "1");
                resolver.update(ringUri, values, null, null);
                return ringUri;
            } else {
                // the audio file is not exist in audio table of 
                //com.android.providers.media/databases/external.db
                return null;
            }
        } catch (Exception e) {
            Log.e(TAG, "SET RINGTONE EXCEPTION " + e);
            return null;
        } finally {
            if (null != cursor)
                cursor.close();
        }
    }

    private static String[] CHMOD = new String[] { "chmod", "777", "" };
    private static String[] CHMODFOLDER = new String[] { "chmod", "777", "" };

    public final static boolean changePermission(String path) {
        // Log.v(TAG,"CHANGE PERMISSION OF "+path);
        String folderPath = path.substring(0, path.lastIndexOf("/"));
        CHMOD[2] = path;
        CHMODFOLDER[2] = folderPath;
        try {
            Runtime.getRuntime().exec(CHMOD);
            Runtime.getRuntime().exec(CHMODFOLDER);
        } catch (Exception e) {
            Log.e(TAG, "exception during change file permission");
            return false;
        }
        return true;
    }

}
