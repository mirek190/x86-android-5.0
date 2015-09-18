/* Android Modem Traces and Logs
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
 * Author: Morgane Butscher <morganeX.butscher@intel.com>
 * Author: Erwan Bracq <erwan.bracq@intel.com>
 */

package com.intel.amtl.gui;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Context;
import android.graphics.Color;
import android.text.InputType;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.TextView;

import com.intel.amtl.AMTLApplication;
import com.intel.amtl.helper.LogManager;
import com.intel.amtl.helper.TelephonyStack;


public class UIHelper {

    private final static String TAG = AMTLApplication.getAMTLApp().getLogTag();

    /* Print pop-up message with ok and cancel buttons */
    public static void warningDialog(final Activity A, String title, String message,
            DialogInterface.OnClickListener listener) {
        new AlertDialog.Builder(A)
                .setTitle(title)
                .setMessage(message)
                .setCancelable(false)
                .setPositiveButton("OK", listener)
                .setNegativeButton("Cancel", listener)
                .show();
    }

    /* Print pop-up message with ok and cancel buttons */
    public static void warningExitDialog(final Activity A, String title, String message) {
        new AlertDialog.Builder(A)
                .setTitle(title)
                .setMessage(message)
                .setCancelable(false)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int whichButton) {
                        /* Nothing to do */
                    }
                })
                .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int whichButton) {
                        /* Exit application */
                        A.finish();
                    }
                })
                .show();
    }

    /* Print pop-up message with ok and cancel buttons to save */
    public static void savePopupDialog(final Activity A, String title, String message,
            final Context context, final LogManager snaplog, final SaveLogFrag logProgressFrag) {
        final EditText saveInput = new EditText(context);
        saveInput.setText("snapshot", TextView.BufferType.EDITABLE);
        saveInput.setTextColor(Color.parseColor("#66ccff"));
        saveInput.setInputType(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        new AlertDialog.Builder(A)
                .setView(saveInput)
                .setTitle(title)
                .setMessage(message)
                .setCancelable(false)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int whichButton) {
                        String snapshotTag = saveInput.getText().toString();
                        InputMethodManager imm = (InputMethodManager)
                            A.getSystemService(context.INPUT_METHOD_SERVICE);
                        if (imm != null) {
                            imm.hideSoftInputFromWindow(saveInput.getWindowToken(), 0);
                        }
                        // if im is null, no specific issue, virtual keyboard will not be cleared
                        snaplog.setTag(snapshotTag);
                        logProgressFrag.launch(snaplog);
                    }
                })
                .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int whichButton) {
                        /* Nothing to do */
                    }
                })
                .show();
    }

    /* Print pop-up message with ok and cancel buttons to save */
    public static void logTagDialog(final Activity A, String title, String message,
            final Context context) {
        final EditText saveInput = new EditText(context);
        saveInput.setText("TAG_TO_SET_IN_LOGS", TextView.BufferType.EDITABLE);
        saveInput.setTextColor(Color.parseColor("#66ccff"));
        saveInput.setInputType(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        new AlertDialog.Builder(A)
                .setView(saveInput)
                .setTitle(title)
                .setMessage(message)
                .setCancelable(false)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int whichButton) {
                        String logTag = saveInput.getText().toString();
                        InputMethodManager imm = (InputMethodManager)
                            A.getSystemService(context.INPUT_METHOD_SERVICE);
                        if (imm != null) {
                            imm.hideSoftInputFromWindow(saveInput.getWindowToken(), 0);
                        }
                        // if im is null, no specific issue, virtual keyboard will not be cleared
                        Log.d(TAG, "UIHelper: " + logTag);
                    }
                })
                .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int whichButton) {
                        /* Nothing to do */
                    }
                })
                .show();
    }

    /* Print pop-up message with ok button */
    public static void okDialog(Activity A, String title, String message) {
        new AlertDialog.Builder(A)
                .setTitle(title)
                .setMessage(message)
                .setCancelable(false)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        /* Nothing to do, waiting for user to press OK button */
                    }
                })
                .show();
    }

    /* Print a dialog before exiting application */
    public static void exitDialog(final Activity A, String title, String message) {
        new AlertDialog.Builder(A)
                .setTitle(title)
                .setMessage(message)
                .setCancelable(false)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        A.finish();
                    }
                })
                .show();
    }

    /* Print pop-up message with ok and cancel buttons */
    public static void messageExitActivity(final Activity A, String title, String message) {
        new AlertDialog.Builder(A)
                .setTitle(title)
                .setMessage(message)
                .setCancelable(false)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int whichButton) {
                        A.finish();
                    }
                })
                .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int whichButton) {
                        /* Nothing to do */
                    }
                })
                .show();
    }

    /* Print pop-up message with ok and cancel buttons, dedicated to telephony stack
     * property.
     */
    public static void messageSetupTelStack(final Activity A, String title, String message,
            final TelephonyStack telStackSetter) {
        new AlertDialog.Builder(A)
                .setTitle(title)
                .setMessage(message)
                .setCancelable(false)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int whichButton) {
                        /* Enable stack and exit. */
                        telStackSetter.enableStack();
                        exitDialog(A, "REBOOT REQUIRED.",
                            "As you enable telephony stack, AMTL will exit. "
                            + " Please reboot your device.");
                    }
                })
                .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int whichButton) {
                        /* We just exit. */
                        exitDialog(A, "Exit",
                            "AMTL will exit. Telephony stack is disabled.");
                    }
                })
                .show();
    }

}
