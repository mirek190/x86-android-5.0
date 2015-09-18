/* AudioHardwareALSA.h
 **
 ** Copyright 2008-2009, Wind River Systems
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#ifndef ANDROID_AUDIO_HARDWARE_ALSA_H
#define ANDROID_AUDIO_HARDWARE_ALSA_H

#include <utils/List.h>
#include <list>
#include <string>
#include <vector>
#include <hardware_legacy/AudioHardwareBase.h>

#include <tinyalsa/asoundlib.h>

#include <hardware/hardware.h>

#include <utils/threads.h>
#include "AudioUtils.h"
#include "SampleSpec.h"
#include "Utils.h"

#include <hardware/audio_effect.h>
#include <audio_utils/echo_reference.h>
#include <audio_effects/effect_aec.h>

class CParameterMgrPlatformConnector;
class ISelectionCriterionTypeInterface;
class ISelectionCriterionInterface;

struct echo_reference_itfe;

namespace android_audio_legacy
{

class CParameterMgrPlatformConnectorLogger;
class AudioHardwareALSA;
class CAudioRouteManager;
class CAudioRoute;
class CAudioStreamRoute;
class AudioResampler;
class AudioConverter;
class AudioConversion;
class AudioStreamOutALSA;
class AudioStreamInALSA;
class ALSAStreamOps;

const uint32_t DEVICE_OUT_BLUETOOTH_SCO_ALL = AudioSystem::DEVICE_OUT_BLUETOOTH_SCO | \
        AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET | \
        AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT;


class AudioHardwareALSA : public AudioHardwareBase
{
public:
    AudioHardwareALSA();
    virtual            ~AudioHardwareALSA();

    /**
     * check to see if the audio hardware interface has been initialized.
     * return status based on values defined in include/utils/Errors.h
     */
    virtual android::status_t    initCheck();

    /** set the audio volume of a voice call. Range is between 0.0 and 1.0 */
    virtual android::status_t    setVoiceVolume(float volume);

    /**
     * set the audio volume for all audio activities other than voice call.
     * Range between 0.0 and 1.0. If any value other than NO_ERROR is returned,
     * the software mixer will emulate this capability.
     */
    virtual android::status_t    setMasterVolume(float volume);

    // mic mute
    virtual android::status_t    setMicMute(bool state);
    virtual android::status_t    getMicMute(bool* state);

    // set/get global audio parameters
    virtual android::status_t    setParameters(const String8& keyValuePairs);
    virtual String8              getParameters(const String8& keys);

    // set Stream Parameters
    virtual android::status_t    setStreamParameters(ALSAStreamOps* pStream,
                                                     const String8 &keyValuePairs);


    // Returns audio input buffer size according to parameters passed or 0 if one of the
    // parameters is not supported
    virtual size_t    getInputBufferSize(uint32_t sampleRate, int format, int channels);

    /** This method creates and opens the audio hardware output stream */
    virtual AudioStreamOut* openOutputStream(
            uint32_t devices,
            int* format = NULL,
            uint32_t* channels = NULL,
            uint32_t* sampleRate = NULL,
            status_t* status = NULL);
    virtual    void        closeOutputStream(AudioStreamOut* out);

    /** This method creates and opens the audio hardware input stream */
    virtual AudioStreamIn* openInputStream(
            uint32_t devices,
            int* format,
            uint32_t* channels,
            uint32_t* sampleRate,
            status_t* status,
            AudioSystem::audio_in_acoustics acoustics);
    virtual    void        closeInputStream(AudioStreamIn* in);

    /**This method dumps the state of the audio hardware */
    //virtual status_t dumpState(int fd, const Vector<String16>& args);

    static AudioHardwareInterface* create();

    int                 mode()
    {
        return mMode;
    }

    // Reconsider the routing
    android::status_t startStream(ALSAStreamOps *stream);

    android::status_t stopStream(ALSAStreamOps *stream);

protected:
    virtual status_t    dump(int fd, const Vector<String16>& args);

    /*
     * Reset the echo reference.
     * the purpose of this function is
     * - to stop the processing (i.e. writing of playback frames as echo reference for
     * for AEC effect) in AudioSteamOutALSA
     * - reset locally stored echo reference
     *
     * @param[in|out] reference: pointer to echo reference to reset
     *
     * @return none
     */
    void resetEchoReference(struct echo_reference_itfe* reference);

    /*
     * Echo reference provider.
     * the purpose of this function is
     * - create echo_reference_itfe using input stream and output stream parameters
     * - add echo_reference_itfs to AudioSteamOutALSA which will use it for providing
     *         playback frames as echo reference for AEC effect
     * - store locally the created reference
     * - return created echo_reference_itfe to caller (i.e. AudioSteamInALSA)
     * Note: created echo_reference_itfe is used as backlink between playback which
     *         provides reference of output data and record which applies AEC effect
     *
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

    struct echo_reference_itfe* mEchoReference;

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

    friend class AudioStreamOutALSA;
    friend class AudioStreamInALSA;
    friend class ALSAStreamOps;
    friend class CAudioRouteManager;
    friend class AudioConverter;

private:
    AudioHardwareALSA(const AudioHardwareALSA &);
    AudioHardwareALSA& operator = (const AudioHardwareALSA &);

    struct hw_module {
        const char* module_id;
        const char* module_name;
    };

    static const uint32_t DEFAULT_SAMPLE_RATE;
    static const uint32_t DEFAULT_CHANNEL_COUNT;
    static const uint32_t DEFAULT_FORMAT;

    static const char* const DEFAULT_GAIN_PROP_NAME;
    static const float DEFAULT_GAIN_VALUE;
    static const char* const AUDIENCE_IS_PRESENT_PROP_NAME;
    static const bool AUDIENCE_IS_PRESENT_DEFAULT_VALUE;

private:
    CAudioRouteManager* mRouteMgr;
};

};        // namespace android
#endif    // ANDROID_AUDIO_HARDWARE_ALSA_H
