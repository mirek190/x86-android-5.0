/*
 * Copyright (C) Intel 2014
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
 */

package com.intel.atproxy;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AuthenticatorDescription;
import android.app.ActivityManager;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageStats;
import android.content.pm.IPackageStatsObserver;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.hardware.input.InputManager;
import android.hardware.input.InputManager.InputDeviceListener;
import android.net.ConnectivityManager;
import android.telephony.TelephonyManager;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.provider.ContactsContract;
import android.provider.CalendarContract.Events;
import android.provider.CallLog.Calls;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Video;
import android.provider.MediaStore.Audio;
import android.provider.Telephony;
import android.provider.Telephony.Mms;
import android.provider.Telephony.Sms;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.net.TrafficStats;
import android.net.Uri;
import android.os.Environment;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

import com.android.internal.telephony.IIccPhoneBook;
import com.android.internal.telephony.uicc.AdnRecord;
import com.android.internal.telephony.uicc.IccConstants;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.lang.reflect.Method;
import java.util.List;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;

// ATService is a service which handles commands from atproxy.
public class ATService extends Service {

    // Tag for logs
    private static final String TAG = "ATProxy";

    private static final int SOCKET_OPEN = 0;
    private static final int CLIENT_CONNECT = 1;
    private static final int GET_COMMAND = 2;

    // local variables
    private boolean isSim1 = true;
    private boolean open_socket = false;
    private boolean flag = true;
    private boolean client_connected = false;
    private int appSize;

    // Socket stuff
    private static final String SOCKET_NAME_ATPROXY = "atproxy-apb";
    private static final int SOCKET_OPEN_RETRY_MILLIS = 4 * 1000;
    private LocalSocket mSocket;

    private boolean CMEREnable = true;
    private boolean CTSAEnable = true;

    private ConnectivityManager mConnService;
    private TelephonyManager mTelephonyService;

    @Override
    public void onCreate() {
        mConnService = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);
        mTelephonyService = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
        Log.d(TAG, "onCreate() call");
    }

    @Override
    public void onStart(Intent intent, int startId) {
        Log.d(TAG, "onStart() call");
        super.onStart(intent, startId);
        if (open_socket==true) return;
        new SocketClientThread().start();
        open_socket = true;
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy() call");
        super.onDestroy();
        if (mSocket != null) {
            try{
                mSocket.close();
            } catch (IOException ex) {
                // ignore failure to close socket
            }
            mSocket = null;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    // method: Log SocketClientThread state and process socket messages.
    private Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case SOCKET_OPEN:
                Log.d(TAG, "SOCKET_OPEN");
                break;
            case CLIENT_CONNECT:
                break;
            case GET_COMMAND:
                String command = (String) msg.obj;
                handlerCommand(command);
                break;
            default:
                break;
            }
        };
    };

    // method: Process socket messages and log actions
    private void handlerCommand(String command) {
        if ("AT+CTACT=NR".equals(command)) {
            response(getCount(buildContactUri()));
        } else if ("AT+CTACT=SZ".equals(command)) {
            double size = getDatabaseSize(getResources().getString(R.string.dbpath_contacts),
                    getResources().getString(R.string.dbname_contacts));
            responseString(String.valueOf(size)+" KB");
        } else if ("AT+CCALD=NR".equals(command)) {
            response(getCount(Events.CONTENT_URI));
        } else if ("AT+CCALD=SZ".equals(command)) {
            double size = getDatabaseSize(getResources().getString(R.string.dbpath_calendar),
                    getResources().getString(R.string.dbname_calendar));
            responseString(String.valueOf(size)+" KB");
        } else if ("AT+CCLGS=DC".equals(command)) {
            response(getCallNumber(Calls.OUTGOING_TYPE));
        } else if ("AT+CCLGS=RC".equals(command)) {
            response(getCallNumber(Calls.INCOMING_TYPE));
        } else if ("AT+CCLGS=MC".equals(command)) {
            response(getCallNumber(Calls.MISSED_TYPE));
        } else if ("AT+CDUR=LC".equals(command)) {
            responseString(getCallDuration(0));
        } else if ("AT+CDUR=DC".equals(command)) {
            responseString(getCallDuration(Calls.OUTGOING_TYPE));
        } else if ("AT+CDUR=RC".equals(command)) {
            responseString(getCallDuration(Calls.INCOMING_TYPE));
        } else if ("AT+CDUR=AC".equals(command)) {
            responseString(getCallDuration(Calls.OUTGOING_TYPE+Calls.INCOMING_TYPE));
        } else if ("AT+CDVOL=TX".equals(command)) {
            responseString(Long.toString(TrafficStats.getMobileTxBytes()/1000)+" KB");
        } else if ("AT+CDVOL=RX".equals(command)) {
            responseString(Long.toString(TrafficStats.getMobileRxBytes()/1000)+" KB");
        } else if ("AT+CSHM=IB,NR".equals(command)) {
            response(getCount(Sms.Inbox.CONTENT_URI));
        } else if ("AT+CSHM=IB,SZ".equals(command)) {
            double size = getSMSSize(getContentResolver().query(
                Sms.Inbox.CONTENT_URI, new String[] {Sms.Inbox.BODY}, null, null, null));
            responseString(String.valueOf(size));
        } else if ("AT+CSHM=OB,NR".equals(command)) {
            response(getCount(Sms.Outbox.CONTENT_URI));
        } else if ("AT+CSHM=OB,SZ".equals(command)) {
            double size = getSMSSize(getContentResolver().query(
                Sms.Outbox.CONTENT_URI, new String[] {Sms.Outbox.BODY}, null, null, null));
            responseString(String.valueOf(size));
        } else if ("AT+CSHM=SB,NR".equals(command)) {
            response(getCount(Sms.Sent.CONTENT_URI));
        } else if ("AT+CSHM=SB,SZ".equals(command)) {
            double size = getSMSSize(getContentResolver().query(
                Sms.Sent.CONTENT_URI, new String[] {Sms.Sent.BODY}, null, null, null));
            responseString(String.valueOf(size));
        } else if ("AT+CSHM=AR,NR".equals(command)) {
            response(getCount(Sms.Draft.CONTENT_URI));
        } else if ("AT+CSHM=AR,SZ".equals(command)) {
            double size = getSMSSize(getContentResolver().query(
                Sms.Draft.CONTENT_URI, new String[] {Sms.Draft.BODY}, null, null, null));
            responseString(String.valueOf(size));
        } else if ("AT+CEMAIL=NR".equals(command)) {
            response(getAccount(0));
        } else if ("AT+CBCAST=NR".equals(command)) {
            Cursor data = getCbsCursor();
            response((data != null) ? data.getCount() : 0);
        } else if ("AT+CBCAST=SZ".equals(command)) {
            responseString(String.valueOf(getCbsSize()));
        } else if ("AT+CMMS=IB,NR".equals(command)) {
            response(getCount(Mms.Inbox.CONTENT_URI));
        } else if ("AT+CMMS=IB,SZ".equals(command)) {
            double size = 0;
            Cursor data = getContentResolver().query(
                Mms.Inbox.CONTENT_URI, new String[] {Mms.Inbox.MESSAGE_SIZE}, null, null, null);
            if (data != null && data.moveToFirst()) {
                do {
                    size = Integer.parseInt(data.getString(0)) + size;
                } while (data.moveToNext());
                data.close();
            }
            responseString(String.valueOf(size/1000));
        } else if ("AT+CDCONT=IM,NR".equals(command)) {
            response(getCount(Images.Media.EXTERNAL_CONTENT_URI));
        } else if ("AT+CDCONT=IM,SZ".equals(command)) {
            response(getMediaSize(Images.Media.EXTERNAL_CONTENT_URI, 2));
        } else if ("AT+CDCONT=AUD,NR".equals(command)) {
            response(getCount(Audio.Media.EXTERNAL_CONTENT_URI));
        } else if ("AT+CDCONT=AUD,SZ".equals(command)) {
            response(getMediaSize(Audio.Media.EXTERNAL_CONTENT_URI, 3));
        } else if ("AT+CDCONT=VID,NR".equals(command)) {
            response(getCount(Video.Media.EXTERNAL_CONTENT_URI));
        } else if ("AT+CDCONT=VID,SZ".equals(command)) {
            response(getMediaSize(Video.Media.EXTERNAL_CONTENT_URI, 3));
        } else if ("AT+CDCONT=GA,NR".equals(command)) {
            List<ApplicationInfo> applications =
                getPackageManager().getInstalledApplications(PackageManager.GET_META_DATA);
            if (applications != null) {
                response(applications.size());
            } else {
                response(0);
            }
        } else if ("AT+CDCONT=GA,SZ".equals(command)) {
            responseString(String.valueOf(getAppSize()));
        } else if ("AT+CSYNC=NR".equals(command)) {
            response(getAccount(1));
        } else if ("AT+CBLTH=NM".equals(command)) {
            BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
            responseString(adapter.getName());
        } else if ("AT+CBLTH=NR".equals(command)) {
            response(getBtPairNumber());
        } else if (command.contains("AT+CBLTH=")) {
            String prefix = "AT+CBLTH=";
            String cmd = command.substring(command.indexOf(prefix)+prefix.length());
            if (cmd.isEmpty()) {
                responseString("ERROR");
            } else {
                String[] tokens = cmd.split(",");
                int index = Integer.parseInt(tokens[0]);
                responseString(getBtPairName(index));
            }
        } else if ("AT+CALRM=NR".equals(command)) {
            int count = getCount(Uri.parse(getResources().getString(R.string.uri_alarm)));
            if (count >= 0) {
                response(count);
            } else {
                responseString("ERROR");
            }
        } else if (command.contains("AT+CALRM=")) {
            String prefix = "AT+CALRM=";
            String cmd = command.substring(command.indexOf(prefix)+prefix.length());
            int index = Integer.parseInt(cmd);
            responseString(getAlarmTime(index));
        } else if ("AT+CTMRV=NR".equals(command)) {
            response(getTimerCount());
        } else if (command.contains("AT+CALRM=")) {
            String prefix = "AT+CALRM=";
            String cmd = command.substring(command.indexOf(prefix)+prefix.length());
            int index = Integer.parseInt(cmd);
            responseString(getTimerTime(index));
        } else if ("AT+CVRCD=NR".equals(command)) {
            response(getVoiceRecordCount());
        } else if ("AT+CVRCD=SZ".equals(command)) {
            responseString(String.valueOf(getVoiceRecordSize()));
        } else if ("ATI1".equals(command)) {
            responseString("["+SystemProperties.get("ro.build.version.incremental", "")+"]["
                    + SystemProperties.get("sys.ifwi.version", "")+"]");
        } else if ("AT+CSMCT=NR".equals(command)) {
            response(getSimContactCount());
        } else if ("AT+CTASK=NR".equals(command)) {
            int count = 0;
            ActivityManager am = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
            count = am.getRunningAppProcesses().size();
            response(count);
        } else if ("AT+CRST=FS".equals(command)) {
            factoryReset();
            responseString("OK");
        } else if (command.startsWith("AT+CMER")) {
            String prefix = "AT+CMER";
            String ret;
            String subCommand = command.substring(prefix.length());
            ret = CMERParser(subCommand);
            responseString(ret);
        } else if (command.startsWith("AT+CMEC")) {
            String prefix = "AT+CMEC";
            String ret;
            String subCommand = command.substring(prefix.length());
            ret = CMECParser(subCommand);
            responseString(ret);
        } else if (command.startsWith("AT+CTSA")) {
            String prefix = "AT+CTSA";
            String ret;
            String subCommand = command.substring(prefix.length());
            ret  = CTSAParser(subCommand);
            responseString(ret);
        } else if (command.toUpperCase().startsWith("AT+CGDCONT")) {
            String ret = CGDCONTParser(command);
            responseString(ret);
        } else if (command.toUpperCase().startsWith("AT+CGACT")) {
            String ret = CGACTParser(command);
            responseString(ret);
        } else if (command.toUpperCase().startsWith("AT+CFUN")) {
            String ret = CFUNParser(command);
            responseString(ret);
        } else {
            Log.e(TAG, "Unknown command: " + command);
            responseString("ERROR");
        }
    }

    private String CGDCONTParser(String command) {
        Log.d(TAG, "Handling " + command);
        return "OK";
    }

    private String CGACTParser(String command) {
        String argstring = command.substring("AT+CGACT=".length());
        String[] args = argstring.split(",");
        Log.d(TAG, "Handling " + command + " enable=" + args[0] + " cid=" + args[1]);
        mTelephonyService.setDataEnabled("1".equals(args[0]) ? true : false);
        return "OK";
    }

    private String CFUNParser(String command) {
        String argstring = command.substring("AT+CFUN=".length());
        Log.d(TAG, "Handling " + command + " enabled '" + argstring + "'");
        mConnService.setAirplaneMode(!"1".equals(argstring));
        return "OK";
    }

    private String CMERParser(String str) {
         if (str.equals("?"))
            return "+CMER:,,,,,0";
         else if (str.equals("=0")) {
            Log.d(TAG, "Set mode 0");
            return "Set mode 0";
         }  else if (str.equals("=?"))
            return "+CMER:,,,,,0";
         else if (str.equals("=0,,,,,0")) {
            CMEREnable = false;
            return "+CMER:0,,,,,0";
        } else if (str.equals("=0,,,,,1")) {
            CMEREnable = true;
            return "+CMER:0,,,,,1";
        } else if (str.equals("=0,,,,,2")) {
            CMEREnable = true;
            return "+CMER:0,,,,,2";
        } else if (str.equals("=0,,,,,3")) {
            return "ERROR";
        } else
            return "ERROR";
    }

    private String CMECParser(String str) {
         if (str.equals("?"))
            return "+CMEC:,,,0";
         else if (str.equals("=?"))
            return "+CMEC:,,,0";
         else if (str.equals("=,,,0")) {
            CTSAEnable = false;
            return "+CTSA: Disable";
         } else if (str.equals("=,,,1"))
            return "ERROR";
         else if (str.equals("=,,,2")) {
            CTSAEnable = true;
            return "+CTSA: Enable";
         } else
            return "ERROR";
   }

    private String CTSAParser(String str) {
        int inputSource = InputDevice.SOURCE_UNKNOWN;
        String[] cmdArray = str.split(";");
        boolean positiveResponse = true;

        if (CTSAEnable == false)
            return "ERROR";

        inputSource = getSource(inputSource, InputDevice.SOURCE_TOUCHSCREEN);

        if (str.equals("?"))
            return "+CTSA:0,1,2,3";
        else if (str.equals("=?"))
            return "ERROR";

        Log.d(TAG, "# of CTSA patterns:" + cmdArray.length);

        if (cmdArray.length == 1) { // Depress or Release or Single Tap or Double Tap
            String[] pos = cmdArray[0].split("=|,");
            if (pos.length == 4) {
                if (pos[1].equals("1")) {
                    Log.d(TAG, "Depress: x:" + pos[2] + "y:" + pos[3]);
                    sendDepress(inputSource, Float.parseFloat(pos[2]), Float.parseFloat(pos[3]));
                } else if (pos[1].equals("0")) {
                    Log.d(TAG, "Release: x:" + pos[2] + "y:" + pos[3]);
                    sendRelease(inputSource, Float.parseFloat(pos[2]), Float.parseFloat(pos[3]));
                } else if (pos[1].equals("2")) {
                    Log.d(TAG, "Single Tap: x:" + pos[2] + "y" + pos[3]);
                    sendTap(inputSource, Float.parseFloat(pos[2]), Float.parseFloat(pos[3]));
                } else if (pos[1].equals("3")) {
                    Log.d(TAG, "Double Tap: x:" + pos[2] + "y" + pos[3]);
                    sendTap(inputSource, Float.parseFloat(pos[2]), Float.parseFloat(pos[3]));
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException e ) {
                        e.printStackTrace();
                    }
                    sendTap(inputSource, Float.parseFloat(pos[2]), Float.parseFloat(pos[3]));
                } else {
                    Log.d(TAG, "Wrong parameter");
                    positiveResponse = false;
                }
            } else {
                Log.d(TAG, "wrong parameter");
                positiveResponse = false;
            }
        }

        if (cmdArray.length == 2) { // Drag
            String[] sPos = cmdArray[0].split("=|,"); // start position
            String[] ePos = cmdArray[1].split("=|,"); // end position
            if (sPos.length == 4 && ePos.length == 4) { // correct format
                 Log.d(TAG, "Drag from " + sPos[2] + " " + sPos[3] + "to" + " "
                        + ePos[2] + " " + ePos[3]);
                 sendSwipe(inputSource, Float.parseFloat(sPos[2]), Float.parseFloat(sPos[3]),
                        Float.parseFloat(ePos[2]), Float.parseFloat(ePos[3]),
                        1000); // 1 seconds for swipe time
            } else {
                Log.d(TAG, "wrong parameter");
                positiveResponse = false;
            }
        }

        if (cmdArray.length == 3) { // Draw
            String[] sPos = cmdArray[0].split("=|,"); // start position
            String[] mPos = cmdArray[1].split("=|,"); // middle position
            String[] ePos = cmdArray[2].split("=|,"); // end position
            if (sPos.length == 4 && mPos.length == 4 && ePos.length == 4) {
                Log.d(TAG, "Draw from  " + sPos[2] + " " + sPos[3] + " to " + mPos[2] + " "
                        + mPos[3] + " to " + ePos[2] + " " + ePos[3]);
                // spend 1s from x1,y1 to x2,y2 and 1s from x2,y2 to x3.y3
                sendDraw(inputSource, Float.parseFloat(sPos[2]), Float.parseFloat(sPos[3]),
                        Float.parseFloat(mPos[2]), Float.parseFloat(mPos[3]),
                        Float.parseFloat(ePos[2]), Float.parseFloat(ePos[3]), 1000);
            } else {
                Log.d(TAG, "wrong parameter");
                positiveResponse = false;
            }
        }
        if (positiveResponse == true)
            return "+CTSA:OK";
        else
            return "ERROR";
    }

// ++ process touch event
    private void sendKeyEvent(int inputSource, int keyCode, boolean longpress) {
        long now = SystemClock.uptimeMillis();
        injectKeyEvent(new KeyEvent(now, now, KeyEvent.ACTION_DOWN, keyCode, 0, 0,
                KeyCharacterMap.VIRTUAL_KEYBOARD, 0, 0, inputSource));
        if (longpress) {
            injectKeyEvent(new KeyEvent(now, now, KeyEvent.ACTION_DOWN, keyCode, 1, 0,
                    KeyCharacterMap.VIRTUAL_KEYBOARD, 0, KeyEvent.FLAG_LONG_PRESS,
                    inputSource));
        }
        injectKeyEvent(new KeyEvent(now, now, KeyEvent.ACTION_UP, keyCode, 0, 0,
                KeyCharacterMap.VIRTUAL_KEYBOARD, 0, 0, inputSource));
    }

    private void sendDepress(int inputSource, float x, float y) {
        long now = SystemClock.uptimeMillis();
        injectMotionEvent(inputSource, MotionEvent.ACTION_DOWN, now, x, y, 0.0f);
    }

    private void sendRelease(int inputSource, float x, float y) {
        long now = SystemClock.uptimeMillis();
        injectMotionEvent(inputSource, MotionEvent.ACTION_UP, now, x, y, 0.0f);
    }

    private void sendTap(int inputSource, float x, float y) {
        long now = SystemClock.uptimeMillis();
        injectMotionEvent(inputSource, MotionEvent.ACTION_DOWN, now, x, y, 1.0f);
        injectMotionEvent(inputSource, MotionEvent.ACTION_UP, now, x, y, 0.0f);
    }

    private void sendMove(int inputSource, float x1, float y1, float x2, float y2, int duration) {
        if (duration < 0) {
            duration = 300;
        }
        long now = SystemClock.uptimeMillis();
        long startTime = now;
        long endTime = startTime + duration;
        while (now < endTime) {
            long elapsedTime = now - startTime;
            float alpha = (float) elapsedTime / duration;
            injectMotionEvent(inputSource, MotionEvent.ACTION_MOVE, now, lerp(x1, x2, alpha),
                    lerp(y1, y2, alpha), 1.0f);
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
            }
            now = SystemClock.uptimeMillis();
        }
    }

    private void sendSwipe(int inputSource, float x1, float y1, float x2, float y2, int duration) {
        sendDepress(inputSource, x1, y1);
        sendMove(inputSource, x1, y1, x2, y2, duration);
        sendRelease(inputSource, x2, y2);
    }

    private void sendDraw(int inputSource, float x1, float y1, float x2, float y2, float x3,
            float y3, int duration) {
        sendDepress(inputSource, x1, y1);
        sendMove(inputSource, x1, y1, x2, y2, duration);
        sendMove(inputSource, x2, y2, x3, y3, duration);
        sendRelease(inputSource, x3, y3);
    }

    /**
     * Sends a simple zero-pressure move event.
     *
     * @param inputSource the InputDevice.SOURCE_* sending the input event
     * @param dx change in x coordinate due to move
     * @param dy change in y coordinate due to move
     */
    private void sendMove(int inputSource, float dx, float dy) {
        long now = SystemClock.uptimeMillis();
        injectMotionEvent(inputSource, MotionEvent.ACTION_MOVE, now, dx, dy, 0.0f);
    }

    private void injectKeyEvent(KeyEvent event) {
        Log.i(TAG, "injectKeyEvent: " + event);
        InputManager.getInstance().injectInputEvent(event,
                InputManager.INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
    }

    /**
     * Builds a MotionEvent and injects it into the event stream.
     *
     * @param inputSource the InputDevice.SOURCE_* sending the input event
     * @param action the MotionEvent.ACTION_* for the event
     * @param when the value of SystemClock.uptimeMillis() at which the event happened
     * @param x x coordinate of event
     * @param y y coordinate of event
     * @param pressure pressure of event
     */
    private void injectMotionEvent(int inputSource, int action, long when, float x, float y,
            float pressure) {
        final float DEFAULT_SIZE = 1.0f;
        final int DEFAULT_META_STATE = 0;
        final float DEFAULT_PRECISION_X = 1.0f;
        final float DEFAULT_PRECISION_Y = 1.0f;
        final int DEFAULT_DEVICE_ID = 0;
        final int DEFAULT_EDGE_FLAGS = 0;
        MotionEvent event = MotionEvent.obtain(when, when, action, x, y, pressure, DEFAULT_SIZE,
                DEFAULT_META_STATE, DEFAULT_PRECISION_X, DEFAULT_PRECISION_Y, DEFAULT_DEVICE_ID,
                DEFAULT_EDGE_FLAGS);
        event.setSource(inputSource);
        Log.i(TAG, "injectMotionEvent: " + event);
        InputManager.getInstance().injectInputEvent(event,
                InputManager.INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
    }

    private static final float lerp(float a, float b, float alpha) {
        return (b - a) * alpha + a;
    }

    private static final int getSource(int inputSource, int defaultSource) {
        return inputSource == InputDevice.SOURCE_UNKNOWN ? defaultSource : inputSource;
    }

// -- process touch event

    // method: Write data on socket (answer) JAVA application -> at-proxy service
    private void response(int value) {
        OutputStream outputStream = null;
        Log.d(TAG, "response, int :" + String.valueOf(value));
        try {
            if (mSocket != null) {
                outputStream = mSocket.getOutputStream();
                BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(
                        outputStream));
                writer.write(String.valueOf(value)+"\r\nOK\r");
                writer.newLine();
                writer.flush();
            }
        } catch (IOException e) {
            Log.d(TAG, e.toString());
        }
    }

   private void responseString(String str) {
        OutputStream outputStream = null;
        Log.d(TAG, "response: "+ str);
        try {
            if (mSocket != null) {
                outputStream = mSocket.getOutputStream();
                BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(
                        outputStream));
                // return OK after response string to prevent phonetool failure.
                // This can be remove if not using phonetool.
                writer.write(str+"\r\nOK\r");
                writer.newLine();
                writer.flush();
            }
        } catch (IOException e) {
            Log.d(TAG, e.toString());
        }
    }

    private int getCount(Uri uri) {
        int count = 0;
        try {
            Cursor data = getContentResolver().query(uri, null, null, null, null);
            if (data != null) {
                count = data.getCount();
                data.close();
            }
        } catch (Exception e) {
            Log.d(TAG, e.toString());
            count = -1;
        }
        return count;
    }

    private double getSMSSize(Cursor data) {
        double size = 0;
        if (data != null && data.moveToFirst()) {
            do {
                size += data.getString(0).length();
            } while (data.moveToNext());
            data.close();
        }
        return size / 1024;
    }

    private Uri buildContactUri() {
        return ContactsContract.Contacts.CONTENT_URI.buildUpon()
                .appendQueryParameter("address_book_index_extras", "true")
                .appendQueryParameter(ContactsContract.DIRECTORY_PARAM_KEY,
                String.valueOf(0)).build();
    }

    private double getDatabaseSize(String path, String dbname) {
        double size = 0;
        try {
            File file = new File(path, dbname);
            if (file.exists())
                size = file.length() / 1024;
        } catch (Exception e) {
           Log.e(TAG, "showDatabaseSize FAILED. "+ e.toString());
        }
        return size;
    }

    private int getCallNumber(int callType) {
        int calls = 0;
        Cursor data = getContentResolver().query(Calls.CONTENT_URI,
                null, null, null, Calls.DEFAULT_SORT_ORDER);

        if (data != null && data.moveToFirst()) {
            int index = data.getColumnIndex(Calls.TYPE);
            do {
                try {
                    if (data.getInt(index) == callType) {
                        calls ++;
                    }
                } catch (Exception e) {
                }
            } while (data.moveToNext());
            data.close();
        }
        return calls;
    }

    private String getCallDuration(int callType) {
        long duration = 0;
        Cursor data = getContentResolver().query(Calls.CONTENT_URI,
                null, null, null, Calls.DEFAULT_SORT_ORDER);
        if (data != null && data.moveToFirst()) {
            int index = data.getColumnIndex(Calls.DURATION);
            int indexOfType = data.getColumnIndex(Calls.TYPE);

            if (callType == 0) {
                return formatDuration((index != -1) ? data.getLong(index) : 0);
            }

            do {
                try {
                    if(callType == (Calls.INCOMING_TYPE + Calls.OUTGOING_TYPE)) {
                        duration += data.getLong(index);
                    } else if (callType == data.getInt(indexOfType)) {
                        duration += data.getLong(index);
                    }
                } catch (Exception e) {
                }
            } while (data.moveToNext());
            data.close();
        }

        return formatDuration(duration);
    }

    private String formatDuration(long elapsedSeconds) {
        long hours = 0, minutes = 0, seconds = 0;

        if (elapsedSeconds >= 60) {
            minutes = elapsedSeconds / 60;
            elapsedSeconds -= minutes * 60;
        }

        if (minutes >= 60) {
            hours = minutes / 60;
            minutes -= hours * 60;
        }

        seconds = elapsedSeconds;

        return getString(R.string.durationformat, hours, minutes, seconds);
    }

    private int getAccount(int all) {
        HashMap<String, String> hashMap = new HashMap<String, String>();
        AuthenticatorDescription[] descriptions = AccountManager.get(this).getAuthenticatorTypes();
        for (int i = 0; i < descriptions.length; i++) {
            try {
                Context authContext = createPackageContext(descriptions[i].packageName, 0);
                String label = authContext.getResources().getText(descriptions[i].labelId)
                        .toString();
                String type = descriptions[i].type;
                // count exchange in
                if ((type.contains("google") || type.contains("android")))
                    hashMap.put(type, label);
            } catch (Exception e) {
            }
        }

        Account[] accounts = AccountManager.get(this).getAccounts();
        if (all == 1)
            return accounts.length;

        int count = 0;
        for (int i=0;i<accounts.length;i++) {
            if (hashMap.containsKey(accounts[i].type))
                count++;
        }
        return count;
    }

    private Cursor getCbsCursor() {
        Uri CONTENT_URI = Uri.parse("content://cellbroadcasts/");

        SQLiteOpenHelper openHelper = new CellBroadcastDatabaseHelper(this);
        SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
        qb.setTables(CellBroadcastDatabaseHelper.TABLE_NAME);
        SQLiteDatabase db = openHelper.getReadableDatabase();
        return qb.query(db, new String[] {Telephony.CellBroadcasts.MESSAGE_BODY},
                null, null, null, null, Telephony.CellBroadcasts.DEFAULT_SORT_ORDER);
    }

    private double getCbsSize() {
        double size = 0;
        Cursor data = getCbsCursor();
        if (data != null && data.moveToFirst()) {
            do {
                for (int i=0; i < data.getCount(); i++) {
                    size += (data.getString(0)).length();
                }
            } while (data.moveToNext());
            data.close();
            size = size / 1024;
        }
        return size;
    }

    private int getMediaSize(Uri uri, int column) {
        int size = 0;
        Cursor data = getContentResolver().query(uri, null, null, null, null);
        if (data != null && data.moveToFirst()) {
            do {
                size += Integer.parseInt(data.getString(column));
            } while (data.moveToNext());
            data.close();
        }
        return size / 1024;
    }

    private int getAppSize() {
        final PackageManager pm = getPackageManager();
        List<ApplicationInfo> applications =
                pm.getInstalledApplications(PackageManager.GET_META_DATA);
        appSize = 0;
        if (applications != null) {
            for (int i = 0; i<applications.size(); i++) {
                ApplicationInfo app = applications.get(i);
                try {
                    Method getPackageSizeInfo = pm.getClass().getMethod(
                            "getPackageSizeInfo", String.class, IPackageStatsObserver.class);
                    getPackageSizeInfo.invoke(pm, app.packageName,
                            new IPackageStatsObserver.Stub() {
                        @Override
                        public void onGetStatsCompleted(PackageStats pStats, boolean succeeded)
                            throws RemoteException {
                                appSize += (pStats.codeSize + pStats.dataSize)/1000;
                            }
                    });
                } catch (Exception e) {
                }
            }
        }
        return appSize;
    }

    private int getBtPairNumber() {
        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        Set<BluetoothDevice> devices = adapter.getBondedDevices();
        int number = 0;
        if (devices != null) {
            for (BluetoothDevice device : devices) {
                if (device != null && device.getBondState() != BluetoothDevice.BOND_NONE) {
                    number++;
                }
            }
        }
        return number;
    }

    private String getBtPairName(int index) {
        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        Set<BluetoothDevice> devices = adapter.getBondedDevices();
        if (devices != null) {
            int count = 0;
            for (BluetoothDevice device : devices) {
                if ((count == index) && (device != null)
                        && (device.getBondState() != BluetoothDevice.BOND_NONE)) {
                    return device.getName();
                }
                count++;
            }
        }
        return "ERROR";
    }

    private String getAlarmTime(int index) {
        Cursor data = null;
        try {
            data = getContentResolver().query(Uri.parse
                    (getResources().getString(R.string.uri_alarm)), null, null, null, null);
        } catch (Exception e) {
            if (data != null)
                data.close();
            return "ERROR";
        }

        String result = "ERROR";
        if (data != null && data.moveToFirst()) {
            int i = 0;
            int indexOfHour = data.getColumnIndex("hour");
            int indexOfMin = data.getColumnIndex("minutes");

            do {
                if (i == index) {
                    try {
                        result = getString(R.string.timeformat, data.getInt(indexOfHour),
                                data.getInt(indexOfMin));
                    } catch (Exception e) {
                    }
                    break;
                }
                i++;
            } while (data.moveToNext());
            data.close();
        }
        return result;
    }

    private int getTimerCount() {
        try {
            Context context = createPackageContext(getResources().getString(R.string.pkg_countdown),
                    Context.MODE_WORLD_WRITEABLE);
            SharedPreferences prefs = context.getSharedPreferences(
                    context.getPackageName() + "_preferences", Context.MODE_WORLD_READABLE);

            File file = new File("/data/data" + context.getPackageName() + "/shared_prefs",
                    context.getPackageName() + "_preferences.xml");
            if (!file.exists() || !file.canRead())
                return 0;

            Object[] timerStrings = prefs.getStringSet("timers_list",
                    new HashSet<String>()).toArray();
            return timerStrings.length;

        } catch (Exception e) {
            return 0;
        }
    }

    private String getTimerTime(int index) {
        try {
            Context context = createPackageContext(getResources().getString(R.string.pkg_countdown),
                    Context.MODE_WORLD_WRITEABLE);
            SharedPreferences prefs = context.getSharedPreferences(
                    context.getPackageName() + "_preferences", Context.MODE_WORLD_READABLE);

            File file = new File("/data/data" + context.getPackageName() + "/shared_prefs",
                    context.getPackageName() + "_preferences.xml");

            if (!file.exists() || !file.canRead())
                return "ERROR";

            Object[] timerStrings = prefs.getStringSet("timers_list",
                    new HashSet<String>()).toArray();
            if (timerStrings.length > 0) {
                for (int i = 0; i < timerStrings.length; i++) {
                    if (i == index) {
                        String timerId = (String) timerStrings[i];
                        long sec = (long) prefs.getLong("timer_setup_timet_" + timerId, 0) / 1000;
                        return formatDuration(sec);
                    }
                }
            }

        } catch (Exception e) {
            return "ERROR";
        }

        return "ERROR";
    }

    private int getVoiceRecordCount() {
        int count = 0;
        File dir = Environment.getExternalStorageDirectory();
        if (dir == null)
            return 0;
        File[] files = dir.listFiles();
        for (File f : files) {
            if (f.isDirectory())
                continue;
            if (f.getName().startsWith("recording")) {
                ++count;
            }
        }
        return count;
    }

    private long getVoiceRecordSize() {
        long size = 0;
        File dir = Environment.getExternalStorageDirectory();
        if (dir == null)
            return 0;
        File[] files = dir.listFiles();
        for (File f : files) {
            if (f.isDirectory())
                continue;
            if (f.getName().startsWith("recording")) {
                size += f.length();
            }
        }
        return size / 1024;
    }

    private int getSimContactCount() {
        String service = isSim1 ? "simphonebook" : "simphonebook2";
        int size = 0;
        try {
            IIccPhoneBook iccIpb = IIccPhoneBook.Stub.asInterface(
                    ServiceManager.getService(service));
            List<AdnRecord> adnRecords = iccIpb.getAdnRecordsInEf(IccConstants.EF_ADN);
            final int N = adnRecords.size();
            for (int i = 0; i < N ; i++) {
                if (!adnRecords.get(i).isEmpty())
                    size++;
            }
        } catch (Exception e) {
        }
        return size;
    }

    private void factoryReset() {
        sendBroadcast(new Intent("android.intent.action.MASTER_CLEAR"));
    }

    // AT-Proxy Socket Client Receiver class
    class SocketClientThread extends Thread {
        @Override
        public void run() {
            int retryCount = 0;
            int retryDelay = SOCKET_OPEN_RETRY_MILLIS;

            try {
                while (flag) {
                    LocalSocket s = null;
                    LocalSocketAddress l;

                    // try to connect socket
                    try {
                        s = new LocalSocket();
                        l = new LocalSocketAddress(SOCKET_NAME_ATPROXY,
                                LocalSocketAddress.Namespace.RESERVED);
                        s.connect(l);
                    } catch (IOException ex) {
                        Log.d(TAG, "open socket fail: ", ex);

                        // connect fail
                        try {
                            if (s != null) {
                                s.close();
                            }
                        } catch (IOException ex2) {
                            // ignore failure to close after failure to connect
                        }
                        // don't print an error message after the the first time
                        // or after the 8th time
                        if (retryCount == 8) {
                            Log.d (TAG, "Couldn't find '" + SOCKET_NAME_ATPROXY
                                    + "' socket after " + retryCount
                                    + " times, continuing to retry silently");
                        } else if (retryCount > 0 && retryCount < 8) {
                            Log.d (TAG, "Couldn't find '" + SOCKET_NAME_ATPROXY
                                    + "' socket; retrying after timeout");
                        }

                        try {
                            /* maybe at-proxy service isn't runing, so dont wake up the system too
                               often in that case. Exponential retry with 20 minutes maximum */
                            if (retryDelay < 1200000)
                                retryDelay *= 2;
                            Thread.sleep(retryDelay);
                        } catch (InterruptedException er) {
                        }

                        retryCount++;
                        continue;
                    }
                    retryCount = 0;
                    mSocket = s;
                    handler.sendEmptyMessage(SOCKET_OPEN);

                    int length = 0;
                    // read data on socket
                    try {
                        InputStream inputStream = mSocket.getInputStream();
                        BufferedReader bufferedReader = new BufferedReader(
                                new InputStreamReader(inputStream));
                        String command = null;
                        while (flag) {
                            if ((bufferedReader.ready()) == true) {
                                try {
                                    command = bufferedReader.readLine();
                                    if (command != null && command.length() > 1) {
                                        Log.d(TAG,command);
                                        Message msg = new Message();
                                        msg.obj = command;
                                        msg.what = GET_COMMAND;
                                        // log msg
                                        handler.sendMessage(msg);
                                    }
                                } catch (java.io.IOException ex) {
                                    Log.e(TAG, "'" + SOCKET_NAME_ATPROXY +
                                            "' socket read exception", ex);
                                }
                            }
                        }
                    } catch (java.io.IOException ex) {
                        Log.d(TAG, "'" + SOCKET_NAME_ATPROXY + "' socket closed", ex);
                    } catch (Throwable tr) {
                        Log.e(TAG, "Uncaught exception read length=" + length + "Exception:" +
                                tr.toString());
                    }
                    Log.d(TAG, "Disconnected from '" + SOCKET_NAME_ATPROXY + "' socket");
                    try {
                        mSocket.close();
                    } catch (IOException ex) {
                        // ignore failure to close socket
                    }
                    mSocket = null;
                }
            } catch (Throwable tr) {
                Log.e(TAG, "Uncaught exception", tr);
            }
        }
    }
}
