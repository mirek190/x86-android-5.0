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

#include "ExifCreater.h"
#include "3ATypes.h"
#include "camera/CameraMetadata.h"
#include "EXIFMetaData.h"


#ifndef _EXIFMAKER_H_
#define _EXIFMAKER_H_

namespace android {
namespace camera2 {

/**
 * \class EXIFMaker
 *
 */
class EXIFMaker {
private:
    ExifCreater encoder;
    exif_attribute_t exifAttributes;
    int mThumbSize[2];
    size_t exifSize;
    bool initialized;

    void clear();
public:
    EXIFMaker();
    ~EXIFMaker();

    void initialize(int width, int height);
    bool isInitialized() { return initialized; }
    void initializeLocation(sp<ExifMetaData> metadata);
    void setMakerNote(const ia_binary_data &aaaMkNoteData);
    uint32_t getMakerNoteDataSize() const;
    void setDriverData(const atomisp_makernote_info &ispData);
    void setSensorAeConfig(const SensorAeConfig &sensorParams);
    void pictureTaken(sp<ExifMetaData> exifmetadata);
    void enableFlash();
    void setThumbnail(unsigned char *data, size_t size);
    bool isThumbnailSet() const;
    size_t makeExif(unsigned char **data);
    size_t makeExifInPlace(unsigned char *bufferStartAddr,
                           unsigned char *dqtAddress,
                           size_t jpegSize,
                           bool usePadding);
    void setMaker(const char *data);
    void setModel(const char *data);
    void setSoftware(const char *data);

// prevent copy constructor and assignment operator
private:
    EXIFMaker(const EXIFMaker& other);
    EXIFMaker& operator=(const EXIFMaker& other);
};

}; // namespace camera2
}; // namespace android
#endif // _EXIFMAKER_H_
