/*
 * Copyright (C) 2011 The Android Open Source Project
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
#define LOG_TAG "Camera_FaceDetector"

#ifdef ENABLE_INTEL_EXTRAS

#include "FaceDetector.h"
#include "LogHelper.h"
#include "sqlite3.h"
#include "cutils/properties.h"
#include "ICameraHwControls.h"

#include "ia_coordinate.h"

namespace android {
namespace camera2 {

FaceDetector::FaceDetector() :
    mContext(ia_face_init(NULL))
{
    LOG1("@%s", __FUNCTION__);
}

FaceDetector::~FaceDetector()
{
    LOG1("@%s", __FUNCTION__);

    Mutex::Autolock lock(mLock);
    ia_face_uninit(mContext);
    mContext = NULL;
}

int FaceDetector::faceDetect(ia_frame *frame)
{
    LOG2("@%s", __FUNCTION__);
    Mutex::Autolock lock(mLock);
    ia_face_detect(mContext, frame);
    return mContext->num_faces;
}

status_t FaceDetector::clearFacesDetected()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    Mutex::Autolock lock(mLock);
    if (mContext != NULL)
        ia_face_clear_result(mContext);
    else
        status = INVALID_OPERATION;
    return status;
}

status_t FaceDetector::reset()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    Mutex::Autolock lock(mLock);

    if (mContext != NULL)
        ia_face_reinit(mContext);
    else
        status = INVALID_OPERATION;

    return status;
}


/**
 * Converts the detected faces from ia_face format to metadata format.
 *
 * @param faces_out: [OUT]: Detected faces in Google format
 * @param width: [IN]: Width of the preview frame
 * @param height: [IN]: Height of the preview frame
 * @param activeArrayWidth: [IN]: width of sensor output
 * @param activeArrayHeight: [IN]: Height of sensor output
 *
 * @return Number of faces
 */
int FaceDetector::getFaces(camera_face_t *faces_out, int width, int height,
                              int activeArrayWidth, int activeArrayHeight)
{
    LOG2("@%s", __FUNCTION__);
    Mutex::Autolock lock(mLock);

    for (int i = 0; i < mContext->num_faces; i++)
    {
        camera_face_t& face = faces_out[i];
        ia_face iaFace = mContext->faces[i];

        // ia_face coordinate range is [0 ... width] or [0 ... height]
        // coordinate in Android is base on active array size of sensor
        face.rect[0] = iaFace.face_area.left * activeArrayWidth / width;
        face.rect[1] = iaFace.face_area.top * activeArrayHeight / height;
        face.rect[2] = iaFace.face_area.right * activeArrayWidth / width;
        face.rect[3] = iaFace.face_area.bottom * activeArrayHeight / height;

        face.score = iaFace.confidence;

        // Face ID should be unique for each detected face,
        // so fill the face ID with tracking ID if
        // FR is not enabled (person_id is always 0) or
        // when the face is not recognized
        // (person_id is always -1000).
        // Tracking ID is always unique for each detected face.
        // Use negative values below -1000 so that the application
        // doesn't misinterpret the face as recognized and that the CTS tests pass.
        face.id = iaFace.person_id > 0 ? iaFace.person_id :
                              (-1000 - iaFace.tracking_id);

        face.left_eye[0] = iaFace.left_eye.position.x * activeArrayWidth / width;
        face.left_eye[1] = iaFace.left_eye.position.y * activeArrayHeight / height;

        face.right_eye[0] = iaFace.right_eye.position.x * activeArrayWidth / width;
        face.right_eye[1] = iaFace.right_eye.position.y * activeArrayHeight / height;

        face.mouth[0] = iaFace.mouth.x * activeArrayWidth / width;
        face.mouth[1] = iaFace.mouth.y * activeArrayHeight / height;

        LOG2("face id: %d, score: %d", face.id, face.score);
        LOG2("coordinates: (%d, %d, %d, %d)", face.rect[0], face.rect[1],
                                              face.rect[2], face.rect[3]);
        LOG2("mouth: (%d, %d)", face.mouth[0], face.mouth[1]);
        LOG2("left eye: (%d, %d), blink confidence: %d", face.left_eye[0],
                                                         face.left_eye[1],
                                        iaFace.left_eye.blink_confidence);
        LOG2("right eye: (%d, %d), blink confidence: %d", face.right_eye[0],
                                                          face.right_eye[1],
                                         iaFace.right_eye.blink_confidence);
        LOG2("smile state: %d, score: %d", iaFace.smile_state, iaFace.smile_score);
    }
    return mContext->num_faces;
}

/**
 * Return detected faces in ia_face format and ia_coordinate
 *
 * @param faces_out: [OUT]: Detected faces
 * @param width: [IN]: Width of the preview frame
 * @param height: [IN]: Height of the preview frame
 * @param zoomRatio: [IN]: digital zoomRatio of preview frame multiblied by 100
 *
 * @return Number of faces
 */
void FaceDetector::getFaceState(ia_face_state *faceStateOut,
                                    int width, int height, int zoomRatio)
{
    LOG2("@%s", __FUNCTION__);
    Mutex::Autolock lock(mLock);

    if (faceStateOut == NULL) {
        LOGE("faceStateOut is NULL");
        return;
    }

    faceStateOut->num_faces = mContext->num_faces;
    memcpy(faceStateOut->faces, mContext->faces,
           mContext->num_faces * sizeof(ia_face));

    // ia_face coordinate range is [0 ... width] or [0 ... height]
    ia_coordinate_system srcCoordinateSystem;
    srcCoordinateSystem.top = 0;
    srcCoordinateSystem.left = 0;
    srcCoordinateSystem.bottom = height;
    srcCoordinateSystem.right = width;
    // use zoom ratio calculate where visible frame is in ia_coorinates
    ia_coordinate_system trgCoordinateSystem;
    int64_t targetWidth = IA_COORDINATE_WIDTH * 100 / zoomRatio;
    int64_t targetHeight = IA_COORDINATE_HEIGHT * 100 / zoomRatio;
    trgCoordinateSystem.top = IA_COORDINATE_TOP +
                             (IA_COORDINATE_HEIGHT - targetHeight) / 2;
    trgCoordinateSystem.left = IA_COORDINATE_LEFT +
                              (IA_COORDINATE_WIDTH - targetWidth) / 2;
    trgCoordinateSystem.bottom = trgCoordinateSystem.top + targetHeight;
    trgCoordinateSystem.right = trgCoordinateSystem.left + targetWidth;
    ia_coordinate_convert_faces(faceStateOut,
                                &srcCoordinateSystem,
                                &trgCoordinateSystem);
}

}  // namespace camera2
}  // namespace android

#endif  // ENABLE_INTEL_EXTRAS
