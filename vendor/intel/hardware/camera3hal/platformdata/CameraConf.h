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

#ifndef _CAMERA3_HAL_CAMERA_CONFIGURATION_H_
#define _CAMERA3_HAL_CAMERA_CONFIGURATION_H_

#include <cpf-hal.h>
#include <libtbd.h>
#include <sys/stat.h>
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <system/camera_metadata.h>
#include "ia_cmc_parser.h"
#include "Metadata.h"

namespace android {
namespace camera2 {

class CameraBlob
{
    class Blob : public RefBase
    {
    public:
        Blob(const int size, void *& ptr);
        virtual ~Blob();
    private:
        void *mPtr;
        // Disallow copy and assignment
        Blob(const Blob&);
        void operator=(const Blob&);
    };

public:
    explicit CameraBlob(const int size = 0);
    CameraBlob(const CameraBlob& refBlob, const int offset, const int size);
    CameraBlob(const CameraBlob& refBlob, void * const ptr, const int size);
    virtual inline ~CameraBlob() { clear(); }
    inline void *ptr() const { return mPtr; }
    inline int size() const { return mSize; }
    inline operator void *() const { return mPtr; }
    inline operator bool() const { return mPtr; }
    CameraBlob copy();
    void clear();
private:
    sp<Blob> mBlob;
    void *mPtr;
    int mSize;
};

class AiqConf : public CameraBlob
{
public:
    explicit inline AiqConf(): mCMC(NULL), mMetadata(NULL) { }
    inline AiqConf(const CameraBlob& refBlob) : CameraBlob(refBlob), mCMC(NULL), mMetadata(NULL) { }
    virtual ~AiqConf();
    ia_cmc_t* getCMCHandler();
    void clear();
    status_t fillStaticMetadataFromCMC(camera_metadata_t * metadata);

private:
    status_t initCMC();
    status_t initMapping();
    status_t fillLensStaticMetadata(camera_metadata_t * metadata);
    status_t fillLightSourceStaticMetadata(camera_metadata_t * metadata);
    status_t fillSensorStaticMetadata(camera_metadata_t * metadata);
    status_t fillLscSizeStaticMetadata(camera_metadata_t * metadata);

private:
    ia_cmc_t *mCMC;
    camera_metadata_t* mMetadata;
    // light source map between android and AIQ
    KeyedVector<CMC_Light_Source, camera_metadata_enum_android_sensor_reference_illuminant1> mLightSourceMap;
};


class CpfStore
{
    struct SensorDriver {
        String8 mSensorName;
        String8 mDeviceName;
    };

public:
    explicit CpfStore(const int cameraId);
    virtual ~CpfStore();
public:
    AiqConf AiqConfig;
private:
    status_t initFileNames(String8& cpfPathName);
    status_t initDriverList();
    status_t initDriverListHelper(unsigned major, unsigned minor, SensorDriver& drvInfo);
    status_t findConfigWithDriver(String8& cpfName, int& drvIndex);
    status_t findConfigWithDriverHelper(const String8& fileName, String8& cpfName, int& index);
    status_t findBusAddress(const int drvIndex, int& i2cBus, int& i2cAddress);
    status_t initConf(CameraBlob& aiqConf);
    status_t loadConf(CameraBlob& allConf);
private:
    int mCameraId;
    bool mIsOldConfig;
    String8 mCpfPathName;
    Vector<struct SensorDriver> mRegisteredDrivers;
    Vector<struct stat> mValidatedCpfFiles;
    // Disallow copy and assignment
    CpfStore(const CpfStore&);
    void operator=(const CpfStore&);
};

}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_HAL_CAMERA_CONFIGURATION_H_
