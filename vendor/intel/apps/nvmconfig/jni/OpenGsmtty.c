/* NVM Config
 *
 * Copyright (C) Intel 2012
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <strings.h>
#include <cutils/log.h>

#include "OpenGsmtty.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "NVM_CONFIG"

JNIEXPORT jint JNICALL Java_com_intel_android_nvmconfig_io_GsmTty_OpenSerial(JNIEnv *env, jobject obj, jstring jtty_name, jint baudrate)
{
    int fd = -1;
    const char *tty_name  = (*env)->GetStringUTFChars(env, jtty_name, 0);
    int err = 0;
    struct termios tio;
    struct termios terminalParameters;
    ALOGD("OpenSerial: opening %s\n", tty_name);

    fd = open(tty_name, O_RDWR | CLOCAL | O_NOCTTY);
    if (fd < 0) {
        ALOGE("Unable to open tty port: %s\n", tty_name);
        return fd;
    }

    err = tcgetattr(fd, &terminalParameters);
    if (-1 != err) {
        cfmakeraw(&terminalParameters);
        tcsetattr(fd, TCSANOW, &terminalParameters);
    }

    if ( fcntl(fd, F_SETFL, O_NONBLOCK) < 0 ) {
        ALOGE("fcntl error: %d\n", errno);
        close(fd);
        fd = -1;
        return fd;
    }

    memset(&tio, 0, sizeof(tio));
    tio.c_cflag = B115200;

    tio.c_cflag |= CS8 | CLOCAL | CREAD;
    tio.c_iflag &= ~(INPCK | IGNPAR | PARMRK | ISTRIP | IXANY | ICRNL);
    tio.c_oflag &= ~OPOST;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 10;

    tcflush(fd, TCIFLUSH);
    cfsetispeed(&tio, baudrate);
    tcsetattr(fd, TCSANOW, &tio);

    (*env)->ReleaseStringUTFChars(env, jtty_name, tty_name);
    ALOGD("Opened serial port. fd = %d\n", fd);
    return fd;
}

JNIEXPORT jint JNICALL Java_com_intel_android_nvmconfig_io_GsmTty_CloseSerial(JNIEnv *env, jobject obj, jint fd)
{
    if (fd >= 0) {
        ALOGD("Close serial port. fd = %d\n", fd);
        close(fd);
        fd = -1;
    } else {
        ALOGD("Serial port already closed");
    }
    return 0;
}
