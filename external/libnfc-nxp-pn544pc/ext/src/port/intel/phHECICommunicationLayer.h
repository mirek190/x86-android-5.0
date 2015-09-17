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
 * \file phHECICommunicationLayer.h
 * $Author: Eff'Innov Technologies $
 */

#ifndef PHHECICOMMUNICATIONLAYER_H
#define PHHECICOMMUNICATIONLAYER_H

#include <stdint.h>
#include <stddef.h>


/**
 * @brief Define HECI Result/Status code
 **/
typedef enum _eHECIStatus
{
    HECI_SUCCESS,
    HECI_PENDING,
    HECI_INVALID_PARAM,
    HECI_ALREADY_IN_PROG,
    HECI_FAILED,
}eHECIStatus;

/**
 * @brief Define PN544 mode : Normal or Download/Degraded mode
 **/
typedef enum _eHECIMode
{
    HECI_NORMAL,
    HECI_DOWNLOAD
}eHECIMode;


/**
 * @brief Contains ME Information
 **/
typedef struct tMeInfo
{
    uint8_t RadioVersionSW[3];
    uint8_t Reserved[3];
    uint8_t RadioVersionHard[3];
    uint8_t Slave_Address;
    uint8_t FwIVN;
    uint8_t NFCC_Vendor_ID;
    uint8_t RadioType;
}tMeInfo_t;

/**
 * @brief Send Callback. This function get called when an asynchronous send is done.
 *
 * @param  ... User context.
 * @param  ... HECI Status.
 * @param  ... Sent data size.
 **/
typedef void (heciCallback_SendHci)(void*,eHECIStatus,size_t);

/**
 * @brief Receive Callback. This function get called when an asynchronous read is done and data are available.
 *
 * @param  ... User context
 * @param  ... HECI Status
 * @param  ... Incoming Data
 * @param  ... size of incoming data
 **/
typedef void (heciCallback_ReceiveHci)(void*,eHECIStatus,uint8_t*,size_t);


/**
 * @brief Log Type enumeration
 **/
typedef enum _eHECILogType
{
    HECI_LOG_SEND_HCI,
    HECI_LOG_RECV_HCI
}eHECILogType;

/**
 * @brief Set a global callback function to log data exchange only.
 *
 * @param  ...
 * @param buffer ...
 * @param sz ...
 **/
typedef void (heciCallback_Log)(eHECILogType,const uint8_t* buffer,size_t sz);

/**
 * @brief Reset Driver callback. This function is called when a reset of driver is required.
 * This function get called when communication with HECI or ME is lost.
 *
 * @param  ... User context
 **/
typedef void (heci_ResetDriver)(void*);

/**
 * @brief Stop Driver callback. This function is called when driver should stop.
 * This function get called when HECI or ME goes to sleep before driver.
 *
 * @param  ... User context
 **/
typedef void (heci_StopDriver)(void*);


/**
 * @brief Setup a connection to HECI.
 * This function is synchronous.
 *
 * @param [out]stackContext Empty pointer to store HECI communication layer context.
 * @param [in]userContext User context. Will be used when callbacks get called.
 * @param [in]sendHciCb send callback function pointer.
 * @param [in]receiveHciCb receive callback function pointer.
 * @param [in]sendConnect When connecting to HECI, do we send Connect command or not. Usefull when using installer dedicated channel.
 * @return eHECIStatus HECI_SUCCESS if all is ok.
 **/
eHECIStatus phHECI_ConnectNormalMode(void** stackContext, void* userContext,heciCallback_SendHci *sendHciCb,heciCallback_ReceiveHci *receiveHciCb,uint8_t sendConnect);
/**
 * @brief Send a HCI frame to Radio device.
 * This function is asynchronous.
 *
 * @param [in]stackContext HECI Communication layer context allocated by phHECI_ConnectNormalMode().
 * @param [in]buffer Buffer to send.
 * @param [in]size Size of buffer to send.
 * @return eHECIStatus HECI_SUCCESS if all is ok.
 **/
eHECIStatus phHECI_SendHCIFrame(void* stackContext,const uint8_t *buffer,size_t size);
/**
 * @brief Receive a HCI frame from Radio device.
 * This function is asynchronous.
 *
 * @param [in]stackContext HECI Communication layer context allocated by phHECI_ConnectNormalMode().
 * @param [in]buffer Buffer to receive data.
 * @param [in]size Size of buffer to receive data.
 * @return eHECIStatus HECI_SUCCESS if all is ok.
 **/
eHECIStatus phHECI_ReceiveHCIFrame(void* stackContext,uint8_t *buffer,size_t size);
/**
 * @brief Disconnect from HECI driver.
 * This function is synchronous
 *
 * @param [in]stackContext HECI Communication layer context allocated by phHECI_ConnectNormalMode().
 * @return eHECIStatus HECI_SUCCESS if all is ok.
 **/
eHECIStatus phHECI_Disconnect(void* stackContext);

/**
 * @brief Allow to disable / enable RF Kill mode.
 * RF Kill ON => no more radio, RF Kill Off => radio On
 *
 * @param [in]stackContext HECI Communication layer context allocated by phHECI_ConnectNormalMode().
 * @param [in]onOff 1 for On 0 for Off
 * @return eHECIStatus HECI_SUCCESS if all is ok.
 **/
eHECIStatus phHECI_RfKill(void *stackContext,uint8_t onOff);

/**
 * @brief Change the power mode of Radio.
 * Not available on all platform. If not available HECI_FAILED is returned.
 *
 * @param [in]stackContext HECI Communication layer context allocated by phHECI_ConnectNormalMode().
 * @param [in]onOff 1 for ON, 0 for OFF
 * @return eHECIStatus HECI_SUCCESS if all is ok.
 **/
eHECIStatus phHECI_ChangePowerModeRadio(void *stackContext,uint8_t onOff);

/**
 * @brief Switch Radio device to Normal or Download mode.
 *
 * @param [in]stackContext HECI Communication layer context allocated by phHECI_ConnectNormalMode().
 * @param [in]emode HECI_NORMAL or HECI_DOWNLOAD.
 * @return eHECIStatus HECI_SUCCESS if all is ok.
 **/
eHECIStatus phHECI_ChangeMode(void *stackContext,eHECIMode emode);

/**
 * @brief Send a RAW I2C frame to Radio device. Only available in download mode.
 *
 * @param [in]stackContext HECI Communication layer context allocated by phHECI_ConnectNormalMode().
 * @param [in]buffer Raw I2C data to send.
 * @param [in]size Size of the buffer.
 * @return eHECIStatus HECI_SUCCESS if all is ok.
 **/
eHECIStatus phHECI_FWDataSend(void* stackContext,const uint8_t *buffer,size_t size);

/**
 * @brief Read RAW I2C frame from Radio device. Only available in download mode.
 *
 * @param [in]stackContext HECI Communication layer context allocated by phHECI_ConnectNormalMode().
 * @param [in]buffer Buffer to receive RAW I2C data
 * @param [in]size Size of the buffer
 * @return eHECIStatus HECI_SUCCESS if all is ok.
 **/
eHECIStatus phHECI_FWDataReceive(void* stackContext,uint8_t *buffer,size_t size);

/**
 * @brief Query information on ME.
 *
 * @param [in]stackContext HECI Communication layer context allocated by phHECI_ConnectNormalMode().
 * @param [in]info Structure that will contains platform informations.
 * @return eHECIStatus HECI_SUCCESS if all is ok.
 **/
eHECIStatus phHECI_QueryInfo(void* stackContext,tMeInfo_t *info);

/**
 * @brief Cause a Radio Software reset.
 *
 * @param [in]stackContext HECI Communication layer context allocated by phHECI_ConnectNormalMode().
 * @return eHECIStatus HECI_SUCCESS if all is ok.
 **/
eHECIStatus phHECI_RadioSoftwareReset(void* stackContext);

/**
 * @brief Make an HCI exchange with Radio.
 * This function may blocked to wait HCI answer.
 *
 * @param [in]stackContext HECI Communication layer context allocated by phHECI_ConnectNormalMode().
 * @param [in]buffer HCI Buffer to send
 * @param [in]sz size of HCI Buffer to send.
 * @param [in]expectedAnswer If not NULL, answer will be filled into. If NULL no answer expected.
 * @param [in]expectedSize Size of answer buffer.
 * @param [in]receivedSize Received size.
 * @return eHECIStatus
 **/
eHECIStatus phHECI_ExchangeHCI(void* stackContext,const uint8_t *buffer, size_t sz,uint8_t *expectedAnswer,size_t expectedSize,size_t *receivedSize);

/**
 * @brief Set General Reset function callback. (optional)
 * This function get called when a communication issue occurs with HECI driver.
 * This allows allows to upper layer to reset it self. When this function get called,
 * the radio device might have lost its current configuration, the radio device must be reconfigured.
 *
 * @param [in]hook Callback function pointer
 * @param [in]userContext User context
 * @return void
 **/
void phHECI_SetResetFunc(heci_ResetDriver *hook, void* userContext);

/**
 * @brief Set General Stop function callback. (optional)
 * This function get called when HECI or ME goes to sleep before Driver.
 *
 * @param [in]hook CallBack function pointer
 * @param [in]userContext User context
 * @return void
 **/
void phHECI_SetStopFunc(heci_StopDriver *hook, void* userContext);

/**
 * @brief Enable or disable Logs from HECI Communication layer.
 * For debugging purpose.
 *
 * @param [in]enabled 1 : Enabled, 0 : disabled.
 * @return void
 **/
void phHECI_SetLog(uint8_t enabled);

/**
 * @brief Set function for Data exchange logs.
 *
 * @param [in]function callback function.
 * @return void
 **/
void phHECI_SetLogFunc(heciCallback_Log *function);

/**
 * @brief Disconnect from low communication layer.
 * Allows to recovery when the low communication layer seems to timedout.
 * This function does not return error as it must always succeed.
 *
 * @return void
 **/
void phHECI_EmergencyRelease();

#endif // ndef PHHECICOMMUNICATIONLAYER_H
