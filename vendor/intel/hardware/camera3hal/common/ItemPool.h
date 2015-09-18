/*
 * Copyright (c) 2014 Intel Corporation.
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
#ifndef CAMERA3_HAL_ITEMPOOL_H_
#define CAMERA3_HAL_ITEMPOOL_H_

#include "utils/Vector.h"
#include "utils/Mutex.h"

namespace android {
namespace camera2 {

/**
 * \class ItemPool
 *
 * Pool of items. This class creates a pool of items and manages the acquisition
 * and release of them. This class is thread safe, i.e. it can be called from
 * multiple threads.
 *
 *
 */
template <class ItemType>
class ItemPool {
public:
    ItemPool();
    virtual ~ItemPool();

    status_t init(size_t poolSize);
    void     deInit(void);
    status_t acquireItem(ItemType **item);
    status_t releaseItem(ItemType *item);

    int availableItems() { return mItemsInPool.size(); }
    bool isEmpty() { return mItemsInPool.isEmpty(); }

private: /* members*/
    ItemType           *mAllocatedItems;
    unsigned int        mPoolSize;  /*!< This is the pool capacity */
    Vector<ItemType*>   mItemsInPool;
    Mutex               mLock;
    bool                mInitialized;
};

template <class ItemType>
ItemPool<ItemType>::ItemPool():
    mAllocatedItems(NULL),
    mPoolSize(0),
    mInitialized(false)
{
    mItemsInPool.clear();
}

template <class ItemType>
status_t
ItemPool<ItemType>::init(size_t poolSize)
{
    if (mInitialized) {
        ALOGW("trying to initialize twice the pool");
        deInit();
    }
    mAllocatedItems = new ItemType[poolSize];
    if (!mAllocatedItems) {
        ALOGE("could not create Pool of items, not enough memory");
        return NO_MEMORY;
    }

    mItemsInPool.setCapacity(poolSize);
    for (size_t i = 0; i < poolSize; i++)
        mItemsInPool.add(&mAllocatedItems[i]);

    mPoolSize = poolSize;
    mInitialized = true;
    return NO_ERROR;
}

template <class ItemType>
void
ItemPool<ItemType>::deInit()
{
    if (mAllocatedItems) {
        delete [] mAllocatedItems;
        mAllocatedItems = NULL;
    }

    mItemsInPool.clear();
    mPoolSize = 0;
    mInitialized = false;
}

template <class ItemType>
ItemPool<ItemType>::~ItemPool()
{
    if (mAllocatedItems) {
        delete [] mAllocatedItems;
        mAllocatedItems = NULL;
    }
    mItemsInPool.clear();
}

template <class ItemType>
status_t
ItemPool<ItemType>::acquireItem(ItemType **item)
{
    status_t status = NO_ERROR;
    Mutex::Autolock _al(mLock);
    LOG2("%s size is %d", __FUNCTION__, mItemsInPool.size());
    if (item == NULL) {
        ALOGE("Invalid parameter to acquire item from the pool");
        return BAD_VALUE;
    }

    if (!mItemsInPool.isEmpty()) {
        *item = mItemsInPool.itemAt(0);
        mItemsInPool.removeItemsAt(0);
    } else {
        status = INVALID_OPERATION;
        ALOGW("Pool is empty, cannot acquire item");
    }
    return status;
}

template <class ItemType>
status_t
ItemPool<ItemType>::releaseItem(ItemType *item)
{
    status_t status = NO_ERROR;
    Mutex::Autolock _al(mLock);

    if (item == NULL)
        return BAD_VALUE;

    bool belongs = false;
    for (unsigned int i = 0; i <  mPoolSize; i++)
        if (item == &(mAllocatedItems[i])) {
            belongs = true;
            break;
        }

    if (belongs) {
        mItemsInPool.add(item);
    } else {
        status = BAD_VALUE;
        ALOGW("Trying to release an Item that doesn't belong to this pool");
    }
    LOG2("%s size is %d", __FUNCTION__, mItemsInPool.size());
    return status;
}
} /* namespace camera2 */
} /* namespace android */
#endif /* CAMERA3_HAL_ITEMPOOL_H_ */
