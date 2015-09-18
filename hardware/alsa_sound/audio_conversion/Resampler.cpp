/*
 **
 ** Copyright 2013 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **      http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#define LOG_TAG "Resampler"

#include "Resampler.h"
#include <cutils/log.h>
#include <iasrc_resampler.h>
#include <limits.h>

#define base AudioConverter

using namespace android;

namespace android_audio_legacy{

Resampler::Resampler(SampleSpecItem sampleSpecItem) :
    base(sampleSpecItem),
    _maxFrameCnt(0),
    _context(NULL), _floatInp(NULL), _floatOut(NULL)
{
}

Resampler::~Resampler()
{
    if (_context) {
        iaresamplib_reset(_context);
        iaresamplib_delete(&_context);
    }

    delete []_floatInp;
    delete []_floatOut;
}

status_t Resampler::allocateBuffer()
{
    if (_maxFrameCnt == 0) {
        _maxFrameCnt = BUF_SIZE;
    } else {
        _maxFrameCnt *= 2; // simply double the buf size
    }

    delete []_floatInp;
    delete []_floatOut;

    _floatInp = new float[(_maxFrameCnt + 1) * _ssSrc.getChannelCount()];
    _floatOut = new float[(_maxFrameCnt + 1) * _ssSrc.getChannelCount()];

    if (!_floatInp || !_floatOut) {

        LOGE("cannot allocate resampler tmp buffers.\n");
        delete []_floatInp;
        delete []_floatOut;

        return NO_MEMORY;
    }
    return NO_ERROR;
}

status_t Resampler::configure(const SampleSpec &ssSrc, const SampleSpec &ssDst)
{
    ALOGD("%s: SOURCE rate=%d format=%d channels=%d",  __FUNCTION__, ssSrc.getSampleRate(),
          ssSrc.getFormat(), ssSrc.getChannelCount());
    ALOGD("%s: DST rate=%d format=%d channels=%d", __FUNCTION__, ssDst.getSampleRate(),
          ssDst.getFormat(), ssDst.getChannelCount());

    if (ssSrc.getSampleRate() == _ssSrc.getSampleRate() &&
        ssDst.getSampleRate() == _ssDst.getSampleRate() && _context) {

        return NO_ERROR;
    }

    status_t status = base::configure(ssSrc, ssDst);
    if (status != NO_ERROR) {

        return status;
    }

    if (_context) {
        iaresamplib_reset(_context);
        iaresamplib_delete(&_context);
        _context = NULL;
    }

    if (!iaresamplib_supported_conversion(ssSrc.getSampleRate(), ssDst.getSampleRate())) {

        ALOGE("%s: SRC lib doesn't support this conversion", __FUNCTION__);
        return INVALID_OPERATION;
    }

    iaresamplib_new(&_context, ssSrc.getChannelCount(),
                    ssSrc.getSampleRate(), ssDst.getSampleRate());
    if (!_context) {
        ALOGE("cannot create resampler handle for lacking of memory.\n");
        return BAD_VALUE;
    }

    _convertSamplesFct = static_cast<SampleConverter>(&Resampler::resampleFrames);
    return NO_ERROR;
}

void Resampler::convertShort2Float(int16_t *inp, float *out, size_t sz) const
{
    size_t i;
    for (i = 0; i < sz; i++) {
        *out++ = (float) *inp++;
    }
}

void Resampler::convertFloat2Short(float *inp, int16_t *out, size_t sz) const
{
    size_t i;
    for (i = 0; i < sz; i++) {
        if (*inp > SHRT_MAX) {
            *inp = SHRT_MAX;
        } else if (*inp < SHRT_MIN) {
            *inp = SHRT_MIN;
        }
        *out++ = (short) *inp++;
    }
}

status_t Resampler::resampleFrames(const void *src,
                                    void *dst,
                                    const uint32_t inFrames,
                                    uint32_t *outFrames)
{
    size_t outFrameCount = convertSrcToDstInFrames(inFrames);

    while (outFrameCount > _maxFrameCnt) {

        status_t ret = allocateBuffer();
        if (ret != NO_ERROR) {

            ALOGE("%s: could not allocate memory for resampling operation", __FUNCTION__);
            return ret;
        }
    }
    unsigned int outNbFrames;
    convertShort2Float((short *)src, _floatInp, inFrames * _ssSrc.getChannelCount());
    iaresamplib_process_float(_context, _floatInp, inFrames, _floatOut, &outNbFrames);
    convertFloat2Short(_floatOut, (short *)dst, outNbFrames * _ssSrc.getChannelCount());

    *outFrames = outNbFrames;

    return NO_ERROR;
}

}; // namespace android
