/*
 * Copyright (C) 2012,2013 Intel Corporation
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

#ifndef _CAMERA3_HAL_CAMERA_AREAS_H_
#define _CAMERA3_HAL_CAMERA_AREAS_H_

#include <utils/Errors.h>
#include <utils/Vector.h>

namespace android {
namespace camera2 {

typedef struct CameraWindow {
    int x_left;
    int x_right;
    int y_top;
    int y_bottom;
    int weight;
} CameraWindow;

/**
 * \class CameraAreas
 *
 */
class CameraAreas {
public:
    CameraAreas();
    ~CameraAreas();

    CameraAreas(const CameraAreas& other);
    CameraAreas& operator=(const CameraAreas& other);

    bool operator==(const CameraAreas& other) const;
    bool operator!=(const CameraAreas& other) const;

    status_t scan(const int * area, int areasNum);

    bool isEmpty() const;
    int numOfAreas() const;

    void toWindows(CameraWindow *windows) const;

// private types
private:
    struct CameraAreaItem {
        int x_left;
        int x_right;
        int y_top;
        int y_bottom;
        int weight;

        bool operator==(const CameraAreaItem& other) const;
        bool operator!=(const CameraAreaItem& other) const;
    };

// private methods
private:
    bool isValidArea(const struct CameraAreaItem& area) const;

// private data
private:
    Vector<CameraAreaItem> mAreas;

}; // class CameraAreas

} // namespace camera2
} // namespace android

#endif // _CAMERA3_HAL_CAMERA_AREAS_H_
