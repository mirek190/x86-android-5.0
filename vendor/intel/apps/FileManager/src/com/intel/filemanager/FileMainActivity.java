package com.intel.filemanager;

import android.app.ActionBar;
import android.app.ActionBar.Tab;
import android.app.ActionBar.TabListener;
import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v13.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager.SimpleOnPageChangeListener;
import android.text.TextUtils;
import android.view.Display;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.WindowManager;
import android.widget.Toast;
import android.app.FragmentManager;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.LogHelper;

public class FileMainActivity extends Activity implements TabListener {
    private static LogHelper Log = LogHelper.getLogger();
    private static final String TAG = "FileMainActivity";

    public static final String ACTION_PASTE_FILE = "com.intel.filemanager.action.pastefile";

    public final static int TAB_INDEX_CATEGORY = 0;
    public final static int TAB_INDEX_DIRECTORY = 1;

    public static final String EXTRA_TAB_INDEX = "extra_tab_index";
    public static final String EXTRA_FILE_PATH = "extra_file_path";

    // use this preference to check if it is the first launch
    // an create index alert dialog will be shown when first launch FileManager
    public static final String PREF_FIRST_LAUNCH = "pref_first_launch";
    public static final String DIRECTORY_TAG = "directory";
    public static final String CATEGORY_TAG = "category";

    private ActionBar mActionBar;
    private Tab mTabCategory;
    private Tab mTabList;
    private CustomViewPager mViewPager;
    private ViewPagerAdapter mViewPagerAdapter;

    private Fragment mFragmentCategory;
    private Fragment mFragmentDir;
    private SharedPreferences mPreference;
    private Activity mActivity;

    public static boolean isFirstLaunch = false;

    private static final String DIR_SORT_MODE = "dir_sort_mode";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "FileMainActivity.onCreate()");
        super.onCreate(savedInstanceState);

        if (null != savedInstanceState) {
            mFragmentCategory = getFragmentManager().findFragmentByTag(
                    savedInstanceState.getString(CATEGORY_TAG));
            mFragmentDir = getFragmentManager().findFragmentByTag(
                    savedInstanceState.getString(DIRECTORY_TAG));
        }

        setContentView(R.layout.file_main);
        mActivity = this;
        mPreference = PreferenceManager.getDefaultSharedPreferences(this);

        mActionBar = getActionBar();
        mActionBar.setNavigationMode(ActionBar.NAVIGATION_MODE_TABS);
        mActionBar.setDisplayShowHomeEnabled(false);
        mActionBar.setDisplayShowTitleEnabled(false);

        mViewPager = (CustomViewPager) findViewById(R.id.pager);
        mViewPagerAdapter = new ViewPagerAdapter(getFragmentManager());
        mViewPager.setAdapter(mViewPagerAdapter);
        mViewPager.setOnPageChangeListener(new PageChangeListener());
        mTabCategory = mActionBar.newTab().setText(R.string.tab_title_categroy)
                .setTabListener(this);
        mTabList = mActionBar.newTab().setText(R.string.tab_title_file)
                .setTabListener(this);

        mActionBar.addTab(mTabCategory);
        mActionBar.addTab(mTabList);

        int tabIndex = -1;
        if (getIntent().hasExtra(EXTRA_TAB_INDEX)) {
            tabIndex = getIntent().getIntExtra(EXTRA_TAB_INDEX,
                    TAB_INDEX_CATEGORY);
        } else {
            tabIndex = mPreference.getInt(EXTRA_TAB_INDEX, TAB_INDEX_CATEGORY);
        }
        mActionBar.selectTab(getTab(tabIndex));
        mActionBar.setDisplayOptions(0);
        handleIntent(getIntent());

        registerIntent();

        isFirstLaunch = mPreference.getBoolean(PREF_FIRST_LAUNCH, true);
    }

    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        super.onResume();

        WindowManager windowManager = getWindowManager();
        Display display = windowManager.getDefaultDisplay();
        FileUtil.SCREEN_WIDTH = display.getWidth();
        FileUtil.SCREEN_HEIGHT = display.getHeight();

        if (!FileUtil.checkIfSDExist(this)) {
            Toast.makeText(this, R.string.sdcard_unmount, Toast.LENGTH_SHORT).show();
            this.finish();
            return;
        }
        invalidateOptionsMenu();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    protected BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.v(TAG, "SubCategoryActivity receive intent:" + intent.toString());
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
            } else if (action.equals(Intent.ACTION_MEDIA_REMOVED)
                    || action.equals(Intent.ACTION_MEDIA_EJECT)
                    || action.equals(Intent.ACTION_MEDIA_UNMOUNTED)) {
                finish();
            } else if (action.equals(Intent.ACTION_MEDIA_SCANNER_FINISHED)) {
                FileDirFragment mFileDir = (FileDirFragment) mViewPagerAdapter
                        .getItem(TAB_INDEX_DIRECTORY);
                FileCategoryFragment mFileCategory = (FileCategoryFragment) mViewPagerAdapter
                        .getItem(TAB_INDEX_CATEGORY);

                mFileDir.refreshThumbnail();
                mFileCategory.refreshCategoryInfo();
            }
        }
    };

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        // TODO Auto-generated method stub
        super.onSaveInstanceState(outState);
        try {
            String tagCate = mFragmentCategory.getTag();
            String tagDir = mFragmentDir.getTag();
            outState.putString(CATEGORY_TAG, tagCate);
            outState.putString(DIRECTORY_TAG, tagDir);
        } catch (NullPointerException e) {
            e.printStackTrace();
            return;
        }
    }

    private void registerIntent() {
        IntentFilter intentFilter = new IntentFilter(
                Intent.ACTION_MEDIA_MOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
        intentFilter.addAction(Intent.ACTION_MEDIA_REMOVED);
        intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_FINISHED);
        intentFilter.addDataScheme("file");
        this.registerReceiver(mReceiver, intentFilter);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        this.getMenuInflater().inflate(R.menu.options, menu);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        // TODO Auto-generated method stub
        super.onPrepareOptionsMenu(menu);

        if (mActionBar.getSelectedNavigationIndex() == TAB_INDEX_CATEGORY
                && mFragmentCategory != null) {
            menu.findItem(R.id.action_setting).setVisible(true);
            menu.findItem(R.id.action_new).setVisible(false);
            menu.findItem(R.id.action_property).setVisible(false);
            menu.findItem(R.id.action_order).setVisible(false);
            menu.findItem(R.id.action_fav).setVisible(false);
        } else if (mActionBar.getSelectedNavigationIndex() == TAB_INDEX_DIRECTORY
                && mFragmentDir != null) {
            menu.findItem(R.id.action_setting).setVisible(true)
                    .setShowAsAction(MenuItem.SHOW_AS_ACTION_WITH_TEXT);
            menu.findItem(R.id.action_new).setVisible(true);
            menu.findItem(R.id.action_property).setVisible(true);
            menu.findItem(R.id.action_order).setVisible(true);
            menu.findItem(R.id.action_fav).setVisible(true);
            switch (mPreference.getInt(DIR_SORT_MODE, FileUtil.SORT_BY_NAME)) {
                case 1:
                    menu.findItem(R.id.action_order_by_name).setChecked(true);
                    break;
                case 2:
                    menu.findItem(R.id.action_order_by_size).setChecked(true);
                    break;
                case 3:
                    menu.findItem(R.id.action_order_by_time).setChecked(true);
                    break;
                case 4:
                    menu.findItem(R.id.action_order_by_type).setChecked(true);
                    break;
            }
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_setting:
                Intent settingIntent = new Intent();
                settingIntent.setClass(this, FileManagerSettings.class);
                startActivity(settingIntent);
                break;
            default:
                ((FileDirFragment) mFragmentDir).onOptionsItemSelected(item);
                break;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        Log.d(TAG,
                "FileMainActivity.onNewIntent(), intent is: " + intent.toURI());
        setIntent(intent);
        handleIntent(intent);
    }

    private void handleIntent(Intent intent) {
        FileDirFragment mFileDir = (FileDirFragment) mViewPagerAdapter
                .getItem(TAB_INDEX_DIRECTORY);

        String action = intent.getAction();
        if (!TextUtils.isEmpty(action)) {
            if (action.equals(Intent.ACTION_VIEW)) {
                mFileDir.mLaunchedWithPath = false;
                mActionBar.selectTab(getTab(TAB_INDEX_DIRECTORY));
            } else if (action.equals(ACTION_PASTE_FILE)) {
                mFileDir.mLaunchedWithPath = false;
                mActionBar.selectTab(getTab(TAB_INDEX_DIRECTORY));
                mFileDir.showPasteButton();
            } else if (intent.hasExtra("Path")) {
                mFileDir.mLaunchedWithPath = false;
                mActionBar.selectTab(getTab(TAB_INDEX_DIRECTORY));
                intent.putExtra(FileMainActivity.EXTRA_FILE_PATH, intent.getStringExtra("Path"));
            }
        }

    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        WindowManager windowManager = getWindowManager();
        Display display = windowManager.getDefaultDisplay();
        FileUtil.SCREEN_WIDTH = display.getWidth();
        FileUtil.SCREEN_HEIGHT = display.getHeight();
        invalidateOptionsMenu();
        super.onConfigurationChanged(newConfig);
    }

    @Override
    protected void onDestroy() {
        mPreference
                .edit()
                .putInt(EXTRA_TAB_INDEX,
                        mActionBar.getSelectedNavigationIndex()).commit();
        try {
            unregisterReceiver(mReceiver);
        } catch (Exception e) {
            Log.e(TAG, "unregisterReceiver error when onDestroy");
        }

        super.onDestroy();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (mViewPager.getCurrentItem() == TAB_INDEX_DIRECTORY) {
            if (mFragmentDir != null
                    && ((FileDirFragment) mFragmentDir).handleKeyDown(keyCode,
                            event)) {
                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    private Tab getTab(int position) {
        switch (position) {
            case TAB_INDEX_CATEGORY:
                return mTabCategory;
            case TAB_INDEX_DIRECTORY:
                return mTabList;
        }
        return null;
    }

    public Fragment getFileDirFragment() {
        return mFragmentDir;
    }

    public void setPageScrollable(boolean enabled) {
        mViewPager.setPagingEnabled(enabled);
    }

    @Override
    public void onTabSelected(Tab tab, FragmentTransaction ft) {
        mViewPager.setCurrentItem(tab.getPosition());
        mActivity.invalidateOptionsMenu();
    }

    @Override
    public void onTabUnselected(Tab tab, FragmentTransaction ft) {

    }

    @Override
    public void onTabReselected(Tab tab, FragmentTransaction ft) {

    }

    public class ViewPagerAdapter extends FragmentPagerAdapter {
        public ViewPagerAdapter(FragmentManager fm) {
            super(fm);
        }

        @Override
        public Fragment getItem(int position) {
            if (position == TAB_INDEX_CATEGORY) {
                if (mFragmentCategory == null)
                    mFragmentCategory = new FileCategoryFragment();
                return mFragmentCategory;
            } else if (position == TAB_INDEX_DIRECTORY) {
                if (mFragmentDir == null)
                    mFragmentDir = new FileDirFragment();
                return mFragmentDir;
            }
            throw new IllegalStateException("No fragment at position "
                    + position);
        }

        @Override
        public int getCount() {
            return TAB_INDEX_DIRECTORY + 1;
        }
    }

    private class PageChangeListener extends SimpleOnPageChangeListener {
        @Override
        public void onPageSelected(int position) {
            Log.d(TAG, "onPageSelected position is: " + position + "");
            mActionBar.selectTab(getTab(position));
            mActivity.invalidateOptionsMenu();
            if (position == TAB_INDEX_CATEGORY && mFragmentCategory != null) {
                ((FileCategoryFragment) mFragmentCategory).refreshCategoryInfo();
            }
        }
    }
}
