/*
 * Copyright (C) 2014 Intel Corporation
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
#ifndef _CAMERA3_HAL_CAMERA3REQUEST_H_
#define _CAMERA3_HAL_CAMERA3REQUEST_H_

#include "utils/Vector.h"
#include "utils/RefBase.h"
#include "hardware/camera3.h"
#include "camera/CameraMetadata.h"
#include "CameraStreamNode.h"
#include "CameraBuffer.h"

namespace android {
namespace camera2 {
/**
 * This define is only used for the purpose of the allocation of output buffer
 * pool
 * The exact value for this should be coming from the static metadata tag
 * maxNumOutputStreams. But at this stage we cannot query it because we do
 * not know the camera id. This value should always be bigger than the static
 * tag.
 */
#define MAX_NUMBER_OUTPUT_STREAMS 8


/**
 * \class IRequestCallback
 *
 * This interface is implemented by the ResultProcessor
 * It is used by CameraStreams to report that an output buffer that belongs to
 * a particular request is done
 * It is used by PSL entities to report that  part of the result information is
 * ready
 */
class IRequestCallback {
public:
    virtual ~IRequestCallback() {}

    virtual status_t shutterDone(Camera3Request* request,
                                 int64_t timestamp) = 0;

    virtual status_t metadataDone(Camera3Request* request,
                                  int resultIndex = -1) = 0;

    virtual status_t bufferDone(Camera3Request* request,
                                sp<CameraBuffer> buffer) = 0;
};


// generic template for objects that are shared among threads
// If you see deadlocks with SharedObject, you probably didn't let the previous
// incarnation around the same object to go out of scope (destructor releases).
template<typename TYPE, typename MEMBERTYPE>
class SharedObject {
public:
    explicit SharedObject(TYPE &p) :
        mMembers(p.mMembers), mSharedObject(NULL), mRefSharedObject(p) {
        mRefSharedObject.mLock.lock();
    }
    explicit SharedObject(TYPE* p) :
        mMembers(p->mMembers), mSharedObject(p), mRefSharedObject(*p) {
        mSharedObject->mLock.lock();
    }
    ~SharedObject() {
        mRefSharedObject.mLock.unlock();
    }
    MEMBERTYPE &mMembers;
private:
    SharedObject();
    SharedObject(const SharedObject &);
    SharedObject &operator=(const SharedObject &);
    TYPE* mSharedObject;
    TYPE &mRefSharedObject;
};

// following macro adds the parts that SharedObject template requires, in practice:
// mMembers, mLock and friendship with the SharedObject template
#define SHARED_OBJECT_PARTS(TYPE, MEMBERTYPE) \
    MEMBERTYPE mMembers; \
    mutable Mutex mLock; \
    friend class SharedObject<TYPE, TYPE::MEMBERTYPE>; \
    friend class SharedObject<const TYPE, const TYPE::MEMBERTYPE>

/**
 * \class Camera3Request
 *
 * Internal representation of a user request (capture or re-process)
 * Objects of this class are initialized for each capture request received
 * by the camera device. Once those objects are initialized the request is safe
 * for processing by the Platform Specific Layer.
 *
 * Basic integrity checks are performed on initialization.
 * The class also has other utility methods to ease the PSL implementations
 *
 *
 */
class Camera3Request {
public:
    Camera3Request();
    virtual ~Camera3Request();
    status_t init(camera3_capture_request* req,
                  IRequestCallback* cb, CameraMetadata &settings, int cameraId);
    void deInit();

    /* access methods */
    unsigned int getNumberOutputBufs();
    unsigned int getNumberInputBufs();
             int getId();
             int getpartialResultCount() { return mPartialResultBuffers.size(); }
    CameraMetadata* getPartialResultBuffer(unsigned int index);
    const CameraMetadata* getSettings();

    const camera3_capture_request* getUserRequest();
    const Vector<camera3_stream_buffer>* getInputBuffers();
    const Vector<camera3_stream_buffer>* getOutputBuffers();
    const Vector<CameraStreamNode*>* getOutputStreams();
    const Vector<CameraStreamNode*>* getInputStreams();
    sp<CameraBuffer>    findBuffer(CameraStreamNode* stream);
    bool  isInputBuffer(sp<CameraBuffer> buffer);

    class Members {
    public:
        CameraMetadata mSettings;
    };

    IRequestCallback * mCallback;

private:  /* methods */
    bool isRequestValid(camera3_capture_request * request3);
    status_t checkInputStreams(camera3_capture_request * request3);
    status_t checkOutputStreams(camera3_capture_request * request3);
    status_t initPartialResultBuffers(int cameraId);
    status_t allocatePartialResultBuffers(int bufferCount);
    void freePartialResultBuffers(void);
    void reAllocateResultBuffer(camera_metadata_t* m, int index);

private:  /* types and members */
    // following macro adds the parts that SharedObject template requires, in practice:
    // mMembers, mLock and friendship with the SharedObject template
    SHARED_OBJECT_PARTS(Camera3Request, Members);
    bool  mInitialized;
    CameraMetadata mSettings; /*!< request settings metadata. Always contains a
                                    valid metadata buffer even if the request
                                    had NULL */
    Mutex mAccessLock;  /*!< mutex to ensure thread safe access to private
                             members*/
    unsigned int mRequestId;    /*!< the frame_count from the original request struct */
    camera3_capture_request mRequest3;
    Vector<camera3_stream_buffer> mOutBuffers;
    Vector<camera3_stream_buffer> mInBuffers;
    Vector<CameraStreamNode*> mOutStreams;
    Vector<CameraStreamNode*> mInStreams;
    sp<CameraBuffer>    mOutputBufferPool[MAX_NUMBER_OUTPUT_STREAMS];
    Vector<sp<CameraBuffer> > mOutputBuffers;
    sp<CameraBuffer>        mInputBufferPool;
    sp<CameraBuffer>        mInputBuffer;
    /* Partial result support */
    bool mResultBufferAllocated;
   /**
    * \struct MemoryManagedMetadata
    * This struct is used to store the metadata buffers that are created from
    * memory managed by the HAL.
    * This is needed to avoid continuous allocation/de-allocation of metadata
    * buffers. The underlying memory for this metadata buffes is allocated
    * once but the metadata object can be cleared many times.
    * The need for this struct comes from the fact that there is no API to
    * clear the contents of a metadata buffer completely.
    */
    typedef struct {
         CameraMetadata *metaBuf;
         void   *baseBuf;
         size_t size;
         int dataCap;
         int entryCap;
    } MemoryManagedMetadata;
    Vector<MemoryManagedMetadata> mPartialResultBuffers;
};
typedef SharedObject<const Camera3Request, const Camera3Request::Members> ReadRequest;
typedef SharedObject<Camera3Request, Camera3Request::Members> ReadWriteRequest;


} /* namespace camera2 */
} /* namespace android */
#endif /* _CAMERA3_HAL_CAMERA3REQUEST_H_ */
