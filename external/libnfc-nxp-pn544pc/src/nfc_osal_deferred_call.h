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
 */

#ifndef __NFC_OSAL_DEFERRED_CALL_H_
#define __NFC_OSAL_DEFERRED_CALL_H_

/**
 * \ingroup grp_osal_nfc
 *\brief Deferred call declaration.
 * This type of API is called from ClientApplication ( main thread) to notify
 * specific callback.
 */
typedef  pphLibNfc_DeferredCallback_t nfc_osal_def_call_t;

/**
 * \ingroup grp_osal_nfc
 *\brief Deferred message specific info declaration.
 * This type information packed as WPARAM  when \ref PH_OSALNFC_MESSAGE_BASE type
 *windows message is posted to main thread.
 */
typedef phLibNfc_DeferredCall_t nfc_osal_def_call_msg_t;

/**
 * \ingroup grp_osal_nfc
 *\brief Deferred call declaration.
 * This Deferred call post message of type \ref PH_OSALNFC_TIMER_MSG along with
 * timer specific details.ain thread,which is responsible for timer callback notification
 * consumes of this message and notifies respctive timer callback.
 *\note: This API packs upper timer specific callback notification  information and post
 *\ref PH_OSALNFC_MESSAGE_BASE to main thread via windows post messaging mechanism.
 */

void nfc_osal_deferred_call(nfc_osal_def_call_t func, void *param);

#endif
