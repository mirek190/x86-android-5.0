
package com.intel.filemanager;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FileDBOP;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.LogHelper;

import android.content.Context;
import android.text.TextUtils;

public class OperationUnzip extends Operation {
    private static LogHelper Log = LogHelper.getLogger();
    private final static String TAG = "OperUnzip";

    private String mTitle;

    public OperationUnzip(Context ctx, String curPath, String srcPath) {
        mContext = ctx;
        mCurPath = curPath;
        mSrcPath = srcPath;
        mTitle = ctx.getText(R.string.fm_op_title_unzip).toString();
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

        String fileName = mSrcPath;
        int index = mSrcPath.lastIndexOf(".");
        if (-1 != index)
            fileName = mSrcPath.substring(0, index);

        File file = new File(fileName);
        if (file.exists()) {
            sendMsgandWait(ERROR_CALL_YNDIALOG);
            Log.v(TAG, "f.getName() = " + file.getName());
            if (isThreadCancel())
                return;
            else
                FileUtil.deleteFiles(file);
        }

        try {
            mTotalSize = FileUtil.getSizeOfUnZip(new File(mSrcPath));
        } catch (IOException e) {
            handleResult(ERROR_READ_ZIP_FIAL);
            return;
        } catch (IllegalArgumentException e) {
            handleResult(ERROR_READ_ZIP_FIAL);
            return;
        }

        if (!FileUtil.isFolderCanWrite(mCurPath)) {
            handleResult(ERROR_FILE_READ_ONLY);
            return;
        }

        if (!FileUtil.checkSpaceRestriction(mContext, mCurPath, mTotalSize)) {
            handleResult(ERROR_NO_ENOUGH_SPACE);
            return;
        }

        if (!file.mkdirs()) {
            Log.d(TAG, "UNABLE TO MKDIR " + mSrcPath);
            handleResult(ERROR_CREATE_FOLDER);
        }
        try {
            unZipFile(new File(mSrcPath),
                    new File(mSrcPath.substring(0, mSrcPath.lastIndexOf("."))));
        } catch (Exception e) {
            Log.e(TAG, "UN ZIP() FILE EXCEPTION " + e);
        } finally {
            FileDBOP.startMediaScan(mContext, mCurPath);
        }
    }

    // unzip a file
    private void unZipFile(File from, File to) throws IOException {
        Log.v(TAG, "from: " + from.getName() + "| to : " + to.getName());
        ZipInputStream zin = new ZipInputStream(new FileInputStream(from));
        ZipEntry entry = null;

        while ((entry = zin.getNextEntry()) != null) {
            if ((entry == null) || isThreadCancel()) {
                break;
            }
            // Log.v(TAG,"entry  === "+entry.getName());
            if (entry.isDirectory()) {
                File directory = new File(to.getAbsolutePath(), entry.getName());
                if (!directory.exists()) {
                    if (!directory.mkdirs())
                        throw new IOException("can not create directory");
                }
                FileUtil.changePermission(directory);
                zin.closeEntry();
            } else {
                // if signal file maybe need mkdir for it
                String foutPath = to.getAbsolutePath() + "/" + entry.getName();

                if (!TextUtils.isEmpty(foutPath))
                    createParentFile(foutPath);
                FileOutputStream fout = new FileOutputStream(foutPath);
                DataOutputStream dout = new DataOutputStream(fout);
                byte[] b = new byte[OPERATIONBUFFER];
                int len = 0;
                int temp = 0;
                try {
                    while ((len = zin.read(b, 0, b.length)) != -1) {
                        if (isThreadCancel())
                            return;
                        mTotalOperSize += len;
                        dout.write(b, 0, len);

                        if (temp == 0 || temp >= mTotalSize * 0.1 || mTotalSize == mTotalOperSize) {
                            temp = 0;

                            // update progress
                            updateProgress(mTitle + ": " + entry.getName());
                        }
                        temp += len;

                    }

                    FileUtil.changePermission(new File(foutPath));
                } catch (IOException e) {
                    zin.closeEntry();
                    zin.close();
                    Log.e(TAG, "unzip file close all ");
                } finally {
                    dout.close();
                    fout.close();
                }
            }
        }

        zin.closeEntry();
        zin.close();
    }

    private void createParentFile(String path) {
        File desFile = new File(path);
        File parentFile = desFile.getParentFile();
        if (parentFile != null && !parentFile.exists()) {
            createParentFile(parentFile.getAbsolutePath());
            parentFile.mkdir();
        } else {
            return;
        }
    }

    @Override
    public void setOk() {
        // TODO Auto-generated method stub

        synchronized (mLock) {
            mIsCancel = false;
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
                mYNDlgTitle = mContext.getText(R.string.op_file_overwrite).toString();
                mYNDlgInfo = mContext.getString(R.string.op_file_exists);
                mTask.notifyUpdateUI(ERROR_CALL_YNDIALOG);
                break;
            case ERROR_READ_ZIP_FIAL:
                mErrorInfo = mContext.getString(R.string.op_read_zip_error);
                mTask.notifyUpdateUI(ERROR_DEL_FAIL);
                break;
            case ERROR_FILE_READ_ONLY:
                mErrorInfo = mContext.getString(R.string.file_unwrite);
                mTask.notifyUpdateUI(ERROR_FILE_READ_ONLY);
                break;
            case ERROR_NO_ENOUGH_SPACE: // no enough space
                mYNDlgTitle = mContext.getText(R.string.op_no_space).toString();
                mYNDlgInfo = mContext.getString(R.string.op_no_enough_space);
                mTask.notifyUpdateUI(ERROR_NO_ENOUGH_SPACE);
                break;
            case ERROR_CREATE_FOLDER:
                mErrorInfo = mContext.getString(R.string.op_error_no_dir, mSrcPath);
                mTask.notifyUpdateUI(ERROR_CREATE_FOLDER);
                break;
            default:
                break;
        }
    }
}
