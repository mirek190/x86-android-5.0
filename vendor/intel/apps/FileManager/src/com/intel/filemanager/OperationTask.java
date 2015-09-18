
package com.intel.filemanager;

import com.intel.filemanager.R;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.AsyncTask;
import android.os.PowerManager;
import android.util.Log;
import android.view.WindowManager.BadTokenException;
import android.widget.Toast;

class OperationTask extends AsyncTask<Integer, Integer, Long> {
    private static final String TAG = "OperationTask";
    private static final int CALL_YNDIALOG = 101;
    private static final int ERROR_NO_ENOUGH_SPACE = 104;

    private Context mContext;
    private Operation mOp;
    private ProgressDialog mPdlg;
    PowerManager.WakeLock mWakeLock;

    public OperationTask(Operation op, Context ctx) {
        mContext = ctx;
        mOp = op;
    }

    @Override
    protected void onPreExecute() {
        PowerManager pm = (PowerManager) mContext
                .getSystemService(Context.POWER_SERVICE);

        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                FileDirFragment.TAG);
        mWakeLock.acquire();
        prepareProgressDlg();
    }

    @Override
    protected Long doInBackground(Integer... op) {
        mOp.startOperation();
        return Long.valueOf((long) 0);
    }

    // use this method to update progress, popup dialog, popup toast
    // visit ui thread
    @Override
    protected void onProgressUpdate(Integer... progress) {
        if (mOp.isThreadCancel())
            return;
        try {
            if (progress[0] <= 100) { // update progress dialog
                if (!mPdlg.isShowing())
                    mPdlg.show();
                mPdlg.setProgress(progress[0]);
                mPdlg.setTitle(mOp.getProgressDlgTitle());
            } else if (progress[0] == CALL_YNDIALOG) {
                showYNDialog();
            } else if (progress[0] == ERROR_NO_ENOUGH_SPACE) {
                showConfirmDialog();
            } else {
                Toast.makeText(mContext, mOp.getErrorInfo(), Toast.LENGTH_SHORT).show();
            }
        } catch (BadTokenException e) {
            Log.d(TAG, "activity maybe finished ,and task still doinbackground");
        } catch (IllegalStateException e) {
            Log.d(TAG, e.getMessage());
        }
    }

    @Override
    protected void onPostExecute(Long result) {
        if (mWakeLock.isHeld()) {
            mWakeLock.release();
        }

        if (mOp.isThreadCancel())
            return;

        try {
            mPdlg.dismiss();
        } catch (IllegalStateException e) {
            Log.d(TAG, "activity maybe finished ,and task still doinbackground!");
        }

        mOp.callOperationEndListener();

    }

    private void prepareProgressDlg() {
        String buttonText = mContext.getString(android.R.string.cancel).toString();
        mPdlg = new ProgressDialog(mContext);
        mPdlg.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        mPdlg.setCancelable(false);
        mPdlg.setButton(buttonText, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                // TODO Auto-generated method stub
                mOp.setExit();
            }
        });

        mPdlg.setTitle(mOp.getProgressDlgTitle());
    }

    public void notifyUpdateUI(Integer progress) {
        this.publishProgress(progress);
    }

    private void showYNDialog() {
        CharSequence infor = mOp.getYNDlgInfo();
        CharSequence title = mOp.getYNDlgTitle();

        new AlertDialog.Builder(mContext)
                .setIconAttribute(android.R.attr.alertDialogIcon)
                .setTitle(title)
                .setMessage(infor)
                .setCancelable(false)
                .setPositiveButton(R.string.dialog_ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int whichButton) {
                                mOp.setOk();
                            }

                        })
                .setNegativeButton(R.string.dialog_cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int whichButton) {
                                mOp.setCancel();
                            }
                        }).show();
    }

    private void showConfirmDialog() {
        CharSequence infor = mOp.getYNDlgInfo();
        CharSequence yesKey = mOp.getYNDlgTitle();
        new AlertDialog.Builder(mContext)
                .setIconAttribute(android.R.attr.alertDialogIcon)
                .setTitle(R.string.confirm_dialog_title)
                .setMessage(infor)
                .setCancelable(false)
                .setPositiveButton(yesKey,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int whichButton) {
                                mOp.setCancel();
                            }
                        }).show();
    }
}
