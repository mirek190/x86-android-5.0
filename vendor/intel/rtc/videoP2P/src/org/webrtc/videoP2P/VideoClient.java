/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
package org.webrtc.videoP2P;

import java.util.ArrayList;
import java.util.List;
import java.io.File;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

import org.webrtc.videoP2P.Peer.PeerType;
import org.webrtc.videoengine.ViERenderer;
//import org.webrtc.videoengineapp.ViEAndroidJavaAPI;


import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.SensorManager;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Handler;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.OrientationEventListener;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.Spinner;
import android.widget.TextView;

import android.content.res.Configuration;
import android.hardware.Camera;
import android.view.Display;
import android.view.WindowManager;
import android.view.Surface;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.nio.ByteBuffer;
import android.opengl.GLSurfaceView;
import org.webrtc.videoP2P.PeerGLSurfaceView;
import android.view.WindowManager;

import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.RotateAnimation;

public class VideoClient extends Activity {
    private static boolean USE_VIDEO_ENGINE = false;


    final static String LOG_TAG = "j-libjingle-VideoClient";


    final static short CALL_CALLING = 0;
    final static short CALL_ANSWERED = 1;
    final static short CALL_REJECTED = 2;
    final static short CALL_INPROGRESS = 3;
    final static short CALL_RECIVEDTERMINATE = 4;
    final static short CALL_INCOMING = 5;
    final static short CALL_PRESENCE_DETECTED = 6;
    final static short CALL_PRESENCE_DROPPED = 7;


    final static short XMPPENGINE_CLOSED = 0;
    final static short XMPPENGINE_OPEN = 1;
    final static short XMPPENGINE_OPENING = 2;
    final static short XMPPENGINE_START = 3;

    final static short XMPPERROR_NONE = 0;
    final static short XMPPERROR_XML = 1;
    final static short XMPPERROR_STREAM = 2;
    final static short XMPPERROR_VERSION = 3;
    final static short XMPPERROR_UNAUTH = 4;
    final static short XMPPERROR_TLS = 5;
    final static short XMPPERROR_AUTH = 6;
    final static short XMPPERROR_BIND = 7;
    final static short XMPPERROR_CONN_CLOSED = 8;
    final static short XMPPERROR_DOC_CLOSED = 9;
    final static short XMPPERROR_SOCK_ERR = 10;
    final static short XMPPERROR_UNKNOWN = 11;

    final int MAX_CALLEE = 1;
    final int DIALING_TIMEOUT_MS = 10*1000;
    final int RINGING_TIMEOUT_MS = 15*1000;

    private Settings m_Settings;
    private boolean m_videoSetOn = true;
    private boolean m_voiceSetOn = true;

    private ConnectivityManager mCm;
    private AudioManager _audioManager;
    private Context mContext;
    private static boolean autoAccept = true;

    private static String def_myUser = "";
    private static String def_myPass = "";
    private static String def_myDomain = "gmail.com";

    private static String confFile = "/sdcard/videoP2Pconfig.txt";

    private String myUser = null;
    private String myPass = null;
    private String myDomain = null;

    private static String myXMPPServer = "talk.google.com";
    private static boolean useSSL = true;

    AlertDialog.Builder builder;
    AlertDialog alert;

    private static VideoClient _instance;
    private ProgressDialog callDialog;
    private AlertDialog mInComingDialog;

    private SurfaceView localSurfaceView = null;
    private LinearLayout remoteSurfaceLayout = null;
    private LinearLayout localSurfaceLayout = null;

    private ListView contactListView;
    private static PeerArrayAdapter listAdapter;
    // TODO: (khanh) Add flag to switch between user and native window
    //       private PeerGLSurfaceView remoteSurfaceView = null;
    private SurfaceView remoteSurfaceView = null;
    private LinearLayout mLlRemoteSurface = null;
    private LinearLayout mLlLocalSurface = null;
    private SurfaceView svLocal = null;

    private static ContactList contactList;


    //private ViEAndroidJavaAPI ViEAndroidAPI = null;
    private boolean enableVoice = false;
    private boolean enableVideo = true;
    private boolean enableVideoSend = true;
    private boolean enableVideoReceive = true;
    private boolean useOpenGLRender = false;
    private boolean vieRunning = false;

    // debug only with non-vie api
    private boolean enableStartCapture = false; //!USE_VIDEO_ENGINE;
    private boolean useVideoEngine = USE_VIDEO_ENGINE;

    private boolean enableTrace = false;
    private boolean enablePreview = false;

    private boolean previewCameraActive = false;

    private int channel = 0;
    private int cameraId = 1;
    private int voiceChannel = -1;
    private boolean usingFrontCamera = true;
    private OrientationEventListener orientationListener;
    int currentOrientation = OrientationEventListener.ORIENTATION_UNKNOWN;
    int currentCameraOrientation = 0;
    private int currentQuadrant = -1;
    private final int rotationThreshold = 67;

    private int codecType = 0;
    private int codecSizeWidth = 352;
    private int codecSizeHeight = 288;

    private static final int RECEIVE_CODEC_FRAMERATE = 30;
    private static final int SEND_CODEC_FRAMERATE = 30;
    private static final int INIT_BITRATE = 400;
    private static Handler mHandler;
    private LayoutInflater inflater;
    private native int SetCamera(int deviceId, String deviceUniqueStr);
    private native int SetImageOrientation(int degrees);
    private native int SetRemoteSurfaceView(SurfaceView surface);
    private native int SetRemoteSurface(Object glSurface);
    private native int SetBufferCount(int count);
    private native int SetBuffer(ByteBuffer[] yuv, int index);
    private native int SetBufferDone(boolean done);

    private EditText fromUserField;
    private EditText fromPasswordField;

    private AudioJackReceiver audioJackReceiver;
    private ConnectivityChangeHandler mConnectivityChangeHandler;

    private int lastAudioMode;
    private boolean wasSpeakerphoneOn;

    enum CallLayout {
        MAIN,
        CONTACT,
        INCALL,
    }
    private CallLayout currentLayout = CallLayout.MAIN;

    public VideoClient() {
        super();
    }

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mContext = this;
        _instance = this;
        mCm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        audioJackReceiver = new AudioJackReceiver();
        mConnectivityChangeHandler = new ConnectivityChangeHandler();
        mHandler = new Handler();
        if(mHandler == null) {
            Log.e(LOG_TAG, "Unable to create handler");
        }
        contactList = new ContactList();
        callDialog = new ProgressDialog(this);
        m_Settings = new Settings(this,IsTestModeActive());

        //Without this, audio record buffer was not being emptied as fast as it was filled.
        //We probably only want to do this when a call is actually happening.
        android.os.Process.setThreadPriority(
                android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);

        // Load the user credential configs from confFile
        // Warning: For real credentials, use encrypted storage
        // This is for development convenience
        BufferedReader reader = null;
        try {
            File conf = new File(confFile);
            reader = new BufferedReader(new FileReader(conf));

            String toUser = reader.readLine();
            myUser = reader.readLine();
            myPass = reader.readLine();
            myDomain = reader.readLine();
        } catch (IOException e) {
            Log.e("*Webrtc*", "I/O error");
        } finally {
            if (reader != null) {
              try {
                reader.close();
                reader = null;
              } catch (IOException ioe) {
                Log.e("*Webrtc*", "I/O error");
              }
            }
        }

        if(myDomain == null || myDomain.length() == 0) {
            // There was a failure, fallback to hardcoded defaults
            myUser = def_myUser;
            myPass = def_myPass;
            myDomain = def_myDomain;
        }

        orientationListener =
            new OrientationEventListener(this,SensorManager.SENSOR_DELAY_UI) {
                public void onOrientationChanged (int orientation) {
                    //Log.d(LOG_TAG,"onOrientationChanged("+orientation+")");
                    if(currentLayout==CallLayout.INCALL){
                        if (orientation != ORIENTATION_UNKNOWN) {
                            currentOrientation = orientation;
                            int quadrant = getQuadrant(orientation);
                            if (currentQuadrant != quadrant) {
                                setQuadrant(quadrant);
                                currentQuadrant = quadrant;
                                SetImageOrientation(quadrant*90);
                            }
                        }
                    }
                }
            };
        orientationListener.enable ();

        localSurfaceView = ViERenderer.CreateLocalRenderer(this);
        Log.v(LOG_TAG, "Create Local Render");
        if (localSurfaceView == null) {
            Log.e(LOG_TAG, "[StartMain]: Failed to create local surface view");
        }

        // TODO: (khanh) add flag for switching between NativeWindow and SW Rendering
        if(useOpenGLRender) {
            Log.v(LOG_TAG, "Create Remote OpenGL Render");
            //remoteSurfaceView = ViERenderer.CreateRenderer(this, true);
            remoteSurfaceView = new PeerGLSurfaceView(this);
        } else {
            Log.v(LOG_TAG, "Create Remote SurfaceView Render");
            remoteSurfaceView = ViERenderer.CreateRenderer(this, false);
        }
        try {
            SetRemoteSurfaceView(remoteSurfaceView);
            SetRemoteSurface(remoteSurfaceView.getHolder().getSurface());
        } catch (Exception e) {
            Log.d(LOG_TAG, "SetRemoteSurface ***[Exception]***");
            Log.d(e.toString(), "***");
        }

        switchLayoutTo(CallLayout.MAIN);
    }

    @Override
        public void onResume() {
            IntentFilter filter = new IntentFilter(Intent.ACTION_HEADSET_PLUG);
            registerReceiver(audioJackReceiver, filter);
            registerReceiver(mConnectivityChangeHandler, new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));
            super.onResume();
        }

    @Override
        public void onPause() {
            if (currentLayout == CallLayout.INCALL) {
                _instance.HangUp();
            }
            switchLayoutTo(CallLayout.MAIN);

            unregisterReceiver(mConnectivityChangeHandler);
            unregisterReceiver(audioJackReceiver);
            super.onPause();
        }

    @Override
        public void onStart() {
        //If it's playing audio out of the speaker, switch this to get earpiece.
        if (_audioManager == null) {
            _audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        }

        if (_audioManager == null) {
            Log.e("*Webrtc*", "Could not change audio routing - no audio manager");
        } else {
            lastAudioMode = _audioManager.getMode();
            wasSpeakerphoneOn = _audioManager.isSpeakerphoneOn();
            _audioManager.setMode(AudioManager.MODE_NORMAL);
            _audioManager.setSpeakerphoneOn(true);
        }
        super.onStart();
    }

    @Override
    public void onStop() {
        if (_audioManager != null) {
          // restore audio manager setting
          _audioManager.setMode(lastAudioMode);
          _audioManager.setSpeakerphoneOn(wasSpeakerphoneOn);
        }
        super.onStop();
        Destroy();
    }

    public void sendLogin(View v) {

        myUser = fromUserField.getText().toString();
        myPass = fromPasswordField.getText().toString();

        if((myUser.length()==0) || (myPass.length()==0) ) {
            showDialog(0);
        }
        else {
            VideoClient.contactList.getList().clear();
            Login(myUser, myPass, myDomain, myXMPPServer, useSSL);
            _instance.switchLayoutTo(CallLayout.CONTACT);
        }
    }

    private ContactList getContactList(){
        return contactList;
    }

    protected void onDestroy() {
        super.onDestroy();
    }

    void switchLayoutTo(CallLayout layout) {
        currentLayout = layout;
        if(localSurfaceLayout!=null) localSurfaceLayout.removeAllViews();
        if(remoteSurfaceLayout!=null) remoteSurfaceLayout.removeAllViews();
        if(isDialogShowing()) cancelDialog();

        if(layout==CallLayout.MAIN){
            setMainLayout();
        }
        else if(layout==CallLayout.CONTACT) {
            // Need to set orientation after Login as Login
            // creates the thread and Conductor object.
            setContactLayout();
        }
        else if(layout==CallLayout.INCALL) {
            usingFrontCamera = true;
            SelectCamera(Camera.CameraInfo.CAMERA_FACING_FRONT);
            StartSurface();
        }
    }

    private void SelectCamera(int requestFacing) {
        int cameraId = 0;
        String cameraUniqueName = "";
        Camera.CameraInfo info = new Camera.CameraInfo();
        for (int i=0; i < Camera.getNumberOfCameras(); i++) {
            Camera.getCameraInfo(i, info);
            if (info.facing == requestFacing) {
                cameraId = i;
            }
        }

        Camera.getCameraInfo(cameraId, info);

        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            cameraUniqueName = "Camera " + cameraId + ", Facing front, Orientation "+info.orientation;
        }
        else if (info.facing == Camera.CameraInfo.CAMERA_FACING_BACK) {
            cameraUniqueName = "Camera " + cameraId + ", Facing back, Orientation "+info.orientation;
        }

//        SetImageOrientation(info.orientation);
        SetCamera(cameraId, cameraUniqueName);
    }
    private void setMainLayout() {
        setContentView(R.layout.mainview);
        fromUserField = (EditText) findViewById(R.id.fromusertext);
        fromPasswordField = (EditText) findViewById(R.id.frompasstext);
        fromUserField.setText(myUser);
        fromPasswordField.setText(myPass);
        TextView conn = (TextView)findViewById(R.id.login_connectivity);
        Button login = (Button) findViewById(R.id.Login);
        if(mConnectivityChangeHandler.isConnected()) {
            conn.setText(null);
            login.setEnabled(true);
        } else {
            conn.setText(mContext.getString(R.string.no_connection));
            login.setEnabled(false);
        }
    }

    private void setContactLayout() {
        InputMethodManager keyManager = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        keyManager.hideSoftInputFromWindow(fromPasswordField.getWindowToken(),0);

        setContentView(R.layout.contactview);
        contactListView = (ListView) findViewById(R.id.contactlistview);
        listAdapter = new PeerArrayAdapter(this, getContactList());
        contactListView.setAdapter(listAdapter);

        //add filter
        Spinner spinner = (Spinner) findViewById(R.id.filterspinner);
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item,
                new String[]{"All", "Intel", "Gmail"});
        spinner.setAdapter(adapter);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setOnItemSelectedListener(new Spinner.OnItemSelectedListener(){
                @Override
                public void onItemSelected(AdapterView<?> parent,
                    View view, int position, long id) {
                PeerType type = null;
                if(position==1) type = PeerType.INTEL;
                else if(position==2) type = PeerType.GMAIL;

                listAdapter.setFilter(type);
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {

                }
                });
        spinner.setSelection(1);

        RelativeLayout.LayoutParams params2 = new RelativeLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        params2.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);

        LinearLayout rl_callbuttons_extras = (LinearLayout)findViewById(R.id.rl_callbutton_extras);
        if( IsTestModeActive() ) {
            rl_callbuttons_extras.setVisibility( LinearLayout.VISIBLE );
        }

        Button callButton = (Button)findViewById(R.id.callbutton);
        callButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View arg0) {
                    String callee = contactList.getSelection();
                    if(TextUtils.isEmpty(callee)) return;

                    switchLayoutTo(CallLayout.INCALL); // call is placed when surface is created
                    PlaceCall(callee, true, true);
                }
        });

        Button videoonly_callButton = (Button)findViewById(R.id.callbutton_videoonly);
        videoonly_callButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View arg0) {
                String callee = contactList.getSelection();
                if(TextUtils.isEmpty(callee)) return;
                    switchLayoutTo(CallLayout.INCALL); // call is placed when surface is created
                    PlaceCall(callee, true, false);
                }
                });

        Button audioonly_callButton = (Button)findViewById(R.id.callbutton_audioonly);
        audioonly_callButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View arg0) {
                String callee = contactList.getSelection();
                if(TextUtils.isEmpty(callee)) return;
                    switchLayoutTo(CallLayout.INCALL); // call is placed when surface is created
                    PlaceCall(callee, false, true);
                }
                });

        TextView conn = (TextView)findViewById(R.id.dial_connectivity);
        if(mConnectivityChangeHandler.isConnected()) {
            conn.setText(null);
            callButton.setEnabled(true);
        } else {
            conn.setText(mContext.getString(R.string.no_connection));
            callButton.setEnabled(false);
        }
    }

    @Override
        public boolean onKeyUp(int keyCode, KeyEvent event) {
            if(keyCode == KeyEvent.KEYCODE_MENU) {
                if( ( currentLayout != null ) && ( (currentLayout == CallLayout.MAIN) || (currentLayout == CallLayout.CONTACT) ) ) {
                    m_Settings.Show();
                    return true;
                }
            }
            return super.onKeyUp(keyCode, event);
        }

    private void StartSurface() {
        setContentView(R.layout.videoview);
        m_videoSetOn = true;
        m_voiceSetOn = true;
        localSurfaceLayout = (LinearLayout) findViewById(R.id.localView);
        if (localSurfaceLayout == null) {
            Log.e(LOG_TAG, "[StartMain]: Failed to create local surface layout");
        }
        else {
            localSurfaceLayout.removeAllViews();
            if (localSurfaceView == null) {
                Log.e(LOG_TAG, "[StartMain]: Failed to create local surface view for layout");
            }
            else if (localSurfaceLayout != null) {
                Log.d(LOG_TAG, "[StartMain]: LocalSurfaceView added to Layout");
                try {
                    localSurfaceLayout.addView(localSurfaceView);
                } catch (Exception e) {
                    Log.d(LOG_TAG, "***[Exception]***");
                    Log.d(e.toString(), "***");
                }
            }
        }

        if (enableVideoReceive) {
            remoteSurfaceLayout = (LinearLayout) findViewById(R.id.remoteView);
            if (remoteSurfaceLayout == null) {
                Log.e(LOG_TAG, "[StartMain]: Failed to create remote surface layout");
            }
            else {
                remoteSurfaceLayout.removeAllViews();
                if (remoteSurfaceLayout != null) {
                    Log.d(LOG_TAG, "[StartMain]: RemoteSurfaceView added to Layout");
                    try {
                        remoteSurfaceLayout.addView(remoteSurfaceView);
                    } catch (Exception e) {
                        Log.d(LOG_TAG, "***[Exception]***");
                        Log.d(e.toString(), "***");
                    }
                }
            }
        }


    }

    protected Dialog onCreateDialog(int id) {
        builder = new AlertDialog.Builder(this);
        builder.setMessage("Please enter a valid username or password")
            .setCancelable(false)
            .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                    removeDialog(id);
                    }
                    });
        alert = builder.create();

        return alert;
    }

    /*  TODO (khanh) add flag to switch between NW and SW Rendering
        private final int mBufferCount = 30;
        private ByteBuffer mYUVPlane[][] = new ByteBuffer[mBufferCount][3];
        private ByteBuffer mDeparture = null;
        private int mWidth = 0;
        private int mHeight = 0;

        public int popFromArrivalQueue() {
        return remoteSurfaceView.popFromArrivalQueue();
        }

        public void pushIntoDepartureQueue(int value) {
        remoteSurfaceView.pushIntoDepartureQueue(value);
        }

        public void renderSize(int width, int height) {

        SetBufferCount(mBufferCount);

        for( int i = 0; i < mBufferCount; i++ ) {
        mYUVPlane[i][0] = ByteBuffer.allocateDirect(width * height);
        mYUVPlane[i][0].clear();
        mYUVPlane[i][1] = ByteBuffer.allocateDirect(width * height / 4);
        mYUVPlane[i][1].clear();
        mYUVPlane[i][2] = ByteBuffer.allocateDirect(width * height / 4);
        mYUVPlane[i][2].clear();

        SetBuffer(mYUVPlane[i], i);
        }

        mWidth = width;
        mHeight = height;

        remoteSurfaceView.queueEvent(new Runnable() {
        @Override
        public void run() {
        remoteSurfaceView.setSize(mWidth, mHeight);
        remoteSurfaceView.setYUVPlanes(mYUVPlane);
        SetBufferDone(true);
        }
        });
        }
        */
    private native static boolean NativeInit();
    private native int Login(String user_name, String password, String domain, String server, boolean UseSSL);
    private native int PlaceCall(String user_name, boolean video, boolean audio);
    private native int HangUp();
    private native int Destroy();
    private native int AcceptCall(boolean video, boolean audio);
    private native int RejectCall();
    private native boolean SetVideo(boolean enable);
    private native boolean SetVoice(boolean enable);
    private native String GetCaller();
    private native boolean IsTestModeActive();
    private native boolean IsIncomingCallSupportVideo();
    private native boolean IsIncomingCallSupportAudio();

    private void RejectAndSwitchView() {
        switchLayoutTo(CallLayout.CONTACT);
        RejectCall();
    }

    public static void callBackXMPPError(String code) { 
        short shortCode = new Short(code);
        switch(shortCode){
            case VideoClient.XMPPERROR_NONE:
                break;
            case VideoClient.XMPPERROR_XML:
                break;
            case VideoClient.XMPPERROR_STREAM:
                break;
            case VideoClient.XMPPERROR_VERSION:
                break;
            case VideoClient.XMPPERROR_UNAUTH:
                break;
            case VideoClient.XMPPERROR_TLS:
                break;
            case VideoClient.XMPPERROR_AUTH:
                break;
            case VideoClient.XMPPERROR_BIND:
                break;
            case VideoClient.XMPPERROR_CONN_CLOSED:
                break;
            case VideoClient.XMPPERROR_DOC_CLOSED:
                break;
            case VideoClient.XMPPERROR_SOCK_ERR:
                break;
            case VideoClient.XMPPERROR_UNKNOWN:
            default:
                break;
        }
        if(shortCode != VideoClient.XMPPERROR_NONE){
            Log.e(LOG_TAG, "AndroidTest.callBackXMPPError: " + code);
        }
    }

    public static void callBackCallState(String code) { 
        final short shortCode = new Short(code);
        mHandler.post(new Runnable() {
                public void run() {
                _instance.handleCallStateChange(shortCode);
                }
                });
        Log.d(LOG_TAG, "AndroidTest.callBackCallState: " + code);
    }

    private void handleCallStateChange(short code) {
        switch(code){
            case VideoClient.CALL_CALLING:
                handleOutgoingCall();
                callDialog.setMessage("Calling...");
                callDialog.getButton(DialogInterface.BUTTON_NEGATIVE).setText("Cancel");
                break;
            case VideoClient.CALL_ANSWERED:
                callDialog.dismiss();
                break;
            case VideoClient.CALL_REJECTED:
                _instance.switchLayoutTo(CallLayout.CONTACT);
                callDialog.setMessage("Call Rejected.");
                break;
            case VideoClient.CALL_INPROGRESS:
                SetImageOrientation(currentQuadrant * 90);
                setQuadrant(currentQuadrant);
                break;
            case VideoClient.CALL_RECIVEDTERMINATE:
                if(mInComingDialog!=null && mInComingDialog.isShowing()) {
                    //remote cancel call
                    mInComingDialog.dismiss();
                }
                callDialog.setMessage("Call Terminated.");
                Button btn = callDialog.getButton(DialogInterface.BUTTON_NEGATIVE);
                if(btn!=null) btn.setText("Close");
                callDialog.dismiss();
                _instance.switchLayoutTo(CallLayout.CONTACT);
                break;
            case VideoClient.CALL_INCOMING:
                if (_instance == null) {
                    Log.d(LOG_TAG, "Incoming call from unknown, _instance is null!");
                }
                else {
                    Log.d(LOG_TAG, "Incoming call from " + _instance.GetCaller());
                    switchLayoutTo(CallLayout.INCALL);
                    handleIncomingCall();
                }
                break;
            default:
                //Shouldn't hit this case
                break;
        }
    }

    private void handleOutgoingCall() {
        final Runnable dialTimeoutWork = new Runnable() {
            @Override
            public void run() {
                if(callDialog.isShowing()) {
                    callDialog.dismiss();
                    _instance.onHangup(null);
                    Log.i(LOG_TAG, "dialTimeoutWork cancel call");
                }
            }
        };
        callDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        callDialog.setTitle("Call Status");
        callDialog.setCancelable(false);
        callDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "dismiss", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                _instance.onHangup(null);
                }
                });
        callDialog.setOnDismissListener(new OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface arg0) {
                mHandler.removeCallbacks(dialTimeoutWork);
            }
        });
        callDialog.show();
        mHandler.postDelayed(dialTimeoutWork, DIALING_TIMEOUT_MS);
    }

    private void handleIncomingCall() {
        if( !IsTestModeActive() ) {
            // Put up the Accept/Reject message box
            AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
            mInComingDialog = builder
                .setTitle("Incoming call")
                .setMessage("Accept call?")
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setPositiveButton("Accept", new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                        // Accept button clicked
                        Log.d(LOG_TAG, "Accepting incoming call from " + _instance.GetCaller());
                        _instance.AcceptCall(true, true);
                        mInComingDialog = null;
                        }
                        })
            .setNegativeButton("Reject", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                    // Reject button clicked
                    Log.d(LOG_TAG, "Rejecting incoming call from " + _instance.GetCaller());
                    _instance.RejectAndSwitchView();
                    mInComingDialog = null;
                    }
                    })
            .show(); 
        } else {
            final boolean video = IsIncomingCallSupportVideo();
            final boolean audio = IsIncomingCallSupportAudio();
            CharSequence[] accept_choices = {"Accept", "Reject"};
            if(video && !audio)
                accept_choices[0] = "Accept - Video only";
            else if(!video && audio)
                accept_choices[0] = "Accept - Audio only";
            else if(!video && !audio)
                Log.e(LOG_TAG, "Error: No supported media, or calling IsIncomingCallSupport in outgoing call");

            AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
            mInComingDialog = builder
                .setTitle("Incoming call")
                .setItems(accept_choices, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                        switch(which) {
                        case 0:
                        // Accept button clicked
                        Log.d(LOG_TAG, "Accepting incoming call from " + _instance.GetCaller());
                        _instance.AcceptCall(video, audio);
                        break;
                        case 1:
                        // Reject button clicked
                        Log.d(LOG_TAG, "Rejecting incoming call from " + _instance.GetCaller());
                        _instance.RejectAndSwitchView();
                        break;
                        }

                        mInComingDialog = null;
                        }
                })
            .show();
        }
        final Runnable ringingTimeoutWork = new Runnable() {
            @Override
            public void run() {
                if(mInComingDialog.isShowing()) {
                    mInComingDialog.dismiss();
                    _instance.switchLayoutTo(CallLayout.CONTACT);
                    Log.i(LOG_TAG, "ringingTimeoutWork cancel call");
                }
            }
        };
        mInComingDialog.setOnDismissListener(new OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface arg0) {
                mHandler.removeCallbacks(ringingTimeoutWork);
            }
        });
        mHandler.postDelayed(ringingTimeoutWork, RINGING_TIMEOUT_MS);
    }

    private boolean isDialogShowing() {
        return ( (mInComingDialog!=null && mInComingDialog.isShowing()) || (callDialog!=null && callDialog.isShowing()) );
    }

    private void cancelDialog() {
        if(mInComingDialog!=null && mInComingDialog.isShowing()) {
            mInComingDialog.dismiss();
        } else if(callDialog!=null && callDialog.isShowing()) {
            callDialog.dismiss();
        }
    }

    public static void callBackXMPPEngineState(String code) {
        short shortCode = new Short(code);
        switch(shortCode){
            case VideoClient.XMPPENGINE_CLOSED:
                Log.i(LOG_TAG, "Logged Out.");
                break;
            case VideoClient.XMPPENGINE_OPEN:
                Log.i(LOG_TAG, "Logged In.");
                break;
            case VideoClient.XMPPENGINE_OPENING:
                break;
            case VideoClient.XMPPENGINE_START:
                break;
            default:
                break;
        }
        Log.d(LOG_TAG, "AndroidTest.callBackXMPPEngineState: " + code);
    }

    public static void callBackPresenceState(String code, String data) {
        short shortCode = new Short(code);
        System.out.println("DISHA Code:" + shortCode);
        switch(shortCode){
            case VideoClient.CALL_PRESENCE_DETECTED:
                Log.i(LOG_TAG, "VideoClient.callBackPresenceState - Presence Detected: " + data);
                contactList.add(data);

                break;
            case VideoClient.CALL_PRESENCE_DROPPED:
                Log.i(LOG_TAG, "VideoClient.callBackPresenceState- Presence Dropped: " + data);
                contactList.remove(data);
                break;
            default:
                break;
        }
        mHandler.post(new Runnable() {
                public void run(){
                _instance.updateListWithContacts();
                }
                });
    }

    private void updateListWithContacts() {
        // TODO Auto-generated method stub
        listAdapter.notifyDataSetChanged(); 
    }

    public void onHangup(View v) {
        _instance.HangUp();
        this.switchLayoutTo(CallLayout.CONTACT);
    }

    public void onSetVideo(View v) {
        try {
            boolean result = _instance.SetVideo( !m_videoSetOn );
            Log.e(LOG_TAG, "Set video result :" + result );

            if( result ) {
                ImageButton videoSetButton = (ImageButton) _instance.findViewById( R.id.videoSetButton );
                m_videoSetOn = !m_videoSetOn;
                videoSetButton.setImageResource( m_videoSetOn ? R.drawable.video_on : R.drawable.video_off );
            }
        } catch(Exception e) {
            Log.e(LOG_TAG, "Unable to set video : " + e.getMessage() );
        }
    }

    public void onSetVoice(View v) {
        try {
            boolean result = _instance.SetVoice( !m_voiceSetOn );
            Log.e(LOG_TAG, "Set voice result :" + result );

            if( result ) {
                ImageButton voiceSetButton = (ImageButton) _instance.findViewById( R.id.voiceSetButton );
                m_voiceSetOn = !m_voiceSetOn;
                voiceSetButton.setImageResource( m_voiceSetOn ? R.drawable.voice_on : R.drawable.voice_off );
            }
        } catch(Exception e) {
            Log.e(LOG_TAG, "Unable to set voice : " + e.getMessage() );
        }
    }

    public void onSwitchCamera(View v) {
        usingFrontCamera = !usingFrontCamera;
        SelectCamera(usingFrontCamera ? Camera.CameraInfo.CAMERA_FACING_FRONT
                : Camera.CameraInfo.CAMERA_FACING_BACK);
    }

    private int getQuadrant(int orientation) {
        int quadrant=currentQuadrant;
        int mindeg = (((currentQuadrant==0)?4:currentQuadrant) * 90) - rotationThreshold;
        int maxdeg = (currentQuadrant * 90) + rotationThreshold;

        switch(currentQuadrant){
        case 0:
            if ((orientation > maxdeg) && (orientation < (maxdeg + 90))){
                quadrant = 1;
            }
            else if((orientation < mindeg) && (orientation > (mindeg - 90))){
                quadrant = 3;
            }
            break;
        case 1:
            if (orientation > maxdeg){
                quadrant = 2;
            }
            else if (orientation < mindeg){
                quadrant = 0;
            }
            break;
        case 2:
            if (orientation > maxdeg){
                quadrant = 3;
            }
            else if (orientation < mindeg){
                quadrant = 1;
            }
            break;
        case 3:
            if (orientation > maxdeg){
                quadrant = 0;
            }
            else if (orientation < mindeg){
                quadrant = 2;
            }
            break;
         default:
            if (orientation <= 45 || orientation > 315)
                quadrant = 0;
            else if (orientation > 45 && orientation <= 135)
                quadrant = 1;
            else if (orientation > 135 && orientation <=225)
                quadrant = 2;
            else if (orientation > 225 && orientation <= 315)
                quadrant = 3;
        }
        return quadrant;
    }

    private void setQuadrant(int quadrant) {
        final ImageButton hangup = (ImageButton)findViewById(R.id.hangupButton);
        final ImageButton videoSet = (ImageButton)findViewById(R.id.videoSetButton);
        final ImageButton voiceSet = (ImageButton)findViewById(R.id.voiceSetButton);
        final ImageButton switchCamera = (ImageButton)findViewById(R.id.switchCameraButton);

        boolean bClockwise = false;
        float fromDegree=0.0f, toDegree=90.0f;
        switch (quadrant) {
            case 0:
            default:
                if (currentQuadrant == 1)
                    bClockwise = true;
                if (bClockwise) {
                    fromDegree=270.0f;
                    toDegree=360.0f;
                }
                else {
                    fromDegree=90.0f;
                    toDegree=0.0f;
                }
                break;

            case 1:
                if (currentQuadrant == 2)
                    bClockwise = true;
                if (bClockwise) {
                    fromDegree=180.0f;
                    toDegree=270.0f;
                }
                else {
                    fromDegree=360.0f;
                    toDegree=270.0f;
                }
                break;

            case 2:
                if (currentQuadrant == 3)
                    bClockwise = true;
                if (bClockwise) {
                    fromDegree=90.0f;
                    toDegree=180.0f;
                }
                else {
                    fromDegree=270.0f;
                    toDegree=180.0f;
                }
                break;

            case 3:
                if (currentQuadrant == 0)
                    bClockwise = true;
                if (bClockwise) {
                    fromDegree=0.0f;
                    toDegree=90.0f;
                }
                else {
                    fromDegree=180.0f;
                    toDegree=90.0f;
                }
                break;
        }

        RotateAnimation rAnim = new RotateAnimation(fromDegree, toDegree, RotateAnimation.RELATIVE_TO_SELF, 0.5f, RotateAnimation.RELATIVE_TO_SELF, 0.5f);
        rAnim.setStartOffset(0);
        rAnim.setDuration(500);
        rAnim.setFillAfter(true);
        rAnim.setFillEnabled(true);
        hangup.setAnimation(rAnim);
        voiceSet.setAnimation(rAnim);
        videoSet.setAnimation(rAnim);
        switchCamera.setAnimation(rAnim);
        rAnim.setAnimationListener(new AnimationListener() {
                public void onAnimationStart(Animation anim) {
                }

                public void onAnimationRepeat(Animation anim) {
                }

                public void onAnimationEnd(Animation anim) {
                hangup.setImageResource(R.drawable.hangup);
                voiceSet.setImageResource( m_voiceSetOn ? R.drawable.voice_on : R.drawable.voice_off );
                videoSet.setImageResource( m_videoSetOn ? R.drawable.video_on : R.drawable.video_off );
                switchCamera.setImageResource(R.drawable.switch_cam);
                }
                });
        hangup.startAnimation(rAnim);
    }

    @Override
        public void onConfigurationChanged(Configuration newConfig) {

            super.onConfigurationChanged(newConfig);
            /* handle this later
               cameraId = 1;
               Camera.CameraInfo info = new Camera.CameraInfo();
               Camera.getCameraInfo(cameraId, info);

               int rotation = getWindowManager().getDefaultDisplay().getRotation();
               int degrees = 0;
               switch(rotation) {
               case Surface.ROTATION_0: degrees = 0; break;
               case Surface.ROTATION_90: degrees = 90; break;
               case Surface.ROTATION_180: degrees = 180; break;
               case Surface.ROTATION_270: degrees = 270; break;
               }

               int result;
               if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
               result = (info.orientation + degrees) % 360;
               result = (360 - result) % 360;
               } else {
               result = (info.orientation - degrees + 360) % 360;
               }
               if (previewCameraActive) {
               SetPreviewRotation(result);
               } else {
               SetLocalViewRotation(result);
               }
               */
        }

    static {
        //System.loadLibrary("stlport_shared");
        //System.loadLibrary("crypto_jingle");
        //System.loadLibrary("webrtc_audio_preprocessing");
        //System.loadLibrary("webrtc_voice");
        //System.loadLibrary("jingle");
        //System.loadLibrary("webrtc_video");
        System.load("/system/lib/videoP2P/libwebrtc-video-p2p-jni.so");

        if (!NativeInit()) {
            Log.e(LOG_TAG, "Native init failed");
            throw new RuntimeException("Native init failed");
        } else {
            Log.d(LOG_TAG, "Native init successful");
        }
    }



    private class AudioJackReceiver extends BroadcastReceiver {

        private final static String TAG = "AudioJackReceiver";

        @Override
            public void onReceive(Context context, Intent intent) {
                //Log.d(TAG, "headset state received");
                if (_audioManager != null &&
                    intent.getAction().equals(Intent.ACTION_HEADSET_PLUG)) {
                    int state = intent.getIntExtra("state", -1);
                    switch (state) {
                        case 0:
                            //Log.d(TAG, "headset is unplugged");
                            _audioManager.setSpeakerphoneOn(true);
                            break;
                        case 1:
                            //Log.d(TAG, "headset is plugged-in");
                            _audioManager.setSpeakerphoneOn(false);
                            break;
                        default:
                            //Log.d(TAG, "unknown headset state="+state);
                            _audioManager.setSpeakerphoneOn(true);
                            break;
                    }
                }
            }
    }

    class ConnectivityChangeHandler extends BroadcastReceiver {
        static final int CANCEL_CALL_TIMEOUT_MS = 30*1000;
        Boolean mConnected = null;

        boolean isConnected() {
            NetworkInfo ni = _instance.mCm.getActiveNetworkInfo();
            return (ni!=null && ni.isConnected());
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            boolean conn = isConnected();
            if(mConnected!=null && mConnected==conn) return;
            else {
                Log.d(LOG_TAG, "onReceive mConnected=" + mConnected);
                mConnected = conn;
            }

            if(_instance.currentLayout==CallLayout.MAIN) {
                //update login button
                _instance.switchLayoutTo(CallLayout.MAIN);
            } else if(_instance.currentLayout==CallLayout.CONTACT) {
                if(!conn) {
                     //to login layout, because XMPP already disconnected
                     _instance.switchLayoutTo(CallLayout.MAIN);
                     VideoClient.contactList.getList().clear();
                }
            } else if(_instance.currentLayout==CallLayout.INCALL) {
                if(conn) {
                    mHandler.removeCallbacks(cancelCallTask);
                } else if(_instance.isDialogShowing()) {
                    _instance.switchLayoutTo(CallLayout.CONTACT);
                } else {
                    mHandler.removeCallbacks(cancelCallTask);
                    mHandler.postDelayed(cancelCallTask, CANCEL_CALL_TIMEOUT_MS);
                }
            }
        }

        final Runnable cancelCallTask = new Runnable() {
            @Override
            public void run() {
                Log.d(LOG_TAG, "cancelCallTask hangup()");
                _instance.HangUp();
                _instance.switchLayoutTo(CallLayout.MAIN);
            }
        };

    }
}
