
package com.intel.filemanager;

import java.io.File;
import java.util.ArrayList;
import java.util.concurrent.locks.ReentrantLock;

import android.content.Context;
import android.text.TextUtils;

import com.intel.filemanager.util.FileDBOP;
import com.intel.filemanager.util.LogHelper;

public abstract class Operation {
    private static LogHelper Log = LogHelper.getLogger();
    private final static String TAG = "Operation";
    public final static String MINITHUMB_PATH = "/sdcard/.thumbnails/minithumb/";
    public final static String THUMB_PATH = "/sdcard/.thumbnails/thumb/";

    protected static final int ERROR_CALL_YNDIALOG = 101;
    protected static final int ERROR_SCR_EQLDES = 102;
    protected static final int ERROR_SRC_FILE_NOT_EXIST = 103;
    protected static final int ERROR_NO_ENOUGH_SPACE = 104;
    protected static final int ERROR_FILE_READ_ONLY = 105;
    protected static final int ERROR_DEL_FAIL = 106;
    protected static final int ERROR_READ_ZIP_FIAL = 107;
    protected static final int ERROR_CREATE_FOLDER = 108;
    protected static final int ERROR_ENCRYPT = 110;

    protected static final int OPERATIONBUFFER = 16 * 1024;
    protected static final int OPERATIONTHROLD = 16 * 1024; // if left space <
                                                            // 32k, can't use
    public static final int DELAY = 50;

    // control task run or stop
    protected boolean mIsCancel = false;

    protected String mCurPath;
    protected String mSrcPath;
    protected long mTotalSize = 0;
    protected long mTotalOperSize = 0;
    protected ArrayList<String> mPathList = new ArrayList<String>();
    // progress dialog info
    protected String mProgressTitle;
    protected String mErrorInfo;
    protected OperationTask mTask;
    protected Context mContext;
    protected String mYNDlgInfo = "";
    protected String mYNDlgTitle = "";

    protected ReentrantLock mLock = new ReentrantLock();
    private OnOperationEndListener mEndListener;

    abstract public void startOperation();

    abstract public void setOk();

    abstract public void setCancel();

    protected void handleResult(int result) {
    }

    public void setExit() {
        synchronized (mLock) {
            mIsCancel = true;
        }
    }

    abstract public void start();

    protected String getYNDlgInfo() {
        return mYNDlgInfo;
    }

    protected String getYNDlgTitle() {
        return mYNDlgTitle;
    }

    protected String getProgressDlgTitle() {
        return mProgressTitle;
    }

    protected String getErrorInfo() {
        return mErrorInfo;
    }

    public void setOnOperationEndListener(OnOperationEndListener onOperationEndListener) {
        mEndListener = onOperationEndListener;
    }

    public void callOperationEndListener() {
        if (mEndListener != null)
            mEndListener.onOperationEnd();
    }

    private final static String THIS_FOLDER = ".";
    private final static String PARENTS_FOLDER = "..";

    protected boolean calTotalFileSize(String path) {
        String srcPath = path;
        Log.v(TAG, "srcPath = " + srcPath);
        if (TextUtils.isEmpty(srcPath)) {
            return false;
        }
        String[] name = srcPath.split(FileDBOP.SEPARATOR);

        int len = name.length;
        for (int i = 0; i < len; i++) {
            File f = new File(name[i]);
            Log.d(TAG, "File:" + f);
            if (f.exists()) {
                getFileSize(f);
            }
        }

        return true;
    }

    // get all file size and list it in array
    private void getFileSize(File file) {
        if (file.getName().equals(THIS_FOLDER)
                || file.getName().equals(PARENTS_FOLDER)) {
            return;
        }

        mTotalSize++;
        mPathList.add(file.getAbsolutePath());

        if (file.isFile()) {
            mTotalSize += file.length();
        } else if (file.isDirectory()) {
            mTotalSize++;
            File[] files = file.listFiles();
            if (null != files) {
                for (File f : files) {
                    getFileSize(f);
                }
            }
        }
    }

    protected void deleteThumb(File srcFile) {
        if (srcFile.isFile()) {
            String thumbName = srcFile.getAbsolutePath().hashCode() + "_"
                    + srcFile.lastModified() + "_" + srcFile.getName();

            /* delete the thumbnails of the pictures */
            String thumbPath = THUMB_PATH + thumbName;
            File thumbFile = new File(thumbPath);

            String minithumbpath = MINITHUMB_PATH + thumbName;
            File minithumb = new File(minithumbpath);

            if (thumbFile.exists() && thumbFile.delete()) {
                Log.d(TAG, "thumbFile deleted!!!");
            }

            if (minithumb.exists() && minithumb.delete()) {
                Log.d(TAG, "minithumb file deleted!!!");
            }
        }
    }

    protected void updateProgress(String title) {
        int progress;
        if (mTotalSize <= 0) {
            progress = 0;
        } else {
            progress = (int) ((100 * mTotalOperSize) / mTotalSize);
        }
        if (progress > 100) {
            progress = 100;
        }

        mProgressTitle = title;
        mTask.notifyUpdateUI(progress);
    }

    protected void sendMsgandWait(int info) {
        handleResult(info);
        try {
            synchronized (mLock) {
                mLock.wait();
            }
        } catch (InterruptedException e1) {
            // TODO Auto-generated catch block
            e1.printStackTrace();
        }
    }

    protected boolean isThreadCancel() {
        synchronized (mLock) {
            return mIsCancel;
        }
    }

    public interface OnOperationEndListener {
        public void onOperationEnd();
    }
}
