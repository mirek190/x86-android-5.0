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

#include <string>

class CATCommand
{

enum Status
{
    StatusUnset,
    StatusError,
    StatusOK
};

public:
    /** Instanciate an AT command.
     *
     * @param[in] strCommand The AT command string
     * @param[in] strRespPrefix The AT command response prefix
     * @param[in] responseEventId The event ID associated to the response reception of the AT
     *                            command, or 0 is no event to be broadcasted uppon response
     *                            reception
     */
    CATCommand(const std::string &strCommand, const std::string &strRespPrefix,
               uint32_t responseEventId = 0)
        : mCommand(strCommand),
          mRespPrefix(strRespPrefix),
          mStatus(StatusUnset),
          mEventId(responseEventId) {}

    virtual ~CATCommand() {}

    /** Clear the command status, returning to the post constructor state. */
    void clearStatus()
    {
        // Answer status
        mStatus = StatusUnset;

        // Answer
        mAnswer.clear();
    }

    /** @return the AT command string. */
    const std::string &getCommand() const
    {
        return mCommand;
    }

    /** Answer fragment */
    virtual void addAnswerFragment(const std::string &strAnswerFragment)
    {
        mAnswer += strAnswerFragment + '\n';
    }

    /** @return the command answer */
    const std::string &getAnswer() const
    {
        return mAnswer;
    }

    /** Set the command status.
     *
     * @param bIsOK If true, the command is a success,
     *              If false, the command failed.
     */
    virtual void setAnswerOK(bool bIsOK)
    {
        mStatus = bIsOK ? StatusOK : StatusError;
    }

    /** Process the command respond. */
    virtual void doProcessAnswer() {}

    /** Get Status. */
    bool isAnswerOK() const
    {
        return mStatus == StatusOK;
    }

    /** @return true if the answer is complete, false otherwise. */
    bool isComplete() const
    {
        return mStatus != StatusUnset;
    }

    /** @return the respond prefix. */
    const std::string &getPrefix() const
    {
        return mRespPrefix;
    }

    /* True if the respond has a prefix, false otherwise */
    bool hasPrefix() const
    {
        return !mRespPrefix.empty();
    }

    /** Returns the event ID code associated to the AT command response reception.
     *
     * If O, then there is not event ID to be broadcasted upon such AT response reception.
     * @return The event ID code
     */
    uint32_t getResponseEventId() const
    {
        return mEventId;
    }

private:
    /** AT Command */
    std::string mCommand;
    /** Expected answer prefix */
    std::string mRespPrefix;
    /** Answer placeholder */
    std::string mAnswer;
    /** Command status */
    Status mStatus;
    /** Event ID associated to the AT response reception */
    uint32_t mEventId;
};
