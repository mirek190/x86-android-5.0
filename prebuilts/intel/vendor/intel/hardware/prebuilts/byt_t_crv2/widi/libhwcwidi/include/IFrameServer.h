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

#ifndef ANDROID_I_FRAME_SERVER_H
#define ANDROID_I_FRAME_SERVER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include "IFrameTypeChangeListener.h"

/**
 * Policy to be used by HWC to process frames.
 * It specifies attributes such as the width
 * and height to scale the frames.
 */
struct FrameProcessingPolicy {
    uint32_t scaledWidth;       // Width chosen by the Widi subsystem to display on the screen
    uint32_t scaledHeight;      // Height chosen by the Widi subsystem to display on the screen
    uint32_t xdpi;              // dpi of the screen
    uint32_t ydpi;
    uint32_t refresh;           // Refresh rate of the screen per second
};

/**
 * An interface for serving video frames to an external module.
 * The external module needs to register an IFrameListener to
 * be notified when a new frame is available.
 */
class IFrameServer : public android::IInterface
{
public:
    DECLARE_META_INTERFACE(FrameServer);

    /**
     * Start serving frames.
     *
     * @param typeChangeListener An object of IFrameTypeChangeListener interface
     *                       which will be used by HWC to notify the external module.
     * @return NO_ERROR if successful. TODO: Add other error codes.
     */
    virtual android::status_t start(android::sp<IFrameTypeChangeListener> typeChangeListener, bool disableExtVideoMode) { return 0;}
    virtual android::status_t start(android::sp<IFrameTypeChangeListener> typeChangeListener){ return 0;}
    /**
     * Stop serving frames.
     *
     * @param isConnected TODO: Check if this is really needed.
     * @return NO_ERROR if successful TODO: Add other error codes.
     */
    virtual android::status_t stop(bool isConnected) = 0;
    /**
     * New policy is available for frame processing.
     *
     * HWC shall destroy its old buffers, allocate new buffers based
     * on the policy argument and start using the new buffers when this
     * call returns.
     *
     * @param policy New frame processing policy.
     * @param frameListener An instance of IFrameListener that will be
     *                      used to receive new frames.
     * @return NO_ERROR if successful TODO: Add other error codes.
     */
    virtual android::status_t setResolution(const FrameProcessingPolicy& policy, android::sp<IFrameListener> frameListener) = 0;

#ifndef INTEL_WIDI_BAYTRAIL
    /**
     * Signal HWC that a buffer has returned and can be reused.
     *
     * @param handle Handle of the buffer
     * @return NO_ERROR if successful TODO: Add other error codes.
     */
    virtual android::status_t notifyBufferReturned(int handle) = 0;
#endif
};

class BnFrameServer : public android::BnInterface<IFrameServer>
{
public:
    virtual android::status_t onTransact(   uint32_t code,
                                            const android::Parcel& data,
                                            android::Parcel* reply,
                                            uint32_t flags = 0);
};

#endif // ANDROID_I_FRAME_SERVER_H
