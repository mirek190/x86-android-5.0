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

#ifndef __SHARE_BUFFER_H__
#define __SHARE_BUFFER_H__
#include <utils/RefBase.h>
#include <utils/threads.h>

#define SHARE_PTR_COUNT_MAX 16
#define SHARE_PTR_ALIGNMENT 128
#define SHARE_PTR_ALIGN(width) ((((width) + SHARE_PTR_ALIGNMENT - 1) / SHARE_PTR_ALIGNMENT) * SHARE_PTR_ALIGNMENT)


typedef struct {
    unsigned char *pointer;
    int width;
    int height;
    int stride;
    int allocatedSize;
    int dataSize;
} SharedBufferType;

typedef enum {
    BS_SUCCESS = 0,
    BS_INVALID_STATE,
    BS_INVALID_PARAMETER,
    BS_PEER_DOWN,
    BS_UNKNOWN,
} BufferShareStatus;

namespace android {
typedef enum {
    BS_LOADED = 0,
    BS_SHARE_SETTING,
    BS_SHARE_SET,
    BS_SHARE_UNSETTING,
    BS_SHARE_UNSET,
    BS_INVALID,
} BufferShareState;

static const char* StatusStringTable[] = {
    "BS_SUCCESS",
    "BS_INVALID_STATE",
    "BS_INVALID_PARAMETER",
    "BS_PEER_DOWN",
    "BS_UNKNOWN",
};

static const char* StateStringTable[] = {
    "BS_LOADED",
    "BS_SHARE_SETTING",
    "BS_SHARE_SET",
    "BS_SHARE_UNSETTING",
    "BS_SHARE_UNSET",
    "BS_INVALID",
};

class BufferShareRegistry : public RefBase {

    /*singleton control methods*/
public:
    static sp<BufferShareRegistry> getInstance(void);

private:
    BufferShareRegistry();
    ~BufferShareRegistry();

    /*state machine control methods*/
private:
    BufferShareStatus transitToState(BufferShareState target_state);
    void onStateTransitionCompleted(BufferShareState target_state);
    void initStateMachine(void);
    BufferShareStatus enableSharingMode(void);
    BufferShareStatus disableSharingMode(void);

    /*debug helpers*/
private:
    const char* statusToString(BufferShareStatus status);
    const char* stateToString(BufferShareState state);

    /*external functional interfaces*/
public:
    BufferShareStatus sourceGetSharedBuffer(SharedBufferType *p_buffer_info, int *p_buffer_cnt);
    BufferShareStatus sourceEnterSharingMode(void);
    BufferShareStatus sourceExitSharingMode(void);

    BufferShareStatus sourceRequestToEnableSharingMode(void);
    BufferShareStatus sourceRequestToDisableSharingMode(void);

    bool isBufferSharingModeSet(void);
    bool isBufferSharingModeEnabled(void);

    BufferShareStatus encoderSetSharedBuffer(SharedBufferType *p_buffer_info, int buffer_cnt);
    BufferShareStatus encoderEnterSharingMode(void);
    BufferShareStatus encoderExitSharingMode(void);

    BufferShareStatus encoderRequestToEnableSharingMode(void);
    BufferShareStatus encoderRequestToDisableSharingMode(void);

    /*singleton control data*/
private:
    static Mutex sRegistryLock;
    static sp<BufferShareRegistry> sInstance;

    /*state machine control data*/
private:
    Mutex *mStateLock;
    Condition *mStateCond;
    volatile BufferShareState mState;
    bool isSourceSharingModeEnabled;
    bool isEncoderSharingModeEnabled;
    /*share buffer registry*/
private:
    int sharedBufferCount;
    SharedBufferType sharedBufferInfo[SHARE_PTR_COUNT_MAX];
    bool mEncoderHasEntered;
    bool mSourceHasEntered;
};

/* dynamic loading interface */
extern "C" {
    sp<BufferShareRegistry>  GetRegistryInstance(void);
}

}
#endif
