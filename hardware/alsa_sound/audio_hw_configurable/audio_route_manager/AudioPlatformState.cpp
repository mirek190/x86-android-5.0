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

#define LOG_TAG "AudioPlatformState"

#include <hardware_legacy/AudioHardwareBase.h>
#include <utils/Log.h>
#include <cutils/bitops.h>
#include "VolumeKeys.h"
#include "AudioRouteManager.h"
#include "AudioPlatformState.h"

#define DIRECT_STREAM_FLAGS (AUDIO_OUTPUT_FLAG_DIRECT | AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD)

namespace android_audio_legacy
{

CAudioPlatformState::CAudioPlatformState(CAudioRouteManager* pAudioRouteManager) :
    _bModemAudioAvailable(false),
    _bModemAlive(false),
    _iAndroidMode(AudioSystem::MODE_NORMAL),
    _iTtyDirection(0),
    _bIsHacModeEnabled(false),
    _bBtHeadsetNrEcEnabled(false),
    _eBtHeadsetBandType(CAudioBand::ENarrow),
    _bIsBtEnabled(false),
    mScoResourceRequested(false),
    _micMute(false),
    _uiInputSource(0),
    _bFmIsOn(false),
    _uiDirectStreamsRefCount(0),
    _iHwMode(AudioSystem::MODE_NORMAL),
    _eCsvBandType(CAudioBand::ENarrow),
    _eVoipBandType(CAudioBand::ENarrow),
    _bIsSharedI2SGlitchSafe(false),
    _bScreenOn(true),
    _bIsContextAwarenessEnabled(false),
    _isAlwaysListeningEnabled(false),
    _bypassedNonLinearPp(false),
    _bypassedLinearPp(false),
    _uiPlatformEventChanged(false),
    _pAudioRouteManager(pAudioRouteManager)
{
    _uiDevices[EInput] = 0;
    _uiDevices[EOutput] = 0;
}

CAudioPlatformState::~CAudioPlatformState()
{

}

bool CAudioPlatformState::hasPlatformStateChanged(int iEvents) const
{
    return (_uiPlatformEventChanged & iEvents) != 0;
}

void CAudioPlatformState::setPlatformStateEvent(int iEvent)
{
    _uiPlatformEventChanged |= iEvent;
}

// Set the modem status
void CAudioPlatformState::setModemAlive(bool bIsAlive)
{
    if (_bModemAlive == bIsAlive) {

        return ;
    }

    _bModemAlive = bIsAlive;

    if (!_bModemAlive) {

        // Modem is down, the audio link as well...
        _bModemAudioAvailable = false;
    }

    // Any state change of the modem will require the changed flag to be set
    setPlatformStateEvent(EModemStateChange);

    updateHwMode();
}

// Set the modem embedded state
void CAudioPlatformState::setModemEmbedded(bool bIsEmbedded)
{
    _bModemEmbedded = bIsEmbedded;
}

// Set the modem Audio available
void CAudioPlatformState::setModemAudioAvailable(bool bIsAudioAvailable)
{
    if (_bModemAudioAvailable == bIsAudioAvailable) {

        return ;
    }
    _bModemAudioAvailable = bIsAudioAvailable;
    setPlatformStateEvent(EModemAudioStatusChange);

    // Do not force the platform state change flag.
    // let the update Hw mode raise the flag or not
    updateHwMode();
}

//
// PLATFORM DEPENDANT FUNCTION:
// shared bus may have some conditions in which
// it may not be available
//
// This function indicates if the shared I2S bus can
// be used (avoid glitches, electrical conflicts)
// 2 conditions:
//      -Modem must be up
//      -A call is being set (Android Mode already changed) but
//          modem audio status is still inactive
// if there is no modem, I2S bus can always be used
//
bool CAudioPlatformState::isSharedI2SBusAvailable() const
{
    return !isModemEmbedded() || (isModemAlive() && _bIsSharedI2SGlitchSafe);
}

// Set Android Telephony Mode
void CAudioPlatformState::setMode(int iMode)
{
    if (_iAndroidMode == iMode) {

        return ;
    }
    _iAndroidMode = iMode;
    setPlatformStateEvent(EAndroidModeChange);
}

// Set TTY mode
void CAudioPlatformState::setTtyDirection(int iTtyDirection)
{
    if (_iTtyDirection == iTtyDirection) {

        return ;
    }
    setPlatformStateEvent(ETtyDirectionChange);
    _iTtyDirection = iTtyDirection;
}

// Set HAC mode
void CAudioPlatformState::setHacMode(bool bEnabled)
{
    if (bEnabled == _bIsHacModeEnabled) {

        return ;
    }
    setPlatformStateEvent(EHacModeChange);
    _bIsHacModeEnabled = bEnabled;
}

void CAudioPlatformState::setMicMute(bool state)
{
    ALOGD("%s(state %s)",
          __FUNCTION__, (state == true) ? "true" : "false");
    if (state != _micMute) {

        setPlatformStateEvent(EMicMuteChange);
        _micMute = state;
    }
}

void CAudioPlatformState::setBtEnabled(bool bIsBtEnabled)
{
    if (_bIsBtEnabled == bIsBtEnabled) {

        return ;
    }
    setPlatformStateEvent(EBtEnableChange);
    _bIsBtEnabled = bIsBtEnabled;
}

void CAudioPlatformState::setScoResourceRequested(bool scoEnabled)
{
    if (mScoResourceRequested != scoEnabled) {
        mScoResourceRequested = scoEnabled;
        setPlatformStateEvent(EScoRscReqStateChange);
    }
}

// Set BT_NREC
void CAudioPlatformState::setBtHeadsetNrEc(bool bIsAcousticSupportedOnBT)
{
    if (_bBtHeadsetNrEcEnabled == bIsAcousticSupportedOnBT) {

        return ;
    }
    setPlatformStateEvent(EBtHeadsetNrEcChange);
    _bBtHeadsetNrEcEnabled = bIsAcousticSupportedOnBT;
}

void CAudioPlatformState::setBtHeadsetBandType(CAudioBand::Type eBtHeadsetBandType)
{
    if (_eBtHeadsetBandType == eBtHeadsetBandType) {

        return;
    }
    setPlatformStateEvent(EBtHeadsetBandTypeChange);
    _eBtHeadsetBandType = eBtHeadsetBandType;
}

// Set devices
void CAudioPlatformState::setDevices(uint32_t devices, bool bIsOut)
{
    if (_uiDevices[bIsOut] == devices) {

        return ;
    }
    if (bIsOut) {

        setPlatformStateEvent(EOutputDevicesChange);
    } else {

        setPlatformStateEvent(EInputDevicesChange);
    }

    _uiDevices[bIsOut] = devices;
}

// Set devices
void CAudioPlatformState::setInputSourceMask(uint32_t inputSource)
{
    if (_uiInputSource == inputSource) {

        return ;
    }
    _uiInputSource = inputSource;
    setPlatformStateEvent(EInputSourceChange);
}

// Set FM state
void CAudioPlatformState::setFmState(bool bIsFmOn)
{
    if (_bFmIsOn == bIsFmOn) {

        return;
    }
    _bFmIsOn = bIsFmOn;
    setPlatformStateEvent(EFmStateChange);
}

CAudioBand::Type CAudioPlatformState::getBandType() const
{
    return getHwMode() == AudioSystem::MODE_IN_COMMUNICATION ? _eVoipBandType : _eCsvBandType ;
}

void CAudioPlatformState::setBandType(CAudioBand::Type eBandType, int iForMode)
{
    CAudioBand::Type ePreviousBandType = getBandType();
    if (iForMode == AudioSystem::MODE_IN_COMMUNICATION) {

        _eVoipBandType = eBandType;
    } else if (iForMode == AudioSystem::MODE_IN_CALL) {

        _eCsvBandType = eBandType;
    } else {

        return ;
    }

    if (ePreviousBandType == getBandType()) {

        return ;
    }
    setPlatformStateEvent(EBandTypeChange);
}

void CAudioPlatformState::setScreenState(bool bScreenOn)
{
    if (_bScreenOn == bScreenOn) {

        return ;
    }
    _bScreenOn = bScreenOn;
    setPlatformStateEvent(EScreenStateChange);
}

// set context awareness status
void CAudioPlatformState::setContextAwarenessStatus(bool bEnabled)
{
    if (_bIsContextAwarenessEnabled == bEnabled) {
        // the status does not change -> returning
        return;
    }

    _bIsContextAwarenessEnabled = bEnabled;
    setPlatformStateEvent(EContextAwarenessStateChange);
}

// set always listening status
void CAudioPlatformState::setAlwaysListeningStatus(bool enabled)
{
    if (_isAlwaysListeningEnabled == enabled) {
        // the status does not change -> returning
        return;
    }

    _isAlwaysListeningEnabled = enabled;
    setPlatformStateEvent(EAlwaysListeningStateChange);
}

// set bypass non linear post processing status
void CAudioPlatformState::bypassNonLinearPp(bool bypassed)
{
    if (_bypassedNonLinearPp != bypassed) {
        _bypassedNonLinearPp = bypassed;
        setPlatformStateEvent(EBypassNonLinearPpStateChange);
    }
}

// set bypass linear post processing status
void CAudioPlatformState::bypassLinearPp(bool bypassed)
{
    if (_bypassedLinearPp != bypassed) {
        _bypassedLinearPp = bypassed;
        setPlatformStateEvent(EBypassLinearPpStateChange);
    }
}

void CAudioPlatformState::updateHwMode()
{
    if (checkHwMode()) {

        ALOGV("%s: mode has changed to hwMode=%d", __FUNCTION__, getHwMode());

        setPlatformStateEvent(EHwModeChange);
    }
}

//
// This function checks the "Hardware" that implies the audio route
// to be used.
// It matches the android telephony mode except if:
//      - InRingtoneMode:
//      - Multiple outputdevice selected during IN CALL or IN COMM mode
//                => mode NORMAL forced
//
bool CAudioPlatformState::checkHwMode()
{
    int iTmpHwMode = _iAndroidMode;
    bool bWasSharedI2SGlitchSafe = _bIsSharedI2SGlitchSafe;
    _bIsSharedI2SGlitchSafe = true;
    bool bHwModeHasChanged = false;

    switch(_iAndroidMode) {

    case AudioSystem::MODE_NORMAL:
    case AudioSystem::MODE_RINGTONE:
        break;

    case AudioSystem::MODE_IN_CALL:

        if (popcount(getDevices(EOutput)) > 1 || !isModemAlive() || !isModemAudioAvailable()) {

            iTmpHwMode = AudioSystem::MODE_NORMAL;
            _bIsSharedI2SGlitchSafe = false;

        }
        break;

    case AudioSystem::MODE_IN_COMMUNICATION:

        if (popcount(getDevices(EOutput)) > 1) {

            iTmpHwMode = AudioSystem::MODE_NORMAL;
        }

        break;
    }

    if (iTmpHwMode != getHwMode()) {

        _iHwMode = iTmpHwMode;

        if (_iHwMode == AudioSystem::MODE_IN_CALL) {
            CVolumeKeys::wakeupEnable();
        } else {
            CVolumeKeys::wakeupDisable();
        }

        bHwModeHasChanged = true;
    }
    if (_bIsSharedI2SGlitchSafe != bWasSharedI2SGlitchSafe) {

        setPlatformStateEvent(ESharedI2SStateChange);
    }
    return bHwModeHasChanged;
}

void CAudioPlatformState::clearPlatformStateEvents()
{
    _uiPlatformEventChanged = 0;
}


void CAudioPlatformState::setDirectStreamEvent(uint32_t uiFlags)
{
    ALOGV("%s: flags=0x%X refCount=%d", __FUNCTION__, uiFlags, _uiDirectStreamsRefCount);

    bool hadDirectStreams = (_uiDirectStreamsRefCount != 0);

    if (uiFlags & DIRECT_STREAM_FLAGS) {

        _uiDirectStreamsRefCount += 1;
    } else {

        ALOGW_IF(_uiDirectStreamsRefCount == 0,
                            "%s: Decrement null direct stream reference counter "
                            "ok if restarting media", __FUNCTION__);
        if (_uiDirectStreamsRefCount > 0) {

            _uiDirectStreamsRefCount -= 1;
        }
    }
    if (hadDirectStreams != (_uiDirectStreamsRefCount != 0)) {

        setPlatformStateEvent(EStreamEvent);
    }
}

}       // namespace android
