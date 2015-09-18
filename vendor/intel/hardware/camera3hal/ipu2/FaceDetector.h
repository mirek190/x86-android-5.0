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

#ifndef ANDROID_FACEDETECTOR_H
#define ANDROID_FACEDETECTOR_H

#include <utils/threads.h>
#include <system/camera.h>
#include "MessageQueue.h"
#include "ia_face.h"

namespace android {
namespace camera2 {


/**
 * the maximum number of faces detectable at the same time
 */
#define MAX_FACES_DETECTABLE 32

class FaceDetector {
// constructor/destructor
 public:
#ifdef ENABLE_INTEL_EXTRAS
    FaceDetector();
    ~FaceDetector();

    int getFaces(camera_face_t *faces, int width, int height,
                    int activeArrayWidth, int activeArrayHeight);
    void getFaceState(ia_face_state *faceStateOut, int width, int height,
                      int zoomRatio);
    int faceDetect(ia_frame *frame);
    status_t clearFacesDetected();
    status_t reset();

// private data
 private:
    ia_face_state* mContext;
    mutable Mutex mLock;
// function stubs for building without Intel extra features
#else
    FaceDetector() {}
    ~FaceDetector() {}

    int getFaces(camera_face_t *faces, int width, int height,
                    int activeArrayWidth, int activeArrayHeight) { return 0; }
    void getFaceState(ia_face_state *faceStateOut, int width, int height,
                      int zoomRatio) {
        assert(faceStateOut != NULL);
        faceStateOut->num_faces = 0;
    }
    int faceDetect(ia_frame *frame) { return 0; }
    status_t reset() { return UNKNOWN_ERROR; }

// Thread overrides
 public:
    status_t requestExitAndWait() { return UNKNOWN_ERROR; }

// inherited from Thread
 private:
    virtual bool threadLoop() { return false; }
#endif

// prevent copy constructor and assignment operator
 private:
    FaceDetector(const FaceDetector& other);
    FaceDetector& operator=(const FaceDetector& other);
};  // class FaceDetector

}  // namespace camera2
}  // namespace android

#endif  // ANDROID_FACEDETECTOR_H
