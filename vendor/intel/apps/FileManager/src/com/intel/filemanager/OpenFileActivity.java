package com.intel.filemanager;

import java.io.File;
import java.util.List;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FileDBOP;
import com.intel.filemanager.util.LogHelper;
import com.intel.filemanager.util.MediaFile;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

public class OpenFileActivity extends Activity {

    private static LogHelper Log = LogHelper.getLogger();
    private final static String TAG = "OpenFileActivity";
    private final static int PROGDIALOG = 1;

    private LayoutInflater mInflater;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        mInflater = LayoutInflater.from(this);
        showDialog(PROGDIALOG);

        Intent intent = this.getIntent();
        // String path = intent.getDataString();

        // Uri fileUri = Uri.fromFile(new File(path));
        Uri fileUri = intent.getData();
        if (fileUri != null && fileUri.toString().length() > 0) {
            String schema = fileUri.getScheme();
            if (!schema.equals("file")) {
                Toast.makeText(this, R.string.op_uri_error, Toast.LENGTH_SHORT)
                        .show();
                this.finish();
                return;
            }
        } else {
            Toast.makeText(this, R.string.op_uri_error, Toast.LENGTH_SHORT)
                    .show();
            this.finish();
            return;
        }

        String path = fileUri.getPath();
        Log.v(TAG, "FILE PATH = " + path);
        File f = new File(path);
        if (!f.exists()) {
            Log.v(TAG, "file not exist finish");
            Toast.makeText(this, R.string.fm_op_open_fail, Toast.LENGTH_SHORT)
                    .show();
            this.finish();
            return;
        }
        if (!path.startsWith("/local")
                && !path.startsWith(Environment.getExternalStorageDirectory()
                        .getAbsolutePath())) {
            Log.v(TAG, "file don't have permission finish");
            Toast.makeText(this, R.string.op_permission_deny,
                    Toast.LENGTH_SHORT).show();
            this.finish();
            return;
        }

        String mimetype = null;
        if (MediaFile.isMediaFile(path)) {
            mimetype = FileDBOP.getFileType(getContentResolver(), path);
        } else {
            mimetype = MediaFile.getMimeType(path);
        }

        Log.v(TAG, "fileUri = " + fileUri + " MIMETYPE = " + mimetype);

        if (mimetype == null) {
            Toast.makeText(this, R.string.fm_cannot_open_file,
                    Toast.LENGTH_SHORT).show();
            this.finish();
            return;
        }
        dismissDialog(PROGDIALOG);
        Intent commIntent = new Intent();
        commIntent.setAction(Intent.ACTION_VIEW);
        commIntent.setDataAndType(fileUri, mimetype);
        PackageManager mPackageManager = this.getPackageManager();

        List<ResolveInfo> mResList = mPackageManager.queryIntentActivities(
                commIntent, PackageManager.MATCH_DEFAULT_ONLY);
        if ((null == mResList) || (mResList.isEmpty())
                || (MediaFile.getFileTypeForSupportedMimeType(mimetype) == 0)) {
            Toast.makeText(this, R.string.fm_op_open_fail, Toast.LENGTH_SHORT)
                    .show();
            this.finish();
            return;
        }
        startActivity(commIntent);
        this.finish();
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        // TODO Auto-generated method stub
        switch (id) {
            case PROGDIALOG:
                View content1 = mInflater.inflate(
                        R.layout.dialog_progress_circular, null);
                TextView tx = (TextView) content1.findViewById(R.id.text1);
                tx.setText(R.string.op_open_app);
                return new AlertDialog.Builder(OpenFileActivity.this)
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .setTitle(R.string.op_search_app).setView(content1)
                        .create();
        }
        return null;
    }

}
