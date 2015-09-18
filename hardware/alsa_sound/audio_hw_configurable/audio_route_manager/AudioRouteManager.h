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

#include <list>
#include <vector>
#include <utils/threads.h>
#include <hardware_legacy/AudioHardwareBase.h>
#include <tinyalsa/asoundlib.h>
#include <BitField.hpp>

#include "AudioRoute.h"
#include "SyncSemaphoreList.h"
#include "Utils.h"
#include "ModemAudioManagerObserver.h"
#include "AudioPlatformState.h"
#include "EventListener.h"


class CEventThread;


class CParameterMgrPlatformConnector;
class CParameterHandle;
class ISelectionCriterionTypeInterface;
class ISelectionCriterionInterface;
struct IModemAudioManagerInterface;
struct echo_reference_itfe;

namespace android_audio_legacy
{
using android::RWLock;
using android::Mutex;

class CParameterMgrPlatformConnectorLogger;
class AudioHardwareALSA;
class AudioStreamInALSA;
class AudioStreamOutALSA;
class ALSAStreamOps;
class AudioStreamInALSA;
class CAudioRoute;
class CAudioPortGroup;
class CAudioPort;
class CAudioStreamRoute;
class CAudioParameterHandler;
class CAudioPlatformState;

class CAudioRouteManager : private IModemAudioManagerObserver, public IEventListener
{
    // Criteria Types
    enum CriteriaType {
        EModeCriteriaType = 0,
        EBooleanCriteriaType,
        ETtyDirectionCriteriaType,
        ERoutingStageCriteriaType,
        ERouteCriteriaType,
        EInputDeviceCriteriaType,
        EOutputDeviceCriteriaType,
        EInputSourceCriteriaType,
        EBandRingingCriteriaType,
        EBandCriteriaType,
        EOffOnCriteriaType,

        ENbCriteriaTypes
    };

    enum EventType {
        EUpdateModemAudioBand,
        EUpdateModemState,
        EUpdateModemAudioStatus,
        EUpdateRouting
    };

    /*
     * Routing stage bits description.
     * It is used to feed the criteria of the Parameter Framework.
     * This criterion is inclusive and is used to encode
     * 5 steps of the routing (Muting, Disabling, Configuring, Enabling, Unmuting)
     * Muting -> EFlow
     * Disabling -> EPath
     * Configuring -> EConfigure
     * Enabling -> EConfigure | EPath
     * Unmuting -> EConfigure | EPath | EFlow
     */
    enum RoutingStage {
        EFlow = (1 << 0),       /**< It refers to umute/unmute steps.   */
        EPath = (1 << 1),       /**< It refers to enable/disable steps  */
        EConfigure = (1 << 2)   /**< It refers to configure step        */
    };

    typedef list<CAudioRoute*>::iterator RouteListIterator;
    typedef list<CAudioRoute*>::const_iterator RouteListConstIterator;
    typedef list<CAudioPort*>::iterator PortListIterator;
    typedef list<CAudioPort*>::iterator PortListConstIterator;
    typedef list<CAudioPortGroup*>::iterator PortGroupListIterator;
    typedef list<CAudioPortGroup*>::iterator PortGroupListConstIterator;


    typedef list<ALSAStreamOps*>::iterator ALSAStreamOpsListIterator;
    typedef list<ALSAStreamOps*>::const_iterator ALSAStreamOpsListConstIterator;

public:
    // PFW type value pairs type
    struct SSelectionCriterionTypeValuePair
    {
        int iNumerical;
        const char* pcLiteral;
    };

    CAudioRouteManager(AudioHardwareALSA* pParent);
    virtual           ~CAudioRouteManager();

    status_t setStreamParameters(android_audio_legacy::ALSAStreamOps* pStream, const String8 &keyValuePairsSET, int iMode);

    status_t startStream(bool bIsStreamOut);

    status_t stopStream(bool bIsStreamOut);

    status_t setParameters(const String8& keyValuePairs);
    String8 getParameters(const String8& keyValuePairs);

    // Add a new group to route manager
    void addPort(uint32_t uiPortIndex);

    // Add a new group to route manager
    void addPortGroup(uint32_t uiPortGroupIndex);

    // Add a stream to route manager
    void addStream(ALSAStreamOps* pStream);

    // Remove a stream from route manager
    void removeStream(ALSAStreamOps* pStream);

    // Start route manager service
    status_t start();

   /**
    * Apply a mute configuration to fasten the first routing.
    * So all default configurations are applied by the PFW at start-up.
    */
    void initRouting();

    bool isStarted() const;

    /**
     * Sets the voice volume.
     * Called from AudioSystem/Policy to apply the volume on the voice call stream which is
     * platform dependent.
     *
     * @param[in] gain the volume to set in float format in the expected range [0 .. 1.0]
     *                 Note that any attempt to set a value outside this range will return -ERANGE.
     *
     * @return OK if success, error code otherwise.
     */
    status_t setVoiceVolume(float gain);

    /**
     * Reset the Echo Reference.
     * The purpose of this function is
     * - to stop the processing (i.e. writing of playback frames as echo reference for
     * for AEC effect) in AudioSteamOutALSA
     * - reset locally stored echo reference
     * @param[in] reference: pointer to echo reference to reset
     */
    void resetEchoReference(struct echo_reference_itfe* reference);

    /**
     * Get an Echo Reference for AEC.
     * The purpose of this function is
     *     - create echo_reference_itfe using input stream and output stream parameters
     *     - add echo_reference_itfs to AudioSteamOutALSA which will use it for
     *         providing playback frames as echo reference for AEC effect
     *     - store locally the created reference
     *     - return created echo_reference_itfe to caller (i.e. AudioSteamInALSA)
     * Note: created echo_reference_itfe is used as backlink between playback which
     *         provides reference of output data and record which applies AEC effect
     * @param[in] format: input stream format
     * @param[in] channel_count: input stream channels count
     * @param[in] sampling_rate: input stream sampling rate
     *
     * @return NULL is creation of echo_reference_itfe failed overwise,
     *         pointer to created echo_reference_itfe
     */
    struct echo_reference_itfe* getEchoReference(int format,
                                                 uint32_t channel_count,
                                                 uint32_t sampling_rate);

    /**
     * Get the default pcm configuration.
     * Upon creation, streams need to provide latency and buffer size. As stream are not attached
     * to any route at creation, they must get a default pcm configuration dependant of the
     * platform to provide information of latency and buffersize (inferred from ALSA ring buffer).
     *
     * @param[in] bIsOut direction of the stream requesting the configuration
     * @param[in] uiFlags only valid for output stream, depends on flag, might use different
     *                    buffering model.
     *
     * @return reference of default pcm_config
     */
    const pcm_config& getDefaultPcmConfig(bool bIsOut, uint32_t uiFlags = 0) const;

    /**  Socket Id enumerator**/
    enum UeventSocketDesc {
        FdFromSstDriver,

        NbSocketDesc
    };

    /** Uevent socket fd */
    int uevent_fd;

private:
    CAudioRouteManager(const CAudioRouteManager &);
    CAudioRouteManager& operator = (const CAudioRouteManager &);

    void setDevices(ALSAStreamOps* pStream, uint32_t devices);

    /**
     * Update the active input source. Only one active input source at the time.
     *
     * @param[in] pStream input stream pointer.
     * @param[in] inputSource the stream input source mask.
     */
    void setInputSourceMask(AudioStreamInALSA* pStreamIn, uint32_t inputSource);

    void setOutputFlags(AudioStreamOutALSA* pStreamOut, uint32_t uiFlags);

    void reconsiderRouting(bool bIsSynchronous = true);

    /**
     * Open uevent socket and listen to it
     */
    void uevent_init();

    // Routing within worker thread context
    void doReconsiderRouting();

    // Virtually connect routes
    bool prepareRouting();
    bool prepareRouting(bool bIsOut);
    void prepareRoute(CAudioRoute* pRoute, bool bIsOut);

    // Route stage dispatcher
    void executeRouting();

    // Mute the routes
    void executeMuteStage();
    void muteRoutes(bool bIsOut);

    // Unmute the routes
    void executeUnmuteStage();

    // Configure the routes
    void executeConfigureStage();
    void configureRoutes(bool bIsOut);

    // Disable the routes
    void executeDisableStage();

    /**
     * Computes the PFW route criteria to disable the routes.
     * It appends to the closing route criterion the route that were opened before the routing
     * reconsideration and that will not be used anymore after. It also removes from opened routes
     * criterion these routes.
     *
     * @param[in] bIsOut direction of the routes to disable.
     */
    void prepareDisableRoutes(bool bIsOut);

    /**
     * Performs the disabling of the route.
     * It only concerns the action that needs to be done on routes themselves, ie detaching
     * streams, closing alsa devices.
     *
     * @param[in] isOut direction of the routes to disable.
     * @param[in] isPostDisable if set, it indicates that the disable happens after unrouting.
     */
    void doDisableRoutes(bool isOut, bool isPostDisable = false);

    /**
     * Performs the post-disabling of the route.
     * It only concerns the action that needs to be done on routes themselves, ie detaching
     * streams, closing alsa devices. Some platform requires to close stream before unrouting.
     * Behavior is encoded in the route itself.
     *
     * @tparam isOut direction of the routes to disable.
     */
    template <bool isOut>
    inline void doPostDisableRoutes()
    {

        doDisableRoutes(isOut, true);
    }

    // Enable the routes
    void executeEnableStage();

    /**
     * Performs the enabling of the routes.
     * It only concerns the action that needs to be done on routes themselves, ie attaching
     * streams, opening alsa devices.
     *
     * @param[in] iOut direction of the routes to disable.
     * @param[in] isPreEnable if set, it indicates that the enable happens before routing.
     */
    void doEnableRoutes(bool iOut, bool isPreEnable = false);

    /**
     * Performs the pre-enabling of the routes.
     * It only concerns the action that needs to be done on routes themselves, ie attaching
     * streams, opening alsa devices. Some platform requires to open stream before routing.
     * Behavior is encoded in the route itself.
     *
     * @tparam isOut direction of the routes to disable.
     */
    template <bool isOut>
    inline void doPreEnableRoutes()
    {

        doEnableRoutes(isOut, true);
    }

    // unsigned integer parameter value retrieval
    uint32_t getIntegerParameterValue(const string& strParameterPath, uint32_t uiDefaultValue) const;

    // unsigned integer parameter value set
    status_t setIntegerParameterValue(const string& strParameterPath, uint32_t uiValue);

    // unsigned integer parameter value setter
    status_t setIntegerArrayParameterValue(const string& strParameterPath, vector<uint32_t>& uiArray) const;

    /**
     * Get a handle on the platform dependent parameter.
     * It first reads the string value of the dynamic parameter which represents the path of the
     * platform dependent parameter.
     * Then it gets a handle on this platform dependent parameter.
     *
     * @param[in] strDynamicParameterPath path of the dynamic parameter (platform agnostic).
     *
     * @return CParameterHandle handle on the parameter, NULL pointer if error.
     */
    CParameterHandle* getDynamicParameterHandle(const string& strDynamicParamPath);

    /**
     * Get a handle on the platform dependent voice volume parameter.
     *
     * @return CParameterHandle handle on the voice volume parameter, NULL pointer if error.
     */
    CParameterHandle* getVoiceVolumeHandle();

    /**
     * Get a string value from a parameter path.
     * It returns the value stored in the parameter framework under the given path.
     *
     * @param[in] strParameterPath path of the parameter.
     * @param[out] strValue string value stored under this path.
     *
     * @return status_t OK if success, error code otherwise and strValue is kept unchanged.
     */
    status_t getStringParameterValue(const string& strParameterPath, string& strValue) const;

    // For a given streamroute, find an applicable in/out stream
    ALSAStreamOps* findApplicableStreamForRoute(bool bIsOut, const CAudioRoute* pRoute);

    // Retrieve route pointer from its name
    CAudioRoute* findRouteById(uint32_t uiRouteId);

    // Retrieve port pointer from its name
    CAudioPort* findPortById(uint32_t uiPortId);

    // Reset availability of all routes
    void resetAvailability();

    // Start the AT Manager
    void startModemAudioManager();

    bool isAecEffect(const effect_uuid_t *uuid);
    status_t getAudioEffectUuidFromHandle(effect_handle_t effect, effect_uuid_t* uuid);
    status_t doAddAudioEffect(AudioStreamInALSA* pStream, effect_handle_t effect);
    status_t doRemoveAudioEffect(AudioStreamInALSA *pStream, effect_handle_t effect);

    /// Inherited from IModemAudioManagerObserver
    virtual void onModemAudioStatusChanged();
    virtual void onModemStateChanged();
    virtual void onModemAudioBandChanged();

    // Inherited from IEventListener: Event processing
    virtual bool onEvent(int iFd);
    virtual bool onError(int iFd);
    virtual bool onHangup(int iFd);
    virtual void onAlarm();
    virtual void onPollError();
    virtual bool onProcess(uint16_t uiEvent);

    /**
     * Do parameters pop & set.
     * Pop all parameters provided in the key/value and set those which are handled.
     *
     * @param[in] keyValuePairs: the key/value pairs of parameters
     *
     * @return status_t
     */
    status_t doSetParameters(const String8& keyValuePairs);

    /**
     * Do Screen State specific parameters pop & set.
     * Pop all screen state parameters provided in the key/value and set those which are handled.
     * Screen key value pair will be removed if found.
     *
     * @param[in/out] param: reference of parameters helper object.
     */
    void doSetScreenStateParameters(AudioParameter &param);

    /**
     * Do TTY specific parameters pop & set.
     * Pop all TTY specific parameters provided in the key/value and set those which are handled.
     * TTY key value pair will be removed if found.
     *
     * @param[in/out] param: reference of parameters helper object.
     */
    void doSetTtyParameters(AudioParameter &param);

    /**
     * Do SCO request parameter pop & set.
     *
     * @param[in/out] param: reference of parameters helper object.
     */
    void doScoResourceRequestParameters(AudioParameter &param);

    /**
     * Do bypass non linear post processing parameter pop & set.
     *
     * @param[in/out] param: reference of parameters helper object.
     */
    void doBypassNonLinearPpParameters(AudioParameter &param);

    /**
     * Do bypass linear post processing parameter pop & set.
     *
     * @param[in/out] param: reference of parameters helper object.
     */
    void doBypassLinearPpParameters(AudioParameter &param);

    /**
     * Do HAC specific parameters pop & set.
     * Pop all HAC specific parameters provided in the key/value and set those which are handled.
     * HAC key value pair will be removed if found.
     *
     * @param[in/out] param: reference of parameters helper object.
     */
    void doSetHacParameters(AudioParameter &param);

    /**
     * This function sets the MicMute feature to state.
     *
     * @param[in] state if true, it indicates that the mic should be muted, and unmuted otherwise.
     */
    void setMicMute(bool state);

    /**
      * This functions returns the state of mic mute feature.
      *
      * @return true if the state of mic mute feature is true, false otherwise.
      */
    bool getMicMute();

    /**
     * Do Stream Flags specific parameters pop & set.
     * Pop all Stream Flags  specific parameters provided in the key/value
     * and set those which are handled.
     * Stream Flags key value pair will be removed if found.
     *
     * @param[in/out] param: reference of parameters helper object.
     */
    void doSetStreamFlagsParameters(AudioParameter &param);

    /**
     * Do FM specific parameters pop & set.
     * Pop all FM specific parameters provided in the key/value and set those which are handled.
     * FM key value pairs will be removed if found.
     *
     * @param[in/out] param: reference of parameters helper object.
     */
    void doSetFmParameters(AudioParameter &param);

    /**
     * Do BT specific parameters pop & set.
     * Pop all BT specific parameters provided in the key/value and set those which are handled.
     * BT key value pairs will be removed if found.
     *
     * @param[in/out] param: reference of parameters helper object.
     */
    void doSetBTParameters(AudioParameter &param);

    /**
     * Do Context Awareness specific parameters pop & set.
     * Pop all BT specific parameters provided in the key/value and set those which are handled.
     * Context Awareness key value pair will be removed if found.
     *
     * @param[in/out] param: reference of parameters helper object.
     */
    void doSetContextAwarenessParameters(AudioParameter &param);

    /**
     * Do "Always Listening"-specific parameters pop & set.
     * Pop all LPAL specific parameters provided in the key/value and set those which are handled.
     * Always Listening key value pair will be removed if found.
     *
     * @param[in,out] param: reference of parameters helper object.
     */
    void doSetAlwaysListeningParameters(AudioParameter &param);

    void createsRoutes();

    void createAudioHardwarePlatform();

    // Used to fill types for PFW
    ISelectionCriterionTypeInterface* createAndFillSelectionCriterionType(CriteriaType eCriteriaType) const;

    static const char* const gpcVoiceVolume;

    static const char* const LINE_IN_TO_HEADSET_LINE_VOLUME;
    static const char* const LINE_IN_TO_SPEAKER_LINE_VOLUME;
    static const char* const LINE_IN_TO_EAR_SPEAKER_LINE_VOLUME;

    static const char* const BLUETOOTH_HFP_SUPPORTED_PROP_NAME;
    static const bool BLUETOOTH_HFP_SUPPORTED_DEFAULT_VALUE;
    static const char* const PFW_CONF_FILE_NAME_PROP_NAME;
    static const char* const gPfwConfFileDefaultName;
    static const char* const ROUTING_LOCKED_PROP_NAME;

    static const char* const gapcLineInToHeadsetLineVolume;
    static const char* const gapcLineInToSpeakerLineVolume;
    static const char* const gapcLineInToEarSpeakerLineVolume;

    static const char* const MODEM_LIB_PROP_NAME;

    static const uint32_t VOIP_RATE_FOR_NARROW_BAND_PROCESSING;

    bool _bBluetoothHFPSupported;

    // The connector
    CParameterMgrPlatformConnector* _pParameterMgrPlatformConnector;
    // Logger
    CParameterMgrPlatformConnectorLogger* _pParameterMgrPlatformConnectorLogger;

    // Voice volume Parameter handle
    CParameterHandle* _pVoiceVolumeParamHandle;

    // Mode type
    static const SSelectionCriterionTypeValuePair MODE_VALUE_PAIRS[];
    // Band type
    static const SSelectionCriterionTypeValuePair BAND_TYPE_VALUE_PAIRS[];
    // TTY Mode
    static const SSelectionCriterionTypeValuePair TTY_DIRECTION_VALUE_PAIRS[];
    // Band Ringing
    static const SSelectionCriterionTypeValuePair BAND_RINGING_VALUE_PAIRS[];
    // Routing Mode
    static const SSelectionCriterionTypeValuePair ROUTING_STAGE_VALUE_PAIRS[];
    // Audio Source
    static const SSelectionCriterionTypeValuePair AUDIO_SOURCE_VALUE_PAIRS[];
    // Boolean
    static const SSelectionCriterionTypeValuePair BOOLEAN_VALUE_PAIRS[];
    // Route
    // Selected Input Device type
    static const SSelectionCriterionTypeValuePair INPUT_DEVICE_VALUE_PAIRS[];
    // Selected Output Device type
    static const SSelectionCriterionTypeValuePair OUTPUT_DEVICE_VALUE_PAIRS[];
    // Off/On
    static const SSelectionCriterionTypeValuePair OFF_ON_VALUE_PAIRS[];

    struct SSelectionCriterionTypeInterface
    {
        CriteriaType eCriteriaType;
        const SSelectionCriterionTypeValuePair* _pCriterionTypeValuePairs;
        uint32_t _uiNbValuePairs;
        bool _bIsInclusive;
    };

    static const SSelectionCriterionTypeInterface ARRAY_CRITERIA_TYPES[];

    ISelectionCriterionTypeInterface* _apCriteriaTypeInterface[ENbCriteriaTypes];

    // Criteria
    enum Criteria {
        ESelectedMode = 0,
        ESelectedFmMode,
        ESelectedTtyDirection,
        ESelectedRoutingStage,
        EClosingCaptureRoutes,
        EClosingPlaybackRoutes,
        EOpenedCaptureRoutes,
        EOpenedPlaybackRoutes,
        ESelectedInputDevice,
        ESelectedOutputDevice,
        ESelectedInputSource,
        ESelectedBandRinging,
        ESelectedBand,
        ESelectedBtHeadsetNrEc,
        ESelectedBtHeadsetBand,
        ESelectedHacMode,
        ESelectedScreenState,
        ESelectedBypassNonLinearPp,
        ESelectedBypassLinearPp,
        ESelectedMicMute,
        ENbCriteria
    };
    struct CriteriaInterface {
        const char* pcName;
        CriteriaType eCriteriaType;
    };
    static const CriteriaInterface ARRAY_CRITERIA_INTERFACE[ENbCriteria];

    ISelectionCriterionInterface* _apSelectedCriteria[ENbCriteria];

    inline ISelectionCriterionInterface* selectedClosingRoutes(bool bIsOut) {

        Criteria eCriteria = (bIsOut? EClosingPlaybackRoutes : EClosingCaptureRoutes);
        return _apSelectedCriteria[eCriteria];
    }

    inline ISelectionCriterionInterface* selectedOpenedRoutes(bool bIsOut) {

        Criteria eCriteria = (bIsOut ? EOpenedPlaybackRoutes : EOpenedCaptureRoutes);
        return _apSelectedCriteria[eCriteria];
    }

    inline ISelectionCriterionInterface* selectedDevice(bool bIsOut) {

        Criteria eCriteria = (bIsOut ? ESelectedOutputDevice : ESelectedInputDevice);
        return _apSelectedCriteria[eCriteria];
    }

    // Input/Output Streams list
    list<ALSAStreamOps*> _streamsList[CUtils::ENbDirections];

    // List of route
    list<CAudioRoute*> _routeList;

    // List of port
    list<CAudioPort*> _portList;

    // List of port group
    list<CAudioPortGroup*> _portGroupList;

    // Audio AT Manager interface
    IModemAudioManagerInterface* _pModemAudioManagerInterface;

    bool isModemLess() const { return _pModemAudioManagerInterface == NULL; }

    // Platform state pointer
    CAudioPlatformState* _pPlatformState;

    // Worker Thread
    CEventThread* _pEventThread;

    // Client wait semaphore list
    CSyncSemaphoreList _clientWaitSemaphoreList;

    // Started service flag
    bool _bIsStarted;

    /*
     * Routing Protection Required.
     * This allows to handle platform with strong locking strategy
     * (streams may switch to one route to another dynamically)
     */
    bool _bRoutingLocked;

    // Routing timeout
    static const uint32_t _uiTimeoutSec;

    // Routing lock protection
    RWLock _lock;

    struct {

        // Bitfield of route that needs reconfiguration, it includes route
        // that were enabled and need to be disabled
        uint32_t uiNeedReconfig;
        // Bitfield of enabled route
        uint32_t uiEnabled;
        // Bitfield of previously enabled route
        uint32_t uiPrevEnabled;
    } _stRoutes[CUtils::ENbDirections];

    /** Uevent message max length */
    static const int UEVENT_MSG_MAX_LEN;

    /** Socket buffer default size (64K) */
    static const int SOCKET_BUFFER_DEFAULT_SIZE;

protected:
    friend class AudioHardwareALSA;

    AudioHardwareALSA *     _pParent;

    // For backup and restore audio parameters
    CAudioParameterHandler* _pAudioParameterHandler;

    struct echo_reference_itfe* _pEchoReference;
};
};        // namespace android

