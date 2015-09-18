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
 **
 */

#include "ad_streamer.h"

#define LOG_TAG "LIBAD-STREAMER"
#include "cutils/log.h"

#include <full_rw.h>
#include <hardware_legacy/power.h>

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#define false   0
#define true    1

/* Time out used by the run_writefile thread to detect
 * that the capture thread has stopped and does not produce
 * buffer anymore. Assuming a bandwith of about 70KB/s for the
 * catpure thread, and a buffer size of 1024B, the capture thread
 * should produce a buffer about every 14 ms. Since the time out
 * value must be in seconds, the closest value is 1 second.
 */
#define READ_TIMEOUT_IN_SEC 1
/* wait for streaming capture starts */
#define START_TIMEOUT_IN_SEC 1
/* Waiting until all audience stream is flushed in micro seconds */
#define THREAD_EXIT_WAIT_IN_US 100000

/* Private data */
static const unsigned char startStreamCmd[AD_CMD_SIZE] = { 0x80, 0x25, 0x00, 0x01 };
static const unsigned char stopStreamCmd[AD_CMD_SIZE] = { 0x80, 0x25, 0x00, 0x00 };

static const char *ad_streamer_id = "adStreamer";

static void clear_streaming_handler(ad_streamer_handler *handler)
{
    if (handler != NULL) {

        handler->cap_idx = 0;
        handler->wrt_idx = 0;
        handler->read_bytes = 0;
        handler->written_bytes = 0;
        handler->stop_requested = false;
        handler->is_catpure_started = false;
    }
}

ad_streamer_handler *alloc_streaming_handler(int audience_fd, int output_fd)
{
    ad_streamer_handler *handler = NULL;

    /* Check arguments */
    if (audience_fd < 0 || output_fd < 0) {

        ALOGE("Invalid file descriptors [%d, %d]", audience_fd, output_fd);
        return NULL;
    }
    handler = malloc(sizeof(ad_streamer_handler));

    if (handler != NULL) {

        clear_streaming_handler(handler);
        /* Save file descriptors */
        handler->audience_fd = audience_fd;
        handler->outfile_fd = output_fd;
    }

    return handler;
}

void free_streaming_handler(ad_streamer_handler *handler)
{
    free(handler);
}


/* Thread to read stream on Audience chip */
void *run_capture(void *arg)
{
    int index = 0;
    int byte_read;
    struct timespec ts;
    int buffer_level;
    ad_streamer_handler *handler = arg;

    if (handler == NULL) {

        return NULL;
    }
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += START_TIMEOUT_IN_SEC;

    if (sem_timedwait(&handler->sem_start, &ts)) {

        ALOGE("Error: read start timed out.");
        handler->stop_requested = true;
        return NULL;
    }
    while (!handler->stop_requested) {

        byte_read = full_read(handler->audience_fd, handler->data[index], ES3X5_BUFFER_SIZE);
        if (byte_read != ES3X5_BUFFER_SIZE) {

            ALOGE("Error: read has failed.");
            handler->stop_requested = true;
            return NULL;
        }
        handler->cap_idx++;

        if (UINT_MAX - handler->read_bytes >= ES3X5_BUFFER_SIZE) {

            handler->read_bytes += ES3X5_BUFFER_SIZE;
        } else {

            ALOGW("read bytes are overflowed.");
        }

        index++;
        index &= ES3X5_BUFFER_COUNT - 1;

        buffer_level = handler->cap_idx + handler->wrt_idx;

        if (buffer_level > ES3X5_BUFFER_COUNT) {

            ALOGW("Capture: buffer overflow (%d/%d)", buffer_level, ES3X5_BUFFER_COUNT);
        }
        sem_post(&handler->sem_read);
    }
    return NULL;
}

/* Thread to write stream to output file */
void *run_writefile(void *arg)
{
    int index = 0;
    int byte_writen;
    struct timespec ts;
    ad_streamer_handler *handler = arg;

    if (handler == NULL) {

        return NULL;
    }
    while (true) {

        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += READ_TIMEOUT_IN_SEC;

        if (sem_timedwait(&handler->sem_read, &ts)) {

            if (handler->read_bytes > handler->written_bytes) {

                ALOGE("Write: timed out.");
                handler->stop_requested = true;
            }
            return NULL;
        }

        byte_writen = full_write(handler->outfile_fd, handler->data[index], ES3X5_BUFFER_SIZE);
        if (byte_writen != ES3X5_BUFFER_SIZE) {

            ALOGE("Error: write has failed.");
            handler->stop_requested = true;
            return NULL;
        }
        handler->wrt_idx--;

        if (UINT_MAX - handler->written_bytes >= ES3X5_BUFFER_SIZE) {

            handler->written_bytes += ES3X5_BUFFER_SIZE;
        } else {

            ALOGW("written bytes are overflowed");
        }

        index++;
        index &= ES3X5_BUFFER_COUNT - 1;
    }
    return NULL;
}

static int isHandlerValidForStart(ad_streamer_handler *handler)
{
    // Check handler
    if (handler->audience_fd < 0 ||
            handler->outfile_fd < 0 ||
            handler->cap_idx != 0 ||
            handler->read_bytes != 0 ||
            handler->stop_requested ||
            handler->written_bytes != 0 ||
            handler->wrt_idx != 0 ||
            handler->is_catpure_started) {

        return -1;
    } else {

        return 0;
    }
}

int start_streaming(ad_streamer_handler *handler)
{
    int rc;
    struct sched_param param;
    pthread_attr_t thread_attr;

    // Check argument
    if (handler == NULL) {

        ALOGE("Null pointer as handler.");
        return -1;
    }

    // Check handler
    if (isHandlerValidForStart(handler) != 0) {

        ALOGE("Invalid streaming handler");
        return -1;
    }

    // Initialze semaphore and threads
    if (sem_init(&handler->sem_read, 0, 0) != 0) {

        ALOGE("Initialzing sem_read semaphore failed");
        return -1;
    }
    if (sem_init(&handler->sem_start, 0, 0) != 0) {

        ALOGE("Initialzing sem_start semaphore failed");
        goto destroy_sem_read;
    }
    if (pthread_attr_init(&thread_attr) != 0) {

        ALOGE("Initialzing thread attr failed");
        goto destroy_sem_start;
    }
    if (pthread_attr_setschedpolicy(&thread_attr, SCHED_RR) != 0) {

        ALOGE("Initialzing thread policy failed");
        goto destroy_attr;
    }
    param.sched_priority = sched_get_priority_max(SCHED_RR);
    if (pthread_attr_setschedparam (&thread_attr, &param) != 0) {

        ALOGE("Initialzing thread priority failed");
        goto destroy_attr;
    }
    if (pthread_create(&handler->pt_capture, &thread_attr, run_capture, handler) != 0) {

        ALOGE("Creating read thread failed");
        goto destroy_attr;
    }
    if (pthread_create(&handler->pt_write, NULL, run_writefile, handler) != 0) {

        ALOGE("Creating write thread failed");
        goto join_capture;
    }
    acquire_wake_lock(PARTIAL_WAKE_LOCK, ad_streamer_id);

    // Start Streaming
    rc = full_write(handler->audience_fd, startStreamCmd, sizeof(startStreamCmd));
    if (rc < 0) {

        ALOGE("Audience start stream command failed: %s (%d)",
              strerror(errno),
              rc);

        release_wake_lock(ad_streamer_id);
        goto join_write;
    }

    // Let thread start recording
    sem_post(&handler->sem_start);
    handler->is_catpure_started = true;

    pthread_attr_destroy(&thread_attr);
    return 0;

join_write:
    pthread_join(handler->pt_write, NULL);

join_capture:
    pthread_join(handler->pt_capture, NULL);

destroy_attr:
    pthread_attr_destroy(&thread_attr);

destroy_sem_start:
    sem_destroy(&handler->sem_start);

destroy_sem_read:
    sem_destroy(&handler->sem_read);

    return -1;
}

int stop_streaming(ad_streamer_handler *handler)
{

    int rc;

    // Check argument
    if (handler == NULL) {

        ALOGE("Null pointer as handler.");
        return -1;
    }
    // Check handler
    if (handler->audience_fd < 0 ||
            handler->outfile_fd < 0 ||
            handler->is_catpure_started != true) {

        ALOGE("Invalid streaming handler");
        return -1;
    }

    release_wake_lock(ad_streamer_id);

    // Stop Streaming
    rc = full_write(handler->audience_fd, stopStreamCmd, sizeof(stopStreamCmd));
    if (rc < 0) {

        ALOGE("audience stop stream command failed: %d", rc);
        return -1;
    }

    // Because there is no way to know whether audience I2C buffer is empty, sleep is required.
    usleep(THREAD_EXIT_WAIT_IN_US);
    handler->stop_requested = true;

    ALOGD("Stopping capture.");
    pthread_join(handler->pt_capture, NULL);
    ALOGD("Capture stopped.");
    pthread_join(handler->pt_write, NULL);

    sem_destroy(&handler->sem_read);
    sem_destroy(&handler->sem_start);

    ALOGV("Total of %d bytes read", handler->written_bytes);

    clear_streaming_handler(handler);

    return 0;
}
