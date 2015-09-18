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

#ifndef _CAMERA3_HAL_COMMON_MACROS_H_
#define _CAMERA3_HAL_COMMON_MACROS_H_

#define CAMERA_OPERATION_FOLDER "/data/misc/media/"

/**
 * \macro CLIP
 * Used to clip the Number value to between the Min and Max
 */
#define CLIP(Number, Max, Min)    \
        ((Number) > (Max) ? (Max) : ((Number) < (Min) ? (Min) : (Number)))

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define PAGE_ALIGN(x) ((x + 0xfff) & 0xfffff000)

/**
 *  \macro TIMEVAL2USECS
 *  Convert timeval struct to value in microseconds
 *  Helper macro to convert timeval struct to microsecond values stored in a
 *  long long signed value (equivalent to int64_t)
 */
#define TIMEVAL2USECS(x) (int64_t)(((x)->tv_sec*1000000000LL + \
                                    (x)->tv_usec*1000LL)/1000LL)

/**
 * \macro TIMEVAL2NSECS
 *  Convert timeval struct to value in nanoseconds
 *  Helper macro to convert timeval struct to nanosecond values stored in a
 *  long long signed value (equivalent to int64_t)
 */
#define TIMEVAL2NSECS(x) (int64_t)(((x)->tv_sec*1000000000LL + \
                                    (x)->tv_usec*1000LL))
/**
 * \macro UNUSED
 *  applied to parameters not used in a method in order to avoid the compiler
 *  warning
 */
#define UNUSED(x) (void)(x)

#define ALIGN8(x) (((x) + 7) & ~7)
#define ALIGN16(x) (((x) + 15) & ~15)
#define ALIGN32(x) (((x) + 31) & ~31)
#define ALIGN64(x) (((x) + 63) & ~63)
#define ALIGN128(x) (((x) + 127) & ~127)

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#ifdef __GNUC__
#define INDEXED_FIELD_INITIALIZER(x) [x]=
#define NAMED_FIELD_INITIALIZER(x) .x=
#else
#define INDEXED_FIELD_INITIALIZER(x)
#define NAMED_FIELD_INITIALIZER(x)
#endif

#define COMPARE_RESOLUTION(b1, b2) ( \
    ((b1->width() > b2->width()) || (b1->height() > b2->height())) ? 1 : \
    ((b1->width() == b2->width()) && (b1->height() == b2->height())) ? 0 : -1)

/**
 * \macro IS_SAME_RESOLUTION_RATIO
 *  if (w1, h1) has the same resoltuion ratio with (w2, h2), the function will return true.
 */
#define IS_SAME_RESOLUTION_RATIO(w1, h1, w2, h2) ( \
    (fabs(((float)(w1) / (float)(h1)) / ((float)(w2) / (float)(h2)) - 1) < 0.01f) ? true : false)

#endif /* _CAMERA3_HAL_COMMON_MACROS_H_ */
