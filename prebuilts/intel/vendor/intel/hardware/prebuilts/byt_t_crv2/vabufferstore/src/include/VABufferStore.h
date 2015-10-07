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

#ifndef __VA_BUFFER_STORE_H__
#define __VA_BUFFER_STORE_H__

#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/threads.h>
#include <utils/Vector.h>
#include <va/va.h>

/**
 * Maximum length of the string identifying a session
 */
#define MAX_SESSION_NAME_LENGH  128
/**
 * convenience macro to group all the session state classes as friends
 */
#define SESSION_CLASS_IS_A_FRIEND   friend class BufferStoreSession; \
                                    friend class SessionState; \
                                    friend class SettingUpState; \
                                    friend class ReadyState;

using namespace android;

namespace intel
{
/**
 * This is the structured stored that is shared between the entities.
 * The surface ID is complemented with other fields needed to use effectively
 * the surface.
 */
typedef struct {
    VASurfaceID surface;
    int width;
    int height;
    int cropWidth;
    int cropHeight;
    VADisplay display;
    long long int timestamp;
} VABS_SharedBuffer;


typedef enum BufferStoreRole {
    BUFFER_STORE_ROLE_ALLOCATOR,
    BUFFER_STORE_ROLE_USER,
} BufferStoreRole;

typedef enum BufferFlowRole {
    BUFFER_FLOW_ROLE_PRODUCER,
    BUFFER_FLOW_ROLE_CONSUMER,
} BufferFlowRole;
/**
 * States of the session state machine
 */
typedef enum {
    BSS_CREATED = 0,
    BSS_SETTINGUP,
    BSS_READY,
    BSS_CLOSING,
    BSS_MAX
} VABS_SessionState;

static const char* VABS_StatusStringTable[] = {
    "VABS_SUCCESS",
    "VABS_INVALID_STATE",
    "VABS_INVALID_PARAMETER",
    "VABS_PEER_DOWN",
    "VABS_NO_MEMORY",
    "VABS_UNKNOWN",
};

/**
 * Possible status values given by the library
 */
typedef enum {
    VABS_SUCCESS = 0,
    VABS_INVALID_ROLE,
    VABS_INVALID_PARAMETER,
    VABS_PEER_DOWN,
    VABS_NO_MEMORY,
    VABS_UNKNOWN,
} VABS_Status;

/* forward class declaration */
class BufferStoreSession;


/*************************************************************
 * Helper classes
 * - BufferProducerIF
 * - BufferProducerIF
 * Pure abstract interfaces to control the buffer flow
 */

/**
 * Interface implemented by the buffer producer
 * The consumer will use this interface to return the buffers
 *
 */
class BufferProducerIF
{
public:
    virtual status_t ReturnBuffer(VABS_SharedBuffer *b) = 0;
    virtual ~BufferProducerIF() {};
};

/**
 * Interface implemented by the consumer.
 * The producer will use it to send the buffers
 *
 */
class BufferConsumerIF
{
public:
    virtual status_t SendBuffer(VABS_SharedBuffer *b) = 0;
    virtual status_t Pause() = 0;
    virtual ~BufferConsumerIF() {};
};

/**
 * class VABSClientInfo
 *
 * This class is meant to be extended by the users of this library
 * to receive communication from the VA Buffer store.
 * The notifyChange() method is the only one that must be implemented
 * The others may be overriden, but a default implementation is provided
 *
 * Clients will instantiate and object of this class and pass it to
 * the store.
 * The store will use this object to notify the clients in the status
 * of the session they are part of
 *
 * The VA buffer store will set the appropriate values and notify the client
 * when the are changes are done.
 */

class VABSClientInfo {
public:
    /**
     * normal constructor
     */
    VABSClientInfo();

    VABS_SharedBuffer*  getSharedBuffers()      {return mBufs;};
    int32_t             getSharedBufferCount()  {return mBufAmount;};
    BufferConsumerIF*   getConsumerIF() {return mC;};
    BufferProducerIF*   getProducerIF() {return mP;};
    VABS_SessionState   getState()      {return mState;};
    void                reset();

    SESSION_CLASS_IS_A_FRIEND;

protected:
    virtual ~VABSClientInfo();
    virtual VABS_Status notifyChange();

    /* Protected setters.
     * Only the children classes or the session should be able to use these */
    virtual void setSharedBuffers(VABS_SharedBuffer *mBufs, int32_t mBufAmount);
    virtual void setConsumerIF(BufferConsumerIF *mC);
    virtual void setProducerIF(BufferProducerIF *mP);
    virtual void setState(VABS_SessionState mState);

protected:
    VABS_SessionState   mState;
    BufferProducerIF    *mP;
    BufferConsumerIF    *mC;
    VABS_SharedBuffer   *mBufs;
    int32_t             mBufAmount;

};


/**
 *
 * Handle to a VABufferStore session,
 * an object of this class is returned upon successful session creation
 * or after joining and existing one.
 * This class is the interface towards the session. Once the
 * client info is set successfully the session moves to the next state.
 *
 */
class BufferStoreClient: public RefBase
{

public:
    /**
     * Signals to the store session that the information from this client is ready
     * @param info: instance of the client info class that contains the information
     *
     * The object is assumed to be valid during the life of the session.
     * checks on the integrity of the data are performed by the client class.
     * Failing any of the integrity checks will result in an error.
     */
    status_t                 setClientInfo(VABSClientInfo *info);
    /**
     * Possibility to get the peers instance of the VABSClientInfo object.
     * This can be use to exchange more information than the one in the VABS_SharedBuffer
     * It implies that the client know more about the object (i.e. it can be re casted)
     */
    const VABSClientInfo*    getPeerInfo()   {return (const VABSClientInfo*)mPeerInfo;};

    friend class VABufferStore;
    SESSION_CLASS_IS_A_FRIEND;

private:

    VABSClientInfo*     getInfo() { return mInfo; };
    bool                wantsToWait() {return mWaitingClient;};
    void                setPeerInfo(VABSClientInfo *aPeerInfo);

    BufferStoreClient(BufferStoreSession *aSession, BufferStoreRole aRole, bool waits);
    ~BufferStoreClient();

private:
    BufferStoreSession  *mSession;
    BufferStoreRole     mRole;
    BufferFlowRole      mFlowRole;
    bool                mWaitingClient;
    VABSClientInfo      *mInfo;
    VABSClientInfo      *mPeerInfo;
};

/**
 * Singleton class that has visibility of all the sessions
 * it is the single point of contact for session creation/destruction
 *
 */
class VABufferStore : public RefBase
{

public:
    /**
     * Main accessor method. It returns the singleton instance.
     */
    static sp<VABufferStore> getInstance(void);
    /**
     * Starts or joins an existing session. Returns a pointer to a handle class BuffserSToreClient
     * or NULL in case of failure. More information about the problem can be found by examining the
     * content of the aStatus pointer.
     *
     * @param name string with the unique name for the session
     * @param aRole the role that this client will play in this session
     * @param aStatus pointer to a user allocated VABS_Status variable where any error will be returned.
     * NULL can be passed, in which case nothing will be set there.
     * @param waits boolean to control what happens to the session once the peer leaves
     *
     * @return VABS_SUCCESS on success
     * @return  VABS_INVALID_ROLE in case the role is already taken in this session
     * @return  VABS_INVALID_PARAMETER in case any of the parameters is invalid
     *
     */
    static BufferStoreClient* joinSession(const char* name, BufferStoreRole aRole, VABS_Status *aStatus, bool waits);

    /**
     * Signals the VABS Session that the client wants to abandon the session.
     * The handle object is returned for disposal.
     * @param aClient handle obtained upon joining a session
     *
     * @return OK in case of success.
     */
    static status_t abandonSession(BufferStoreClient* aClient);


private:
    VABufferStore();
    ~VABufferStore();

    const char* statusToString(VABS_Status status);

    /*singleton control data*/
private:
    static Mutex sRegistryLock;
    static sp<VABufferStore> sInstance;
    static Vector<BufferStoreSession*>   sSessions;

};

/* dynamic loading interface */
extern "C" {
    sp<VABufferStore>  GetVABufferStoreInstance(void);
}

} // namespace intel
#endif
