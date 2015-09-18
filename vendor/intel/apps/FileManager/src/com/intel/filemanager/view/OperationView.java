package com.intel.filemanager.view;

import java.io.File;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.text.InputFilter;
import android.text.TextUtils;
import android.view.DragEvent;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.filemanager.CustomViewPager;
import com.intel.filemanager.FileMainActivity;
import com.intel.filemanager.FileOperator;
import com.intel.filemanager.Operation;
import com.intel.filemanager.Operation.OnOperationEndListener;
import com.intel.filemanager.FileDetailInfor;
import com.intel.filemanager.OperationDelete;
import com.intel.filemanager.OperationUnzip;
import com.intel.filemanager.OperationZip;
import com.intel.filemanager.R;
import com.intel.filemanager.util.EditTextLengthFilter;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.FileUtil.Defs;
import com.intel.filemanager.util.LogHelper;

public class OperationView extends HorizontalScrollView {
    private static final String TAG = "OperationView";
    private static LogHelper Log = LogHelper.getLogger();

    private final static int MAX_INPUT = 80;
    private final static int MSG_OPERATIONVIEW_MOVE = 1000;
    public static final int TYPE_CATEGORY_LIST = 1;
    public static final int TYPE_DIRECTORY_LIST = 2;
    private static final int RENAME_FILE = 3;
    private static final int RENAME_FOLDER = 4;

    OperationButton bShare;
    OperationButton bCopy;
    OperationButton bCut;
    OperationButton bOpenPath;
    OperationButton bDelete;
    OperationButton bRingtone;
    OperationButton bWallpaper;
    OperationButton bProperty;
    OperationButton bAddToFav;
    OperationButton bDelFromFav;
    OperationButton bZip;
    OperationButton bUnzip;
    OperationButton bRename;
    OperationButton bMore;
    LinearLayout bRootLine;

    private int mType;
    private boolean mIsCollapsed;
    private int mCategory;
    private String mFilePath;
    private String mCurPath;
    private int mPosition;
    private Context mContext;
    private OperationListener mOperationListener;
    private Operation mCurOp = null;

    private boolean allowdismiss = false;
    private EditText mEditText;

    public OperationView(Context context, int category, String path,
            String curPath) {
        super(context);
        mContext = context;
        mCurPath = curPath;
        if (TextUtils.isEmpty(curPath)) {
            mType = TYPE_CATEGORY_LIST;
        } else {
            mType = TYPE_DIRECTORY_LIST;
        }
        init();
        update(category, path, 0);
    }

    private int getMinButtonWidth() {
        int width;
        int buttonCount = 5;
        
        if (bRootLine.getChildCount() < 1) {
            switch (mType) {
                case TYPE_DIRECTORY_LIST:
                    buttonCount = 5;
                    break;
                case TYPE_CATEGORY_LIST:
                    if (mCategory == FileUtil.CATEGORY_FAV) {
                        buttonCount = 1;
                    } else if (mCategory == FileUtil.CATEGORY_APP) {
                        buttonCount = 2;
                    } else {
                        buttonCount = 5;
                    }
                    break;
            }
        } else {
            buttonCount = bRootLine.getChildCount() - 1 < 5 ? bRootLine.getChildCount() - 1 : 5;
        }

        if (FileUtil.SCREEN_WIDTH > FileUtil.SCREEN_HEIGHT) {
            if (buttonCount < 5) {
                width = FileUtil.SCREEN_WIDTH / buttonCount;
            } else {
                int count = FileUtil.SCREEN_WIDTH / (FileUtil.SCREEN_HEIGHT / buttonCount);
                count = bRootLine.getChildCount() - 1 < count ? bRootLine
                        .getChildCount() - 1 : count;
                width = FileUtil.SCREEN_WIDTH / count;
                
            }
        } else {
            width = FileUtil.SCREEN_WIDTH / buttonCount;
        }
        return width;
    }

    private int getMaxButtonCount() {
        int buttonNum = FileUtil.SCREEN_WIDTH / getMinButtonWidth();
        return buttonNum;
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();

        // start an animation when this view attache to window
        // Animation animation = new TranslateAnimation(600, 0, 0, 0);
        // animation.setDuration(200);
        // startAnimation(animation);
    }

    public void update(int category, String path, int position) {
        File file = new File(path);
        mCurPath = file.getParent();
        mCategory = category;
        mFilePath = path;
        mPosition = position;

        bRootLine.removeAllViews();

        LinearLayout.LayoutParams lineParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);

        String Oper = getOperSortByCategory(category, path);
        String tempStr;
        OperationButton tempView;
        
        boolean  ifDisplayFileSize = false;
        if(FileMainActivity.isFirstLaunch == true){
                ifDisplayFileSize = PreferenceManager.getDefaultSharedPreferences(mContext)
                    .getBoolean("displayfilesize", true);
        }
        
        for (int i = 0; i < Oper.length(); i = i + 2) {
            tempStr = Oper.substring(i, i + 2);
            tempView = getView(Integer.valueOf(tempStr));
            if (tempView != null) {
                if (mCategory == FileUtil.CATEGORY_APP) {
                    if (tempView == bDelete) {
                        tempView.setText(R.string.operation_uninstall);
                    } else if (tempView == bProperty) {
                        tempView.setText(R.string.operation_app_info);
                    }
                }
                bRootLine.addView(tempView, lineParams);
                if(mType == TYPE_DIRECTORY_LIST){
                    bRootLine.removeView(bOpenPath);
                }
                if(file.isFile()){
                   bRootLine.removeView(bProperty);
                }
            }
        }

        bRootLine.addView(bMore, lineParams);
        lineParams.width = getMinButtonWidth();
        
        invalidate();
        collapseOperation();
    }

    private OperationButton getView(int operType) {
        OperationButton view;
        switch (operType) {
        case FileUtil.OPERATION_TYPE_SHARE:
            view = bShare;
            break;
        case FileUtil.OPERATION_TYPE_COPY:
            view = bCopy;
            break;
        case FileUtil.OPERATION_TYPE_CUT:
            view = bCut;
            break;
        case FileUtil.OPERATION_TYPE_ZIP:
            view = bZip;
            break;
        case FileUtil.OPERATION_TYPE_UNZIP:
            view = bUnzip;
            break;
        case FileUtil.OPERATION_TYPE_DELETE:
            view = bDelete;
            break;
        case FileUtil.OPERATION_TYPE_RENAME:
            view = bRename;
            break;
        case FileUtil.OPERATION_TYPE_SET_RINGTONE:
            view = bRingtone;
            break;
        case FileUtil.OPERATION_TYPE_SET_WALLPAPER:
            view = bWallpaper;
            break;
        case FileUtil.OPERATION_TYPE_SHOW_PROPERTY:
            view = bProperty;
            break;
        case FileUtil.OPERATION_TYPE_FAV_ADD:
            view = bAddToFav;
            break;
        case FileUtil.OPERATION_TYPE_FAV_REMOVE:
            view = bDelFromFav;
            break;
        case FileUtil.OPERATION_TYPE_OPEN_PATH:
            view = bOpenPath;
            break;
        case FileUtil.OPERATION_TYPE_MORE:
            view = bMore;
            break;
        default:
            view = null;
            break;
        }

        return view;
    }

    private String getOperType(OperationButton view) {
        String type = "";
        int tag = (Integer) view.getTag();
        if (tag < 10) {
            type = "0" + String.valueOf(tag);
        } else {
            type = String.valueOf(tag);
        }

        return type;
    }

    private String getOperSortByCategory(int category, String path) {
        String OperSort;
        // bShare 01 bCopy 02 bCut 03 bZip 05 bUnzip 06 bDelete 07 bRename 08 bRingtone 09
        // bWallpaper 10 bProperty 11 bAddToFav 12 bDelFromFav 13 bOpenPath 14
        boolean isFav = FileOperator.isFavorite(path, mContext);

        switch (category) {
        case FileUtil.CATEGORY_IMAGE:
            OperSort = isFav ? "011413110807020305" : "011412110807020305";
            break;
        case FileUtil.CATEGORY_VIDEO:
            OperSort = isFav ? "011413110807020305" : "011412110807020305";
            break;
        case FileUtil.CATEGORY_AUDIO:
            OperSort = isFav ? "09011413110807020305" : "09011412110807020305";
            break;
        case FileUtil.CATEGORY_DOC:
            OperSort = isFav ? "011413110807020305" : "011412110807020305";
            break;
        case FileUtil.CATEGORY_APP:
            OperSort = "1107";
            break;
        case FileUtil.CATEGORY_ARC:
            OperSort = isFav ? "060114131108070203" : "060114121108070203";
            break;
        case FileUtil.CATEGORY_FAV:
            OperSort = "13";
            break;
        case FileUtil.CATEGORY_BOOK:
            OperSort = isFav ? "011413110807020305" : "011412110807020305";
            break;
        case FileUtil.CATEGORY_APK:
            OperSort = isFav ? "0114131108070203" : "0114121108070203";
            break;
        default:
            OperSort = isFav ? "13110807020305" : "12110807020305";
            break;
        }

        return OperSort;
    }
    public int getCateory() {
        return mCategory;
    }

    public int getPosition() {
        return mPosition;
    }

    public void setOperationListener(OperationListener listener) {
        mOperationListener = listener;
    }

    public OperationListener getOperationListener() {
        return mOperationListener;
    }

    public String getPath() {
        return mFilePath;
    }

    public Operation getCurrentOp() {
        return mCurOp;
    }

    private void init() {
        bShare = new OperationButton(mContext);
        bShare.setImage(R.drawable.operation_share);
        bShare.setText(getOpeartionTip(FileUtil.OPERATION_TYPE_SHARE));
        bShare.setTag(FileUtil.OPERATION_TYPE_SHARE);
        bShare.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                FileOperator.doShare(mFilePath, mContext);
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_SHARE, mPosition);
                }
            }
        });

        bCopy = new OperationButton(mContext);
        bCopy.setImage(R.drawable.operation_copy);
        bCopy.setText(getOpeartionTip(FileUtil.OPERATION_TYPE_COPY));
        bCopy.setTag(FileUtil.OPERATION_TYPE_COPY);
        bCopy.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                FileOperator.copy(mFilePath, mContext);
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_COPY, mPosition);
                }
            }
        });

        bCut = new OperationButton(mContext);
        bCut.setImage(R.drawable.operation_cut);
        bCut.setText(getOpeartionTip(FileUtil.OPERATION_TYPE_CUT));
        bCut.setTag(FileUtil.OPERATION_TYPE_CUT);
        bCut.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                FileOperator.cut(mFilePath, mContext);
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_CUT, mPosition);
                }
            }
        });
        
        bOpenPath = new OperationButton(mContext);
        bOpenPath.setImage(R.drawable.operation_openfolder);
        bOpenPath.setText(getOpeartionTip(FileUtil.OPERATION_TYPE_OPEN_PATH));
        bOpenPath.setTag(FileUtil.OPERATION_TYPE_OPEN_PATH);
        bOpenPath.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                File file = new File(mFilePath);
                if (file.exists()) {
                    String path = mFilePath;
                    if (file.isFile()) {
                        path = file.getParent();
                    } else if (file.isDirectory()) {
                        path = mFilePath;
                    }
                    FileUtil.openFolder(mContext, path);
                } else {
                    Toast.makeText(mContext, R.string.error_file_not_exist, Toast.LENGTH_SHORT);
                }
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_OPEN_PATH, mPosition);
                }
            }
        });

        bDelete = new OperationButton(mContext);
        bDelete.setImage(R.drawable.operation_delete);
        bDelete.setText(getOpeartionTip(FileUtil.OPERATION_TYPE_DELETE));
        bDelete.setTag(FileUtil.OPERATION_TYPE_DELETE);
        bDelete.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (null != mCurOp)
                    mCurOp.setExit();
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_DELETE, mPosition);
                }
                if (mCategory == FileUtil.CATEGORY_APP) {
                    Uri uri = Uri.fromParts("package", mFilePath, null);
                    Intent intent = new Intent(Intent.ACTION_DELETE);
                    intent.setData(uri);
                    mContext.startActivity(intent);
                } else {
                    mCurOp = new OperationDelete(mContext,
                            OperationDelete.OP_DELETE_CONFIRM, mFilePath,
                            mCurPath);
                    mCurOp.setOnOperationEndListener(mOperationListener);
                    mCurOp.start();
                }
            }
        });

        bRingtone = new OperationButton(mContext);
        bRingtone.setImage(R.drawable.operation_ringtone);
        bRingtone
                .setText(getOpeartionTip(FileUtil.OPERATION_TYPE_SET_RINGTONE));
        bRingtone.setTag(FileUtil.OPERATION_TYPE_SET_RINGTONE);
        bRingtone.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                FileOperator.setRingTone(mFilePath, mContext);
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_SET_RINGTONE, mPosition);
                }
            }
        });

        bWallpaper = new OperationButton(mContext);
        bWallpaper.setImage(R.drawable.operation_wallpaper);
        bWallpaper
                .setText(getOpeartionTip(FileUtil.OPERATION_TYPE_SET_WALLPAPER));
        bWallpaper.setTag(FileUtil.OPERATION_TYPE_SET_WALLPAPER);
        bWallpaper.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                FileOperator.setWallPaper(mFilePath, mContext);
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_SET_WALLPAPER, mPosition);
                }
            }
        });

        bProperty = new OperationButton(mContext);
        bProperty.setImage(R.drawable.operation_property);
        bProperty
                .setText(getOpeartionTip(FileUtil.OPERATION_TYPE_SHOW_PROPERTY));
        bProperty.setTag(FileUtil.OPERATION_TYPE_SHOW_PROPERTY);
        bProperty.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mCategory == FileUtil.CATEGORY_APP) {
                    Intent intent = new Intent(
                            Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                    Uri uri = Uri.fromParts("package", mFilePath, null);
                    intent.setData(uri);
                    mContext.startActivity(intent);
                } else {
                    FileOperator.showProperty(mFilePath, mContext);
                }
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_SHOW_PROPERTY, mPosition);
                }
            }
        });

        bAddToFav = new OperationButton(mContext);
        bAddToFav.setImage(R.drawable.operation_add_fav);
        bAddToFav.setText(getOpeartionTip(FileUtil.OPERATION_TYPE_FAV_ADD));
        bAddToFav.setTag(FileUtil.OPERATION_TYPE_FAV_ADD);
        bAddToFav.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                FileOperator.addFavorite(mFilePath, mCategory, mContext);
                Toast.makeText(mContext, R.string.add_fav_success,
                        Toast.LENGTH_SHORT).show();
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_FAV_ADD, mPosition);
                }
            }
        });

        bDelFromFav = new OperationButton(mContext);
        bDelFromFav.setImage(R.drawable.operation_delete_fav);
        bDelFromFav
                .setText(getOpeartionTip(FileUtil.OPERATION_TYPE_FAV_REMOVE));
        bDelFromFav.setTag(FileUtil.OPERATION_TYPE_FAV_REMOVE);
        bDelFromFav.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                FileOperator.deleteFavorite(mFilePath, mContext);
                Toast.makeText(mContext, R.string.del_fav_success,
                        Toast.LENGTH_SHORT).show();
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_FAV_REMOVE, mPosition);
                }
            }
        });

        bRename = new OperationButton(mContext);
        bRename.setImage(R.drawable.operation_rename);
        bRename.setText(getOpeartionTip(FileUtil.OPERATION_TYPE_RENAME));
        bRename.setTag(FileUtil.OPERATION_TYPE_RENAME);
        bRename.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showInputDialog(mContext, FileUtil.OPERATION_TYPE_RENAME,
                        R.string.text_dialog_rename, mFilePath);
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_FAV_REMOVE, mPosition);
                }
            }
        });

        bZip = new OperationButton(mContext);
        bZip.setImage(R.drawable.operation_zip);
        bZip.setText(getOpeartionTip(FileUtil.OPERATION_TYPE_ZIP));
        bZip.setTag(FileUtil.OPERATION_TYPE_ZIP);
        bZip.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showInputDialog(mContext, FileUtil.OPERATION_TYPE_ZIP,
                        R.string.text_dialog_zip, mFilePath);
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_FAV_REMOVE, mPosition);
                }
            }
        });

        bUnzip = new OperationButton(mContext);
        bUnzip.setImage(R.drawable.operation_unzip);
        bUnzip.setText(getOpeartionTip(FileUtil.OPERATION_TYPE_UNZIP));
        bUnzip.setTag(FileUtil.OPERATION_TYPE_UNZIP);
        bUnzip.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (null != mCurOp)
                    mCurOp.setExit();
                File file = new File(mFilePath);
                mCurOp = new OperationUnzip(mContext, file.getParent(),
                        mFilePath);
                mCurOp.setOnOperationEndListener(mOperationListener);
                if (mOperationListener != null) {
                    mOperationListener.onOperationClicked(
                            FileUtil.OPERATION_TYPE_FAV_REMOVE, mPosition);
                }

                mCurOp.start();
            }
        });

        bMore = new OperationButton(mContext);
        bMore.setImage(R.drawable.operation_more);
        bMore.setText(getOpeartionTip(FileUtil.OPERATION_TYPE_MORE));
        bMore.setTag(FileUtil.OPERATION_TYPE_MORE);
        bMore.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mIsCollapsed) {
                    expandOperation();                                   
                } else {
                    // here will not done
                    collapseOperation();                    
                }
            }
        });
        bMore.setBackgroundResource(R.drawable.operation_last_button_background);
        
        bRootLine = new LinearLayout(mContext);
        bRootLine.setOrientation(LinearLayout.HORIZONTAL);

        addView(bRootLine);
        setHorizontalScrollBarEnabled(false);
    }

    private void collapseOperation() {
        mIsCollapsed = true;

        int visibleIndex = getMaxButtonCount() - 1;

        if (bRootLine.getChildCount() <= getMaxButtonCount() + 1) {
            for (int i = 0; i < bRootLine.getChildCount(); i++) {
                OperationButton button = (OperationButton) bRootLine
                        .getChildAt(i);
                String j = String.valueOf(i);
                button.setVisibility(View.VISIBLE);
            }
            bMore.setVisibility(View.GONE);
            OperationButton view = (OperationButton)bRootLine.getChildAt(bRootLine.getChildCount()-2);
            view.setBackgroundResource(R.drawable.operation_last_button_background);  
        } else {
            for (int i = 0; i < bRootLine.getChildCount(); i++) {
                OperationButton button = (OperationButton) bRootLine
                        .getChildAt(i);
                String j = String.valueOf(i);
                if (i >= visibleIndex) {
                    button.setVisibility(View.GONE);
                } else {
                    button.setVisibility(View.VISIBLE);
                }
            }
            bMore.setVisibility(View.VISIBLE);
        }
    }

    private void expandOperation() {
        mIsCollapsed = false;
        int visibleIndex = getMaxButtonCount() - 1;
        for (int i = 0; i < bRootLine.getChildCount(); i++) {
            OperationButton button = (OperationButton) bRootLine.getChildAt(i);
            if (i > visibleIndex - 1) {
                button.setVisibility(View.VISIBLE);
            }
        }          
        
        bMore.setVisibility(View.GONE);
        OperationButton view = (OperationButton)bRootLine.getChildAt(bRootLine.getChildCount()-2);
        view.setBackgroundResource(R.drawable.operation_last_button_background); 
        
        int count = bRootLine.getChildCount();
        int width = getMinButtonWidth();
        int move = (count - 1) * width - FileUtil.SCREEN_WIDTH;

        moveView(move, 100);

//        postDelayed(new Runnable(){
//            @Override
//            public void run() {
//                smoothScrollBy(FileUtil.SCREEN_WIDTH - getMinButtonWidth() , 0);
//            }
//        }, 100);
    }

    private Handler mHandler = new Handler () {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MSG_OPERATIONVIEW_MOVE:
                Bundle data = msg.getData();
                int move = data.getInt("move");
                smoothScrollBy(move, 0);
                if (move > FileUtil.SCREEN_WIDTH - getMinButtonWidth()) {
                    moveView(FileUtil.SCREEN_WIDTH - move - getMinButtonWidth(), 400);
                }
                break;
            }
        }

    };

    private void moveView(int move, long time){
        Message msg = mHandler.obtainMessage(MSG_OPERATIONVIEW_MOVE);
        Bundle data = new Bundle();
        data.putInt("move", move);
        msg.setData(data);
        mHandler.sendMessageDelayed(msg, time);
    }
    private int getOpeartionTip(Integer operationType) {
        switch (operationType) {
        case FileUtil.OPERATION_TYPE_COPY:
            return R.string.operation_copy;
        case FileUtil.OPERATION_TYPE_CUT:
            return R.string.operation_cut;
        case FileUtil.OPERATION_TYPE_DELETE:
            return R.string.operation_delete;
        case FileUtil.OPERATION_TYPE_SHARE:
            return R.string.operation_share;
        case FileUtil.OPERATION_TYPE_SHOW_PROPERTY:
            return R.string.operation_property;
        case FileUtil.OPERATION_TYPE_RENAME:
            return R.string.operation_rename;
        case FileUtil.OPERATION_TYPE_FAV_ADD:
            return R.string.operation_fav_add;
        case FileUtil.OPERATION_TYPE_FAV_REMOVE:
            return R.string.operation_fav_remove;
        case FileUtil.OPERATION_TYPE_ZIP:
            return R.string.operation_zip;
        case FileUtil.OPERATION_TYPE_UNZIP:
            return R.string.operation_unzip;
        case FileUtil.OPERATION_TYPE_SET_RINGTONE:
            return R.string.operation_set_ringtone;
        case FileUtil.OPERATION_TYPE_SET_WALLPAPER:
            return R.string.operation_set_wallpaper;
        case FileUtil.OPERATION_TYPE_OPEN_PATH:
            return R.string.operation_open_path;
        case FileUtil.OPERATION_TYPE_MORE:
            return R.string.operation_more;
        default:
            return -1;
        }
    }

    private void showInputDialog(Context context, int type, int title,
            String originalName) {
        try {
            Log.v(TAG, "enter setDialog, originalName:" + originalName);
            if (null == originalName) {
                return;
            }

            AlertDialog inputDialog = new AlertDialog.Builder(context).setIcon(
                    android.R.drawable.ic_dialog_alert).create();

            initInputDialog(inputDialog, type, title, originalName);
            inputDialog.show();
        } catch (Exception ex) {
            Log.v(TAG, "showInputDialog() Exception : " + ex.toString());
        }
    }

    private void initInputDialog(AlertDialog dialog, int type, int title,
            final String filePath) {
        dialog.getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);

        File file = new File(filePath);
        String fileName = file.getName();
        if (type == FileUtil.OPERATION_TYPE_ZIP) {
            if ((null != fileName) && (-1 != fileName.indexOf('.')))
                fileName = fileName.substring(0, fileName.indexOf('.'));
        }
        final Context context = dialog.getContext();
        mEditText = new EditText(mContext);

        // setting dialog's edit text
        InputFilter[] filters = new InputFilter[1];
        filters[0] = new EditTextLengthFilter(MAX_INPUT);
        ((EditTextLengthFilter) filters[0])
                .setTriger(new EditTextLengthFilter.Triger() {
                    public void onGetMax() {
                        Toast toast = Toast.makeText(context,
                                R.string.max_input_reached, Toast.LENGTH_SHORT);
                        toast.setGravity(Gravity.BOTTOM, 0, 0);
                        toast.show();
                    }
                });
        mEditText.setFilters(filters);
        mEditText.setSingleLine(true);
        initSelection(mEditText, fileName);
        dialog.setView(mEditText);

        // set dialog title
        dialog.setTitle(title);
        final int operatetype = type;

        // setting dialog's message
        String message = null;
       if(file.isDirectory() && type != FileUtil.OPERATION_TYPE_ZIP){
          message = this.getResources().getString(R.string.enter_foldername);
        }else{
          message = this.getResources().getString(R.string.enter_filename);
        }
        dialog.setMessage(message);

        final AlertDialog textInputDialog = dialog;
        textInputDialog.setCanceledOnTouchOutside(false);
        textInputDialog.setButton(context.getText(R.string.dialog_ok),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface arg0, int arg1) {
                        startOperation(operatetype, filePath, textInputDialog);
                    }
                });
        textInputDialog.setButton2(context.getText(R.string.dialog_cancel),
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
        textInputDialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
            public boolean onKey(DialogInterface arg0, int arg1, KeyEvent event) {
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

    private void initSelection(EditText editText, String name) {
        editText.setText(name);
        int index = name.lastIndexOf(".");
        if (index == -1) {
            editText.setSelection(0, name.length());
        } else {
            editText.setSelection(0, index);
        }
    }

    private void startOperation(int type, final String filePath,
            AlertDialog inputDialog) {
        File file = new File(filePath);
        int Rename_type ;

        String newName = mEditText.getText().toString().trim();
        final String path = file.getParent();

        if (type == FileUtil.OPERATION_TYPE_ZIP) {
            newName = newName + ".zip";
        }
        
        if(file.isDirectory()){
            Rename_type = RENAME_FOLDER;
        }else{
            Rename_type = RENAME_FILE;
        }
        int ret = FileUtil.checkFileName(newName, path);
        if (0 != ret) {
            initSelection(mEditText, newName);
            String msg = handleInputInvalid(ret,Rename_type);
            inputDialog.setMessage(msg);
            allowdismiss = false;
            return;
        }

        final String fileName = newName;
        switch (type) {
        case FileUtil.OPERATION_TYPE_RENAME:
            file.renameTo(new File(path + "/" + newName));
            FileOperator.deleteFavorite(filePath, mContext);
            new Thread() {
                @Override
                public void run() {
                    FileUtil.updateFileInfo(mContext, path + "/" + fileName,
                            filePath);
                }
            }.start();
            if (mOperationListener != null) {
                mOperationListener.onOperationEnd();
            }
            FileUtil.showToast(mContext, R.string.fm_operate_over);
            allowdismiss = true;
            break;

        case FileUtil.OPERATION_TYPE_ZIP:
            allowdismiss = true;
            if (null != mCurOp)
                mCurOp.setExit();
            mCurOp = new OperationZip(mContext, path, filePath, newName);
            mCurOp.setOnOperationEndListener(mOperationListener);
            mCurOp.start();
            break;
        }
    }

    private String handleInputInvalid(int err, int rename_type) {
        int id_msg = 0;
        switch (err) {
        case Defs.ERR_EMPTY:
            id_msg = R.string.fm_input_name_first;
            break;
        case Defs.ERR_INVALID_CHAR:
            if(rename_type == RENAME_FOLDER){
               id_msg = R.string.text_contain_unacceptable_char_folder;
            }else{
               id_msg = R.string.text_contain_unacceptable_char_file;
            }
            break;
        case Defs.ERR_INVALID_INIT_CHAR:
            if(rename_type == RENAME_FOLDER){
               id_msg = R.string.fm_name_invalid_head_folder;
            }else{
               id_msg = R.string.fm_name_invalid_head_file;
            }
            break;
        case Defs.ERR_TOO_LONG:
            if(rename_type == RENAME_FOLDER){
               id_msg = R.string.op_filename_too_long_folder;
            }else{
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
            if(rename_type == RENAME_FOLDER){
                id_msg = R.string.fm_name_invalid_name_folder;
            }else{
                id_msg = R.string.fm_name_invalid_name_file;
            }
            break;
        }

        return this.getResources().getString(id_msg);
    }

    private class OperationButton extends LinearLayout {
        ImageView icon;
        TextView text;

        public OperationButton(Context context) {
            super(context);

            setOrientation(LinearLayout.VERTICAL);
            setBackgroundResource(R.drawable.operation_button_background);
            setGravity(Gravity.CENTER);
            setPadding(0, 0, 8, 0);
            
            icon = new ImageView(context);
            text = new TextView(context);
            text.setGravity(Gravity.CENTER);
            text.setTextColor(Color.WHITE);
            text.setSingleLine();
            text.setEllipsize(android.text.TextUtils.TruncateAt.END);
            text.setPadding(5, 0, 5, 0);

            addView(icon);
            addView(text);
        }

        public void setText(CharSequence cs) {
            text.setText(cs);
        }

        public void setText(int resId) {
            text.setText(resId);
        }

        public void setImage(int resId) {
            icon.setImageResource(resId);
        }
    }

    public interface OperationListener extends OnOperationEndListener {
        public void onOperationClicked(int opType, int position);
    }
}
