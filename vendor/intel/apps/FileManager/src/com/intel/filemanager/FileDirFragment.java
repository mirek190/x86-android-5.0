package com.intel.filemanager;

import java.io.File;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Fragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.DialogInterface.OnKeyListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.Parcelable;
import android.preference.PreferenceManager;
import android.text.InputFilter;
import android.text.TextUtils;
import android.view.ActionMode;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.filemanager.R;
import com.intel.filemanager.util.EditTextLengthFilter;
import com.intel.filemanager.util.FMThumbnailHelper;
import com.intel.filemanager.util.FileDBOP;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.FileUtil.Defs;
import com.intel.filemanager.util.LogHelper;
import com.intel.filemanager.util.MediaFile;
import com.intel.filemanager.view.OperationView;

public class FileDirFragment extends Fragment implements FileUtil.Defs {

    private static LogHelper Log = LogHelper.getLogger();

    public final static String TAG = "FileDirFragment";

    private final static int OP_RENAMEFILE = 0;

    private final static int OP_RENAMEFOLDER = 1;

    private final static int OP_ZIP = 3;

    private final static int OP_NEWFLODER = 4;

    private final static int FOLDER_TYPE = 4;

    private final static int MAX_INPUT = 80;

    private final static int UPDATELIST = 0;

    private final static int LOADBACKGROUND = 2;

    private final static int GOT_ALL_SELECTED = 5;

    public final static String UPDATETITLE = "updatetitle";

    public static boolean ISOPSUCCESS = true;

    private boolean moveToParentFolder = false;

    private boolean dialogDismiss = false;

    private static String CURRENTSD = Environment.getExternalStorageDirectory()
            .getAbsolutePath();

    private static String CURRENTPHONE = "/local";

    public final static String INITSD = Environment
            .getExternalStorageDirectory().getAbsolutePath();

    public final static String INITPHONE = "/local";
    // T2 phone external path, different with others
    public final static String T2_EXTERNAL_PATH = "/mnt/sdcard/sdcard_ext";

    public static String CURRENTPATH;

    public final static String SEPARATOR = File.pathSeparator;

    private int mSortMode = FileUtil.SORT_BY_NAME;

    private static int SHOWMODE = 0;

    private static int OLDSHOWMODE = 0;

    public static int TASKTYPE = -1;
    public boolean mFlag = false;

    // store the last path for launch search.
    private String OLDPATH;

    // last modified time the file
    private long mLastModified;

    private GridView mGridView;

    private AdapterView<?> mAdapterView;

    private ListView mListView;

    private TextView mNoFileView = null;

    private ArrayList<FileNode> mArrayList = new ArrayList<FileNode>();

    private HashMap<String, Integer> mStepTraceMap = new HashMap<String, Integer>();
    private boolean mIsScrolled = false;

    private Toast toast = null;

    private EditText mEditText = null;

    private static String srcPath = "";

    private String dialogText = "";

    private String mMimeType = "";

    private int operatetype = 0;

    private boolean allowdismiss = true;

    private boolean isMultiTrans = false;

    public int mFilesLen = -1;

    private int mSelectedFileCnt = 0;

    private int mFolderCount = 0;

    public boolean mLaunchedWithPath = false;

    private String mLaunchedPath;

    public boolean mIsLoading;

    public static boolean bHas2ndCard; // added for snnaper which mount the
                                       // external
    public static boolean bHas3thCard; // added for snnaper which mount the
                                        // external

    private boolean inited = false;

    private boolean click_event_processing = false;

    private boolean mOptionPrepared = false;
    private ImageView mSwitchButton;
    private TextView mTitleView;

    private boolean mAllSelected = false;

    private boolean mInActionMode = false;

    private ActionMode mMode;

    private ModeCallback mModeCallback = new ModeCallback();

    private FMAdapter mAdapt;

    private boolean isInternalMaster = true;

    private Operation mCurOp = null;

    private int mOpType = -1;

    private Activity mActivity;

    private View mBottomButton;

    private boolean mIsPasteMode;

    private UpdateViewThread mFileLoadThread;

    private static final String DIR_SORT_MODE = "dir_sort_mode";

    private boolean ifDisplayFileSize;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mActivity = getActivity();
        mSortMode = PreferenceManager.getDefaultSharedPreferences(mActivity)
                .getInt(DIR_SORT_MODE, FileUtil.SORT_BY_NAME);

        if (FileMainActivity.isFirstLaunch == true) {
            ifDisplayFileSize = PreferenceManager.getDefaultSharedPreferences(mActivity)
                    .getBoolean("displayfilesize", true);
        }

        bHas2ndCard = FileUtil.has2ndCard(mActivity);
        bHas3thCard = FileUtil.has3thCard(mActivity);
        mOptionPrepared = true;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.file_directory, container, false);
        initRes(rootView);
        init();
        inited = true;
        SHOWMODE = FMAdapter.LISTVIEW;

        mActivity.getActionBar().setDisplayHomeAsUpEnabled(true);
        return rootView;
    }

    private void initMasterState() {
        isInternalMaster = FileUtil.isInternalMaster();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (null != mAdapt) {
            mAdapt.notifyConfigChange();
        }
    }

    private void initRes(View rootView) {
        mListView = (ListView) rootView.findViewById(R.id.file_list);
        mListView.setOnItemClickListener(itemclick);
        mListView.setOnScrollListener(scrollListener);
        mListView.setVisibility(View.GONE);
        mListView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE_MODAL);
        mListView.setMultiChoiceModeListener(mModeCallback);
        this.registerForContextMenu(mListView);
        mAdapterView = mListView;

        mGridView = (GridView) rootView.findViewById(R.id.file_grid);
        mGridView.setOnItemClickListener(itemclick);
        mGridView.setOnScrollListener(scrollListener);
        mGridView.setVisibility(View.GONE);
        mGridView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE_MODAL);
        mGridView.setMultiChoiceModeListener(mModeCallback);
        this.registerForContextMenu(mGridView);

        mNoFileView = (TextView) rootView.findViewById(android.R.id.empty);
        mNoFileView.setText(R.string.no_file);

        mSwitchButton = (ImageView) rootView.findViewById(R.id.switch_sd_button);
        mSwitchButton.setImageResource(R.drawable.main_card);
        if(bHas3thCard) {
            mSwitchButton.setVisibility(bHas3thCard ? View.VISIBLE : View.GONE);
        } else if (bHas2ndCard) {
            mSwitchButton.setVisibility(bHas2ndCard ? View.VISIBLE : View.GONE);
        }
        mSwitchButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mStepTraceMap.clear();
                bHas3thCard = FileUtil.has3thCard(mActivity);
                if(bHas3thCard == true) {
                    if (CURRENTPATH != null && CURRENTPATH.equals(FileUtil.PATH_SDCARD_3)) {
                        setPath(INITSD);
                        mSwitchButton.setImageResource(R.drawable.main_card_onpressed);
                    } else if (CURRENTPATH != null && CURRENTPATH.equals(FileUtil.PATH_SDCARD_2)) {
                        setPath(FileUtil.PATH_SDCARD_3);
                        mSwitchButton.setImageResource(R.drawable.extral_card_onpressed);
                    } else if (CURRENTPATH != null && CURRENTPATH.equals(INITSD)) {
                        setPath(FileUtil.PATH_SDCARD_2);
                        mSwitchButton.setImageResource(R.drawable.extral_card_onpressed);
                    }
                } else {
                    if (CURRENTPATH != null && CURRENTPATH.equals(FileUtil.PATH_SDCARD_2)) {
                        setPath(INITSD);
                        mSwitchButton.setImageResource(R.drawable.extral_card_onpressed);
                    } else if (CURRENTPATH != null && CURRENTPATH.equals(INITSD)) {
                        setPath(FileUtil.PATH_SDCARD_2);
                        mSwitchButton.setImageResource(R.drawable.extral_card_onpressed);
                    }
                }
                changeBottom(null);
            }
        });

        ImageView homeButton = (ImageView) rootView.findViewById(R.id.home_button);
        homeButton.setImageResource(R.drawable.home_button);
        homeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (CURRENTPATH != null && !TextUtils.isEmpty(FileUtil.PATH_SDCARD_3)
                        && CURRENTPATH.equals(FileUtil.PATH_SDCARD_3)) {
                    if (CURRENTPATH.equals(FileUtil.PATH_SDCARD_3)) {
                        showToast(getString(R.string.sd_rootpath));
                        return;
                    }
                    setPath(FileUtil.PATH_SDCARD_3);
                } else if (CURRENTPATH != null && !TextUtils.isEmpty(FileUtil.PATH_SDCARD_2)
                        && CURRENTPATH.equals(FileUtil.PATH_SDCARD_2)) {
                    if (CURRENTPATH.equals(FileUtil.PATH_SDCARD_2)) {
                        showToast(getString(R.string.sd_rootpath));
                        return;
                    }
                    setPath(FileUtil.PATH_SDCARD_2);
                } else if (CURRENTPATH != null && CURRENTPATH.startsWith(INITSD)) {
                    if (CURRENTPATH.equals(INITSD)) {
                        showToast(getString(R.string.sd_rootpath));
                        return;
                    }
                    setPath(INITSD);
                }
                changeBottom(null);
            }
        });

        mBottomButton = rootView.findViewById(R.id.bottom_button);
        Button pasteButton = (Button) rootView.findViewById(R.id.bottom_button_paste);
        pasteButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (null != mCurOp)
                    mCurOp.setExit();
                mCurOp = new OperationPaste(mActivity, CURRENTPATH, FileOperator.copyPath);
                mCurOp.setOnOperationEndListener(mOperationListener);
                mCurOp.start();
            }
        });

        Button cancelButton = (Button) rootView.findViewById(R.id.bottom_button_cancel);
        cancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                FileOperator.isCanPaste = false;
                hidePasteButton();
            }
        });

        mTitleView = (TextView) rootView.findViewById(R.id.title_button);
    }

    public void showPasteButton() {
        mIsPasteMode = true;
        if (mListView != null) {
            mListView.setChoiceMode(ListView.CHOICE_MODE_NONE);
        }

        if (mBottomButton != null) {
            mBottomButton.setVisibility(View.VISIBLE);
            Button pasteButton = (Button) mBottomButton.findViewById(R.id.bottom_button_paste);
            if (!TextUtils.isEmpty(CURRENTPATH)) {
                if (CURRENTPATH.equalsIgnoreCase(FileOperator.srcParentPath)) {
                    pasteButton.setEnabled(false);
                } else {
                    pasteButton.setEnabled(true);
                }
            } else {
                return;
            }
        }
    }

    public void hidePasteButton() {
        mIsPasteMode = false;
        if (mListView != null) {
            mListView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE_MODAL);
        }
        if (mBottomButton != null) {
            mBottomButton.setVisibility(View.GONE);
        }
    }

    private void changeBottom(String action) {
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mActivity.invalidateOptionsMenu();
            }
        }, 100);
    }

    public void setActionBarTitle(CharSequence title) {
        if (FileUtil.getMountedSDCardCount(mActivity) == 1
                && ((!TextUtils.isEmpty(CURRENTPATH)) && (CURRENTPATH.equals(PATH_SDCARD_1) || CURRENTPATH
                        .equals(FileUtil.PATH_SDCARD_2) || CURRENTPATH.equals(FileUtil.PATH_SDCARD_3)))) {
            title = getText(R.string.fm_directory_title);
        } else if (FileUtil.getMountedSDCardCount(mActivity) == 3){
            if (!TextUtils.isEmpty(CURRENTPATH) && CURRENTPATH.equals(FileUtil.PATH_SDCARD_3)) {
                title = (String) getText(R.string.fm_title_external2);
            } else if (!TextUtils.isEmpty(CURRENTPATH) && CURRENTPATH.equals(FileUtil.PATH_SDCARD_2)) {
                title = (String) getText(R.string.fm_title_external);
            } else if (!TextUtils.isEmpty(CURRENTPATH) && CURRENTPATH.equals(PATH_SDCARD_1)) {
                title = (String) getText(R.string.fm_title_internal);
            }
        } else {
            if (!TextUtils.isEmpty(CURRENTPATH) && CURRENTPATH.equals(FileUtil.PATH_SDCARD_2)) {
                title = (String) getText(R.string.fm_title_external);
            } else if (!TextUtils.isEmpty(CURRENTPATH) && CURRENTPATH.equals(PATH_SDCARD_1)) {
                title = (String) getText(R.string.fm_title_internal);
            }
        }

        mTitleView.setText(title);
    }

    private void init() {
        initMasterState();
        mActivity.setTitle(R.string.fm_title_title);
        FMThumbnailHelper.initThumbnailCathe();
        CURRENTPATH = INITSD;

        if (FileUtil.getMountedSDCardCount(mActivity) == 1) {
                if (CURRENTPATH.startsWith(FileUtil.PATH_SDCARD_2)
                        && !FileUtil.isMounted(mActivity, FileUtil.PATH_SDCARD_2)) {
                    CURRENTPATH = PATH_SDCARD_1;
                } else if (CURRENTPATH.startsWith(PATH_SDCARD_1)
                        && !FileUtil.isMounted(mActivity, PATH_SDCARD_1)) {
                    CURRENTPATH = FileUtil.PATH_SDCARD_2;
                }
        } else {
                if (CURRENTPATH.startsWith(FileUtil.PATH_SDCARD_3)
                        && !FileUtil.isMounted(mActivity, FileUtil.PATH_SDCARD_3)) {
                    CURRENTPATH = PATH_SDCARD_1;
                } else if (CURRENTPATH.startsWith(FileUtil.PATH_SDCARD_2)
                        && !FileUtil.isMounted(mActivity, FileUtil.PATH_SDCARD_2)) {
                    CURRENTPATH = FileUtil.PATH_SDCARD_3;
                } else if (CURRENTPATH.startsWith(PATH_SDCARD_1)
                        && !FileUtil.isMounted(mActivity, PATH_SDCARD_1)) {
                    CURRENTPATH = FileUtil.PATH_SDCARD_2;
                }
        }
        setActionBarTitle(CURRENTPATH);
    }

    public void refreshList() {
        setPath(CURRENTPATH);
    }

    private void setPath(String path) {
        if (TextUtils.isEmpty(CURRENTPATH) && TextUtils.isEmpty(path))
            path = PATH_SDCARD_1;

        // check if the path exist, if not exist change to the root path
        path = getCheckedPath(path);
        mIsScrolled = false;
        if (null != mFileLoadThread)
            mFileLoadThread.setCancel();

        mFileLoadThread = new UpdateViewThread(path, mActivity,
                System.currentTimeMillis());
        mFileLoadThread.start();
    }

    // if the path not exist return the root path
    private String getCheckedPath(String path) {
        File file = new File(path);
        if (!file.exists()) {
            if (!path.startsWith(PATH_SDCARD_1)) {
                return FileUtil.PATH_SDCARD_2;
            } else {
                return PATH_SDCARD_1;
            }
        } else {
            return path;
        }
    }

    private void parentViewShow() {
        File tempf = new File(CURRENTPATH);
        File file = tempf.getParentFile();
        if (file != null && file.exists()) {
            setPath(file.getAbsolutePath());
        }
    }

    /**
     * restore first visible position of the list view, force the list view
     * scroll to the file users selected before should be called when the path
     * changed or the operations finished
     */
    private void restorePosition() {
        if (mIsScrolled) {
            return;
        }
        if (mStepTraceMap.containsKey(CURRENTPATH)) {
            int savedPosition = mStepTraceMap.get(CURRENTPATH);
            if (savedPosition < mArrayList.size()) {
                mAdapterView.setSelection(savedPosition);
                mStepTraceMap.remove(CURRENTPATH);
            }
            Log.v(TAG, "restorePosisition position[" + savedPosition + "]");
        }
    }

    /**
     * save the first visible position of the list view, when click the item of
     * select the item. should be called when do operations of the file items
     */

    private void savePosition() {
        int firstPosition = mListView.getFirstVisiblePosition();
        mStepTraceMap.put(CURRENTPATH, firstPosition);
    }

    private void initViewShow() {
        if (SHOWMODE == FMAdapter.LISTVIEW) {
            mListView.setVisibility(View.VISIBLE);
            mGridView.setVisibility(View.GONE);
            updateView();
        } else if (SHOWMODE == FMAdapter.GRIDVIEW) {
            mListView.setVisibility(View.GONE);
            mGridView.setVisibility(View.VISIBLE);
            updateView();
        }
        if (isMultiTrans)
            setActionBarTitle(getString(R.string.bt_trans));
        if (FileOperator.isDeleteFiles)
            setActionBarTitle(getString(R.string.fm_op_title_delete));
    }

    private void updateView() {
        if (SHOWMODE == FMAdapter.LISTVIEW) {
            setListAdapter();
        } else {
            setGridAdapter();
        }
    }

    private void setListAdapter() {
        mAdapt = new FMAdapter(mActivity, FMAdapter.LISTVIEW, this.mArrayList,
                this.CURRENTPATH, mListView, isMultiTrans, mFolderCount,
                FMAdapter.FLAG_DIR_FRAGMENT
                , ifDisplayFileSize);
        mAdapt.setOperationListener(mOperationListener);
        mListView.setAdapter(mAdapt);
        mAdapt.loadThumbnail();

        restorePosition();
    }

    public void refreshThumbnail() {
        if (null != mAdapt) {
            mAdapt.loadThumbnail();
        }
    }

    private void setGridAdapter() {
        // mGridView.setAdapter(new FMAdapter(this, GRIDVIEW));
    }

    private void sortList(int type) {
        try {
            Collections.sort(mArrayList, new CollatorComparator(type));
        } catch (Exception ex) {
            ex.printStackTrace();
        }
        for (int i = 0; i < mArrayList.size(); i++) {
            mArrayList.get(i).id = i;
        }
        mSortMode = type;
        PreferenceManager.getDefaultSharedPreferences(mActivity).edit()
                .putInt(DIR_SORT_MODE, mSortMode).commit();
        mHandler.sendEmptyMessage(UPDATELIST);
    }

    private class FileComparator implements Comparator {
        private int type;

        public FileComparator(int type) {
            this.type = type;
        }

        @Override
        public int compare(Object lhs, Object rhs) {
            if (lhs instanceof File && rhs instanceof File) {
                File lFile = (File) lhs;
                File rFile = (File) rhs;

                String lName = lFile.getName();
                String rName = rFile.getName();

                if (!lFile.isFile() && rFile.isFile())
                    return -1;
                if (lFile.isFile() && !rFile.isFile())
                    return 1;
                switch (type) {
                    case FileUtil.SORT_BY_NAME:
                        return lName.compareToIgnoreCase(rName);
                    case FileUtil.SORT_BY_SIZE:
                        // biggest first
                        return (int) (lFile.length() - rFile.length());
                    case FileUtil.SORT_BY_TIME:
                        // latest first
                        return (int) (rFile.lastModified() - lFile.lastModified());
                    case FileUtil.SORT_BY_TYPE:
                        int temp1 = lName.lastIndexOf(".");
                        int temp2 = rName.lastIndexOf(".");

                        if (temp1 == -1 && temp2 != -1) {
                            return -1;
                        } else if (temp2 == -1 && temp1 != -1) {
                            return 1;
                        } else if (temp1 == -1 && temp2 == -1) {
                            return lName.compareTo(rName);
                        } else {
                            String sub1 = lName.substring(temp1);
                            String sub2 = rName.substring(temp2);

                            if (!sub1.equalsIgnoreCase(sub2)) {
                                return sub1.compareToIgnoreCase(sub2);
                            } else {
                                return lName.compareTo(rName);
                            }
                        }
                    default:
                        return lName.compareTo(rName);
                }
            } else {
                return 0;
            }
        }
    }

    public static class CollatorComparator implements Comparator {

        int comparetype = 0;

        long fileSize = 0;

        public CollatorComparator(int type) {
            this.comparetype = type;
        }

        public int compare(Object o1, Object o2) {
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
                case FileUtil.SORT_BY_NAME: // by name
                    int ret = s1.compareToIgnoreCase(s2);
                    return ret;
                case FileUtil.SORT_BY_SIZE: // by size
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
                case FileUtil.SORT_BY_TIME: // by last time modify
                    s1 = String.valueOf(a1.lastModifiedTime);
                    s2 = String.valueOf(a2.lastModifiedTime);
                    l1 = Long.parseLong(s1);
                    l2 = Long.parseLong(s2);
                    if (l1 > l2) {
                        return -1;
                    } else if (l1 <= l2) {
                        return 1;
                    }
                case FileUtil.SORT_BY_TYPE: // by file type
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

    private void changeListShow() {
        if (SHOWMODE == FMAdapter.GRIDVIEW) {
            SHOWMODE = FMAdapter.LISTVIEW;
            initViewShow();
        }
        if (SHOWMODE == FMAdapter.LISTVIEW) {
            if (mListView.getChoiceMode() == ListView.CHOICE_MODE_NONE) {
                mListView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
                // mDirIndexBtn.setVisibility(View.GONE);
                this.unregisterForContextMenu(mListView);
            } else if (mListView.getChoiceMode() == ListView.CHOICE_MODE_MULTIPLE) {
                mListView.setChoiceMode(ListView.CHOICE_MODE_NONE);
                this.registerForContextMenu(mListView);
                // initActionBar();
            }
            setListAdapter();
        }
    }

    private boolean setNewFloderName() {
        int count = 1;
        File f = new File(CURRENTPATH + "/"
                + this.getText(R.string.fm_options_new));
        if (!f.exists()) {
            dialogText = f.getName();
            return true;
        }
        while (true) {
            f = new File(CURRENTPATH + "/"
                    + this.getText(R.string.fm_options_new) + "(" + (count++)
                    + ")");
            if (!f.exists()) {
                dialogText = f.getName();
                break;
            }
        }
        return true;
    }

    private void doMultiShare() {
        if (mSelectedFileCnt == 1) {
            isMultiTrans = false;
            try {
                srcPath = srcPath.substring(0, srcPath.indexOf(":"));
                File f = new File(srcPath);
                String mime = null;
                mime = MediaFile.getMimeTypeForFile(f);
                Intent shareIntent = new Intent(Intent.ACTION_SEND);
                if (mime == null || mime.equals("") || mime.equals("unknown")) {
                    mime = "*/*";
                }
                shareIntent.setType(mime);
                shareIntent.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(f));
                startActivity(Intent.createChooser(shareIntent, null));
            } catch (android.content.ActivityNotFoundException e) {
                showToast(R.string.fm_share_no_way_to_share);
            }
        } else {
            isMultiTrans = false;
            String[] pathArray = new String[mSelectedFileCnt];
            ArrayList<Parcelable> pathList = new ArrayList<Parcelable>();
            for (int i = 0; i < mSelectedFileCnt; i++) {
                pathArray[i] = srcPath.substring(0, srcPath.indexOf(":"));
                srcPath = srcPath.substring(srcPath.indexOf(":") + 1,
                        srcPath.length());
                pathList.add(Uri.fromFile(new File(pathArray[i])));
            }

            Intent intent = new Intent("android.intent.action.SEND_MULTIPLE");
            intent.setType("*/*");
            intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, pathList);

            startActivity(intent);
        }
    }

    private void showProperty(String str, int type) {
        try {
            Intent result = new Intent(mActivity,
                    FileDetailInfor.class);
            result.putExtra(FileDetailInfor.PATHTAG, str);
            result.putExtra(FileDetailInfor.TYPETAG, "Folder");
            startActivity(result);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    private String getAllSelectedPath() {
        StringBuffer sb = new StringBuffer();
        srcPath = "";
        int count = 0;
        for (int i = 0; i < mListView.getCount(); i++) {
            if (mListView.isItemChecked(i)) {
                if (!mArrayList.get(i).path.equals(T2_EXTERNAL_PATH)) {
                    sb.append(CURRENTPATH).append("/")
                            .append(mArrayList.get(i).name).append(SEPARATOR);
                    count++;
                }
            }
        }
        srcPath = sb.toString();
        if (srcPath.length() > 0 && !FileOperator.isDeleteFiles) {
            FileOperator.isCanPaste = true;
        }
        FileOperator.copyPath = srcPath;
        Log.v(TAG, "file's count = " + count + " | copyPath = "
                + FileOperator.copyPath);
        mSelectedFileCnt = count;
        return sb.toString();
    }

    private void selectAll(boolean flag) {
        int start = isMultiTrans ? mFolderCount : 0;
        for (int i = start; i < mListView.getCount(); i++) {
            mListView.setItemChecked(i, flag);
        }
    }

    private void showToast(String str) {
        if (null == toast) {
            toast = Toast.makeText(mActivity, "", Toast.LENGTH_SHORT);
            toast.setGravity(Gravity.BOTTOM, 0, 0);
        }
        toast.setText(str);
        toast.setDuration(Toast.LENGTH_SHORT);
        toast.show();
    }

    private void showToast(int resid) {
        if (null == toast) {
            toast = Toast.makeText(mActivity, "", Toast.LENGTH_SHORT);
            toast.setGravity(Gravity.BOTTOM, 0, 0);
        }
        String str = this.getText(resid).toString();
        toast.setText(str);
        toast.setDuration(Toast.LENGTH_SHORT);
        toast.show();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        switch (item.getItemId()) {
            case R.id.action_new: // mknew dir
                setNewFloderName();
                showInputDialog(OP_NEWFLODER, R.string.fm_options_new, dialogText);
                break;
            case R.id.action_property: // property
                showProperty(CURRENTPATH, 1);
                break;
            case R.id.action_order_by_name: // sort by name
                sortList(FileUtil.SORT_BY_NAME);
                break;
            case R.id.action_order_by_size: // sort by size
                sortList(FileUtil.SORT_BY_SIZE);
                break;
            case R.id.action_order_by_time: // sort by date
                sortList(FileUtil.SORT_BY_TIME);
                break;
            case R.id.action_order_by_type: // sort by type
                sortList(FileUtil.SORT_BY_TYPE);
                break;
            case R.id.action_fav:
                FileOperator.addFavorite(CURRENTPATH, FileUtil.CATEGORY_UNKNOWN, mActivity);
                Toast.makeText(mActivity, R.string.add_fav_success, Toast.LENGTH_SHORT).show();
        }
        return true;
    }

    private void onMultiConfirmed() {
        new Thread() {
            public void run() {
                getAllSelectedPath();
                mHandler.sendEmptyMessage(GOT_ALL_SELECTED);
            }
        }.start();
    }

    @Override
    public void onResume() {
        super.onResume();

        if (OLDPATH != null) {
            CURRENTPATH = OLDPATH;
        }

        Intent intent = mActivity.getIntent();
        if (!mLaunchedWithPath) {
            if (!TextUtils.isEmpty(intent.getStringExtra(FileMainActivity.EXTRA_FILE_PATH))) {
                Log.v(TAG,
                        "has path    path = "
                                + intent.getStringExtra(FileMainActivity.EXTRA_FILE_PATH));
                mLaunchedPath = intent.getStringExtra(FileMainActivity.EXTRA_FILE_PATH);
                CURRENTPATH = mLaunchedPath;
                if (!new File(CURRENTPATH).exists()) {
                    showToast(R.string.create_shortcut_not_exist);
                    mActivity.finish();
                }
            }

            mLaunchedWithPath = true;
        }

        ifDisplayFileSize = PreferenceManager.getDefaultSharedPreferences(mActivity)
                .getBoolean("displayfilesize", true);

        setListAdapter();

        boolean ifShowHiddenFile = PreferenceManager.getDefaultSharedPreferences(mActivity)
                .getBoolean("showHiddenfile", false);

        if (TextUtils.isEmpty(OLDPATH) || !OLDPATH.equals(CURRENTPATH)
                || new File(OLDPATH).lastModified() != mLastModified || ifShowHiddenFile != mFlag) {
            // path changed or content of the path changed, call setPath to
            // refresh the file list
            setPath(CURRENTPATH);
            mFlag = ifShowHiddenFile;
        }

    }

    @Override
    public void onPause() {
        super.onPause();
        OLDPATH = CURRENTPATH;

        File file = new File(OLDPATH);
        mLastModified = file.lastModified();
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    public void reset() {
        CURRENTSD = INITSD;
        CURRENTPHONE = INITPHONE;

        OLDPATH = null;
    }

    @Override
    public void onDestroy() {
        if (inited) {
            CURRENTSD = INITSD;
            reset();
            SHOWMODE = FMAdapter.LISTVIEW;
        }

        if (null != mHandler) {
            mHandler.removeMessages(LOADBACKGROUND);
            mHandler.removeMessages(UPDATELIST);
            mHandler.removeMessages(GOT_ALL_SELECTED);
        }

        if (null != mFileLoadThread) {
            mFileLoadThread.setCancel();
        }

        if (null != mCurOp)
            mCurOp.setExit();
        if (null != mAdapt)
            mAdapt.setExitOperation();

        super.onDestroy();
    }

    public boolean handleKeyDown(int keyCode, KeyEvent event) {
        Log.d(TAG, "handle key event in FileListFragment, keyCode is: "
                + keyCode + ", event is: " + event);
        switch (event.getKeyCode()) {
            case KeyEvent.KEYCODE_BACK:
                if (mAdapt != null && ((FMAdapter) mAdapt).isOperationShow()) {
                    ((FMAdapter) mAdapt).hideOperation();
                    return true;
                }
                if ((SHOWMODE == FMAdapter.LISTVIEW)
                        && (null != mListView)
                        && (mListView.getChoiceMode() == ListView.CHOICE_MODE_MULTIPLE)) {
                    isMultiTrans = false;
                    FileOperator.isDeleteFiles = false;
                    if (OLDSHOWMODE == FMAdapter.GRIDVIEW) {
                        SHOWMODE = FMAdapter.GRIDVIEW;
                        initViewShow();
                    } else {
                        changeListShow();
                    }
                    mListView.clearChoices();
                    setActionBarTitle(CURRENTPATH);
                    mActivity.invalidateOptionsMenu();

                    return true;
                } else if (CURRENTPATH.equals(INITSD)
                        || CURRENTPATH.equals(INITPHONE)
                        || CURRENTPATH.equals(FileUtil.PATH_SDCARD_2)) {
                    Log.v(TAG, "finish this list activity reinit sd and phone path");
                    CURRENTSD = INITSD;
                    CURRENTPHONE = INITPHONE;
                    return false;
                } else {
                    if (mListView != null) {
                        mListView.setAdapter(null);
                    }
                    if (mGridView != null) {
                        mGridView.setAdapter(null);
                    }
                    if (null == mTitleView) {
                        return false;
                    }
                    parentViewShow();
                    this.setActionBarTitle(CURRENTPATH);
                }
                return true;
            case KeyEvent.KEYCODE_MENU:
                if (mOptionPrepared && (null != mActivity)) {
                    mActivity.invalidateOptionsMenu();
                    mOptionPrepared = false;
                }

                if ((null != mAdapt) && mAdapt.isOperationShow()) {
                    mAdapt.hideOperation();
                }

                if (mMode != null) {
                    mMode.finish();
                }
                break;
        }
        return false;
    }

    private OperationView.OperationListener mOperationListener = new OperationView.OperationListener() {

        @Override
        public void onOperationEnd() {
            try {
                mCurOp = null;
                dialogDismiss = true;
                refreshList();
                hidePasteButton();
            } catch (NullPointerException e) {
                e.printStackTrace();
                return;
            }
        }

        @Override
        public void onOperationClicked(int opType, int position) {
            savePosition();
        }
    };

    private void showInputDialog(int type, int title, String originalName) {
        try {
            Log.v(TAG, "enter setDialog, originalName:" + originalName);
            if (null == originalName) {
                return;
            }

            AlertDialog inputDialog = new AlertDialog.Builder(mActivity).setIcon(
                    android.R.drawable.ic_dialog_alert).create();

            initInputDialog(inputDialog, type, title, originalName);
            inputDialog.show();
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    private void initInputDialog(AlertDialog ad, int type, int title, String originalName) {
        ad.getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);

        LayoutInflater factory = LayoutInflater
                .from(mActivity);
        mEditText = (EditText) factory
                .inflate(R.layout.edit_text, null);

        // setting dialog's edit text
        InputFilter[] filters = new InputFilter[1];
        filters[0] = new EditTextLengthFilter(MAX_INPUT);
        ((EditTextLengthFilter) filters[0])
                .setTriger(new EditTextLengthFilter.Triger() {
                    public void onGetMax() {
                        showToast(R.string.max_input_reached);
                    }
                });
        mEditText.setFilters(filters);
        mEditText.setSingleLine(true);
        setEditText(originalName, type);
        ad.setView(mEditText);

        // set dialog title
        dialogText = originalName;
        ad.setTitle(title);
        operatetype = type;

        // setting dialog's message
        String message;
        if (OP_NEWFLODER == type) {
            message = this.getResources().getString(R.string.enter_foldername);
        } else {
            message = this.getResources().getString(R.string.enter_filename);
        }
        ad.setMessage(message);

        final AlertDialog textInputDialog = ad;
        textInputDialog.setCanceledOnTouchOutside(false);
        textInputDialog.setButton(this.getText(R.string.dialog_ok),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface arg0, int arg1) {
                        Log.v(TAG, "textinPutdialog.onclick,   dialogtext="
                                + dialogText);
                        pressOK(operatetype, dialogText, textInputDialog);
                    }
                });
        textInputDialog.setButton2(this.getText(R.string.dialog_cancel),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface arg0, int arg1) {
                        allowdismiss = true;
                    }
                });
        textInputDialog.setOnDismissListener(new OnDismissListener() {
            public void onDismiss(DialogInterface arg0) {
                if (!allowdismiss) {
                    textInputDialog.show();
                }
            }
        });
        textInputDialog.setOnKeyListener(new OnKeyListener() {
            public boolean onKey(DialogInterface arg0, int arg1, KeyEvent event) {
                Log.v(TAG, "DIALOG PRESS BACK KEY");
                switch (event.getKeyCode()) {
                    case KeyEvent.KEYCODE_BACK:
                        if (KeyEvent.ACTION_DOWN == event.getAction()) {
                            allowdismiss = true;
                            textInputDialog.dismiss();
                            return true;
                        }
                }
                return false;
            }
        });
    }

    private void setEditText(String name, int type) {
        mEditText.setText(name);
        int index = name.lastIndexOf(".");
        if (index == -1) {
            mEditText.setSelection(0, name.length());
        } else {
            mEditText.setSelection(0, index);
        }
    }

    private void pressOK(int type, final String originalName, AlertDialog inputDlg) {
        final String name = mEditText.getText().toString().trim();
        String tmpName = name;

        if (type == OP_ZIP) {
            tmpName = name + ".zip";
        }
        int ret = FileUtil.checkFileName(tmpName, CURRENTPATH);

        if (0 != ret) {
            setEditText(originalName, type);
            String msg = handleInputInvalid(ret, type);
            inputDlg.setMessage(msg);
            allowdismiss = false;
            return;
        }

        switch (type) {
            case OP_RENAMEFILE:
            case OP_RENAMEFOLDER:
                File tempfile = new File(srcPath);
                tempfile.renameTo(new File(CURRENTPATH + "/" + name));
                new Thread() {
                    @Override
                    public void run() {
                        // TODO Auto-generated method stub
                        FileUtil.updateFileInfo(mActivity, name, originalName);
                    }
                }.start();

                showToast(R.string.fm_operate_over);
                allowdismiss = true;
                setPath(CURRENTPATH);
                break;
            case OP_NEWFLODER:
                File temp = new File(CURRENTPATH + "/" + name);
                if (FileUtil.getFreeSpace(mActivity, CURRENTPATH) != 0) {
                    temp.mkdirs();
                } else {
                    Toast.makeText(mActivity, R.string.op_no_enough_space,
                            Toast.LENGTH_SHORT).show();
                }
                FileUtil.changePermission(temp);

                showToast(R.string.fm_operate_over);
                allowdismiss = true;
                setPath(CURRENTPATH);
                break;

            case OP_ZIP:
                allowdismiss = true;
                if (null != mCurOp)
                    mCurOp.setExit();
                mCurOp = new OperationZip(mActivity, CURRENTPATH, srcPath,
                        tmpName);
                mCurOp.setOnOperationEndListener(mOperationListener);
                mCurOp.start();
                break;
        }
    }

    private String handleInputInvalid(int err, int type) {
        int id_toast = 0;
        int id_msg = 0;
        switch (err) {
            case Defs.ERR_EMPTY:
                id_msg = R.string.fm_input_name_first;
                break;
            case Defs.ERR_INVALID_CHAR:
                if (FOLDER_TYPE == type) {
                    id_msg = R.string.text_contain_unacceptable_char_folder;
                } else {
                    id_msg = R.string.text_contain_unacceptable_char_file;
                }
                break;
            case Defs.ERR_INVALID_INIT_CHAR:
                if (FOLDER_TYPE == type) {
                    id_msg = R.string.fm_name_invalid_head_folder;
                } else {
                    id_msg = R.string.fm_name_invalid_head_file;
                }
                break;
            case Defs.ERR_TOO_LONG:
                if (FOLDER_TYPE == type) {
                    id_msg = R.string.op_filename_too_long_folder;
                } else {
                    id_msg = R.string.op_filename_too_long_file;
                }
                break;
            case Defs.ERR_FOLDER_ALREADY_EXIST:
                id_msg = R.string.fm_paste_folder_already_exist;
                break;
            case Defs.ERR_FILE_ALREADY_EXIST:
                id_msg = R.string.fm_paste_file_already_exist;
                break;
            default:
                if (FOLDER_TYPE == type) {
                    id_msg = R.string.fm_name_invalid_name_folder;
                } else {
                    id_msg = R.string.fm_name_invalid_name_file;
                }
                break;
        }

        return this.getResources().getString(id_msg);
    }

    private class UpdateViewThread extends Thread {
        private final static int NUM_FILE = 40;
        private boolean mIsFinished = false;
        private boolean mWait = false;
        private boolean mIsFirstUpdateUI = true;
        private long mTimeStamp = 0;
        private String mPath;
        private Context mContext;

        private ArrayList<FileNode> mFileList = new ArrayList<FileNode>();
        private ArrayList<FileNode> mExportList;

        UpdateViewThread(String path, Context ctx, long time) {
            mPath = path;
            mContext = ctx;
            mTimeStamp = time;
        }

        public void setCancel() {
            mIsFinished = true;
        }

        public void setContinue() {
            mWait = false;
        }

        public boolean isFinished() {
            return mIsFinished;
        }

        public ArrayList<FileNode> getLoadFiles() {
            return mExportList;
        }

        public long getTimeStamp() {
            return mTimeStamp;
        }

        public String getPath() {
            return mPath;
        }

        public boolean isFirstUpdateUI() {
            return mIsFirstUpdateUI;
        }

        public void run() {
            mFileList.clear();
            if ((mIsFinished) || (null == mPath))
                return;

            mIsLoading = true;
            File[] files = FileOperator.getSubFileArray(mPath, mActivity);

            if (null == files)
                return;
            mFilesLen = files.length;

            List<File> sortList = Arrays.asList(files);
            sortDownloadList(sortList, mSortMode);

            for (int i = 0; (i < files.length) && (!mIsFinished); i++) {
                getFileItemInfo(sortList.get(i));
                if ((i % NUM_FILE) == (NUM_FILE - 1)) {
                    mWait = true;

                    notifyLoadResults();
                    while ((mWait) && (!mIsFinished)) {
                        sleep(500);
                    }
                }
                if (i >= NUM_FILE - 1)
                    mIsFirstUpdateUI = false;

                sleep(10);
            }

            if (!mIsFinished) {
                notifyLoadResults();
                mIsFinished = true;
            }
            mIsLoading = false;
        }

        private void sleep(int time) {
            try {
                Thread.sleep(time);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        private void sortDownloadList(List<File> sortList, int type) {
            try {
                Collections.sort(sortList, new FileComparator(type));
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }

        public void notifyLoadResults() {
            if (mExportList != null) {
                ArrayList<FileNode> tmp = (ArrayList<FileNode>) mExportList.clone();
                tmp.addAll(mFileList);
                mExportList = tmp;
            } else {
                mExportList = mFileList;
            }

            Message msg = mHandler.obtainMessage(LOADBACKGROUND, mTimeStamp);
            mHandler.sendMessage(msg);

            mFileList = new ArrayList<FileNode>();
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
                    mimeType = MediaFile.getRealMimeType(mContext,
                            file.getAbsolutePath());
                } else if (fileName.endsWith(".dcf") || fileName.endsWith("dm")) {
                    mimeType = MediaFile.getRealMimeType(mContext,
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
                mFolderCount++;
            }

            temp.id = mFileList.size();
            temp.iconRes = fileIcon;
            temp.path = filePath;
            temp.name = fileName;
            temp.isFile = file.isFile();
            temp.lastModifiedTime = file.lastModified();
            temp.lengthInByte = file.length();
            temp.type = mimeType;
            temp.hasThumbnail = hasThumbnail;
            mFileList.add(temp);
            temp = null;
        }
    };

    private Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            if (null == getActivity())
                return;

            switch (msg.what) {
                case UPDATELIST:
                    initViewShow();
                    if (moveToParentFolder == true || dialogDismiss == true) {
                        restorePosition();
                    }
                    break;
                case LOADBACKGROUND:
                    long curThTimeStamp = mFileLoadThread.getTimeStamp();
                    long finThTimeStamp = (Long) msg.obj;

                    if (curThTimeStamp == finThTimeStamp) {
                        ArrayList<FileNode> list = mFileLoadThread.getLoadFiles();
                        CURRENTPATH = mFileLoadThread.getPath();

                        if (list.size() == 0) {
                            mListView.setAdapter(null);
                            mArrayList = list;
                            mNoFileView.setVisibility(View.VISIBLE);
                        } else {
                            mNoFileView.setVisibility(View.GONE);
                            mArrayList = list;

                            if (mFileLoadThread.isFirstUpdateUI()) {
                                initViewShow();
                            } else {
                                mAdapt.setDataList(mArrayList);
                                mAdapt.notifyDataSetChanged();
                            }

                            if (moveToParentFolder == true || dialogDismiss == true) {
                                restorePosition();
                            }
                        }

                        setActionBarTitle(CURRENTPATH);

                        if (!mFileLoadThread.isFinished())
                            mFileLoadThread.setContinue();

                        if (mIsPasteMode) {
                            showPasteButton();
                        } else {
                            hidePasteButton();
                        }
                        mActivity.invalidateOptionsMenu();
                    }
                    break;

                case GOT_ALL_SELECTED:
                    if (srcPath.length() == 0) { // must select file first
                        FileOperator.isCanPaste = false;
                        showToast(R.string.fm_op_select_empty);
                        break;
                    } else if (isMultiTrans) {
                        doMultiShare();
                    }
                    setActionBarTitle(CURRENTPATH);
                    mOptionPrepared = true;

                    mListView.clearChoices();
                    if (mMode != null) {
                        mMode.finish();
                    }

                    if (mOpType == FileUtil.OPERATION_TYPE_COPY
                            || mOpType == FileUtil.OPERATION_TYPE_CUT) {
                        FileOperator.sendPasteIntent(mActivity);
                    }

                    if (FileOperator.isDeleteFiles) { // delete file or
                                                      // something
                                                      // else
                        FileOperator.isDeleteFiles = false;
                        FileOperator.isCanPaste = false;

                        if (null != mCurOp)
                            mCurOp.setExit();
                        mCurOp = new OperationDelete(mActivity,
                                OperationDelete.OP_MULT_DELETE_CONFIRM, srcPath, CURRENTPATH);
                        mCurOp.setOnOperationEndListener(mOperationListener);
                        mCurOp.start();
                    }
                    break;
            }
        }

    };

    private OnItemClickListener itemclick = new OnItemClickListener() {

        public void onItemClick(AdapterView<?> arg0, View arg1, int position,
                long arg3) {
            synchronized (FileDirFragment.this) {
                if (click_event_processing)
                    return;
                click_event_processing = true;
            }
            String name = (String) mArrayList.get(position).name;
            String path = null;
            savePosition();
            path = CURRENTPATH + "/" + name;
            if (mInActionMode) {
                if (isMultiTrans && (position < mFolderCount)) {
                    mListView.setItemChecked(position, false);
                    showToast(R.string.bt_select_invalid);
                    click_event_processing = false;
                    return;
                } else {
                    mListView.setItemChecked(position,
                            mListView.isItemChecked(position));
                }
                mActivity.invalidateOptionsMenu();
                click_event_processing = false;
                return;
            }

            File file = new File(path);
            if (file.isDirectory()) {

                setPath(path);
            } else if ((path.toLowerCase().endsWith(".zip") && file.isFile()))
            { // unzip
                srcPath = path;
                openCompressFile(mActivity, path);
            } else {

                if (!mInActionMode) {
                    Uri fileUri = Uri.fromFile(new File(path));
                    String mimetype = null;
                    try {
                        mimetype = mArrayList.get(position).type;
                        if (mimetype.length() <= 0) {
                            mimetype = MediaFile.getMimeType(path);
                        }
                    } catch (Exception ex) {
                        ex.printStackTrace();
                    }

                    if (mimetype == null) {
                        showToast(R.string.fm_cannot_open_file);
                        click_event_processing = false;
                        return;
                    }
                    Intent commIntent = new Intent();
                    commIntent.setAction(Intent.ACTION_VIEW);
                    commIntent.setDataAndType(fileUri, mimetype);
                    commIntent.putExtra("orderBy", mSortMode);
                    PackageManager mPackageManager = mActivity.getPackageManager();

                    List<ResolveInfo> mResList = mPackageManager
                            .queryIntentActivities(commIntent,
                                    PackageManager.MATCH_DEFAULT_ONLY);
                    if ((null == mResList) || (mResList.isEmpty())) {
                        showToast(R.string.fm_op_open_fail);
                        click_event_processing = false;
                        return;
                    }
                    startActivity(commIntent);
                    Log.v(TAG, "send intent ...  URI = " + fileUri + " mime = "
                            + mimetype);

                }
            }
            click_event_processing = false;
        }
    };

    private void openCompressFile(final Context context, final String path) {
        Uri fileUri = Uri.fromFile(new File(path));
        String mimetype = MediaFile.getMimeType(path);

        Intent fileIntent = new Intent();
        fileIntent.setAction(Intent.ACTION_VIEW);
        fileIntent.setDataAndType(fileUri, mimetype);
        List<ResolveInfo> mResList = context.getPackageManager()
                .queryIntentActivities(fileIntent,
                        PackageManager.MATCH_DEFAULT_ONLY);
        if ((null == mResList) || (mResList.isEmpty())) {
            Log.d(TAG, "No apps found to open zip file, zip the file by FileManager");
            AlertDialog dialog = new AlertDialog.Builder(context)
                    .setTitle(R.string.dialog_title_unzip)
                    .setMessage(R.string.dialog_msg_unzip)
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            startDecompressOperation(path, context);
                        }
                    }).setNegativeButton(android.R.string.cancel, null).create();
            dialog.show();
        } else {
            startActivity(fileIntent);
        }
    }

    private void startDecompressOperation(String path, Context context) {
        if (null != mCurOp)
            mCurOp.setExit();
        mCurOp = new OperationUnzip(mActivity, CURRENTPATH, path);
        mCurOp.setOnOperationEndListener(mOperationListener);
        mCurOp.start();
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

    private class ModeCallback implements ListView.MultiChoiceModeListener {
        private View mMultiSelectActionBarView;
        private ImageView mMultiSelect;
        private Menu mMenu;
        private int mSelectedFolderCount = 0;

        @Override
        public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
            switch (item.getItemId()) {
                case R.id.modal_action_copy:
                    OLDSHOWMODE = SHOWMODE;
                    mOpType = FileUtil.OPERATION_TYPE_COPY;
                    setActionBarTitle(getString(R.string.fm_options_copy));
                    FileOperator.isDeleteFiles = false;
                    FileOperator.srcParentPath = CURRENTPATH;
                    FileOperator.isCut = false;
                    FileOperator.isCanPaste = false;
                    onMultiConfirmed();
                    break;
                case R.id.modal_action_cut:
                    OLDSHOWMODE = SHOWMODE;
                    mOpType = FileUtil.OPERATION_TYPE_CUT;
                    setActionBarTitle(getString(R.string.fm_options_cut));
                    FileOperator.isDeleteFiles = false;
                    FileOperator.srcParentPath = CURRENTPATH;
                    FileOperator.isCut = true;
                    onMultiConfirmed();
                    break;
                case R.id.modal_action_delete:
                    OLDSHOWMODE = SHOWMODE;
                    mOpType = FileUtil.OPERATION_TYPE_DELETE;
                    setActionBarTitle(getString(R.string.fm_op_title_delete));
                    FileOperator.isDeleteFiles = true;
                    onMultiConfirmed();
                    break;
                case R.id.modal_action_multi_share:
                    OLDSHOWMODE = SHOWMODE;
                    mOpType = FileUtil.OPERATION_TYPE_SHARE;
                    setActionBarTitle(getString(R.string.fm_options_multi_share));
                    FileOperator.srcParentPath = CURRENTPATH;
                    isMultiTrans = true;
                    onMultiConfirmed();
                    break;
                case R.id.modal_action_fav:
                    mOpType = FileUtil.OPERATION_TYPE_FAV_ADD;
                    String paths = getAllSelectedPath();
                    for (String path : paths.split(File.pathSeparator)) {
                        FileOperator.addFavorite(path, FileUtil.getCategoryByPath(path), mActivity);
                    }
                    mode.finish();
                    Toast.makeText(mActivity, R.string.add_fav_success, Toast.LENGTH_SHORT).show();
                    break;
                default:
                    break;
            }
            return true;
        }

        @Override
        public boolean onCreateActionMode(ActionMode mode, Menu menu) {
            if ((null != mAdapt) && ((FMAdapter) mAdapt).isOperationShow()) {
                ((FMAdapter) mAdapt).hideOperation();
            }

            mMode = mode;
            mInActionMode = true;
            MenuInflater inflater = mActivity.getMenuInflater();
            inflater.inflate(R.menu.menu, menu);
            mMenu = menu;
            if (mMultiSelectActionBarView == null) {
                mMultiSelectActionBarView = (ViewGroup) LayoutInflater.from(mActivity).inflate(
                        R.layout.actionbar_multi_mode, null);

                mMultiSelect = (ImageView) mMultiSelectActionBarView
                        .findViewById(R.id.multi_select);
            }
            mode.setCustomView(mMultiSelectActionBarView);
            ((TextView) mMultiSelectActionBarView.findViewById(R.id.title))
                    .setText(R.string.fm_multi_select);
            ((FileMainActivity) getActivity()).setPageScrollable(false);
            return true;
        }

        @Override
        public void onDestroyActionMode(ActionMode mode) {
            mInActionMode = false;
            mMode = null;
            mSelectedFolderCount = 0;
            ((FileMainActivity) getActivity()).setPageScrollable(true);
        }

        @Override
        public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
            if (mMultiSelectActionBarView == null) {
                ViewGroup v = (ViewGroup) LayoutInflater.from(mActivity).inflate(
                        R.layout.actionbar_multi_mode, null);
                mode.setCustomView(v);

                mMultiSelect = (ImageView) v.findViewById(R.id.multi_select);
            }

            mMultiSelect.setOnClickListener(mSelectListener);

            return true;
        }

        @Override
        public void onItemCheckedStateChanged(ActionMode mode, int position,
                long id, boolean checked) {
            if (mListView.getCheckedItemCount() == mListView.getCount()) {
                mAllSelected = true;
                mMultiSelect
                        .setImageResource(R.drawable.action_icon_all_reverse_choose_nor);
            } else {
                mAllSelected = false;
                mMultiSelect
                        .setImageResource(R.drawable.action_icon_choice_of_nor);
            }

            savePosition();
            String path = mArrayList.get(position).path;

            // if user long click sdcard_ext only, operate below code to exit
            // multi mode
            if (path.equals(T2_EXTERNAL_PATH)
                    && mListView.getCheckedItemCount() == 1) {
                showToast(R.string.fm_sdcard_ext_no_operation);
                mode.finish();
            }

            File file = new File(path);
            if (file.isDirectory()) {
                if (checked) {
                    mSelectedFolderCount++;
                } else {
                    mSelectedFolderCount--;
                }
            }

            if (mSelectedFolderCount == 0) {
                mMenu.findItem(R.id.modal_action_multi_share).setVisible(true);
            } else if (mSelectedFolderCount > 0) {
                mMenu.findItem(R.id.modal_action_multi_share).setVisible(false);
            }
        }
    }

    private View.OnClickListener mSelectListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (!mAllSelected) {
                mAllSelected = true;
                selectAll(true);
            } else {
                mAllSelected = false;
                selectAll(false);
            }
        }
    };

}
