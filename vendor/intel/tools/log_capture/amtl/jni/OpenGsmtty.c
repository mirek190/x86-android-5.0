/* Android Modem Traces and Logs
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
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <strings.h>
#include <cutils/log.h>

#include "OpenGsmtty.h"

#define TYPE_JAVA "Ljava/lang/String;"
#define VAR_JAVA "mNativeTag"

#define TTY_CLOSED -1
#define UNUSED __attribute__((__unused__))

#ifndef AMTLLOGI
#define AMTLLOGI(tag, ...) \
        android_printLog(ANDROID_LOG_INFO, tag, __VA_ARGS__)
#endif

#ifndef AMTLLOGV
#define AMTLLOGV(tag, ...) \
        android_printLog(ANDROID_LOG_VERBOSE, tag, __VA_ARGS__)
#endif

#ifndef AMTLLOGE
#define AMTLLOGE(tag, ...) \
        android_printLog(ANDROID_LOG_ERROR, tag, __VA_ARGS__)
#endif

#ifndef AMTLLOGD
#define AMTLLOGD(tag, ...) \
        android_printLog(ANDROID_LOG_DEBUG, tag, __VA_ARGS__)
#endif

static void getLogTag(JNIEnv *env, UNUSED jobject obj, jstring *valueString, char **tag)
{
    jfieldID fieldID
        = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, obj), VAR_JAVA, TYPE_JAVA);
    *valueString = (jstring) (*env)->GetObjectField(env, obj, fieldID);
    *tag = (char*) (*env)->GetStringUTFChars(env, *valueString, NULL);
}

static void freeLogTag(JNIEnv *env, jstring valueString, char *tag)
{
    (*env)->ReleaseStringUTFChars(env, valueString, tag);
}

JNIEXPORT jint JNICALL Java_com_intel_amtl_modem_Gsmtty_OpenSerial(JNIEnv *env, UNUSED jobject obj,
        jstring jtty_name, jint baudrate)
{
    int fd = TTY_CLOSED;
    jstring valueString = NULL;
    char *tag = NULL;
    const char *tty_name = (*env)->GetStringUTFChars(env, jtty_name, 0);
    getLogTag(env, obj, &valueString, &tag);
    struct termios tio;
    AMTLLOGI(tag, "OpenSerial: opening %s", tty_name);

    fd = open(tty_name, O_RDWR | CLOCAL | O_NOCTTY);
    if (fd < 0) {
        AMTLLOGE(tag, "OpenSerial: %s (%d)", strerror(errno), errno);
        goto open_serial_failure;
    }

    struct termios terminalParameters;
    if (tcgetattr(fd, &terminalParameters)) {
        AMTLLOGE(tag, "OpenSerial: %s (%d)", strerror(errno), errno);
        goto open_serial_failure;
    }

    cfmakeraw(&terminalParameters);
    if (tcsetattr(fd, TCSANOW, &terminalParameters)) {
        AMTLLOGE(tag, "OpenSerial: %s (%d)", strerror(errno), errno);
        goto open_serial_failure;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0 ) {
        AMTLLOGE(tag, "OpenSerial: %s (%d)", strerror(errno), errno);
        goto open_serial_failure;
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

    goto open_serial_success;

open_serial_failure:
    if (fd >= 0) {
        close(fd);
        fd = TTY_CLOSED;
    }

open_serial_success:
    if (fd != TTY_CLOSED)
        AMTLLOGI(tag, "OpenSerial: %s opened (%d)", tty_name, fd);
    (*env)->ReleaseStringUTFChars(env, jtty_name, tty_name);
    freeLogTag(env, valueString, tag);
    return fd;
}

JNIEXPORT jint JNICALL Java_com_intel_amtl_modem_Gsmtty_CloseSerial(UNUSED JNIEnv *env,
        UNUSED jobject obj, jint fd)
{
    jstring valueString = NULL;
    char *tag = NULL;
    getLogTag(env, obj, &valueString, &tag);
    AMTLLOGI(tag, "CloseSerial: closing file descriptor (%d)", fd);
    if (fd >= 0) {
        close(fd);
        fd = TTY_CLOSED;
        AMTLLOGD(tag, "CloseSerial: closed");
    }
    else {
        AMTLLOGD(tag, "CloseSerial: already closed");
    }
    freeLogTag(env, valueString, tag);
    return 0;
}
