package com.intel.filemanager;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashSet;
import java.util.Set;

import android.app.Fragment;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FMThumbnailHelper;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.LogHelper;
import com.intel.filemanager.view.OperationView;
import com.intel.filemanager.view.OperationView.OperationListener;

public class FMAdapter extends BaseAdapter implements
        ListView.OnScrollListener, OperationListener {

    private static final String TAG = "FMAdapter";
    private static LogHelper Log = LogHelper.getLogger();
    private final static int LOAD_THUMBNAIL = 1;

    public static final int FLAG_PICK = 1;
    public static final int FLAG_DIR_FRAGMENT = 2;
    final LayoutInflater mInflate;

    TextView mTextView;

    int mPosition;

    private Context mContext;

    private ThumbnailTask mThumbnailTask = null;

    private ArrayList<FileNode> mArrayList;

    private String CURRENTPATH;

    private ListView mListView;

    public final static int LISTVIEW = 0;

    public final static int GRIDVIEW = 1;

    private boolean mIsMultiTrans;

    private int mFolderCount;

    private boolean mStyle = false;

    private OperationView mOperationView;
    private OperationView.OperationListener mOperationListener;
    private boolean mIsOperationShow;

    private String mSelectPath;
    private int mAdaptFlag;

    public FMAdapter(Context context, int viewMode,
            ArrayList<FileNode> arrayList, String currentPath,
            ListView listview, boolean isMultiTrans,
            int folderCount, int flag, boolean displayStyle) {
        mContext = context;
        mInflate = LayoutInflater.from(mContext);
        mArrayList = arrayList;
        CURRENTPATH = currentPath;
        mListView = listview;
        mListView.setOnScrollListener(this);
        mIsMultiTrans = isMultiTrans;
        mFolderCount = folderCount;
        mAdaptFlag = flag;
        mStyle = displayStyle;
        mListView.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_BACK)
                    return false;
                mSelectPath = null;
                removeOperationView();
                return false;
            }
        });
    }

    public void setDataList(ArrayList<FileNode> arrayList) {
        mArrayList = arrayList;
    }

    @Override
    public int getCount() {
        return mArrayList.size();
    }

    @Override
    public Object getItem(int position) {
        return mArrayList.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
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

    public void addOperationView(View view, int category, String path,
            int position) {
        if (mOperationView == null) {
            mOperationView = new OperationView(mContext,
                    OperationView.TYPE_DIRECTORY_LIST, path, CURRENTPATH);
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

    public void removeOperationView() {
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

    public void setOperationListener(
            OperationView.OperationListener operationListener) {
        mOperationListener = operationListener;
    }

    @Override
    public void onOperationClicked(int opType, int position) {
        removeOperationView();
        mSelectPath = null;
        if (mOperationListener != null) {
            mOperationListener.onOperationClicked(opType, position);
        }
    }

    public void setExitOperation() {
        if ((null != mOperationView) && (null != mOperationView.getCurrentOp())) {
            mOperationView.getCurrentOp().setExit();
        }
    }

    @Override
    public void onOperationEnd() {
        if (mOperationListener != null) {
            mOperationListener.onOperationEnd();
        }
    }

    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {

        View mainView = null;

        mainView = null;
        mainView = convertView;
        mPosition = position;
        if (mainView == null)
            mainView = mInflate.inflate(R.layout.file_list_item, null);

        ImageView iv = (ImageView) mainView.findViewById(R.id.icon);
        mTextView = (TextView) mainView.findViewById(R.id.file_name);

        TextView stv = (TextView) mainView.findViewById(R.id.file_size);
        TextView ttv = (TextView) mainView.findViewById(R.id.file_modiftime);

        Date date = new Date(mArrayList.get(position).lastModifiedTime);

        String format = Settings.System.getString(mContext.getContentResolver(),
                Settings.System.DATE_FORMAT);
        if (TextUtils.isEmpty(format)) {
            format = "yyyy-MM-dd HH:mm:ss";
        }
        SimpleDateFormat formatter;

        formatter = new SimpleDateFormat(format);

        String strDate = formatter.format(date);

        if (mArrayList.get(position).isFile) {
            stv.setText(FileUtil.formatSize(mArrayList.get(position).lengthInByte));
            ttv.setText(strDate);
            ttv.setVisibility(View.VISIBLE);
            stv.setVisibility(View.VISIBLE);
        } else if (mArrayList.get(position).type == "folder") {
            ttv.setText(strDate);
            ttv.setVisibility(View.VISIBLE);
            stv.setVisibility(View.GONE);
        } else {
            stv.setVisibility(View.GONE);
            ttv.setVisibility(View.GONE);
        }

        mTextView.setText(mArrayList.get(position).name);

        iv.setImageResource(mArrayList.get(position).iconRes);

        Drawable d = null;
        String name = CURRENTPATH + "/" + mArrayList.get(position).name;
        if (null != FMThumbnailHelper.getThumbnailCathe())
            d = (Drawable) FMThumbnailHelper.getThumbnailCathe().get(name);
        if (null != d)
            iv.setImageDrawable(d);

        final String path = mArrayList.get(position).path;
        final int category = FileUtil.getCategoryByPath(path);
        final View view = mainView;
        ImageView expander = (ImageView) view.findViewById(R.id.expander);
        expander.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (TextUtils.isEmpty(mSelectPath) || !path.equals(mSelectPath)) {
                    mSelectPath = path;
                    removeOperationView();
                    addOperationView(view, category, path, position);
                    checkPositionAndScroll(position);
                } else {
                    mSelectPath = null;
                    removeOperationView();
                }
                Fragment fragment = ((FileMainActivity) mContext).getFileDirFragment();
                if (null != fragment) {
                    ((FileDirFragment) fragment).hidePasteButton();
                }
            }
        });

        if (mAdaptFlag == FLAG_PICK) {
            expander.setVisibility(View.GONE);
        }
        // the view of list item will be reused
        // add the operation view into the item that have added the view before
        if (mOperationView != null) {
            ViewGroup operationParent = (ViewGroup) mOperationView.getParent();
            if (operationParent == view && !TextUtils.isEmpty(mSelectPath)
                    && !path.equals(mSelectPath)) {
                removeOperationView();
            } else if (operationParent == null && path.equals(mSelectPath)) {
                addOperationView(view, category, mSelectPath, position);
            }
            if (mListView.isItemChecked(position))
                mOperationView = null;
        }

        if (mIsMultiTrans && position < mFolderCount) {
            mainView.setEnabled(false);
        }
        return mainView;
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

    @Override
    public void onScroll(AbsListView view, int firstVisibleItem,
            int visibleItemCount, int totalItemCount) {
    }

    @Override
    public void onScrollStateChanged(AbsListView view, int scrollState) {
        switch (scrollState) {
            case OnScrollListener.SCROLL_STATE_IDLE:
                if (view.getAdapter() != this)
                    return;
                loadThumbnail();
                break;
            case OnScrollListener.SCROLL_STATE_TOUCH_SCROLL:
            case OnScrollListener.SCROLL_STATE_FLING:
                break;
        }
    }

    public void loadThumbnail() {
        mThumbnailHandler.removeMessages(LOAD_THUMBNAIL);
        mThumbnailHandler.sendMessageDelayed(
                mThumbnailHandler.obtainMessage(LOAD_THUMBNAIL), 200);
    }

    private class ThumbnailTask extends AsyncTask<Object, Object, Object> {
        private Set<String> mFileNames;

        @Override
        protected Object doInBackground(Object... params) {
            for (String name : mFileNames) {
                FMThumbnailHelper.findThumbnailInCache(mContext, name);
            }
            return null;
        }

        @Override
        protected void onProgressUpdate(Object... progress) {
        }

        @Override
        protected void onPreExecute() {
            int first = mListView.getFirstVisiblePosition();
            int count = mListView.getChildCount();

            mFileNames = new HashSet<String>(count);
            for (int i = 0; (i < count) && (first + i < mArrayList.size()); i++) {
                if (mArrayList.get(first + i).hasThumbnail) {
                    mFileNames.add(mArrayList.get(first + i).path);
                }
            }
            super.onPreExecute();
        }

        @Override
        protected void onPostExecute(Object result) {
            notifyDataSetChanged();
            super.onPostExecute(result);
        }
    }

    Handler mThumbnailHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == LOAD_THUMBNAIL) {
                // handler.postDelayed(highlightThread, 0);
                if (mThumbnailTask != null
                        && mThumbnailTask.getStatus() != AsyncTask.Status.FINISHED) {
                    mThumbnailTask.cancel(true);
                }
                mThumbnailTask = new ThumbnailTask();
                mThumbnailTask.execute();
            }
        }
    };

}
