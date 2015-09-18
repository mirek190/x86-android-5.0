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
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler.Callback;
import android.os.Message;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.amtl.AMTLApplication;
import com.intel.amtl.R;
import com.intel.amtl.config_parser.ConfigParser;
import com.intel.amtl.gui.UIHelper;
import com.intel.amtl.models.config.ModemConf;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.List;

import org.xmlpull.v1.XmlPullParserException;

public class ExpertSetupFrag extends Fragment implements OnClickListener {

    private final String TAG = AMTLApplication.getAMTLApp().getLogTag();
    private final String MODULE = "ExpertSetupFrag";

    private String confPath = "/etc/amtl/catalog/default_expert.cfg";

    private Button buttonChoose = null;
    private EditText editTextExpertConf = null;
    private TextView textViewTrace = null;
    private TextView textViewXsystrace = null;
    private TextView textViewXsio = null;

    private ConfigParser modemConfParser = null;
    private ModemConf selectedConf = null;
    private File chosenFile = null;

    private OnExpertModeApplied expertModemCallBack = nullCb;
    private OnModeChanged modeChangedCallBack = nullModeCb;

    // Callback interface to give the expert config loaded to Main layout
    public interface OnExpertModeApplied {
        public void onExpertConfApplied(ModemConf mod);
    }

    private static OnExpertModeApplied nullCb = new OnExpertModeApplied() {
        public void onExpertConfApplied(ModemConf mod) { }
    };

    // Callback interface to detect if a button has been pressed
    // since last reboot
    public interface OnModeChanged {
        public void onModeChanged(Boolean changed);
    }

    private static OnModeChanged nullModeCb = new OnModeChanged() {
        public void onModeChanged(Boolean changed) { }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {

        View view = inflater.inflate(R.layout.expertsetupfraglayout, container, false);

        this.buttonChoose = (Button)view.findViewById(R.id.buttonChoose);
        this.editTextExpertConf = (EditText)view.findViewById(R.id.editTextConfPath);
        this.textViewTrace = (TextView)view.findViewById(R.id.textViewTrace);
        this.textViewXsystrace = (TextView)view.findViewById(R.id.textViewXsystrace);
        this.textViewXsio = (TextView)view.findViewById(R.id.textViewXsio);

        return view;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (this.buttonChoose != null) {
            this.buttonChoose.setOnClickListener(this);
        }
        if (this.editTextExpertConf != null) {
            this.editTextExpertConf.setText(this.confPath);
        }
        this.modemConfParser = new ConfigParser();
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        expertModemCallBack = (OnExpertModeApplied) activity;
        modeChangedCallBack = (OnModeChanged) activity;
    }

    @Override
    public void onDetach() {
        super.onDetach();
        expertModemCallBack = nullCb;
        modeChangedCallBack = nullModeCb;
    }

    private File checkConfPath(String path) {
        File ret = null;
        File file = new File(path);
        if (file.exists()) {
            ret = file;
            Log.d(TAG, MODULE + ": file: " + path + " has been chosen");
        } else {
            UIHelper.okDialog(this.getActivity(),
                    "Error", "file: " + path + " has not been found");
            Log.e(TAG, MODULE + ": file: " + path + " has not been found");
        }
        return ret;
    }

    private ModemConf applyConf(File conf) {
        ModemConf ret = null;
        FileInputStream fin = null;
        if (conf != null) {
            try {
                fin = new FileInputStream(conf);
                if (fin != null) {
                    ret = modemConfParser.parseShortConfig(fin);
                    expertModemCallBack.onExpertConfApplied(ret);
                }
            } catch (FileNotFoundException ex) {
                Log.e(TAG, MODULE + ": file " + conf + " has not been found");
                UIHelper.okDialog(this.getActivity(), "Error", ex.toString());
            } catch (XmlPullParserException ex) {
                Log.e(TAG, MODULE + ": an issue occured during parsing " + ex);
                UIHelper.okDialog(this.getActivity(), "Error", ex.toString());
            } catch (IOException ex) {
                Log.e(TAG, MODULE + ": an issue occured during parsing " + ex);
                UIHelper.okDialog(this.getActivity(), "Error", ex.toString());
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
        return ret;
    }

    private void showConfDetails(boolean display, ModemConf modConf) {
        if (display && modConf != null) {
            this.textViewTrace.setText(modConf.getTrace());
            this.textViewXsystrace.setText(modConf.getXsystrace());
            this.textViewXsio.setText(modConf.getXsio());
        } else {
            this.textViewTrace.setText("");
            this.textViewXsystrace.setText("");
            this.textViewXsio.setText("");
        }
    }

    public void onClick(View v) {
        modeChangedCallBack.onModeChanged(true);
        switch (v.getId()) {
            case R.id.buttonChoose:
                // Hide keyboard.
                InputMethodManager imm = (InputMethodManager)getActivity()
                        .getSystemService(Context.INPUT_METHOD_SERVICE);
                if (imm != null) {
                    imm.hideSoftInputFromWindow(editTextExpertConf.getWindowToken(), 0);
                }
                this.showConfDetails(false, null);
                this.confPath = this.editTextExpertConf.getText().toString();
                this.chosenFile = this.checkConfPath(this.confPath);
                if (this.chosenFile != null && !this.chosenFile.isDirectory()) {
                    this.selectedConf = this.applyConf(this.chosenFile);
                    this.showConfDetails(true, this.selectedConf);
                    Toast.makeText(this.getActivity(), "Configuration saved, please go back to "
                            + "General Setup, switch Expert Mode on and Execute configuration.",
                            Toast.LENGTH_LONG).show();
                }
                break;
        }
    }
}
