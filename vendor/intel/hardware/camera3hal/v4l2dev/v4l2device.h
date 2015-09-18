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

#ifndef _CAMERA3_HAL_V4L2DEVICE_H_
#define _CAMERA3_HAL_V4L2DEVICE_H_

#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/Mutex.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include "PerformanceTraces.h"
#include "HalTypes.h"

#ifdef LIBCAMERA_RD_FEATURES
#define pioctl(fd, ctrlId, attr) \
({ \
    int reti; \
    if (gPerfLevel & CAMERA_DEBUG_LOG_PERF_IOCTL_BREAKDOWN) { \
        PERFORMANCE_TRACES_IO_BREAKDOWN(#ctrlId); \
        reti = ioctl(fd, ctrlId, attr); \
    } else { \
        reti = ioctl(fd, ctrlId, attr); \
    } \
    reti; \
})

#define popen(name, attr) \
({ \
    int fd; \
    if (gPerfLevel & CAMERA_DEBUG_LOG_PERF_IOCTL_BREAKDOWN) { \
        PERFORMANCE_TRACES_IO_BREAKDOWN("open"); \
        fd = ::open(name, attr); \
    } else { \
        fd = ::open(name, attr); \
    } \
    fd; \
})

#define pclose(fd) \
({ \
    int ret; \
    if (gPerfLevel & CAMERA_DEBUG_LOG_PERF_IOCTL_BREAKDOWN) { \
        PERFORMANCE_TRACES_IO_BREAKDOWN("Close"); \
        ret = ::close(fd); \
    } else { \
        ret = ::close(fd); \
    } \
    ret; \
})

#define pxioctl(device, ctrlId, attr) \
({ \
    int reti; \
    if (gPerfLevel & CAMERA_DEBUG_LOG_PERF_IOCTL_BREAKDOWN) { \
        PERFORMANCE_TRACES_IO_BREAKDOWN(#ctrlId); \
        reti = device->xioctl(ctrlId, attr); \
    } else { \
        reti = device->xioctl(ctrlId, attr); \
    } \
    reti; \
})

#define pbxioctl(ctrlId, attr) \
({ \
    int reti; \
    if (gPerfLevel & CAMERA_DEBUG_LOG_PERF_IOCTL_BREAKDOWN) { \
        PERFORMANCE_TRACES_IO_BREAKDOWN(#ctrlId); \
        reti = V4L2DeviceBase::xioctl(ctrlId, attr); \
    } else { \
        reti = V4L2DeviceBase::xioctl(ctrlId, attr); \
    } \
    reti; \
 })

#define perfpoll(fd, value, timeout) \
({ \
    int reti; \
    if (gPerfLevel & CAMERA_DEBUG_LOG_PERF_IOCTL_BREAKDOWN) { \
        PERFORMANCE_TRACES_IO_BREAKDOWN("poll"); \
        reti = ::poll(fd, value, timeout); \
    } else { \
        reti = ::poll(fd, value, timeout); \
    } \
    reti; \
})

#else // LIBCAMERA_RD_FEATURES

#define pioctl(fd, ctrlId, attr) \
    ioctl(fd, ctrlId, attr)

#define popen(name, attr) \
    ::open(name, attr)

#define pclose(fd) \
    ::close(fd)

#define pxioctl(device, ctrlId, attr) \
    device->xioctl(ctrlId, attr)

#define pbxioctl(ctrlId, attr) \
    V4L2DeviceBase::xioctl(ctrlId, attr)

#define perfpoll(fd, value, timeout) \
    ::poll(fd, value, timeout)
#endif // LIBCAMERA_RD_FEATURES
namespace android {
namespace camera2 {

/**
 * v4l2 buffer descriptor.
 *
 * This information is stored in the pool
 */
struct v4l2_buffer_info {
    void *data;
    size_t length;
    int width;
    int height;
    int format;
    int cache_flags;        /*!< initial flags used when creating buffers */
    struct v4l2_buffer vbuffer;
};

/**
 * Video node types
 */
enum VideoNodeType {
    VIDEO_GENERIC,
    VIDEO_MIPI_COMPRESSED,
    VIDEO_MIPI_UNCOMPRESSED,
    VIDEO_RAW_BAYER,
    VIDEO_RAW_BAYER_SCALED,
    AA_STATS,
    AF_STATS,
    AE_HISTOGRAM
};

/**
 * A class encapsulating simple V4L2 device operations
 *
 * Base class that contains common V4L2 operations used by video nodes and
 * subdevices. It provides a slightly higher interface than IOCTLS to access
 * the devices. It also stores:
 * - State of the device to protect from invalid usage sequence
 * - Name
 * - File descriptor
 */
class V4L2DeviceBase: public RefBase {
public:
    V4L2DeviceBase(const char *name);
    virtual ~V4L2DeviceBase();

    virtual status_t open();
    virtual status_t close();

    virtual int xioctl(int request, void *arg) const;
    virtual int poll(int timeout);

    virtual status_t setControl(int aControlNum, const int value, const char *name);
    virtual status_t getControl(int aControlNum, int *value);
    virtual status_t queryMenu(v4l2_querymenu &menu);
    virtual status_t queryControl(v4l2_queryctrl &control);

    virtual int subscribeEvent(int event);
    virtual int unsubscribeEvent(int event);
    virtual int dequeueEvent(struct v4l2_event *event);

    bool isOpen() { return mFd != -1; }
    int getFd() { return mFd; }

    const char* name() { return mName.string(); }

    static int pollDevices(const Vector<sp<V4L2DeviceBase> > &devices,
                           Vector<sp<V4L2DeviceBase> > &activeDevices,
                           Vector<sp<V4L2DeviceBase> > &inactiveDevices,
                           int timeOut, int flushFd = -1);

protected:
    String8      mName;     /*!< path to device in file system, ex: /dev/video0 */
    int          mFd;       /*!< file descriptor obtained when device is open */

};


/**
 * A class encapsulating simple V4L2 video device node operations
 *
 * This class extends V4L2DeviceBase and adds extra internal state
 * and more convenience methods to  manage an associated buffer pool
 * with the device.
 * This class introduces new methods specifics to control video device nodes
 *
 */
class V4L2VideoNode: public V4L2DeviceBase {
public:
    enum VideNodeDirection {
        INPUT_VIDEO_NODE,   /*!< input video devices like cameras or capture cards */
        OUTPUT_VIDEO_NODE  /*!< output video devices like displays */
    };

public:
    V4L2VideoNode(const char *name,
                  VideNodeDirection nodeDirection = INPUT_VIDEO_NODE,
                  VideoNodeType nodeType = VIDEO_GENERIC);
    virtual ~V4L2VideoNode();

    virtual status_t open();
    virtual status_t close();

    virtual status_t setInput(int index);
    virtual status_t queryCap(struct v4l2_capability *cap);
    virtual status_t enumerateInputs(struct v4l2_input *anInput);

    // Video node configuration
    virtual void setInputBufferType(enum v4l2_buf_type v4l2BufType);
    virtual void setOutputBufferType(enum v4l2_buf_type v4l2BufType);
    virtual int getFramerate(float * framerate, int width, int height, int pix_fmt);
    virtual status_t setParameter (struct v4l2_streamparm *aParam);
    virtual status_t setFormat(FrameInfo &aConfig);
    virtual status_t setFormat(struct v4l2_format &aFormat);
    virtual status_t queryCapturePixelFormats(Vector<v4l2_fmtdesc> &formats);

    // Buffer pool management -- DEPRECATED!
    virtual status_t setBufferPool(void **pool, unsigned int poolSize, FrameInfo *aFrameInfo, bool cached);
    virtual void destroyBufferPool();
    virtual int createBufferPool(unsigned int buffer_count);

     // New Buffer pool management
    virtual status_t setBufferPool(Vector<struct v4l2_buffer> &pool,
                                   bool cached,
                                   int memType = V4L2_MEMORY_USERPTR);

    // Buffer flow control
    virtual int stop(bool keepBuffers = false);
    virtual int start(int initialSkips);

    virtual int grabFrame(struct v4l2_buffer_info *buf);
    virtual int putFrame(struct v4l2_buffer const *buf);
    virtual int putFrame(unsigned int index);

    // Convenience accessors
    virtual bool isStarted() const { return mState == DEVICE_STARTED; }
    virtual unsigned int getFrameCount() const { return mFrameCounter; }
    virtual unsigned int getBufsInDeviceCount() const { return mBuffersInDevice; }
    virtual unsigned int getInitialFrameSkips() const { return mInitialSkips; }
    virtual VideoNodeType getType() const { return mType; }
    virtual void setType(VideoNodeType type) { mType = type; }
    virtual void getConfig(FrameInfo &config) const { config = mConfig; }
protected:
    virtual int qbuf(struct v4l2_buffer_info *buf);
    virtual int dqbuf(struct v4l2_buffer_info *buf);
    virtual int newBuffer(int index, struct v4l2_buffer_info &buf,
                          int memType = V4L2_MEMORY_USERPTR);
    virtual int freeBuffer(struct v4l2_buffer_info *buf_info);
    virtual int requestBuffers(size_t num_buffers,
                               int memType = V4L2_MEMORY_USERPTR);

protected:

    enum VideoNodeState  {
            DEVICE_CLOSED = 0,  /*!< kernel device closed */
            DEVICE_OPEN,        /*!< device node opened */
            DEVICE_CONFIGURED,  /*!< device format set, IOC_S_FMT */
            DEVICE_PREPARED,    /*!< device has requested buffers (set_buffer_pool)*/
            DEVICE_STARTED,     /*!< stream started, IOC_STREAMON */
            DEVICE_ERROR        /*!< undefined state */
        };

    VideoNodeState  mState;
    // Device capture configuration
    FrameInfo mConfig;

    volatile int32_t mBuffersInDevice;      /*!< Tracks the amount of buffers inside the driver. Goes from 0 to the size of the pool*/
    unsigned int mFrameCounter;             /*!< Tracks the number of output buffers produced by the device. Running counter. It is reset when we start the device*/
    unsigned int mInitialSkips;

    Vector<struct v4l2_buffer_info> mSetBufferPool; /*!< DEPRECATED:This is the buffer pool set before the device is prepared*/
    Vector<struct v4l2_buffer_info> mBufferPool;    /*!< This is the active buffer pool */

    VideNodeDirection mDirection;
    VideoNodeType     mType;
    enum v4l2_buf_type mInBufType;
    enum v4l2_buf_type mOutBufType;
};

/**
 * A class encapsulating simple V4L2 sub device node operations
 *
 * Sub-devices are control points to the new media controller architecture
 * in V4L2
 *
 */
class V4L2Subdevice: public V4L2DeviceBase {
public:
    V4L2Subdevice(const char *name);
    virtual ~V4L2Subdevice();

    virtual status_t open();
    virtual status_t close();

    status_t queryFormats(int pad, Vector<uint32_t> &formats);
    status_t setFormat(int pad, int width, int height, int formatCode);
    status_t setSelection(int pad, int target, int top, int left, int width, int height);
    status_t getPadFormat(int padIndex, int &width, int &height, int &code);

private:
    status_t setFormat(struct v4l2_subdev_format &aFormat);
    status_t getFormat(struct v4l2_subdev_format &aFormat);
    status_t setSelection(struct v4l2_subdev_selection &aSelection);
    status_t getSelection(struct v4l2_subdev_selection &aSelection);

private:
    enum SubdevState  {
            DEVICE_CLOSED = 0,  /*!< kernel device closed */
            DEVICE_OPEN,        /*!< device node opened */
            DEVICE_CONFIGURED,  /*!< device format set, IOC_S_FMT */
            DEVICE_ERROR        /*!< undefined state */
        };

    SubdevState     mState;
    FrameInfo       mConfig;
};

} // namespace camera2
} // namespace android
#endif // _CAMERA3_HAL_V4L2DEVICE_H_
