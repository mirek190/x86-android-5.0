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

/**
 * \file phDalNfc_i2c.c
 * \brief DAL I2C port implementation for linux
 *
 * Project: Trusted NFC Linux
 *
 */

#define LOG_TAG "NFC_i2c"
#include <cutils/log.h>
#include <hardware/nfc.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>

#include <phDal4Nfc_debug.h>
#include <phDal4Nfc_i2c.h>
#include <phOsalNfc.h>
#include <phNfcStatus.h>
#if defined(ANDROID)
#include <string.h>
#endif

#include <linux/pn544.h>

#define   INTERNAL_READ_BUFFER
typedef struct
{
   int  nHandle;
   char nOpened;
   uint8_t  mode;
#ifdef INTERNAL_READ_BUFFER
#define INTERNAL_READ_BUFFER_LEN     0x400
   uint8_t total_read;
   uint8_t prev_read;
   uint8_t read_buffer[INTERNAL_READ_BUFFER_LEN];
#endif /* INTERNAL_READ_BUFFER */

} phDal4Nfc_I2cPortContext_t;


/*-----------------------------------------------------------------------------------
                                      VARIABLES
------------------------------------------------------------------------------------*/
static phDal4Nfc_I2cPortContext_t gI2cPortContext;



/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_set_open_from_handle

PURPOSE:  Initialize internal variables

-----------------------------------------------------------------------------*/

void phDal4Nfc_i2c_initialize(void)
{
   memset(&gI2cPortContext, 0, sizeof(phDal4Nfc_I2cPortContext_t));
}


/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_set_open_from_handle

PURPOSE:  The application could have opened the link itself. So we just need
          to get the handle and consider that the open operation has already
          been done.

-----------------------------------------------------------------------------*/

void phDal4Nfc_i2c_set_open_from_handle(phHal_sHwReference_t * pDalHwContext)
{
   gI2cPortContext.nHandle = (int) pDalHwContext->p_board_driver;
   DAL_ASSERT_STR(gI2cPortContext.nHandle >= 0, "Bad passed com port handle");
   gI2cPortContext.nOpened = 1;
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_is_opened

PURPOSE:  Returns if the link is opened or not. (0 = not opened; 1 = opened)

-----------------------------------------------------------------------------*/

int phDal4Nfc_i2c_is_opened(void)
{
   return gI2cPortContext.nOpened;
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_flush

PURPOSE:  Flushes the link ; clears the link buffers

-----------------------------------------------------------------------------*/

void phDal4Nfc_i2c_flush(void)
{
     /* Nothing to do (driver has no internal buffers) */
#ifdef INTERNAL_READ_BUFFER
      gI2cPortContext.total_read = 0;
      gI2cPortContext.prev_read = 0;
      memset(gI2cPortContext.read_buffer, 0, sizeof(gI2cPortContext.read_buffer));
#endif
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_close

PURPOSE:  Closes the link

-----------------------------------------------------------------------------*/

void phDal4Nfc_i2c_close(void)
{
   DAL_PRINT("Closing port\n");
   if (gI2cPortContext.nOpened == 1)
   {
      close(gI2cPortContext.nHandle);
      gI2cPortContext.nHandle = 0;
      gI2cPortContext.nOpened = 0;
   }
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_open_and_configure

PURPOSE:  Closes the link

-----------------------------------------------------------------------------*/

NFCSTATUS phDal4Nfc_i2c_open_and_configure(pphDal4Nfc_sConfig_t pConfig, void ** pLinkHandle)
{
   DAL_ASSERT_STR(gI2cPortContext.nOpened==0, "Trying to open but already done!");

   DAL_DEBUG("Opening port=%s\n", pConfig->deviceNode);

   /* open port */
   gI2cPortContext.nHandle = open(pConfig->deviceNode, O_RDWR | O_NOCTTY);
   if (gI2cPortContext.nHandle < 0)
   {
       DAL_DEBUG("Open failed: open() returned %d\n", gI2cPortContext.nHandle);
      *pLinkHandle = NULL;
      return PHNFCSTVAL(CID_NFC_DAL, NFCSTATUS_INVALID_DEVICE);
   }

   gI2cPortContext.nOpened = 1;
   *pLinkHandle = (void*)gI2cPortContext.nHandle;

   DAL_PRINT("Open succeed\n");

   return NFCSTATUS_SUCCESS;
}


/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_read

PURPOSE:  Reads nNbBytesToRead bytes and writes them in pBuffer.
          Returns the number of bytes really read or -1 in case of error.

-----------------------------------------------------------------------------*/

int phDal4Nfc_i2c_read(uint8_t * pBuffer, int nNbBytesToRead)
{
    int ret;
    int numRead = 0;
    struct timeval tv;
    struct timeval *ptv;
    fd_set rfds;

    DAL_ASSERT_STR(gI2cPortContext.nOpened == 1, "read called but not opened!");
    DAL_DEBUG("_i2c_read() called to read %d bytes", nNbBytesToRead);

    ptv = &tv;
    // Read with 2 second timeout, so that the read thread can be aborted
    // when the pn544 does not respond and we need to switch to FW download
    // mode. This should be done via a control socket instead.
#ifdef INTERNAL_READ_BUFFER
    while (nNbBytesToRead > (gI2cPortContext.total_read -
                  gI2cPortContext.prev_read))
    {
#else
    while (numRead < nNbBytesToRead)
    {
#endif /* INTERNAL_READ_BUFFER */
        FD_ZERO(&rfds);
        FD_SET(gI2cPortContext.nHandle, &rfds);
        if(nNbBytesToRead > 1)
        {
           tv.tv_sec = 2;
           tv.tv_usec = 0;
        }
        else
        {
           ptv = NULL;
        }
        ret = select(gI2cPortContext.nHandle + 1, &rfds, NULL, NULL, ptv);
        if (ret < 0) {
            DAL_DEBUG("##I2C## select() errno=%d", errno);
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            return -1;
        } else if (ret == 0) {
            DAL_PRINT("##I2C## timeout!");
            return -1;
        }
#ifdef INTERNAL_READ_BUFFER
        gI2cPortContext.total_read = 0;
        gI2cPortContext.prev_read = 0;

        ret = read(gI2cPortContext.nHandle, gI2cPortContext.read_buffer, nNbBytesToRead);
#else
        ret = read(gI2cPortContext.nHandle, pBuffer + numRead, nNbBytesToRead - numRead);
#endif /* INTERNAL_READ_BUFFER */
        if (ret > 0) {
            DAL_DEBUG("##I2C## read %d bytes", ret);
#ifdef INTERNAL_READ_BUFFER
            gI2cPortContext.total_read += ret;
            if( nNbBytesToRead > gI2cPortContext.total_read )
            {
                nNbBytesToRead = gI2cPortContext.total_read;
            }
#else
            numRead += ret;
#endif
        } else if (ret == 0) {
            DAL_PRINT("_i2c_read() EOF");
            return -1;
        } else {
            DAL_DEBUG("_i2c_read() errno=%d", errno);
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            return -1;
        }
    }

#ifdef INTERNAL_READ_BUFFER
    memcpy( pBuffer, gI2cPortContext.read_buffer +
                 gI2cPortContext.prev_read, nNbBytesToRead );
    gI2cPortContext.prev_read += nNbBytesToRead;
    return nNbBytesToRead;
#else
    return numRead;
#endif /* INTERNAL_READ_BUFFER */
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_write

PURPOSE:  Writes nNbBytesToWrite bytes from pBuffer to the link
          Returns the number of bytes that have been wrote to the interface or -1 in case of error.

-----------------------------------------------------------------------------*/

int phDal4Nfc_i2c_write(uint8_t * pBuffer, int nNbBytesToWrite)
{
    int ret;
    int numWrote = 0;

    DAL_ASSERT_STR(gI2cPortContext.nOpened == 1, "write called but not opened!");
    DAL_DEBUG("_i2c_write() called to write %d bytes\n", nNbBytesToWrite);

    while (numWrote < nNbBytesToWrite) {
        ret = write(gI2cPortContext.nHandle, pBuffer + numWrote, nNbBytesToWrite - numWrote);
        if (ret > 0) {
            DAL_DEBUG("wrote %d bytes", ret);
            numWrote += ret;
        } else if (ret == 0) {
            DAL_PRINT("_i2c_write() EOF");
            return -1;
        } else {
            DAL_DEBUG("_i2c_write() errno=%d", errno);
            if (NFC_HIF_MODE_FW == gI2cPortContext.mode )
            {
                /*
                 * RU: Do not need to perform I2C retry in this mode
                 *
                 */
                return -1;
            }
            else if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            return -1;
        }
    }
    return numWrote;
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_reset

PURPOSE:  Reset the PN544, using the VEN pin

-----------------------------------------------------------------------------*/
int phDal4Nfc_i2c_reset(long level)
{
    DAL_DEBUG("phDal4Nfc_i2c_reset, VEN level = %ld", level);

    /* RU: Store the Level */
    gI2cPortContext.mode = level;
#ifdef INTERNAL_READ_BUFFER
    gI2cPortContext.total_read = 0;
    gI2cPortContext.prev_read = 0;
#endif

#if defined( PN544_SET_PWR )
    return ioctl(gI2cPortContext.nHandle, PN544_SET_PWR, level);
#elif defined ( PN544_SET_FW_MODE )
   switch(level)
   {
        case NFC_HIF_MODE_FW:
        {
            usleep(10000);
            return ioctl(gI2cPortContext.nHandle, PN544_SET_FW_MODE, TRUE );
        }
        case NFC_HIF_MODE_OFF:
        case NFC_HIF_MODE_ON:
        {
            return ioctl(gI2cPortContext.nHandle, PN544_SET_FW_MODE, FALSE );
        }
        default
        {
            DAL_DEBUG(" Invalid I2C Link Reset Option: %ld",  level);
            return -1;
        }
   }
#else
    return -1;
#endif

}
