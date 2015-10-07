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

#ifndef ANDROID_I_FRAME_TYPE_CHANGE_LISTENER_H
#define ANDROID_I_FRAME_TYPE_CHANGE_LISTENER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <utils/List.h>
#include <ui/GraphicBuffer.h>
#include "IFrameListener.h"

/**
 * Enumeration for the type of frame being served.
 */
enum HWCFrameType {
    HWC_FRAMETYPE_NOTHING,           /// Not providing content, get frames from SurfaceFlinger
    HWC_FRAMETYPE_FRAME_BUFFER,      /// Frames from frame buffer
    HWC_FRAMETYPE_VIDEO,             /// Decoded output from video decoder
    HWC_FRAMETYPE_INCOMING_CALL,
};

/**
 * Information about the currently active mode.
 * Used as an argument to IFrameTypeChangeListener::modeChanged().
 */
struct FrameInfo {
    /**
     * Type of frame. If frameType is HWC_FRAMETYPE_VIDEO,
     * the rest of the fields carry additional info.
     */
    HWCFrameType frameType;
    uint32_t contentWidth;
    uint32_t contentHeight;
    uint32_t cropLeft;
    uint32_t cropTop;
    uint32_t bufferWidth;
    uint32_t bufferHeight;
    uint32_t bufferFormat;
    uint16_t lumaUStride;
    uint16_t chromaUStride;
    uint16_t chromaVStride;
    uint16_t contentFrameRateN;
    uint16_t contentFrameRateD;
    bool isProtected;
};

/**
 * An interface for receiving notifications from HWC
 * about changes to frame type. The external module needs
 * to implement this interface and register with HWC
 * through IFrameTypeChangeListener.
 */
class IFrameTypeChangeListener : public android::IInterface
{
public:
    DECLARE_META_INTERFACE(FrameTypeChangeListener);

    /**
     * Frame type has changed. This usually happens when user goes
     * in and out of video playback.
     *
     * @param frameInfo Info about the new frame type
     * @return Return NO_ERROR if successful;  TODO: Add other error codes.
     */
    virtual android::status_t frameTypeChanged(const FrameInfo& frameInfo) = 0;

    /**
     * Buffer info has changed.
     */
    virtual android::status_t bufferInfoChanged(const FrameInfo& frameInfo) = 0;

    virtual android::status_t shutdownVideo() = 0;
};

class BnFrameTypeChangeListener : public android::BnInterface<IFrameTypeChangeListener>
{
public:
    virtual android::status_t onTransact(   uint32_t code,
                                            const android::Parcel& data,
                                            android::Parcel* reply,
                                            uint32_t flags = 0);
};

#endif // ANDROID_I_FRAME_TYPE_CHANGE_LISTENER_H
