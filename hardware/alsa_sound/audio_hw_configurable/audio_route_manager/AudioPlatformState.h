/*
 ** Copyright 2013 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **      http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */
#pragma once
#include "AudioBand.h"

#define ADD_EVENT(eventName) E##eventName = 1 << eventName

namespace android_audio_legacy
{

#define TTY_DOWNLINK    0x1
#define TTY_UPLINK      0x2

class CAudioRouteManager;

class CAudioPlatformState
{
    enum Direction {

        EInput = 0,
        EOutput,

        ENbDirections
    };

    enum TtyDirection {

        ETtyDownlink    = 0x1,
        ETtyUplink      = 0x2
    };

public:
    enum EventName_t {
        AndroidModeChange,
        HwModeChange,
        ModemStateChange,
        ModemAudioStatusChange,
        HacModeChange,
        TtyDirectionChange,
        BtEnableChange,
        ScoRscReqStateChange,
        BtHeadsetNrEcChange,
        BtHeadsetBandTypeChange,
        BandTypeChange,
        InputDevicesChange,
        OutputDevicesChange,
        InputSourceChange,
        SharedI2SStateChange,
        StreamEvent,
        ScreenStateChange,
        ContextAwarenessStateChange,
        AlwaysListeningStateChange,
        FmStateChange,
        BypassNonLinearPpStateChange,
        BypassLinearPpStateChange,
        MicMuteChange,

        NbEvents
    };


    enum EventType_t {
        EAllEvents                  = -1,
        ADD_EVENT(AndroidModeChange),
        ADD_EVENT(HwModeChange),
        ADD_EVENT(ModemStateChange),
        ADD_EVENT(ModemAudioStatusChange),
        ADD_EVENT(HacModeChange),
        ADD_EVENT(TtyDirectionChange),
        ADD_EVENT(BtEnableChange),
        ADD_EVENT(ScoRscReqStateChange),
        ADD_EVENT(BtHeadsetNrEcChange),
        ADD_EVENT(BtHeadsetBandTypeChange),
        ADD_EVENT(BandTypeChange),
        ADD_EVENT(InputDevicesChange),
        ADD_EVENT(OutputDevicesChange),
        ADD_EVENT(InputSourceChange),
        ADD_EVENT(SharedI2SStateChange),
        ADD_EVENT(StreamEvent),
        ADD_EVENT(ScreenStateChange),
        ADD_EVENT(ContextAwarenessStateChange),
        ADD_EVENT(AlwaysListeningStateChange),
        ADD_EVENT(FmStateChange),
        ADD_EVENT(BypassNonLinearPpStateChange),
        ADD_EVENT(BypassLinearPpStateChange),
        ADD_EVENT(MicMuteChange)
    };

    CAudioPlatformState(CAudioRouteManager* pAudioRouteManager);
    virtual           ~CAudioPlatformState();

    // Get the modem status
    bool isModemAlive() const { return _bModemAlive; }

    // Set the modem status
    void setModemAlive(bool bIsAlive);

    // Get the modem audio call status
    bool isModemAudioAvailable() const { return _bModemAudioAvailable; }

    // Set the modem Audio available
    void setModemAudioAvailable(bool bIsAudioAvailable);

    // Get the modem embedded status
    bool isModemEmbedded() const { return _bModemEmbedded; }

    // Set the modem embedded status
    void setModemEmbedded(bool bIsPresent);

    // Check if the Modem Shared I2S is safe to use
    bool isSharedI2SBusAvailable() const;

    // Set telephony mode
    void setMode(int iMode);

    // Get telephony mode
    int getMode() const { return _iAndroidMode; }

    // Get the HW mode
    int getHwMode() const { return _iHwMode; }

    // Set TTY mode
    void setTtyDirection(int iTtyDirection);

    // Get TTY mode
    int getTtyDirection() const { return _iTtyDirection; }

    // Set HAC mode
    void setHacMode(bool bEnabled);

    // Get HAC Mode
    bool isHacEnabled() const { return _bIsHacModeEnabled; }

    /**
     * Set the BT headset NREC.
     *
     * @param[in] bIsAcousticSupportedOnBT: true means the BT device embeds its acoustic algorithms
     */
    void setBtHeadsetNrEc(bool bIsAcousticSupportedOnBT);

    // Get BT NREC
    bool isBtHeadsetNrEcEnabled() const { return _bBtHeadsetNrEcEnabled; }

    /**
     * Set the BT headset negociated Band Type.
     * Band Type results of the negociation between device and the BT HFP headset.
     *
     * @param[in] eBtHeadsetBandType: the band type to be set
     */
    void setBtHeadsetBandType(CAudioBand::Type eBtHeadsetBandType);

    /**
     * Get the BT headset negociated Band Type.
     * Band Type results of the negociation between device and the BT HFP headset device.
     *
     * @return Negociated band type of the BT headset (default: ENarrow)
     */
    CAudioBand::Type getBtHeadsetBandType() const { return _eBtHeadsetBandType; }

    // Set BT Enabled flag
    void setBtEnabled(bool bIsBtEnabled);

    // Get BT Enabled flag
    bool isBtEnabled() const { return _bIsBtEnabled; }

    /**
     * Set SCO resource requested flag
     *
     * @param[in] scoEnabled: if true, the SCO resource is requested.
     */
    void setScoResourceRequested(bool scoEnabled);

    /**
     * Get SCO resource requested flag
     *
     * @return true if SCO resource is requested.
     */
    bool isScoResourceRequested() const { return mScoResourceRequested; }

    bool hasDirectStreams() const { return (_uiDirectStreamsRefCount != 0); }

    // Get devices
    uint32_t getDevices(bool bIsOut) const { return _uiDevices[bIsOut]; }

    // Set devices
    void setDevices(uint32_t devices, bool bIsOut);

    // Get input source
    uint32_t getInputSource() const { return _uiInputSource; }

    /**
     * Set input source mask
     *
     * @param[in] inputSource the stream input source mask.
     */
    void setInputSourceMask(uint32_t inputSource);

    /**
     * Set Context Awareness status
     *
     * @param[in] bEnabled if true, enables the context awareness feature
     */
    void setContextAwarenessStatus(bool bEnabled);

    /**
     * Get Context Awareness status
     *
     * @return true if the context awareness feature is enabled
     */
    bool isContextAwarenessEnabled() const { return _bIsContextAwarenessEnabled; }

    /**
     * Set "Always Listening" status
     *
     * @param[in] bEnabled if true, enables the "always listening" feature
     */
    void setAlwaysListeningStatus(bool enabled);

    /**
     * Get "Always Listening" status
     *
     * @return true if the "always listening" feature is enabled
     */
    bool isAlwaysListeningEnabled() const { return _isAlwaysListeningEnabled; }

    /**
     * Set "Bypass Non linear mode" status
     *
     * @param[in] bypassed if true, disables the non linear post processing
     */
    void bypassNonLinearPp(bool bypassed);

    /**
     * Set "Bypass Linear mode" status
     *
     * @param[in] bypassed if true, disables the linear post processing
     */
    void bypassLinearPp(bool bypassed);

    /**
     * Get "Bypass Non linear status" status
     *
     * @return true if the non linear post processing is disabled
     */
    bool bypassedNonLinearPp() const { return _bypassedNonLinearPp; }

    /**
     * Get "Bypass Linear status" status
     *
     * @return true if the linear post processing is disabled
     */
    bool bypassedLinearPp() const { return _bypassedLinearPp; }

    /**
     * Get FM State.
     *
     * @return true if FM module is powered on by FM stack.
     */
    inline bool isFmStateOn() const { return _bFmIsOn; }

    void setFmState(bool bIsFmOn);

    /**
      * Set the Band Type.
      * Band Type may come from either the modem (CSV) or from the stream (VoIP).
      *
      * @param[in] eBandType: the band to set
      * @param[in] iForMode: mode to identify the source of the band change
      *
      */
    void setBandType(CAudioBand::Type eBandType, int iForMode);

    /**
      * Get the Band Type.
      * Band Type is unique and depending on the use case (telephony mode)
      * it may return from either the modem (CSV) or from the stream (VoIP) band
      *
      * @return Band of the active voice use case, default is CSV band
      *
      */
    CAudioBand::Type getBandType() const;

    void setScreenState(bool _bScreenOn);

    bool isScreenOn() { return _bScreenOn; }

    void setPlatformStateEvent(int iEvent);

    void clearPlatformStateEvents();

    bool hasPlatformStateChanged(int iEvents = EAllEvents) const;

    // update the HW mode
    void updateHwMode();

    void setDirectStreamEvent(uint32_t uiFlags);

    /**
     * Set "Mic Mute" feature.
     *
     * @param[in] state new value of micmute feature.
     */
    void setMicMute(bool state);

    /**
     * Get "Mic Mute" feature state.
     *
     * @return true if the state of mic mute feature is set to true, false otherwise.
     */
    bool getMicMute() const { return _micMute; }

private:
    // Check if the Hw mode has changed
    bool checkHwMode();

    // Modem Call state
    bool _bModemAudioAvailable;

    // Modem State
    bool _bModemAlive;

    // Indicate if platform embeds a modem
    bool _bModemEmbedded;

    // Android Telephony mode cache
    int _iAndroidMode;

    // TTY Mode
    int _iTtyDirection;

    // HAC mode set
    bool _bIsHacModeEnabled;

    // BTNREC set
    bool _bBtHeadsetNrEcEnabled;

    // BT negociated Band Type
    CAudioBand::Type _eBtHeadsetBandType;

    // BT Enabled flag
    bool _bIsBtEnabled;

    /** SCO resource requested flag */
    bool mScoResourceRequested;

    /**
      * This attribute indicates the state of the "Mic Mute" feature.
      */
    bool _micMute;

    // Input/output Devices bit field
    uint32_t _uiDevices[ENbDirections];

    uint32_t _uiInputSource;

    bool _bFmIsOn;

    uint32_t _uiDirectStreamsRefCount;

    uint32_t _uiRoutes[];

    // Hw Mode: translate the use case, indeed it implies the audio route to follow
    int32_t _iHwMode;

    CAudioBand::Type _eCsvBandType;
    CAudioBand::Type _eVoipBandType;

    // Glitch safe flag for the shared I2S bus
    bool _bIsSharedI2SGlitchSafe;

    // Screen State
    bool _bScreenOn;

    //Context Awareness status
    bool _bIsContextAwarenessEnabled;
    // Always Listening status
    bool _isAlwaysListeningEnabled;

    /**
     * Bypass Non Linear PP status
     */
    bool _bypassedNonLinearPp;

    /**
     * Bypass Linear PP status
     */
    bool _bypassedLinearPp;

    uint32_t _uiPlatformEventChanged;

    uint32_t _uiPlatformComponentsState;

    CAudioRouteManager* _pAudioRouteManager;
};
};        // namespace android

