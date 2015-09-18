/* Android Modem NVM Configuration Tool
 *
 * Copyright (C) Intel 2012
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
 * Author: Edward Marmounier <edwardx.marmounier@intel.com>
 */

package com.intel.android.nvmconfig.activities;

import java.io.File;
import java.io.IOException;
import java.util.List;

import com.intel.android.nvmconfig.R;
import com.intel.android.nvmconfig.adapters.VariantAdapter;
import com.intel.android.nvmconfig.config.VariantParser;
import com.intel.android.nvmconfig.controllers.AsyncCommandExecutor;
import com.intel.android.nvmconfig.controllers.AsyncCommandExecutor.AsyncTaskEventListener;
import com.intel.android.nvmconfig.controllers.ModemController;
import com.intel.android.nvmconfig.controllers.medfield.MedfieldModemController;
import com.intel.android.nvmconfig.helpers.CommandHelper;
import com.intel.android.nvmconfig.helpers.MessageBoxHelper;
import com.intel.android.nvmconfig.helpers.SettingsHelper;
import com.intel.android.nvmconfig.models.config.Variant;
import com.intel.android.nvmconfig.models.modem.ModemControlCommandList;
import com.intel.android.nvmconfig.models.modem.ModemControlResponseList;
import com.intel.android.nvmconfig.models.modem.at.ATControlCommand;
import com.intel.android.nvmconfig.models.modem.at.ATControlResponse;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Message;
import android.os.Handler.Callback;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;

public class MainActivity extends Activity implements Callback, OnClickListener, AsyncTaskEventListener<ATControlResponse> {

    private Spinner spinnerVariants = null;
    private TextView textViewVariantDetails = null;
    private Button buttonApply = null;
    private Button buttonShow = null;
    private Button buttonSetPath = null;
    private TextView textViewLogs = null;
    private EditText editTextVariantPath = null;
    private ScrollView scrollViewLog = null;
    private ProgressBar progressBar = null;

    @SuppressWarnings("rawtypes")
    private ModemController modemController = null;
    private VariantParser variantParser = null;
    private AsyncCommandExecutor<ATControlCommand, ATControlResponse> executor = null;

    private String currentVariantPath = "/";
    private List<Variant> loadedVariants = null;
    private Variant selectedVariant = null;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        super.requestWindowFeature(Window.FEATURE_CUSTOM_TITLE);

        super.setContentView(R.layout.main);

        if (super.getWindow() != null) {
            super.getWindow().setFeatureInt(Window.FEATURE_CUSTOM_TITLE, R.layout.title_bar);
        }

        this.spinnerVariants = (Spinner)super.findViewById(R.id.spinnerVariants);
        this.buttonApply = (Button)super.findViewById(R.id.buttonApply);
        this.buttonShow = (Button)super.findViewById(R.id.buttonVariantDetails);
        this.buttonSetPath = (Button)super.findViewById(R.id.buttonSetPath);
        this.textViewVariantDetails = (TextView)super.findViewById(R.id.textViewVariantDetails);
        this.textViewLogs = (TextView)super.findViewById(R.id.textViewLog);
        this.editTextVariantPath = (EditText)super.findViewById(R.id.editTextVariantsPath);
        this.scrollViewLog = (ScrollView)super.findViewById(R.id.scrollViewLog);
        this.progressBar = (ProgressBar)super.findViewById(R.id.progressBar);

        if (this.buttonShow != null) {
            this.buttonShow.setOnClickListener(this);
        }
        if (this.buttonApply != null) {
            this.buttonApply.setOnClickListener(this);
        }
        if (this.buttonSetPath != null) {
            this.buttonSetPath.setOnClickListener(this);
        }

        if (this.spinnerVariants != null) {
            this.spinnerVariants.setOnItemSelectedListener(new OnItemSelectedListener() {

                public void onItemSelected(AdapterView<?> adapterView, View view, int position, long id) {
                    MainActivity.this.selectedVariant = MainActivity.this.loadedVariants.get(position);

                    MainActivity.this.textViewVariantDetails.setText(selectedVariant.getDescription());

                    MainActivity.this.scrollViewLog.setVisibility(selectedVariant.isVerbose() ? View.VISIBLE : View.GONE);
                }

                public void onNothingSelected(AdapterView<?> arg0) {
                    MainActivity.this.textViewVariantDetails.setText("");
                }
            });
        }

        try {

            this.variantParser = new VariantParser();

            this.setVariantScanPath(SettingsHelper.getVariantScanPath(this));

            this.loadVariants();
            this.initModemController();
        }
        catch (Exception ex) {
            MessageBoxHelper.showException(this, ex);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if (this.executor != null) {
            this.executor.cancel(true);
        }

        if (this.modemController != null) {
            try {
                this.modemController.close();
            }
            catch (IOException ex) {
                Log.e("MainActivity", ex.toString());
            }
        }
    }

    private void loadVariants() {

        try {
            this.loadedVariants = this.variantParser.parseVariants(new File(this.currentVariantPath));
            if (this.loadedVariants != null) {
                this.textViewVariantDetails.setText("Please pick a folder containing valid variant XML files.");
                if (this.spinnerVariants != null) {
                    this.spinnerVariants.setAdapter(new VariantAdapter(this, this.loadedVariants));
                }
            }
        } catch (Exception ex) {
            Log.e("MainActivity", ex.toString());
            MessageBoxHelper.showException(this, ex);
        }
    }

    private void initModemController() {

        try {
            this.modemController = new MedfieldModemController();
        }
        catch (Exception ex) {
            Log.e("MainActivity", ex.toString());
            MessageBoxHelper.showException(this, ex);
        }
    }

    public boolean handleMessage(Message msg) {

        if (msg != null) {
            switch (msg.what) {

            case MedfieldModemController.MSG_ERROR:
                MessageBoxHelper.showException(this, (Exception)msg.obj);
                break;
            }
        }
        return true;
    }

    private void showVariantDetails() {
        Intent intent = new Intent(MainActivity.this, VariantDetailsActivity.class);

        intent.putExtra(Intents.EXTRA_VARIANT, (Variant)MainActivity.this.spinnerVariants.getSelectedItem());

        MainActivity.this.startActivity(intent);
    }

    @SuppressWarnings("unchecked")
    private void applyVariant() {

        Variant selectedVariant = (Variant)this.spinnerVariants.getSelectedItem();

        if (selectedVariant != null) {

            if (this.executor != null) {
                this.executor.cancel(true);
                this.executor = null;
            }
            this.executor = new AsyncCommandExecutor<ATControlCommand, ATControlResponse>(this, this.modemController);

            ModemControlCommandList<ATControlCommand> commands = CommandHelper.extractCommandsFromVariant(selectedVariant);

            this.textViewLogs.setText("");

            this.setUIEnabled(false);

            executor.execute(commands);
        }
    }

    private void pickFolder() {

        Intent intent = new Intent(this, DirectoryPickerActivity.class);

        intent.putExtra(DirectoryPickerActivity.START_DIR, "/");
        intent.putExtra(DirectoryPickerActivity.ONLY_DIRS, true);
        intent.putExtra(DirectoryPickerActivity.SHOW_HIDDEN, false);

        super.startActivityForResult(intent, 0);
    }

    private void setVariantScanPath(String path) {

        this.currentVariantPath = path;

        if (this.editTextVariantPath != null) {
            this.editTextVariantPath.setText(path);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        try {
            if (requestCode == 0 && resultCode == Activity.RESULT_OK) {

                String newPath = data.getStringExtra(DirectoryPickerActivity.CHOSEN_DIRECTORY);

                SettingsHelper.setVariantScanPath(this, newPath);
                this.setVariantScanPath(newPath);

                this.loadVariants();
            }
        }
        catch (Exception ex) {
            MessageBoxHelper.showException(this, ex);
        }
    }

    public void onClick(View v) {
        try {
            switch (v.getId()) {
            case R.id.buttonApply:
                this.applyVariant();
                break;
            case R.id.buttonVariantDetails:
                this.showVariantDetails();
                break;
            case R.id.buttonSetPath:
                this.pickFolder();
                break;
            }
        }
        catch (Exception ex) {
            MessageBoxHelper.showException(this, ex);
        }
    }

    public void onError(Exception ex) {

        this.setUIEnabled(true);
        MessageBoxHelper.showException(this, ex);
        Log.e("MainActivity", ex.toString());
    }

    public void onFinished(ModemControlResponseList<ATControlResponse> result) {

        this.setUIEnabled(true);
        if (this.selectedVariant != null) {
            MessageBoxHelper.showMessage(this, this.selectedVariant.getOnCompleteMsg());
        }
        Log.i("MainActivity", "Operation complete");
    }

    public void onCancel() {
        this.setUIEnabled(true);
        Log.i("MainActivity", "Operation cancelled");
    }

    public void onProgress(String progress) {

        Log.i("MainActivity", progress);
        this.textViewLogs.append(progress + "\r\n");
        this.scrollViewLog.scrollBy(0, 100);
    }

    private void setUIEnabled(boolean enabled) {
        this.buttonApply.setEnabled(enabled);
        this.spinnerVariants.setEnabled(enabled);
        this.buttonSetPath.setEnabled(enabled);
        this.progressBar.setVisibility(enabled ? View.INVISIBLE : View.VISIBLE);
    }
}
