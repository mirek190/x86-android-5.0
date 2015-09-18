/* ALSAStreamOps.h
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

#pragma once

#include <media/AudioBufferProvider.h>
#include <SampleSpec.h>
#include <utils/String8.h>
#include "Utils.h"

/**
 * For debug purposes only, property-driven (dynamic)
 */
#include "HALAudioDump.h"

typedef android::RWLock::AutoRLock AutoR;
typedef android::RWLock::AutoWLock AutoW;

namespace android_audio_legacy
{

class AudioHardwareALSA;
class CAudioStreamRoute;
class AudioConversion;
struct acoustic_device_t;
struct alsa_handle_t;

class ALSAStreamOps
{
public:
    virtual            ~ALSAStreamOps();

    android::status_t   set(int* format, uint32_t* channels, uint32_t* rate);

    android::status_t   setParameters(const android::String8& keyValuePairs);
    android::String8    getParameters(const android::String8& keys);

    inline uint32_t     sampleRate() const { return mSampleSpec.getSampleRate(); }

    /**
     * Get the buffer size to help upper layer to calibrate the transfer with Audio HAL.
     * Number of bytes returned takes stream sample rate into account
     * Ensure also the buffer is a multiple of 16 frames (AudioFlinger requirement).
     *
     * @param[in] flags: outputFlags for output stream, inputSource for input stream.
     *
     * @return size of a period in bytes.
     */
    size_t              getBufferSize(uint32_t flags = 0) const;
    inline int          format() const { return mSampleSpec.getFormat(); }
    inline uint32_t     channelCount() const { return mSampleSpec.getChannelCount(); }
    inline uint32_t     channels() const { return mSampleSpec.getChannelMask(); }

    /**
     * Checks if a stream is fully routed or not.
     * Note that a stream is considered as routed when
     *          -not only when the audio device is opened
     *          -but also all the controls are applyed on the audio path.
     *
     * @return true if routed, audio path is ready for stream operations, false otherwise.
     */
    bool                isRouteAvailable() const;

    /**
     * Called from route manager during routing of a stream. It attaches the stream to the new
     * route, sets the audio device.
     *
     * @return OK is success, error code otherwise.
     */
    android::status_t attachRoute();

    /**
     * Called from route manager during unrouting of a stream. It detaches the stream to the new
     * route, resets the audio device.
     *
     * @return OK is success, error code otherwise.
     */
    android::status_t detachRoute();

    android::status_t   setStandby(bool bIsSet);

    virtual bool        isOut() const = 0;

    /**
     * Set the route pointer to the new route.
     * No need to lock, newRoute is for exclusive access for route manager, from atomic context.
     *
     * @param[in] attachRoute Route to attach the stream with.
     */
    void  setNewRoute(CAudioStreamRoute *attachRoute);

    /**
     * Check if the stream has a new route assigned.
     * A stream might be eligible for a stream route. If so, the new route pointer will be set.
     * This function will inform if the stream has been assigned or not to a route.
     *
     * @return true if the stream has already a route assigned to it.
     */
    inline bool         isRouteAssignedToStream() const { return mNewRoute != NULL; }
    void                resetRoute();

    uint32_t            getNewDevices() const
    {
        return mNewDevices;
    }
    void                setNewDevices(uint32_t uiNewDevices);


    /**
     * Get the current output device(s).
     * No need to lock, mCurrentDevices is for exclusive access for route manager,
     * from atomic context.
     * @return output device(s) currently active.
     */
    uint32_t            getCurrentDevices() const
    {
        return mCurrentDevices;
    }

    /**
     * Set the current output device(s).
     * No need to lock, uiCurrentDevices is for exclusive access for route manager,
     * from atomic context.
     *
     * @param[in] uiCurrentDevices  set the current device
     */
    void                setCurrentDevices(uint32_t uiCurrentDevices);

    /**
     * Get the route attached to the stream.
     * Called from locked context.
     *
     * @return stream route attached to the stream, may be NULL if not routed.
     */
    CAudioStreamRoute *getCurrentRouteL() const { return mCurrentRoute; }

    /**
     * Get the current stream state
     *
     * @return boolean indicating the stream state (true=playing, false=standby|stopped)
     */
    bool                isStarted();

    /**
     * Set the current stream state
     *
     * @param[in] isStarted boolean used to set the stream state
     *            (true=playing, false=standby|stopped)
     */
    void                setStarted(bool isStarted);

    /** Applicability mask.
     * It depends on the direction of the stream.
     * @return applicability Mask
     */
    virtual uint32_t    getApplicabilityMask() const = 0;

    /**
     * Get audio dump object before conversion for debug purposes
     *
     * @return a CHALAudioDump object before conversion
     */
    CHALAudioDump *getDumpObjectBeforeConv() const;


    /**
     * Get audio dump objects after conversion for debug purposes
     *
     * @return a CHALAudioDump object after conversion
     */
    CHALAudioDump *getDumpObjectAfterConv() const;

protected:
    ALSAStreamOps(AudioHardwareALSA* parent, const char* pcLockTag);
    friend class AudioHardwareALSA;
    ALSAStreamOps(const ALSAStreamOps &);
    ALSAStreamOps& operator = (const ALSAStreamOps &);

    /**
     * Attaches the stream to the new route, sets the audio device.
     * Must be called with stream lock held.
     *
     * @return OK is success, error code otherwise.
     */
    virtual android::status_t attachRouteL();

    /**
     * Detaches the stream to the new route, resets the audio device.
     * Must be called with stream lock held.
     *
     * @return OK is success, error code otherwise.
     */
    virtual android::status_t detachRouteL();

    android::status_t applyAudioConversion(const void* src, void** dst, uint32_t inFrames, uint32_t* outFrames);
    android::status_t getConvertedBuffer(void* dst, const uint32_t outFrames, android::AudioBufferProvider* pBufferProvider);

    uint32_t            latency() const;
    void                updateLatency(uint32_t uiFlags = 0);

    /**
     * Checks if a stream is fully routed or not.
     * Note that a stream is considered as routed when
     *          -not only when the audio device is opened
     *          -but also all the controls are applyed on the audio path.
     * Must be called with stream lock held.
     *
     * @return true if routed, audio path is ready for stream operations, false otherwise.
     */
    bool isRouteAvailableL() const;

    /**
     * Init audio dump if dump properties are activated to create the dump object(s).
     * Triggered when the stream is started.
     */
    void                initAudioDump();
    /* Used to sleep on the current thread.
     *
     * This function is used to get a POSIX-compliant way
     * to accurately sleep the current thread.
     *
     * If function is successful, zero is returned
     * and request has been honored, if function fails,
     * EINTR has been raised by the system and -1 is returned.
     *
     * The other two errors considered by standard
     * are not applicable in our context (EINVAL, ENOSYS)
     *
     * @param[in] time desired to sleep, in microseconds.
     *
     * @return on success true is returned, false otherwise.
     */
    bool                safeSleep(uint32_t uiSleepTimeUs);

    /**
     * Prints debug information from LPE debug files
     *
     */
    void printLPEfwDebugInfo();


    AudioHardwareALSA*      mParent;
    pcm*                    mHandle;

    bool                    mStandby;
    uint32_t                mDevices;
    SampleSpec             mSampleSpec;
    SampleSpec             mHwSampleSpec;

    /**
     * Audio dump object used if one of the dump property before
     * conversion is true (check init.rc file)
     */
    CHALAudioDump         *dumpBeforeConv;

    /**
     * Audio dump object used if one of the dump property after
     * conversion is true (check init.rc file)
     */
    CHALAudioDump         *dumpAfterConv;
    /**
     * maximum number of read/write retries.
     *
     * This constant is used to set maximum number of retries to do
     * on write/read operations before stating that error is not
     * recoverable and reset media server.
     */
    static const uint32_t MAX_READ_WRITE_RETRIES = 50;

    /** Ratio between microseconds and milliseconds */
    static const uint32_t USEC_PER_MSEC = 1000;

    /** Ratio between nanoseconds and microseconds */
    static const uint32_t NSEC_PER_USEC = 1000;

    /**
     * Lock to protect not only the access to pcm device but also any access to device dependant
     * parameters as sample specification.
     * Sensitive data are the:
     *  -data read / set by both Route Manager and Stream (mCurrentRoute,
     * mStandby, mHandle, mDevices, mHwSampleSpec,
     * applicability mask (inputStreamMask for input stream, outputFlags for output stream,
     * effects list for input streams only).
     *
     * Using in both Read or Write mode is quite interesting, but required to use mutable attribute.
     */
    mutable android::RWLock _streamLock;

private:
    // Configure the audio conversion chain
    android::status_t configureAudioConversion(const SampleSpec &ssSrc, const SampleSpec &ssDst);

    int         headsetPmDownDelay;
    int         speakerPmDownDelay;
    int         voicePmDownDelay;

    bool        mIsReset;
    CAudioStreamRoute*       mCurrentRoute;
    CAudioStreamRoute*       mNewRoute;

    uint32_t                mCurrentDevices;
    uint32_t                mNewDevices;

    uint32_t                mLatencyUs;

    bool                    mPowerLock;
    const char*             mPowerLockTag;

    // Audio Conversion utility class
    AudioConversion *mAudioConversion;

    static const uint32_t STR_FORMAT_LENGTH;

    static const uint32_t MAX_DEBUG_STREAM_SIZE;

    /**
     * Array of property names before conversion
     */
    static const std::string dumpBeforeConvProps[CUtils::ENbDirections];


    /**
     * Array of property names after conversion
     */
    static const std::string dumpAfterConvProps[CUtils::ENbDirections];
    /** maximum sleep time to be allowed by HAL, in microseconds. */
    static const uint32_t MAX_SLEEP_TIME = 1000000UL;
};

};        // namespace android
