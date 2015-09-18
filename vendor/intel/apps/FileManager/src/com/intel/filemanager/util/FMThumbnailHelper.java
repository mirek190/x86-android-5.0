package com.intel.filemanager.util;

import java.io.File;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

import com.intel.filemanager.R;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.PixelFormat;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.NinePatchDrawable;
import android.media.MediaMetadataRetriever;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.util.DisplayMetrics;

public class FMThumbnailHelper {

    private static LogHelper Log = LogHelper.getLogger();
    public static final String TAG = "FMThumbnailHelper";

    private static LRUCache mThumbnailCache;

    private static final int CACHE_MAX_SIZE = 40;

    public static void initThumbnailCathe() {
        mThumbnailCache = LRUCache.getInstanceOfLRUCache(CACHE_MAX_SIZE);
        if (null != mThumbnailCache) {
            mThumbnailCache.clear();
        } else {
            Log.d(TAG, "onCreate create thumbnail cache failed");
        }
    }

    public static LRUCache getThumbnailCathe() {
        return mThumbnailCache;
    }

    public static Drawable findThumbnailInCache(Context context, String fileName) {
        // find bitmap in the cache
        return findThumbnailInCache(context, fileName, -1);
    }
    
    public static Drawable findThumbnailInCache(Context context, String fileName, int type) {
        // find bitmap in the cache
    	Drawable d = null;
        
        if (null != mThumbnailCache) {
            d = (Drawable) mThumbnailCache.get(fileName);
        } else {
            mThumbnailCache = LRUCache.getInstanceOfLRUCache(CACHE_MAX_SIZE);
        }
        
        if (d == null) {
            if (type == -1) {
                d = getThumbnailBitmap(context, fileName);
            } else {
                d = getThumbnailBitmap(context, fileName, type);
            }
            if (d == null) return null;
            mThumbnailCache.put(fileName, d);
        }
        return d;
    }
    
    public static void updateThumbnailInCache(String newName, String oldName){
        if (mThumbnailCache == null || mThumbnailCache.get(oldName) == null) return;
        mThumbnailCache.put(newName, mThumbnailCache.get(oldName));
    }


    /**
     * the entry of this class
     */
    public static Drawable getThumbnailBitmap(Context context, String fullPath) {
    	Drawable d = null;
        String str = fullPath.replace("\'", "\''");
        if (MediaFile.isImageFileType(fullPath)) {
            d = getImageThumbnail(context, str);
        } else if (MediaFile.isVideoFileType(fullPath)) {
            d = getVideoThumbnail(context, str);
        } else if (fullPath.toLowerCase().endsWith(".apk")) {
            d = getUninstallApkIcon(context, fullPath);
        } else {
//            BitmapDrawable bd = (BitmapDrawable) context.getResources()
//                    .getDrawable(MediaFile.getIconForPath(fullPath));
//            b = bd.getBitmap();
            d = null;
        }
        return d;
    }
    
    public static Drawable getThumbnailBitmap(Context context, String fullPath, int type) {
        Drawable d = null;
        String str = fullPath.replace("\'", "\''");
        switch(type){
        case FileUtil.CATEGORY_IMAGE:
            d = getImageThumbnail(context, str);
            break;
        case FileUtil.CATEGORY_VIDEO:
            d = getVideoThumbnail(context, str);
            break;
        case FileUtil.CATEGORY_APK:
            d = getUninstallApkIcon(context, fullPath);
            break;
        case FileUtil.CATEGORY_APP:
            d = getInstallAppIcon(context, fullPath);
            break;
        case FileUtil.CATEGORY_AUDIO:
            d = getAudioThumbnail(context, str);
            break;
        default:
//            BitmapDrawable bd = (BitmapDrawable) context.getResources()
//                    .getDrawable(MediaFile.getIconForPath(fullPath));
//            b = bd.getBitmap();
            d = null;
            break;
        }
        return d;
    }
    
    public static Drawable getAudioThumbnail(Context context, String fileName) {
        BitmapFactory.Options bitmapOptions = new BitmapFactory.Options();
        Bitmap albumart = null;
        Bitmap bitmap = null;
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        String path = fileName;
        
        try {
            int sampleSize = 1;
            retriever.setDataSource(path);
            byte[] pic = retriever.getEmbeddedPicture();
            bitmapOptions.inJustDecodeBounds = true;

            BitmapFactory.decodeByteArray(pic, 0, pic.length, bitmapOptions);

            bitmapOptions.inJustDecodeBounds = false;

            int nextWidth = bitmapOptions.outWidth;
            int nextHeight = bitmapOptions.outHeight;

            while (nextWidth > 128 && nextHeight > 128) {
                sampleSize <<= 1;
                nextWidth >>= 1;
                nextHeight >>= 1;
            }
            bitmapOptions.inSampleSize = sampleSize;

            albumart = BitmapFactory.decodeByteArray(pic, 0, pic.length,
                    bitmapOptions);

            Bitmap tmp = Bitmap.createScaledBitmap(albumart, 64, 64, false);
            bitmap = tmp;
        } catch (Exception e) {
            return context.getResources().getDrawable(
                    R.drawable.file_icon_audio);
           } catch (OutOfMemoryError err) {
            return context.getResources().getDrawable(
                    R.drawable.file_icon_audio);
           } finally {
              if (albumart != null) {
                  albumart.recycle();
                } 
            }
        
          return new BitmapDrawable(context.getResources(), bitmap);
        
    }


    /**
     * get the thumbnail of image from com.android.providers.media DB.
     * 
     * @param cr
     *            the ContentResolver of this Context.
     * @param fileName
     *            the image's full name which hava thumbnail in DB
     * @return bitmap the bitmap type thumbnail.
     */
    public static Drawable getImageThumbnail(Context context, String fileName) {
        ContentResolver cr = context.getContentResolver();
        Bitmap bitmap = null;
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inDither = false;
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;
        String whereClause = "upper(" + MediaStore.Images.ImageColumns.DATA
                + ") = '" + fileName.toUpperCase() + "'";
        Cursor cursor = null;
        try {
            cursor = cr.query(MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
                    new String[] { MediaStore.Images.Media._ID,
                            MediaStore.Images.Media.ORIENTATION }, whereClause,
                    null, null);
        } catch (Exception ex) {
            Log.e(TAG, "query error: " + ex.getStackTrace());
        }
        if (cursor == null)
            return null;
        if (cursor.getCount() == 0) {
            cursor.close();
            return null;
        }
        cursor.moveToFirst();
        String imageId = cursor.getString(cursor
                .getColumnIndex(MediaStore.Images.Media._ID));
        int orientation = cursor.getInt(cursor
                .getColumnIndex(MediaStore.Images.Media.ORIENTATION));
        if (imageId == null) {
            cursor.close();
            return null;
        }
        cursor.close();
        long imageIdLong = Long.parseLong(imageId);

        bitmap = Images.Thumbnails.getThumbnail(cr, imageIdLong,
                Images.Thumbnails.MICRO_KIND, options);

        if (orientation > 0) {
            bitmap = rotateBitmap(bitmap, orientation);
        }
        
        try {
            Bitmap tmp = Bitmap.createScaledBitmap(bitmap, 64, 64, false);
            bitmap = tmp;
        } catch (Exception e) {
            Log.v(TAG, " createScaledBitmap failed, file name: " + fileName);
            e.printStackTrace();
            return null;
        }
        
        return new BitmapDrawable(context.getResources(), bitmap);
    }

    private static Bitmap rotateBitmap(Bitmap b, int orientation) {
        if(b == null){
            return null;
        }
        Matrix matrix = new Matrix();
        matrix.postScale(1, 1);
        switch (orientation) {
        case 90:
            matrix.setRotate(90);
            break;
        case 270:
            matrix.setRotate(270);
            break;
        case 180:
            matrix.setRotate(180);
            break;
        default:
            Log.d(TAG, "orientation =" + orientation
                    + ", nothing to be done here!");
            break;
        }
        return Bitmap.createBitmap(b, 0, 0, b.getWidth(), b.getHeight(),
                matrix, true);
    }

    /**
     * get the thumbnail of video from com.android.providers.media DB.
     * 
     * @param cr
     *            the ContentResolver of this Context.
     * @param fileName
     *            the video's full name which hava thumbnail in DB
     * @return bitmap the bitmap type thumbnail.
     */
    public static Drawable getVideoThumbnail(Context context, String fileName) {
        ContentResolver cr = context.getContentResolver();
        Bitmap bitmap = null;
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inDither = false;
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;
        String whereClause = "upper(" + MediaStore.Video.Media.DATA + ") = '"
                + fileName.toUpperCase() + "'";
        Cursor cursor =null;
        try{
            cursor = cr.query(MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                    new String[] { MediaStore.Video.Media._ID }, whereClause, null,
                    null);
            if (cursor == null)
                return null;
            if (cursor.getCount() == 0) {
                cursor.close();
                return null;
            }
            cursor.moveToFirst();
            String videoId = cursor.getString(cursor
                    .getColumnIndex(MediaStore.Video.Media._ID));
            if (videoId == null) {
                cursor.close();
                return null;
            }
            long videoIdLong = Long.parseLong(videoId);
            bitmap = MediaStore.Video.Thumbnails.getThumbnail(cr, videoIdLong,
                    Images.Thumbnails.MICRO_KIND, options);
            
            try {
                Bitmap tmp = Bitmap.createScaledBitmap(bitmap, 64, 64, false);
                bitmap = tmp;
            } catch (Exception e) {
                Log.v(TAG, " createScaledBitmap failed, file name: " + fileName);
                e.printStackTrace();
                return null;
            }
            
            return new BitmapDrawable(context.getResources(), bitmap);
        }catch(SQLiteException e){
            Log.d(TAG,e.getMessage());
            return null;
        }finally{
            if(cursor!=null  && !cursor.isClosed()){
                cursor.close();
            }
        }
        
    }
    
    public static Drawable getInstallAppIcon(Context context, String packageName){
        try{
            return context.getPackageManager().getApplicationIcon(packageName);
        }catch(NameNotFoundException nameException){
            Log.v(TAG, "package not found, packageName = " + packageName);
            return null;
        }
    }

    /**
     * get the icon of a apk file which in sdcard and not installed.
     * 
     * @param context
     * @param archiveFilePath
     *            APK file's path.
     * @return icon the Drawable type icon.
     */
    
/*    public static Bitmap getUninstallApkIcon(Context context,
            String archiveFilePath) {
        Bitmap icon = null;
        Drawable dicon = null;
        // get a PackageManager instance from this context.
        PackageManager pm = context.getPackageManager();
        try {
            Uri uri = Uri.fromFile(new File(archiveFilePath));
            ApplicationInfo ai = getApplicationInfo(uri);

            Resources pRes = context.getResources();
            AssetManager assmgr = new AssetManager();
            assmgr.addAssetPath(archiveFilePath);
            Resources res = new Resources(assmgr, pRes.getDisplayMetrics(),
                    pRes.getConfiguration());

            icon = ((BitmapDrawable) res.getDrawable(ai.icon)).getBitmap();
        } catch (Exception e) {
            Log.v(TAG, "Can't find the application icon!");
        }
        return icon;
    }
    
    public static  ApplicationInfo getApplicationInfo(Uri packageURI) {
        final String archiveFilePath = packageURI.getPath();
        PackageParser packageParser = new PackageParser(archiveFilePath);
        File sourceFile = new File(archiveFilePath);
        DisplayMetrics metrics = new DisplayMetrics();
        metrics.setToDefaults();
        PackageParser.Package pkg = packageParser.parsePackage(sourceFile, archiveFilePath, metrics, 0);
        if (pkg == null) {
            return null;
        }
        return pkg.applicationInfo;
    }
*/    

    private static final String PACKAGE_PARSER = "android.content.pm.PackageParser";
    private static final String ASSET_MANAGER = "android.content.res.AssetManager";
    public static Drawable getUninstallApkIcon(Context context, String archiveFilePath) {
        Drawable icon = null;
        try {
            Class pkgParserCls = Class.forName(PACKAGE_PARSER);
            Class[] typeArgs = new Class[1];
            typeArgs[0] = String.class;
            Constructor pkgParserCt = pkgParserCls.getConstructor(typeArgs);
            Object[] valueArgs = new Object[1];
            valueArgs[0] = archiveFilePath;
            Object pkgParser = pkgParserCt.newInstance(valueArgs);

            DisplayMetrics metrics = new DisplayMetrics();
            metrics.setToDefaults();
            typeArgs = new Class[4];
            typeArgs[0] = File.class;
            typeArgs[1] = String.class;
            typeArgs[2] = DisplayMetrics.class;
            typeArgs[3] = Integer.TYPE;
            
            Method pkgParser_parsePackageMtd = pkgParserCls.getDeclaredMethod("parsePackage", typeArgs);
            valueArgs = new Object[4];
            valueArgs[0] = new File(archiveFilePath);
            valueArgs[1] = archiveFilePath;
            valueArgs[2] = metrics;
            valueArgs[3] = 0;
            Object pkgParserPkg = pkgParser_parsePackageMtd.invoke(pkgParser,
                    valueArgs);

            Field appInfoFld = pkgParserPkg.getClass().getDeclaredField(
                    "applicationInfo");
            
            ApplicationInfo info = (ApplicationInfo) appInfoFld
                    .get(pkgParserPkg);
            
            Class assetMagCls = Class.forName(ASSET_MANAGER);
            Constructor assetMagCt = assetMagCls.getConstructor((Class[]) null);
            Object assetMag = assetMagCt.newInstance((Object[]) null);
            typeArgs = new Class[1];
            typeArgs[0] = String.class;
            Method assetMag_addAssetPathMtd = assetMagCls.getDeclaredMethod(
                    "addAssetPath", typeArgs);
            valueArgs = new Object[1];
            valueArgs[0] = archiveFilePath;
            assetMag_addAssetPathMtd.invoke(assetMag, valueArgs);
            
            Resources res = context.getResources();// getResources();
            typeArgs = new Class[3];
            typeArgs[0] = assetMag.getClass();
            typeArgs[1] = res.getDisplayMetrics().getClass();
            typeArgs[2] = res.getConfiguration().getClass();
            Constructor resCt = Resources.class.getConstructor(typeArgs);
            valueArgs = new Object[3];
            valueArgs[0] = assetMag;
            valueArgs[1] = res.getDisplayMetrics();
            valueArgs[2] = res.getConfiguration();
            res = (Resources) resCt.newInstance(valueArgs);
            
            icon = ((BitmapDrawable) res.getDrawable(info.icon));
        } catch (Exception e) {
            e.printStackTrace();
        }
        return icon;

    }
    

}
