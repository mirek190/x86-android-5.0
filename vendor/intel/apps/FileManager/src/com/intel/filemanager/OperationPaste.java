
package com.intel.filemanager;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import android.content.Context;
import android.text.TextUtils;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FileDBOP;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.LogHelper;

class OperationPaste extends Operation {
    private static LogHelper Log = LogHelper.getLogger();
    private final static String TAG = "OperPaste";
    private static final String NOSPACE_ERROR = "No space left on device";
    private static final String NOSPACE_ERROR_INDEX = "No space";
    private String mTitle;

    private boolean mIsOverWrite = false;
    private boolean mIsChkOverride = false;
    private String mCurrentSrcFile;
    private String mCurrentDescFile;

    public OperationPaste(Context ctx, String curPath, String srcPath) {
        mContext = ctx;
        mCurPath = curPath;
        mSrcPath = srcPath;
        mTitle = ctx.getText(R.string.fm_op_title_paste).toString();
        mProgressTitle = mTitle;
    }

    @Override
    public void start() {
        mTask = new OperationTask(this, mContext);
        mTask.execute(1);
    }

    @Override
    public void startOperation() {
        // TODO Auto-generated method stub
        if (isThreadCancel())
            return;

        if (!calTotalFileSize(mSrcPath)) {
            handleResult(ERROR_SRC_FILE_NOT_EXIST);// src file path is null
            return;
        }

        if (!FileUtil.checkSpaceRestriction(mContext, mCurPath, mTotalSize)) {
            sendMsgandWait(ERROR_NO_ENOUGH_SPACE);
            return;
        }
        paste();

    }

    @Override
    public void setOk() {
        // TODO Auto-generated method stub
        mIsOverWrite = true;
        synchronized (mLock) {
            mLock.notifyAll();
        }
    }

    @Override
    public void setCancel() {
        // TODO Auto-generated method stub
        mIsOverWrite = false;
        synchronized (mLock) {
            mLock.notifyAll();
        }
    }

    // do processing of paste
    public void paste() {

        // the "paste fail" message, because others succeeded
        if (!TextUtils.isEmpty(FileOperator.srcParentPath)
                && (FileOperator.srcParentPath.equals(mCurPath)
                || mCurPath.startsWith(mSrcPath))) {
            handleResult(ERROR_SCR_EQLDES);
            return;
        }

        int listLen = mPathList.size();
        String path = "";
        updateProgress(mTitle);
        for (int i = 0; i < listLen; i++) {
            Log.d(TAG, "paste: filesize=" + mTotalSize + "| totalopersize=" + mTotalOperSize);
            if (isThreadCancel())
                break;
            path = mPathList.get(i);
            File srcFile = new File(path);
            mCurrentSrcFile = srcFile.getAbsolutePath();
            if (!srcFile.exists()) {
                handleResult(ERROR_SRC_FILE_NOT_EXIST);
                continue;
            }

            if (srcFile.length() < Long.MAX_VALUE) {
                String desPath = null;
                if (TextUtils.isEmpty(FileOperator.srcParentPath)) {
                    desPath = mCurPath + "/" + srcFile.getName();
                } else {
                    desPath = mCurPath
                            + srcFile.getPath().substring(FileOperator.srcParentPath.length());
                }

                File desFile = new File(desPath);
                mCurrentDescFile = desPath;

                Log.d(TAG, "paste file, src: " + mCurrentSrcFile + ", dest: " + mCurrentDescFile);

                // check if file exist and then query user to decide how to do
                if (desFile.exists()) {
                    if (!mIsChkOverride) {
                        sendMsgandWait(ERROR_CALL_YNDIALOG);
                        mIsChkOverride = true;
                    }

                    if (mCurrentDescFile.equals(mCurrentSrcFile))
                        continue;

                    if (mIsOverWrite) {
                        FileUtil.deleteFiles(desFile);
                    } else {
                        mTotalOperSize = FileUtil.getSize(desFile);
                        updateProgress(mTitle + ": " + srcFile.getName());
                        continue;
                    }
                }

                if (srcFile.isFile()) {
                    try {
                        mCurrentSrcFile = srcFile.getAbsolutePath();
                        pasteFile(srcFile, desFile);
                        updateProgress(mTitle + ": " + srcFile.getName());
                    } catch (Exception e) {
                        Log.e(TAG, "paste file error " + e);
                        continue;
                    }

                    if (FileOperator.isCut) {
                        FileOperator.deleteFavorite(srcFile.getPath(), mContext);
                        if (TextUtils.isEmpty(FileOperator.srcParentPath)) {
                            FileDBOP.deleteFileInDB(mContext, srcFile.getPath());
                        }
                        if (srcFile.delete()) {
                            deleteThumb(srcFile);
                        } else {
                            handleResult(ERROR_FILE_READ_ONLY);
                        }
                    }
                } else if (srcFile.isDirectory()) {
                    mTotalOperSize++;
                    updateProgress(mTitle + ": " + srcFile.getName());

                    if (!desFile.mkdirs()) {
                        Log.d(TAG, "can't mkdir at " + desPath);
                    }
                }

                try {
                    if (FileUtil.changePermission(desFile)) {
                    } else {
                        Log.v(TAG, "change permission " + desFile.getAbsolutePath() + " error");
                    }
                } catch (Exception ex) {
                    Log.e(TAG, "changePermission() exception : " + ex.toString());
                }

                if (FileOperator.isCut) {
                    if (!isThreadCancel()) {
                        cutFolders(mSrcPath);
                    }
                    if (TextUtils.isEmpty(FileOperator.srcParentPath)) {
                        FileDBOP.startMediaScan(mContext, mCurPath + "/" + srcFile.getName());
                    } else if (mCurPath.startsWith(FileOperator.srcParentPath + "/")) {
                        FileDBOP.startMediaScan(mContext, FileOperator.srcParentPath + "/" + srcFile.getName());
                    } else if (FileOperator.srcParentPath.startsWith(mCurPath + "/")) {
                        FileDBOP.startMediaScan(mContext, mCurPath + "/" + srcFile.getName());
                    } else {
                        FileDBOP.startMediaScan(mContext, FileOperator.srcParentPath + "/" + srcFile.getName());
                        FileDBOP.startMediaScan(mContext, mCurPath + "/" + srcFile.getName());
                    }
                } else {
                    FileDBOP.startMediaScan(mContext, mCurPath + "/" + srcFile.getName());
                }
            } else {
                sendMsgandWait(ERROR_NO_ENOUGH_SPACE);
                continue;
            }
        }
    }

    // for cut operation, delete the folder
    private void cutFolders(String path) {
        String[] name = path.split(FileDBOP.SEPARATOR);
        int len = name.length;
        for (int i = 0; i < len; i++) {
            File f = new File(name[i]);
            if (f.exists() && f.isDirectory()) {
                FileUtil.deleteFiles(f);
                FileOperator.deleteFavorite(f.getPath(), mContext);
            }
        }
    }

    // real operation on paste
    private final void pasteFile(File srcFile, File desFile) throws Exception {
        FileInputStream input = new FileInputStream(srcFile);
        FileOutputStream output = new FileOutputStream(desFile);
        byte[] b = new byte[OPERATIONBUFFER];
        int len;
        int temp = 0;
        try {
            while (((len = input.read(b)) != -1) && !isThreadCancel()) {
                output.write(b, 0, len);
                mTotalOperSize += len;

                if (temp == 0 || temp >= mTotalSize * 0.1 || mTotalSize == mTotalOperSize) {
                    temp = 0;

                    // update progress
                    updateProgress(mTitle + ": " + srcFile.getName());
                }
                temp += len;

            }
        } catch (IOException e) {
            long freeSpace = FileUtil.getDiskFreeSpace(mContext, desFile.getAbsolutePath());
            String tmp = e.toString();
            tmp = tmp.substring(tmp.indexOf(NOSPACE_ERROR_INDEX));
            if (tmp.equals(NOSPACE_ERROR) || (freeSpace < OPERATIONTHROLD)) {
                sendMsgandWait(ERROR_NO_ENOUGH_SPACE);
            }
        } finally {
            output.flush();
            output.close();
            input.close();
        }
    }

    @Override
    public void handleResult(int result) {

        switch (result) {
            case ERROR_CALL_YNDIALOG:
                mYNDlgTitle = mContext.getText(R.string.op_file_overwrite).toString();
                mYNDlgInfo = mContext.getString(R.string.op_file_exists);
                mTask.notifyUpdateUI(ERROR_CALL_YNDIALOG);
                break;
            case ERROR_SCR_EQLDES:
                mErrorInfo = mContext.getString(R.string.op_file_srcequaldes);
                mTask.notifyUpdateUI(ERROR_SCR_EQLDES);
                break;
            case ERROR_SRC_FILE_NOT_EXIST: // original file not exist
                mErrorInfo = mContext.getString(R.string.op_file_not_exist,
                        mCurrentSrcFile);
                mTask.notifyUpdateUI(ERROR_SRC_FILE_NOT_EXIST);
                break;

            case ERROR_NO_ENOUGH_SPACE: // no enough space
                mYNDlgTitle = mContext.getText(R.string.op_no_space).toString();
                mYNDlgInfo = mContext.getString(R.string.op_no_enough_space);
                mTask.notifyUpdateUI(ERROR_NO_ENOUGH_SPACE);
                break;

            case ERROR_FILE_READ_ONLY:
                mErrorInfo = mContext.getString(R.string.file_unwrite, mCurrentDescFile);
                mTask.notifyUpdateUI(ERROR_FILE_READ_ONLY);
                break;
            default:
                break;
        }
    }
}
