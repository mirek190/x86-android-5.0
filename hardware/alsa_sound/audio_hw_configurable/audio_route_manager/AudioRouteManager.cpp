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

#define LOG_TAG "RouteManager"
#include <utils/Log.h>
#include <cutils/bitops.h>
#include <string>

#include <AudioHardwareALSA.h>
#include <ALSAStreamOps.h>
#include <AudioStreamInALSA.h>
#include <AudioStreamOutALSA.h>
#include "EventThread.h"
#include "AudioRouteManager.h"
#include "AudioRoute.h"
#include "AudioStreamRoute.h"
#include "AudioPortGroup.h"
#include "AudioPort.h"
#include "AudioPlatformState.h"
#include "ParameterMgrPlatformConnector.h"
#include "SelectionCriterionInterface.h"
#include "ModemAudioManagerInterface.h"
#include "Property.h"
#include "InterfaceProviderLib.h"

#include "AudioPlatformHardware.h"
#include "AudioParameterHandler.h"
#include "AudioCommsAssert.hpp"

#include <fcntl.h>
#include <cutils/uevent.h>

using namespace android;
using namespace std;
using audio_comms::utilities::BitField;

typedef RWLock::AutoRLock AutoR;
typedef RWLock::AutoWLock AutoW;

namespace android_audio_legacy
{

const char* const CAudioRouteManager::ROUTING_LOCKED_PROP_NAME = "AudioComms.HAL.isLocked";

// Defines the name of the Android property describing the name of the PFW configuration file
const char* const CAudioRouteManager::PFW_CONF_FILE_NAME_PROP_NAME = "AudioComms.PFW.ConfPath";

const char* const CAudioRouteManager::gPfwConfFileDefaultName =
                                "/etc/parameter-framework/ParameterFrameworkConfiguration.xml";

const uint32_t CAudioRouteManager::_uiTimeoutSec = 2;

const char* const CAudioRouteManager::gpcVoiceVolume =
                                        "/Audio/CONFIGURATION/VOICE_VOLUME_CTRL_PARAMETER";

const uint32_t CAudioRouteManager::VOIP_RATE_FOR_NARROW_BAND_PROCESSING = 8000;

const int CAudioRouteManager::UEVENT_MSG_MAX_LEN = 1024;

const int CAudioRouteManager::SOCKET_BUFFER_DEFAULT_SIZE = 64 * 1024;

/// PFW related definitions
// Logger
class CParameterMgrPlatformConnectorLogger : public CParameterMgrPlatformConnector::ILogger
{
public:
    CParameterMgrPlatformConnectorLogger() {}

    virtual void log(bool bIsWarning, const string& strLog)
    {
        const static char format[] = "parameter-framework: %s";

        if (bIsWarning) {

            ALOGW(format, strLog.c_str());
        } else {

            ALOGD(format, strLog.c_str());
        }
    }
};

const char* const CAudioRouteManager::LINE_IN_TO_HEADSET_LINE_VOLUME =
        "/Audio/CIRRUS/SOUND_CARD/MIXER/HEADPHONE_LINE/INPUT_PATH_SOURCE/VOLUME";
const char* const CAudioRouteManager::LINE_IN_TO_SPEAKER_LINE_VOLUME =
        "/Audio/CIRRUS/SOUND_CARD/MIXER/SPEAKERPHONE_LINE/INPUT_PATH_SOURCE/VOLUME";

const char* const CAudioRouteManager::LINE_IN_TO_EAR_SPEAKER_LINE_VOLUME =
        "/Audio/CIRRUS/SOUND_CARD/MIXER/EAR_SPEAKER_LINE/INPUT_PATH_SOURCE/VOLUME";

const CAudioRouteManager::CriteriaInterface CAudioRouteManager::ARRAY_CRITERIA_INTERFACE[CAudioRouteManager::ENbCriteria] = {
    {"Mode",                    CAudioRouteManager::EModeCriteriaType},
    {"FmMode",                  CAudioRouteManager::EOffOnCriteriaType},
    {"TTYDirection",            CAudioRouteManager::ETtyDirectionCriteriaType},
    {"RoutageState",            CAudioRouteManager::ERoutingStageCriteriaType},
    {"ClosingCaptureRoutes",    CAudioRouteManager::ERouteCriteriaType},
    {"ClosingPlaybackRoutes",   CAudioRouteManager::ERouteCriteriaType},
    {"OpenedCaptureRoutes",     CAudioRouteManager::ERouteCriteriaType},
    {"OpenedPlaybackRoutes",    CAudioRouteManager::ERouteCriteriaType},
    {"SelectedInputDevice",     CAudioRouteManager::EInputDeviceCriteriaType},
    {"SelectedOutputDevice",    CAudioRouteManager::EOutputDeviceCriteriaType},
    {"AudioSource",             CAudioRouteManager::EInputSourceCriteriaType},
    {"BandRinging",             CAudioRouteManager::EBandRingingCriteriaType},
    {"BandType",                CAudioRouteManager::EBandCriteriaType},
    {"BtHeadsetNrEc",           CAudioRouteManager::EBooleanCriteriaType},
    {"BtHeadsetBandType",       CAudioRouteManager::EBandCriteriaType},
    {"HAC",                     CAudioRouteManager::EOffOnCriteriaType},
    {"ScreenState",             CAudioRouteManager::EOffOnCriteriaType},
    {"BypassNonLinearPp",       CAudioRouteManager::EOffOnCriteriaType},
    {"BypassLinearPp",          CAudioRouteManager::EOffOnCriteriaType},
    {"MicMute",                 CAudioRouteManager::EOffOnCriteriaType},

};

// Mode type
const CAudioRouteManager::SSelectionCriterionTypeValuePair CAudioRouteManager::MODE_VALUE_PAIRS[] = {
    { AudioSystem::MODE_NORMAL,             "Normal" },
    { AudioSystem::MODE_RINGTONE,           "RingTone" },
    { AudioSystem::MODE_IN_CALL,            "InCsvCall" },
    { AudioSystem::MODE_IN_COMMUNICATION,   "InVoipCall" },
    { AudioSystem::NUM_MODES + 1,           "InImsCall" }
};

// Selected Input Device type
const CAudioRouteManager::SSelectionCriterionTypeValuePair CAudioRouteManager::INPUT_DEVICE_VALUE_PAIRS[] = {
    { AudioSystem::DEVICE_IN_COMMUNICATION,         "Communication" },
    { AudioSystem::DEVICE_IN_AMBIENT,               "Ambient" },
    { AudioSystem::DEVICE_IN_BUILTIN_MIC,           "Main" },
    { AudioSystem::DEVICE_IN_BLUETOOTH_SCO_HEADSET, "SCO_Headset" },
    { AudioSystem::DEVICE_IN_WIRED_HEADSET,         "Headset" },
    { AudioSystem::DEVICE_IN_AUX_DIGITAL,           "AuxDigital" },
    { AudioSystem::DEVICE_IN_VOICE_CALL,            "VoiceCall" },
    { AudioSystem::DEVICE_IN_BACK_MIC,              "Back" }
};

// Selected Output Device type
const CAudioRouteManager::SSelectionCriterionTypeValuePair CAudioRouteManager::OUTPUT_DEVICE_VALUE_PAIRS[] = {
    { AudioSystem::DEVICE_OUT_EARPIECE,                     "Earpiece" },
    { AudioSystem::DEVICE_OUT_SPEAKER,                      "IHF" },
    { AudioSystem::DEVICE_OUT_WIRED_HEADSET,                "Headset" },
    { AudioSystem::DEVICE_OUT_WIRED_HEADPHONE,              "Headphones" },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_SCO,                "SCO" },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET,        "SCO_Headset" },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT,         "SCO_CarKit" },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP,               "A2DP" },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES,    "A2DP_Headphones" },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER,       "A2DP_Speaker" },
    { AudioSystem::DEVICE_OUT_AUX_DIGITAL,                  "AuxDigital" },
    { AudioSystem::DEVICE_OUT_WIDI,                         "_Widi"},
};

/**
 * Routing stage criteria.
 *
 * A Flow stage on ClosingRoutes leads to a Mute.
 * A Flow stage on OpenedRoutes leads to an Unmute.
 * A Path stage on ClosingRoutes leads to an Disable.
 * A Path stage on OpenedRoutes leads to an Enable.
 * A Configure stage on ClosingRoutes leads to reset the configuration.
 * A Configure stage on OpenedRoute lead to set the configuration.
 */
const CAudioRouteManager::SSelectionCriterionTypeValuePair CAudioRouteManager::ROUTING_STAGE_VALUE_PAIRS[] = {
    { CAudioRouteManager::EFlow ,       "Flow" },
    { CAudioRouteManager::EPath ,       "Path" },
    { CAudioRouteManager::EConfigure ,  "Configure" }
};

// Audio Source
const CAudioRouteManager::SSelectionCriterionTypeValuePair CAudioRouteManager::AUDIO_SOURCE_VALUE_PAIRS[] = {
    { 0,                                               "None" },
    { static_cast<int>(BitField::indexToMask(AUDIO_SOURCE_DEFAULT)),             "Default" },
    { static_cast<int>(BitField::indexToMask(AUDIO_SOURCE_MIC)),                 "Mic" },
    { static_cast<int>(BitField::indexToMask(AUDIO_SOURCE_VOICE_UPLINK)),        "VoiceUplink" },
    { static_cast<int>(BitField::indexToMask(AUDIO_SOURCE_VOICE_DOWNLINK)),      "VoiceDownlink" },
    { static_cast<int>(BitField::indexToMask(AUDIO_SOURCE_VOICE_CALL)),          "VoiceCall" },
    { static_cast<int>(BitField::indexToMask(AUDIO_SOURCE_CAMCORDER)),           "Camcorder" },
    { static_cast<int>(BitField::indexToMask(AUDIO_SOURCE_VOICE_RECOGNITION)),
      "VoiceRecognition" },
    { static_cast<int>(BitField::indexToMask(AUDIO_SOURCE_VOICE_COMMUNICATION)),
      "VoiceCommunication" }
};

// Voice Codec Band
// Band ringing
const CAudioRouteManager::SSelectionCriterionTypeValuePair CAudioRouteManager::BAND_TYPE_VALUE_PAIRS[] = {
    { 0 , "Unknown" },
    { 1 , "NB" },
    { 2 , "WB" },
    { 3 , "SuperWB" },
};

// TTY mode
const CAudioRouteManager::SSelectionCriterionTypeValuePair CAudioRouteManager::TTY_DIRECTION_VALUE_PAIRS[] = {
    { 1 , "Downlink" },
    { 2 , "Uplink" },
};

// Band ringing
const CAudioRouteManager::SSelectionCriterionTypeValuePair CAudioRouteManager::BAND_RINGING_VALUE_PAIRS[] = {
    { 1 , "NetworkGenerated" },
    { 2 , "PhoneGenerated" }
};

const CAudioRouteManager::SSelectionCriterionTypeValuePair CAudioRouteManager::BOOLEAN_VALUE_PAIRS[] = {
    { 0 , "False" },
    { 1 , "True" }
};

const CAudioRouteManager::SSelectionCriterionTypeValuePair CAudioRouteManager::OFF_ON_VALUE_PAIRS[] = {
    { 0 , "Off" },
    { 1 , "On" }
};

const CAudioRouteManager::SSelectionCriterionTypeInterface CAudioRouteManager::ARRAY_CRITERIA_TYPES[] = {
    // Mode
    {
        CAudioRouteManager::EModeCriteriaType,
        CAudioRouteManager::MODE_VALUE_PAIRS,
        sizeof(CAudioRouteManager::MODE_VALUE_PAIRS) /
            sizeof(CAudioRouteManager::MODE_VALUE_PAIRS[0]),
        false
    },
    // Boolean
    {
        CAudioRouteManager::EBooleanCriteriaType,
        CAudioRouteManager::BOOLEAN_VALUE_PAIRS,
        sizeof(CAudioRouteManager::BOOLEAN_VALUE_PAIRS) /
            sizeof(CAudioRouteManager::BOOLEAN_VALUE_PAIRS[0]),
        false
    },
    // TTY Direction
    {
        CAudioRouteManager::ETtyDirectionCriteriaType,
        CAudioRouteManager::TTY_DIRECTION_VALUE_PAIRS,
        sizeof(CAudioRouteManager::TTY_DIRECTION_VALUE_PAIRS) /
            sizeof(CAudioRouteManager::TTY_DIRECTION_VALUE_PAIRS[0]),
        true
    },
    // Routing Stage
    {
        CAudioRouteManager::ERoutingStageCriteriaType,
        CAudioRouteManager::ROUTING_STAGE_VALUE_PAIRS,
        sizeof(CAudioRouteManager::ROUTING_STAGE_VALUE_PAIRS) /
            sizeof(CAudioRouteManager::ROUTING_STAGE_VALUE_PAIRS[0]),
        true
    },
    // Route
    {
        CAudioRouteManager::ERouteCriteriaType,
        NULL,
        CAudioPlatformHardware::getNbRoutes(),
        true
    },
    // InputDevice
    {
        CAudioRouteManager::EInputDeviceCriteriaType,
        CAudioRouteManager::INPUT_DEVICE_VALUE_PAIRS,
        sizeof(CAudioRouteManager::INPUT_DEVICE_VALUE_PAIRS) /
            sizeof(CAudioRouteManager::INPUT_DEVICE_VALUE_PAIRS[0]),
        true
    },
    // OutputDevice
    {
        CAudioRouteManager::EOutputDeviceCriteriaType,
        CAudioRouteManager::OUTPUT_DEVICE_VALUE_PAIRS,
        sizeof(CAudioRouteManager::OUTPUT_DEVICE_VALUE_PAIRS) /
            sizeof(CAudioRouteManager::OUTPUT_DEVICE_VALUE_PAIRS[0]),
        true
    },
    // Input Source
    {
        CAudioRouteManager::EInputSourceCriteriaType,
        CAudioRouteManager::AUDIO_SOURCE_VALUE_PAIRS,
        sizeof(CAudioRouteManager::AUDIO_SOURCE_VALUE_PAIRS) /
            sizeof(CAudioRouteManager::AUDIO_SOURCE_VALUE_PAIRS[0]),
        false
    },
    // Band Ringing
    {
        CAudioRouteManager::EBandRingingCriteriaType,
        CAudioRouteManager::BAND_RINGING_VALUE_PAIRS,
        sizeof(CAudioRouteManager::BAND_RINGING_VALUE_PAIRS) /
            sizeof(CAudioRouteManager::BAND_RINGING_VALUE_PAIRS[0]),
        false
    },
    // Band Type
    {
        CAudioRouteManager::EBandCriteriaType,
        CAudioRouteManager::BAND_TYPE_VALUE_PAIRS,
        sizeof(CAudioRouteManager::BAND_TYPE_VALUE_PAIRS) /
            sizeof(CAudioRouteManager::BAND_TYPE_VALUE_PAIRS[0]),
        false
    },
    // Off/On
    {
        CAudioRouteManager::EOffOnCriteriaType,
        CAudioRouteManager::OFF_ON_VALUE_PAIRS,
        sizeof(CAudioRouteManager::OFF_ON_VALUE_PAIRS) /
            sizeof(CAudioRouteManager::OFF_ON_VALUE_PAIRS[0]),
        false
    },
};

// MAMGR library name property
const char* const CAudioRouteManager::MODEM_LIB_PROP_NAME = "audiocomms.modemLib";

const char* const CAudioRouteManager::BLUETOOTH_HFP_SUPPORTED_PROP_NAME = "Audiocomms.BT.HFP.Supported";
const bool CAudioRouteManager::BLUETOOTH_HFP_SUPPORTED_DEFAULT_VALUE = true;

CAudioRouteManager::CAudioRouteManager(AudioHardwareALSA *pParent) :
    uevent_fd(-1),
    _pParameterMgrPlatformConnectorLogger(new CParameterMgrPlatformConnectorLogger),
    _pVoiceVolumeParamHandle(NULL),
    _pModemAudioManagerInterface(NULL),
    _pPlatformState(new CAudioPlatformState(this)),
    _pEventThread(new CEventThread(this)),
    _bIsStarted(false),
    _bRoutingLocked(TProperty<bool>(ROUTING_LOCKED_PROP_NAME, true)),
    _pParent(pParent),
    _pAudioParameterHandler(new CAudioParameterHandler()),
    _pEchoReference(NULL)
{
    _stRoutes[CUtils::EInput].uiNeedReconfig = 0;
    _stRoutes[CUtils::EOutput].uiNeedReconfig = 0;

    _stRoutes[CUtils::EInput].uiEnabled = 0;
    _stRoutes[CUtils::EOutput].uiEnabled = 0;

    _stRoutes[CUtils::EInput].uiPrevEnabled = 0;
    _stRoutes[CUtils::EOutput].uiPrevEnabled = 0;

    // Try to connect a ModemAudioManager Interface
    NInterfaceProvider::IInterfaceProvider* pMAMGRInterfaceProvider = getInterfaceProvider(TProperty<string>(MODEM_LIB_PROP_NAME).getValue().c_str());
    if (pMAMGRInterfaceProvider == NULL) {

        ALOGW("No MAMGR library");
    } else {

        // Retrieve the ModemAudioManager Interface
        _pModemAudioManagerInterface = static_cast<IModemAudioManagerInterface*>(pMAMGRInterfaceProvider->queryInterface(IModemAudioManagerInterface::getInterfaceName()));
        if (_pModemAudioManagerInterface == NULL) {

            ALOGW("Failed to get ModemAudioManager interface");
        } else {

            // Declare ourselves as observer
            _pModemAudioManagerInterface->setModemAudioManagerObserver(this);
            ALOGI("Connected to a ModemAudioManager interface");
        }
    }

    /// Connector
    // Fetch the name of the PFW configuration file: this name is stored in an Android property
    // and can be different for each hardware
    string strParameterConfigurationFilePath = TProperty<string>(PFW_CONF_FILE_NAME_PROP_NAME,
                                                                 gPfwConfFileDefaultName);
    ALOGI("parameter-framework: using configuration file: %s", strParameterConfigurationFilePath.c_str());

    // Actually create the Connector
    _pParameterMgrPlatformConnector = new CParameterMgrPlatformConnector(strParameterConfigurationFilePath);

    // Logger
    _pParameterMgrPlatformConnector->setLogger(_pParameterMgrPlatformConnectorLogger);

    int index;
    /// Criteria Types
    for (index = 0; index < ENbCriteriaTypes; index++) {

        _apCriteriaTypeInterface[index] = createAndFillSelectionCriterionType((CriteriaType)index);
    }

    /// Criteria
    for (index = 0; index < ENbCriteria; index++) {

        _apSelectedCriteria[index] = _pParameterMgrPlatformConnector->createSelectionCriterion(ARRAY_CRITERIA_INTERFACE[index].pcName,
                                                                                           _apCriteriaTypeInterface[ARRAY_CRITERIA_INTERFACE[index].eCriteriaType]);
    }

    createAudioHardwarePlatform();

    //Check if platform supports Bluetooth HFP
    _bBluetoothHFPSupported = TProperty<bool>(BLUETOOTH_HFP_SUPPORTED_PROP_NAME, BLUETOOTH_HFP_SUPPORTED_DEFAULT_VALUE);
    if (_bBluetoothHFPSupported){

        ALOGI("%s(): platform supports Bluetooth HFP", __FUNCTION__);
    } else {

        ALOGI("%s(): platform does NOT support Bluetooth HFP", __FUNCTION__);
    }

    // Platform embeds a modem if we found a modem interface
    _pPlatformState->setModemEmbedded(pMAMGRInterfaceProvider != NULL);
    if (_pPlatformState->isModemEmbedded()) {

        ALOGD("%s: platform embeds a Modem chip", __FUNCTION__);
    } else {

        ALOGD("%s: platform does NOT embed a Modem chip", __FUNCTION__);
    }
}

CAudioRouteManager::~CAudioRouteManager()
{
    AutoW lock(_lock);

    if (_pModemAudioManagerInterface != NULL) {

        // Unsuscribe & stop ModemAudioManager
        _pModemAudioManagerInterface->setModemAudioManagerObserver(NULL);
        _pModemAudioManagerInterface->stop();
    }

    if (_bIsStarted) {

        _pEventThread->stop();
    }
    delete _pEventThread;

    RouteListIterator it;
    // Delete all routes
    for (it = _routeList.begin(); it != _routeList.end(); ++it) {

        delete *it;
    }

    // Remove Voice Call Volume Parameter Handle
    delete _pVoiceVolumeParamHandle;
    // Unset logger
    _pParameterMgrPlatformConnector->setLogger(NULL);
    // Remove logger
    delete _pParameterMgrPlatformConnectorLogger;
    // Remove connector
    delete _pParameterMgrPlatformConnector;
    // Remove parameter handler
    delete _pAudioParameterHandler;
    // Remove Platform State component
    delete _pPlatformState;
}

void CAudioRouteManager::initRouting()
{
    muteRoutes(CUtils::EInput);
    muteRoutes(CUtils::EOutput);
    _apSelectedCriteria[ESelectedRoutingStage]->setCriterionState(EConfigure|EPath|EFlow);
    _pParameterMgrPlatformConnector->applyConfigurations();
}

status_t CAudioRouteManager::start()
{
    AutoW lock(_lock);

    assert(!_bIsStarted);

    _bIsStarted = true;

    uevent_init();

    // Start Event thread
    _pEventThread->start();

    // Start Modem Audio Manager
    startModemAudioManager();

    ALOGD_IF(isModemLess(), "%s: Note that this platform is MODEM-LESS", __FUNCTION__);

    // Start PFW
    std::string strError;
    if (!_pParameterMgrPlatformConnector->start(strError)) {

        ALOGE("parameter-framework start error: %s", strError.c_str());
        _pEventThread->stop();
        return NO_INIT;
    }
    initRouting();
    ALOGI("parameter-framework successfully started!");

    return NO_ERROR;
}

void CAudioRouteManager::uevent_init()
{
    uevent_fd = uevent_open_socket(SOCKET_BUFFER_DEFAULT_SIZE, true);
    ALOGE_IF(uevent_fd < 0, "uevent_init: uevent_open_socket failed");

    if (uevent_fd >= 0) {
        // Add & Listen
        _pEventThread->addOpenedFd(FdFromSstDriver, uevent_fd, true);
    }
}

//
// Must be called from WLocked context
//
void CAudioRouteManager::reconsiderRouting(bool bIsSynchronous)
{
    ALOGD("%s", __FUNCTION__);

    assert(_bStarted && !_pEventThread->inThreadContext());

    if (!bIsSynchronous) {

        // Trigs the processing of the list
        _pEventThread->trig(EUpdateRouting);
    } else {

        // Synchronization semaphore
        CSyncSemaphore syncSemaphore;

        // Push sync semaphore
        _clientWaitSemaphoreList.add(&syncSemaphore);

        // Trig the processing of the list
        _pEventThread->trig(EUpdateRouting);

        // Unlock to allow for sem wait
        _lock.unlock();

        // Wait
        syncSemaphore.wait();

        // Relock
        _lock.writeLock();
    }

    ALOGD("%s: DONE", __FUNCTION__);
}

//
// From worker thread context
// This function requests to evaluate the routing for all the streams
// after a mode change, a modem event ...
// must be called with AudioHardwareALSA::mLock held
//
void CAudioRouteManager::doReconsiderRouting()
{
    // Reset availability of all route (All routes to be available)
    resetAvailability();

    // Parse all streams and affect route to it according to applicability of the route
    bool bRoutesWillChange = prepareRouting();

    ALOGD("%s: %s",__FUNCTION__,
          bRoutesWillChange? "      Platform State:" : "      Platform Changes:");
    ALOGD_IF(!isModemLess() &&
             (bRoutesWillChange ||
              _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EModemStateChange)),
             "%s:          -Modem Alive = %d %s", __FUNCTION__,
             _pPlatformState->isModemAlive(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EModemStateChange) ? "[has changed]" : "");
    ALOGD_IF(!isModemLess() &&
             (bRoutesWillChange ||
              _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EModemAudioStatusChange)),
             "%s:          -Modem Call Active = %d %s", __FUNCTION__,
             _pPlatformState->isModemAudioAvailable(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EModemAudioStatusChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::ESharedI2SStateChange),
             "%s:          -Is Shared I2S glitch free=%d %s", __FUNCTION__, _pPlatformState->isSharedI2SBusAvailable(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::ESharedI2SStateChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EAndroidModeChange),
             "%s:          -Android Telephony Mode = %s %s", __FUNCTION__,
             _apCriteriaTypeInterface[EModeCriteriaType]->getFormattedState(_pPlatformState->getMode()).c_str(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EAndroidModeChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EHwModeChange),
             "%s:          -RTE MGR HW Mode = %s %s", __FUNCTION__,
             _apCriteriaTypeInterface[EModeCriteriaType]->getFormattedState(_pPlatformState->getHwMode()).c_str(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EHwModeChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange ||
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EBtEnableChange),
             "%s:          -BT Enabled = %d %s", __FUNCTION__, _pPlatformState->isBtEnabled(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EBtEnableChange) ?
                 "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange ||
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EScoRscReqStateChange),
             "%s:          -ScoRscReqStateChange = %d %s", __FUNCTION__,
             _pPlatformState->isScoResourceRequested(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EScoRscReqStateChange) ?
             "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange ||
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EBtHeadsetNrEcChange),
             "%s:          -BT NREC = %d %s", __FUNCTION__,
             _pPlatformState->isBtHeadsetNrEcEnabled(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EBtHeadsetNrEcChange) ?
                 "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange ||
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EBtHeadsetBandTypeChange),
             "%s:          -BT Band = %s %s", __FUNCTION__,
             _apCriteriaTypeInterface[EBandCriteriaType]->getFormattedState(
                 _pPlatformState->getBtHeadsetBandType()).c_str(),
             _pPlatformState->hasPlatformStateChanged(
                 CAudioPlatformState::EBtHeadsetBandTypeChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EOutputDevicesChange),
             "%s:          -Platform output device = %s %s", __FUNCTION__,
             _apCriteriaTypeInterface[EOutputDeviceCriteriaType]->getFormattedState(_pPlatformState->getDevices(true)).c_str(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EOutputDevicesChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EInputDevicesChange),
             "%s:          -Platform input device = %s %s", __FUNCTION__,
             _apCriteriaTypeInterface[EInputDeviceCriteriaType]->getFormattedState(_pPlatformState->getDevices(false)).c_str(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EInputDevicesChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EInputSourceChange),
             "%s:          -Platform input source = %s %s", __FUNCTION__,
             _apCriteriaTypeInterface[EInputSourceCriteriaType]->getFormattedState(_pPlatformState->getInputSource()).c_str(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EInputSourceChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EBandTypeChange),
             "%s:          -Platform Band type = %s %s", __FUNCTION__,
             _apCriteriaTypeInterface[EBandCriteriaType]->getFormattedState(_pPlatformState->getBandType()).c_str(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EBandTypeChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EStreamEvent),
             "%s:          -Platform Has Direct Stream = %s %s", __FUNCTION__,
             _pPlatformState->hasDirectStreams() ? "yes" : "no",
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EStreamEvent) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::ETtyDirectionChange),
             "%s:          -Platform TTY direction = %s %s", __FUNCTION__,
             _apCriteriaTypeInterface[ETtyDirectionCriteriaType]->getFormattedState(_pPlatformState->getTtyDirection()).c_str(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::ETtyDirectionChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EHacModeChange),
             "%s:          -Platform HAC Mode = %s %s", __FUNCTION__,
             _apCriteriaTypeInterface[EOffOnCriteriaType]->getFormattedState(_pPlatformState->isHacEnabled()).c_str(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EHacModeChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EScreenStateChange),
             "%s:          -Platform Screen State = %s %s", __FUNCTION__,
             _apCriteriaTypeInterface[EOffOnCriteriaType]->getFormattedState(_pPlatformState->isScreenOn()).c_str(),
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EScreenStateChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EContextAwarenessStateChange),
             "%s:          -Platform Context Awareness = %s %s", __FUNCTION__,
             _pPlatformState->isContextAwarenessEnabled() ? "true" : "false",
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EContextAwarenessStateChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(
                CAudioPlatformState::EAlwaysListeningStateChange),
             "%s:          -Platform Always Listening = %s %s",
             __FUNCTION__,
             _pPlatformState->isAlwaysListeningEnabled() ? "true" : "false",
             _pPlatformState->hasPlatformStateChanged(
                CAudioPlatformState::EAlwaysListeningStateChange) ? "[has changed]" : ""
            );
    ALOGD_IF(bRoutesWillChange || _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EFmStateChange),
             "%s:          -Platform FM State = %s %s", __FUNCTION__,
             _pPlatformState->isFmStateOn()? "On" : "Off",
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EFmStateChange) ? "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange ||
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EBypassNonLinearPpStateChange),
             "%s:          -Platform Bypass Non Linear PP State = %s %s", __FUNCTION__,
             _pPlatformState->bypassedNonLinearPp()? "On" : "Off",
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EBypassNonLinearPpStateChange) ?
                                                      "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange ||
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EBypassLinearPpStateChange),
             "%s:          -Platform Bypass Linear PP State = %s %s", __FUNCTION__,
             _pPlatformState->bypassedLinearPp()? "On" : "Off",
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EBypassLinearPpStateChange) ?
                                                      "[has changed]" : "");
    ALOGD_IF(bRoutesWillChange ||
             _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EMicMuteChange),
             "%s:          -Platform MicMute = %s %s", __FUNCTION__,
             _apCriteriaTypeInterface[EOffOnCriteriaType]->
             getFormattedState(_pPlatformState->getMicMute()).c_str(),
             _pPlatformState->hasPlatformStateChanged(
                 CAudioPlatformState::EMicMuteChange) ? "[has changed]" : "");

    if (bRoutesWillChange) {

        ALOGD("%s:      Route state:", __FUNCTION__);
        ALOGD("%s:          -Previously Enabled Route in Input = %s", __FUNCTION__,
              _apCriteriaTypeInterface[ERouteCriteriaType]->getFormattedState(_stRoutes[CUtils::EInput].uiPrevEnabled).c_str());
        ALOGD("%s:          -Previously Enabled Route in Output = %s", __FUNCTION__,
              _apCriteriaTypeInterface[ERouteCriteriaType]->getFormattedState(_stRoutes[CUtils::EOutput].uiPrevEnabled).c_str());
        ALOGD("%s:          -Selected Route in Input = %s", __FUNCTION__,
              _apCriteriaTypeInterface[ERouteCriteriaType]->getFormattedState(_stRoutes[CUtils::EInput].uiEnabled).c_str());
        ALOGD("%s:          -Selected Route in Output = %s", __FUNCTION__,
              _apCriteriaTypeInterface[ERouteCriteriaType]->getFormattedState(_stRoutes[CUtils::EOutput].uiEnabled).c_str());
        ALOGD("%s:          -Route that need reconfiguration in Input = %s", __FUNCTION__,
              _apCriteriaTypeInterface[ERouteCriteriaType]->getFormattedState(_stRoutes[CUtils::EInput].uiNeedReconfig).c_str());
        ALOGD("%s:          -Route that need reconfiguration in Output = %s", __FUNCTION__,
              _apCriteriaTypeInterface[ERouteCriteriaType]->getFormattedState(_stRoutes[CUtils::EOutput].uiNeedReconfig).c_str());

        executeRouting();
    }

    // Clear Platform State flag
    _pPlatformState->clearPlatformStateEvents();

    // Complete all synchronous requests
    _clientWaitSemaphoreList.sync();

    ALOGV("%s: End of Routing Reconsideration", __FUNCTION__);
}

bool CAudioRouteManager::isStarted() const
{
    return _pParameterMgrPlatformConnector && _pParameterMgrPlatformConnector->isStarted();
}

void CAudioRouteManager::executeRouting()
{
    executeMuteStage();

    executeDisableStage();

    executeConfigureStage();

    executeEnableStage();

    executeUnmuteStage();
}

//
// From ALSA Stream In / Out via AudiohardwareALSA
//
status_t CAudioRouteManager::setStreamParameters(ALSAStreamOps* pStream, const String8 &keyValuePairs, int iMode)
{
    AutoW lock(_lock);

    AudioParameter param = AudioParameter(keyValuePairs);
    status_t status;
    bool bForOutput = pStream->isOut();
    String8 key;

    /// Stream Flags
    if (bForOutput) {
        int iFlags;
        key = String8(AudioParameter::keyStreamFlags);
        status = param.getInt(key, iFlags);
        if (status == NO_ERROR) {

            // Remove parameter
            param.remove(key);

            AudioStreamOutALSA* pStreamOut = static_cast<AudioStreamOutALSA*>(pStream);
            setOutputFlags(pStreamOut, iFlags);
        }
    }

    /// Devices (ie routing key)
    int devices;
    key = String8(AudioParameter::keyRouting);
    status = param.getInt(key, devices);

    if (status == NO_ERROR) {

        // Remove parameter
        param.remove(key);

        // Ask the route manager to update the stream devices
        setDevices(pStream, devices);

        // Retrieve if provide, the input source for an input stream
        if (!bForOutput) {

            key = String8(AudioParameter::keyInputSource);

            /// Get concerned input source
            int inputSource;
            status = param.getInt(key, inputSource);

            AudioStreamInALSA* pStreamIn = static_cast<AudioStreamInALSA*>(pStream);
            if (status == NO_ERROR) {

                // Remove parameter
                param.remove(key);

                // Found the input source
                setInputSourceMask(pStreamIn, BitField::indexToMask(inputSource));
            }
            if (devices == 0) {

                // When this function is called with a null device, considers it as
                // an unrouting request, restore source to None (0) within route manager
                setInputSourceMask(pStreamIn, 0);
            }
        } else {

            // For output streams, latch Android Mode
            _pPlatformState->setMode(iMode);
        }

        _pPlatformState->updateHwMode();
    }

    /// Process pending platform changes
    if (_pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EOutputDevicesChange | CAudioPlatformState::EInputDevicesChange |
                                                 CAudioPlatformState::EStreamEvent | CAudioPlatformState::EInputSourceChange |
                                                 CAudioPlatformState::EAndroidModeChange | CAudioPlatformState::EHwModeChange)) {

        ALOGD("%s: {+++ RECONSIDER ROUTING +++} due to %s stream parameter change",
              __FUNCTION__,
              pStream->isOut() ? "output": "input");
        // Reconsider the routing now
        reconsiderRouting(_bRoutingLocked);

    } else {

        ALOGD("%s: identical Platform State, do not reconsider routing", __FUNCTION__);
    }

    // No more?
    if (param.size()) {

        // Just log!
        ALOGW("Unhandled argument.");
    }

    return NO_ERROR;
}

//
// From ALSA Stream In / Out via ALSAStreamOps
//
status_t CAudioRouteManager::startStream(bool bIsStreamOut)
{
    AutoW lock(_lock);

    ALOGD("%s: {+++ RECONSIDER ROUTING +++} due to %s stream start event",
          __FUNCTION__,
          bIsStreamOut ? "output" : "input");
    //
    // SYNCHRONOUS RECONSIDERATION of the routing in case of stream start
    //
    reconsiderRouting();
    return NO_ERROR;
}

//
// From ALSA Stream In / Out via ALSAStreamOps
//
status_t CAudioRouteManager::stopStream(bool bIsStreamOut)
{
    AutoW lock(_lock);

    ALOGD("%s: {+++ RECONSIDER ROUTING +++} due to %s stream stop event",
          __FUNCTION__,
          bIsStreamOut ? "output" : "input");
    //
    // SYNCHRONOUS RECONSIDERATION of the routing in case of stream stop
    //
    reconsiderRouting();
    return NO_ERROR;
}

//
// Called from AudioHardwareALSA
//
status_t CAudioRouteManager::setParameters(const String8& keyValuePairs)
{
    AutoW lock(_lock);

    AudioParameter param = AudioParameter(keyValuePairs);
    status_t status;
    String8 strRst;

    String8 key = String8("restarting");
    status = param.get(key, strRst);
    if (status == NO_ERROR) {

        if (strRst == "true") {

            // Restore the audio parameters when mediaserver is restarted in case of crash.
            ALOGI("Restore audio parameters as mediaserver is restarted param=%s", _pAudioParameterHandler->getParameters().string());
            doSetParameters(_pAudioParameterHandler->getParameters());
        }
        param.remove(key);
    } else {

        // Set the audio parameters
        status = doSetParameters(keyValuePairs);
        if (status == NO_ERROR) {

            // Do not save Stream parameter
            param.remove(String8(AudioParameter::keyStreamFlags));

            ALOGV("%s: saving %s", __FUNCTION__, keyValuePairs.string());
            // Save the audio parameters for recovering audio parameters in case of crash.
            _pAudioParameterHandler->saveParameters(param.toString());
        } else {

            return status;
        }
    }
    return NO_ERROR;
}

//
// Called from AudioHardwareALSA
//
String8 CAudioRouteManager::getParameters(const String8& keyValuePairs)
{
    ALOGV("%s: key value pair %s", __FUNCTION__, keyValuePairs.string());

    //
    // Search FM state parameter
    //
    if (keyValuePairs.find(String8("fm-mode")) < 0) {
       AudioParameter param = AudioParameter(keyValuePairs);
       return param.toString();
    }

    return String8(_pPlatformState->isFmStateOn() ? "fm-mode=on" : "fm-mode=off");
}

//
// Called from locked context
//
status_t CAudioRouteManager::doSetParameters(const String8& keyValuePairs)
{
    ALOGV("%s: key value pair %s", __FUNCTION__, keyValuePairs.string());

    AudioParameter param = AudioParameter(keyValuePairs);

    // Search stream flags settings
    doSetStreamFlagsParameters(param);

    // Search TTY settings
    doSetTtyParameters(param);

    // Search HAC settings
    doSetHacParameters(param);

    // Search screen settings
    doSetScreenStateParameters(param);

    // Search FM settings
    doSetFmParameters(param);

    // Search BT settings
    doSetBTParameters(param);

    /** Search SCO resource request */
    doScoResourceRequestParameters(param);

    // Search context awareness parameter
    doSetContextAwarenessParameters(param);

    // Search always listening parameter
    doSetAlwaysListeningParameters(param);

    // Search bypass non linear PP parameter
    doBypassNonLinearPpParameters(param);

    // Search bypass linear PP parameter
    doBypassLinearPpParameters(param);

    if (param.size()) {

        ALOGW("%s: Unhandled argument.", __FUNCTION__);
    }

    // Reconsider the routing now
    if (_pPlatformState->hasPlatformStateChanged()) {

        //
        // ASYNCHRONOUS RECONSIDERATION of the routing in case of a parameter change
        //
        ALOGD("%s: key value pair %s, {+++ RECONSIDER ROUTING +++} due to External parameter change",
              __FUNCTION__,
              keyValuePairs.string());
        reconsiderRouting(_bRoutingLocked);
    }

    return NO_ERROR;
}

void CAudioRouteManager::doSetContextAwarenessParameters(AudioParameter &param)
{
    String8 strContextAwarenessStatus;
    String8 key = String8(AUDIO_PARAMETER_KEY_CONTEXT_AWARENESS_STATUS);

    // Search context awareness parameter
    if (param.get(key, strContextAwarenessStatus) == NO_ERROR) {

        _pPlatformState->setContextAwarenessStatus(
                    strContextAwarenessStatus == AUDIO_PARAMETER_VALUE_CONTEXT_AWARENESS_ON);
        param.remove(key);
    }
}

void CAudioRouteManager::doSetAlwaysListeningParameters(AudioParameter &param)
{
    String8 alwaysListeningStatus;
    String8 key(AUDIO_PARAMETER_KEY_ALWAYS_LISTENING_STATUS);

    // Search always listening parameter
    if (param.get(key, alwaysListeningStatus) == NO_ERROR) {

        _pPlatformState->setAlwaysListeningStatus(
            alwaysListeningStatus == AUDIO_PARAMETER_VALUE_ALWAYS_LISTENING_ON);
        param.remove(key);
    }
}

void CAudioRouteManager::doSetBTParameters(AudioParameter &param)
{
    status_t status;
    String8 key;
    String8 value;

    //
    // Search BT STATE parameter
    //
    key = String8(AUDIO_PARAMETER_KEY_BLUETOOTH_STATE);

    // Get concerned devices
    status = param.get(key, value);
    // Returns no_error if the key was found
    if (status == NO_ERROR)
    {

        _pPlatformState->setBtEnabled(value == AUDIO_PARAMETER_VALUE_BLUETOOTH_STATE_ON);
        param.remove(key);
    }

    //
    // Search BT NREC parameter
    //
    key = String8(AUDIO_PARAMETER_KEY_BT_NREC);

    // Get BT NREC setting value
    status = param.get(key, value);
    // Returns no_error if the key was found
    if (status == NO_ERROR) {

        param.remove(key);

        // BT NREC must be ignored if we don't want to support HFP devices
        if (_bBluetoothHFPSupported) {

            // OFF means the headset embeds its noise reduction and echo cancellation algorithms"
            _pPlatformState->setBtHeadsetNrEc(value == AUDIO_PARAMETER_VALUE_OFF);
        } else {

            LOGD("HFP not supported: ignore %s=%s", key.string(), value.string());
        }
    }

    //
    // Search BT WBS parameter
    //

    key = String8(AUDIO_PARAMETER_KEY_BT_WBS);

    // Get BT WBS setting value
    status = param.get(key, value);
    // Returns no_error if the key was found
    if (status == NO_ERROR) {

        param.remove(key);

        // BT WBS must be ignored if we don't want to support HFP devices
        if (_bBluetoothHFPSupported) {

            _pPlatformState->setBtHeadsetBandType(value == AUDIO_PARAMETER_VALUE_ON ?
                                                      CAudioBand::EWide :
                                                      CAudioBand::ENarrow
                                                      );
        } else {

            LOGD("HFP not supported: ignore %s=%s", key.string(), value.string());
        }
    }
}

void CAudioRouteManager::doScoResourceRequestParameters(AudioParameter &param)
{
    String8 strScoRequestSetting;
    String8 key(AUDIO_PARAMETER_KEY_SCO_RESOURCE);

    /** Search SCO resource request setting key */
    if (param.get(key, strScoRequestSetting) == NO_ERROR) {

        _pPlatformState->setScoResourceRequested(
            strScoRequestSetting == AUDIO_PARAMETER_VALUE_SCO_RESOURCE_REQUESTED);
        param.remove(key);
    }
}

void CAudioRouteManager::doBypassNonLinearPpParameters(AudioParameter &param)
{
    String8 strBypassNonLinearPpSetting;
    String8 key = String8(AUDIO_PARAMETER_KEY_BYPASS_NON_LINEAR_POSTPROCESSING_SETTING);

    // Search bypass non linear PP setting key
    if (param.get(key, strBypassNonLinearPpSetting) == NO_ERROR) {

        _pPlatformState->bypassNonLinearPp(
            strBypassNonLinearPpSetting == AUDIO_PARAMETER_VALUE_BYPASS_NON_LINEAR_PP_ON);
        param.remove(key);
    }
}

void CAudioRouteManager::doBypassLinearPpParameters(AudioParameter &param)
{
    String8 strBypassLinearPpSetting;
    String8 key = String8(AUDIO_PARAMETER_KEY_BYPASS_LINEAR_POSTPROCESSING_SETTING);

    // Search bypass linear PP setting key
    if (param.get(key, strBypassLinearPpSetting) == NO_ERROR) {

        _pPlatformState->bypassLinearPp(
            strBypassLinearPpSetting == AUDIO_PARAMETER_VALUE_BYPASS_LINEAR_PP_ON);
        param.remove(key);
    }
}

void CAudioRouteManager::doSetTtyParameters(AudioParameter &param)
{
    String8 strTtyDevice;
    String8 key = String8(AUDIO_PARAMETER_KEY_TTY_MODE);

    // Search TTY key
    if (param.get(key, strTtyDevice) == NO_ERROR) {

        int iTtyDirection = 0;
        if (strTtyDevice == AUDIO_PARAMETER_VALUE_TTY_FULL) {

            iTtyDirection = TTY_DOWNLINK | TTY_UPLINK;
        }
        else if (strTtyDevice == AUDIO_PARAMETER_VALUE_TTY_HCO) {

            iTtyDirection = TTY_UPLINK;
        }
        else if (strTtyDevice == AUDIO_PARAMETER_VALUE_TTY_VCO) {

            iTtyDirection = TTY_DOWNLINK;
        }

        _pPlatformState->setTtyDirection(iTtyDirection);
        param.remove(key);
    }
}

void CAudioRouteManager::doSetHacParameters(AudioParameter &param)
{
    String8 strHacSetting;
    String8 key = String8(AUDIO_PARAMETER_KEY_HAC_SETTING);

    // Search HAC setting key
    if (param.get(key, strHacSetting) == NO_ERROR) {

        _pPlatformState->setHacMode(strHacSetting == AUDIO_PARAMETER_VALUE_HAC_ON);
        param.remove(key);
    }
}

void CAudioRouteManager::setMicMute(bool state)
{
    AutoW lock(_lock);
    _pPlatformState->setMicMute(state);
    doReconsiderRouting();
}

bool CAudioRouteManager::getMicMute() {
    return _pPlatformState->getMicMute();
}

void CAudioRouteManager::doSetStreamFlagsParameters(AudioParameter &param)
{
    int iFlags;
    String8 key = String8(AudioParameter::keyStreamFlags);

    // Search Stream Flags key
    if (param.getInt(key, iFlags) == NO_ERROR) {

        _pPlatformState->setDirectStreamEvent(iFlags);
        param.remove(key);
    }
}

void CAudioRouteManager::doSetScreenStateParameters(AudioParameter &param)
{
    String8 strScreenState;
    String8 key = String8(AUDIO_PARAMETER_KEY_SCREEN_STATE);

    // Search Screen state key
    if (param.get(key, strScreenState) == NO_ERROR) {

        _pPlatformState->setScreenState(strScreenState == AUDIO_PARAMETER_VALUE_ON);
        param.remove(key);
    }
}

void CAudioRouteManager::doSetFmParameters(AudioParameter &param)
{
    static const char* const FM_ROUTE_PARAM_KEY = "route-fm";
    String8 strKey = String8(FM_ROUTE_PARAM_KEY);
    String8 strFmRoute;

    // Search FM key
    if (param.get(strKey, strFmRoute) == NO_ERROR) {

        // Search FM parameter values
        static const char* const FM_ROUTE_REQUESTING_SPEAKER = "speaker";
        static const char* const FM_ROUTE_REQUESTING_HEADSET = "headset";

        _pPlatformState->setFmState(strFmRoute == FM_ROUTE_REQUESTING_SPEAKER ||
                                    strFmRoute == FM_ROUTE_REQUESTING_HEADSET);
        param.remove(strKey);
    }
}

//
// Update devices attribute of the stream
//
void CAudioRouteManager::setDevices(ALSAStreamOps* pStream, uint32_t devices)
{
    ALOGD("%s: 0x%X", __FUNCTION__, devices);
    if ((devices & AUDIO_DEVICE_BIT_IN) != 0) {

        devices &= ~AUDIO_DEVICE_BIT_IN;
    }
    ALOGD("%s: 0x%X", __FUNCTION__, devices);

    bool bIsOut = pStream->isOut();

    // Update Platform state: in/out devices
    _pPlatformState->setDevices(devices, bIsOut);

    ALOGD("%s: set device = %s to %s stream", __FUNCTION__,
          bIsOut ?
              _apCriteriaTypeInterface[EOutputDeviceCriteriaType]->getFormattedState(devices).c_str() :
              _apCriteriaTypeInterface[EInputDeviceCriteriaType]->getFormattedState(devices).c_str(),
          bIsOut ? "output": "input");

    // set the new device for this stream
    pStream->setNewDevices(devices);
}

//
// From AudioInStreams: update the active input source
// Assumption: only one active input source at one time.
// @todo: Does it make sense to keep it in the platform state???
//
void CAudioRouteManager::setInputSourceMask(AudioStreamInALSA* pStreamIn, uint32_t inputSource)
{
    pStreamIn->setInputSourceMask(inputSource);

    ALOGD("%s: inputSource = %s", __FUNCTION__,
          _apCriteriaTypeInterface[EInputSourceCriteriaType]->getFormattedState(inputSource).c_str());
    _pPlatformState->setInputSourceMask(inputSource);

    if (inputSource == BitField::indexToMask(AUDIO_SOURCE_VOICE_COMMUNICATION)) {

        CAudioBand::Type eBand = CAudioBand::EWide;
        if (pStreamIn->sampleRate() == VOIP_RATE_FOR_NARROW_BAND_PROCESSING) {

            eBand = CAudioBand::ENarrow;

        }
        _pPlatformState->setBandType(eBand, AudioSystem::MODE_IN_COMMUNICATION);
    }
}

void CAudioRouteManager::setOutputFlags(AudioStreamOutALSA* pStreamOut, uint32_t uiFlags)
{
    uint32_t uiPreviousFlags = pStreamOut->getFlags();
    pStreamOut->setFlags(uiFlags);

    ALOGD("%s: output flags = 0x%X (Prev Flags=0x%X)", __FUNCTION__, uiFlags, uiPreviousFlags);
}

void CAudioRouteManager::createsRoutes()
{
    for (uint32_t i = 0; i < CAudioPlatformHardware::getNbRoutes(); i++) {

        CAudioRoute* pRoute = CAudioPlatformHardware::createAudioRoute(i, _pPlatformState);

        if (!pRoute) {

            ALOGE("%s: could not find routeIndex %d", __FUNCTION__, i);
            return ;
        }
        ALOGV("%s: add %s (Index=%d) to route manager", __FUNCTION__, pRoute->getName().c_str(), i);

        _routeList.push_back(pRoute);

        // Add ports to the route
        uint32_t uiPortsUsed = CAudioPlatformHardware::getPortsUsedByRoute(i);

        ALOGV("%s: uiPortsUsed=0x%X",  __FUNCTION__, uiPortsUsed);

        LOG_ALWAYS_FATAL_IF(popcount(uiPortsUsed) > 2);

        for (uint32_t uiPort = 0; uiPort < CAudioPlatformHardware::getNbPorts(); uiPort++) {

            uint32_t uiPortId = CAudioPlatformHardware::getPortId(uiPort);
            if (uiPortsUsed & uiPortId) {

                pRoute->addPort(findPortById(uiPortId));
            }
        }
    }

}

void CAudioRouteManager::createAudioHardwarePlatform()
{
    AutoW lock(_lock);

    ALOGV("%s", __FUNCTION__);

    for (uint32_t i = 0; i < CAudioPlatformHardware::getNbPorts(); i++) {

        addPort(i);
    }

    for (uint32_t i = 0; i < CAudioPlatformHardware::getNbPortGroups(); i++) {

        addPortGroup(i);
    }

    createsRoutes();
}


void CAudioRouteManager::addPort(uint32_t uiPortIndex)
{
    ALOGV("%s", __FUNCTION__);

    CAudioPort* pPort = new CAudioPort(uiPortIndex);

    if (pPort == NULL) {

        return ;
    }
    _portList.push_back(pPort);
}

void CAudioRouteManager::addPortGroup(uint32_t uiPortGroupIndex)
{
    ALOGV("%s", __FUNCTION__);

    CAudioPortGroup* pAudioPortGroup = new CAudioPortGroup();

    if (pAudioPortGroup == NULL) {

        return ;
    }

    _portGroupList.push_back(pAudioPortGroup);

    uint32_t uiPortsUsedByPortGroup = CAudioPlatformHardware::getPortsUsedByPortGroup(uiPortGroupIndex);

    for (uint32_t i = 0; i < CAudioPlatformHardware::getNbPorts(); i++) {

        if (uiPortsUsedByPortGroup & CAudioPlatformHardware::getPortId(i)) {

            pAudioPortGroup->addPortToGroup(findPortById(CAudioPlatformHardware::getPortId(i)));
        }
    }
}

//
// This functionr resets the availability of all routes:
// ie it resets both used flags.
//
void CAudioRouteManager::resetAvailability()
{
    ALOGV("%s", __FUNCTION__);

    CAudioRoute *aRoute =  NULL;
    RouteListIterator it;

    // Find the applicable route for this route request
    for (it = _routeList.begin(); it != _routeList.end(); ++it) {

        aRoute = *it;
        aRoute->resetAvailability();
    }

    // Reset Port availability
    PortListIterator itport;

    // Find the applicable route for this route request
    for (itport = _portList.begin(); itport != _portList.end(); ++itport) {

        CAudioPort* aPort = *itport;
        aPort->resetAvailability();
    }
}

//
// Add a stream to route manager
//
void CAudioRouteManager::addStream(ALSAStreamOps* pStream)
{
    AutoW lock(_lock);

    bool bIsOut = pStream->isOut();

    ALOGV("%s: add %s stream to route manager", __FUNCTION__, bIsOut ? "output" : "input");

    // Add Stream Out to the list
    _streamsList[bIsOut].push_back(pStream);
}

//
// Remove a stream from route manager
//
void CAudioRouteManager::removeStream(ALSAStreamOps* pStream)
{
    AutoW lock(_lock);

    ALOGV("%s", __FUNCTION__);

    ALSAStreamOpsListIterator it;

    bool isOut = pStream->isOut();

    for (it = _streamsList[isOut].begin(); it != _streamsList[isOut].end(); ++it) {

        ALSAStreamOps* pOps = *it;

        if (pOps == pStream) {

            // Remove element
            _streamsList[isOut].erase(it);

            // Done
            break;
        }
    }
}

//
// Returns true if the routing scheme has changed, false otherwise
//
bool CAudioRouteManager::prepareRouting()
{
    ALOGV("\t\t %s", __FUNCTION__);

    // Return true if any changes observed routes (input or output direction)
    return prepareRouting(CUtils::EOutput) | prepareRouting(CUtils::EInput);
}

//
// This function virtually connects all streams to their
// new route according to:
//     -applicability of the route
//     -availability of the route
//
// Returns true if previous enabled route is different from current enabled route
//              or if any route needs to be reconfigured.
//         false otherwise (the list of enabled route did not change, no route
//              needs to be reconfigured)
//
bool CAudioRouteManager::prepareRouting(bool bIsOut)
{
    ALOGV("%s for %s", __FUNCTION__,
          bIsOut ? "output" : "input");

    // Save Enabled routes bit field
    _stRoutes[bIsOut].uiPrevEnabled = _stRoutes[bIsOut].uiEnabled;

    // Reset Enabled Routes
    _stRoutes[bIsOut].uiEnabled = 0;

    // Reset Need reconfiguration Routes
    _stRoutes[bIsOut].uiNeedReconfig = 0;

    // Go through the list of routes
    RouteListIterator it;

    // Find the applicable route for this route request
    for (it = _routeList.begin(); it != _routeList.end(); ++it) {

        CAudioRoute *pRoute =  *it;

        prepareRoute(pRoute, bIsOut);

    }
    return (_stRoutes[bIsOut].uiPrevEnabled != _stRoutes[bIsOut].uiEnabled) || (_stRoutes[bIsOut].uiNeedReconfig != 0);
}

void CAudioRouteManager::prepareRoute(CAudioRoute* pRoute, bool bIsOut)
{
    uint32_t uiDevices = _pPlatformState->getDevices(bIsOut);
    int iMode = _pPlatformState->getHwMode();

    //
    // First check if the route has slaves routes to evaluate
    // slave routes first
    //
    uint32_t uiSlaveRoutes = pRoute->getSlaveRoutes();
    if (uiSlaveRoutes) {

        // Go through the list of routes
        RouteListIterator it;
        int iOneSlaveAtLeastEnabled = 0;

        // Find the applicable route for this route request
        for (it = _routeList.begin(); it != _routeList.end(); ++it) {

            CAudioRoute *pSlaveRoute =  *it;
            uint32_t uiRouteId = pSlaveRoute->getRouteId();

            if (uiSlaveRoutes & uiRouteId) {

                prepareRoute(pSlaveRoute, bIsOut);
                iOneSlaveAtLeastEnabled += pSlaveRoute->willBeUsed(bIsOut);
            }
        }
        if (!iOneSlaveAtLeastEnabled) {

            ALOGV("%s: route %s not applicable as no dependant routes applicable", __FUNCTION__, pRoute->getName().c_str());
            return ;
        }
    }

    if (pRoute->getRouteType() == CAudioRoute::EExternalRoute || pRoute->getRouteType() == CAudioRoute::ECompressedStreamRoute) {

        if (pRoute->isApplicable(uiDevices, iMode, bIsOut)) {

            ALOGV("%s: route %s is applicable [mode = %s devices= %s]", __FUNCTION__,
                  pRoute->getName().c_str(),
                  _apCriteriaTypeInterface[EModeCriteriaType]->getFormattedState(iMode).c_str(),
                  bIsOut ?
                      _apCriteriaTypeInterface[EOutputDeviceCriteriaType]->getFormattedState(uiDevices).c_str() :
                      _apCriteriaTypeInterface[EInputDeviceCriteriaType]->getFormattedState(uiDevices).c_str());

            // Route is not condemned -> its port are now busy by this ext route
            // It will automatically condemn all mutual exclusive ports used by this route
            pRoute->setUsed(bIsOut);
            // Add route to enabled route bit field
            _stRoutes[bIsOut].uiEnabled |= pRoute->getRouteId();
        }

    } else if (pRoute->getRouteType() == CAudioRoute::EStreamRoute) {

        ALSAStreamOps* pStreamOps;

        // Check if a stream route is applicable for this stream
        pStreamOps = findApplicableStreamForRoute(bIsOut, pRoute);

        if (pStreamOps) {

            CAudioStreamRoute* pStreamRoute = static_cast<CAudioStreamRoute*>(pRoute);
            pStreamRoute->setStream(pStreamOps);

            pRoute->setUsed(bIsOut);

            // Add route to enabled route bit field
            _stRoutes[bIsOut].uiEnabled |= pRoute->getRouteId();

            pRoute->checkAndSetNeedRerouting(bIsOut);
        }
    }

    if (pRoute->needReconfiguration(bIsOut)) {

        // Add route to NeedToReconfigureRoute bit field
        _stRoutes[bIsOut].uiNeedReconfig |= pRoute->getRouteId();
    }
}

ALSAStreamOps* CAudioRouteManager::findApplicableStreamForRoute(bool bIsOut, const CAudioRoute *pRoute)
{
    if (pRoute->getRouteType() != CAudioRoute::EStreamRoute) {

        return NULL;
    }

    ALSAStreamOpsListIterator it;

    for (it = _streamsList[bIsOut].begin(); it != _streamsList[bIsOut].end(); ++it) {

        ALSAStreamOps* pOps = *it;
        assert(isOut == pOps->isOut());

        // Check stream state - only evaluates streams that are started
        if (pOps->isStarted() && !pOps->isRouteAssignedToStream()) {

            // Check if the route is applicable
            // Applicability will also check if this route is already busy or not.

            // Applicability mask depends on the direction of the stream
            //  -Output stream: output Flags
            //  -Input stream: input source
            uint32_t uiApplicabilityMask = pOps->getApplicabilityMask();

            if (pRoute->isApplicable(_pPlatformState->getDevices(bIsOut),
                                     _pPlatformState->getHwMode(),
                                     bIsOut,
                                     uiApplicabilityMask)) {

                ALOGV("%s: route %s is applicable", __FUNCTION__, pRoute->getName().c_str());

                // Got it! Bailing out
                return pOps;
            }
        }
    }
    return NULL;
}

void CAudioRouteManager::executeMuteStage()
{
    ALOGD("%s: --------------- Routing Stage = Mute ---------------", __FUNCTION__);

    _apSelectedCriteria[ESelectedRoutingStage]->setCriterionState(EFlow);

    // Mute Routes that
    //      -are disabled
    //      -need reconfiguration
    // Routing order criterion = Mute
    muteRoutes(CUtils::EInput);
    muteRoutes(CUtils::EOutput);

    _pParameterMgrPlatformConnector->applyConfigurations();
}

void CAudioRouteManager::muteRoutes(bool bIsOut)
{
    ALOGV("%s for %s:", __FUNCTION__,
          bIsOut ? "output" : "input");

    //
    // OpenedRoutes criteria: Routes that were opened before reconsidering the routing,
    //                        and will remain enabled and do not need to be reconfigured
    uint32_t uiOpenedRoutes = _stRoutes[bIsOut].uiPrevEnabled &
                                _stRoutes[bIsOut].uiEnabled &
                                ~_stRoutes[bIsOut].uiNeedReconfig;

    // ClosingRoutes criteria: routes that were opened before reconsidering the routing,
    //                         and either will be closed or need reconfiguration
    //                         Mute action will be applied on ClosingRoutes.
    uint32_t uiClosingRoutes = (_stRoutes[bIsOut].uiPrevEnabled &
                                ~_stRoutes[bIsOut].uiEnabled) |
                                _stRoutes[bIsOut].uiNeedReconfig;

    ALOGD_IF(uiClosingRoutes,
             "%s: Expected Routes to be muted in %s = %s", __FUNCTION__,
             bIsOut ? "Output" : "Input",
             _apCriteriaTypeInterface[ERouteCriteriaType]->getFormattedState(uiClosingRoutes).c_str());

    // Warn PFW
    selectedClosingRoutes(bIsOut)->setCriterionState(uiClosingRoutes);
    selectedOpenedRoutes(bIsOut)->setCriterionState(uiOpenedRoutes);
}

void CAudioRouteManager::executeUnmuteStage()
{
    ALOGD("%s: --------------- Routing Stage = Unmute ---------------", __FUNCTION__);

    // Warn PFW
    _apSelectedCriteria[ESelectedRoutingStage]->setCriterionState(EConfigure|EPath|EFlow);

    _pParameterMgrPlatformConnector->applyConfigurations();
}

void CAudioRouteManager::executeConfigureStage()
{
    ALOGD("%s: --------------- Routing Stage = Configure ---------------", __FUNCTION__);

    // Warn PFW
    _apSelectedCriteria[ESelectedRoutingStage]->setCriterionState(EConfigure);

    configureRoutes(CUtils::EOutput);
    configureRoutes(CUtils::EInput);

    // Change here the devices, the mode, ... all the criteria required for the routing
    _apSelectedCriteria[ESelectedMode]->setCriterionState(
                _pPlatformState->getHwMode());

    _apSelectedCriteria[ESelectedTtyDirection]->setCriterionState(
                _pPlatformState->getTtyDirection());

    _apSelectedCriteria[ESelectedBtHeadsetNrEc]->setCriterionState(
                _pPlatformState->isBtHeadsetNrEcEnabled());

    _apSelectedCriteria[ESelectedBtHeadsetBand]->setCriterionState(
                _pPlatformState->getBtHeadsetBandType());

    _apSelectedCriteria[ESelectedBand]->setCriterionState(
                _pPlatformState->getBandType());

    _apSelectedCriteria[ESelectedHacMode]->setCriterionState(
                _pPlatformState->isHacEnabled());

    _apSelectedCriteria[ESelectedScreenState]->setCriterionState(
                _pPlatformState->isScreenOn());

    _apSelectedCriteria[ESelectedBypassNonLinearPp]->setCriterionState(
                _pPlatformState->bypassedNonLinearPp());

    _apSelectedCriteria[ESelectedBypassLinearPp]->setCriterionState(
                _pPlatformState->bypassedLinearPp());

    _apSelectedCriteria[ESelectedMicMute]->setCriterionState(
                _pPlatformState->getMicMute());

    _pParameterMgrPlatformConnector->applyConfigurations();
}

void CAudioRouteManager::configureRoutes(bool bIsOut)
{
    ALOGV("%s for %s:", __FUNCTION__,
          bIsOut ? "output" : "input");

    RouteListIterator it;
    for (it = _routeList.begin(); it != _routeList.end(); ++it) {

        CAudioRoute *route = *it;

        if (!route->needRerouting(bIsOut) &&
            ( route->getRouteId() & _stRoutes[bIsOut].uiNeedReconfig)) {
            route->configure(bIsOut);
        }
    }
    //
    // OpenedRoutes criteria: Routes that will be opened after reconsidering the routing,
    // ClosingRoutes criteria: reset here, as no more closing action required
    //
    selectedDevice(bIsOut)->setCriterionState(_pPlatformState->getDevices(bIsOut));
    if (!bIsOut) {

        _apSelectedCriteria[ESelectedInputSource]->setCriterionState(_pPlatformState->getInputSource());
    }
    selectedClosingRoutes(bIsOut)->setCriterionState(0);
    selectedOpenedRoutes(bIsOut)->setCriterionState(_stRoutes[bIsOut].uiEnabled);
}

void CAudioRouteManager::executeDisableStage()
{
    ALOGD("%s: --------------- Routing Stage = Disable ---------------", __FUNCTION__);

    _apSelectedCriteria[ESelectedRoutingStage]->setCriterionState(EPath);

    // Routing order criterion = Disable
    // starting from input streams

    prepareDisableRoutes(CUtils::EInput);
    prepareDisableRoutes(CUtils::EOutput);

    doDisableRoutes(CUtils::EInput);
    doDisableRoutes(CUtils::EOutput);

    _pParameterMgrPlatformConnector->applyConfigurations();

    doPostDisableRoutes<CUtils::EInput>();
    doPostDisableRoutes<CUtils::EOutput>();
}

void CAudioRouteManager::prepareDisableRoutes(bool bIsOut)
{
    ALOGV("%s for %s:", __FUNCTION__,
          bIsOut ? "output" : "input");

    //
    // OpenedRoutes criteria: Routes that were opened before reconsidering the routing,
    //                        and will remain enabled so that the PFW mutes these routes
    // ClosingRoutes criteria: routes that were opened before reconsidering the routing,
    //                         and will be closed
    //
    uint32_t uiOpenedRoutes = _stRoutes[bIsOut].uiPrevEnabled & _stRoutes[bIsOut].uiEnabled;
    uint32_t uiClosingRoutes = _stRoutes[bIsOut].uiPrevEnabled & ~_stRoutes[bIsOut].uiEnabled;

    ALOGD_IF(uiClosingRoutes,
             "%s: Routes to be disabled(unrouted) in %s = %s",  __FUNCTION__,
             bIsOut ? "Output" : "Input",
             _apCriteriaTypeInterface[ERouteCriteriaType]->getFormattedState(uiClosingRoutes).c_str());

    selectedClosingRoutes(bIsOut)->setCriterionState(uiClosingRoutes);
    selectedOpenedRoutes(bIsOut)->setCriterionState(uiOpenedRoutes);
}

void CAudioRouteManager::doDisableRoutes(bool isOut, bool isPostDisable)
{
    RouteListIterator it;

    //
    // Performs the unrouting on the routes
    //
    for (it = _routeList.begin(); it != _routeList.end(); ++it) {

        CAudioRoute *route = *it;

        //
        // Disable Routes that were opened before reconsidering the routing
        // and will be closed after.
        //
        if ((route->currentlyUsed(isOut) && !route->willBeUsed(isOut)) ||
                route->needRerouting(isOut)) {
            route->unroute(isOut, isPostDisable);
        }
    }
}

void CAudioRouteManager::executeEnableStage()
{
    ALOGD("%s: --------------- Routing Stage = Enable ---------------", __FUNCTION__);

    // Warn PFW
    _apSelectedCriteria[ESelectedRoutingStage]->setCriterionState(EPath|EConfigure);

    doPreEnableRoutes<CUtils::EOutput>();
    doPreEnableRoutes<CUtils::EInput>();

    _pParameterMgrPlatformConnector->applyConfigurations();

    // Connect all streams that need to be connected (starting from output streams)
    doEnableRoutes(CUtils::EOutput);
    doEnableRoutes(CUtils::EInput);
}

void CAudioRouteManager::doEnableRoutes(bool isOut, bool isPreEnable)
{
    ALOGV("%s for %s:", __FUNCTION__,
          isOut ? "output" : "input");

    RouteListIterator it;

    // Enable Routes that were not enabled and will be enabled after
    // the routing reconsideration
    //
    ALOGD_IF(_stRoutes[isOut].uiEnabled & ~_stRoutes[isOut].uiPrevEnabled,
             "%s: Routes to be enabled(routed) in %s = %s", __FUNCTION__,
             isOut ? "Output" : "Input",
             _apCriteriaTypeInterface[ERouteCriteriaType]->getFormattedState(
                 _stRoutes[isOut].uiEnabled & ~_stRoutes[isOut].uiPrevEnabled).c_str());

    for (it = _routeList.begin(); it != _routeList.end(); ++it) {

        CAudioRoute* route = *it;

        // If the route is external and was set busy -> it needs to be routed
        if ((!route->currentlyUsed(isOut) && route->willBeUsed(isOut)) ||
                route->needRerouting(isOut)) {

            if (route->route(isOut, isPreEnable) != NO_ERROR) {
                // Just logging
                ALOGE("\t error while routing %s", route->getName().c_str());
            }
        }
    }
}

CAudioRoute* CAudioRouteManager::findRouteById(uint32_t uiRouteId)
{
    CAudioRoute* pFoundRoute =  NULL;
    RouteListIterator it;

    // Find the applicable route for this route request
    for (it = _routeList.begin(); it != _routeList.end(); ++it) {

        CAudioRoute* pRoute = *it;
        if (uiRouteId == pRoute->getRouteId()) {

            pFoundRoute = pRoute;
            break;
        }
    }
    return pFoundRoute;
}

CAudioPort* CAudioRouteManager::findPortById(uint32_t uiPortId)
{
    CAudioPort* pFoundPort =  NULL;
    PortListIterator it;

    // Find the applicable route for this route request
    for (it = _portList.begin(); it != _portList.end(); ++it) {

        CAudioPort* pPort = *it;
        if (uiPortId == pPort->getPortId()) {

            pFoundPort = pPort;
            break;
        }
    }
    return pFoundPort;
}

void CAudioRouteManager::startModemAudioManager()
{
    if (isModemLess()) {

        return;
    }
    // Starts the ModemAudioManager
    if(!_pModemAudioManagerInterface->start()) {

        ALOGW("%s: could not start ModemAudioManager.", __FUNCTION__);
        _pModemAudioManagerInterface->setModemAudioManagerObserver(NULL);
        _pModemAudioManagerInterface = NULL;
        return;
    }
    /// Initialize current modem status
    // Modem status
    _pPlatformState->setModemAlive(_pModemAudioManagerInterface->isModemAlive());
    // Modem audio availability
    _pPlatformState->setModemAudioAvailable(_pModemAudioManagerInterface->isModemAudioAvailable());
    // Modem band
    _pPlatformState->setBandType(_pModemAudioManagerInterface->getAudioBand(), AudioSystem::MODE_IN_CALL);

    ALOGD("%s: success", __FUNCTION__);
}

//
// From IModemStatusNotifier
// Need to hold AudioHardwareALSA::mLock
//
void CAudioRouteManager::onModemAudioStatusChanged()
{
    ALOGD("%s: in", __FUNCTION__);

    _pEventThread->trig(EUpdateModemAudioStatus);
}

//
// From IModemStatusNotifier
// Called on Modem State change reported by ModemAudioManager
// Need to hold AudioHardwareALSA::mLock
//
void  CAudioRouteManager::onModemStateChanged()
{
    ALOGD("%s: in", __FUNCTION__);

    _pEventThread->trig(EUpdateModemState);
}

//
// From IModemStatusNotifier
// Called on Modem Audio band change report
//
void  CAudioRouteManager::onModemAudioBandChanged()
{
    ALOGD("%s: in", __FUNCTION__);

    _pEventThread->trig(EUpdateModemAudioBand);
}

//
// Worker thread context
// Event processing
//
bool CAudioRouteManager::onEvent(int iFd)
{
    if (iFd == _pEventThread->getFd(FdFromSstDriver)) {
        char msg[UEVENT_MSG_MAX_LEN + 1];
        char *cp;
        int n;

        n = uevent_kernel_multicast_recv(uevent_fd, msg, UEVENT_MSG_MAX_LEN);
        if (n <= 0 || n > UEVENT_MSG_MAX_LEN) {
            return false;
        }

        msg[n] = '\0';
        cp = msg;
        while (cp < msg + n) {
            if (!strcmp(cp, "EVENT_TYPE=SST_RECOVERY")) {
                LOGE("Encountered SST driver event : %s", cp);
                //Restart Mediaserver to complete LPE recovery
                AUDIOCOMMS_ASSERT(false, "Restarting MediaServer due to SST_RECOVERY event");
                break;
            }
           cp += strlen(cp) + 1;
        }
    }
    return true;
}

//
// Worker thread context
//
bool CAudioRouteManager::onError(int __UNUSED iFd)
{
    return false;
}

//
// Worker thread context
//
bool CAudioRouteManager::onHangup(int __UNUSED iFd)
{
    return false;
}

//
// Worker thread context
//
void CAudioRouteManager::onAlarm()
{
    ALOGD("%s", __FUNCTION__);
}

//
// Worker thread context
//
void CAudioRouteManager::onPollError()
{

}

//
// Worker thread context
//
bool CAudioRouteManager::onProcess(uint16_t __UNUSED uiEvent)
{
    AutoW lock(_lock);

    switch(uiEvent) {
    case EUpdateModemAudioBand:

        ALOGD("%s: {+++ RECONSIDER ROUTING +++} due to Modem Band change", __FUNCTION__);
        // Update the platform state
        _pPlatformState->setBandType(_pModemAudioManagerInterface->getAudioBand(), AudioSystem::MODE_IN_CALL);
        break;

    case EUpdateModemState:
        ALOGD("%s: {+++ RECONSIDER ROUTING +++} due to Modem State change", __FUNCTION__);
        // Update the platform state
        _pPlatformState->setModemAlive(_pModemAudioManagerInterface->isModemAlive());
        // Force Parameters synchronization to reduce cold latency
        if (_pModemAudioManagerInterface->isModemAlive()) {
            _pParameterMgrPlatformConnector->applyConfigurations();
        }
        break;

    case EUpdateModemAudioStatus:
        ALOGD("%s: {+++ RECONSIDER ROUTING +++} due to Modem Audio Status change", __FUNCTION__);
        // Update the platform state
        _pPlatformState->setModemAudioAvailable(_pModemAudioManagerInterface->isModemAudioAvailable());
        break;

    case EUpdateRouting:
        // Nothing to update before call of doReconsiderRouting()
        break;

    default:
        ALOGE("%s: Unhandled event.", __FUNCTION__);
        break;
    }
    doReconsiderRouting();

    return false;
}

// Used to fill types for PFW
ISelectionCriterionTypeInterface* CAudioRouteManager::createAndFillSelectionCriterionType(CriteriaType eCriteriaType) const
{
    uint32_t uiIndex;

    uint32_t uiNbEntries = ARRAY_CRITERIA_TYPES[eCriteriaType]._uiNbValuePairs;
    bool bIsInclusive = ARRAY_CRITERIA_TYPES[eCriteriaType]._bIsInclusive;

    ISelectionCriterionTypeInterface* pSelectionCriterionType = _pParameterMgrPlatformConnector->createSelectionCriterionType(bIsInclusive);

    const SSelectionCriterionTypeValuePair* pSelectionCriterionTypeValuePairs = ARRAY_CRITERIA_TYPES[eCriteriaType]._pCriterionTypeValuePairs;

    for (uiIndex = 0; uiIndex < uiNbEntries; uiIndex++) {

        if (eCriteriaType == ERouteCriteriaType) {

            pSelectionCriterionType->addValuePair(CAudioPlatformHardware::getRouteId(uiIndex), CAudioPlatformHardware::getRouteName(uiIndex));
        } else {

            const SSelectionCriterionTypeValuePair* pValuePair = &pSelectionCriterionTypeValuePairs[uiIndex];
            pSelectionCriterionType->addValuePair(pValuePair->iNumerical, pValuePair->pcLiteral);
        }
    }
    return pSelectionCriterionType;
}

uint32_t CAudioRouteManager::getIntegerParameterValue(const string& strParameterPath, uint32_t uiDefaultValue) const
{
    ALOGV("%s in", __FUNCTION__);

    if (!_pParameterMgrPlatformConnector->isStarted()) {

        return uiDefaultValue;
    }

    string strError;
    // Get handle
    CParameterHandle* pParameterHandle = _pParameterMgrPlatformConnector->createParameterHandle(strParameterPath, strError);

    if (!pParameterHandle) {

        ALOGE("Unable to get parameter handle: '%s' '%s'", strParameterPath.c_str(), strError.c_str());

        ALOGV("%s returning %d", __FUNCTION__, uiDefaultValue);

        return uiDefaultValue;
    }

    // Retrieve value
    uint32_t uiValue;

    strError.clear();
    if (!pParameterHandle->getAsInteger(uiValue, strError)) {

        ALOGE("Unable to get value: %s, from parameter path: %s", strError.c_str(), strParameterPath.c_str());

        ALOGV("%s returning %d", __FUNCTION__, uiDefaultValue);

        // Remove handle
        delete pParameterHandle;

        return uiDefaultValue;
    }

    // Remove handle
    delete pParameterHandle;

    ALOGV("%s: %s is %d", __FUNCTION__, strParameterPath.c_str(), uiValue);

    return uiValue;
}

status_t CAudioRouteManager::getStringParameterValue(const string& strParameterPath,
                                                     string& strValue) const
{
    ALOGV("%s in", __FUNCTION__);

    if (!_pParameterMgrPlatformConnector->isStarted()) {

        return DEAD_OBJECT;
    }

    string strError;
    // Get handle
    CParameterHandle* pParameterHandle =
            _pParameterMgrPlatformConnector->createParameterHandle(strParameterPath, strError);

    if (!pParameterHandle) {

        ALOGE("Unable to get parameter handle: '%s' '%s'", strParameterPath.c_str(), strError.c_str());

        return DEAD_OBJECT;
    }

    // Retrieve value
    status_t ret = OK;
    strError.clear();
    if (!pParameterHandle->getAsString(strValue, strError)) {

        ALOGE("Unable to get value: %s, from parameter path: %s", strError.c_str(),
                                                                  strParameterPath.c_str());
        ret = BAD_VALUE;
    }

    // Remove handle
    delete pParameterHandle;

    ALOGV_IF(!ret, "%s: %s is %s", __FUNCTION__, strParameterPath.c_str(), strValue.c_str());

    return ret;
}

status_t CAudioRouteManager::setIntegerParameterValue(const string& strParameterPath, uint32_t uiValue)
{
    ALOGV("%s in", __FUNCTION__);

    if (!_pParameterMgrPlatformConnector->isStarted()) {

        return INVALID_OPERATION;
    }

    string strError;
    // Get handle
    CParameterHandle* pParameterHandle = _pParameterMgrPlatformConnector->createParameterHandle(strParameterPath, strError);

    if (!pParameterHandle) {

        ALOGE("%s: Unable to set parameter handle: '%s' '%s'", __FUNCTION__, strParameterPath.c_str(), strError.c_str());

        return NAME_NOT_FOUND;
    }

    // set value
    strError.clear();
    if (!pParameterHandle->setAsInteger(uiValue, strError)) {

        ALOGE("%s: Unable to set value: %s, from parameter path: %s", __FUNCTION__, strError.c_str(), strParameterPath.c_str());

        // Remove handle
        delete pParameterHandle;

        return NAME_NOT_FOUND;
    }

    // Remove handle
    delete pParameterHandle;

    ALOGV("%s: %s set to %d", __FUNCTION__, strParameterPath.c_str(), uiValue);

    return NO_ERROR;
}

status_t CAudioRouteManager::setIntegerArrayParameterValue(const string& strParameterPath, std::vector<uint32_t>& uiArray) const
{
    ALOGV("%s in", __FUNCTION__);

    if (!_pParameterMgrPlatformConnector->isStarted()) {

        return INVALID_OPERATION;
    }

    string strError;
    // Get handle
    CParameterHandle* pParameterHandle = _pParameterMgrPlatformConnector->createParameterHandle(strParameterPath, strError);

    if (!pParameterHandle) {

        ALOGE("Unable to get parameter handle: '%s' '%s'", strParameterPath.c_str(), strError.c_str());

        return NAME_NOT_FOUND;
    }

    // set value
    if (!pParameterHandle->setAsIntegerArray(uiArray, strError)) {

        ALOGE("Unable to set value: %s, from parameter path: %s", strError.c_str(), strParameterPath.c_str());

        // Remove handle
        delete pParameterHandle;

        return INVALID_OPERATION;
    }

    // Remove handle
    delete pParameterHandle;

    return NO_ERROR;
}

CParameterHandle* CAudioRouteManager::getDynamicParameterHandle(const string& strDynamicParamPath)
{
    string strParameter;

    // First retrieve the platform dependant parameter path
    status_t ret = getStringParameterValue(strDynamicParamPath, strParameter);
    if (ret != NO_ERROR) {

        ALOGE("Could not retrieve volume path handler err=%d", ret);
        return NULL;
    }
    ALOGD("%s  Platform specific parameter path=%s", __FUNCTION__, strParameter.c_str());

    string strError;
    CParameterHandle* pHandle = _pParameterMgrPlatformConnector->createParameterHandle(strParameter,
                                                                                       strError);
    if (!pHandle) {

        ALOGE("%s: Unable to get parameter handle for %s: '%s'",
                                        __FUNCTION__,
                                        strParameter.c_str(),
                                        strError.c_str());
    }
    return pHandle;
}

CParameterHandle* CAudioRouteManager::getVoiceVolumeHandle()
{
    if (_pVoiceVolumeParamHandle == NULL) {

        _pVoiceVolumeParamHandle = getDynamicParameterHandle(gpcVoiceVolume);
    }
    return _pVoiceVolumeParamHandle;
}

status_t CAudioRouteManager::setVoiceVolume(float gain)
{
    // Protect here concurrent access to Parameter Framework
    AutoR lock(_lock);
    string strError;

    if (!_pPlatformState->isModemEmbedded()) {

        ALOGD("%s: platform does NOT embed a Modem chip", __FUNCTION__);
        return NO_ERROR;
    }

    if (gain < 0.0 || gain > 1.0) {

        ALOGW("%s(%f) out of range [0.0 .. 1.0]", __FUNCTION__, gain);
        return -ERANGE;
    }

    ALOGD("%s gain=%f", __FUNCTION__, gain);
    CParameterHandle* pVoiceVolumeHandle = getVoiceVolumeHandle();

    if (!pVoiceVolumeHandle) {

        ALOGE("Could not retrieve volume path handle");
        return INVALID_OPERATION;
    }

    if (!pVoiceVolumeHandle->setAsDouble(gain, strError)) {

        ALOGE("%s: Unable to set value %f, from parameter path: %s, error=%s",
              __FUNCTION__, gain, pVoiceVolumeHandle->getPath().c_str(), strError.c_str());
        return INVALID_OPERATION;
    }

    return OK;
}

void CAudioRouteManager::resetEchoReference(struct echo_reference_itfe* reference)
{
    AutoW lock(_lock);
    ALOGD(" %s(reference=%p)", __FUNCTION__, reference);
    if (reference == NULL || _pEchoReference != reference) {

        /* Nothing to do */
        return ;
    }

    // First try to find the stream out which that used to provide this echo reference
    ALSAStreamOpsListIterator it;

    for (it = _streamsList[CUtils::EOutput].begin(); it != _streamsList[CUtils::EOutput].end(); ++it) {

        AudioStreamOutALSA* pOut = static_cast<AudioStreamOutALSA*>(*it);
        struct echo_reference_itfe* pReference = pOut->getEchoReference();
        if (pReference != NULL && pReference == reference) {

            pOut->removeEchoReference(reference);
            release_echo_reference(_pEchoReference);
            _pEchoReference = NULL;
            // Only one output is expected to provide the reference
            return ;
        }
    }
    ALOGE("%s: nothing to do, reference not found!", __FUNCTION__);
    release_echo_reference(_pEchoReference);
    _pEchoReference = NULL;
}

struct echo_reference_itfe* CAudioRouteManager::getEchoReference(int format,
                                                                 uint32_t channel_count,
                                                                 uint32_t sampling_rate)
{
    AutoW lock(_lock);

    ALOGD("%s ()", __FUNCTION__);
    resetEchoReference(_pEchoReference);

    if (_streamsList[CUtils::EOutput].empty()) {

        ALOGE("%s: list of output streams is empty,"
              " so problem to provide data reference for AEC effect!", __FUNCTION__);
        return NULL;
    }

    ALSAStreamOpsListIterator it;

    // By default, use the first output stream routed in the output streams list
    for (it = _streamsList[CUtils::EOutput].begin(); it != _streamsList[CUtils::EOutput].end(); ++it) {

        ALSAStreamOps* pOps = *it;
        if (pOps->isRouteAvailable()) {

            ALOGD("%s: format=%d channels=%d samplerate=%d", __FUNCTION__,
                                pOps->format(), pOps->channelCount(), pOps->sampleRate());
            int iWriteFormat = pOps->format();
            uint32_t uiWriteChannelCount = pOps->channelCount();
            uint32_t uiWriteSampleRate = sampling_rate;

            if (create_echo_reference((audio_format_t)format,
                                      channel_count,
                                      sampling_rate,
                                      (audio_format_t)iWriteFormat,
                                      uiWriteChannelCount,
                                      uiWriteSampleRate,
                                      &_pEchoReference) < 0) {

                ALOGE("%s: Could not create echo reference", __FUNCTION__);
                return NULL;
            }
            AudioStreamOutALSA* pOut = static_cast<AudioStreamOutALSA*>(pOps);
            pOut->addEchoReference(_pEchoReference);
        }

    }
    ALOGD(" %s() will return that mEchoReference=%p", __FUNCTION__, _pEchoReference);
    return _pEchoReference;
}

const pcm_config& CAudioRouteManager::getDefaultPcmConfig(bool bIsOut, uint32_t uiFlags) const
{
    return CAudioPlatformHardware::getDefaultPcmConfig(bIsOut, uiFlags);
}

}       // namespace android
