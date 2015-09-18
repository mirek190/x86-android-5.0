
package com.intel.filemanager;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.AsyncQueryHandler;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.CursorIndexOutOfBoundsException;
import android.database.MatrixCursor;
import android.database.StaleDataException;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.SparseBooleanArray;
import android.view.ActionMode;
import android.view.Display;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.CursorAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FMThumbnailHelper;
import com.intel.filemanager.util.FileDBOP;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.LogHelper;
import com.intel.filemanager.util.MediaFile;
import com.intel.filemanager.view.OperationView;
import com.intel.filemanager.view.OperationView.OperationListener;

public class SubCategoryActivity extends ListActivity {
    private static final String TAG = "SubCategoryActivity";
    private static LogHelper Log = LogHelper.getLogger();

    public static final String EXTRA_CATEGORY = "extra_category";
    public static final String EXTRA_URI = "extra_uri";
    public static final String EXTRA_SELECTION = "extra_selection";

    private static final String CATEGORY_SORT_MODE = "category_sort_mode";
    private static final String SIZE = "Size";

    private static final int LOAD_THUMBNAIL = 1;
    private static final int UPDATE_WEIGHT = 2;
    private static final int UPDATE_CURSOR = 3;

    private static final int UPDATE_INTERVAL = 500;
    private static final int LOAD_THUMBNAIL_INTERVAL = 300;
    private Context mContext;

    private ThumbnailTask mThumbnailTask;
    private LoadAppTask mLoadAppTask;

    private Cursor mCursor;
    private FileListAdapter mAdapter;
    private QueryHandler mQueryHandler;

    private int mCategory;
    private Uri mUri;
    private String mSelection;

    private ListView mListView;
    private Operation mCurOp;
    private ModeCallback mModeCallback;
    private ActionMode mMode;
    private int mSortMode;

    public static int SCREEN_WIDTH;
    public static int SCREEN_HEIGHT;

    private SharedPreferences mPreference;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == LOAD_THUMBNAIL) {
                if (mThumbnailTask != null
                        && mThumbnailTask.getStatus() != AsyncTask.Status.FINISHED) {
                    mThumbnailTask.cancel(true);
                }
                mThumbnailTask = new ThumbnailTask();
                mThumbnailTask.execute();
            } else if (msg.what == UPDATE_WEIGHT) {
                String path = (String) msg.obj;
                if (!TextUtils.isEmpty(path)) {
                    updateWeight(path);
                }
            } else if (msg.what == UPDATE_CURSOR) {
                query(mSortMode);
            }
        }
    };

    private class LoadAppTask extends AsyncTask {

        @Override
        protected Object doInBackground(Object... params) {
            mCursor = queryApp();
            return null;
        }

        @Override
        protected void onPostExecute(Object result) {
            initEmptyView();
            setTitle(getResources().getString(
                    FileCategoryFragment.getTextResIdByCategory(mCategory), mCursor.getCount()));
            mAdapter.changeCursor(mCursor);
            loadThumbnail();
        }
    }

    private class QueryHandler extends AsyncQueryHandler {

        public QueryHandler(ContentResolver cr) {
            super(cr);
        }

        @Override
        public void startQuery(int token, Object cookie, Uri uri,
                String[] projection, String selection, String[] selectionArgs,
                String orderBy) {
            super.startQuery(token, cookie, uri, projection, selection,
                    selectionArgs, orderBy);
        }

        @Override
        protected void onQueryComplete(int token, Object cookie, Cursor cursor) {
            initEmptyView();
            if (cursor != null && cursor.getCount() >= 0) {
                mCursor = cursor;
                setTitle(getResources().getString(
                        FileCategoryFragment.getTextResIdByCategory(mCategory), mCursor.getCount()));
                mAdapter.changeCursor(mCursor);
                loadThumbnail();
            }
        }

    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        getActionBar().setDisplayShowHomeEnabled(true);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setDisplayShowTitleEnabled(true);
        Intent intent = getIntent();
        mUri = intent.getData();
        mCategory = intent.getIntExtra(EXTRA_CATEGORY, 0);
        mSelection = intent.getStringExtra(EXTRA_SELECTION);

        setTitle(getResources().getString(
                FileCategoryFragment.getTextResIdByCategory(mCategory), 0));

        mPreference = PreferenceManager.getDefaultSharedPreferences(this);

        mAdapter = new FileListAdapter(this, null);
        setListAdapter(mAdapter);
        mModeCallback = new ModeCallback();

        mListView = getListView();

        mListView.setAdapter(mAdapter);
        mListView.setOnScrollListener(mAdapter);
        mListView.setOnItemClickListener(mItemClickListener);
        if (mCategory == FileUtil.CATEGORY_APP) {
            mListView.setChoiceMode(ListView.CHOICE_MODE_NONE);
        } else {
            mListView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE_MODAL);
            mListView.setMultiChoiceModeListener(mModeCallback);
        }

        mSortMode = mPreference.getInt(CATEGORY_SORT_MODE, FileUtil.SORT_BY_NAME);

        if (mCategory == FileUtil.CATEGORY_APP) {
            mLoadAppTask = new LoadAppTask();
            mLoadAppTask.execute();
        } else {
            query(mSortMode);
        }

        registerIntent();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        // TODO Auto-generated method stub
        super.onConfigurationChanged(newConfig);

        WindowManager windowManager = getWindowManager();
        Display display = windowManager.getDefaultDisplay();
        FileUtil.SCREEN_WIDTH = display.getWidth();
        FileUtil.SCREEN_HEIGHT = display.getHeight();

        mAdapter.notifyConfigChange();
    }

    private void query(int sortMode) {
        String orderBy = null;
        switch (sortMode) {
            case FileUtil.SORT_BY_NAME:
                if (mCategory == FileUtil.CATEGORY_IMAGE ||
                        mCategory == FileUtil.CATEGORY_AUDIO ||
                        mCategory == FileUtil.CATEGORY_VIDEO) {
                    orderBy = MediaStore.MediaColumns.DISPLAY_NAME;
                } else {
                    orderBy = MediaStore.MediaColumns.TITLE;
                }
                break;
            case FileUtil.SORT_BY_SIZE:
                orderBy = MediaStore.MediaColumns.SIZE;
                break;
            case FileUtil.SORT_BY_TIME:
                orderBy = MediaStore.MediaColumns.DATE_ADDED + " desc";
                break;
            case FileUtil.SORT_BY_TYPE:
                if (mCategory == FileUtil.CATEGORY_FAV) {
                    orderBy = FavoriteProvider.FavoriteColumns.CATEGORY;
                } else if (mCategory == FileUtil.CATEGORY_APK) {
                    orderBy = MediaStore.MediaColumns.TITLE;
                } else {
                    orderBy = MediaStore.MediaColumns.MIME_TYPE;
                }
                break;
            default:
                orderBy = MediaStore.MediaColumns.TITLE;
        }
        startQuery(orderBy);
    }

    private void startQuery(String orderBy) {
        String[] projection = null;
        String order = null;
        if (mCategory != FileUtil.CATEGORY_FAV) {
            projection = new String[] {
                    MediaStore.MediaColumns._ID,
                    MediaStore.MediaColumns.DATA, MediaStore.MediaColumns.TITLE
            };
            order = orderBy;
        } else {
            order = FavoriteProvider.FavoriteColumns.WEIGHT + " desc";
        }
        mQueryHandler = new QueryHandler(getContentResolver());
        mQueryHandler.startQuery(mCategory, null, mUri, projection, mSelection,
                null, order);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!FileUtil.checkIfSDExist(this)) {
            Toast.makeText(this, R.string.sdcard_unmount, Toast.LENGTH_SHORT).show();
            this.finish();
            return;
        }

        // for application install or uninstall
        if (mCategory == FileUtil.CATEGORY_APP) {
            mLoadAppTask = new LoadAppTask();
            mLoadAppTask.execute();
        }
    }

    protected BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.v(TAG, "SubCategoryActivity receive intent:" + intent.toString());
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
            } else if (action.equals(Intent.ACTION_MEDIA_REMOVED)
                    || action.equals(Intent.ACTION_MEDIA_EJECT)
                    || action.equals(Intent.ACTION_MEDIA_UNMOUNTED)) {
                finish();
            } else if (action.equals(Intent.ACTION_MEDIA_SCANNER_FINISHED)) {
                loadThumbnail();
            }
        }
    };

    private void registerIntent() {
        IntentFilter intentFilter = new IntentFilter(
                Intent.ACTION_MEDIA_MOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
        intentFilter.addAction(Intent.ACTION_MEDIA_REMOVED);
        intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_FINISHED);
        intentFilter.addDataScheme("file");
        this.registerReceiver(mReceiver, intentFilter);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.options_subcategory, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        menu.findItem(R.id.action_order).setVisible(
                mCategory != FileUtil.CATEGORY_APP
                        && mCategory != FileUtil.CATEGORY_FAV);
        menu.findItem(R.id.action_order_by_type).setVisible(
                mCategory != FileUtil.CATEGORY_APP
                        && mCategory != FileUtil.CATEGORY_FAV);
        menu.findItem(R.id.action_order_by_name).setVisible(
                mCategory != FileUtil.CATEGORY_FAV);
        switch (mPreference.getInt(CATEGORY_SORT_MODE, FileUtil.SORT_BY_NAME)) {
            case 1:
                menu.findItem(R.id.action_order_by_name).setChecked(true);
                break;
            case 2:
                menu.findItem(R.id.action_order_by_size).setChecked(true);
                break;
            case 3:
                menu.findItem(R.id.action_order_by_time).setChecked(true);
                break;
            case 4:
                menu.findItem(R.id.action_order_by_type).setChecked(true);
                break;
        }
        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_order_by_name: // sort by name
                mSortMode = FileUtil.SORT_BY_NAME;
                query(FileUtil.SORT_BY_NAME);
                mPreference.edit().putInt(CATEGORY_SORT_MODE, mSortMode).commit();
                break;
            case R.id.action_order_by_size: // sort by size
                mSortMode = FileUtil.SORT_BY_SIZE;
                query(FileUtil.SORT_BY_SIZE);
                mPreference.edit().putInt(CATEGORY_SORT_MODE, mSortMode).commit();
                break;
            case R.id.action_order_by_time: // sort by date
                mSortMode = FileUtil.SORT_BY_TIME;
                query(FileUtil.SORT_BY_TIME);
                mPreference.edit().putInt(CATEGORY_SORT_MODE, mSortMode).commit();
                break;
            case R.id.action_order_by_type: // sort by type
                mSortMode = FileUtil.SORT_BY_TYPE;
                query(FileUtil.SORT_BY_TYPE);
                mPreference.edit().putInt(CATEGORY_SORT_MODE, mSortMode).commit();
                break;
            case android.R.id.home:
                finish();
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onDestroy() {
        if (mHandler != null) {
            mHandler.removeMessages(LOAD_THUMBNAIL);
        }

        if (mThumbnailTask != null
                && mThumbnailTask.getStatus() != AsyncTask.Status.FINISHED) {
            mThumbnailTask.cancel(true);
        }
        if (mQueryHandler != null) {
            mQueryHandler.cancelOperation(mCategory);
        }
        if (mCursor != null && !mCursor.isClosed()) {
            mCursor.close();
        }

        try {
            unregisterReceiver(mReceiver);
        } catch (Exception e) {
            Log.e(TAG, "unregisterReceiver error when onDestroy");
        }

        if (null != mCurOp)
            mCurOp.setExit();
        if (null != mAdapter)
            mAdapter.setExitOperation();

        super.onDestroy();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (event.getKeyCode()) {
            case KeyEvent.KEYCODE_BACK:
                if (mAdapter.isOperationShow()) {
                    mAdapter.hideOperation();
                    return true;
                }
            case KeyEvent.KEYCODE_MENU:
                if (mAdapter.isOperationShow()) {
                    mAdapter.hideOperation();
                }
                if (mMode != null) {
                    mMode.finish();
                }

                break;
        }
        return super.onKeyDown(keyCode, event);

    }

    public static int getEmptyTextResIdByCategory(int categoryType) {
        switch (categoryType) {
            case FileUtil.CATEGORY_IMAGE:
                return R.string.no_image;
            case FileUtil.CATEGORY_VIDEO:
                return R.string.no_video;
            case FileUtil.CATEGORY_AUDIO:
                return R.string.no_audio;
            case FileUtil.CATEGORY_DOC:
                return R.string.no_doc;
            case FileUtil.CATEGORY_APP:
                return R.string.no_app;
            case FileUtil.CATEGORY_ARC:
                return R.string.no_arc;
            case FileUtil.CATEGORY_FAV:
                return R.string.no_fav;
            case FileUtil.CATEGORY_BOOK:
                return R.string.no_book;
            case FileUtil.CATEGORY_APK:
                return R.string.no_apk;
            default:
                return -1;
        }
    }

    private void initEmptyView() {
        TextView emptyView = new TextView(mContext);
        emptyView.setText(getEmptyTextResIdByCategory(mCategory));
        emptyView.setTextAppearance(mContext, R.style.empty_text_style);
        emptyView.setGravity(Gravity.CENTER);
        ViewGroup.LayoutParams layoutParams = new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT);
        emptyView.setLayoutParams(layoutParams);
        emptyView.setVisibility(View.GONE);
        ((ViewGroup) mListView.getParent()).addView(emptyView);
        mListView.setEmptyView(emptyView);
    }

    private Cursor queryApp() {
        System.setProperty("java.util.Arrays.useLegacyMergeSort", "true");
        MatrixCursor cursor = new MatrixCursor(new String[] {
                MediaStore.MediaColumns._ID, MediaStore.MediaColumns.DATA,
                MediaStore.MediaColumns.TITLE
        });

        try {
            final PackageManager pm = getPackageManager();
            List<PackageInfo> packages = pm.getInstalledPackages(0);

            // sort the packages by label
            Collections.sort(packages, new Comparator<PackageInfo>() {
                @Override
                public int compare(PackageInfo lhs, PackageInfo rhs) {
                    String label1 = (String) lhs.applicationInfo.loadLabel(pm);
                    String label2 = (String) rhs.applicationInfo.loadLabel(pm);
                    return label1.compareTo(label2);
                }

            });

            int i = 0;
            for (PackageInfo info : packages) {
                if ((info.applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) != 0)
                    continue;
                cursor.addRow(new Object[] {
                        i, info.packageName,
                        info.applicationInfo.loadLabel(pm)
                });
                i++;
            }
        } catch (RuntimeException e) {
            Log.e(TAG, e.getMessage());
        }

        return cursor;
    }

    private Map<String, Integer> getVisibleItems() {
        Map<String, Integer> items = new HashMap<String, Integer>();
        int start = getListView().getFirstVisiblePosition();
        int count = getListView().getChildCount();
        if (start == -1) { // when items not display, may return -1
            if (count > 0)
                loadThumbnail();
            return items;
        }

        for (int i = 0; i < count; i++) {
            try {
                mCursor.moveToPosition(start + i);
                String path = mCursor.getString(mCursor
                        .getColumnIndex(MediaStore.MediaColumns.DATA));
                int category;
                if (mCategory == FileUtil.CATEGORY_FAV) {
                    category = mCursor
                            .getInt(mCursor
                                    .getColumnIndex(FavoriteProvider.FavoriteColumns.CATEGORY));
                } else {
                    category = mCategory;
                }
                items.put(path, category);
            } catch (CursorIndexOutOfBoundsException e) {
                return items;
            } catch (StaleDataException e) {
                Log.e(TAG, e.getMessage());
                return items;
            } catch (IllegalStateException e) {
                Log.e(TAG, e.getMessage());
                return items;
            } catch (NullPointerException e) {
                return items;
            }
        }
        return items;
    }

    private synchronized void loadThumbnail() {
        mHandler.removeMessages(LOAD_THUMBNAIL);
        Message loadThumbnail = mHandler.obtainMessage(LOAD_THUMBNAIL);
        mHandler.sendMessageDelayed(loadThumbnail, LOAD_THUMBNAIL_INTERVAL);
    }

    private void updateWeight(String path) {
        path = path.replaceAll("'", "''");
        String wherestr = MediaStore.MediaColumns.DATA + " = '" + path + "'";
        Cursor cursor = mContext.getContentResolver().query(
                FavoriteProvider.FavoriteColumns.CONTENT_URI, null,
                wherestr, null, null);
        int rowid = -1;
        if (cursor.moveToFirst()) {
            rowid = cursor.getInt(cursor.getPosition());
        }
        try {
            int weight = cursor.getInt(cursor
                    .getColumnIndex(FavoriteProvider.FavoriteColumns.WEIGHT)) + 1;
            cursor.close();

            ContentValues values = new ContentValues();
            values.put(FavoriteProvider.FavoriteColumns.WEIGHT, weight);
            mContext.getContentResolver().update(
                    Uri.withAppendedPath(
                            FavoriteProvider.FavoriteColumns.CONTENT_URI,
                            String.valueOf(rowid)), values, null, null);
        } catch (CursorIndexOutOfBoundsException e) {
            Log.e(TAG, e.getMessage());
        }
    }

    private OnItemClickListener mItemClickListener = new OnItemClickListener() {

        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position,
                long id) {

            if (null == mCursor)
                return;
            mCursor.moveToPosition(position);
            String path = mCursor.getString(mCursor
                    .getColumnIndex(MediaStore.MediaColumns.DATA));

            int category = mCategory;

            if (mCategory == FileUtil.CATEGORY_FAV) {
                Message msg = mHandler.obtainMessage(UPDATE_WEIGHT);
                msg.obj = path;
                mHandler.sendMessageDelayed(msg, 1000);
            }
            File file = new File(path);
            if (category == FileUtil.CATEGORY_APP) {
                launchApp(path);
            } else if (file.isDirectory()) {
                openDir(path);
            } else {
                openCommonFile(path);
            }
        }

        private void launchApp(String packageName) {
            Intent appIntent = getPackageManager().getLaunchIntentForPackage(
                    packageName);
            // when the app is Background program, the appIntent maybe null
            if (null == appIntent) {
                Toast.makeText(mContext, R.string.launch_app_error,
                        Toast.LENGTH_SHORT).show();
                return;
            }
            startActivity(appIntent);
        }

        private void openDir(String path) {
            FileUtil.openFolder(mContext, path);
        }

        private void openCommonFile(String path) {
            Uri fileUri = Uri.fromFile(new File(path));
            String mimetype = MediaFile.getMimeType(path);

            Intent fileIntent = new Intent();
            fileIntent.setAction(Intent.ACTION_VIEW);
            fileIntent.setDataAndType(fileUri, mimetype);
            List<ResolveInfo> mResList = getPackageManager()
                    .queryIntentActivities(fileIntent,
                            PackageManager.MATCH_DEFAULT_ONLY);
            if ((null == mResList) || (mResList.isEmpty())) {
                if (path.toLowerCase().endsWith(".zip")) {
                    openCompressFile(mContext, path);
                } else {
                    Toast.makeText(mContext, R.string.fm_cannot_open_file,
                            Toast.LENGTH_SHORT).show();
                }
                return;
            }
            startActivity(fileIntent);
        }

        private void openCompressFile(Context context, final String path) {
            AlertDialog dialog = new AlertDialog.Builder(context)
                    .setTitle(R.string.dialog_title_unzip)
                    .setMessage(R.string.dialog_msg_unzip)
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            startZipOperation(path);
                        }
                    }).setNegativeButton(android.R.string.cancel, null).create();
            dialog.show();
        }

        private void startZipOperation(String path) {
            if (null != mCurOp)
                mCurOp.setExit();
            mCurOp = new OperationUnzip(mContext, new File(path).getParent(), path);
            mCurOp.setOnOperationEndListener(mAdapter);
            mCurOp.start();
        }

    };

    private class FileListAdapter extends CursorAdapter implements
            ListView.OnScrollListener, OperationListener {
        private LayoutInflater mInflater;
        private String mSelectPath;
        private OperationView mOperationView;
        private boolean mIsOperationShow;
        private PackageManager mPackageManager;
        private boolean mInActionMode = false;

        public FileListAdapter(Context context, Cursor c) {
            super(context, c);
            mPackageManager = context.getPackageManager();
            mInflater = LayoutInflater.from(context);
        }

        public void notifyConfigChange() {
            if (mOperationView == null || mOperationView.getParent() == null)
                return;
            View view = (View) mOperationView.getParent();
            int category = mOperationView.getCateory();
            String path = mOperationView.getPath();
            mSelectPath = path;
            int position = mOperationView.getPosition();
            removeOperationView();
            addOperationView(view, category, path, position);

        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            if (mCategory == FileUtil.CATEGORY_VIDEO
                    || mCategory == FileUtil.CATEGORY_IMAGE
                    || mCategory == FileUtil.CATEGORY_AUDIO) {
                return mInflater.inflate(R.layout.file_list_item_big, parent,
                        false);
            } else {
                return mInflater
                        .inflate(R.layout.file_list_item, parent, false);
            }
        }

        @Override
        public void bindView(final View view, Context context,
                final Cursor cursor) {
            ImageView iv = (ImageView) view.findViewById(R.id.icon);
            TextView tv = (TextView) view.findViewById(R.id.file_name);
            TextView stv = (TextView) view.findViewById(R.id.file_size);
            TextView ttv = (TextView) view.findViewById(R.id.file_modiftime);
            ImageView expander = (ImageView) view.findViewById(R.id.expander);
            long fileSize = 0;

            boolean ifDisplayFileSize = false;

            if (FileMainActivity.isFirstLaunch == true) {
                ifDisplayFileSize = PreferenceManager.getDefaultSharedPreferences(mContext)
                        .getBoolean("displayfilesize", true);
            }

            try {
                final String path = cursor.getString(cursor
                        .getColumnIndex(MediaStore.MediaColumns.DATA));
                if (mCategory == FileUtil.CATEGORY_APP) {
                    String appName = cursor.getString(cursor
                            .getColumnIndex(MediaStore.MediaColumns.TITLE));
                    tv.setText(appName);
                } else {
                    int nameIndex = path.lastIndexOf(File.separatorChar);
                    tv.setText(path.substring(nameIndex + 1));

                }

                final int category;
                final int position = cursor.getPosition();

                category = mCategory;

                expander.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        File clickFile = new File(path);
                        if (!clickFile.exists()
                                && (mCategory != FileUtil.CATEGORY_APP)) {
                            FileDBOP.deleteFileInDB(mContext, path);
                            Toast.makeText(mContext, R.string.error_file_not_exist,
                                    Toast.LENGTH_SHORT).show();
                            return;
                        }

                        if (TextUtils.isEmpty(mSelectPath)
                                || !path.equals(mSelectPath)) {
                            mSelectPath = path;
                            removeOperationView();
                            addOperationView(view, category, path, position);
                            checkPositionAndScroll(position);
                        } else {
                            mSelectPath = null;
                            removeOperationView();
                        }
                        if (mMode != null) {
                            mMode.finish();
                        }
                    }
                });
                File file = new File(path);
                Date date = new Date(file.lastModified());

                String format = Settings.System.getString(mContext.getContentResolver(),
                        Settings.System.DATE_FORMAT);
                if (TextUtils.isEmpty(format)) {
                    format = "yyyy-MM-dd HH:mm:ss";
                }
                SimpleDateFormat formatter;

                formatter = new SimpleDateFormat(format);

                String strDate = formatter.format(date);

                if (file != null && mCategory != FileUtil.CATEGORY_APP) {
                    fileSize = file.length();
                    stv.setText(FileUtil.formatSize(fileSize));
                    ttv.setText(strDate);
                    stv.setVisibility(View.VISIBLE);
                    ttv.setVisibility(View.VISIBLE);
                    if (mCategory == FileUtil.CATEGORY_FAV && file.isDirectory()) {
                        ttv.setText(strDate);
                        stv.setVisibility(View.GONE);
                    }
                } else {
                    stv.setVisibility(View.GONE);
                    ttv.setVisibility(View.GONE);
                }

                if (file != null && file.isDirectory()) {
                    iv.setImageResource(R.drawable.file_icon_folder);
                } else {
                    Drawable d = null;
                    if (null != FMThumbnailHelper.getThumbnailCathe()) {
                        d = (Drawable) FMThumbnailHelper.getThumbnailCathe().get(path);
                    }
                    if (d != null) {
                        iv.setImageDrawable(d);
                    } else {
                        Drawable icon = getDefaultIconByCategory(mCategory);
                        if (null == icon) {
                            iv.setImageResource(MediaFile.getIconForPath(path));
                        } else {
                            iv.setImageDrawable(icon);
                        }
                    }
                }

                // the view of list item will be reused
                // add the operation view into the item that have added the view
                // before
                if (mOperationView != null) {
                    ViewGroup parent = (ViewGroup) mOperationView.getParent();
                    if (parent == view && !TextUtils.isEmpty(mSelectPath)
                            && !path.equals(mSelectPath)) {
                        removeOperationView();
                    } else if (parent == null && path.equals(mSelectPath)) {
                        addOperationView(view, category, mSelectPath,
                                cursor.getPosition());
                    }

                    if (mListView.isItemChecked(cursor.getPosition()))
                        mOperationView = null;
                }
            } catch (IllegalStateException e) {
                Log.d(TAG, "cursor is closed here");
            }
        }

        private void checkPositionAndScroll(final int position) {
            if (position == mListView.getLastVisiblePosition()
                    || position == mListView.getLastVisiblePosition() - 1) {
                Log.d(TAG,
                        "Show operations of the last item, scroll the list view,  position: "
                                + position);
                mListView.invalidate();
                mListView.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        mListView.smoothScrollToPosition(position);
                    }
                }, 100);
            }
        }

        public Drawable getDefaultIconByCategory(int categoryType) {
            Drawable icon = null;
            Resources resources = mContext.getResources();

            switch (categoryType) {
                case FileUtil.CATEGORY_IMAGE:
                    icon = resources.getDrawable(R.drawable.file_icon_image);
                    break;
                case FileUtil.CATEGORY_VIDEO:
                    icon = resources.getDrawable(R.drawable.file_icon_video);
                    break;
                case FileUtil.CATEGORY_AUDIO:
                    icon = resources.getDrawable(R.drawable.file_icon_audio);
                    break;
                case FileUtil.CATEGORY_APP:
                case FileUtil.CATEGORY_APK:
                    icon = mContext.getPackageManager().getDefaultActivityIcon();
                    break;
                default:
                    break;
            }

            return icon;
        }

        @Override
        public void onScrollStateChanged(AbsListView view, int scrollState) {
            if (scrollState == SCROLL_STATE_IDLE) {
                loadThumbnail();
            }
        }

        @Override
        public void onScroll(AbsListView view, int firstVisibleItem,
                int visibleItemCount, int totalItemCount) {
        }

        @Override
        public void onOperationClicked(int opType, int position) {
            removeOperationView();
            mSelectPath = null;
            mCurOp = mOperationView.getCurrentOp();
        }

        @Override
        public void onOperationEnd() {
            try {
                if (null != getListView()) {
                    if (null != mCursor) {
                        setTitle(getResources().getString(
                                FileCategoryFragment.getTextResIdByCategory(mCategory),
                                mCursor.getCount()));
                    }
                    loadThumbnail();
                }
            } catch (NullPointerException e) {
                Log.d(TAG, "" + e.getMessage());
                return;
            } catch (StaleDataException e) {
                Log.d(TAG,
                        "onOperationEnd is called after doinbackground, here curosr maybe already closed.");
                return;
            }
        }

        private void addOperationView(View view, int category, String path,
                int position) {

            if (mOperationView == null) {
                mOperationView = new OperationView(mContext,
                        OperationView.TYPE_CATEGORY_LIST, path, null);
                mOperationView.setOperationListener(this);
            }
            mOperationView.update(category, path, position);
            if (!(view instanceof ViewGroup))
                return;

            ((ViewGroup) view).addView(mOperationView);
            ((ImageView) view.findViewById(R.id.expander))
                    .setImageResource(R.drawable.expander_close);
            mIsOperationShow = true;
        }

        private void removeOperationView() {
            if (mOperationView == null || mOperationView.getParent() == null)
                return;
            ViewGroup parent = (ViewGroup) mOperationView.getParent();
            ((ImageView) parent.findViewById(R.id.expander))
                    .setImageResource(R.drawable.expander_open);
            parent.removeView(mOperationView);
            mIsOperationShow = false;
        }

        public boolean isOperationShow() {
            return mIsOperationShow;
        }

        public void hideOperation() {
            mSelectPath = null;
            removeOperationView();
        }

        public void setActionMode(boolean actionMode) {
            mInActionMode = actionMode;
        }

        @Override
        protected void onContentChanged() {
            if (mHandler.hasMessages(UPDATE_CURSOR)) {
                mHandler.removeMessages(UPDATE_CURSOR);
            }

            Message updateCursor = mHandler.obtainMessage(UPDATE_CURSOR);
            mHandler.sendMessageDelayed(updateCursor, UPDATE_INTERVAL);
        }

        public void setExitOperation() {
            if ((null != mOperationView) && (null != mOperationView.getCurrentOp())) {
                mOperationView.getCurrentOp().setExit();
            }
        }
    }

    private class ModeCallback implements ListView.MultiChoiceModeListener {
        private View mMultiSelectActionBarView;
        private ImageView mMultiSelect;
        private boolean mAllSelected;
        private int mSelectedFolderCount = 0;
        private Menu mMenu;

        @Override
        public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
            String paths = getSelectedPath();
            switch (item.getItemId()) {
                case R.id.modal_action_copy:
                    FileOperator.copy(paths, mContext);
                    break;
                case R.id.modal_action_cut:
                    FileOperator.cut(paths, mContext);
                    break;
                case R.id.modal_action_delete:
                    FileOperator.isDeleteFiles = false;
                    FileOperator.isCanPaste = false;
                    if (null != mCurOp)
                        mCurOp.setExit();
                    mCurOp = new OperationDelete(mContext,
                            OperationDelete.OP_MULT_DELETE_CONFIRM, paths);
                    mCurOp.setOnOperationEndListener(mAdapter);
                    mCurOp.start();
                    break;
                case R.id.modal_action_multi_share:
                    FileOperator.doShare(paths, mContext);
                    break;
                case R.id.modal_action_fav:
                    if (mCategory == FileUtil.CATEGORY_FAV) {
                        for (String path : paths.split(File.pathSeparator)) {
                            FileOperator.deleteFavorite(path, mContext);
                        }
                        Toast.makeText(mContext, R.string.del_fav_success, Toast.LENGTH_SHORT)
                                .show();
                    } else {
                        for (String path : paths.split(File.pathSeparator)) {
                            FileOperator.addFavorite(path, mCategory, mContext);
                        }
                        Toast.makeText(mContext, R.string.add_fav_success, Toast.LENGTH_SHORT)
                                .show();
                    }
                    break;
                default:
                    break;
            }
            mode.finish();
            return true;
        }

        @Override
        public boolean onCreateActionMode(ActionMode mode, Menu menu) {
            MenuInflater inflater = getMenuInflater();
            inflater.inflate(R.menu.menu, menu);
            mMenu = menu;
            mMode = mode;
            mSelectedFolderCount = 0;
            if (mCategory == FileUtil.CATEGORY_FAV) {
                menu.findItem(R.id.modal_action_fav)
                        .setIcon(R.drawable.operation_delete_fav)
                        .setTitle(R.string.operation_fav_remove);
            } else {
                menu.findItem(R.id.modal_action_fav).setIcon(
                        R.drawable.operation_add_fav);
            }

            menu.findItem(R.id.modal_action_copy).setVisible(
                    mCategory != FileUtil.CATEGORY_APP
                            && mCategory != FileUtil.CATEGORY_FAV);
            menu.findItem(R.id.modal_action_cut).setVisible(
                    mCategory != FileUtil.CATEGORY_APP
                            && mCategory != FileUtil.CATEGORY_FAV);
            menu.findItem(R.id.modal_action_multi_share).setVisible(
                    mCategory != FileUtil.CATEGORY_APP
                            && mCategory != FileUtil.CATEGORY_FAV);
            menu.findItem(R.id.modal_action_delete).setVisible(
                    mCategory != FileUtil.CATEGORY_APP
                            && mCategory != FileUtil.CATEGORY_FAV);

            if (mMultiSelectActionBarView == null) {
                mMultiSelectActionBarView = (ViewGroup) LayoutInflater.from(
                        mContext).inflate(R.layout.actionbar_multi_mode, null);
                mMultiSelect = (ImageView) mMultiSelectActionBarView
                        .findViewById(R.id.multi_select);
            }
            mode.setCustomView(mMultiSelectActionBarView);
            ((TextView) mMultiSelectActionBarView.findViewById(R.id.title))
                    .setText(R.string.fm_multi_select);

            if (mAdapter != null) {
                mAdapter.setActionMode(true);
            }

            return true;
        }

        @Override
        public void onDestroyActionMode(ActionMode mode) {
            if (null != mAdapter) {
                mAdapter.setActionMode(false);
            }
        }

        @Override
        public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
            if (mAdapter.isOperationShow()) {
                mAdapter.hideOperation();
                mAdapter.setActionMode(true);
                return true;
            }

            mMultiSelect.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (!mAllSelected) {
                        mAllSelected = true;
                        selectAll(true);
                    } else {
                        mAllSelected = false;
                        selectAll(false);
                    }
                }
            });
            return true;
        }

        @Override
        public void onItemCheckedStateChanged(ActionMode mode, int position,
                long id, boolean checked) {
            Log.d(TAG, "check state changed, position: " + position + ", checked: " + checked);
            if (mListView.getCheckedItemCount() == mListView.getCount()) {
                mAllSelected = true;
                mMultiSelect
                        .setImageResource(R.drawable.action_icon_all_reverse_choose_nor);
            } else {
                mAllSelected = false;
                mMultiSelect
                        .setImageResource(R.drawable.action_icon_choice_of_nor);
            }

            if (null != mCursor) {
                try {
                    mCursor.moveToPosition(position);
                    String path = mCursor.getString(mCursor
                            .getColumnIndex(MediaStore.MediaColumns.DATA));

                    File file = new File(path);
                    if (file.isDirectory()) {
                        if (checked) {
                            mSelectedFolderCount++;
                        } else {
                            mSelectedFolderCount--;
                        }
                    }
                } catch (CursorIndexOutOfBoundsException e) {
                    Log.d(TAG, "cursor content maybe changed while background thread may change it");
                }

                if (mSelectedFolderCount == 0 && mCategory != FileUtil.CATEGORY_FAV) {
                    mMenu.findItem(R.id.modal_action_multi_share).setVisible(
                            true);
                } else if (mSelectedFolderCount > 0) {
                    mMenu.findItem(R.id.modal_action_multi_share).setVisible(
                            false);
                }
            }
        }

        private void selectAll(boolean flag) {
            int start = 0;
            for (int i = start; i < mListView.getCount(); i++) {
                mListView.setItemChecked(i, flag);
            }
        }

        private String getSelectedPath() {
            StringBuilder sb = new StringBuilder();
            SparseBooleanArray selectPositions = mListView
                    .getCheckedItemPositions();
            String path = null;
            for (int position = 0; position < selectPositions.size(); position++) {
                if (selectPositions.get(selectPositions.keyAt(position))) {
                    mCursor.moveToPosition(selectPositions.keyAt(position));
                    path = mCursor.getString(mCursor
                            .getColumnIndex(MediaStore.MediaColumns.DATA));
                    sb.append(path);
                    sb.append(File.pathSeparatorChar);
                }
            }
            if (sb.length() > 1)
                sb.deleteCharAt(sb.length() - 1);
            return sb.toString();
        }
    }

    private class ThumbnailTask extends AsyncTask {

        @Override
        protected Object doInBackground(Object... params) {
            Map<String, Integer> visibleItems = getVisibleItems();
            for (String name : visibleItems.keySet()) {
                int type = mCategory;
                if (mCategory == FileUtil.CATEGORY_FAV) {
                    type = visibleItems.get(name);
                }
                Drawable d = FMThumbnailHelper.findThumbnailInCache(mContext,
                        name, type);
            }
            return null;
        }

        @Override
        protected void onProgressUpdate(Object... progress) {
        }

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
        }

        @Override
        protected void onPostExecute(Object result) {
            ((BaseAdapter) getListAdapter()).notifyDataSetChanged();
            super.onPostExecute(result);
        }
    }
}
