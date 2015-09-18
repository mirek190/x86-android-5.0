
package com.intel.filemanager;

import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FileDBOP;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.LogHelper;

import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;
import android.provider.MediaStore;
import android.text.TextUtils;

public class OperationDelete extends Operation {
    private static LogHelper Log = LogHelper.getLogger();
    private final static String TAG = "OperDelete";

    public static final int OP_DELETE_CONFIRM = 5;
    public static final int OP_MULT_DELETE_CONFIRM = 7;

    private String mTitle;
    private String mCurrentSrcFile;
    private int mDelType;
    private boolean mHasDirectory;

    public OperationDelete(Context ctx, int type, String srcPath) {
        mContext = ctx;
        mDelType = type;
        mSrcPath = srcPath;
        mTitle = ctx.getText(R.string.fm_op_title_delete).toString();
        mProgressTitle = mTitle;
    }

    public OperationDelete(Context ctx, int type, String srcPath, String curPath) {
        mContext = ctx;
        mDelType = type;
        mSrcPath = srcPath;
        mCurPath = curPath;
        mTitle = ctx.getText(R.string.fm_op_title_delete).toString();
        mProgressTitle = mTitle;
    }

    @Override
    public void start() {
        mTask = new OperationTask(this, mContext);
        mTask.execute(1);
    }

    @Override
    public void startOperation() {
        if (isThreadCancel())
            return;

        sendMsgandWait(ERROR_CALL_YNDIALOG);

        if (calTotalFileSize(mSrcPath))
            delete();
        else
            handleResult(ERROR_SRC_FILE_NOT_EXIST);// src file path is null
    }

    private void delete() {
        Log.d(TAG, "start to delete files");

        String[] name = mSrcPath.split(FileDBOP.SEPARATOR);
        int len = name.length;
        Set<String> filePaths = new HashSet<String>();
        for (int i = 0; i < len; i++) {
            if (isThreadCancel()) { // cancel should do what need analyze
                break;
            }
            File f = new File(name[i]);
            filePaths.add(f.getPath());
            if (f.exists()) {
                deleteFiles(f);
            }
        }

        if (!mHasDirectory) {
            // delete the file in media db
            for (String deleteFileName : filePaths) {
                FileDBOP.deleteFileInDB(mContext, deleteFileName);
            }
        } else if (!TextUtils.isEmpty(mCurPath)) {
            FileDBOP.startMediaScan(mContext, mCurPath);
        }
    }

    // delete file is file or folder
    private void deleteFiles(File srcFile) {
        Log.d(TAG, "Enter deleteFile, srcFile:" + srcFile);
        FileOperator.deleteFavorite(srcFile.getPath(), mContext);
        if (srcFile.isFile()) {
            Log.d(TAG, "isFile()");
            mCurrentSrcFile = srcFile.getAbsolutePath();
            if (srcFile.exists()) {
                mTotalOperSize += srcFile.length();

                if (false == srcFile.delete()) {
                    Log.d(TAG, "### FILE:" + srcFile + " delete failure");
                    handleResult(ERROR_DEL_FAIL);
                } else {
                    Log.d(TAG, "### FILE:" + srcFile + " deleted");
                    updateProgress(mTitle + ": " + srcFile.getName());
                    deleteThumb(srcFile);
                }
            }
        } else if (srcFile.isDirectory()) {
            Log.v(TAG, "isDirectory()");
            mHasDirectory = true;
            mTotalOperSize++;
            updateProgress(mTitle + ": " + srcFile.getName());
            File[] files = srcFile.listFiles();
            if (files != null) {
                for (File f : files) {
                    if (isThreadCancel()) {
                        break;
                    }

                    deleteFiles(f);
                }
            }
            srcFile.delete();
        }
    }

    @Override
    public void setOk() {
        // TODO Auto-generated method stub
        synchronized (mLock) {
            mLock.notifyAll();
        }
    }

    @Override
    public void setCancel() {
        // TODO Auto-generated method stub
        synchronized (mLock) {
            mIsCancel = true;
            mLock.notifyAll();
        }
    }

    public void handleResult(int result) {
        switch (result) {
            case ERROR_CALL_YNDIALOG:
                if (mDelType == OP_DELETE_CONFIRM) {
                    mYNDlgTitle = mContext.getText(R.string.fm_options_delete).toString();
                    mYNDlgInfo = mContext.getString(R.string.dialog_delete_confirm, mSrcPath);
                }
                else {
                    mYNDlgTitle = mContext.getText(R.string.fm_options_delete).toString();
                    mYNDlgInfo = mContext.getString(R.string.dialog_mult_delete_confirm);
                }
                mTask.notifyUpdateUI(ERROR_CALL_YNDIALOG);
                break;
            case ERROR_DEL_FAIL:
                mErrorInfo = mContext.getString(R.string.file_unwrite,
                        mCurrentSrcFile);
                mTask.notifyUpdateUI(ERROR_DEL_FAIL);
                break;
            case ERROR_SRC_FILE_NOT_EXIST: // original file not exist
                mErrorInfo = mContext.getString(R.string.op_file_not_exist,
                        mCurrentSrcFile);
                mTask.notifyUpdateUI(ERROR_SRC_FILE_NOT_EXIST);
                break;
            default:
                break;
        }
    }
}
