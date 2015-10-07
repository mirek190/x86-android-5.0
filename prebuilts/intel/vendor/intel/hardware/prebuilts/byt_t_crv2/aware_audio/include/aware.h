/*************************************************************************************
 * INTEL CONFIDENTIAL
 * Copyright 2011 Intel Corporation All Rights Reserved.
 * The source code contained or described herein and all documents related
 * to the source code ("Material") are owned by Intel Corporation or its
 * suppliers or licensors. Title to the Material remains with Intel
 * Corporation or its suppliers and licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * suppliers and licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel’s prior
 * express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 ************************************************************************************/


#ifndef ANDROID_AWARE_HAL_INTERFACE_H
#define ANDROID_AWARE_HAL_INTERFACE_H

#include <hardware/hardware.h>

__BEGIN_DECLS

/**
 * The id of this module
 */
#define AWARE_HARDWARE_MODULE_ID "aware"

/**
 * Name of the aware devices to open
 */
#define AWARE_HARDWARE_INTERFACE "aware_hw_if"


/* Use version 0.1 to be compatible with first generation of audio hw module with version_major
 * hardcoded to 1. No aware module API change.
 */
#define AWARE_MODULE_API_VERSION_0_1 HARDWARE_MODULE_API_VERSION(0, 1)
#define AWARE_MODULE_API_VERSION_CURRENT AWARE_MODULE_API_VERSION_0_1

/* First generation of audio devices had version hardcoded to 0. all devices with versions < 1.0
 * will be considered of first generation API.
 */
#define AWARE_DEVICE_API_VERSION_0_0 HARDWARE_DEVICE_API_VERSION(0, 0)
#define AWARE_DEVICE_API_VERSION_1_0 HARDWARE_DEVICE_API_VERSION(1, 0)
#define AWARE_DEVICE_API_VERSION_2_0 HARDWARE_DEVICE_API_VERSION(2, 0)
#define AWARE_DEVICE_API_VERSION_CURRENT AWARE_DEVICE_API_VERSION_2_0

/**
 * List of known aware HAL modules. This is the base name of the aware HAL
 * library composed of the "aware." prefix, one of the base names below and
 * a suffix specific to the device.
 * e.g: aware.primary.goldfish.so or audio.a2dp.default.so
 */

#define AWARE_HARDWARE_MODULE_ID_PRIMARY "primary"

using namespace android;

/**************************************/

/**
 *  standard aware parameters that the HAL may need to handle
 */

/**
 *  aware device parameters
 */

/**************************************/

/* aware classifier parameters */
struct aware_classifier_param {
    int32_t framesize;                  /* Framesize for feature extraction */
    int16_t mfccorder;                  /* mfcc order */
    int16_t gmmorder;                   /* gmm order */
    int16_t fborder;                    /* filterbank order */
    int32_t minNumFrms;                 /* min num frames parameter used by classifier */
    int32_t thresholdSpeech;            /* threshold for speech detection */
    int32_t thresholdNonspeech;         /* threshold for non speech detection */
    int32_t dB_offset;                  /* db offset */
    int32_t alpha;                      /* smoothing parameter alpha */
    int32_t rastacoeff_a;               /* rasta filter coefficient a */
    int32_t rastacoeff_b0;              /* rasta/delta filter coefficient b0 */
    int32_t rastacoeff_b1;              /* rasta/delta filter coefficient b1 */
    int16_t numframesperblock;          /* number of frames per  block */
    int16_t numblocksperanalysis;       /* number of blocks per analysis window */
};

typedef struct aware_classifier_param aware_classifier_param_t;


/**********************************************************************/

/**
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */
struct aware_module {
    struct hw_module_t common;
};

struct aware_hw_device {
    struct hw_device_t common;

    /** Activate the aware session */
    status_t (*activate_aware_session)(hw_device_t *device, aware_classifier_param_t *params);

    /** Deactivate the aware session */
    status_t (*deactivate_aware_session)(hw_device_t *device);

    /** Lock for the device */
    pthread_mutex_t lock;
};

typedef struct aware_hw_device aware_hw_device_t;


/** convenience API for opening and closing a supported device */

static inline status_t aware_hw_device_open(const struct hw_module_t* module,
                                       struct aware_hw_device** device)
{
    return module->methods->open(module, AWARE_HARDWARE_INTERFACE,
                                 (struct hw_device_t**)device);
}

static inline status_t aware_hw_device_close(struct aware_hw_device* device)
{
    if (device != NULL) {
        return device->common.close(&device->common);
    } else {
        return BAD_VALUE;
    }
}


__END_DECLS

#endif  // ANDROID_AWARE_INTERFACE_H
