/*
 * Copyright (c) 2013, Intel Corporation. All rights reserved.
 *
 * Redistribution.
 * Redistribution and use in binary form, without modification, are
 * permitted provided that the following conditions are met:
 *  * Redistributions must reproduce the above copyright notice and
 * the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *  * Neither the name of Intel Corporation nor the names of its
 * suppliers may be used to endorse or promote products derived from
 * this software without specific  prior written permission.
 *  * No reverse engineering, decompilation, or disassembly of this
 * software is permitted.
 *
 * Limited patent license.
 * Intel Corporation grants a world-wide, royalty-free, non-exclusive
 * license under patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this
 * software, but solely to the extent that any such patent is necessary
 * to Utilize the software alone, or in combination with an operating
 * system licensed under an approved Open Source license as listed by
 * the Open Source Initiative at http://opensource.org/licenses.
 * The patent license shall not apply to any other combinations which
 * include this software. No hardware per se is licensed hereunder.
 *
 * DISCLAIMER.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef ANDROID_I_FRAME_LISTENER_H
#define ANDROID_I_FRAME_LISTENER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

enum HWCBufferHandleType {
    HWC_HANDLE_TYPE_GRALLOC,
    HWC_HANDLE_TYPE_KBUF,
};

/**
 * An interface for obtaining video frames from HWC.
 * The external module needs to implement this interface
 * and register with HWC through IFrameListener.
 */
class IFrameListener : public android::IInterface
{
public:
    DECLARE_META_INTERFACE(FrameListener);

    /**
     * A new buffer is being processed.
     *
     * HWC shall call this method when it receives a buffer but has not
     * yet completed processing (such as scaling or color conversion).
     * This signal tells the consumer that a new frame will be available
     * for use shortly.
     *
     * @param renderTimestamp Timestamp of when HWC received the buffer.
     * @param mediaTimestamp Timestamp provided by the decoder if this buffer
     *                       is associated with video playback; -1 if not related
     *                       to playback.
     * @return Return NO_ERROR if successful;  TODO: Add other error codes.
     */
    virtual android::status_t onFramePrepare(int64_t renderTimestamp, int64_t mediaTimestamp) = 0;

    /**
     * A new buffer is available for use.
     *
     * HWC shall call this method when it has completed processing
     * (such as scaling or color conversion) and the buffer can be
     * consumed. This signal shall follow a previous onFramePrepare
     * signal and shall contain the same renderTimestamp and mediaTimestamp
     * provided by the last onFramePrepare().
     *
     * @param handle Handle of the available buffer.
     * @param handleType Type of handle
     * @param renderTimestamp Timestamp of when HWC received the buffer.
     * @param mediaTimestamp Timestamp provided by the decoder if this buffer
     *                       is associated with video playback; -1 if not related
     *                       to playback.
     * @return Return NO_ERROR if successful;  TODO: Add other error codes.
     */
    virtual android::status_t onFrameReady(int32_t handle, HWCBufferHandleType handleType,
                                           int64_t renderTimestamp, int64_t mediaTimestamp) = 0;
    /**
     * A new buffer is available for use.
     *
     * HWC shall call this method when it has received a buffer
     * The aquireFenceFd needs to be waited upon before the buffer can be accessed.
     *
     * @param handle Handle of the available buffer.
     * @param renderTimestamp Timestamp of when HWC received the buffer.
     * @param mediaTimestamp Timestamp provided by the decoder if this buffer
     *                       is associated with video playback; -1 if not related
     *                       to playback.
     * @param acquireFenceFd FenceFd that needs to be waited on before the listener accesses the buffer
     * @param releaseFenceFd Returned FenceFd that needs to be waited on before the server can reuse the buffer
     * @return Return NO_ERROR if successful;  TODO: Add other error codes.
     */
    virtual android::status_t onFrameReady(const native_handle* handle,
                                           int64_t renderTimestamp, int64_t mediaTimestamp,
                                           int acquireFenceFd, int* releaseFenceFd) = 0;
};

class BnFrameListener : public android::BnInterface<IFrameListener>
{
public:
    virtual android::status_t onTransact(   uint32_t code,
                                            const android::Parcel& data,
                                            android::Parcel* reply,
                                            uint32_t flags = 0);
};

#endif // ANDROID_I_FRAME_LISTENER_H
