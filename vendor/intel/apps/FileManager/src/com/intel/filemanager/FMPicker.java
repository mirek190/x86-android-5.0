package com.intel.filemanager;

import java.io.File;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Locale;

import android.app.ActionBar.LayoutParams;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.text.InputFilter;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.AbsListView.OnScrollListener;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.LogHelper;
import com.intel.filemanager.util.MediaFile;

public class FMPicker extends Activity {

    private static LogHelper Log = LogHelper.getLogger();
    private final static String TAG = "FMPicker";

    private final static int FILEPICKER = 0;
    private final static int FOLDERPICKER = 1;
    private final static int SHORTCUT = 2;
    private int mPickerType; // FILE OR FOLDER PICKER
    private boolean mNeedHideButton = false;

    private final static int OPTION_MENU_OK = 1;
    private final static int OPTION_MENU_CANCEL = 2;
    private final static int OPTION_MENU_NEWFOLDER = 3;
    private final static int OPTION_MENU_SAVESELECTED = 6;

    // result
    public static final String ACTRESULT = "com.intel.filemanager/result";

    // used for Handler
    private final static int FOLDER_EXISTS = 1;
    private final static int INVALID_HEAD = 2;
    private final static int INVALID_NAME = 3;
    private final static int LOAD_DATA_BACKGROUND = 4;

    private final static int YNDIALOG = 0;
    private final static int UNSPTDIALOG = 1;
    private final static int INVALIDHEADDIALOG = 2;
    private final static int FILENAME_TOO_LONG = 3;
    private final static int INVALIDNAMEDIALOG = 4;

    public final static String FILE_VIDEO = "video";
    public final static String FILE_AUDIO = "audio";
    public final static String FILE_IMAGE = "image";
    public final static String FILE_FOLDER = "folder";

    public final static String ACTIONS = "actions";
    public final static String ACTION_MOVE = "move";
    public final static String ACTION_SAVE = "save";
    public final static String PATH = "path";
    public final static String INITSD = Environment
            .getExternalStorageDirectory().getAbsolutePath();
    public final static String INITPHONE = "/local";

    private String CURRENTFOLDER = INITSD;
    private ArrayList<FileNode> mArrayList = new ArrayList<FileNode>();
    private ArrayList<FileNode> mArrayListTmp = null;
    private ListView mListView;
    private TextView nameText;
    private TextView actionText;

    private String mInputName = null;
    private String mActionType = null;
    private CharSequence mOriginalName = null;
    private boolean bRemoveEdittext = false; // used for the those only need the
                                             // folder path
    private boolean bNeedNewFolder = true; // some apps doesn't need new folder
    private String mDialogString = null;
    private AlertDialog mNewFileDlg = null;
    private EditText mDlgText = null;
    private TextView mNoFileView = null;

    private boolean mHas2ndCard;
    private boolean mHas3thCard;
    private boolean isInternalMaster;
    private boolean mIsRegisterReceiver = false;

    private boolean mIsScrolled = false;
    private HashMap<String, Integer> mStepTraceMap = new HashMap<String, Integer>();

    private boolean displayStyle = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        Log.v(TAG, "onCreate()");
        super.onCreate(savedInstanceState);

        getActionBar().setDisplayShowTitleEnabled(true);
        // check sdcard status
        if (!checkIfSdOK()) {
            Toast.makeText(this, R.string.sdcard_unmount, Toast.LENGTH_SHORT)
                    .show();
            this.finish();
        } else {
            // requestWindowFeature(Window.FEATURE_NO_TITLE);
            reinit(); // reset path for multi-instance env
            initRes();
            init();
            registerIntent();
            mStepTraceMap.clear();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        displayStyle = PreferenceManager.getDefaultSharedPreferences(this)
                .getBoolean("displayfilesize", false);

        setAdapter();

        Log.v(TAG, "onResume()");
    }

    @Override
    protected void onPause() {
        super.onPause();
        mInputName = nameText.getText().toString();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mIsRegisterReceiver)
            unregisterReceiver(mReceiver);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        Log.v(TAG, "onCreateOptionsMenu()");

        if (bNeedNewFolder) {
            menu.add(0, OPTION_MENU_NEWFOLDER, 0, R.string.fm_options_new);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // TODO Auto-generated method stub
        switch (item.getItemId()) {
            case OPTION_MENU_NEWFOLDER:
                doOptionNewFolder();
                break;
        }
        return false;
    }

    private void doShortcut() {
        Log.d(TAG, "enter doShortcut");
        Intent intent = new Intent();
        Intent i = new Intent("com.intel.filemanager.action.FILELISTVIEW");// "oms.filemanager.action.FILELISTVIEW"
        i.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP
                | Intent.FLAG_ACTIVITY_NEW_TASK);
        i.putExtra("Path", CURRENTFOLDER);
        i.setType("com.intel.filemanager/removablepick");
        Log.d(TAG, "CURRENTFOLDER:" + CURRENTFOLDER);
        intent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, i);
        intent.putExtra(Intent.EXTRA_SHORTCUT_NAME,
                CURRENTFOLDER.substring(CURRENTFOLDER.lastIndexOf("/") + 1));
        intent.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE,
                Intent.ShortcutIconResource.fromContext(FMPicker.this,
                        R.drawable.ic_launcher_filemanager));
        setResult(RESULT_OK, intent);
        this.finish();
    }

    private void doFolderPickerOK() {
        Log.v(TAG, "doFolderPickerOK()");
        Intent intent = new Intent(ACTRESULT);
        String name = nameText.getText().toString();
        name = FileUtil.fixFileName(name);
        if (null == name) {
            showDialog(INVALIDNAMEDIALOG);
            return;
        }
        File file = new File(CURRENTFOLDER, name);
        boolean nameValid = checkNameValid(name, file);
        Log.v(TAG, "nameValid = " + nameValid);

        if (mActionType != null) {
            if (mActionType.equals(ACTION_SAVE) && nameValid) {
                intent.setData(Uri.fromFile(file));
            } else if (mActionType.equals(ACTION_MOVE)) {
                intent.setData(Uri.fromFile(new File(CURRENTFOLDER)));
            }
        } else if (bRemoveEdittext) {
            intent.setData(Uri.fromFile(new File(CURRENTFOLDER)));
        } else if (nameValid) {
            intent.setData(Uri.fromFile(file));
        }
        Log.v(TAG, "DATA = intent  " + intent.getDataString());
        setResult(RESULT_OK, intent);
        if (nameValid) {
            this.finish();
        }
    }

    private boolean checkNameValid(String filename, File file) {
        if (null != filename && null != file) {
            byte[] utf8str = null;
            try {
                utf8str = filename.getBytes("utf-8");
            } catch (Exception e) {
                Log.e(TAG, "checkNameValid exception: " + e.toString());
            }

            if (utf8str.length > 255) {
                showDialog(FILENAME_TOO_LONG);
                nameText.setText(mOriginalName);
                return false;
            } else if (filename.contains("/") || filename.contains("\\")
                    || filename.contains(":") || filename.contains("?")
                    || filename.contains("*") || filename.contains("\"")
                    || filename.contains("|") || filename.contains("'")
                    || filename.contains("<") || filename.contains(">")) {
                showDialog(UNSPTDIALOG);
                return false;
            } else if (!FileUtil.isValidHead(filename)
                    || filename.startsWith(".")) {
                showDialog(INVALIDHEADDIALOG);
                return false;
            } else if (file.exists()) {
                showDialog(YNDIALOG);
                return false;
            }

            return true;
        }
        return false;
    }

    private boolean checkIfSdOK() {
        return FileUtil.getMountedSDCardCount(this) >= 1;
    }

    private void doFilePickerOK(String path) {
        Log.v(TAG, "entering Result OK send. mSelectedFile " + path);
        reinit();
        if (null == path) {
            Intent intent = new Intent(ACTRESULT);
            intent.setData(null);
            sendBroadcast(intent);
            return;
        }
        Intent intent = new Intent(ACTRESULT);
        intent.setData(Uri.fromFile(new File(path)));
        setResult(RESULT_OK, intent);
        this.finish();
    }

    private void doOptionCancel() {
        Log.v(TAG, "doOptionCancel()");
        setResult(RESULT_CANCELED);
        finish();
    }

    private void initDialog() {
        InputFilter[] filters = new InputFilter[1];
        filters[0] = new InputFilter.LengthFilter(255);
        if (null == mDlgText) {
            mDlgText = new EditText(this);
            mDlgText.setSingleLine();
        }
        mDlgText.setFilters(filters);

        mNewFileDlg = new AlertDialog.Builder(FMPicker.this)
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setTitle(R.string.fm_options_new)
                .setMessage("")
                .setView(mDlgText)
                .setPositiveButton(R.string.dialog_ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int whichButton) {
                                doNewFolder();
                                /* User clicked Yes so do some stuff */
                            }
                        })
                .setNegativeButton(R.string.dialog_cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int whichButton) {

                                /* User clicked No so do some stuff */
                            }
                        }).create();
    }

    private void doOptionNewFolder() {
        mNewFileDlg.setMessage(this.getResources().getString(
                R.string.enter_foldername)); // clear AlertDialog prompt message
                                             // before create new folder
        setNewFloderName();
        mNewFileDlg.show();
    }

    private void setNewFloderName() {
        int count = 1;

        File f = new File(CURRENTFOLDER + "/"
                + this.getText(R.string.fm_options_new));
        if (!f.exists()) {
            mDialogString = f.getName();
            mDlgText.setText(mDialogString);
            mDlgText.setSelection(0, mDialogString.length());
            return;
        }
        while (true) {
            f = new File(CURRENTFOLDER + "/"
                    + this.getText(R.string.fm_options_new) + "(" + (count++)
                    + ")");
            if (!f.exists()) {
                mDialogString = f.getName();
                break;
            }
        }
        mDlgText.setText(mDialogString);
        mDlgText.setSelection(0, mDialogString.length());
    }

    private void doNewFolder() {
        mNewFileDlg.setMessage(""); // clear AlertDialog prompt message before
                                    // create new folder
        String filename = mDlgText.getText().toString();
        filename = FileUtil.fixFileName(filename);
        if (null == filename || filename.startsWith(".")
                || filename.length() == 0 || filename.contains("/")
                || filename.contains("\\") || filename.contains(":")
                || filename.contains("?") || filename.contains("*")
                || filename.contains("\"") || filename.contains("|")
                || filename.contains("'") || filename.contains("<")
                || filename.contains(">")) {
            Toast.makeText(this, R.string.fm_name_invalid_name_folder,
                    Toast.LENGTH_SHORT).show();
            return;
        }

        // folder(file) name max length 254
        byte[] utf8Str = null;
        try {
            utf8Str = filename.getBytes("utf-8");
        } catch (Exception e) {

        }
        if (null != utf8Str && utf8Str.length > 254) {
            Toast.makeText(this, R.string.op_filename_too_long,
                    Toast.LENGTH_SHORT).show();
            return;
        }

        File f = new File(CURRENTFOLDER + "/" + filename);
        if (f.exists()) {
            mHandler.sendEmptyMessage(FOLDER_EXISTS);
            return;
        }
        if (filename.startsWith(".") || !FileUtil.isValidHead(filename)) {
            mHandler.sendEmptyMessage(INVALID_HEAD);
            return;
        }

        if (!FileUtil.isValidName(filename)) {
            mHandler.sendEmptyMessage(INVALID_NAME);
            return;
        }

        try {
            if (!f.mkdir()) {
                Toast.makeText(this, R.string.op_no_space, Toast.LENGTH_SHORT)
                        .show();
                Log.d(TAG, "f.mkdir() failed in doNewFolder()");
            }
            initView();
        } catch (Exception ex) {
            Log.e(TAG, "doNewFolder() Exception : " + ex.toString());
        }
    }

    private void reinit() {
        CURRENTFOLDER = INITSD;
    }

    private void initRes() {
        Log.v(TAG, "initRes()");
        setContentView(R.layout.file_picker);
        // mTitleText = (TextView) this.findViewById(R.id.actionbar_custom);
        mListView = (ListView) this.findViewById(R.id.list);
        actionText = (TextView) findViewById(R.id.saveas);
        nameText = (TextView) this.findViewById(R.id.namefield);
        initDialog();
        mNoFileView = (TextView) findViewById(R.id.no_file_picker);
        mNoFileView.setText(R.string.no_file);
        mListView.setOnItemClickListener(mClick);

        mHas2ndCard = FileUtil.has2ndCard(this);
        mHas3thCard = FileUtil.has3thCard(this);

        if (mHas3thCard) {
            final ImageView switchButton = new ImageView(this);
            switchButton.setImageResource(R.drawable.main_card_fm);
            LayoutParams params = new LayoutParams(Gravity.RIGHT);
            switchButton.setLayoutParams(params);
            switchButton.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (CURRENTFOLDER != null
                            && CURRENTFOLDER
                                    .startsWith(FileUtil.PATH_SDCARD_2)) {
                        CURRENTFOLDER = FileUtil.PATH_SDCARD_3;
                        initView();
                        switchButton.setImageResource(R.drawable.extral_card_onpressed_fm);
                    } else if (CURRENTFOLDER != null
                            && CURRENTFOLDER
                                    .startsWith(FileUtil.PATH_SDCARD_3)) {
                        CURRENTFOLDER = FileUtil.Defs.PATH_SDCARD_1;
                        switchButton.setImageResource(R.drawable.main_card_onpressed_fm);
                        initView();
                    } else if (CURRENTFOLDER != null
                            && CURRENTFOLDER
                                    .startsWith(FileUtil.Defs.PATH_SDCARD_1)) {
                        CURRENTFOLDER = FileUtil.PATH_SDCARD_2;
                        switchButton.setImageResource(R.drawable.extral_card_onpressed_fm);
                        initView();
                    }
                }
            });
            getActionBar().setCustomView(switchButton);
        } else if (mHas2ndCard) {
            final ImageView switchButton = new ImageView(this);
            switchButton.setImageResource(R.drawable.main_card_fm);
            LayoutParams params = new LayoutParams(Gravity.RIGHT);
            switchButton.setLayoutParams(params);
            switchButton.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (CURRENTFOLDER != null
                            && CURRENTFOLDER
                                    .startsWith(FileUtil.PATH_SDCARD_2)) {
                        CURRENTFOLDER = FileUtil.Defs.PATH_SDCARD_1;
                        initView();
                        switchButton.setImageResource(R.drawable.main_card_onpressed_fm);
                    } else if (CURRENTFOLDER != null
                            && CURRENTFOLDER
                                    .startsWith(FileUtil.Defs.PATH_SDCARD_1)) {
                        CURRENTFOLDER = FileUtil.PATH_SDCARD_2;
                        switchButton.setImageResource(R.drawable.extral_card_onpressed_fm);
                        initView();
                    }
                }
            });
            getActionBar().setCustomView(switchButton);
        }
        getActionBar().setDisplayShowCustomEnabled(true);

        isInternalMaster = FileUtil.isInternalMaster();

        String type = getIntent().getType();
        String action = getIntent().getAction();

        if (!TextUtils.isEmpty(action) && action.equals("android.intent.action.GET_CONTENT")
                || (!TextUtils.isEmpty(type) && (type.equals("com.intel.filemanager/filepick")
                || type.equals("android.intent.action.GET_CONTENT")))) {
            mNeedHideButton = true;
        }

        Button okButton = (Button) this.findViewById(R.id.bottom_button_ok);
        if (this.mNeedHideButton) {
            okButton.setVisibility(View.GONE);
        } else {
            okButton.setVisibility(View.VISIBLE);
        }

        okButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mPickerType == SHORTCUT) {
                    doShortcut();
                } else if (mPickerType == FOLDERPICKER) {
                    doFolderPickerOK();
                }
            }
        });

        Button cancelButton = (Button) this.findViewById(R.id.bottom_button_cancel);
        if (this.mNeedHideButton) {
            cancelButton.setVisibility(View.GONE);
        } else {
            cancelButton.setVisibility(View.VISIBLE);
        }

        cancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                doOptionCancel();
            }
        });

    }

    private OnScrollListener scrollListener = new OnScrollListener() {

        public void onScroll(AbsListView view, int firstVisibleItem,
                int visibleItemCount, int totalItemCount) {

        }

        public void onScrollStateChanged(AbsListView arg0, int arg1) {
            mIsScrolled = true;
            if (arg1 == SCROLL_STATE_IDLE) {
                savePosition();
            }
        }
    };

    /**
     * restore first visible position of the list view, force the list view
     * scroll to the file users selected before should be called when the path
     * changed or the operations finished
     */
    private void restorePosition() {
        if (mIsScrolled) {
            return;
        }
        if (mStepTraceMap.containsKey(CURRENTFOLDER)) {
            int savedPosition = mStepTraceMap.get(CURRENTFOLDER);
            if (savedPosition < mArrayList.size()) {
                mListView.setSelection(savedPosition);
                mStepTraceMap.remove(CURRENTFOLDER);
            }
            Log.d(TAG, "restore position, path: " + CURRENTFOLDER + ", position: " + savedPosition);
        }
    }

    /**
     * save the first visible position of the list view, when click the item of
     * select the item. should be called when do operations of the file items
     */

    private void savePosition() {
        int firstPosition = mListView.getFirstVisiblePosition();
        mStepTraceMap.put(CURRENTFOLDER, firstPosition);

        Log.d(TAG, "save position, path: " + CURRENTFOLDER + ", position: " + firstPosition);
    }

    private void setActionBarTitle(CharSequence title) {
        if (!mHas2ndCard
                && (CURRENTFOLDER.equals(FileUtil.Defs.PATH_SDCARD_1) || CURRENTFOLDER
                        .equals(FileUtil.PATH_SDCARD_2))) {
            title = getText(R.string.fm_title_title);
        } else  if (!mHas3thCard) {
            if (CURRENTFOLDER.equals(FileUtil.PATH_SDCARD_2)) {
                title = (String) getText(R.string.fm_title_external);
            } else if (CURRENTFOLDER.equals(FileUtil.Defs.PATH_SDCARD_1)) {
                title = (String) getText(R.string.fm_title_internal);
            }
        } else {
            if (CURRENTFOLDER.equals(FileUtil.PATH_SDCARD_3)) {
                title = (String) getText(R.string.fm_title_external2);
            } else if (CURRENTFOLDER.equals(FileUtil.PATH_SDCARD_2)) {
                title = (String) getText(R.string.fm_title_external);
            } else if (CURRENTFOLDER.equals(FileUtil.Defs.PATH_SDCARD_1)) {
                title = (String) getText(R.string.fm_title_internal);
            }
        }
        setTitle(title);
    }

    private void init() {

        Log.d(TAG, "init(), CURRENTFOLDER:" + CURRENTFOLDER);
        Intent intent = getIntent();
        String type = intent.getType();

        // judge the picker type from intent. filepicker? folderpicker?
        // shortcut?
        Log.v(TAG, "intent.getAction = " + intent.getAction());
        boolean isShortCut = (intent.getAction()
                .equals(Intent.ACTION_CREATE_SHORTCUT)) ? true : false;
        Log.v(TAG, "picker type = " + type);

        if (isShortCut) {
            mPickerType = SHORTCUT;
        } else {
            if (TextUtils.isEmpty(type)) {
                FMPicker.this.finish();
            } else {
                if (type.equals("com.intel.filemanager/filepick")
                        || type.equals("android.intent.action.GET_CONTENT")) {
                    mPickerType = FILEPICKER;
                } else if (type.equals("com.intel.filemanager/folderpick")) {
                    mPickerType = FOLDERPICKER;
                }
            }
        }

        if (mPickerType == FILEPICKER) {
            findViewById(R.id.nameline).setVisibility(View.GONE);
            findViewById(R.id.seperator).setVisibility(View.GONE);
            if (intent.hasExtra("newfolder")) {
                bNeedNewFolder = intent.getBooleanExtra("newfolder", true);
            }
        } else if (mPickerType == FOLDERPICKER) {
            if (intent.hasExtra("name")) {
                Log.v(TAG,
                        "get extera name === " + intent.getStringExtra("name")
                                + " and current is " + mInputName + " .. "
                                + nameText.getText());
                String temp = nameText.getText().toString();
                if (temp != null && temp.length() > 0) {
                    mInputName = temp;
                }
                if (mInputName == null) {
                    mInputName = intent.getStringExtra("name");
                    mOriginalName = mInputName;
                }
            }
            if (intent.hasExtra("rmnametext")) {
                bRemoveEdittext = intent.getBooleanExtra("rmnametext", false);
            }
            if (bRemoveEdittext) {
                findViewById(R.id.nameline).setVisibility(View.GONE);
                findViewById(R.id.seperator).setVisibility(View.GONE);
            } else {
                findViewById(R.id.nameline).setVisibility(View.VISIBLE);
                findViewById(R.id.seperator).setVisibility(View.VISIBLE);
            }
            if (intent.hasExtra("newfolder")) {
                bNeedNewFolder = intent.getBooleanExtra("newfolder", true);
            }
            if (intent.hasExtra(ACTIONS)) {
                mActionType = intent.getStringExtra(ACTIONS);
            }
            if (mInputName == null) {
                nameText.setText(R.string.fm_save_tmp);
            } else if (0 == mInputName.length()) {
                nameText.setText(R.string.fm_save_tmp);
            } else {
                Log.v(TAG, "mInputName = " + mInputName);
                nameText.setText(mInputName);
                nameText.setSelected(true);
                if (mInputName.contains(".")) {
                    ((EditText) nameText).setSelection(0,
                            (mInputName.length() - 4));
                }
            }
            if (mActionType != null) {
                if (mActionType.equals(ACTION_MOVE)) {
                    nameText.setVisibility(View.INVISIBLE);
                    actionText.setText(R.string.fm_move_moveto);
                }
            }
        } else if (mPickerType == SHORTCUT) {
            mPickerType = SHORTCUT;
            findViewById(R.id.nameline).setVisibility(View.GONE);
            findViewById(R.id.seperator).setVisibility(View.GONE);
            Log.d(TAG, "shortcut   init, CURRENTFOLDER:" + CURRENTFOLDER);
        }

        Button okButton = (Button) this.findViewById(R.id.bottom_button_ok);
        if (mPickerType == SHORTCUT) {
            okButton.setText(R.string.picker_button_choose);
        } else {
            okButton.setText(R.string.picker_button_save);
        }

        initView();
    }

    private void initView() {
        File file = new File(CURRENTFOLDER);
        if (null == file)
            finish();
        else {
            if (false == file.exists())
                finish();
        }

        final File files[] = FileOperator.getSubFileArray(CURRENTFOLDER, this);

        if (files == null || files.length == 0) {
            if (mListView != null) {
                mListView.setAdapter(null);
            }
            mNoFileView.setVisibility(View.VISIBLE);
            mListView.setVisibility(View.GONE);
            setActionBarTitle(CURRENTFOLDER);
            return;
        }

        mNoFileView.setVisibility(View.GONE);
        mListView.setVisibility(View.VISIBLE);
        setActionBarTitle(CURRENTFOLDER);

        new Thread(new Runnable() {
            @Override
            public void run() {
                getFileType(files);
            }

        }).start();
    }

    private void sortList(int type) {
        Collections.sort(mArrayList, new CollatorComparator(type));
    }

    public static class CollatorComparator // orderDefaultFolder static
            implements Comparator {

        int comparetype = 0;

        long fileSize = 0;

        Collator collator = Collator.getInstance(Locale.CHINA);

        public CollatorComparator(int type) {
            this.comparetype = type;
        }

        public int compare(Object o1, Object o2) {
            // TODO Auto-generated method stub
            FileNode a1 = (FileNode) o1;
            FileNode a2 = (FileNode) o2;
            String s1, s2;
            Long l1, l2;
            boolean f1 = (Boolean) a1.isFile;
            boolean f2 = (Boolean) a2.isFile;

            s1 = a1.name;
            s2 = a2.name;

            if (!f1 && f2) {
                return -1;
            } else if (f1 && !f2) {
                return 1;
            }
            switch (comparetype) {
                case 0: // by if file

                    break;
                case 1: // by name
                    int ret = collator.compare(s1, s2);
                    return ret;
                case 2: // by size
                    if (!f1 && f2) {
                        break;
                    }
                    s1 = String.valueOf(a1.lengthInByte);
                    s2 = String.valueOf(a2.lengthInByte);
                    l1 = Long.parseLong(s1);
                    l2 = Long.parseLong(s2);
                    if (l1 > l2) {
                        return 1;
                    } else if (l1 < l2) {
                        return -1;
                    } else {
                        return s1.compareToIgnoreCase(s2);
                    }
                case 3: // by last time modify
                    s1 = String.valueOf(a1.lastModifiedTime);
                    s2 = String.valueOf(a2.lastModifiedTime);
                    l1 = Long.parseLong(s1);
                    l2 = Long.parseLong(s2);
                    if (l1 > l2) {
                        return -1;
                    } else if (l1 <= l2) {
                        return 1;
                    }
                case 4: // by file type
                    if (!f1 && f2) {
                        return s1.compareToIgnoreCase(s2);
                    }
                    if (f2 && f1) {
                        int temp1 = s1.lastIndexOf(".");
                        int temp2 = s2.lastIndexOf(".");
                        if (temp1 == -1 && temp2 != -1) {
                            return -1;
                        } else if (temp2 == -1 && temp1 != -1) {
                            return 1;
                        } else if (temp1 == -1 && temp2 == -1) {
                            return s1.compareToIgnoreCase(s2);
                        } else {
                            String sub1 = s1.substring(temp1);
                            String sub2 = s2.substring(temp2);
                            if (!sub1.equalsIgnoreCase(sub2)) {
                                return sub1.compareToIgnoreCase(sub2);
                            } else {
                                return s1.compareToIgnoreCase(s2);
                            }
                        }
                    }

            }
            return s1.compareToIgnoreCase(s2);
        }
    }

    private final static String UNDELFILE = "lost+found";

    private void getFileType(File files[]) {
        mArrayListTmp = new ArrayList<FileNode>();

        for (int i = 0; i < files.length; i++) {
            getFileItemInfo(files[i]);
        }

        mArrayList = mArrayListTmp;
        mHandler.sendEmptyMessage(LOAD_DATA_BACKGROUND);
        Log.v(TAG, "HOW MANY FILES ? " + mArrayList.size());
    }

    private void getFileItemInfo(File file) {
        // get the info of file, then add to mArrayList
        if (null == file) {
            Log.v(TAG, "The param for getFileItemInfo() is invalid!");
            return;
        }
        String filePath = file.getAbsolutePath();
        String fileName = file.getName();
        boolean hasThumbnail = false;
        int fileIcon = 0;
        String mimeType = "";
        FileNode temp = new FileNode();

        if (file.isFile()) {
            if (fileName.endsWith(".mp4") || fileName.endsWith(".3gp")
                    || fileName.endsWith(".asf")) {
                mimeType = MediaFile.getRealMimeType(this,
                        file.getAbsolutePath());
            } else if (fileName.endsWith(".dcf") || fileName.endsWith("dm")) {
                mimeType = MediaFile.getRealMimeType(this,
                        file.getAbsolutePath());
            } else {
                mimeType = MediaFile.getMimeType(file.getAbsolutePath());
            }
            fileIcon = FileUtil.getIconByMimeType(mimeType);

            int type = MediaFile.getFileTypeForMimeType(mimeType);
            if (MediaFile.isVideoFileType(type)
                    || MediaFile.isImageFileType(type)
                    || type == MediaFile.FILE_TYPE_APK) {
                hasThumbnail = true;
            }
        } else {
            fileIcon = R.drawable.file_icon_folder;
            mimeType = FileNode.FILE_FOLDER;
        }

        temp.id = mArrayListTmp.size();
        temp.iconRes = fileIcon;
        temp.path = filePath;
        temp.name = fileName;
        temp.isFile = file.isFile();
        temp.lastModifiedTime = file.lastModified();
        temp.lengthInByte = file.length();
        temp.type = mimeType;
        temp.hasThumbnail = hasThumbnail;
        mArrayListTmp.add(temp);
        temp = null;
    }

    private void toParentsView() {
        if (CURRENTFOLDER.equals(INITSD) || CURRENTFOLDER.equals(INITPHONE)
                || CURRENTFOLDER.equals(FileUtil.PATH_SDCARD_2) || CURRENTFOLDER.equals(FileUtil.PATH_SDCARD_3)) {
            this.finish();
            return;
        }
        File f = new File(CURRENTFOLDER);
        CURRENTFOLDER = f.getParent();
        initView();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                toParentsView();
                return true;
        }
        return false;
    }

    private AdapterView.OnItemClickListener mClick = new AdapterView.OnItemClickListener() {

        public void onItemClick(AdapterView<?> arg0, View arg1, int position,
                long arg3) {
            savePosition();
            File file = new File(CURRENTFOLDER + "/"
                    + mArrayList.get(position).name);
            if (file.isDirectory()) {
                CURRENTFOLDER = file.getAbsolutePath();
                initView();
            } else {
                if (mPickerType == FILEPICKER) {
                    doFilePickerOK(CURRENTFOLDER + "/"
                            + mArrayList.get(position).name);
                } else if (mPickerType == SHORTCUT) {
                    nameText.setText(mArrayList.get(position).name);
                }
            }
        }

    };

    @Override
    protected Dialog onCreateDialog(int id) {
        // TODO Auto-generated method stub

        switch (id) {
            case YNDIALOG:
                return new AlertDialog.Builder(FMPicker.this)
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .setTitle(R.string.yn_dialog_title)
                        .setMessage(R.string.op_file_exists)
                        .setPositiveButton(R.string.dialog_yes,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                            int whichButton) {
                                        try {
                                            String name = nameText.getText()
                                                    .toString();
                                            name = FileUtil.fixFileName(name);
                                            File file = new File(CURRENTFOLDER,
                                                    name);
                                            file.delete();
                                        } catch (Exception ex) {
                                        }
                                        doFolderPickerOK();
                                        /* User clicked Yes so do some stuff */
                                    }
                                })
                        .setNegativeButton(R.string.dialog_no,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                            int whichButton) {
                                        /* User clicked No so do some stuff */
                                    }
                                }).create();

            case UNSPTDIALOG:
                return new AlertDialog.Builder(FMPicker.this)
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .setTitle(R.string.unspt_dialog_title)
                        .setMessage(R.string.text_contain_unacceptable_char)
                        .setPositiveButton(R.string.dialog_ok,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                            int whichButton) {
                                        Log.v(TAG, "Unsupport character input!");
                                    }
                                }).create();

            case INVALIDHEADDIALOG:
                return new AlertDialog.Builder(FMPicker.this)
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .setTitle(R.string.unspt_dialog_title)
                        .setMessage(R.string.fm_name_invalid_head)
                        .setPositiveButton(R.string.dialog_ok,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                            int whichButton) {

                                    }
                                }).create();

            case FILENAME_TOO_LONG:
                return new AlertDialog.Builder(FMPicker.this)
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .setTitle(R.string.unspt_dialog_title)
                        .setMessage(R.string.op_filename_too_long)
                        .setPositiveButton(R.string.dialog_ok,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                            int whichButton) {

                                    }
                                }).create();

            case INVALIDNAMEDIALOG:
                return new AlertDialog.Builder(FMPicker.this)
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .setTitle(R.string.unspt_dialog_title)
                        .setMessage(R.string.fm_name_invalid_name)
                        .setPositiveButton(R.string.dialog_ok,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                            int whichButton) {
                                        Log.v(TAG, "Unsupport character input!");
                                    }
                                }).create();
        }
        return null;

    }

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case FOLDER_EXISTS:
                    mNewFileDlg.setMessage(FMPicker.this
                            .getText(R.string.fm_paste_folder_already_exist));
                    mDlgText.setText(mDialogString);
                    mDlgText.setSelection(mDialogString.length());
                    mNewFileDlg.show();
                    break;

                case INVALID_HEAD:
                    mNewFileDlg.setMessage(FMPicker.this
                            .getText(R.string.fm_name_invalid_head_folder));
                    mDlgText.setText(mDialogString);
                    mDlgText.setSelection(mDialogString.length());
                    mNewFileDlg.show();
                    break;

                case INVALID_NAME:
                    mNewFileDlg.setMessage(FMPicker.this
                            .getText(R.string.fm_name_invalid_name_folder));
                    mDlgText.setText(mDialogString);
                    mDlgText.setSelection(mDialogString.length());
                    mNewFileDlg.show();
                    break;
                case LOAD_DATA_BACKGROUND:
                    sortList(1);
                    setAdapter();
                    break;
            }
        }
    };

    private void setAdapter() {
        FMAdapter adapter = new FMAdapter(this, FMAdapter.LISTVIEW,
                FMPicker.this.mArrayList, this.CURRENTFOLDER, mListView,
                false, 0, FMAdapter.FLAG_PICK, displayStyle);
        mListView.setAdapter(adapter);
        restorePosition();
        adapter.loadThumbnail();
    }

    protected BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.v(TAG, "onReceiveIntent " + intent.getAction());
            String action = intent.getAction();
            Log.v(TAG, "action = " + action);
            if (!action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
                Log.v(TAG,
                        "Intent.ACTION_MEDIA_EJECT Intent.ACTION_MEDIA_UNMOUNTED Intent.ACTION_MEDIA_KILL_ALL ");
                closeOptionsMenu();
                FMPicker.this.finish();
                Toast.makeText(FMPicker.this, R.string.sdcard_unmount,
                        Toast.LENGTH_SHORT).show();
            }
        }
    };

    private void registerIntent() {
        IntentFilter intentFilter = new IntentFilter(
                Intent.ACTION_MEDIA_MOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
        // intentFilter.addAction(Intent.ACTION_MEDIA_KILL_ALL);
        intentFilter.addAction(Intent.ACTION_MEDIA_REMOVED);
        intentFilter.addDataScheme("file");
        registerReceiver(mReceiver, intentFilter);
        mIsRegisterReceiver = true;
    }

}
