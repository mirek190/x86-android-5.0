/*
 * Copyright (c) 2014 Intel Corporation.
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

#define LOG_TAG "Camera_FaceDetectionThread"

#include <time.h>
#include <android/sensor.h>
#include <gui/Sensor.h>
#include <gui/SensorManager.h>
#include "LogHelper.h"
#ifdef GRAPHIC_IS_GEN
#include <ufo/graphics.h>
#endif

#include "FaceDetectionThread.h"
#include "IPU2CameraHw.h"
#include "ia_coordinate.h"


namespace android {
namespace camera2 {
FaceDetectionThread::FaceDetectionThread(const CameraMetadata &staticMeta,
                                               IFaceDetectCallback * callback, int cameraId):
    mCameraId(cameraId)
    , mCallbacks(callback)
    , mMessageQueue("FaceDetectionThread", (int)MESSAGE_ID_MAX)
    , mFaceDetector(NULL)
    , mLastReportedNumberOfFaces(0)
    , mThreadRunning(false)
    , mFaceDetectionRunning(false)
    , mRotation(0)
    , mSensorEventQueue(NULL)
    , mFrameCount(0)
    , mNextWriteProcessFrame(0)
    , mNextReadProcessFrame(0)
{
    mMessageThead = new MessageThread(this, "FaceDetectionThread",
                                      PRIORITY_NORMAL + ANDROID_PRIORITY_LESS_FAVORABLE);
    if (mMessageThead == NULL) {
        LOGE("Error create FaceDetectionThread in init");
    }

    mFaceDetector = new FaceDetector();
    if (mFaceDetector == NULL) {
        LOGE("Error creating FaceDetector");
    }

    const IPU2CameraCapInfo * capInfo = getIPU2CameraCapInfo(mCameraId);
    if (capInfo != NULL) {
        int count = 0;
        int32_t * orientation = (int32_t*)MetadataHelper::getMetadataValues(staticMeta,
                                     ANDROID_SENSOR_ORIENTATION,
                                     TYPE_INT32,
                                     &count);
        mCameraOrientation = (orientation != NULL) ? (*orientation) : 90;
        mSensorLayout = capInfo->sensorLayout();
    } else {
        mCameraOrientation = 90;
        mSensorLayout = 0;
    }

    SensorManager& sensorManager(SensorManager::getInstance());
    mSensorEventQueue = sensorManager.createEventQueue();
    if (mSensorEventQueue == NULL) {
        LOGE("sensorManager createEventQueue failed");
    }

    int count = 0;
    int32_t * activeArraySize = (int32_t*)MetadataHelper::getMetadataValues(
                                     staticMeta,
                                     ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                                     TYPE_INT32,
                                     &count);
    if (activeArraySize != NULL && count == 4) {
        mActiveArrayWidth = activeArraySize[2];
        mActiveArrayHeight = activeArraySize[3];
    } else {
        mActiveArrayWidth = 0;
        mActiveArrayHeight = 0;
    }
    ia_frame* frameData = NULL;
    mPendingFrameData.clear();
    mPendingFrameData.setCapacity(FACE_MAX_FRAME_DATA);
    for (int i = 0; i < FACE_MAX_FRAME_DATA; i++) {
         frameData = new ia_frame;
         if (frameData == NULL)
             LOGE("there is no enough memory");
         else {
             CLEAR(*frameData);
             mPendingFrameData.push_back(frameData);
         }
    }
}

FaceDetectionThread::~FaceDetectionThread()
{
    LOG1("@%s", __FUNCTION__);
    ia_frame* frameData = NULL;

    if (mSensorEventQueue != NULL)
        mSensorEventQueue.clear();

    if (mMessageThead != NULL) {
        mMessageThead->requestExitAndWait();
        mMessageThead.clear();
        mMessageThead = NULL;
    }

    if (mFaceDetector != NULL) {
        delete mFaceDetector;
        mFaceDetector = NULL;
    }
    //release all the buffer
    for (unsigned int i = 0; i < FACE_MAX_FRAME_DATA; i++) {
         frameData = mPendingFrameData.editItemAt(i);
         if (frameData->data != NULL) {
             free(frameData->data);
             frameData->data = NULL;
         }
         delete frameData;
         frameData = NULL;
    }
    mPendingFrameData.clear();
}

status_t FaceDetectionThread::processRequest(Camera3Request* request)
{
    LOG2("%s", __FUNCTION__);
    Message msg;
    status_t ret =NO_ERROR;

    msg.id = MESSAGE_ID_NEW_REQUEST;
    msg.request = request;
    ret =  mMessageQueue.send(&msg);
    return ret;
}

status_t FaceDetectionThread::handleNewRequest(Message &msg)
{
    LOG2("%s", __FUNCTION__);
    status_t status = OK;

    ReadWriteRequest rwRequest(msg.request);
    const camera3_capture_request* userReq = msg.request->getUserRequest();
    if (userReq->settings != NULL) {
        // settings is NULL, if settings have not changed
        status = processSettings(rwRequest.mMembers.mSettings);
        if (status != OK) {
            LOGE("@%s bad settings provided for 3A", __FUNCTION__);
        }
    }

    return status;
}

status_t FaceDetectionThread::processSettings(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;
    unsigned int tag = ANDROID_STATISTICS_FACE_DETECT_MODE;
    camera_metadata_ro_entry entry = settings.find(tag);
    if (entry.count == 1) {
        if (entry.data.u8[0] == ANDROID_STATISTICS_FACE_DETECT_MODE_OFF) {
            LOGD("face detection had been off");
            // current status is on,so stop face detection
            if (mFaceDetectionRunning)
                stopFaceDetection(false);  // bool wait
        } else {
            LOGD("face detection had been on");
            startFaceDetection();
        }
    }
    return status;
}

bool FaceDetectionThread::notifyIspEvent(ICssIspListener::IspMessage *msg)
{
    LOG2("@%s", __FUNCTION__);

    if (msg && msg->streamBuffer.get())
        sendFrame(msg->streamBuffer);

    return true;
}

void FaceDetectionThread::startFaceDetection()
{
    LOG1("@%s", __FUNCTION__);
    Message msg;
    msg.id = MESSAGE_ID_START_FACE_DETECTION;
    mMessageQueue.send(&msg);
}

status_t FaceDetectionThread::handleMessageStartFaceDetection()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    // Reset the face detection state:
    mLastReportedNumberOfFaces = 0;
    // .. also keep sync the face status:
    mCallbacks->FaceDetectCallbackFor3A(NULL);
    mCallbacks->FaceDetectCallbackForApp(NULL);

    mFaceDetectionRunning = true;
    mFrameCount = 0;

    return status;
}

void FaceDetectionThread::stopFaceDetection(bool wait)
{
    LOG1("@%s", __FUNCTION__);
    Message msg;
    msg.id = MESSAGE_ID_STOP_FACE_DETECTION;

    if (wait) {
        // wait for reply
        mMessageQueue.send(&msg, MESSAGE_ID_STOP_FACE_DETECTION);
    } else {
        mMessageQueue.send(&msg);
    }
}

status_t FaceDetectionThread::handleMessageStopFaceDetection()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    if (mFaceDetectionRunning) {
        mFaceDetectionRunning = false;
        status = mFaceDetector->clearFacesDetected();
    }

    mMessageQueue.reply(MESSAGE_ID_STOP_FACE_DETECTION, status);

    return status;
}

int FaceDetectionThread::sendFrame(sp<CameraBuffer> img)
{
    if (img != NULL) {
        LOG2("@%s: buf=%p, width=%d height=%d",
             __FUNCTION__, img->data(), img->width(), img->height());
    } else {
        LOG2("@%s: buf=NULL", __FUNCTION__);
    }
    Message msg;
    msg.id = MESSAGE_ID_NEW_IMAGE;

    // Face detection may take long time, which
    // slows down the preview because the buffers are
    // not returned until they are processed.
    // Allow post-processing only when the queue is empty.
    // Otherwise the frame will be skipped,
    if (!mMessageQueue.isEmpty()) {
        LOG1("@%s: skipping frame", __FUNCTION__);
        return -1;
    }

    if (img != NULL) {
        msg.data.frame.img = img;
    } else {
        LOGE("@%s: NULL AtomBuffer frame", __FUNCTION__);
        return -1;
    }
    if (mFaceDetectionRunning)
        frameDataCopy(img);
    if (mMessageQueue.send(&msg) == NO_ERROR)
        return 0;
    else
        return -1;
}

void FaceDetectionThread::detectRotationEvent(void)
{
    LOG2("@%s", __FUNCTION__);

    nsecs_t startTime = systemTime();
    ssize_t num_sensors(0);
    int orientation(-1);
    ASensorEvent sen_events[8];

    if (mSensorEventQueue == NULL)
        return ;

    while ((num_sensors = mSensorEventQueue->read(sen_events, 8)) > 0) {
        for (int i = 0; i < num_sensors; i++) {
            if (sen_events[i].type == Sensor::TYPE_ACCELEROMETER) {
                float x = sen_events[i].acceleration.x;
                float y = sen_events[i].acceleration.y;
                float z = sen_events[i].acceleration.z;

                orientation = (int) (atan2f(-x, y) * 180 / M_PI);

                if (orientation < 0) {
                    orientation += 360;
                }

                LOG2("@%s: Accelerometer event: x = %f y = %f z = %f orientation = %d",
                        __FUNCTION__, x, y, z, orientation);
            }
        }
    }

    if (num_sensors < 0 && num_sensors != -EAGAIN) {
        LOGE("reading sensors events failed: %s", strerror(-num_sensors));
    }

    if (orientation != -1) {
        orientation += 45;
        orientation = (orientation / 90) * 90;
        orientation = orientation % 360;
        mRotation = orientation;
    }

    LOG2("Take %ums to detect the angle of G-sensor", (unsigned int)((systemTime() - startTime)/1000000));
}


status_t FaceDetectionThread::handleFrame(MessageFrame frame)
{
    LOG2("@%s, mFaceDetectionRunning=%d", __FUNCTION__, mFaceDetectionRunning);
    UNUSED(frame);
    status_t status = NO_ERROR;
    if (mFaceDetectionRunning) {
        LOG2("%s: Face detection executing", __FUNCTION__);
        int num_faces;
        int rotation;
        ia_frame *frameData = NULL;

        {
            Mutex::Autolock _l(mPendingQueueLock);
            if (mNextReadProcessFrame == mNextWriteProcessFrame) {
                LOGW("there is no frame yet");
            }
            frameData = mPendingFrameData.editItemAt(mNextReadProcessFrame);
            mNextReadProcessFrame = (mNextReadProcessFrame + 1) % FACE_MAX_FRAME_DATA;
        }
        //add rotation info, detect once every 6 frames(200ms).
        if (mFrameCount % 6 == 0) {
            detectRotationEvent();
        }

        if (mCameraId == 0)
            rotation = (mCameraOrientation + mRotation + mSensorLayout) % 360;
        else
            rotation = (mCameraOrientation - mRotation + mSensorLayout + 360) % 360;

        // frame rotation is counter clock wise in libia_face,
        // while it is clock wise for android (valid for CTP)
        if (rotation == 90)
            frameData->rotation = 270;
        else if (rotation == 270)
            frameData->rotation = 90;
        else
            frameData->rotation = rotation;

        num_faces = mFaceDetector->faceDetect(frameData);

        camera_face_t faces[num_faces];
        camera_frame_metadata_t face_metadata;

        ia_face_state faceState;
        faceState.faces = new ia_face[num_faces];
        if (faceState.faces == NULL) {
            LOGE("Error allocation memory");
            return NO_MEMORY;
        }

        face_metadata.number_of_faces = mFaceDetector->getFaces(faces,
                                                       frameData->width,
                                                       frameData->height,
                                                       mActiveArrayWidth,
                                                       mActiveArrayHeight);
        LOG2("face_metadata.number_of_faces=%d", face_metadata.number_of_faces);
        face_metadata.faces = faces;
        int mZoomRatio = 100;
        mFaceDetector->getFaceState(&faceState, frameData->width, frameData->height, mZoomRatio);
        // Find recognized faces from the data (ID is positive), and pick the first one:
        int faceForFocusInd = 0;
        for (int i = 0; i < num_faces; ++i) {
            if (faceState.faces[i].person_id > 0) {
                LOG2("Face index: %d, ID: %d", i, faceState.faces[i].person_id);
                faceForFocusInd = i;
                break;
            }
        }
        // and put the recognized face as first entry in the array for AF to use
        // No need to swap if face is already in the first pos.
        if (faceForFocusInd > 0) {
            ia_face faceSwapTmp = faceState.faces[0];
            faceState.faces[0] = faceState.faces[faceForFocusInd];
            faceState.faces[faceForFocusInd] = faceSwapTmp;
        }

        // Swap also the face in face metadata going to the application
        // to match the swapped faceState info
        if (face_metadata.number_of_faces > 0 && faceForFocusInd > 0) {
            camera_face_t faceMetaTmp = face_metadata.faces[0];
            face_metadata.faces[0] = face_metadata.faces[faceForFocusInd];
            face_metadata.faces[faceForFocusInd] = faceMetaTmp;
        }

        // pass face info to the callback listener (to be used for 3A)
        if (face_metadata.number_of_faces > 0 || mLastReportedNumberOfFaces != 0) {
            mLastReportedNumberOfFaces = face_metadata.number_of_faces;
            mCallbacks->FaceDetectCallbackFor3A(&faceState);
        }
        // and towards the application
        mCallbacks->FaceDetectCallbackForApp(&face_metadata);

        delete[] faceState.faces;
        faceState.faces = NULL;
    }
    mFrameCount++;

    return status;
}

status_t FaceDetectionThread::yChannelCopy(sp<CameraBuffer> img, ia_frame* dst)
{
    LOG2("@%s", __FUNCTION__);
    if(img.get() == NULL || dst == NULL)
        return UNKNOWN_ERROR;

    status_t status = NO_ERROR;

    dst->width = img->width();
    dst->height = img->height();
    dst->stride = img->stride();
    // only support NV12, to be supported more formats in the future
    // HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED and HAL_PIXEL_FORMAT_YCbCr_420_888
    // are mapped to NV12 now.
    switch(img->format()) {
#ifdef GRAPHIC_IS_GEN
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
    case HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL:
#else
    case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
    case HAL_PIXEL_FORMAT_YCbCr_420_888:
#endif
        memcpy(dst->data, img->data(), dst->size);
        dst->format = ia_frame_format_nv12;
        break;
    default:
        LOGE("ERROR @%s: Unknow format %d", __FUNCTION__, dst->format);
        status = UNKNOWN_ERROR;
        break;
    }
    return status;
}

status_t FaceDetectionThread::frameDataCopy(sp<CameraBuffer> img)
{
    LOG2("@%s", __FUNCTION__);
    ia_frame *frameData = NULL;

    Mutex::Autolock _l(mPendingQueueLock);
    if (mPendingFrameData.isEmpty()) {
        // there is still no buffer in the queue
        LOGE("there is no frame in the queue");
        return UNKNOWN_ERROR;
    }
    frameData = mPendingFrameData.editItemAt(mNextWriteProcessFrame);
    mNextWriteProcessFrame = (mNextWriteProcessFrame + 1) % FACE_MAX_FRAME_DATA;
    if (frameData->size != img->stride() * img->height()) {
        // The passed frame is changed, need re-allocate buffer
        if (frameData->data != NULL) {
            free(frameData->data);
            frameData->data = NULL;
        }
        frameData->size = img->stride() * img->height();
        frameData->data = (unsigned char*) malloc (frameData->size);
        if (frameData->data == NULL)
            return NO_MEMORY;
    }
    // Only Y channel is enough for ia_face
    yChannelCopy(img, frameData);
    return NO_ERROR;
}

void FaceDetectionThread::messageThreadLoop()
{
    LOG2("@%s: Start", __FUNCTION__);

    mThreadRunning = true;
    while (mThreadRunning) {
        status_t status = NO_ERROR;

        Message msg;
        mMessageQueue.receive(&msg);
        PERFORMANCE_HAL_ATRACE_PARAM1("msg", msg.id);
        switch (msg.id) {
        case MESSAGE_ID_EXIT:
            status = handleMessageExit();
            break;

        case MESSAGE_ID_NEW_REQUEST:
            status = handleNewRequest(msg);
            break;

        case MESSAGE_ID_NEW_IMAGE:
            status = handleFrame(msg.data.frame);
            break;

        case MESSAGE_ID_START_FACE_DETECTION:
            status = handleMessageStartFaceDetection();
            break;

        case MESSAGE_ID_STOP_FACE_DETECTION:
            status = handleMessageStopFaceDetection();
            break;

        default:
            LOGE("ERROR @%s: Unknow message %d", __FUNCTION__, msg.id);
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE("    error %d in handling message: %d", status, static_cast<int>(msg.id));
        mMessageQueue.reply(msg.id, status);
    }

    LOG2("%s: Exit", __FUNCTION__);
}
status_t FaceDetectionThread::requestExitAndWait(void)
{
    Message msg;
    msg.id = MESSAGE_ID_EXIT;
    mMessageQueue.send(&msg);
    return true;
}

status_t FaceDetectionThread::handleMessageExit(void)
{
    mThreadRunning = false;
    return NO_ERROR;
}

}  // namespace camera2
}  // namespace android


