/*
 * Copyright (C) 2010-2013 NXP Semiconductors
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
 * Developped by Eff'Innov Technologies : contact@effinnov.com
 */


/*!
 * \file WaiterNotifier.h
 * $Author: Eff'Innov Technologies $
 */

#include <stdint.h>

/**
 * @brief WaiterNotifier object simplifies Wait and Notification mechanism.
 * WaiterNotifier should be use when there is a thread that wait for another thread result.
 */

/**
 * @defgroup groupWaiterNotifier WaiterNotifier object.
 */


typedef enum _eWaiterNotifierStatus
{
    WAITERNOTIFIER_SUCCESS,
    WAITERNOTIFIER_INVALID_PARAMETER,
    WAITERNOTIFIER_FAILED
}eWaiterNotifierStatus;

/**
 * @brief Create a WaiterNotifier object.
 *
 * @param [out] context pointer on WaiterNotifier context.
 * @return eWaiterNotifierStatus WAITERNOTIFIER_SUCCESS if no error
 * @return eWaiterNotifierStatus WAITERNOTIFIER_INVALID_PARAMETER if parameter not valid.
 * @return eWaiterNotifierStatus WAITERNOTIFIER_FAILED on general failure.
 */
eWaiterNotifierStatus WaiterNotifier_Create(void **context);

/**
 * @brief Delete a WaiterNotifier object.
 *
 * @param [in] context pointer on WaiterNotifier context.
 * @return eWaiterNotifierStatus WAITERNOTIFIER_SUCCESS if no error
 * @return eWaiterNotifierStatus WAITERNOTIFIER_INVALID_PARAMETER if parameter not valid.
 * @return eWaiterNotifierStatus WAITERNOTIFIER_FAILED on general failure.
 */
eWaiterNotifierStatus WaiterNotifier_Delete(void *context);

/**
 * @brief Lock a WaiterNotifier object.
 * If another thread already locked this WaiterNotifier, the calling thread will be blocked until other thread Unlock this WaiterNotifier
 * This function will blocked if called recursively.
 *
 * @param context pointer on WaiterNotifier context.
 * @return eWaiterNotifierStatus WAITERNOTIFIER_SUCCESS if no error
 * @return eWaiterNotifierStatus WAITERNOTIFIER_INVALID_PARAMETER if parameter not valid.
 * @return eWaiterNotifierStatus WAITERNOTIFIER_FAILED on general failure.
 */
eWaiterNotifierStatus WaiterNotifier_Lock(void *context);

/**
 * @brief Unlock a WaiterNotifier object.
 * Call this function to unlock this WaiterNotifier object. Any other thread blocked on WaiterNotifier_Lock() will be unblocked.
 *
 * @param context pointer on WaiterNotifier context.
 * @return eWaiterNotifierStatus WAITERNOTIFIER_SUCCESS if no error
 * @return eWaiterNotifierStatus WAITERNOTIFIER_INVALID_PARAMETER if parameter not valid.
 * @return eWaiterNotifierStatus WAITERNOTIFIER_FAILED on general failure.
 */
eWaiterNotifierStatus WaiterNotifier_Unlock(void *context);

/**
 * @brief The current thread will asleep until this WaiterNotifier get notified.
 * To resume this thread, WaiterNotifier_Notify() should be called. If WaiterNotifier_Notify() is called (1 or several time) prior to
 * WaiterNotifier_Wait() the thread will not be block and return instantly from WaiterNotifier_Wait().
 * needLock parameter should be set to 0 if WaiterNotifier_Lock()/WaiterNotifier_Unlock() are called before and after WaiterNotifier_Wait().
 * If WaiterNotifier_Lock()/WaiterNotifier_Unlock() are not called, needLock should be set to 1.
 *
 * @param context pointer on WaiterNotifier context.
 * @param needLock 1 : Lock, 0 : No lock
 * @return eWaiterNotifierStatus WAITERNOTIFIER_SUCCESS if no error
 * @return eWaiterNotifierStatus WAITERNOTIFIER_INVALID_PARAMETER if parameter not valid.
 * @return eWaiterNotifierStatus WAITERNOTIFIER_FAILED on general failure.
 */
eWaiterNotifierStatus WaiterNotifier_Wait(void *context,uint8_t needLock);

/**
 * @brief Unblock a thread blocked in WaiterNotifier_Wait().
 * If any thread call WaiterNotifier_Wait() after this function call, this thread will not be blocked.
 * needLock parameter should be set to 0 if WaiterNotifier_Lock()/WaiterNotifier_Unlock() are called before and after WaiterNotifier_Notify().
 * If WaiterNotifier_Lock()/WaiterNotifier_Unlock() are not called, needLock should be set to 1.
 *
 * @param context pointer on WaiterNotifier context.
 * @param needLock 1 : Lock, 0 : No lock
 * @return eWaiterNotifierStatus WAITERNOTIFIER_SUCCESS if no error
 * @return eWaiterNotifierStatus WAITERNOTIFIER_INVALID_PARAMETER if parameter not valid.
 * @return eWaiterNotifierStatus WAITERNOTIFIER_FAILED on general failure.
 */
eWaiterNotifierStatus WaiterNotifier_Notify(void *context,uint8_t needLock);
