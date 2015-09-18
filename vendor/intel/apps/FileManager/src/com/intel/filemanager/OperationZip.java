
package com.intel.filemanager;

import java.io.BufferedOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.zip.CRC32;
import java.util.zip.CheckedOutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FileDBOP;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.LogHelper;

import android.content.Context;
import android.os.Looper;
import android.text.TextUtils;

public class OperationZip extends Operation {
    private static LogHelper Log = LogHelper.getLogger();
    private final static String TAG = "OperZip";

    private String mTitle;
    private String mCurrentSrcFile;
    private String mZipName;

    public OperationZip(Context ctx, String curPath, String srcPath, String zipName) {
        mContext = ctx;
        mCurPath = curPath;
        mSrcPath = srcPath;
        mZipName = zipName;
        mTitle = ctx.getText(R.string.fm_op_title_zip).toString();
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

        String destPath = mCurPath + "/" + mZipName;
        File file = new File(destPath);
        if (file.exists()) {
            sendMsgandWait(ERROR_CALL_YNDIALOG);
            Log.v(TAG, "f.getName() = " + file.getName());
            if (isThreadCancel())
                return;
            else
                FileUtil.deleteFiles(file);
        }

        if (!FileUtil.isFolderCanWrite(mCurPath)) {
            handleResult(ERROR_FILE_READ_ONLY);
            return;
        }

        if (!FileUtil.checkSpaceRestriction(mContext, mCurPath, mTotalSize)) {
            handleResult(ERROR_NO_ENOUGH_SPACE);
            return;
        }

        if (calTotalFileSize(mSrcPath)) {
            try {
                File srcfile = new File(mSrcPath);
                if (srcfile.isDirectory()) {
                    zip(destPath, srcfile);
                } else if (srcfile.isFile()) {
                    zipSingleFile(mSrcPath, destPath);
                }
            } catch (IOException e) {
                if (FileUtil.getDiskFreeSpace(mContext, destPath) < OPERATIONTHROLD) {
                    sendMsgandWait(ERROR_NO_ENOUGH_SPACE);
                } else {
                    handleResult(ERROR_FILE_READ_ONLY);
                }
            } finally {
                if (isThreadCancel()) {
                    FileUtil.deleteFiles(file);
                } else {
                    FileDBOP.startMediaScan(mContext, mCurPath);
                }
            }
        } else {
            handleResult(ERROR_SRC_FILE_NOT_EXIST);// src file path is null
        }
    }

    // zip folder different with zip signal file
    private void zip(String zipFileName, File inputFile) throws IOException {
        ZipOutputStream out = new ZipOutputStream(new FileOutputStream(
                zipFileName));
        try {
            zip(out, inputFile, inputFile.getName());
            FileUtil.changePermission(new File(zipFileName));
        } finally {
            out.close();
        }
    }

    // really zip folder works here, need control flow
    private void zip(ZipOutputStream out, File f, String base) throws IOException {
        if (isThreadCancel()) {
            return;
        }
        if (f.isDirectory()) {
            File[] files = f.listFiles();
            out.putNextEntry(new ZipEntry(base + "/"));
            base = base.length() == 0 ? "" : base + "/";
            int listLen = files.length;
            for (int i = 0; i < listLen; i++) {
                if (isThreadCancel()) {
                    return;
                }
                zip(out, files[i], base + files[i].getName());
            }
        } else {
            out.putNextEntry(new ZipEntry(base));
            FileInputStream in = new FileInputStream(f);
            byte[] buffer = new byte[OPERATIONBUFFER];
            int bytes_read;
            mCurrentSrcFile = mContext.getString(R.string.processing_zipfile,
                    f.getName());
            try {
                while ((bytes_read = in.read(buffer)) != -1) {
                    if (isThreadCancel()) {
                        return;
                    }
                    mTotalOperSize += bytes_read;
                    out.write(buffer, 0, bytes_read);
                    // update progress
                    updateProgress(mTitle + ": " + f.getName());
                }
            } finally {
                in.close();
            }
        }
    }

    private void zipSingleFile(String input, String output) throws IOException {
        File file = new File(input);
        FileInputStream in = new FileInputStream(file);
        FileOutputStream f = new FileOutputStream(output);
        CheckedOutputStream ch = new CheckedOutputStream(f, new CRC32());
        ZipOutputStream out = new ZipOutputStream(new BufferedOutputStream(ch));
        byte[] buffer = new byte[OPERATIONBUFFER];
        out.putNextEntry(new ZipEntry(file.getName()));
        int c;
        int temp = 0;
        try {
            while ((c = in.read(buffer)) != -1) {
                if (isThreadCancel())
                    break;
                mTotalOperSize += c;
                out.write(buffer, 0, c);
                if (temp == 0 || temp >= mTotalSize * 0.1 || mTotalSize == mTotalOperSize) {
                    temp = 0;

                    // update progress
                    updateProgress(mTitle + ": " + file.getName());
                }
                temp += c;
            }
            FileUtil.changePermission(new File(output));
        } finally {
            in.close();
            out.close();
            // Log.v(TAG,"zip signal file in.close, out.close");
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
            case ERROR_NO_ENOUGH_SPACE: // no enough space
                mYNDlgTitle = mContext.getText(R.string.op_no_space).toString();
                mYNDlgInfo = mContext.getString(R.string.op_no_enough_space);
                mTask.notifyUpdateUI(ERROR_NO_ENOUGH_SPACE);
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
