/*
 ** Copyright 2013 Intel Corporation
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
 */
#pragma once

#include <pthread.h>
#include <semaphore.h>


#ifdef __cplusplus
extern "C" {
#endif

/** Total byte of buffer size would be ES3X5_BUFFER_SIZE * ES3X5_BUFFER_COUNT
 * Audience send 328 bytes for each frame. 164 bytes are half of the frame size.
 * It also need to be less than 256 bytes as the limitation of I2C driver.
 */
#define ES3X5_BUFFER_SIZE        164

/** ES3X5_BUFFER_COUNT must be a power of 2 */
#define ES3X5_BUFFER_COUNT       256

/**
 * The name of the Audience driver
 */
#define ES3X5_DEVICE_PATH    "/dev/audience_es305"

/* Audience command size in bytes */
#define AD_CMD_SIZE              4
/* Delay to wait after a command before to read the chip response (us),
 * as per Audience recommendations. */
#define AD_CMD_DELAY_US          20000

/**
 * Type of handler needed to use the Audience streamer library
 */
typedef struct {
    int audience_fd; /**< File descriptor of Audience driver. */
    int outfile_fd; /**< File descriptor where the stream has to be written to. */
    volatile int stop_requested; /**< Is stop streaming requested ? */
    size_t written_bytes; /**< Number of stream written bytes to output file. */
    size_t read_bytes; /**< Number of stream read bytes from Audience. */
    unsigned char data[ES3X5_BUFFER_COUNT][ES3X5_BUFFER_SIZE]; /**< Stream bytes buffer. */
    sem_t sem_read; /**< Semaphore used to synchronize read and write thread. Once the read thread
                      has read bytes from Audience, it post to this semaphore on which the write
                      thread is waiting on. */
    sem_t sem_start; /**< Semaphore used to start the read thread once streaming is started on
                       Audience chip and all ressources needed for streaming are ready. */
    pthread_t pt_capture; /**< Thread used to capture stream (read from Audience chip). */
    pthread_t pt_write; /**< Thread used to write stream to output file */
    volatile long int cap_idx; /**< Number of bytes frame read from Audience chip */
    volatile long int wrt_idx; /**< Number of bytes frame written to output file */
    int is_catpure_started; /**< Is capture started ? */
} ad_streamer_handler;


/**
 * Allocate a ad_streamer_handler structure to be used with ad_streamer functions.
 *
 * @param[in] audience_fd File descriptor of Audience driver. Must be opened in read+write access.
 * @param[in] output_fd File descriptor of output file. Must be opened in write access at least.
 *
 * @return NULL in case of error, a pointer to the allocated structure otherwise.
 */
ad_streamer_handler *alloc_streaming_handler(int audience_fd, int output_fd);

/**
 * Free a ad_streamer_handler structure used with ad_streamer functions. The handler
 * pointer shall not be used anymore by the caller after calling this function.
 *
 * @param handler[in,out] A pointer to a streaming handler structure
 */
void free_streaming_handler(ad_streamer_handler *handler);

/**
 * Start the streaming on Audience chip sending the start command.
 * The chip must be configured for the desired stream setup before to call start_streaming().
 * The bytes streamed from the chip are written to the file descriptor provided as argument.
 * The stream read operations will be done in a dedicated thread created by the function
 * start_streaming().
 * The write operations to the provided file descriptor will be done in a dedicated thread created
 * by the function start_streaming().
 * The streaming will last until the function stop_streaming() is called.
 *
 * @param handler[in,out] A pointer to a streaming handler structure
 *
 * @return -1 in case of error, 0 otherwise
 */
int start_streaming(ad_streamer_handler *handler);

/**
 * Stop the streaming operation previously started with start_streaming().
 * In case no streaming is on going, the function does nothing and returns 0.
 * The stop command is sent to the Audience chip and then the streaming threads created by
 * the function start_streaming() are stopped.
 *
 * @param handler[in,out] A pointer to a streaming handler structure
 *
 * @return -1 in case of error, 0 otherwise
 */
int stop_streaming(ad_streamer_handler *handler);

#ifdef __cplusplus
}
#endif
