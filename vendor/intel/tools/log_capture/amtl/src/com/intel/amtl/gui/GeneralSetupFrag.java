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
import android.app.Fragment;
import android.app.FragmentManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.os.SystemProperties;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.LinearLayout;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.EditText;

import com.intel.amtl.AMTLApplication;
import com.intel.amtl.R;
import com.intel.amtl.exceptions.ModemConfException;
import com.intel.amtl.exceptions.ModemControlException;
import com.intel.amtl.helper.LogManager;
import com.intel.amtl.models.config.ModemConf;
import com.intel.amtl.models.config.LogOutput;
import com.intel.amtl.modem.CommandParser;
import com.intel.amtl.modem.ModemController;
import com.intel.amtl.mts.MtsManager;
import com.intel.internal.telephony.ModemStatus;

import java.io.File;
import java.io.IOException;
import java.lang.RuntimeException;
import java.util.ArrayList;


public class GeneralSetupFrag extends Fragment implements OnClickListener, OnCheckedChangeListener {

    private final String TAG = AMTLApplication.getAMTLApp().getLogTag();
    private final String MODULE = "GeneralSetupFrag";
    private final String EXPERT_PROPERTY = "persist.service.amtl.expert";
    private final String CONFSETUP_TAG = "AMTL_modem_configuration_setup";
    private final String SAVELOG_TAG = "AMTL_modem_log_backup";
    private final String BACKUP_LOG_PATH = "/data/amtl_logs_backup/";
    private final String AP_LOG_PATH = "/mnt/sdcard/logs/";
    private final String BP_LOG_PATH = "/logs/";

    private final int CONFSETUP_TARGETFRAG = 0;
    private final int SAVELOG_TARGETFRAG = 1;

    // Graphical objects for onClick handling.
    private Switch sExpertMode;
    private Button bAppConf;
    private Button bSavLog;
    private Button bLogTag;
    private TextView tvModemStatus;
    private TextView tvMtsStatus;
    private TextView tvOctStatus;

    private LinearLayout ll;

    private final int appConfId = 100;
    private final int savLogId = 200;
    private final int logTagId = 300;

    private ArrayList<LogOutput> configArray;

    private ModemConf modemConfToApply;
    private ModemConf currentModemConf;
    private ModemConf expertModemConf;

    private MtsManager mtsMgr;

    private Boolean buttonChanged;

    private Boolean firstCreated;

    private int PTI_KILL_WAIT = 1000;
    private Runtime rtm = java.lang.Runtime.getRuntime();

    // Target fragment for progress popup.
    private FragmentManager gsfManager;
    // Callback object pointer. Active only when associated to Main layout.
    private GSFCallBack gsfCb = nullCb;
    private OnModeChanged modeChangedCallBack = nullModeCb;

    // Used to not execute OnCheckedChanged()
    private Boolean isIgnore = false;

    // Callback interface for Toast messages in Main layout.
    public interface GSFCallBack {
        public void onConfigurationApplied(int resultCode);
    }

    public GeneralSetupFrag(ArrayList<LogOutput> outputArray) {
       this.configArray = outputArray;
    }

    // Callback interface to detect if a button has been pressed
    // since last reboot
    public interface OnModeChanged {
        public void onModeChanged(Boolean changed);
    }

    private static OnModeChanged nullModeCb = new OnModeChanged() {
        public void onModeChanged(Boolean changed) { }
    };

    public GeneralSetupFrag() {
    }

    private static GSFCallBack nullCb = new GSFCallBack() {
        public void onConfigurationApplied(int resultCode) { }
    };

    // Function used by AMTLTabLayout to provide to GeneralSetupFrag the Expert conf loaded
    public void setExpertConf(ModemConf conf) {
        this.expertModemConf = conf;
    }

    public void setButtonChanged(Boolean changed) {
        this.buttonChanged = changed;
    }

    private void updateText (String textToDisplay, TextView tv ) {
        if (tv != null) {
            if (textToDisplay.equals("")) {
                tv.setText("stopped");
            } else {
                tv.setText(textToDisplay);
            }
        }
    }

    private void setUIEnabled() {
        this.updateText("UP", tvModemStatus);
        // only enable conf switches if expert property is set to 1
        this.setSwitchesEnabled(!this.isExpertModeEnabled());
        if (this.sExpertMode != null) {
            this.sExpertMode.setEnabled(true);
        }
        if (this.bAppConf != null) {
            this.bAppConf.setEnabled(true);
        }
        this.changeButtonColor(this.buttonChanged);
        this.updateText(this.mtsMgr.getMtsState(), tvMtsStatus);
    }

    private void setUIDisabled() {
        this.updateText("DOWN", tvModemStatus);
        this.setSwitchesEnabled(false);
        if (this.sExpertMode != null) {
            this.sExpertMode.setEnabled(false);
        }
        if(this.bAppConf != null) {
            this.bAppConf.setEnabled(false);
        }
        this.changeButtonColor(false);
        this.updateText(this.mtsMgr.getMtsState(), tvMtsStatus);
    }

    private void changeButtonColor(boolean changed) {
        if (this.bAppConf != null) {
            if (changed) {
                this.bAppConf.setEnabled(true);
                this.bAppConf.setBackgroundColor(Color.BLUE);
                this.bAppConf.setTextColor(Color.WHITE);
            } else {
                this.bAppConf.setEnabled(false);
                this.bAppConf.setBackgroundColor(Color.LTGRAY);
                this.bAppConf.setTextColor(Color.BLACK);
            }
        }
    }

    private void updateUi(ModemConf curModConf) {
        SharedPreferences prefs = this.getActivity().getSharedPreferences("AMTLPrefsData",
                Context.MODE_PRIVATE);
        int id = prefs.getInt("index", -2);
        Log.d(TAG, MODULE + ": Current index = " + id);
        if (id >= 0) {
            if (!curModConf.confTraceEnable()) {
                if (mtsMgr.getMtsState().equals("running")) {
                    Log.e(TAG, MODULE + ": stopping mts running wrongly");
                    mtsMgr.stopServices();
                }
                Log.d(TAG, MODULE + ": reinit switches");
                this.setSwitchesChecked(false);
            } else {
                for (LogOutput o: configArray) {
                    o.getConfSwitch().setChecked(configArray.indexOf(o) == id);
                }
            }
        } else {
            if ((!curModConf.confTraceEnable()) && mtsMgr.getMtsState().equals("running")) {
                mtsMgr.stopServices();
            }
            this.updateText(this.mtsMgr.getMtsState(), tvMtsStatus);
            if (this.isExpertModeEnabled() && (curModConf.confTraceEnable())) {
                if (curModConf.isMtsRequired() && mtsMgr.getMtsState().equals("running")) {
                    this.sExpertMode.setChecked(true);
                } else if (!curModConf.isMtsRequired()) {
                    this.sExpertMode.setChecked(true);
                }
            } else {
                SystemProperties.set(EXPERT_PROPERTY, "0");
                this.sExpertMode.setChecked(false);
                // if traces are not enabled or mts not started, uncheck all the conf switches
                if ((!curModConf.confTraceEnable()) ||
                        (!mtsMgr.getMtsState().equals("running")
                        && curModConf.isMtsRequired())) {
                    this.setSwitchesChecked(false);
                // if traces are enabled, check the corresponding conf switch
                } else {
                    if (this.configArray != null) {
                        for (LogOutput o: configArray) {
                            if (o != null
                                    && o.getConfSwitch() != null
                                    && o.getXsio() != null
                                    && curModConf != null
                                    && curModConf.getXsio() != null
                                    && o.getMtsOutput() != null
                                    && curModConf.getMtsConf() != null
                                    && curModConf.getMtsConf().getOutput() != null) {
                                o.getConfSwitch().setChecked(
                                    o.getXsio().equals(curModConf.getXsio())
                                    && o.getMtsOutput().equals(
                                        curModConf.getMtsConf().getOutput()));
                            }
                        }
                    }
                }
            }
        }
        this.updateText(curModConf.getOctMode(), tvOctStatus);
        this.updateText(this.mtsMgr.getMtsState(), tvMtsStatus);
    }

    public void setFirstCreated(boolean created) {
        this.firstCreated = created;
    }

    // retrieve the property value to know if expert mode is set
    private boolean isExpertModeEnabled() {
        boolean ret = false;
        String expertMode = SystemProperties.get(EXPERT_PROPERTY, null);
        if (expertMode != null) {
            if (expertMode.equals("1")) {
                ret = true;
            }
        }
        return ret;
    }

    private void setSwitchesEnabled(boolean enabled) {
        if (this.configArray != null) {
            for (LogOutput o: configArray) {
                o.getConfSwitch().setEnabled(enabled);
            }
        }
    }

    private void setSwitchesChecked(boolean checked) {
        if (this.configArray != null) {
            for (LogOutput o: configArray) {
                o.getConfSwitch().setChecked(checked);
            }
        }
    }

    // Function overrides for the Fragment instance.
    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        gsfCb = (GSFCallBack) activity;
        modeChangedCallBack = (OnModeChanged) activity;
    }

    @Override
    public void onDetach() {
        super.onDetach();
        gsfCb = nullCb;
        modeChangedCallBack = nullModeCb;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        gsfManager = getFragmentManager();

        ConfigApplyFrag appConf = (ConfigApplyFrag)gsfManager.findFragmentByTag(CONFSETUP_TAG);
        SaveLogFrag savLog = (SaveLogFrag)gsfManager.findFragmentByTag(SAVELOG_TAG);

        if (appConf != null) {
            appConf.setTargetFragment(this, CONFSETUP_TARGETFRAG);
        }
        if (savLog != null) {
            savLog.setTargetFragment(this, SAVELOG_TARGETFRAG);
        }

        String event = AMTLApplication.getAMTLApp().getModemEvent();
        this.getActivity().registerReceiver(mMessageReceiver, new IntentFilter(event));
        mtsMgr = new MtsManager();
    }

    @Override
    public void onResume() {
        super.onResume();
        if (!firstCreated) {
            // update modem status when returning from another fragment
            if (ModemController.getModemStatus() == ModemStatus.UP) {
                try {
                    ModemController mdmCtrl = ModemController.getInstance();
                    this.currentModemConf = checkModemConfig(mdmCtrl);
                    this.mtsMgr.printMtsProperties();
                    this.updateUi(this.currentModemConf);
                    this.setUIEnabled();
                    mdmCtrl = null;
                } catch (ModemControlException ex) {
                    Log.e(TAG, MODULE + ": fail to send command to the modem " + ex);
                }
            } else if (ModemController.getModemStatus() == ModemStatus.DOWN) {
                this.setUIDisabled();
            }
        }
    }

    @Override
    public void onDestroy() {
        this.getActivity().unregisterReceiver(mMessageReceiver);
        super.onDestroy();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {

        View view = inflater.inflate(R.layout.generalsetupfraglayout, container, false);
        if (view != null) {

            this.sExpertMode = (Switch) view.findViewById(R.id.switchExpertMode);
            this.tvModemStatus = (TextView) view.findViewById(R.id.modemStatusValueTxt);
            this.tvMtsStatus = (TextView) view.findViewById(R.id.mtsStatusValueTxt);
            this.tvOctStatus = (TextView) view.findViewById(R.id.octStatusValueTxt);
            this.ll = (LinearLayout) view.findViewById(R.id.generalsetupfraglayout);

            // definition of switch listeners
            if (this.configArray != null) {
                for (LogOutput o: configArray) {
                    o.setConfigSwitch(ll, configArray.lastIndexOf(o), this.getActivity(), view);
                }
            }

            this.defineExecConfButton(view);
            this.defineSaveLogsButton(view);
            this.defineLogTagButton(view);
        } else {

            UIHelper.exitDialog(this.getActivity(), "Error in UI", "View cannot be displayed.\n"
                    + "AMTL will exit.");
            Log.e(TAG, MODULE + ": context or view are null, AMTL will exit");
            // This is a UI bug - AMTL will end
        }

        return view;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (view != null) {

            if (this.configArray != null) {
                for (LogOutput o: configArray) {
                    if (view != null) {
                        ((Switch) view.findViewById(o.getSwitchId()))
                                .setOnCheckedChangeListener(this);
                    }
                }
            }
            if (this.sExpertMode != null) {
                this.sExpertMode.setOnCheckedChangeListener(this);
            }
            if (this.bAppConf != null) {
                this.bAppConf.setOnClickListener(this);
            }
            if (this.bSavLog != null) {
                this.bSavLog.setOnClickListener(this);
            }
            if (this.bLogTag != null) {
                this.bLogTag.setOnClickListener(this);
            }
        }
    }

    private BroadcastReceiver mMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // Get extra data included in the Intent
            String message = intent.getStringExtra("message");
            if (message != null) {
                if (message.equals("UP")) {
                    try {
                        ModemController mdmCtrl = ModemController.getInstance();
                        currentModemConf = checkModemConfig(mdmCtrl);
                        mtsMgr.printMtsProperties();
                        updateUi(currentModemConf);
                        setUIEnabled();
                        if (AMTLApplication.getPauseState()) {
                            // Application on Pause => close gsmtty
                            mdmCtrl.close();
                        }
                        mdmCtrl = null;
                    } catch (ModemControlException ex) {
                        Log.e(TAG, MODULE + ": fail to send command to the modem " + ex);
                    }
                    AMTLApplication.setCloseTtyEnable(true);
                } else if (message.equals("DOWN")) {
                    setUIDisabled();
                }
            }
        }
    };

    private ModemConf checkModemConfig(ModemController mdmCtrl) throws ModemControlException {
        String atXsioResponse;
        String atXsystraceResponse;
        String octMode;
        String atTraceResponse = "";
        ModemConf retModemConf = null;
        CommandParser cmdParser = new CommandParser();

        atXsioResponse = mdmCtrl.checkAtXsioState();
        atTraceResponse = mdmCtrl.checkAtTraceState();
        atXsystraceResponse = mdmCtrl.checkAtXsystraceState();

        octMode = mdmCtrl.checkOct();
        Bundle bundle = new Bundle();
        bundle.putString(ModemConf.KEY_XSIO, atXsioResponse);
        bundle.putString(ModemConf.KEY_TRACE, atTraceResponse);
        bundle.putString(ModemConf.KEY_XSYSTRACE, atXsystraceResponse);
        bundle.putString(ModemConf.KEY_FLCMD, "");
        bundle.putString(ModemConf.KEY_OCTMODE, octMode);

        try {
            retModemConf = ModemConf.getInstance(bundle);
        } catch (ModemConfException e) {
            Log.e(TAG, MODULE + ": Create the modconf error.");
        }
        return retModemConf;
    }

    public void defineExecConfButton(View view) {
        LinearLayout.LayoutParams appExecConf = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT);
        appExecConf.setMargins(0, 100, 0, 0);
        this.bAppConf = new Button(this.getActivity());
        this.bAppConf.setId(this.appConfId);
        this.bAppConf.setGravity(Gravity.CENTER);
        this.bAppConf.setTextColor(Color.BLACK);
        this.bAppConf.setText("Execute configuration");
        ll.addView(this.bAppConf, appExecConf);
        this.bAppConf = ((Button)view.findViewById(this.appConfId));
    }

    public void defineSaveLogsButton(View view) {
        LinearLayout.LayoutParams appSaveLogs = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT);
        appSaveLogs.setMargins(0, 50, 0, 0);
        this.bSavLog = new Button(this.getActivity());
        this.bSavLog.setId(this.savLogId);
        this.bSavLog.setGravity(Gravity.CENTER);
        this.bSavLog.setTextColor(Color.BLACK);
        this.bSavLog.setText("Save Logs");
        ll.addView(this.bSavLog, appSaveLogs);
        this.bSavLog = ((Button)view.findViewById(this.savLogId));
    }

    public void defineLogTagButton(View view) {
        LinearLayout.LayoutParams appLogTag = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT);
        appLogTag.setMargins(0, 50, 0, 0);
        this.bLogTag = new Button(this.getActivity());
        this.bLogTag.setId(this.logTagId);
        this.bLogTag.setGravity(Gravity.CENTER);
        this.bLogTag.setTextColor(Color.BLACK);
        this.bLogTag.setText("Inject TAG in logs");
        ll.addView(this.bLogTag, appLogTag);
        this.bLogTag = ((Button)view.findViewById(this.logTagId));
    }

    public void saveLogs() {

        // Create root backup folder
        File file = new File(BACKUP_LOG_PATH);
        if (!file.exists() && (!file.mkdirs())) {
            UIHelper.okDialog(this.getActivity(), "Error", "Unable to create"
                    + " log backup folder: " + BACKUP_LOG_PATH);
            bAppConf.setEnabled(true);
            return;
        }

        LogManager snaplog = new LogManager(BACKUP_LOG_PATH, AP_LOG_PATH, BP_LOG_PATH);
        if (snaplog == null) {
            UIHelper.okDialog(this.getActivity(), "Error", "The backup path doesn't exist");
            this.bAppConf.setEnabled(true);
            return;
        }

        SaveLogFrag logProgressFrag = new SaveLogFrag(gsfManager, SAVELOG_TAG,
                SAVELOG_TARGETFRAG);

        UIHelper.savePopupDialog(this.getActivity(), "Log Backup", "Please"
                + " select your backup tag, it will be located in:\n" + BACKUP_LOG_PATH,
                this.getActivity(), snaplog, logProgressFrag);
    }

    public void applyConf() {
        SharedPreferences prefs = this.getActivity().getSharedPreferences("AMTLPrefsData",
                Context.MODE_PRIVATE);
        /* Determine current index */
        int id = prefs.getInt("index", -2);

        if (!this.sExpertMode.isChecked()) {
            SystemProperties.set(EXPERT_PROPERTY, "0");
            if (this.configArray != null) {
                boolean buttonChecked = false;
                for (LogOutput o: configArray) {
                    if (o.getConfSwitch().isChecked()) {
                        try {
                            modemConfToApply = ModemConf.getInstance(o);
                        } catch (ModemConfException e){
                            Log.e(TAG, MODULE + ":Create the modemConfToApply error.");
                        }
                        modemConfToApply.activateConf(true);
                        buttonChecked = true;
                    }
                }
                if (!buttonChecked) {
                    Bundle bundle = new Bundle();
                    bundle.putString(ModemConf.KEY_XSIO, "");
                    bundle.putString(ModemConf.KEY_TRACE, "AT+TRACE=0\r\n");
                    bundle.putString(ModemConf.KEY_XSYSTRACE, "AT+XSYSTRACE=0\r\n");
                    bundle.putString(ModemConf.KEY_OCTMODE, "");
                    bundle.putString(ModemConf.KEY_FLCMD, "");
                    try {
                        modemConfToApply = ModemConf.getInstance(bundle);
                    } catch (ModemConfException e) {
                        Log.e(TAG, MODULE + ":Create the modemConfToApply error.");
                    }
                }
            }
        } else {
            if (this.expertModemConf != null) {
                this.modemConfToApply = this.expertModemConf;
                SystemProperties.set(EXPERT_PROPERTY, "1");
            } else {
                if (this.sExpertMode.isChecked()) {
                    UIHelper.okDialog(this.getActivity(), "Warning", "There is no Expert"
                            + " configuration saved, please go to Expert Options");
                    this.sExpertMode.performClick();
                    return;
                }
            }
        }

        /* Determine if pti is activated */
        if (id >= 0) {
            if (configArray.get(id).getSigusr1ToSend().equalsIgnoreCase("true")) {
                /* pti is activated => need to send sigusr1 signal */
                try {
                    Log.d(TAG, MODULE + ": send SIGUSR1 signal");
                    rtm.exec("start pti_sigusr1");
                    android.os.SystemClock.sleep(PTI_KILL_WAIT);
                } catch (IOException e) {
                    Log.e(TAG, MODULE + ": can't send sigusr1 signal");
                }
            }
        }

        ConfigApplyFrag progressFrag = ConfigApplyFrag.newInstance(CONFSETUP_TAG,
                CONFSETUP_TARGETFRAG);
        progressFrag.launch(modemConfToApply, this, gsfManager);
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        if (this.isIgnore) {
            /* Ignore this callback */
            this.isIgnore = false;
        } else {
            SharedPreferences prefs = this.getActivity().getSharedPreferences("AMTLPrefsData",
                    Context.MODE_PRIVATE);
            /* Determine current index */
            int currentIndex = prefs.getInt("index", -2);

            this.updateText(this.mtsMgr.getMtsState(), tvMtsStatus);
            /* set config buttons exclusive by retrieving the id
             * of the button checked and disabling all the others */
            if (this.configArray != null) {
                int idChecked = -1;
                for (LogOutput o: configArray) {
                    if (buttonView.getId() == o.getSwitchId()) {
                        this.buttonChanged = true;
                        modeChangedCallBack.onModeChanged(this.buttonChanged);
                        if (isChecked) {
                            idChecked = o.getSwitchId();
                        }
                        if (currentIndex == idChecked) {
                            /* Same configuration than actual one */
                            /* => Deactivate "Execute Configuration" button */
                            this.changeButtonColor(false);
                        } else {
                            /* New configuration */
                            /* => Activate "Execute Configuration" button */
                            this.changeButtonColor(true);
                        }
                    }
                }

                if (idChecked != -1) {
                    for (LogOutput o: configArray) {
                        if ((o.getSwitchId() != idChecked) && (o.getConfSwitch().isChecked())) {
                            /* We must ignore the next call to onCheckedChanged because
                            /* it will be due to te modification of this switch */
                            this.isIgnore = true;
                            o.getConfSwitch().setChecked(false);
                            break;
                        }
                    }
                }
            }
        }

        switch (buttonView.getId()) {
            case R.id.switchExpertMode:
                this.buttonChanged = true;
                modeChangedCallBack.onModeChanged(this.buttonChanged);
                this.changeButtonColor(this.buttonChanged);
                this.setSwitchesEnabled(!isChecked);
                if (isChecked) {
                    this.setSwitchesChecked(false);
                }
                break;
        }
    }

    @Override
    public void onClick(View view) {
        this.updateText(this.mtsMgr.getMtsState(), tvMtsStatus);

        switch (view.getId()) {
            case savLogId:
                this.bAppConf.setEnabled(false);
                this.saveLogs();
                this.bAppConf.setEnabled(true);
                break;
            case appConfId:
                this.applyConf();
                this.buttonChanged = false;
                modeChangedCallBack.onModeChanged(this.buttonChanged);
                this.changeButtonColor(this.buttonChanged);
                break;
            case logTagId:
                UIHelper.logTagDialog(this.getActivity(), "Log TAG", "Please select the TAG"
                        + " you want to set in logs:\n", this.getActivity());
                break;
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == CONFSETUP_TARGETFRAG) {
            // Inform the Main layout - in charge to display Toast messages.
            gsfCb.onConfigurationApplied(resultCode);
        }
    }
}
