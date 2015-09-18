/*****************************************************************
 * INTEL CONFIDENTIAL
 * Copyright 2011 Intel Corporation All Rights Reserved.
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or its suppliers or licensors. Title to the Material remains with
 * Intel Corporation or its suppliers and licensors. The Material contains
 * trade secrets and proprietary and confidential information of Intel or
 * its suppliers and licensors. The Material is protected by worldwide
 * copyright and trade secret laws and treaty provisions. No part of the
 * Material may be used, copied, reproduced, modified, published, uploaded,
 * posted, transmitted, distributed, or disclosed in any way without Intel’s
 * prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */

#include "serial.h"
#include "atdebug.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <strings.h>
#include <termios.h>
#include <poll.h>

int open_serial(const char *tty_name, int baudrate)
{
    struct termios tio;
    int fd = -1;

    ALOGD(TAG"open_serial: opening %s\n", tty_name);

    fd = open(tty_name, O_RDWR | CLOCAL | O_NOCTTY);
    if (fd < 0) {
        ALOGE(TAG"Unable to open tty port: %s\n", tty_name);
        return fd;
    }

    {
        struct termios terminalParameters;
        int err = tcgetattr(fd, &terminalParameters);
        if (-1 != err) {
            cfmakeraw(&terminalParameters);
            tcsetattr(fd, TCSANOW, &terminalParameters);
        }
    }

    if ( fcntl(fd, F_SETFL, O_NONBLOCK) < 0 ) {
        ALOGE(TAG"fcntl error: %d\n", errno);
        close(fd);
        fd = -1;
        return fd;
    }

    bzero(&tio, sizeof(tio));
    tio.c_cflag = B115200;

    tio.c_cflag |= CS8 | CLOCAL | CREAD;
    tio.c_iflag &= ~(INPCK | IGNPAR | PARMRK | ISTRIP | IXANY | ICRNL);
    tio.c_oflag &= ~OPOST;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 10;

    tcflush(fd, TCIFLUSH);
    cfsetispeed(&tio, baudrate);
    tcsetattr(fd, TCSANOW, &tio);

    ALOGD(TAG"Opened serial port.  fd = %d\n", fd);
    return fd;
}

int open_pc_serial(const char *tty_name, int baudrate)
{
    struct termios tio;
    int fd = -1;

    ALOGD(TAG"open_pc_serial: opening %s\n", tty_name);

    fd = open(tty_name, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        return fd;
    }

    tio.c_iflag = IGNBRK | IGNPAR;
    tio.c_cflag = CLOCAL | CREAD | CS8 | HUPCL | CRTSCTS;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 10;
    cfsetispeed(&tio, baudrate);
    tcsetattr(fd, TCSANOW, &tio);

    ALOGD(TAG"Opened PC serial port.  fd = %d\n", fd);
    return fd;
}

int wait_for_data( int fd, long wait_time_ms)
{
    struct pollfd fds[1];
    int poll_ret;
    int retVal = AT_CMD_OK;

    fds[0].fd = fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    poll_ret = poll( fds, 1, wait_time_ms);
    if ( 0 < poll_ret ) {
        if ( fds[0].revents & POLLHUP ) {
            ALOGE(TAG"device hung up\n");
            retVal = AT_CMD_REOPEN;
            goto Exit_Handler;

        } else if ( fds[0].revents & POLLIN ) {
            ALOGV(TAG"Received response data\n");

        } else {
            /* Not sure we will ever get here */
            ALOGE(TAG"Unexpected event while waiting for response\n");
            retVal = AT_CMD_RESEND;
            goto Exit_Handler;
        }
    } else if ( 0 == poll_ret ) {
        ALOGE(TAG"Timed out while waiting for response\n");
        retVal = AT_CMD_RESEND;
        goto Exit_Handler;

    } else {
        ALOGE(TAG"Error occured polling for response.  errno: %d\n", errno);
        retVal = AT_CMD_RESEND;
        goto Exit_Handler;
    }

    retVal = AT_CMD_OK;

Exit_Handler:
    return retVal;
}

