/*
 * INTEL CONFIDENTIAL
 * Copyright  2013-2014 Intel
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
#include <AudioBand.h>
#include <AudioCommsAssert.hpp>

struct IModemAudioManagerObserver;

struct IModemAudioManagerInterface : public NInterfaceProvider::IInterface
{
    INTERFACE_NAME("ModemAudioManagerInterface");

    enum AudioStatus
    {
        AudioDetach = 0, /**< The modem audio is not enabled */
        AudioActive,     /**< The modem audio is active but not speech */
        VoiceActive      /**< The modem audio is active and is speech */
    };

    /**
     * Set the observer of the ModemAudioManager which will be notified for any state change on
     * the ModemAudioManager.
     * @warning only one observer is allowed per ModemAudioManager
     * @param[in] observer the observer to be notified
     */
    virtual void setObserver(IModemAudioManagerObserver *observer) = 0;

    /**
     * @return true if the modem is alive (powered on and initialized) of false otherwise (modem
     *              state is unknown)
     */
    virtual bool isAlive() = 0;

    /**
     * This method returns the current modem audio status.
     * @return the current modem audio status
     */
    virtual AudioStatus getAudioStatus() = 0;

    /**
     * @return the audio band of the audio signal going through the modem
     */
    virtual CAudioBand::Type getAudioBand() = 0;

    /**
     * Start the ModemAudioManager service.
     * @return true the ModemAudioManager has started, flase otherwise
     */
    virtual bool start() = 0;

    /**
     * Stop the ModemAudioManager service.
     */
    virtual void stop() = 0;

    /**
     * @param[in] audioStatus the modem audio status
     * @return the std::string repesentation of the audioStatus
     */
    static const std::string &toLiteral(AudioStatus audioStatus)
    {
        static const std::string audioDetach("AudioDetach");
        static const std::string audioActive("AudioActive");
        static const std::string voiceActive("VoiceActive");

        switch (audioStatus) {
        case AudioDetach:
            return audioDetach;
        case AudioActive:
            return audioActive;
        case VoiceActive:
            return voiceActive;
        }
        AUDIOCOMMS_ASSERT(false, "Invalid AudioStatus (" << static_cast<int>(audioStatus) << ')');
    }
};
