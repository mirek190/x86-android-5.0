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
 * \file phOsEvent_Interface.h
 * $Author: Eff'Innov Technologies $
 */

#ifndef PHOSEVENT_INTERFACE_H
#define PHOSEVENT_INTERFACE_H

/**
 * @brief Return values for this layer.
 **/
typedef enum _eEventStatus
{
    EVENT_SUCCESS,
    EVENT_INVALID_PARAM,
    EVENT_FAILED
}eEventStatus;

/**
 * @brief Internal value when a Fault is detected with IMEI/HECI driver.
 **/
typedef enum _eFaultStatus
{
    FAULT_CONNECTION_FAILED,
    FAULT_SEND_FAILED,
    FAULT_INCOMPATIBLE_IVN,
    FAULT_INCOMPATIBLE_RADIO,
    FAULT_SEND_TIMEOUT
}eFaultStatus;



/**
 * @brief Initialize OS Event handler layer.
 *
 * @param [out]context Empty pointer to store context.
 * @return eEventStatus EVENT_SUCCESS if all is ok
 **/
eEventStatus phOsEvent_Init(void **context);
/**
 * @brief Deinitialize OS Event handler layer.
 *
 * @param [in]context context allocated by phOsEvent_Init()
 * @return eEventStatus EVENT_SUCCESS if all is ok
 **/
eEventStatus phOsEvent_Deinit(void *context);

typedef void (phOsEvent_MeResetCallBack)(void*);

/**
 * @brief Register a callback that get called when ME Reset is detected.
 *
 * @param [in]context context allocated by phOsEvent_Init()
 * @param cb Callback when ME reset is detected.
 * @param userContext context to provide with callback.
 * @return eEventStatus EVENT_SUCCESS if all is ok
 **/
eEventStatus phOsEvent_RegisterMeReset(void *context,phOsEvent_MeResetCallBack *cb,void *userContext);

/**
 * @brief Unregister ME reset callback.
 *
 * @param [in]context context allocated by phOsEvent_Init()
 * @return eEventStatus EVENT_SUCCESS if all is ok
 **/
eEventStatus phOsEvent_UnregisterMeReset(void *context);

/**
 * @brief Post an event to OS when a ME/HECI.
 *
 * @param [in]context context allocated by phOsEvent_Init()
 * @param status a value from eFaultStatus
 * @return eEventStatus EVENT_SUCCESS if all is ok
 **/
eEventStatus phOsEvent_GenerateEvent(void *context,eFaultStatus status);


#endif // ndef PHOSEVENT_INTERFACE_H
