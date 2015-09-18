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

#define LOG_TAG "ImgEncoder"
static const unsigned char JPEG_MARKER_EOI[2] = {0xFF, 0xD9};  //  JPEG EndOfImage marker

#include "ImgEncoder.h"
#include "LogHelper.h"
#include "Camera3GFXFormat.h"
#include "ImageScaler.h"
#include "Exif.h"

#define COMPARE_RESOLUTION(b1, b2) ( \
    ((b1->width() > b2->width()) || (b1->height() > b2->height())) ? 1 : \
    ((b1->width() == b2->width()) && (b1->height() == b2->height())) ? 0 : -1)

namespace android {
namespace camera2 {
ImgEncoder::ImgEncoder(int cameraid) :
    mHwCompressor(NULL)
    ,mSwCompressor(NULL)
    ,mThumbOutBuf(NULL)
    ,mJpegDataBuf(NULL)
    ,mMainScaled(NULL)
    ,mThumbScaled(NULL)
    ,mJpegSetting(NULL)
    ,mCameraId(cameraid)
{
    LOG2("@%s", __FUNCTION__);
}

ImgEncoder::~ImgEncoder()
{
    LOG2("@%s", __FUNCTION__);
    deInit();
}

status_t ImgEncoder::init(void)
{
    LOG1("@%s", __FUNCTION__);
    if (mHwCompressor == NULL) {
        mHwCompressor = new JpegHwEncoder();
        if (mHwCompressor == NULL)
            LOGE("ERROR: Can't create JpegHwEncoder!");
    }

    if (mSwCompressor == NULL) {
        mSwCompressor = new SWJpegEncoder();
        if (mSwCompressor == NULL)
            LOGE("ERROR: Can't create JpegSwEncoder!");
    }

    if (mHwCompressor == NULL || mSwCompressor == NULL) {
        deInit();
        return NO_MEMORY;
    }

    mJpegSetting = new JpegSettings;
    if (mJpegSetting == NULL) {
        LOGE("Error creating JpegSetting in init");
        deInit();
        return NO_INIT;
    }

    return NO_ERROR;
}

void ImgEncoder::deInit(void)
{
    LOG2("@%s", __FUNCTION__);

    if (mHwCompressor != NULL) {
        delete mHwCompressor;
        mHwCompressor = NULL;
    }
    if (mSwCompressor != NULL) {
        delete mSwCompressor;
        mSwCompressor = NULL;
    }
    if (mJpegSetting != NULL) {
        delete mJpegSetting;
        mJpegSetting = NULL;
    }
    if (mThumbOutBuf.get()) {
        mThumbOutBuf.clear();
    }
    if (mJpegDataBuf.get()) {
        mJpegDataBuf.clear();
    }
}

/**
 * thumbBufferDownScale
 * Downscaling the thumb buffer and allocate the scaled thumb input intermediate
 * buffer if scaling is needed.
 *
 * \param pkg [IN] EncodePackage from the caller
 */
void ImgEncoder::thumbBufferDownScale(EncodePackage & pkg)
{
    LOG2("%s", __FUNCTION__);
    int thumbwidth = mJpegSetting->thumbWidth;
    int thumbheight = mJpegSetting->thumbHeight;

    // Allocate thumbnail input buffer if downscaling is needed.
    if (thumbwidth != 0) {
        if (COMPARE_RESOLUTION(pkg.thumb, mThumbOutBuf) != 0) {
            LOG2("%s: Downscaling for thumbnail: %dx%d -> %dx%d", __FUNCTION__,
                pkg.thumb->width(), pkg.thumb->height(),
                mThumbOutBuf->width(), mThumbOutBuf->height());
            if (mThumbScaled.get()
                && (COMPARE_RESOLUTION(mThumbScaled, mThumbOutBuf) != 0)) {
                mThumbScaled.clear();
            }
            if (!mThumbScaled.get()) {
                // Using thumbwidth as stride for heap buffer
                mThumbScaled = MemoryUtils::allocateHeapBuffer(thumbwidth,
                                                               thumbheight,
                                                               thumbwidth,
                                                               pkg.thumb->v4l2Fmt(),
                                                               mCameraId);
            }
            ImageScaler::downScaleImage(pkg.thumb.get(), mThumbScaled.get(), 0, 0);
            pkg.thumb = mThumbScaled;
        }
    }
}

/**
 * mainBufferDownScale
 * Downscaling the main buffer and allocate the scaled main intermediate
 * buffer if scaling is needed.
 *
 * \param pkg [IN] EncodePackage from the caller
 */
void ImgEncoder::mainBufferDownScale(EncodePackage & pkg)
{
    LOG2("%s", __FUNCTION__);

    // Allocate buffer for main picture downscale
    // Compare the resolution, only do downscaling.
    if (COMPARE_RESOLUTION(pkg.main, pkg.jpegOut) == 1) {
        LOG2("%s: Downscaling for main picture: %dx%d -> %dx%d", __FUNCTION__,
            pkg.main->width(), pkg.main->height(),
            pkg.jpegOut->width(), pkg.jpegOut->height());
        if (mMainScaled.get()
            && (COMPARE_RESOLUTION(mMainScaled, pkg.jpegOut) != 0)) {
            mMainScaled.clear();
        }
        if (!mMainScaled.get()) {
            // Use pkg.jpegOut->width() as stride for the heap buffer
            mMainScaled = MemoryUtils::allocateHeapBuffer(pkg.jpegOut->width(),
                                                          pkg.jpegOut->height(),
                                                          pkg.jpegOut->width(),
                                                          pkg.main->v4l2Fmt(),
                                                          mCameraId);
        }
        LOG2("@%s: downscaling for picture: %dx%d -> %dx%d", __FUNCTION__,
            pkg.main->width(), pkg.main->height(),
            mMainScaled->width(), mMainScaled->height());
        ImageScaler::downScaleImage(pkg.main.get(), mMainScaled.get(), 0, 0);
        pkg.main = mMainScaled;
    }
}

/**
 * allocateBufferAndDownScale
 * This method downscales the main image and thumbnail if necesary. In case it
 * needs to scale, it allocates the intermediate buffers where the scaled version
 * is stored before it is given to the encoders. jpeg.thumbnailSize (0,0) means
 * JPEG EXIF will not contain a thumbnail. We use thumbwidth to determine if
 * Thumbnail size is > 0. In case thumbsize is not 0 we create the thumb output
 * buffer with size provided in the settings. If no thumb input buffer is
 * provided with the package the main buffer is assinged to the thumb input.
 * If thumb input buffer is provided in the package then only down scaling is needed
 *
 * \param pkg [IN] EncodePackage from the caller
 */
status_t ImgEncoder::allocateBufferAndDownScale(EncodePackage & pkg)
{
    LOG2("%s", __FUNCTION__);

    int thumbwidth = mJpegSetting->thumbWidth;
    int thumbheight = mJpegSetting->thumbHeight;

    // Allocate buffer for main image jpeg output if in first time or resolution changed
    if (mJpegDataBuf.get() == NULL || COMPARE_RESOLUTION(mJpegDataBuf, pkg.jpegOut)) {

        if (mJpegDataBuf.get() != NULL)
            mJpegDataBuf.clear();

        LOG1("Allocating jpeg data buffer with %dx%d, stride:%d", pkg.jpegOut->width(),
                pkg.jpegOut->height(), pkg.jpegOut->stride());
        mJpegDataBuf = MemoryUtils::allocateHeapBuffer(pkg.jpegOut->width(),
                                                    pkg.jpegOut->height(),
                                                    pkg.jpegOut->stride(),
                                                    pkg.jpegOut->v4l2Fmt(), mCameraId);
    }

    if (thumbwidth != 0 && !pkg.thumb.get()) {
        pkg.thumb = pkg.main;
    }
    // Allocate buffer for thumbnail output
    if (thumbwidth != 0) {
        if (mThumbOutBuf.get()
            && (mThumbOutBuf->width() != thumbwidth
            || mThumbOutBuf->height() != thumbheight))
            mThumbOutBuf.clear();
        if (!mThumbOutBuf.get()) {
            LOG1("Allocating thumb data buffer with %dx%d", thumbwidth, thumbheight);
            // Use thumbwidth as stride for the heap buffer
            mThumbOutBuf = MemoryUtils::allocateHeapBuffer(thumbwidth,
                                                           thumbheight,
                                                           thumbwidth,
                                                           pkg.thumb->v4l2Fmt(),
                                                           mCameraId);
        }
    }

    thumbBufferDownScale(pkg);
    mainBufferDownScale(pkg);

    return NO_ERROR;
}


/**
 * getJpegSettings
 *
 * Get the JPEG settings needed for image encoding from the Exif
 * metadata and store to internal struct
 * \param settings [IN] EncodePackage from the caller
 * \param metaData [IN] exif metadata
 */
status_t ImgEncoder::getJpegSettings(EncodePackage & pkg, sp<ExifMetaData> metaData)
{
    LOG2("@%s:", __FUNCTION__);
    UNUSED(pkg);
    status_t status = NO_ERROR;

    if (metaData == NULL) {
        LOGE("MetaData struct not intialized");
        return UNKNOWN_ERROR;
    }
    mJpegSetting->jpegQuality = metaData->mJpegSetting.jpegQuality;
    mJpegSetting->jpegThumbnailQuality = metaData->mJpegSetting.jpegThumbnailQuality;
    mJpegSetting->thumbWidth = metaData->mJpegSetting.thumbWidth;
    mJpegSetting->thumbHeight = metaData->mJpegSetting.thumbHeight;
    mJpegSetting->orientation = metaData->mJpegSetting.orientation;

    LOG1("jpegQuality=%d,thumbQuality=%d,thumbW=%d,thumbH=%d,orientation=%d",
              mJpegSetting->jpegQuality,
              mJpegSetting->jpegThumbnailQuality,
              mJpegSetting->thumbWidth,
              mJpegSetting->thumbHeight,
              mJpegSetting->orientation);


    return status;
}

/**
 * startHwEncoding
 *
 * Start asynchronously the HW encoder.
 * This may fail in that case we should failback to SW encoding
 *
 * \param srcBuf [IN] The input buffer to encode
 *
 */
status_t ImgEncoder::startHwEncoding(sp<CameraBuffer> srcBuf,
                                     int jpegQuality)
{
    status_t status = NO_ERROR;
    JpegHwEncoder::InputBuffer *inputBufferArray = NULL;
    JpegHwEncoder::InputBuffer input;
    JpegHwEncoder::OutputBuffer output;
    int inputBufferNum = 0;
    int mMaxCodedSize = 0;
    if (mHwCompressor == NULL) {
        LOGE("Hardware Jpeg encoder component is not exist");
        return BAD_VALUE;
    }
    // suppose there is just one input buffer in camera v3
    inputBufferNum = 1;
    inputBufferArray = new JpegHwEncoder::InputBuffer[inputBufferNum];
    inputBufferArray[0].width = srcBuf->width();
    inputBufferArray[0].height = srcBuf->height();
    inputBufferArray[0].fourcc = srcBuf->v4l2Fmt();
    inputBufferArray[0].bpl = pixelsToBytes(inputBufferArray[0].fourcc,
                            widthToStride(srcBuf->v4l2Fmt(), srcBuf->width()));  // caculate the bpl
    inputBufferArray[0].buf = (unsigned char*)srcBuf->data();
    status = mHwCompressor->setInputBuffers(inputBufferArray, inputBufferNum);
    if (status != NO_ERROR) {
        delete []inputBufferArray;
        return status;
    }
    input = inputBufferArray[0];
    output.clear();
    output.quality = jpegQuality;
    status = mHwCompressor->encodeAsync(input, output, mMaxCodedSize);
    delete []inputBufferArray;
    inputBufferArray = NULL;
    return status;
}

status_t ImgEncoder::completeHwEncode(sp<CameraBuffer> destBuf,
                                        unsigned int destOffset)
{
    LOG2("@%s", __FUNCTION__);
    status_t status= NO_ERROR;
    unsigned int finalSize = 0;
    unsigned int mainSize = 0;
    unsigned char* dstPtr = NULL;

    nsecs_t endTime = systemTime();

    // get JPEG size
    if (mHwCompressor->getOutputSize(mainSize) < 0) {
        LOGE("Could not get Coded size!");
        return UNKNOWN_ERROR;
    }
    if (mainSize <= 0) {
        LOGE("HW JPEG Encoding failed to complete!");
        return UNKNOWN_ERROR;
    }
    finalSize = destOffset + mainSize;
    if (destBuf->data() == NULL) {
        LOGE("No memory for final JPEG file!");
        status = NO_MEMORY;
    } else {
        // get the JPEG data
        dstPtr = (unsigned char*)destBuf->data() + destOffset;
        if (mHwCompressor->getOutput(dstPtr, mainSize) < 0) {
            LOGE("Could not encode picture stream!");
            return UNKNOWN_ERROR;
        } else {
            char *copyTo = reinterpret_cast<char*>(destBuf->data()) \
                           + finalSize - sizeof(JPEG_MARKER_EOI);
            memcpy(copyTo, (void*)(JPEG_MARKER_EOI),
                   sizeof(JPEG_MARKER_EOI));
        }
    }
    LOG1("Picture JPEG size: %d (waited for encode to finish: %ums)", mainSize,
         (unsigned)((systemTime() - endTime) / 1000000));
    return mainSize;
}

int ImgEncoder::doSwEncode(sp<CameraBuffer> srcBuf,
                               int quality,
                               sp<CameraBuffer> destBuf,
                               unsigned int destOffset)
{
    LOG2("@%s", __FUNCTION__);

    SWJpegEncoder::InputBuffer inBuf;
    SWJpegEncoder::OutputBuffer outBuf;

    inBuf.buf = (unsigned char*)srcBuf->data();
    inBuf.width = srcBuf->width();
    inBuf.height = srcBuf->height();
    inBuf.stride = widthToStride(srcBuf->v4l2Fmt(), srcBuf->width());
    inBuf.fourcc = srcBuf->v4l2Fmt();
    inBuf.size = srcBuf->size();

    outBuf.buf = (unsigned char*)destBuf->data() + destOffset;
    outBuf.width = destBuf->width();
    outBuf.height = destBuf->height();
    outBuf.quality = quality;
    outBuf.size = destBuf->size();

    nsecs_t startTime = systemTime();
    int size = mSwCompressor->encode(inBuf, outBuf);
    LOG1("%s: encoding %dx%d need %ums, jpeg size %d, quality %d)", __FUNCTION__,
         destBuf->width(), destBuf->height(),
        (unsigned)((systemTime() - startTime) / 1000000), size, quality);
    return size;
}

/**
 * encodeSync
 *
 * Do HW or SW encoding of the main buffer of the package
 * Also do SW encoding of the thumb buffer
 *
 * \param srcBuf [IN] The input buffer to encode
 * \param metaData [IN] exif metadata
 *
 */
status_t ImgEncoder::encodeSync(EncodePackage & package, sp<ExifMetaData> metaData)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    bool hwEncodeSuccess = false;
    unsigned int mainSize = 0;
    int thumbSize = 0;

    if (package.main == NULL) {
        LOGE("Main buffer for JPEG encoding is NULL");
        return UNKNOWN_ERROR;
    }

    if (package.jpegOut == NULL) {
        LOGE("JPEG output buffer is NULL");
        return UNKNOWN_ERROR;
    }

    getJpegSettings(package, metaData);
    // Allocate buffers for thumbnail if not present and also
    // downscale the buffer for main if scaling is needed
    allocateBufferAndDownScale(package);

    // Start encoding main picture using HW encoder
    status = startHwEncoding(package.main, mJpegSetting->jpegQuality);
    if (status == NO_ERROR)
        hwEncodeSuccess = true;

    // Encode thumbnail as JPEG in parallel with the HW encoding started earlier
    if (package.thumb.get() && mThumbOutBuf.get()) {
        do {
            LOG2("Encoding thumbnail with quality %d",
                 mJpegSetting->jpegThumbnailQuality);
            thumbSize = doSwEncode(package.thumb,
                                   mJpegSetting->jpegThumbnailQuality,
                                   mThumbOutBuf);
            mJpegSetting->jpegThumbnailQuality -= 5;
        } while (thumbSize > 0 && mJpegSetting->jpegThumbnailQuality > 0 &&
                thumbSize > THUMBNAIL_SIZE_LIMITATION);

        if (thumbSize > 0) {
            package.thumbOut = mThumbOutBuf;
            package.thumbSize = thumbSize;
        } else {
            // This is not critical, we can continue with main picture image
            LOGW("Could not encode thumbnail stream!");
        }
    } else {
        // No thumb is not critical, we can continue with main picture image
        LOGW("Exif created without thumbnail stream!");
    }

    if (hwEncodeSuccess) {
        mainSize = completeHwEncode(mJpegDataBuf);
    } else {
        // Encode main picture with SW encoder
        mainSize = doSwEncode(package.main,
                              mJpegSetting->jpegQuality,
                              mJpegDataBuf);
        if (!mainSize) {
           LOGE("Error while SW encoding JPEG");
           status = INVALID_OPERATION;
        }
    }

    if (mainSize) {
        package.encodedData = mJpegDataBuf;
        package.encodedDataSize = mainSize;
    }

    return status;
}

};  // namespace camera2
};  // namespace android
