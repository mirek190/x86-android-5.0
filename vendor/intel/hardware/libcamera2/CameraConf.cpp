/*
 * Copyright 2012 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "Camera_Conf"

#include "CameraConf.h"
#include "LogHelper.h"
#include "PlatformData.h"
#include <dirent.h>        // DIR, dirent
#include <fnmatch.h>       // fnmatch()
#include <fcntl.h>         // open(), close()
#include <linux/media.h>   // media controller
#include <linux/kdev_t.h>  // MAJOR(), MINOR()
#include <utils/Errors.h>  // Error codes

namespace android {

using namespace CPF;

const char *cpfConfigPath = "/etc/atomisp/";  // Where CPF files are located
// FIXME: The spec for following is "dr%02d[0-9][0-9]??????????????.aiqb"
const char *cpfConfigPattern = "%02d*.aiqb";  // How CPF file name should look

// Defining and initializing static members
Vector<struct CpfStore::SensorDriver> CpfStore::RegisteredDrivers;
Vector<struct stat> CpfStore::ValidatedCpfFiles;

CameraBlob::Blob::Blob(const int size, void *& ptr)
{
    ptr = 0;
    if (size == 0) {
        mPtr = 0;
    } else {
        ptr = mPtr = malloc(size);
    }
}

CameraBlob::Blob::~Blob()
{
    free(mPtr);
    mPtr = NULL;
}

CameraBlob::CameraBlob(const int size)
{
    mSize = 0;
    mPtr = 0;
    if (size) {
        mBlob = new Blob(size, mPtr);
        if (mBlob != 0) {
            if (mPtr) {
                mSize = size;
            } else {
                mBlob.clear();
            }
        }
    }
}

CameraBlob::CameraBlob(const CameraBlob& refBlob, const int offset, const int size)
{
    mSize = 0;
    mPtr = 0;
    if (refBlob.mBlob == 0) {
        ALOGE("ERROR referring to null object!");
        return;
    }
    // Must refer only to memory allocated by reference object
    if (refBlob.size() < offset + size) {
        ALOGE("ERROR illegal allocation!");
        return;
    }
    mBlob = refBlob.mBlob;
    mSize = size;
    mPtr = (char *)(refBlob.ptr()) + offset;
}

CameraBlob::CameraBlob(const CameraBlob& refBlob, void * const ptr, const int size)
{
    mSize = 0;
    mPtr = 0;
    if (refBlob.mBlob == 0) {
        ALOGE("ERROR referring to null object!");
        return;
    }
    // Must refer only to memory allocated by reference object
    int offset = (char *)(ptr) - (char *)(refBlob.ptr());
    if ((offset < 0) || (offset + size > refBlob.size())) {
        ALOGE("ERROR illegal allocation!");
        return;
    }
    mBlob = refBlob.mBlob;
    mSize = size;
    mPtr = ptr;
}

CameraBlob CameraBlob::copy()
{
    CameraBlob newBlob(size());
    if (newBlob && ptr() && size()) {
        memcpy(newBlob, ptr(), size());
    }
    return newBlob;
}

void CameraBlob::clear()
{
     mBlob.clear();
     mSize = 0;
     mPtr = 0;
}

CpfStore::CpfStore(const int cameraId)
    : mCameraId(cameraId)
    , mIsOldConfig(false)
{
    CameraBlob aiqConf;

    // If anything goes wrong here, we simply return silently.
    // CPF should merely be seen as a way to do multiple configurations
    // at once; failing in that is not a reason to return with errors
    // and terminate the camera (some cameras may not have any CPF at all)

    if (mCameraId >= PlatformData::numberOfCameras()) {
        ALOGE("ERROR bad camera index!");
        mCameraId = -1;
        return;
    }

    // Find out the related file names
    if (initFileNames(mCpfPathName)) {
        // Error message given already
        return;
    }

    // Obtain the configurations
    if (initConf(aiqConf)) {
        // Error message given already
        return;
    }

    // Provide configuration data for algorithms and image
    // quality purposes, and continue further even if errors did occur.
    // Pointer to that data is cleared later, whenever seen suitable,
    // so that the memory reserved for CPF data can then be freed
    AiqConfig = aiqConf;
}

CpfStore::~CpfStore()
{
}

status_t CpfStore::initFileNames(String8& cpfPathName)
{
    status_t ret = 0;

    // First, we see what drivers we have in the system
    if ((ret = initDriverList())) {
        // Error message given already
        return ret;
    }

    // Secondly, we will find a matching configuration file
    int drvIndex = -1;   // Index of registered driver for which CPF file exists
    String8 cpfFileName; // Name of the corresponding CPF config file
    if ((ret = findConfigWithDriver(cpfFileName, drvIndex))) {
        // Error message given already
        return ret;
    }

    // Here is the correct CPF file
    cpfPathName = cpfConfigPath;
    cpfPathName.appendPath(cpfFileName);

    ALOGD("cpf config file name: %s", cpfPathName.string());

    return ret;
}

status_t CpfStore::initDriverList()
{
    status_t ret = 0;

    if (RegisteredDrivers.size() > 0) {
        // We only need to go through the drivers once
        return 0;
    }

    // Sensor drivers have been registered to media controller
    const char *mcPathName = "/dev/media0";
    int fd = open(mcPathName, O_RDONLY);
    if (fd == -1) {
        ALOGE("ERROR in opening media controller: %s!", strerror(errno));
        return ENXIO;
    }

    struct media_entity_desc entity;
    memset(&entity, 0, sizeof(entity));
    do {
        // Go through the list of media controller entities
        entity.id |= MEDIA_ENT_ID_FLAG_NEXT;
        if (ioctl(fd, MEDIA_IOC_ENUM_ENTITIES, &entity) < 0) {
            if (errno == EINVAL) {
                // Ending up here when no more entities left.
                // Will simply 'break' if everything was ok
                if (RegisteredDrivers.size() == 0) {
                    // No registered drivers found
                    ALOGE("ERROR no sensor driver registered in media controller!");
                    ret = NO_INIT;
                }
            } else {
                ALOGE("ERROR in browsing media controller entities: %s!", strerror(errno));
                ret = FAILED_TRANSACTION;
            }
            break;
        } else {
            if (entity.type == MEDIA_ENT_T_V4L2_SUBDEV_SENSOR) {
                // A driver has been found!
                // The driver is using sensor name when registering
                // to media controller (we will truncate that to
                // first space, if any)
                SensorDriver drvInfo;
                drvInfo.mSensorName = entity.name;
                // Cut the name to first space
                for (int i = 0; (i = drvInfo.mSensorName.find(" ")) > 0; drvInfo.mSensorName.setTo(drvInfo.mSensorName, i));

                unsigned major = entity.v4l.major;
                unsigned minor = entity.v4l.minor;

                // Go thru the subdevs one by one, see which one
                // corresponds to this driver (if there is an error,
                // the looping ends at 'while')
                ret = initDriverListHelper(major, minor, drvInfo);
            }
        }
    } while (!ret);

    if (close(fd)) {
        ALOGE("ERROR in closing media controller: %s!", strerror(errno));
        if (!ret) ret = EPERM;
    }

    return ret;
}

status_t CpfStore::initDriverListHelper(unsigned major, unsigned minor, SensorDriver& drvInfo)
{
    String8 subdevPathNameN;

    const char *subdevPathName = "/dev/v4l-subdev%d";
    for (int n = 0; true; n++) {
        subdevPathNameN = String8::format(subdevPathName, n);
        struct stat fileInfo;
        if (stat(subdevPathNameN, &fileInfo) < 0) {
            // We end up here when there are no more subdevs
            if (errno == ENOENT) {
                ALOGE("ERROR sensor subdev missing: \"%s\"!", subdevPathNameN.string());
                return NO_INIT;
            } else {
                ALOGE("ERROR querying sensor subdev filestat for \"%s\": %s!", subdevPathNameN.string(), strerror(errno));
                return FAILED_TRANSACTION;
            }
        }
        if ((major == MAJOR(fileInfo.st_rdev)) && (minor == MINOR(fileInfo.st_rdev))) {
            drvInfo.mDeviceName = subdevPathNameN.getPathLeaf();
            RegisteredDrivers.push(drvInfo);
            ALOGD("Registered sensor driver \"%s\" found for sensor \"%s\"", drvInfo.mDeviceName.string(), drvInfo.mSensorName.string());
            // All ok
            break;
        }
    }

    return 0;
}

status_t CpfStore::findConfigWithDriver(String8& cpfName, int& drvIndex)
{
    status_t ret = 0;
    bool anyMatch = false;

    // We go the directory containing CPF files thru one by one file,
    // and see if a particular file is something to react upon. If yes,
    // we then see if there is a corresponding driver registered. It
    // is allowed to have more than one CPF file for particular driver
    // (spId values are used for further distinguishing in that case),
    // but having more than one suitable driver registered is a strict no no...

    DIR *dir = opendir(cpfConfigPath);
    if (!dir) {
        ALOGE("ERROR in opening CPF folder \"%s\": %s!", cpfConfigPath, strerror(errno));
        return ENOTDIR;
    }

    do {
        dirent *entry;
        if (errno = 0, !(entry = readdir(dir))) {
            if (errno) {
                ALOGE("ERROR in browsing CPF folder \"%s\": %s!", cpfConfigPath, strerror(errno));
                ret = FAILED_TRANSACTION;
            }
            // If errno was not set, return 0 means end of directory.
            // So, let's see if we found any (suitable) CPF files
            if (drvIndex < 0) {
                if (anyMatch) {
                    ALOGE("NOTE no suitable CPF files found in CPF folder \"%s\" (ok for SOC cameras)", cpfConfigPath);
                } else {
                    ALOGE("NOTE not a single CPF file found in CPF folder \"%s\" (ok for SOC cameras)", cpfConfigPath);
                }
                ret = NO_INIT;
            }
            break;
        }
        if ((strcmp(entry->d_name, ".") == 0) ||
            (strcmp(entry->d_name, "..") == 0)) {
            continue;  // Skip self and parent
        }

        String8 devName(entry->d_name);
        // For multi-sensor case, we may have 2 front camera sensors which will cause we find
        // two CPF files matched with driver. So we can use sensor name to distinguish CPF files.
        if (strlen(PlatformData::sensorName(mCameraId)) > 0) {
            if (devName.find(PlatformData::sensorName(mCameraId)) < 0)
                continue;
        }

        String8 pattern = String8::format(cpfConfigPattern, mCameraId);
        int r = fnmatch(pattern, entry->d_name, 0);

        switch (r) {
        case 0:
            // The file name looks like a valid CPF file name
            anyMatch = true;
            // See if we have corresponding driver registered
            // (if there is an error, the looping ends at 'while')
            ret = findConfigWithDriverHelper(String8(entry->d_name), cpfName, drvIndex);
            continue;
        case FNM_NOMATCH:
            // The file name did not look like a CPF file name
            continue;
        default:
            // Unknown error (the looping ends at 'while')
            ALOGE("ERROR in pattern matching file name \"%s\"!", entry->d_name);
            ret = UNKNOWN_ERROR;
            continue;
        }
    } while (!ret);

    if (closedir(dir)) {
        ALOGE("ERROR in closing CPF folder \"%s\": %s!", cpfConfigPath, strerror(errno));
        if (!ret) ret = EPERM;
    }

    return ret;
}

status_t CpfStore::findConfigWithDriverHelper(const String8& fileName, String8& cpfName, int& index)
{
    status_t ret = 0;

    for (int i = RegisteredDrivers.size(); i-- > 0; ) {
        if (fileName.find(RegisteredDrivers[i].mSensorName) < 0) {
            // Name of this registered driver was not found
            // from within CPF looking file name -> skip it
            continue;
        } else {
            // Since we are here, we do have a registered
            // driver whose name maps to this CPF file name
            if (index < 0) {
                // No previous CPF<>driver pairs
                index = i;
                cpfName = fileName;
            } else {
                if (index == i) {
                    // Multiple CPF files match the driver
                    // If there are cpf files for different products with the same sensor name,
                    // the files are distinguished by spId
                    // Let's check for the vendor_id, platform_family_id and product_line_id
                    String8 vendorPlatformProduct;
                    if ((PlatformData::createVendorPlatformProductName(vendorPlatformProduct) == 0) &&
                        (fileName.find(vendorPlatformProduct) >= 0)) {
                            cpfName = fileName;
                    }
                } else {
                    // We just got lost:
                    // Which is the correct sensor driver?
                    ALOGE("ERROR multiple driver candidates for CPF file \"%s\"!", fileName.string());
                    ret = ENOTUNIQ;
                }
            }
        }
    }

    return ret;
}

status_t CpfStore::initConf(CameraBlob& aiqConf)
{
    status_t ret = 0;

    // First, we load the correct configuration file.
    // It will be behind reference counted MemoryHeapBase
    // object "allConf", meaning that the memory will be
    // automatically freed when it is no longer being pointed at
    if ((ret = loadConf(aiqConf)))
        return ret;

    return ret;
}

status_t CpfStore::loadConf(CameraBlob& allConf)
{
    FILE *file;
    struct stat statCurrent;
    status_t ret = 0;

    ALOGD("Opening CPF file \"%s\"", mCpfPathName.string());
    file = fopen(mCpfPathName, "rb");
    if (!file) {
        ALOGE("ERROR in opening CPF file \"%s\": %s!", mCpfPathName.string(), strerror(errno));
        return NAME_NOT_FOUND;
    }

    do {
        int fileSize;
        if ((fseek(file, 0, SEEK_END) < 0) ||
            ((fileSize = ftell(file)) < 0) ||
            (fseek(file, 0, SEEK_SET) < 0)) {
            ALOGE("ERROR querying properties of CPF file \"%s\": %s!", mCpfPathName.string(), strerror(errno));
            ret = ESPIPE;
            break;
        }

        allConf = CameraBlob(fileSize);
        if (!allConf) {
            ALOGE("ERROR no memory in %s!", __func__);
            ret = NO_MEMORY;
            break;
        }

        if (fread(allConf, fileSize, 1, file) < 1) {
            ALOGE("ERROR reading CPF file \"%s\"!", mCpfPathName.string());
            ret = EIO;
            break;
        }

        // We use file statistics for file identification purposes.
        // The access time was just changed (because of us!),
        // so let's nullify the access time info
        if (stat(mCpfPathName, &statCurrent) < 0) {
            ALOGE("ERROR querying filestat of CPF file \"%s\": %s!", mCpfPathName.string(), strerror(errno));
            ret = FAILED_TRANSACTION;
            break;
        }
        statCurrent.st_atime = 0;
        statCurrent.st_atime_nsec = 0;

    } while (0);

    if (fclose(file)) {
        ALOGE("ERROR in closing CPF file \"%s\": %s!", mCpfPathName.string(), strerror(errno));
        if (!ret) ret = EPERM;
    }

    return ret;
}

} // namespace android
