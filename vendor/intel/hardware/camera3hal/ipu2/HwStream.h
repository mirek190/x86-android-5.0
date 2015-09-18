/*
 * Copyright (C) 2013 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CAMERA3_HAL_HW_STREAM_H_
#define CAMERA3_HAL_HW_STREAM_H_

#include "utils/KeyedVector.h"

#include "v4l2dev/v4l2device.h"

#include "CameraStream.h"
#include "CameraStreamNode.h"
#include "Camera3Request.h"
#include "CameraBuffer.h"
#include "ipu2/IPU2HwIsp.h"
#include "Camera3GFXFormat.h"


namespace android {
namespace camera2 {

class CameraBuffer;
/**
 * \class HwStream
 *
 * Base class for the concrete data streams that the HW can provide.
 *
 */
class HwStream : virtual public RefBase, public CameraStreamNode {
public:
    virtual status_t init() = 0;

// CameraStreamNode override
    virtual status_t capture(sp<CameraBuffer> aBuffer,
                             Camera3Request* request);
    virtual status_t captureDone(sp<CameraBuffer> aBuffer,
                                 Camera3Request* request);
    virtual status_t query(FrameInfo * info);

    virtual status_t setFormat(FrameInfo *aConfig);
    virtual status_t reprocess(sp<CameraBuffer> anInputBuffer,
                               Camera3Request* request);
    virtual status_t attachListener(ICssIspListener *aListener, ICssIspListener::IspEventType event);
    virtual status_t detachListener();
    virtual void dump(bool dumpBuffers = false) const;

    virtual status_t start();
    virtual status_t stop();
    virtual status_t flush();

    virtual const char* getName() const { return "name unset"; } // for logging purposes
    virtual void dump(int fd) const;

protected:  // methods
    HwStream();           /*Only allow derived classes*/
    virtual ~HwStream();
    status_t notifyListeners(ICssIspListener::IspMessage *msg);

protected:

    /* Listeners management */
    Mutex   mListenerLock;  /*!< Protects accesses to the Listener management variables */
    KeyedVector<ICssIspListener::IspEventType, bool> mEventsProvided;
    typedef List< ICssIspListener* > listener_list_t;
    KeyedVector<ICssIspListener::IspEventType, listener_list_t > mListeners;
    /* end of scope for lock mListenerLock */

    /* stream configuration */
    int mFps;
    FrameInfo mConfig;

    /* PRETTY LOGGING SUPPORT */
private:
    status_t configure(void) { return NO_ERROR; };
    static const char *State2String[];
}; // class CameraHWStream

};  // namespace camera2
};  // namespace android

#endif /* CAMERA3_HAL_HW_STREAM_H_ */
