
package com.intel.filemanager;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.LogHelper;
import com.intel.filemanager.util.MediaFile;

import android.text.TextUtils;
import android.view.View;
import android.view.Window;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import android.os.StatFs;

public class FileDetailInfor extends Activity implements FileUtil.Defs {

    private static LogHelper Log = LogHelper.getLogger();

    private final static String TAG = "FileDetailInfor";

    public static final String PATHTAG = "path";

    public static final String TYPETAG = "type";

    private TextView mTVCountFile;

    private TextView mTVCountFolder;

    private TextView mTVSize;

    private TextView mFileName;

    // private TextView mFilePath;

    // private TextView mLastModify;

    // private TextView mType;

    private int fileCount = 0;

    private int folderCount = 0;

    private long fileSize = 0;

    private String path = "";

    private String type = "";

    private boolean bHas2ndCard = false;
    private boolean bHas3thCard = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.file_info);
        bHas2ndCard = FileUtil.has2ndCard(this);
        bHas3thCard = FileUtil.has3thCard(this);
        Bundle b = getIntent().getExtras();
        Log.v(TAG, "B == " + b);
        path = b.getString(PATHTAG);
        type = b.getString(TYPETAG);
        init();
        ShowThread.start();
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.v(TAG, "onresume()...");
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.v(TAG, "onPause()...");
    }

    @Override
    protected void onStop() {
        ShowThread.interrupt();
        super.onStop();
        this.finish();
    }

    private Thread ShowThread = new Thread() {

        @Override
        public void run() {
            try {
                getFilePre(new File(path));
            } catch (InterruptedException e) {
                Log.d(TAG, e.toString());
            }
            Log.v(TAG, "SIZE OF = " + fileSize);
        }
    };

    private void init() {
        mTVCountFolder = (TextView) findViewById(R.id.containfolder);
        mTVCountFile = (TextView) findViewById(R.id.containfile);
        mTVSize = ((TextView) findViewById(R.id.size));
        mFileName = ((TextView) findViewById(R.id.name));
        // mFilePath = ((TextView) findViewById(R.id.path));
        // mLastModify = ((TextView) findViewById(R.id.lastmodified));
        // mType = (TextView) findViewById(R.id.exttype);

        File f = new File(path);

        if (f.getName().equals(PATH_LOCAL)) {
            mFileName.setText(R.string.infor_detail_phone);
            // mFilePath.setText(f.getAbsolutePath());
        } else if (f.getName().equals(PATH_SDCARD_1)) {
            mFileName.setText(R.string.infor_detail_sd);
            // mFilePath.setText(f.getAbsolutePath());
        } else if (FileUtil.isExternalCard(this, f.getAbsolutePath(), bHas2ndCard)) {
            mFileName.setText(getString(R.string.fm_title_external));
            // mFilePath.setText(getString(R.string.fm_title_external));
        } else if (FileUtil.isExternalCard(this, f.getAbsolutePath(), bHas3thCard)) {
            mFileName.setText(getString(R.string.fm_title_external2));
            // mFilePath.setText(getString(R.string.fm_title_external));
        } else if (FileUtil.isInternalCard(this, f.getAbsolutePath(), bHas2ndCard)) {
            mFileName.setText(getString(R.string.fm_title_internal));
            // mFilePath.setText(getString(R.string.fm_title_internal));
        } else if (FileUtil.isInternalCard(this, f.getAbsolutePath(), bHas3thCard)) {
            mFileName.setText(getString(R.string.fm_title_internal));
            // mFilePath.setText(getString(R.string.fm_title_internal));
        } else {
            mFileName.setText(f.getName());
            // if(f.getParentFile()==null){
            // mFilePath.setText(f.getAbsolutePath());
            // }else{
            // mFilePath.setText(f.getParentFile().getAbsolutePath());
            // }
        }
        if (f.isFile()) {
            this.setTitle(R.string.file_info_file_property);
            findViewById(R.id.containfolderline).setVisibility(View.GONE);
            // findViewById(R.id.containfileline).setVisibility(View.GONE);
            // if (type.length() > 0) {
            // if (type.equalsIgnoreCase(MediaFile.BINARY_MIMETYPE))
            // mType.setText(R.string.infor_type_unknow);
            // else
            // mType.setText(type);
            // } else {
            // mType.setText(R.string.infor_type_unknow);
            // }
        } else if (f.isDirectory()) {
            this.setTitle(R.string.file_info_folder_property);
            // mType.setText(R.string.file_info_ext_folder);

            // if sdcard root path, will not show the last modify time.
            // if (path.equals("/local")
            // ||
            // path.equals(Environment.getExternalStorageDirectory().getAbsolutePath()))
            // {
            // ((View)
            // findViewById(R.id.lastmodifiedline)).setVisibility(View.GONE);
            // } else {
            // ((TextView) findViewById(R.id.lastmodify_text))
            // .setText(R.string.file_info_last_modified_label);
            // }
        }
        // Date date = new Date(f.lastModified());
        // Log.v(TAG,
        // "Settings.System.DATE_FORMAT == "
        // + Settings.System.getString(this.getContentResolver(),
        // Settings.System.DATE_FORMAT));
        //
        // String format = Settings.System.getString(this.getContentResolver(),
        // Settings.System.DATE_FORMAT);
        // if (TextUtils.isEmpty(format)) {
        // format = "yyyy-MM-dd";
        // }
        // SimpleDateFormat formatter;
        //
        // formatter = new SimpleDateFormat(format);
        //
        // String strDate = formatter.format(date);
        // mLastModify.setText(strDate);

        if (bHas2ndCard && path.startsWith(FileUtil.PATH_SDCARD_2)) {
            getDeviceInfoOnLinux(FileUtil.PATH_SDCARD_2);
        } else if (bHas3thCard && path.startsWith(FileUtil.PATH_SDCARD_3)) {
            getDeviceInfoOnLinux(FileUtil.PATH_SDCARD_3);
        } else if (path.startsWith(PATH_T2_SDCARD_2)) {
            getDeviceInfoOnLinux(PATH_T2_SDCARD_2);
        } else if (path.startsWith(PATH_SDCARD_1)) {
            getDeviceInfoOnLinux(PATH_SDCARD_1);
        } else if (path.startsWith(PATH_LOCAL)) {
            getDeviceInfoOnLinux(PATH_LOCAL);
        }

        Button positive_button = (Button) findViewById(R.id.positive_button);
        positive_button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                finish();
            }
        });
    }

    final File rootFile = Environment.getExternalStorageDirectory();

    private void getFilePre(File file) throws InterruptedException {
        if (!file.exists()) {
            if (ShowThread.isAlive() && !rootFile.exists()) {
                Log.v(TAG, "FILE NOT FOUND " + file);
                this.finish();
                ShowThread.stop();
            }
            return;
        }
        if (file.isFile()) {
            fileCount++;
            fileSize += file.length();
        } else if (file.isDirectory()) {
            try {
                folderCount++;
                File[] files = file.listFiles();
                for (File f : files) {
                    getFilePre(f);
                    Thread.sleep(10);
                }
            } catch (Exception e) {
                if (e instanceof InterruptedException)
                    throw (InterruptedException) e;
            }
        }
        mHandler.sendEmptyMessage(0);
    }

    private Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case 0:
                    setNewValue();
                    break;
                case 1:
                    Toast.makeText(FileDetailInfor.this, R.string.sdcard_unmount,
                            Toast.LENGTH_SHORT).show();
                    break;
            }
        }

    };

    private void setNewValue() {
        mTVCountFile.setText("  " + fileCount);
        mTVCountFolder.setText("  " + (folderCount - 1));
        if (!(path.equals("/local") || path.equals("/sdcard"))) {
            mTVSize.setText(formatSize(fileSize));
        }
    }

    private String formatSize(long size) {
        long appsize = 0;
        if (size == 0) {
            return "0Byte";
        }
        String suffix = null;
        // add K or M suffix if size is greater than 1K or 1M
        if (size >= 1024) {
            suffix = "K";
            appsize = size % 1024;
            size = size >> 10;
            if (size >= 1024) {
                suffix = "M";
                appsize = size % 1024;
                size = size >> 10;
            }
        } else {
            suffix = "Byte";
        }

        StringBuilder resultBuffer = new StringBuilder(Long.toString(size));

        int commaOffset = resultBuffer.length() - 3;
        while (commaOffset > 0) {
            resultBuffer.insert(commaOffset, ',');
            commaOffset -= 3;
        }
        // if(size < 10){
        resultBuffer.append("." + (appsize / 100));
        // }
        if (suffix != null)
            resultBuffer.append(suffix);
        return resultBuffer.toString();
    }

    private void getDeviceInfoOnLinux(String path) {
        StatFs stat = new StatFs(path);
        long blockSize = stat.getBlockSize();
        long totalBlocks = stat.getBlockCount();
        long availableBlocks = stat.getAvailableBlocks();
        long usedBlocks = totalBlocks - availableBlocks;

        mTVSize.setText(formatSize(blockSize * usedBlocks));
    }
}
