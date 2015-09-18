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

#include <tinyalsa/asoundlib.h>
#include <hardware_legacy/AudioHardwareBase.h>
#include "AudioPlatformState.h"
#include "AudioStreamRoute.h"
#include "AudioCompressedStreamRoute.h"
#include "AudioExternalRoute.h"
#include "AudioRoute.h"
#include "AudioRouteManager.h"
#include "Tokenizer.h"

namespace android_audio_legacy
{

#define DEVICE_OUT_MM_ALL (AudioSystem::DEVICE_OUT_EARPIECE | \
                AudioSystem::DEVICE_OUT_SPEAKER | \
                AudioSystem::DEVICE_OUT_WIRED_HEADSET | \
                AudioSystem::DEVICE_OUT_WIRED_HEADPHONE)


#define DEVICE_IN_BUILTIN_ALL (AudioSystem::DEVICE_IN_WIRED_HEADSET | \
                AudioSystem::DEVICE_IN_BACK_MIC | \
                AudioSystem::DEVICE_IN_AUX_DIGITAL | \
                AudioSystem::DEVICE_IN_BUILTIN_MIC)

#define DEVICE_OUT_BLUETOOTH_SCO_ALL (AudioSystem::DEVICE_OUT_BLUETOOTH_SCO | \
                AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET | \
                AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT)

#define DEVICE_IN_BLUETOOTH_SCO_ALL (AudioSystem::DEVICE_IN_BLUETOOTH_SCO_HEADSET)

#define DEVICE_BLUETOOTH_SCO_ALL(bIsOut) (bIsOut ? DEVICE_OUT_BLUETOOTH_SCO_ALL : \
                                                   DEVICE_IN_BLUETOOTH_SCO_ALL)

#define NOT_APPLICABLE  (0)

#define channel_policy_not_applicable {}

//extern "C" static const pcm_config pcm_config_not_applicable;

static const pcm_config pcm_config_not_applicable = {
   channels             : 0,
   rate                 : 0,
   period_size          : 0,
   period_count         : 0,
   format               : PCM_FORMAT_S16_LE,
   start_threshold      : 0 ,
   stop_threshold       : 0 ,
   silence_threshold    : 0,
   avail_min            : 0,
};

/**
 * Returns the name of the audio codec. It can be used when the same build shall support multiple
 * codecs. The coded name is extracted from the file /proc/asound/cards.
 *
 * @return A pointer to char containing the name of the audio codec. If the file /proc/asound/cards
 * cannot be parsed, the default codec name is "unknown-audio-codec".
 */
const char* getCodecName();

class CAudioPlatformHardware {

public:
    static const CAudioRouteManager::SSelectionCriterionTypeValuePair _stRouteValuePairs[];
    static const uint32_t _uiNbRouteValuePairs;

    static CAudioRoute* createAudioRoute(uint32_t uiRouteIndex, CAudioPlatformState* pPlatformState);

    static uint32_t getNbPorts() { return _uiNbPorts; }

    static uint32_t getNbPortGroups() { return _uiNbPortGroups; }

    static uint32_t getNbRoutes() { return _uiNbRoutes; }

    //
    // Port helpers
    //
    static const char* getPortName(int uiPortIndex) {
        return _acPorts[uiPortIndex];
    }

    static uint32_t getPortId(uint32_t uiPortIndex) { return (uint32_t)1 << uiPortIndex; }

    //
    // Port group helpers
    //
    static uint32_t getPortsUsedByPortGroup(uint32_t uiPortGroupIndex) {

        std::string srtPorts(_acPortGroups[uiPortGroupIndex]);
        uint32_t uiPorts = 0;
        Tokenizer tokenizer(srtPorts, ",");
        std::vector<std::string> astrItems = tokenizer.split();

        uint32_t i;
        for (i = 0; i < astrItems.size(); i++) {

            uiPorts |= getPortIdByName(astrItems[i]);
        }
        LOGD("%s Ports name=%s", __FUNCTION__, srtPorts.c_str());
        return uiPorts;
    }

    //
    // Route helpers
    //
    static std::string getRouteName(int iRouteIndex) {
        return _astAudioRoutes[iRouteIndex].pcRouteName;
    }
    static uint32_t getRouteId(int iRouteIndex) {
        return (uint32_t)1 << iRouteIndex;
    }
    static uint32_t getRouteType(int iRouteIndex) {
        return _astAudioRoutes[iRouteIndex].uiRouteType;
    }
    static uint32_t getPortsUsedByRoute(int iRouteIndex) {

        std::string srtPorts(_astAudioRoutes[iRouteIndex].pcPortsUsed);
        uint32_t uiPorts = 0;
        Tokenizer tokenizer(srtPorts, ",");
        std::vector<std::string> astrItems = tokenizer.split();

        uint32_t i;
        for (i = 0; i < astrItems.size(); i++) {

            uiPorts |= getPortIdByName(astrItems[i].c_str());
        }
        LOGD("%s Ports name=%s", __FUNCTION__, srtPorts.c_str());
        return uiPorts;
    }
    static uint32_t getRouteApplicableDevices(int iRouteIndex, bool bIsOut) {
        return _astAudioRoutes[iRouteIndex].auiApplicableDevices[bIsOut];
    }
    static uint32_t getRouteApplicableMask(int iRouteIndex, bool bIsOut) {
        return _astAudioRoutes[iRouteIndex].uiApplicableMask[bIsOut];
    }
    static uint32_t getRouteApplicableModes(int iRouteIndex, bool bIsOut) {
        return _astAudioRoutes[iRouteIndex].uiApplicableModes[bIsOut];
    }
    static const char* getRouteCardName(int iRouteIndex) {
        return _astAudioRoutes[iRouteIndex].pcCardName;
    }
    static int32_t getRouteDeviceId(int iRouteIndex, bool bIsOut) {
        return _astAudioRoutes[iRouteIndex].aiDeviceId[bIsOut];
    }
    static const pcm_config& getRoutePcmConfig(int iRouteIndex, bool bIsOut) {
        return _astAudioRoutes[iRouteIndex].astPcmConfig[bIsOut];
    }
    static uint32_t getSlaveRoutes(int iRouteIndex) {

        std::string srtSlaveRoutes(_astAudioRoutes[iRouteIndex].pcSlaveRoutes);
        uint32_t uiSlaves = 0;
        Tokenizer tokenizer(srtSlaveRoutes, ",");
        std::vector<std::string> astrItems = tokenizer.split();

        uint32_t i;
        for (i = 0; i < astrItems.size(); i++) {

            uiSlaves |= getRouteIdByName(astrItems[i]);
        }
        LOGD("%s acSlaveRoutes=%s", __FUNCTION__, srtSlaveRoutes.c_str());
        return uiSlaves;
    }

    static uint32_t getRouteIdByName(const std::string& strRouteName) {

        uint32_t index;
        for (index = 0; index < getNbRoutes(); index++) {

            std::string strRouteAtIndexName(_astAudioRoutes[index].pcRouteName);
            if (strRouteName == strRouteAtIndexName) {

                return 1 << index;
            }
        }
        return 0;
    }

    static uint32_t getPortIdByName(const std::string& strPortName) {

        uint32_t index;
        for (index = 0; index < getNbPorts(); index++) {

            std::string strPortAtIndexName(_acPorts[index]);
            if (strPortName == strPortAtIndexName) {

                return 1 << index;
            }
        }
        return 0;
    }

    // Property name indicating time to write silence before first write
    static const char* CODEC_DELAY_PROP_NAME;

    static const vector<SampleSpec::ChannelsPolicy> getChannelsPolicy(int iRouteIndex,
                                                                      bool bIsOut) {

        std::vector<SampleSpec::ChannelsPolicy> channelsPolicyVector(
                    _astAudioRoutes[iRouteIndex].aChannelsPolicy[bIsOut],
                    _astAudioRoutes[iRouteIndex].aChannelsPolicy[bIsOut] +
                            getRoutePcmConfig(iRouteIndex, bIsOut).channels);

        return channelsPolicyVector;
    }

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
    static const pcm_config& getDefaultPcmConfig(bool bIsOut, uint32_t uiFlags)
    {
        bool bIsDeepFlag = uiFlags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER;
        return bIsOut ?
                    (bIsDeepFlag ?
                         pcm_config_deep_media_playback : pcm_config_media_playback) :
                    pcm_config_media_capture;
    }

private:
    static const uint32_t MAX_CHANNELS = 32;

    struct s_route_t {

        /**< Name of the route */
        const string pcRouteName;
        /**< Type of the route, e.g. EExternalRoute */
        CAudioRoute::RouteType uiRouteType;
        /**< I2S or codec port, e.g. IA_I2S1_PORT */
        const char* pcPortsUsed;
        /**< Bit field list - applicable devices per direction:  0 : EInput, 1 : EOutput */
        uint32_t auiApplicableDevices[CUtils::ENbDirections];
        /**< Bit field list - input sources (e.g. AUDIO_SOURCE_MIC) and
           output flags (e.g. AUDIO_OUTPUT_FLAG_PRIMARY) */
        uint32_t uiApplicableMask[CUtils::ENbDirections];
        /**< Bit field list of applicable modes per direction: 0 : EInput, 1 : EOutput */
        uint32_t uiApplicableModes[CUtils::ENbDirections];
        /**< Audio card name used by the route */
        const char* pcCardName;
        /**< Device ID used by the route */
        int32_t aiDeviceId[CUtils::ENbDirections];
        /**< Structure of the capture or uplink config and playback
           or downlink config per direction: 0 : EInput, 1 : EOutput */
        pcm_config astPcmConfig[CUtils::ENbDirections];
        /**< Channels policy used in case of remap operation , e.g. average */
        SampleSpec::ChannelsPolicy aChannelsPolicy[CUtils::ENbDirections][MAX_CHANNELS];
        /**< Literal coma list separated of slave routes */
        const char* pcSlaveRoutes;
    };

    static const uint32_t _uiNbPorts;

    static const uint32_t _uiNbPortGroups;

    static const uint32_t _uiNbRoutes;

    static const char* const _acPorts[];

    static const char* const _acPortGroups[];

    static const s_route_t _astAudioRoutes[];

    static const pcm_config pcm_config_media_playback;

    static const pcm_config pcm_config_media_capture;

    static const pcm_config pcm_config_deep_media_playback;
};
};        // namespace android
