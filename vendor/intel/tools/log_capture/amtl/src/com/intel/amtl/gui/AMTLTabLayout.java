/* Android AMTL
 *
 * Copyright (C) Intel 2013
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Erwan Bracq <erwan.bracq@intel.com>
 * Author: Morgane Butscher <morganeX.butscher@intel.com>
 */

package com.intel.amtl.gui;

import android.app.Activity;
import android.app.Application;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ProgressBar;
import android.widget.TabHost;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.amtl.AMTLApplication;
import com.intel.amtl.R;
import com.intel.amtl.config_parser.ConfigParser;
import com.intel.amtl.exceptions.ModemControlException;
import com.intel.amtl.exceptions.ParsingException;
import com.intel.amtl.helper.TelephonyStack;
import com.intel.amtl.models.config.ModemConf;
import com.intel.amtl.models.config.LogOutput;
import com.intel.amtl.modem.Gsmtty;
import com.intel.amtl.modem.GsmttyManager;
import com.intel.amtl.modem.ModemController;
import com.intel.amtl.platform.Platform;
import com.intel.internal.telephony.MmgrClientException;
import com.intel.internal.telephony.ModemStatus;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;

import org.xmlpull.v1.XmlPullParserException;

// AMTL is using a sharedPreferences file to store volatile data.
// As this is spread through the classes, here the summary of the
// stored data:
// @index (int) the current executed configuration
// @log_active (boolean) indicates if logging is active
// @cpuName (string) the platform CPU name
// @modemName (string) the platform modem name
// @default_flush_cmd (string) if populated, the command to execute to flush to NVM

public class AMTLTabLayout extends Activity implements GeneralSetupFrag.GSFCallBack,
        ExpertSetupFrag.OnExpertModeApplied, ExpertSetupFrag.OnModeChanged,
        MasterSetupFrag.OnModeChanged, GeneralSetupFrag.OnModeChanged {

    private final String TAG = AMTLApplication.getAMTLApp().getLogTag();
    private final String MODULE = "AMTLTabLayout";
    private final String MDM_CONNECTION_TAG = "AMTL_modem_connection";

    // Tab list
    private final String TAB1_FRAG_NAME = "General setup";
    private final String TAB2_FRAG_NAME = "Advanced options";
    private final String TAB3_FRAG_NAME = "Expert options";

    private String currentCatalogPath = null;

    private TabHost mTabHost;

    // Target fragment for progress popup.
    private FragmentManager fragManager;

    private GeneralSetupFrag tab1_frag;
    private MasterSetupFrag tab2_frag;
    private ExpertSetupFrag tab3_frag;

    private ConfigParser configParser = null;
    private ArrayList<LogOutput> configOutputs = null;
    private ModemConf expertModemConf = null;

    // Fragment to display progress dialog during modem connection
    private ModemConnectionFrag mdmConnectFrag = null;

    // Handle modem connection
    private ModemController mdmCtrl;

    private Platform platform = null;

    private Boolean buttonChanged = false;

    private Boolean firstCreated = true;

    // Telephony stack check - in order to enable it if disabled
    private TelephonyStack telStackSetter;

    private void loadConfiguration() throws ParsingException {
        FileInputStream fin = null;
        SharedPreferences.Editor editor = this.getSharedPreferences("AMTLPrefsData",
                Context.MODE_PRIVATE).edit();
        Log.d(TAG, MODULE + ": Will remove default_flush_cmd entry.");
        editor.remove("default_flush_cmd");
        editor.commit();

        try {
            // Use of getXmlPlatform
            this.platform = new Platform();
            this.currentCatalogPath = this.platform.getPlatformConf();

            Log.d(TAG, MODULE + ": Will load " + this.currentCatalogPath +
                    " configuration file");
            fin = new FileInputStream(this.currentCatalogPath);
            if (fin != null) {
                this.configOutputs = this.configParser.parseConfig(fin);
                Log.d(TAG, MODULE + ": Configuration loaded:");
                Log.d(TAG, MODULE + ": =======================================");
                for (LogOutput o: configOutputs) {
                    o.printToLog();
                    Log.d(TAG, MODULE + ": ---------------------------------------");
                }
                Log.d(TAG, MODULE +  ": =======================================");
            }
        } catch (Exception ex) {
             throw new ParsingException("Cannot load config file " + ex.getMessage());
        } finally {
            if (fin != null) {
                try {
                    fin.close();
                } catch (IOException ex) {
                    Log.e(TAG, MODULE + ": Error during close " + ex);
                }
            }
        }
    }

    public void initializeTab() {
        TabHost.TabSpec spec = mTabHost.newTabSpec(TAB1_FRAG_NAME);
        spec.setContent(new TabHost.TabContentFactory() {
            public View createTabContent(String tag) {
                return findViewById(android.R.id.tabcontent);
            }
        });
        spec.setIndicator(createTabView(TAB1_FRAG_NAME));
        mTabHost.addTab(spec);

        spec = mTabHost.newTabSpec(TAB2_FRAG_NAME);
        spec.setContent(new TabHost.TabContentFactory() {
            public View createTabContent(String tag) {
                return findViewById(android.R.id.tabcontent);
            }
        });
        spec.setIndicator(createTabView(TAB2_FRAG_NAME));
        mTabHost.addTab(spec);

        spec = mTabHost.newTabSpec(TAB3_FRAG_NAME);
        spec.setContent(new TabHost.TabContentFactory() {
            public View createTabContent(String tag) {
                return findViewById(android.R.id.tabcontent);
            }
        });
        spec.setIndicator(createTabView(TAB3_FRAG_NAME));
        mTabHost.addTab(spec);

        // Focus on first tab
        mTabHost.setCurrentTab(0);
    }

    TabHost.OnTabChangeListener listener = new TabHost.OnTabChangeListener() {
        public void onTabChanged(String tabId) {
            if (tabId.equals(TAB1_FRAG_NAME)) {
                pushFragments(tabId, tab1_frag);
                tab1_frag.setExpertConf(expertModemConf);
                tab1_frag.setButtonChanged(buttonChanged);
                tab1_frag.setFirstCreated(firstCreated);
            } else if (tabId.equals(TAB2_FRAG_NAME)) {
                pushFragments(tabId, tab2_frag);
                tab2_frag.setButtonChanged(buttonChanged);
            } else if(tabId.equals(TAB3_FRAG_NAME)) {
                pushFragments(tabId, tab3_frag);
            }
            firstCreated = false;
        }
    };

    public void pushFragments(String tag, Fragment fragment){

        FragmentManager manager = getFragmentManager();
        FragmentTransaction ft = manager.beginTransaction();

        ft.replace(android.R.id.tabcontent, fragment);
        ft.commit();
    }

    /*
     * returns the tab view i.e. the tab text
     */
    private View createTabView(final String text) {
        View view = LayoutInflater.from(this).inflate(R.layout.tab_button, null);
        if (view != null) {
            ((TextView) view.findViewById(R.id.tab_button_text)).setText(text);
        } else {
            UIHelper.exitDialog(this, "Error on UI", "View cannot be displayed, AMTL will exit");
        }
        return view;
    }

    // Activity overrides.
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.amtltablayout);
        Log.d(TAG, MODULE + ": creation of AMTL activity ");
        telStackSetter = new TelephonyStack();
        ((AMTLApplication) this.getApplication()).setContext(getBaseContext());

        if (!telStackSetter.isEnabled()) {
            UIHelper.messageSetupTelStack(this,
                    "Telephony stack disabled !", "Would you like to enable it ?",
                    telStackSetter);
        } else {
            try {
                this.configParser = new ConfigParser(this);
                this.loadConfiguration();

            } catch (ParsingException ex) {
                UIHelper.exitDialog(this,
                        "Error during XML configuration file parsing ", "AMTL will exit: "
                        + ex.getMessage());
            }
            this.firstCreated = true;
            tab1_frag = new GeneralSetupFrag(this.configOutputs);
            tab2_frag = new MasterSetupFrag();
            tab3_frag = new ExpertSetupFrag();

            mTabHost = (TabHost)findViewById(android.R.id.tabhost);
            if (mTabHost != null) {
                mTabHost.setOnTabChangedListener(listener);
                mTabHost.setup();
            }

            initializeTab();

            fragManager = getFragmentManager();

            // fragment to display progress dialog during modem connection
            mdmConnectFrag = new ModemConnectionFrag();

            try {
                // instantiation of mdmCtrl to be able to connect and disconnect when exiting AMTL
                this.mdmCtrl = ModemController.getInstance();
            } catch (ModemControlException ex) {
                Log.e(TAG, MODULE + ": connection to Modem Status Manager failed");
                UIHelper.exitDialog(this,
                        "Modem connection failed ", "AMTL will exit: " + ex.getMessage());
            }

            // launch connection to modem with a progress dialog
            if (mdmConnectFrag != null) {
                mdmConnectFrag.handlerConn();
                mdmConnectFrag.show(fragManager, MDM_CONNECTION_TAG);
            }
        }
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, MODULE + ": onDestroy AMTL activity");
        if (this.mdmCtrl != null) {
            this.mdmCtrl.cleanBeforeExit();
            this.mdmCtrl = null;
        }
        super.onDestroy();
    }

    @Override
    public void onResume() {
        super.onResume();
        AMTLApplication.setPauseState(false);
        try {
            if (this.mdmCtrl != null) {
                this.mdmCtrl.acquireResource();
                if (this.mdmCtrl.isModemAcquired()) {
                    this.mdmCtrl.openTty();
                }
            }
        } catch (MmgrClientException ex) {
            Log.e(TAG, MODULE + ": Cannot acquire modem resource " + ex);
            UIHelper.exitDialog(this,
                    "Cannot acquire modem resource ", "AMTL will exit: " + ex);
        } catch (ModemControlException ex) {
            Log.e(TAG, MODULE + ex);
            UIHelper.exitDialog(this,
                    "Cannot open tty ", "AMTL will exit: " + ex);
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        AMTLApplication.setPauseState(true);
        if (this.mdmCtrl != null) {
            this.mdmCtrl.releaseResource();
            if (AMTLApplication.getCloseTtyEnable()) {
                // Close Tty enabled
                this.mdmCtrl.close();
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public void onConfigurationApplied(int resultCode) {

        if (resultCode == Activity.RESULT_OK) {
            Toast.makeText(this, "Configuration applied to modem.", Toast.LENGTH_LONG).show();
        }
        if (resultCode == Activity.RESULT_CANCELED) {
            Toast.makeText(this, "Configuration failure, please check logs.",
                    Toast.LENGTH_LONG).show();
        }
    }

    @Override
    public void onExpertConfApplied(ModemConf conf) {
        if (conf != null) {
            this.expertModemConf = conf;
        }
    }

    @Override
    public void onModeChanged(Boolean changed) {
        if (changed != null) {
            this.buttonChanged = changed;
        }
    }

    // Embedded class to handle connection to modem (Dialog part).
    public static class ModemConnectionFrag extends DialogFragment implements Handler.Callback {
        final static int MSG_CONNECTION_SUCCESS = 0;
        final static int MSG_CONNECTION_FAIL = 1;
        ProgressBar connectProgNot;
        // thread executed while Dialog Box is displayed.
        ConnectModemTask connectMdm;

        public void handlerConn() {
            // This allows to get ModemConnectTerminated on the specified Fragment.
            connectMdm = new ConnectModemTask(this);
        }

        public void ModemConnectTerminated(String exceptReason) {
            /* dismiss() is possible only if we are on the current Activity.
            And will crash if we have switched to another one.*/
            if (isResumed()) {
                dismiss();
            }

            // if the modem connection thread has returned an exception, AMTL exits
            if (exceptReason != null) {
                UIHelper.exitDialog(getActivity(),
                        "Modem connection failed ", "AMTL will exit: " + exceptReason);
            }
            connectMdm = null;
        }

        // Function overrides for the DialogFragment instance.
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setRetainInstance(true);
            // Spawn the thread to connect to modem.
            if (connectMdm != null) {
                connectMdm.start();
            }
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                Bundle savedInstanceState) {
            // Create dialog box.
            View view = inflater.inflate(R.layout.fragment_task, container);
            connectProgNot = (ProgressBar)view.findViewById(R.id.progressBar);
            getDialog().setTitle("Connection to modem");
            setCancelable(false);

            return view;
        }

        @Override
        public void onDestroyView() {
            /* This will allow dialog box to stay even if parent layout configuration is
            changed (rotation). */
            if (getDialog() != null && getRetainInstance()) {
                getDialog().setDismissMessage(null);
            }
            super.onDestroyView();
        }

        @Override
        public void onResume() {
            /* This allows to close dialog box if the thread ends while we are not focused
            on the activity.*/
            super.onResume();
            if (connectMdm == null) {
                dismiss();
            }
        }

        @Override
        public boolean handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_CONNECTION_SUCCESS:
                    ModemConnectTerminated(null);
                    break;
                case MSG_CONNECTION_FAIL:
                    String message = (String)msg.obj;
                    ModemConnectTerminated(message);
                    break;
            }
            return true;
        }
    }

    // embedded class to handle modem connection (thread part).
    public static class ConnectModemTask implements Runnable {
        private final String TAG = "AMTL";
        private final String MODULE = "ConnectModemTask";
        private static final long RETRY_DELAY_MS = 20000;
        private static final int NUM_RETRIES = 3;
        private Handler mHandler;
        private ModemController mdmCtrl;
        private Thread mThread;

        public ConnectModemTask(ModemConnectionFrag modConnFrag) {
            mHandler = new Handler(modConnFrag);
        }

        public void start() {
            mThread = new Thread(this);
            mThread.setName("AMTL Modem connection");
            mThread.start();
        }

        // Function overrides for modem connection thread.
        public void run() {
            String exceptReason = null;
            try {
                mdmCtrl = ModemController.getInstance();
            } catch (ModemControlException ex) {
                mdmCtrl = null;
                exceptReason = ex.getMessage();
                Log.e(TAG, MODULE + ": Connection to modem failed: "
                        + exceptReason);
                if (mHandler != null)
                    mHandler.obtainMessage(ModemConnectionFrag.MSG_CONNECTION_FAIL,
                            exceptReason).sendToTarget();
                return;
            }

            int retry = 0;
            boolean connected = false;
            while (!connected && retry < NUM_RETRIES) {
                try {
                    retry++;
                    mdmCtrl.connectToModem();
                    connected = true;
                } catch (ModemControlException ex) {
                    exceptReason = ex.getMessage();
                    Log.e(TAG, MODULE + ": Connection to modem, try "
                            + retry + " failed: " + exceptReason);
                    try {
                        Thread.currentThread().sleep(RETRY_DELAY_MS);
                    } catch (InterruptedException ie) {
                        Log.d(TAG, MODULE + ": Sleep interrupted: "
                                + ie.getMessage());
                    }
                }
            }

            if (mHandler != null) {
                if (connected) {
                    mHandler.obtainMessage(ModemConnectionFrag.MSG_CONNECTION_SUCCESS)
                            .sendToTarget();
                } else {
                    mdmCtrl = null;
                    mHandler.obtainMessage(ModemConnectionFrag.MSG_CONNECTION_FAIL,
                            exceptReason).sendToTarget();
                }
            }
        }
    }
}
