/*
 * libjingle
 * Copyright 2013, Intel Inc.
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

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Properties;

import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.Spinner;

public class Settings {

    private final String LOG_TAG = "WebRTCClient-Settings";
    private final String NONE_SELECTED = "Default";
    private final String GENERAL_SETTINGS_FILE_NAME = "/webrtc.gen.settings";
    private final String EXTENDED_SETTINGS_FILE_NAME = "/webrtc.ext.settings";

    private int[][] Video_Resolutions_List = {
        {1920, 1080}, {1280, 720}, {1024, 576}, {800, 600}, {720, 480},
        {640, 480}, {640, 360}, {352, 288}, {320, 240}, {176, 144}
    };

    private double[] Video_FrameRate_List = {
        30.0, 29.97, 25.0, 24.0, 23.97, 15, 10
    };

    private int[][] Video_Aspect_Ratio_List = {
        {16, 9}, {5, 3}, {16, 10}, {3, 2}, {4, 3}
    };

    private String[] Video_Codecs_List = {
        "VP8", "H.264"
    };

    private String[] Audio_Codecs_List = {
        "G.711", "G.722", "iLBC", "iSAC", "Opus"
    };

    //Mbps
    private double[] Video_Bitrate_List = {
        1.5, 3.5, 5, 8, 12, 15
    };

    //Kbps
    private double[] Audio_Bitrate_List = {
        96, 128, 160, 192, 320
    };

    public class Config_GeneralSettings {
        public boolean videoResMinSet;
        public int videoResWidthMin;
        public int videoResHeightMin;

        public boolean videoResMaxSet;
        public int videoResWidthMax;
        public int videoResHeightMax;

        public boolean videoAspectRatioMinSet;
        public int videoAspectRatioNumerMin;
        public int videoAspectRatioDenomMin;

        public boolean videoAspectRatioMaxSet;
        public int videoAspectRatioNumerMax;
        public int videoAspectRatioDenomMax;

        public boolean videoFrameRateMinSet;
        public double videoFrameRateMin;

        public boolean videoFrameRateMaxSet;
        public double videoFrameRateMax;

        public Config_GeneralSettings() {
            videoResMinSet = false;
            videoResMaxSet = false;
            videoAspectRatioMinSet = false;
            videoAspectRatioMaxSet = false;
            videoFrameRateMinSet = false;
            videoFrameRateMaxSet = false;
        }
    };

    public class Config_ExtendedSettings {
        public boolean videoCodecSet;
        public String videoCodec;

        public boolean audioCodecSet;
        public String audioCodec;

        public boolean videoBitRateSet;
        public double videoBitRate;

        public boolean audioBitRateSet;
        public double audioBitRate;

        public Config_ExtendedSettings() {
            videoCodecSet = false;
            audioCodecSet = false;
            videoBitRateSet = false;
            audioBitRateSet = false;
        }
    };

    private final String KEY_VIDEO_RESOLUTION_WIDTH_MIN    = "KeyVideoResolutionWidthMin";
    private final String KEY_VIDEO_RESOLUTION_HEIGHT_MIN   = "KeyVideoResolutionHeightMin";
    private final String KEY_VIDEO_RESOLUTION_WIDTH_MAX    = "KeyVideoResolutionWidthMax";
    private final String KEY_VIDEO_RESOLUTION_HEIGHT_MAX   = "KeyVideoResolutionHeightMax";
    private final String KEY_VIDEO_ASPECT_RATIO_NUMER_MIN  = "KeyVideoAspectRatioNumerMin";
    private final String KEY_VIDEO_ASPECT_RATIO_DENOM_MIN  = "KeyVideoAspectRatioDenomMin";
    private final String KEY_VIDEO_ASPECT_RATIO_NUMER_MAX  = "KeyVideoAspectRatioNumerMax";
    private final String KEY_VIDEO_ASPECT_RATIO_DENOM_MAX  = "KeyVideoAspectRatioDenomMax";
    private final String KEY_VIDEO_FRAME_RATE_MIN          = "KeyVideoFrameRateMin";
    private final String KEY_VIDEO_FRAME_RATE_MAX          = "KeyVideoFrameRateMax";
    private final String KEY_VIDEO_CODEC                   = "KeyVideoCodec";
    private final String KEY_AUDIO_CODEC                   = "KeyAudioCodec";
    private final String KEY_VIDEO_BITRATE                 = "KeyVideoBitRate";
    private final String KEY_AUDIO_BITRATE                 = "KeyAudioBitRate";

    private VideoClient m_client = null;
    private View m_view = null;
    private PopupWindow m_popWindow = null;
    private boolean m_testMode = false;

    private Config_GeneralSettings configGS = null;
    private Config_ExtendedSettings configES = null;

    private Spinner m_spnVideoResolutionMin = null;
    private Spinner m_spnVideoResolutionMax = null;
    private Spinner m_spnVideoAspectRatioMin = null;
    private Spinner m_spnVideoAspectRatioMax = null;
    private Spinner m_spnVideoFrameRateMin = null;
    private Spinner m_spnVideoFrameRateMax = null;
    private Spinner m_spnVideoCodec = null;
    private Spinner m_spnAudioCodec = null;
    private Spinner m_spnVideoBitrate = null;
    private Spinner m_spnAudioBitrate = null;

    private int m_idxVideoResolutionMin = Video_Resolutions_List.length;
    private int m_idxVideoResolutionMax = Video_Resolutions_List.length;
    private int m_idxVideoAspectRatioMin = Video_Aspect_Ratio_List.length;
    private int m_idxVideoAspectRatioMax = Video_Aspect_Ratio_List.length;
    private int m_idxVideoFrameRateMin = Video_FrameRate_List.length;
    private int m_idxVideoFrameRateMax = Video_FrameRate_List.length;
    private int m_idxVideoCodec = Video_Codecs_List.length;
    private int m_idxAudioCodec = Audio_Codecs_List.length;
    private int m_idxVideoBitrate = Video_Bitrate_List.length ;
    private int m_idxAudioBitrate = Audio_Bitrate_List.length;

    public Settings(VideoClient client, boolean testMode) {
        m_client = client;
        m_testMode = testMode;

        Log.i( LOG_TAG, "Test Mode is " + testMode );

        LayoutInflater inflater = (LayoutInflater) m_client.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        m_view = inflater.inflate(R.layout.settings, (ViewGroup) m_client.findViewById(R.id.SettingsScreen));

        DisplayMetrics metrics = new DisplayMetrics();
        m_client.getWindowManager().getDefaultDisplay().getMetrics(metrics);
        m_popWindow = new PopupWindow(m_view, metrics.widthPixels, ViewGroup.LayoutParams.WRAP_CONTENT, true);
        m_popWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));

        configGS = new Config_GeneralSettings();

        SetSpinner_VideoResolution();
        SetSpinner_VideoFrameRate();
        SetSpinner_VideoAspectRatio();

        if( m_testMode ) {
            configES = new Config_ExtendedSettings();
            LinearLayout extSettings = (LinearLayout) m_view.findViewById(R.id.LL_ExtendedSettings);
            extSettings.setVisibility( LinearLayout.VISIBLE );
            SetSpinner_VideoCodec();
            SetSpinner_AudioCodec();
            SetSpinner_VideoBitRate();
            SetSpinner_AudioBitRate();
        }

        Button cancelButton = (Button) m_view.findViewById(R.id.BT_SettingsCancel);
        cancelButton.setOnClickListener( new View.OnClickListener() {
                @Override
                public void onClick(View arg0) {
                Cancel(false);
                }
                });

        Button saveButton = (Button) m_view.findViewById(R.id.BT_SettingsSave);
        saveButton.setOnClickListener( new View.OnClickListener() {
                @Override
                public void onClick(View arg0) {
                Cancel(true);
                }
                });

        m_view.setOnKeyListener( new View.OnKeyListener() {
                @Override
                public boolean onKey(View arg0, int arg1, KeyEvent arg2) {
                if( arg1 == KeyEvent.KEYCODE_BACK ) {
                Cancel(false);
                }
                return false;
                }
                });

        LoadSettings();
    }


    public void Show() {
        if( m_popWindow != null ) {
            LoadSettings();
            m_popWindow.showAtLocation(m_view, Gravity.BOTTOM, 0, 0);
        }
    }

    public Config_GeneralSettings GetGeneralSettings() {
        return configGS;
    }

    public Config_ExtendedSettings GetExtendedSettings() {
        return configES;
    }

    private void Cancel( boolean save ) {
        if( m_popWindow != null && m_popWindow.isShowing() ) {
            if( save ) {
                SaveSettings();
            }
            m_popWindow.dismiss();
        }
    }

    private void LoadSettings() {
        Properties gs_prop = new Properties();
        Config_GeneralSettings gs_new = new Config_GeneralSettings();
        String str_temp1;
        String str_temp2;
        int ntemp1;
        int ntemp2;
        double dtemp1;
        int loopcnt;

        try {
            FileInputStream fis1 = null;
            try {
              fis1 = new FileInputStream( Environment.getExternalStorageDirectory().getPath() + GENERAL_SETTINGS_FILE_NAME );
              gs_prop.load(fis1);
            } catch (java.io.IOException ioe) {
                Log.e( LOG_TAG, "I/O error" );
                ioe.printStackTrace();
            } finally {
                if (fis1 != null) {
                  try {
                    fis1.close();
                    fis1 = null;
                  } catch (java.io.IOException ioe) {
                      Log.e( LOG_TAG, "I/O error" );
                      ioe.printStackTrace();
                  }
              }
            }

            try {
                if( ((str_temp1 = gs_prop.getProperty( KEY_VIDEO_RESOLUTION_WIDTH_MIN )) != null)
                        && ((str_temp2 = gs_prop.getProperty( KEY_VIDEO_RESOLUTION_HEIGHT_MIN )) != null) ) {

                    ntemp1 = Integer.parseInt( str_temp1 );
                    ntemp2 = Integer.parseInt( str_temp2 );

                    for( loopcnt = 0; loopcnt < Video_Resolutions_List.length; loopcnt++) {
                        if( (Video_Resolutions_List[loopcnt][0] == ntemp1 ) && (Video_Resolutions_List[loopcnt][1] == ntemp2 ) ) {
                            break;
                        }
                    }

                    if(loopcnt < Video_Resolutions_List.length) {
                        m_idxVideoResolutionMin = loopcnt;
                        gs_new.videoResMinSet = true;
                        gs_new.videoResWidthMin = ntemp1;
                        gs_new.videoResHeightMin = ntemp2;
                    } else {
                        Log.e(LOG_TAG, "Invalid minium resolution.");
                    }
                }
            } catch(Exception ex) {
                Log.e(LOG_TAG, "Unable to load minium resolution.");
                ex.printStackTrace();
            }

            try {
                if( ((str_temp1 = gs_prop.getProperty( KEY_VIDEO_RESOLUTION_WIDTH_MAX )) != null)
                        && ((str_temp2 = gs_prop.getProperty( KEY_VIDEO_RESOLUTION_HEIGHT_MAX )) != null) ) {

                    ntemp1 = Integer.parseInt( str_temp1 );
                    ntemp2 = Integer.parseInt( str_temp2 );

                    for( loopcnt = 0; loopcnt < Video_Resolutions_List.length; loopcnt++) {
                        if( (Video_Resolutions_List[loopcnt][0] == ntemp1 ) && (Video_Resolutions_List[loopcnt][1] == ntemp2 ) ) {
                            break;
                        }
                    }

                    if(loopcnt < Video_Resolutions_List.length) {
                        m_idxVideoResolutionMax = loopcnt;
                        gs_new.videoResMaxSet = true;
                        gs_new.videoResWidthMax = ntemp1;
                        gs_new.videoResHeightMax = ntemp2;
                    } else {
                        Log.e(LOG_TAG, "Invalid maximum resolution.");
                    }
                }
            } catch(Exception ex) {
                Log.e(LOG_TAG, "Unable to load maximum resolution.");
                ex.printStackTrace();
            }

            try {
                if( ((str_temp1 = gs_prop.getProperty( KEY_VIDEO_ASPECT_RATIO_NUMER_MIN )) != null)
                        && ((str_temp2 = gs_prop.getProperty( KEY_VIDEO_ASPECT_RATIO_DENOM_MIN )) != null) ) {

                    ntemp1 = Integer.parseInt( str_temp1 );
                    ntemp2 = Integer.parseInt( str_temp2 );

                    for( loopcnt = 0; loopcnt < Video_Aspect_Ratio_List.length; loopcnt++) {
                        if( (Video_Aspect_Ratio_List[loopcnt][0] == ntemp1 ) && (Video_Aspect_Ratio_List[loopcnt][1] == ntemp2 ) ) {
                            break;
                        }
                    }

                    if(loopcnt < Video_Aspect_Ratio_List.length) {
                        m_idxVideoAspectRatioMin = loopcnt;
                        gs_new.videoAspectRatioMinSet = true;
                        gs_new.videoAspectRatioNumerMin = ntemp1;
                        gs_new.videoAspectRatioDenomMin = ntemp2;
                    } else {
                        Log.e(LOG_TAG, "Invalid minium aspect ratio.");
                    }
                }
            } catch(Exception ex) {
                Log.e(LOG_TAG, "Unable to load minium aspect ratio.");
                ex.printStackTrace();
            }

            try {
                if( ((str_temp1 = gs_prop.getProperty( KEY_VIDEO_ASPECT_RATIO_NUMER_MAX )) != null)
                        && ((str_temp2 = gs_prop.getProperty( KEY_VIDEO_ASPECT_RATIO_DENOM_MAX )) != null) ) {

                    ntemp1 = Integer.parseInt( str_temp1 );
                    ntemp2 = Integer.parseInt( str_temp2 );

                    for( loopcnt = 0; loopcnt < Video_Aspect_Ratio_List.length; loopcnt++) {
                        if( (Video_Aspect_Ratio_List[loopcnt][0] == ntemp1 ) && (Video_Aspect_Ratio_List[loopcnt][1] == ntemp2 ) ) {
                            break;
                        }
                    }

                    if(loopcnt < Video_Aspect_Ratio_List.length) {
                        m_idxVideoAspectRatioMax = loopcnt;
                        gs_new.videoAspectRatioMaxSet = true;
                        gs_new.videoAspectRatioNumerMax = ntemp1;
                        gs_new.videoAspectRatioDenomMax = ntemp2;
                    } else {
                        Log.e(LOG_TAG, "Invalid maximum aspect ratio.");
                    }
                }
            } catch(Exception ex) {
                Log.e(LOG_TAG, "Unable to load maximum aspect ratio.");
                ex.printStackTrace();
            }

            try {
                if( (str_temp1 = gs_prop.getProperty( KEY_VIDEO_FRAME_RATE_MIN )) != null ) {
                    dtemp1 = Double.parseDouble( str_temp1 );

                    for( loopcnt = 0; loopcnt < Video_FrameRate_List.length; loopcnt++) {
                        if( Video_FrameRate_List[loopcnt] == dtemp1 ) {
                            break;
                        }
                    }

                    if(loopcnt < Video_FrameRate_List.length) {
                        m_idxVideoFrameRateMin = loopcnt;
                        gs_new.videoFrameRateMinSet = true;
                        gs_new.videoFrameRateMin = dtemp1;
                    } else {
                        Log.e(LOG_TAG, "Invalid mininum frame rate.");
                    }
                }
            } catch(Exception ex) {
                Log.e(LOG_TAG, "Unable to load minium frame rate.");
                ex.printStackTrace();
            }

            try {
                if( (str_temp1 = gs_prop.getProperty( KEY_VIDEO_FRAME_RATE_MAX )) != null ) {
                    dtemp1 = Double.parseDouble( str_temp1 );

                    for( loopcnt = 0; loopcnt < Video_FrameRate_List.length; loopcnt++) {
                        if( Video_FrameRate_List[loopcnt] == dtemp1 ) {
                            break;
                        }
                    }

                    if(loopcnt < Video_FrameRate_List.length) {
                        m_idxVideoFrameRateMax = loopcnt;
                        gs_new.videoFrameRateMaxSet = true;
                        gs_new.videoFrameRateMax = dtemp1;
                    } else {
                        Log.e(LOG_TAG, "Invalid maximum frame rate.");
                    }
                }
            } catch(Exception ex) {
                Log.e(LOG_TAG, "Unable to load maximum frame rate.");
                ex.printStackTrace();
            }

            configGS = gs_new;

            Log.i(LOG_TAG, "General settings configuration loaded.");

        } catch (Exception ex) {
            Log.e(LOG_TAG, "Unable to load general settings file.");
            ex.printStackTrace();
        }


        if( m_testMode ) {
            Properties es_prop = new Properties();
            Config_ExtendedSettings es_new = new Config_ExtendedSettings();

            try {
                FileInputStream fis = null;
                try {
                  fis = new FileInputStream( Environment.getExternalStorageDirectory().getPath() + EXTENDED_SETTINGS_FILE_NAME );
                  es_prop.load(fis);
                  fis.close();
                } catch (java.io.IOException ioe) {
                  Log.e( LOG_TAG, "I/O error" );
                  ioe.printStackTrace();
                } finally {
                  if (fis != null) {
                    try {
                      fis.close();
                      fis = null;
                  } catch (java.io.IOException ioe) {
                      Log.e( LOG_TAG, "I/O error" );
                      ioe.printStackTrace();
                  }
                }
             }

                try {
                    if( (str_temp1 = es_prop.getProperty( KEY_VIDEO_CODEC )) != null ) {
                        for( loopcnt = 0; loopcnt < Video_Codecs_List.length; loopcnt++) {
                            if( str_temp1.equals( Video_Codecs_List[loopcnt] ) ) {
                                break;
                            }
                        }

                        if(loopcnt < Video_Codecs_List.length) {
                            m_idxVideoCodec = loopcnt;
                            es_new.videoCodecSet = true;
                            es_new.videoCodec = str_temp1;
                        } else {
                            Log.e(LOG_TAG, "Invalid video codec type.");
                        }
                    }
                } catch(Exception ex) {
                    Log.e(LOG_TAG, "Unable to load video codec type.");
                    ex.printStackTrace();
                }

                try {
                    if( (str_temp1 = es_prop.getProperty( KEY_AUDIO_CODEC )) != null ) {
                        for( loopcnt = 0; loopcnt < Audio_Codecs_List.length; loopcnt++) {
                            if( str_temp1.equals( Audio_Codecs_List[loopcnt] ) ) {
                                break;
                            }
                        }

                        if(loopcnt < Audio_Codecs_List.length) {
                            m_idxAudioCodec = loopcnt;
                            es_new.audioCodecSet = true;
                            es_new.audioCodec = str_temp1;
                        } else {
                            Log.e(LOG_TAG, "Invalid audio codec type.");
                        }
                    }
                } catch(Exception ex) {
                    Log.e(LOG_TAG, "Unable to load audio codec type.");
                    ex.printStackTrace();
                }

                try {
                    if( (str_temp1 = es_prop.getProperty( KEY_VIDEO_BITRATE )) != null ) {
                        dtemp1 = Double.parseDouble( str_temp1 );

                        for( loopcnt = 0; loopcnt < Video_Bitrate_List.length; loopcnt++) {
                            if( dtemp1 == Video_Bitrate_List[loopcnt] ) {
                                break;
                            }
                        }

                        if(loopcnt < Audio_Codecs_List.length) {
                            m_idxVideoBitrate = loopcnt;
                            es_new.videoBitRateSet = true;
                            es_new.videoBitRate = dtemp1;
                        } else {
                            Log.e(LOG_TAG, "Invalid video bitrate.");
                        }
                    }
                } catch(Exception ex) {
                    Log.e(LOG_TAG, "Unable to load video bitrate.");
                    ex.printStackTrace();
                }

                try {
                    if( (str_temp1 = es_prop.getProperty( KEY_AUDIO_BITRATE )) != null ) {
                        dtemp1 = Double.parseDouble( str_temp1 );

                        for( loopcnt = 0; loopcnt < Audio_Bitrate_List.length; loopcnt++) {
                            if( dtemp1 == Audio_Bitrate_List[loopcnt] ) {
                                break;
                            }
                        }

                        if(loopcnt < Audio_Bitrate_List.length) {
                            m_idxAudioBitrate = loopcnt;
                            es_new.audioBitRateSet = true;
                            es_new.audioBitRate = dtemp1;
                        } else {
                            Log.e(LOG_TAG, "Invalid audio bitrate.");
                        }
                    }
                } catch(Exception ex) {
                    Log.e(LOG_TAG, "Unable to load audio bitrate.");
                    ex.printStackTrace();
                }

                configES = es_new;
                Log.i(LOG_TAG, "Extended settings configuration loaded.");

            } catch (Exception ex) {
                Log.e(LOG_TAG, "Unable to load extended settings file.");
                ex.printStackTrace();
            }
        }

        SetSelections();
    }

    private void SaveSettings() {
        Properties gs_prop = new Properties();
        Config_GeneralSettings gs_new = new Config_GeneralSettings();

        if( m_idxVideoResolutionMin !=  Video_Resolutions_List.length ) {
            gs_new.videoResWidthMin = Video_Resolutions_List[m_idxVideoResolutionMin][0];
            gs_new.videoResHeightMin = Video_Resolutions_List[m_idxVideoResolutionMin][1];
            gs_prop.setProperty( KEY_VIDEO_RESOLUTION_WIDTH_MIN, Integer.toString( gs_new.videoResWidthMin ) );
            gs_prop.setProperty( KEY_VIDEO_RESOLUTION_HEIGHT_MIN, Integer.toString( gs_new.videoResHeightMin ) );
        }

        if( m_idxVideoResolutionMax !=  Video_Resolutions_List.length ) {
            gs_new.videoResWidthMax = Video_Resolutions_List[m_idxVideoResolutionMax][0];
            gs_new.videoResHeightMax = Video_Resolutions_List[m_idxVideoResolutionMax][1];
            gs_prop.setProperty( KEY_VIDEO_RESOLUTION_WIDTH_MAX, Integer.toString( gs_new.videoResWidthMax ) );
            gs_prop.setProperty( KEY_VIDEO_RESOLUTION_HEIGHT_MAX, Integer.toString( gs_new.videoResHeightMax ) );
        }

        if( m_idxVideoAspectRatioMin !=  Video_Aspect_Ratio_List.length ) {
            gs_new.videoAspectRatioNumerMin = Video_Aspect_Ratio_List[m_idxVideoAspectRatioMin][0];
            gs_new.videoAspectRatioDenomMin = Video_Aspect_Ratio_List[m_idxVideoAspectRatioMin][1];
            gs_prop.setProperty( KEY_VIDEO_ASPECT_RATIO_NUMER_MIN, Integer.toString( gs_new.videoAspectRatioNumerMin ) );
            gs_prop.setProperty( KEY_VIDEO_ASPECT_RATIO_DENOM_MIN, Integer.toString( gs_new.videoAspectRatioDenomMin ) );
        }

        if( m_idxVideoAspectRatioMax !=  Video_Aspect_Ratio_List.length ) {
            gs_new.videoAspectRatioNumerMax = Video_Aspect_Ratio_List[m_idxVideoAspectRatioMax][0];
            gs_new.videoAspectRatioDenomMax = Video_Aspect_Ratio_List[m_idxVideoAspectRatioMax][1];
            gs_prop.setProperty( KEY_VIDEO_ASPECT_RATIO_NUMER_MAX, Integer.toString( gs_new.videoAspectRatioNumerMax ) );
            gs_prop.setProperty( KEY_VIDEO_ASPECT_RATIO_DENOM_MAX, Integer.toString( gs_new.videoAspectRatioDenomMax ) );
        }

        if( m_idxVideoFrameRateMin !=  Video_FrameRate_List.length ) {
            gs_new.videoFrameRateMin = Video_FrameRate_List[m_idxVideoFrameRateMin];
            gs_prop.setProperty( KEY_VIDEO_FRAME_RATE_MIN, Double.toString( gs_new.videoFrameRateMin ) );
        }

        if( m_idxVideoFrameRateMax !=  Video_FrameRate_List.length ) {
            gs_new.videoFrameRateMax = Video_FrameRate_List[m_idxVideoFrameRateMax];
            gs_prop.setProperty( KEY_VIDEO_FRAME_RATE_MAX, Double.toString( gs_new.videoFrameRateMax ) );
        }

        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream( Environment.getExternalStorageDirectory().getPath() + GENERAL_SETTINGS_FILE_NAME );
            gs_prop.store(fos, null);
            configGS = gs_new;
        } catch (java.io.IOException ioe) {
            Log.e( LOG_TAG, "I/O error: unable to save general settings into file" );
            ioe.printStackTrace();
        } catch (Exception ex) {
            Log.e( LOG_TAG, "Unable to store general settings into file" );
            ex.printStackTrace();
        } finally {
            if (fos != null) {
              try {
              fos.close();
              fos = null;
              } catch (java.io.IOException ioe) {
                  Log.e( LOG_TAG, "I/O error: unable to save general settings into file" );
                  ioe.printStackTrace();
                }
            }
        }

        if( m_testMode ) {
            Properties es_prop = new Properties();
            Config_ExtendedSettings es_new = new Config_ExtendedSettings();

            if( m_idxVideoCodec !=  Video_Codecs_List.length ) {
                es_new.videoCodec = new String( Video_Codecs_List[m_idxVideoCodec] );
                es_prop.setProperty( KEY_VIDEO_CODEC, es_new.videoCodec );
            }

            if( m_idxAudioCodec !=  Audio_Codecs_List.length ) {
                es_new.audioCodec = new String( Audio_Codecs_List[m_idxAudioCodec] );
                es_prop.setProperty( KEY_AUDIO_CODEC, es_new.audioCodec );
            }

            if( m_idxVideoBitrate !=  Video_Bitrate_List.length ) {
                es_new.videoBitRate = Video_Bitrate_List[m_idxVideoBitrate];
                es_prop.setProperty( KEY_VIDEO_BITRATE, Double.toString( es_new.videoBitRate ) );
            }

            if( m_idxAudioBitrate !=  Audio_Bitrate_List.length ) {
                es_new.audioBitRate = Audio_Bitrate_List[m_idxAudioBitrate];
                es_prop.setProperty( KEY_AUDIO_BITRATE, Double.toString( es_new.audioBitRate ) );
            }

            FileOutputStream fos2 = null;
            try {
                fos2 = new FileOutputStream( Environment.getExternalStorageDirectory().getPath() + EXTENDED_SETTINGS_FILE_NAME );
                es_prop.store(fos2, null);
                configES = es_new;
            } catch (java.io.IOException ioe) {
                Log.e( LOG_TAG, "I/O error: unable to save general settings into file" );
                ioe.printStackTrace();
            } catch (Exception ex) {
                Log.e( LOG_TAG, "Unable to store extended settings into file" );
                ex.printStackTrace();
            } finally {
                if (fos2 != null) {
                  try {
                  fos2.close();
                  fos2 = null;
                  } catch (java.io.IOException ioe) {
                      Log.e( LOG_TAG, "I/O error: unable to save general settings into file" );
                      ioe.printStackTrace();
                    }
                }
            }
        }

        LoadSettings();
    }

    private void SetSelections() {
        m_spnVideoResolutionMin.setSelection( m_idxVideoResolutionMin );
        m_spnVideoResolutionMax.setSelection( m_idxVideoResolutionMax );
        m_spnVideoAspectRatioMin.setSelection( m_idxVideoAspectRatioMin );
        m_spnVideoAspectRatioMax.setSelection( m_idxVideoAspectRatioMax );
        m_spnVideoFrameRateMin.setSelection( m_idxVideoFrameRateMin );
        m_spnVideoFrameRateMax.setSelection( m_idxVideoFrameRateMax );
        m_spnVideoCodec.setSelection( m_idxVideoCodec );
        m_spnAudioCodec.setSelection( m_idxAudioCodec );
        m_spnVideoBitrate.setSelection( m_idxVideoBitrate );
        m_spnAudioBitrate.setSelection( m_idxAudioBitrate );
    }

    private void SetSpinner_VideoResolution() {
        String[] str_videoRes = new String[Video_Resolutions_List.length + 1];

        str_videoRes[Video_Resolutions_List.length] = new String(NONE_SELECTED);
        for(int i=0; i<Video_Resolutions_List.length; i++) {
            str_videoRes[i] = new String( Integer.toString(Video_Resolutions_List[i][0])
                    + "x" + Integer.toString(Video_Resolutions_List[i][1]) );
        }

        ArrayAdapter<String> min_adapter = new ArrayAdapter<String>(m_client, android.R.layout.simple_spinner_item,	str_videoRes);
        min_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        m_spnVideoResolutionMin = (Spinner) m_view.findViewById(R.id.SP_VideoResolutionMin);
        m_spnVideoResolutionMin.setAdapter(min_adapter);
        m_spnVideoResolutionMin.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent,
                    View view, int position, long id) {
                m_idxVideoResolutionMin = position;
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                Log.e(LOG_TAG, "None selected: Not possible!");
                }
                });

        ArrayAdapter<String> max_adapter = new ArrayAdapter<String>(m_client, android.R.layout.simple_spinner_item,	str_videoRes);
        max_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        m_spnVideoResolutionMax = (Spinner) m_view.findViewById(R.id.SP_VideoResolutionMax);
        m_spnVideoResolutionMax.setAdapter(max_adapter);
        m_spnVideoResolutionMax.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent,
                    View view, int position, long id) {
                m_idxVideoResolutionMax = position;
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                Log.e(LOG_TAG, "None selected: Not possible!");
                }
                });
    }

    private void SetSpinner_VideoFrameRate() {
        String[] str_videofps = new String[Video_FrameRate_List.length + 1];

        str_videofps[Video_FrameRate_List.length] = new String(NONE_SELECTED);
        for(int i=0; i<Video_FrameRate_List.length; i++) {
            str_videofps[i] = new String( Double.toString(Video_FrameRate_List[i]) + " fps" );
        }

        ArrayAdapter<String> min_adapter = new ArrayAdapter<String>(m_client, android.R.layout.simple_spinner_item,	str_videofps);
        min_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        m_spnVideoFrameRateMin = (Spinner) m_view.findViewById(R.id.SP_VideoFrameRateMin);
        m_spnVideoFrameRateMin.setAdapter(min_adapter);
        m_spnVideoFrameRateMin.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent,
                    View view, int position, long id) {
                m_idxVideoFrameRateMin = position;
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                Log.e(LOG_TAG, "None selected: Not possible!");
                }
                });

        ArrayAdapter<String> max_adapter = new ArrayAdapter<String>(m_client, android.R.layout.simple_spinner_item,	str_videofps);
        max_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        m_spnVideoFrameRateMax = (Spinner) m_view.findViewById(R.id.SP_VideoFrameRateMax);
        m_spnVideoFrameRateMax.setAdapter(max_adapter);
        m_spnVideoFrameRateMax.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent,
                    View view, int position, long id) {
                m_idxVideoFrameRateMax = position;
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                Log.e(LOG_TAG, "None selected: Not possible!");
                }
                });
    }

    private void SetSpinner_VideoAspectRatio() {
        String[] str_videoAr = new String[Video_Aspect_Ratio_List.length + 1];

        str_videoAr[Video_Aspect_Ratio_List.length] = new String(NONE_SELECTED);
        for(int i=0; i<Video_Aspect_Ratio_List.length; i++) {
            double ar = Math.floor( ( (double)Video_Aspect_Ratio_List[i][0] / Video_Aspect_Ratio_List[i][1] ) * 100 ) / 100;

            str_videoAr[i] = new String( Integer.toString(Video_Aspect_Ratio_List[i][0])
                    + ":" + Integer.toString(Video_Aspect_Ratio_List[i][1]) + " (" + Double.toString(ar) + ":1" + ")"  );
        }

        ArrayAdapter<String> min_adapter = new ArrayAdapter<String>(m_client, android.R.layout.simple_spinner_item,	str_videoAr);
        min_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        m_spnVideoAspectRatioMin = (Spinner) m_view.findViewById(R.id.SP_VideoAspectRatioMin);
        m_spnVideoAspectRatioMin.setAdapter(min_adapter);
        m_spnVideoAspectRatioMin.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent,
                    View view, int position, long id) {
                m_idxVideoAspectRatioMin = position;
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                Log.e(LOG_TAG, "None selected: Not possible!");
                }
                });

        ArrayAdapter<String> max_adapter = new ArrayAdapter<String>(m_client, android.R.layout.simple_spinner_item,	str_videoAr);
        max_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        m_spnVideoAspectRatioMax = (Spinner) m_view.findViewById(R.id.SP_VideoAspectRatioMax);
        m_spnVideoAspectRatioMax.setAdapter(max_adapter);
        m_spnVideoAspectRatioMax.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent,
                    View view, int position, long id) {
                m_idxVideoAspectRatioMax = position;
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                Log.e(LOG_TAG, "None selected: Not possible!");
                }
                });
    }

    private void SetSpinner_VideoCodec() {
        String[] str_videoCodec = new String[Video_Codecs_List.length + 1];

        str_videoCodec[Video_Codecs_List.length] = new String(NONE_SELECTED);
        for(int i=0; i<Video_Codecs_List.length; i++) {
            str_videoCodec[i] = new String( Video_Codecs_List[i] );
        }

        ArrayAdapter<String> min_adapter = new ArrayAdapter<String>(m_client, android.R.layout.simple_spinner_item,	str_videoCodec);
        min_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        m_spnVideoCodec = (Spinner) m_view.findViewById(R.id.SP_VideoCodec);
        m_spnVideoCodec.setAdapter(min_adapter);
        m_spnVideoCodec.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent,
                    View view, int position, long id) {
                m_idxVideoCodec = position;
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                Log.e(LOG_TAG, "None selected: Not possible!");
                }
                });
    }

    private void SetSpinner_AudioCodec() {
        String[] str_audioCodec = new String[Audio_Codecs_List.length + 1];

        str_audioCodec[Audio_Codecs_List.length] = new String(NONE_SELECTED);
        for(int i=0; i<Audio_Codecs_List.length; i++) {
            str_audioCodec[i] = new String( Audio_Codecs_List[i] );
        }

        ArrayAdapter<String> min_adapter = new ArrayAdapter<String>(m_client, android.R.layout.simple_spinner_item,	str_audioCodec);
        min_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        m_spnAudioCodec = (Spinner) m_view.findViewById(R.id.SP_AudioCodec);
        m_spnAudioCodec.setAdapter(min_adapter);
        m_spnAudioCodec.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent,
                    View view, int position, long id) {
                m_idxAudioCodec = position;
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                Log.e(LOG_TAG, "None selected: Not possible!");
                }
                });
    }

    private void SetSpinner_VideoBitRate() {
        String[] str_videoBr = new String[Video_Bitrate_List.length + 1];

        str_videoBr[Video_Bitrate_List.length] = new String(NONE_SELECTED);
        for(int i=0; i<Video_Bitrate_List.length; i++) {
            str_videoBr[i] = new String( Double.toString(Video_Bitrate_List[i]) + " Mbps" );
        }

        ArrayAdapter<String> min_adapter = new ArrayAdapter<String>(m_client, android.R.layout.simple_spinner_item,	str_videoBr);
        min_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        m_spnVideoBitrate = (Spinner) m_view.findViewById(R.id.SP_VideoBitRate);
        m_spnVideoBitrate.setAdapter(min_adapter);
        m_spnVideoBitrate.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent,
                    View view, int position, long id) {
                m_idxVideoBitrate = position;
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                Log.e(LOG_TAG, "None selected: Not possible!");
                }
                });
    }

    private void SetSpinner_AudioBitRate() {
        String[] str_audioBr = new String[Audio_Bitrate_List.length + 1];

        str_audioBr[Audio_Bitrate_List.length] = new String(NONE_SELECTED);
        for(int i=0; i<Audio_Bitrate_List.length; i++) {
            str_audioBr[i] = new String( Double.toString(Audio_Bitrate_List[i]) + " kbps" );
        }

        ArrayAdapter<String> min_adapter = new ArrayAdapter<String>(m_client, android.R.layout.simple_spinner_item,	str_audioBr);
        min_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        m_spnAudioBitrate = (Spinner) m_view.findViewById(R.id.SP_AudioBitRate);
        m_spnAudioBitrate.setAdapter(min_adapter);
        m_spnAudioBitrate.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent,
                    View view, int position, long id) {
                m_idxAudioBitrate = position;
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                Log.e(LOG_TAG, "None selected: Not possible!");
                }
                });
    }
}
