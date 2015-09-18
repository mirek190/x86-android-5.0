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
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ProgressBar;

import com.intel.amtl.AMTLApplication;
import com.intel.amtl.helper.LogManager;
import com.intel.amtl.R;

// Embedded class to handle delayed configuration setup (Dialog part).
public class SaveLogFrag extends DialogFragment {

    private final String TAG = AMTLApplication.getAMTLApp().getLogTag();
    private final String MODULE = "SaveLogFrag";
    private FragmentManager fm;

    int targetFrag;
    String tag;

    ProgressBar savLogNot;
    // thread executed while Dialog Box is displayed.
    SaveLogTask exeSetup;

    public SaveLogFrag(FragmentManager fm, String tag, int targetFrag) {
        this.tag = tag;
        this.targetFrag = targetFrag;
        this.fm = fm;
    }

    public void handlerSav(SaveLogTask savTask) {
        // This allows to get SaveLogTerminated on the specified Fragment.
        this.exeSetup = savTask;
        this.exeSetup.setFragment(this);
    }

    public void saveLogTerminated(String exceptReason) {
        // dismiss() is possible only if we are on the current Activity.
        // And will crash if we have switched to another one.
        if (isResumed()) {
            dismiss();
        }

        this.exeSetup = null;
        if (!exceptReason.equals("")) {
            Log.e(TAG, MODULE + ": Backup log failed: " + exceptReason);
            UIHelper.okDialog(getActivity(),
                    "Error ", "Log not saved " + exceptReason);
        }

        if (getTargetFragment() != null) {
            getTargetFragment()
                    .onActivityResult(targetFrag, Activity.RESULT_OK, null);
        }
    }

    public void launch(LogManager lm) {
        handlerSav(new SaveLogTask(lm));
        setTargetFragment(this, targetFrag);
        show(this.fm, tag);
    }

    // Function overrides for the DialogFragment instance.
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRetainInstance(true);
        // Spawn the thread to execute modem configuration.
        if (this.exeSetup != null) {
            this.exeSetup.execute();
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        // Create dialog box.
        View view = inflater.inflate(R.layout.fragment_task, container);
        savLogNot = (ProgressBar)view.findViewById(R.id.progressBar);
        getDialog().setTitle("Log Backup... Please wait.");
        setCancelable(false);

        return view;
    }

    @Override
    public void onDestroyView() {
        /* This will allow dialog box to stay even if parent layout configuration
        is changed (rotation).*/
        if (getDialog() != null && getRetainInstance()) {
            getDialog().setDismissMessage(null);
        }
        super.onDestroyView();
        }

    @Override
    public void onResume() {
        /* This allows to close dialog box if the thread ends while we are not focused
        on the activity. */
        super.onResume();
        if (this.exeSetup == null) {
            dismiss();
        }
    }

    // embedded class to handle delayed configuration setup (thread part).
    public static class SaveLogTask extends AsyncTask<Void, Void, Void> {
        private SaveLogFrag savLogFrag;
        String exceptReason = "";
        LogManager lm;

        public SaveLogTask (LogManager lm) {
             this.lm = lm;
        }

        void setFragment(SaveLogFrag svLogFrag) {
            savLogFrag = svLogFrag;
        }

        // Function overrides for Apply configuration thread.
        @Override
        protected Void doInBackground(Void... params) {
            lm.makeBackup();
            return null;
        }

        @Override
        protected void onPostExecute(Void exception) {
            if (savLogFrag == null) {
                return;
            }
            savLogFrag.saveLogTerminated(exceptReason);
        }
    }
}
