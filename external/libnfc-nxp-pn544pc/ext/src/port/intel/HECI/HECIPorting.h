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
 * \file HECIPorting.h
 * $Author: Eff'Innov Technologies $
 */

#ifndef HECIPORTING_H
#define HECIPORTING_H

#include <stdint.h>

typedef enum _eHECICommStatus
{
    HECICOMM_SUCCESS,
    HECICOMM_INVALID_PARAM,
    HECICOMM_NO_HECI,
    HECICOMM_FAILED,
}eHECICommStatus;

/**
 * @brief Init HECI communication layer.
 * This function open a connection on HECI Driver. Implementation of this function is platform dependent.
 *
 * @param [out]context Empty pointer to store HECI layer context.
 * @return eHECICommStatus HECI_SUCCESS if all is ok.
 **/
eHECICommStatus HECI_Init(void **context);

/**
 * @brief Deinit HECI communication layer.
 * This function close any opened HECI connection. This function must released context.
 * NOTE : HECI_EmergencyRelease() may also close the HECI connection.
 *
 * @param [in]context HECI layer context allocated by HECI_Init().
 * @return eHECICommStatus HECI_SUCCESS if all is ok.
 **/
eHECICommStatus HECI_Deinit(void *context);

/**
 * @brief Read data from HECI Driver.
 * This function MUST be synchronous. If data are available, the provided buffer must be filled and readSize
 * parameter must be updated.
 *
 * @param [in]context HECI layer context allocated by HECI_Init().
 * @param [in]buffer empty buffer that will contains received datas.
 * @param [in]size size of buffer.
 * @param [out] readSize read size.
 * @return eHECICommStatus HECI_SUCCESS if all is ok.
 **/
eHECICommStatus HECI_Read(void *context,uint8_t *buffer,uint32_t size,uint32_t *readSize);

/**
 * @brief Write data to HECI Driver.
 * This function MUST be synchronous.
 *
 * @param [in]context HECI layer context allocated by HECI_Init().
 * @param [in]buffer buffer that contains data to send.
 * @param [in]size size of buffer.
 * @param [out]writtenSize written size.
 * @return eHECICommStatus HECI_SUCCESS if all is ok.
 **/
eHECICommStatus HECI_Write(void *context,const uint8_t *buffer,uint32_t size, uint32_t *writtenSize);


/**
 * @brief Return maximum HECI message length.
 *
 * @param [in]context HECI layer context allocated by HECI_Init().
 * @param [out]maxMessageLength Maximum HECI message length.
 * @return eHECICommStatus HECI_SUCCESS if all is ok.
 **/
eHECICommStatus HECI_GetClient(void *context,uint32_t *maxMessageLength);

/**
 * @brief Release HECI connection.
 * This function must not free the context.
 *
 * @param [in]context HECI layer context allocated by HECI_Init().
 * @return void
 **/
void HECI_EmergencyRelease(void *context);

/**
 * @brief Enable or disable logs from HECI porting
 *
 * @param [in]value 1 = On ; 0 = Off
 * @return void
 **/
void HECI_SetLog(uint8_t value);

#endif // ndef HECIPORTING_H
