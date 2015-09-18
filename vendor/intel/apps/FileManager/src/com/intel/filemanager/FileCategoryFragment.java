package com.intel.filemanager;

import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.res.Configuration;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.StatFs;
import android.provider.BaseColumns;
import android.provider.MediaStore;
import android.text.TextUtils;
import android.text.format.Formatter;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.LogHelper;

public class FileCategoryFragment extends Fragment {
    private static LogHelper Log = LogHelper.getLogger();
    private static final String TAG = "FileCategoryFragment";

    private LoadCountTask mLoadCountTask;
    private Uri mUri;
    private String mSelection;
    private Context mContext;

    private static final int STORAGE_MID = 60;
    private static final int STORAGE_WAR = 85;

    private Map<Integer, Integer> mCategoryCountMap = new ConcurrentHashMap<Integer, Integer>();
    private int mOrientation;

    private View mCategoryImage;
    private View mCategoryVideo;
    private View mCategoryAudio;
    private View mCategoryApp;
    private View mCategoryApk;
    private View mCategoryFav;
    private View mCategoryDoc;
    private View mCategoryArc;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mOrientation = getActivity().getResources().getConfiguration().orientation;
        mContext = this.getActivity();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.category_container,
                container, false);
        initView(rootView);

        for (int categoryType = FileUtil.CATEGORY_IMAGE; categoryType <= FileUtil.CATEGORY_APK; categoryType++) {
            if (categoryType == FileUtil.CATEGORY_BOOK)
                continue;

            TextView tv = (TextView) rootView.findViewById(
                    getTextViewIdByCategory(categoryType));
            String text = getString(getTextResIdByCategory(categoryType), 0);
            tv.setText(text);
        }
        return rootView;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        mOrientation = newConfig.orientation;
        initView(getView());
        initStoregeView(getView());
    }

    private void initView(View rootView) {
        ((ViewGroup) rootView).removeAllViews();
        View rootChild = View.inflate(getActivity(), R.layout.file_category,
                (ViewGroup) rootView);

        mCategoryImage = rootView.findViewById(R.id.category_image);
        mCategoryVideo = rootView.findViewById(R.id.category_video);
        mCategoryAudio = rootView.findViewById(R.id.category_audio);
        mCategoryApp = rootView.findViewById(R.id.category_app);
        mCategoryApk = rootView.findViewById(R.id.category_apk);
        mCategoryFav = rootView.findViewById(R.id.category_fav);
        mCategoryDoc = rootView.findViewById(R.id.category_doc);
        mCategoryArc = rootView.findViewById(R.id.category_arc);

        mCategoryImage.setTag(FileUtil.CATEGORY_IMAGE);
        mCategoryVideo.setTag(FileUtil.CATEGORY_VIDEO);
        mCategoryAudio.setTag(FileUtil.CATEGORY_AUDIO);
        mCategoryApp.setTag(FileUtil.CATEGORY_APP);
        mCategoryApk.setTag(FileUtil.CATEGORY_APK);
        mCategoryFav.setTag(FileUtil.CATEGORY_FAV);
        mCategoryDoc.setTag(FileUtil.CATEGORY_DOC);
        mCategoryArc.setTag(FileUtil.CATEGORY_ARC);

        CategotyClickListener categotyClickListener = new CategotyClickListener();
        mCategoryImage.setOnClickListener(categotyClickListener);
        mCategoryVideo.setOnClickListener(categotyClickListener);
        mCategoryAudio.setOnClickListener(categotyClickListener);
        mCategoryApp.setOnClickListener(categotyClickListener);
        mCategoryApk.setOnClickListener(categotyClickListener);
        mCategoryFav.setOnClickListener(categotyClickListener);
        mCategoryDoc.setOnClickListener(categotyClickListener);
        // category_book.setOnClickListener(categotyClickListener);
        mCategoryArc.setOnClickListener(categotyClickListener);

        CategotyTouchListener categotyTouchListener = new CategotyTouchListener();
        mCategoryImage.setOnTouchListener(categotyTouchListener);
        mCategoryVideo.setOnTouchListener(categotyTouchListener);
        mCategoryAudio.setOnTouchListener(categotyTouchListener);
        mCategoryApp.setOnTouchListener(categotyTouchListener);
        mCategoryApk.setOnTouchListener(categotyTouchListener);
        mCategoryFav.setOnTouchListener(categotyTouchListener);
        mCategoryDoc.setOnTouchListener(categotyTouchListener);
        mCategoryArc.setOnTouchListener(categotyTouchListener);

        for (Integer category : mCategoryCountMap.keySet()) {
            int count = mCategoryCountMap.get(category);

            TextView tv = (TextView) getView().findViewById(
                    getTextViewIdByCategory(category));
            String text = getString(getTextResIdByCategory(category), count);
            tv.setText(text);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        refreshCategoryInfo();
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mLoadCountTask != null
                && mLoadCountTask.getStatus() != AsyncTask.Status.FINISHED) {
            mLoadCountTask.cancel(true);
        }
        mLoadCountTask = null;
    }

    public void refreshCategoryInfo() {
        // if getView() return is null,means the fragement is not init.
        if (null == getView())
            return;
        loadCount();
        initStoregeView(getView());
    }

    private void loadCount() {
        if (mLoadCountTask != null
                && mLoadCountTask.getStatus() != AsyncTask.Status.FINISHED) {
            mLoadCountTask.cancel(true);
        }

        mLoadCountTask = new LoadCountTask();
        mLoadCountTask.execute();
    }

    private Cursor queryByCategory(int categoryType) {
        Uri uri = null;
        String selection = null;

        switch (categoryType) {
            case FileUtil.CATEGORY_IMAGE:
                uri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
                break;
            case FileUtil.CATEGORY_VIDEO:
                uri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                break;
            case FileUtil.CATEGORY_AUDIO:
                uri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
                break;
            case FileUtil.CATEGORY_DOC:
                uri = Uri.parse("content://media/external/file");
                selection = getSelectionByCategory(FileUtil.CATEGORY_DOC);
                break;
            case FileUtil.CATEGORY_ARC:
                uri = Uri.parse("content://media/external/file");
                selection = getSelectionByCategory(FileUtil.CATEGORY_ARC);
                break;
            case FileUtil.CATEGORY_FAV:
                uri = FavoriteProvider.FavoriteColumns.CONTENT_URI;
                break;
            case FileUtil.CATEGORY_APK:
                uri = Uri.parse("content://media/external/file");
                selection = MediaStore.Files.FileColumns.DATA + " like '%.apk'";
                break;
            default:
                return null;
        }
        if (uri == null
                || TextUtils.isEmpty(uri.toString() + selection)
                || (null == getActivity())) {
            return null;
        }

        Cursor cursor = null;
        try {
            cursor = getActivity().getContentResolver().query(uri,
                    new String[] {
                        BaseColumns._ID
                    }, selection, null, null);
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

        return cursor;
    }

    private int getCountByCategory(int categoryType) {
        int count = 0;
        if (FileUtil.CATEGORY_APP == categoryType)
            count = getInstalledAppCount();
        else {
            Cursor cursor = null;
            try {
                cursor = queryByCategory(categoryType);
                if (cursor != null) {
                    count = cursor.getCount();
                } else {
                    count = 0;
                }
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
        }
        return count;
    }

    private int getInstalledAppCount() {
        int count = 0;
        try {
            List<PackageInfo> packages = getActivity().getPackageManager()
                    .getInstalledPackages(0);
            for (PackageInfo installedPackage : packages) {
                if ((installedPackage.applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) == 0)
                    count++;
            }
        } catch (IllegalArgumentException e) {
            Log.d(TAG, "getInstalledAppCount failed" + e.getMessage());
        } catch (RuntimeException e) {
            Log.d(TAG, "getInstalledAppCount failed :" + e.getMessage());
        }

        return count;
    }

    public static Uri getUriByCategory(int categoryType) {
        Uri uri = null;
        switch (categoryType) {
            case FileUtil.CATEGORY_IMAGE:
                uri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
                break;
            case FileUtil.CATEGORY_VIDEO:
                uri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                break;
            case FileUtil.CATEGORY_AUDIO:
                uri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
                break;
            case FileUtil.CATEGORY_DOC:
                uri = Uri.parse("content://media/external/file");
                break;
            case FileUtil.CATEGORY_ARC:
                uri = Uri.parse("content://media/external/file");
                break;
            case FileUtil.CATEGORY_FAV:
                uri = FavoriteProvider.FavoriteColumns.CONTENT_URI;
                break;
            case FileUtil.CATEGORY_BOOK:
                uri = Uri.parse("content://media/external/file");
                break;
            case FileUtil.CATEGORY_APK:
                uri = Uri.parse("content://media/external/file");
                break;
            default:
                uri = null;
        }
        return uri;
    }

    public static String getSelectionByCategory(int categoryType) {
        String selection = null;
        switch (categoryType) {
            case FileUtil.CATEGORY_DOC:
                selection = MediaStore.Files.FileColumns.MIME_TYPE
                        + " in ("
                        + "'text/html','text/plain','application/msword','application/vnd.ms-excel',"
                        +
                        "'application/pdf','application/mspowerpoint')";
                break;
            case FileUtil.CATEGORY_ARC:
                selection = MediaStore.Files.FileColumns.DATA + " like '%.zip'";
                break;
            case FileUtil.CATEGORY_FAV:
                break;
            case FileUtil.CATEGORY_APK:
                selection = MediaStore.Files.FileColumns.DATA + " like '%.apk'";
                break;
            case FileUtil.CATEGORY_IMAGE:
            case FileUtil.CATEGORY_VIDEO:
            case FileUtil.CATEGORY_AUDIO:
            default:
                selection = null;
        }
        return selection;
    }

    public static int getTextViewIdByCategory(int categoryType) {
        switch (categoryType) {
            case FileUtil.CATEGORY_IMAGE:
                return R.id.category_image_text;
            case FileUtil.CATEGORY_VIDEO:
                return R.id.category_video_text;
            case FileUtil.CATEGORY_AUDIO:
                return R.id.category_audio_text;
            case FileUtil.CATEGORY_DOC:
                return R.id.category_doc_text;
            case FileUtil.CATEGORY_APP:
                return R.id.category_app_text;
            case FileUtil.CATEGORY_ARC:
                return R.id.category_arc_text;
            case FileUtil.CATEGORY_FAV:
                return R.id.category_fav_text;
                /*
                 * case FileUtil.CATEGORY_BOOK: return R.id.category_book_text;
                 */
            case FileUtil.CATEGORY_APK:
                return R.id.category_apk_text;
            default:
                return -1;
        }
    }

    public static int getTextResIdByCategory(int categoryType) {
        switch (categoryType) {
            case FileUtil.CATEGORY_IMAGE:
                return R.string.image;
            case FileUtil.CATEGORY_VIDEO:
                return R.string.video;
            case FileUtil.CATEGORY_AUDIO:
                return R.string.audio;
            case FileUtil.CATEGORY_DOC:
                return R.string.doc;
            case FileUtil.CATEGORY_APP:
                return R.string.app;
            case FileUtil.CATEGORY_ARC:
                return R.string.arc;
            case FileUtil.CATEGORY_FAV:
                return R.string.fav;
            case FileUtil.CATEGORY_BOOK:
                return R.string.book;
            case FileUtil.CATEGORY_APK:
                return R.string.apk;
            default:
                return -1;
        }
    }

    public static int getImageResIdByCategory(int categoryType) {
        switch (categoryType) {
            case FileUtil.CATEGORY_IMAGE:
                return R.drawable.category_icon_picture;
            case FileUtil.CATEGORY_VIDEO:
                return R.drawable.category_icon_video;
            case FileUtil.CATEGORY_AUDIO:
                return R.drawable.category_icon_music;
            case FileUtil.CATEGORY_DOC:
                return R.drawable.category_icon_document;
            case FileUtil.CATEGORY_APP:
                return R.drawable.category_icon_application;
            case FileUtil.CATEGORY_ARC:
                return R.drawable.category_icon_zip;
            case FileUtil.CATEGORY_FAV:
                return R.drawable.category_icon_favorite;
            case FileUtil.CATEGORY_BOOK:
                return R.drawable.file_icon_book;
            case FileUtil.CATEGORY_APK:
                return R.drawable.category_icon_apk;
            default:
                return -1;
        }
    }

    private class CategoryCount {
        int category;
        int count;
    }

    private class LoadCountTask extends AsyncTask {
        @Override
        protected Object doInBackground(Object... params) {
            try {
                for (int categoryType = FileUtil.CATEGORY_IMAGE; categoryType <= FileUtil.CATEGORY_APK; categoryType++) {
                    if (categoryType == FileUtil.CATEGORY_BOOK)
                        continue;
                    if (isCancelled())
                        break;

                    CategoryCount cc = new CategoryCount();
                    cc.category = categoryType;
                    cc.count = getCountByCategory(categoryType);
                    mCategoryCountMap.put(cc.category, cc.count);
                    publishProgress(cc);
                }
            } catch (NullPointerException e) {
                e.printStackTrace();
                return null;
            }
            return null;
        }

        @Override
        protected void onPostExecute(Object result) {
            super.onPostExecute(result);
        }

        @Override
        protected void onProgressUpdate(Object... values) {
            super.onProgressUpdate(values);
            try {
                for (Object value : values) {
                    if (isCancelled())
                        break;

                    CategoryCount cc = (CategoryCount) value;
                    TextView tv = (TextView) getView().findViewById(
                            getTextViewIdByCategory(cc.category));
                    String text = getString(
                            getTextResIdByCategory(cc.category), cc.count);
                    tv.setText(text);
                }
            } catch (NullPointerException e) {
                e.printStackTrace();
                return;
            }
        }
    }

    private void initStoregeView(View rootView) {
        TextView mInternalPath = (TextView) rootView.findViewById(R.id.tv_internal_path);
        if (FileUtil.getMountedSDCardCount(mContext) == 1) {
            mInternalPath.setText(R.string.fm_directory_title);
        }

        TextView mTotalSpace1 = (TextView) rootView
                .findViewById(R.id.tv_total1);
        TextView mFreeSpace1 = (TextView) rootView.findViewById(R.id.tv_free1);
        ProgressBar pb1 = (ProgressBar) rootView
                .findViewById(R.id.pb_stroge_info1);

        String path1 = FileUtil.Defs.PATH_SDCARD_1;
        String path2 = FileUtil.PATH_SDCARD_2;
        String path3 = FileUtil.PATH_SDCARD_3;

        //sdcard 1
        StatFs stat1 = new StatFs(path1);
        long blockSize1 = stat1.getBlockSize();
        long totalBlocks1 = stat1.getBlockCount();
        long availableBlocks1 = stat1.getAvailableBlocks();
        long usedBlocks1 = totalBlocks1 - availableBlocks1;

        int usedPercentage1 = 0;
        if (0 != totalBlocks1) {
            usedPercentage1 = (int) (usedBlocks1 * 100 / totalBlocks1);
        }

        pb1.setProgress(usedPercentage1);

        pb1 = (ProgressBar) rootView.findViewById(R.id.pb_stroge_info1);
        if (usedPercentage1 <= STORAGE_MID) {
            pb1.setProgressDrawable(getActivity().getResources().getDrawable(
                    R.drawable.progressbar_green));
        } else if (usedPercentage1 > STORAGE_MID
                && usedPercentage1 <= STORAGE_WAR) {
            pb1.setProgressDrawable(getActivity().getResources().getDrawable(
                    R.drawable.progressbar_yellow));
        } else {
            pb1.setProgressDrawable(getActivity().getResources().getDrawable(
                    R.drawable.progressbar_red));
        }

        mTotalSpace1.setText(getActivity().getString(R.string.storege_total)
                + (Formatter.formatFileSize(mContext, blockSize1 * totalBlocks1)));
        mFreeSpace1.setText(getActivity().getString(R.string.storege_free)
                + Formatter.formatFileSize(mContext, blockSize1 * availableBlocks1));

        View storageInfo2 = rootView.findViewById(R.id.usage_info2);
        View storageInfo3 = rootView.findViewById(R.id.usage_info3);
        if (FileUtil.getMountedSDCardCount(getActivity()) <= 1) {
            storageInfo2.setVisibility(View.GONE);
            storageInfo3.setVisibility(View.GONE);
            return;
        } else {
            storageInfo2.setVisibility(View.VISIBLE);
        }
        //sdcard 2
        TextView mTotalSpace2 = (TextView) rootView
                .findViewById(R.id.tv_total2);
        TextView mFreeSpace2 = (TextView) rootView.findViewById(R.id.tv_free2);
        ProgressBar pb2 = (ProgressBar) rootView
                .findViewById(R.id.pb_stroge_info2);

        // android not support two sdcard, to support it, some system define
        // the second sdcard path by themselves, this may lead to some path is
        // invalid
        long blockSize2 = 0, totalBlocks2 = 0, availableBlocks2 = 0;
        try {
            StatFs stat2 = new StatFs(path2);
            blockSize2 = stat2.getBlockSize();
            totalBlocks2 = stat2.getBlockCount();
            availableBlocks2 = stat2.getAvailableBlocks();
        } catch (IllegalArgumentException e) {
            Log.d(TAG, "the path is invalid");
        }

        long usedBlocks2 = totalBlocks2 - availableBlocks2;
        int usedPercentage2 = 0;
        if (0 != totalBlocks2) {
            usedPercentage2 = (int) (usedBlocks2 * 100 / totalBlocks2);
        }

        pb2.setProgress(usedPercentage2);
        if (usedPercentage2 <= STORAGE_MID) {
            pb2.setProgressDrawable(getActivity().getResources().getDrawable(
                    R.drawable.progressbar_green));
        } else if (usedPercentage2 > STORAGE_MID
                && usedPercentage2 <= STORAGE_WAR) {
            pb2.setProgressDrawable(getActivity().getResources().getDrawable(
                    R.drawable.progressbar_yellow));
        } else {
            pb2.setProgressDrawable(getActivity().getResources().getDrawable(
                    R.drawable.progressbar_red));
        }

        mTotalSpace2.setText(getActivity().getString(R.string.storege_total)
                + (Formatter.formatFileSize(mContext, blockSize2 * totalBlocks2)));
        mFreeSpace2.setText(getActivity().getString(R.string.storege_free)
                + Formatter.formatFileSize(mContext, blockSize2 * availableBlocks2));

        if (FileUtil.getMountedSDCardCount(getActivity()) <= 2) {
            storageInfo3.setVisibility(View.GONE);
            return;
        } else {
            storageInfo3.setVisibility(View.VISIBLE);
        }

        //sdcard 3
        TextView mTotalSpace3 = (TextView) rootView
                .findViewById(R.id.tv_total3);
        TextView mFreeSpace3 = (TextView) rootView.findViewById(R.id.tv_free3);
        ProgressBar pb3 = (ProgressBar) rootView
                .findViewById(R.id.pb_stroge_info3);

        // android not support two sdcard, to support it, some system define
        // the second sdcard path by themselves, this may lead to some path is
        // invalid
        long blockSize3 = 0, totalBlocks3 = 0, availableBlocks3 = 0;
        try {
            StatFs stat3 = new StatFs(path3);
            blockSize3 = stat3.getBlockSize();
            totalBlocks3 = stat3.getBlockCount();
            availableBlocks3 = stat3.getAvailableBlocks();
        } catch (IllegalArgumentException e) {
            Log.d(TAG, "the path is invalid");
        }

        long usedBlocks3 = totalBlocks3 - availableBlocks3;
        int usedPercentage3 = 0;
        if (0 != totalBlocks3) {
            usedPercentage3 = (int) (usedBlocks3 * 100 / totalBlocks3);
        }

        pb3.setProgress(usedPercentage3);
        if (usedPercentage3 <= STORAGE_MID) {
            pb3.setProgressDrawable(getActivity().getResources().getDrawable(
                    R.drawable.progressbar_green));
        } else if (usedPercentage3 > STORAGE_MID
                && usedPercentage3 <= STORAGE_WAR) {
            pb3.setProgressDrawable(getActivity().getResources().getDrawable(
                    R.drawable.progressbar_yellow));
        } else {
            pb3.setProgressDrawable(getActivity().getResources().getDrawable(
                    R.drawable.progressbar_red));
        }

        mTotalSpace3.setText(getActivity().getString(R.string.storege_total)
                + (Formatter.formatFileSize(mContext, blockSize3 * totalBlocks3)));
        mFreeSpace3.setText(getActivity().getString(R.string.storege_free)
                + Formatter.formatFileSize(mContext, blockSize3 * availableBlocks3));
    }

    private class CategotyClickListener implements View.OnClickListener {
        @Override
        public void onClick(View v) {
            int category = (Integer) v.getTag();
            Intent intent = new Intent();
            intent.setClassName(SubCategoryActivity.class.getPackage()
                    .getName(), SubCategoryActivity.class.getName());
            intent.putExtra(SubCategoryActivity.EXTRA_CATEGORY, category);
            intent.putExtra(SubCategoryActivity.EXTRA_SELECTION,
                    getSelectionByCategory(category));
            intent.setData(getUriByCategory(category));
            startActivity(intent);
        }
    }

    private class CategotyTouchListener implements View.OnTouchListener {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                setAllCategoryEnabled(false);
                v.setEnabled(true);
            } else if (event.getAction() == MotionEvent.ACTION_UP
                    || event.getAction() == MotionEvent.ACTION_CANCEL) {
                setAllCategoryEnabled(true);
            }
            return false;
        }

        private void setAllCategoryEnabled(boolean enable) {
            mCategoryImage.setEnabled(enable);
            mCategoryVideo.setEnabled(enable);
            mCategoryAudio.setEnabled(enable);
            mCategoryApp.setEnabled(enable);
            mCategoryApk.setEnabled(enable);
            mCategoryFav.setEnabled(enable);
            mCategoryDoc.setEnabled(enable);
            mCategoryArc.setEnabled(enable);
        }

    }

}
