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

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of channel to capture */
#define MAX_CAPTURE_CHANNEL 2

/** Maximum file path/name length in char, including terminal '0' */
#define MAX_FILE_PATH_SIZE 80

/**
 * Type to describe Audience chip version.
 */
typedef enum {
    AUDIENCE_ES305, /**< Audience chip in use is a es305. */
    AUDIENCE_ES325, /**< Audience chip in use is a es325. */
    /* Must be the last element: */
    AUDIENCE_UNKNOWN /**< Audience chip in use is unknown. */
} ad_chip_version_t;

/**
 * Type containing the command description.
 */
typedef struct {

    ad_chip_version_t ad_chip_version; /**< Audience chip version on which the streaming will
                                        *   occur. */
    unsigned int captureChannelsNumber; /**< Number of channel to be captured */
    unsigned int capture_channels[MAX_CAPTURE_CHANNEL]; /**< Channel IDs of the channel(s)
                                                         *   to be captured. */
    unsigned int duration_in_sec; /**< Duration of the capture in seconds. */
    char fname[MAX_FILE_PATH_SIZE]; /**< Capture output file name. */


} ad_streamer_cmd_t;


/**
 * Fill a streaming command as per command line instructions. If the command line instructions are
 * incorrect, the function prints the command help and usage on stdout then returns -1.
 * Optional command line arguments will be set to default if not specified in command line.
 * Moreover, if the Audience chip version is not selected in the command line, an auto-detection
 * will occur to select the correct chip version.
 *
 * @param[in] argc command line argument number.
 * @param[in] argv command line arguments.
 * @param[out] streaming_cmd streaming command to be filled.
 *
 * @return -1 in case of error, 0 otherwise.
 */
int streamer_cmd_from_command_line(int argc,
                                   char *argv[],
                                   ad_streamer_cmd_t *streaming_cmd);

/**
 * Process the Audience Streamer command. If the chip version is not specified in the
 * command description, the function updates it according to the chip detected.
 *
 * @param[in] streaming_cmd streaming command to be executed.
 *
 * @return -1 in case the streamer stops on error, 0 otherwise.
 */
int process_ad_streamer_cmd(const ad_streamer_cmd_t *streaming_cmd);

#ifdef __cplusplus
}
#endif
