/*
 * INTEL CONFIDENTIAL
 * Copyright  2013 Intel
 * Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material contains trade secrets and proprietary
 * and confidential information of Intel or its suppliers and licensors. The
 * Material is protected by worldwide copyright and trade secret laws and
 * treaty provisions. No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or
 * disclosed in any way without Intels prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */
#pragma once

#include "Interface.h"

struct IModemAudioControllerInterface : public NInterfaceProvider::IInterface
{
    enum StreamType {
        EUplink,
        EDownlink,

        ENBStreamTypes
    };

    INTERFACE_NAME("ModemAudioControllerInterface");

    virtual bool enableAudioPort(bool bEnabled) = 0;
    virtual void enableTTY(StreamType eStreamType, bool bEnabled) = 0;
    virtual void setVolume(uint32_t uiVolume) = 0;
    virtual void mute(StreamType eStreamType, bool bMuted) = 0;
    virtual void setAccessory(uint32_t uiAccessory) = 0;
};

