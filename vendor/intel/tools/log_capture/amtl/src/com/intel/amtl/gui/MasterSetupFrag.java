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
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.Spinner;

import com.intel.amtl.AMTLApplication;
import com.intel.amtl.R;
import com.intel.amtl.exceptions.ModemConfException;
import com.intel.amtl.exceptions.ModemControlException;
import com.intel.amtl.models.config.Master;
import com.intel.amtl.models.config.ModemConf;
import com.intel.amtl.models.config.LogOutput;
import com.intel.amtl.modem.ModemController;
import com.intel.internal.telephony.ModemStatus;

import java.util.ArrayList;


public class MasterSetupFrag extends Fragment
        implements OnClickListener, AdapterView.OnItemSelectedListener {

    private final String TAG = AMTLApplication.getAMTLApp().getLogTag();
    private final String MODULE = "MasterSetupFrag";

    static final int CONFSETUP_MASTER_TARGETFRAG = 1;
    static final String CONFSETUP_MASTER_TAG = "AMTL_master_modem_configuration_setup";

    private String[] masterValues = {"OFF", "OCT", "MIPI2"};
    private String[] masterNames = {"bb_sw", "3g_sw", "digrf", "digrfx", "lte_l1_sw", "3g_dsp"};

    // Graphical objects for onClick handling.
    private Spinner spinnerBbSw;
    private Spinner spinner3gSw;
    private Spinner spinnerDigrf;
    private Spinner spinnerDigrfx;
    private Spinner spinnerLteL1Sw;
    private Spinner spinner3gDsp;
    private Button bAppMasterConf;

    // Target fragment for progress popup.
    private FragmentManager gsfManager;

    private ArrayList<Master> masterArray = null;
    private ArrayList<Spinner> spinnerArray = null;

    private Boolean buttonChanged;

    private OnModeChanged modeChangedCallBack = nullModeCb;

    //this counts how many spinners are on the UI
    private int spinnerCount = 6;

    //this counts how many spinners have been initialized
    private int spinnerInitializedCount = 0;

    // Callback interface to detect if a button has been pressed
    // since last reboot
    public interface OnModeChanged {
        public void onModeChanged(Boolean changed);
    }

    private static OnModeChanged nullModeCb = new OnModeChanged() {
        public void onModeChanged(Boolean changed) { }
    };

    public void setButtonChanged(Boolean changed) {
        this.buttonChanged = changed;
    }

    private void setUIEnabled() {
        this.spinnerBbSw.setEnabled(true);
        this.spinner3gSw.setEnabled(true);
        this.spinnerDigrf.setEnabled(true);
        this.spinnerDigrfx.setEnabled(true);
        this.spinnerLteL1Sw.setEnabled(true);
        this.spinner3gDsp.setEnabled(true);
        this.bAppMasterConf.setEnabled(true);
        this.changeButtonColor(this.buttonChanged);
    }

    private void setUIDisabled() {
        this.spinnerBbSw.setEnabled(false);
        this.spinner3gSw.setEnabled(false);
        this.spinnerDigrf.setEnabled(false);
        this.spinnerDigrfx.setEnabled(false);
        this.spinnerLteL1Sw.setEnabled(false);
        this.spinner3gDsp.setEnabled(false);
        this.bAppMasterConf.setEnabled(false);
        this.changeButtonColor(false);
    }

    private void updateUi() {
        if (this.masterArray != null) {
            for (Master m: masterArray) {
                Spinner spinToSet = this.spinnerArray.get(this.masterArray.lastIndexOf(m));
                ArrayAdapter aAdapt = (ArrayAdapter) spinToSet.getAdapter();
                spinToSet.setSelection(aAdapt.getPosition(m.getDefaultPort()));
            }
        }
        this.changeButtonColor(this.buttonChanged);
    }

    private void changeButtonColor(boolean changed) {
        if (changed) {
            this.bAppMasterConf.setBackgroundColor(Color.BLUE);
            this.bAppMasterConf.setTextColor(Color.WHITE);
        } else {
            this.bAppMasterConf.setBackgroundColor(Color.LTGRAY);
            this.bAppMasterConf.setTextColor(Color.BLACK);
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        String event = AMTLApplication.getAMTLApp().getModemEvent();
        this.getActivity().registerReceiver(mMessageReceiver, new IntentFilter(event));
        gsfManager = getFragmentManager();

        ConfigApplyFrag appMasterConf
                = (ConfigApplyFrag)gsfManager.findFragmentByTag(CONFSETUP_MASTER_TAG);

        if (appMasterConf != null) {
            appMasterConf.setTargetFragment(this, CONFSETUP_MASTER_TARGETFRAG);
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

        View view = inflater.inflate(R.layout.mastersetuplayout, container, false);

        bAppMasterConf = (Button) view.findViewById(R.id.applyMasterConfButton);
        spinnerBbSw = (Spinner) view.findViewById(R.id.spinner_bb_sw);
        spinner3gSw = (Spinner) view.findViewById(R.id.spinner_3g_sw);
        spinnerDigrf = (Spinner) view.findViewById(R.id.spinner_digrf);
        spinnerDigrfx = (Spinner) view.findViewById(R.id.spinner_digrfx);
        spinnerLteL1Sw = (Spinner) view.findViewById(R.id.spinner_lte_l1_sw);
        spinner3gDsp = (Spinner) view.findViewById(R.id.spinner_3g_dsp);

        this.spinnerArray = new ArrayList<Spinner>();
        this.spinnerArray.add(spinnerBbSw);
        this.spinnerArray.add(spinner3gSw);
        this.spinnerArray.add(spinnerDigrf);
        this.spinnerArray.add(spinnerDigrfx);
        this.spinnerArray.add(spinnerLteL1Sw);
        this.spinnerArray.add(spinner3gDsp);

        ArrayAdapter<String> masterAdapter = new ArrayAdapter<String>(this.getActivity(),
                android.R.layout.simple_spinner_item, masterValues);
        masterAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

        if (this.spinnerArray != null) {
            for (Spinner s: spinnerArray) {
                s.setAdapter(masterAdapter);
            }
        }
        return view;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (spinnerArray != null) {
            for (Spinner s: spinnerArray) {
                s.setOnItemSelectedListener(this);
            }
        }
        if (this.bAppMasterConf != null) {
            this.bAppMasterConf.setOnClickListener(this);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        // update modem status when returning from another fragment
        if (ModemController.getModemStatus() == ModemStatus.UP) {
            try {
                ModemController mdmCtrl = ModemController.getInstance();
                this.masterArray = new ArrayList<Master>();
                for (String s: masterNames) {
                    this.masterArray.add(new Master(s, "", ""));
                }
                this.masterArray = mdmCtrl.checkAtXsystraceState(this.masterArray);
                this.updateUi();
                if (mdmCtrl.queryTraceState()) {
                    this.setUIEnabled();
                } else {
                    this.setUIDisabled();
                }
                mdmCtrl = null;
            } catch (ModemControlException ex) {
                Log.e(TAG, MODULE + ": cannot send command to the modem");
            }
        } else if (ModemController.getModemStatus() == ModemStatus.DOWN) {
             this.setUIDisabled();
        }
    }

    @Override
    public void onPause() {
        this.spinnerCount = 8;
        this.spinnerInitializedCount = 0;
        super.onPause();
    }

    public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
        if (spinnerInitializedCount < spinnerCount) {
            spinnerInitializedCount++;
        } else {
            this.buttonChanged = true;
            modeChangedCallBack.onModeChanged(this.buttonChanged);
            changeButtonColor(this.buttonChanged);
        }
    }

    public void onNothingSelected(AdapterView<?> parent) {
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.applyMasterConfButton:
                this.buttonChanged = false;
                modeChangedCallBack.onModeChanged(this.buttonChanged);
                this.changeButtonColor(this.buttonChanged);
                setChosenMasterValues();
                setMasterStringToInt();
                ModemConf sysConf = setModemConf();
                ConfigApplyFrag progressFrag = ConfigApplyFrag.newInstance(CONFSETUP_MASTER_TAG,
                        CONFSETUP_MASTER_TARGETFRAG);
                progressFrag.launch(sysConf, this, gsfManager);
                break;
        }
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        modeChangedCallBack = (OnModeChanged) activity;
    }

    @Override
    public void onDetach() {
        super.onDetach();
        modeChangedCallBack = nullModeCb;
    }

    private ModemConf setModemConf() {
        SharedPreferences prefs = this.getActivity().getSharedPreferences("AMTLPrefsData",
                Context.MODE_PRIVATE);
        LogOutput output = new LogOutput();
        output.setIndex(prefs.getInt("index", -2));
        if (masterArray != null) {
            for (Master m: masterArray) {
                output.addMasterToList(m.getName(), m);
            }
        }
        ModemConf modConf = null;
        try {
            modConf = ModemConf.getInstance(output);
        } catch (ModemConfException e) {
            Log.e(TAG, MODULE + ":Create the modConf error.");
        }
        return modConf;
    }

    // Set the master port values according to the current spinner value
    private void setChosenMasterValues() {
        if (spinnerArray != null) {
            for (Spinner s: spinnerArray) {
                masterArray.get(spinnerArray.lastIndexOf(s))
                        .setDefaultPort((String)s.getSelectedItem());
            }
        }
    }

    // Set the master values from string to int
    private void setMasterStringToInt() {
        if (this.masterArray != null) {
            for (Master m: masterArray) {
                int portInt = convertMasterStringToInt(m.getDefaultPort());
                if (portInt != -1) {
                    m.setDefaultPort(String.valueOf(portInt));
                }
            }
        }
    }

    // Convert the master port from strings displayed in the spinners to the corresponding values
    // to send through xsystrace command
    private int convertMasterStringToInt(String masterString) {
        int masterInt = -1;
        if (masterString.equals("OFF")) {
            masterInt = 0;
        } else if (masterString.equals("OCT")) {
            masterInt = 1;
        } else if (masterString.equals("MIPI2")) {
            masterInt = 4;
        }
        return masterInt;
    }

    private BroadcastReceiver mMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // Get extra data included in the Intent
            String message = intent.getStringExtra("message");
            if (message != null && message.equals("UP")) {
                try {
                    ModemController mdmCtrl = ModemController.getInstance();
                    if (!mdmCtrl.queryTraceState()) {
                        setUIDisabled();
                    } else {
                        masterArray = mdmCtrl.checkAtXsystraceState(masterArray);
                        updateUi();
                        setUIEnabled();
                    }
                    mdmCtrl = null;
                } catch (ModemControlException ex) {
                    Log.e(TAG, MODULE + ": cannot send command to the modem");
                }
            } else if (message != null && message.equals("DOWN")) {
                setUIDisabled();
            }
            // In case message is null, no action to take. This is an
            // undefined behavior
        }
    };
}
