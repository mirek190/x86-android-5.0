/*
 **
 ** Copyright 2011 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **      http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 **
 ** Author:
 ** Zhang, Dongsheng <dongsheng.zhang@intel.com>
 **
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <termios.h>
#include <pthread.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/ioctl.h>

#define LOG_TAG "ad_proxy:usb"
#include "cutils/log.h"

#include "ad_usb_tty.h"

#define RETRY_MAX    20
#define RETRY_DELAY  500 /* in ms */

#define MS_TO_US(ms) (1000 * ms)

int ad_open_tty(const char *tty_name, int baudrate)
{
    int i;
    int fd = -1;
    struct termios tio;

    ALOGD("-->%s(%s)", __func__, tty_name);

    for (i=0; i<RETRY_MAX; i++) {
        fd = open(tty_name, O_RDWR | O_NOCTTY);
        if (fd >= 0) {
            break;
        }
        usleep(MS_TO_US(RETRY_DELAY));
    }
    if (fd < 0) {
        ALOGE("%s open error %d", __func__, fd);
        return -1;
    }

    tio.c_iflag = IGNBRK | IGNPAR;
    //PARENB Parity enable. PARODD Odd parity, else even
    tio.c_cflag = CLOCAL | CREAD | CS8 | HUPCL | CRTSCTS | PARENB;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 10;

    cfsetispeed(&tio, baudrate);
    tcsetattr(fd, TCSANOW, &tio);

    ALOGD("<--%s: fd[%d]", __func__, fd);

    return fd;
}

