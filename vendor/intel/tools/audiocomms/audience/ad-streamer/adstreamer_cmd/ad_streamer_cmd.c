/*
 **
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
#include "ad_streamer.h"
#include "ad_streamer_cmd.h"

#include <full_rw.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


#define ES305_CH_PRI_MIC         0x1
#define ES305_CH_SEC_MIC         0x2
#define ES305_CH_CLEAN_SPEECH    0x4
#define ES305_CH_FAR_END_IN      0x8
#define ES305_CH_FAR_END_OUT     0x10

#define ES325_CH_PRI_MIC         0x1
#define ES325_CH_SEC_MIC         0x2
#define ES325_CH_THIRD_MIC       0x3
#define ES325_CH_FOURTH_MIC      0x4
#define ES325_CH_FAR_END_INPUT   0x5
#define ES325_CH_AUDIO_INPUT_1   0x6
#define ES325_CH_AUDIO_INPUT_2   0x7
#define ES325_CH_AUDIO_INPUT_3   0x8
#define ES325_CH_AUDIO_INPUT_4   0x9
#define ES325_CH_UI_TONE_1       0xA
#define ES325_CH_UI_TONE_2       0xB
#define ES325_CH_SPK_REF_1       0xC
#define ES325_CH_SPK_REF_2       0xD
#define ES325_CH_CLEAN_SPEECH    0x10
#define ES325_CH_FAR_END_OUT_1   0x11
#define ES325_CH_FAR_END_OUT_2   0x12
#define ES325_CH_AUDIO_OUTPUT_1  0x13
#define ES325_CH_AUDIO_OUTPUT_2  0x14
#define ES325_CH_AUDIO_OUTPUT_3  0x15
#define ES325_CH_AUDIO_OUTPUT_4  0x16

#define ES3X5_CHANNEL_SELECT_FIELD 3

/* Default values for optional command arguments */
#define DEFAULT_OUTPUT_FILE    "/data/ad_streamer_capture.bin"
#define DEFAULT_CAPTURE_DURATION_IN_SEC 5

/* Chip ID code */
#define AD_CHIP_ID_ES305     0x1008
#define AD_CHIP_ID_ES325     0x1101


static const char* chipVersion2String[] = {
    "es305",
    "es325",
    "Unknown"
};

static const unsigned char setChannelCmdes305[AD_CMD_SIZE] = { 0x80, 0x28, 0x00, 0x03 };
static const unsigned char setChannelCmdes325[AD_CMD_SIZE] = { 0x80, 0x28, 0x80, 0x03 };
static const unsigned char chipIdCmd[AD_CMD_SIZE] = { 0x80, 0x0E, 0x00, 0x00 };

static void cleanup(int audience_fd, int outfile_fd, ad_streamer_handler *handler)
{

    if (audience_fd >= 0) {

        close(audience_fd);
    }

    if (outfile_fd >= 0) {

        close(outfile_fd);
    }

    free_streaming_handler(handler);
}

static int update_audience_chip_version(ad_streamer_cmd_t *streaming_cmd)
{

    int status = -1;
    int rc;
    unsigned int chipId = 0;
    unsigned char chipIdCmdResponse[AD_CMD_SIZE];
    int audience_fd = -1;

    if (streaming_cmd == NULL) {

        return -1;
    }

    audience_fd = open(ES3X5_DEVICE_PATH, O_RDWR);

    if (audience_fd < 0) {

        printf("Cannot open %s: %s (%d)\n",
               ES3X5_DEVICE_PATH,
               strerror(errno),
               errno);
        return -1;
    }

    /* Send the identification command */
    rc = full_write(audience_fd, chipIdCmd, sizeof(chipIdCmd));
    if (rc < 0) {

        printf("Audience command failed (Chip Identification): %s\n",
               strerror(errno));
        goto close_fd;
    }
    /* Wait for command execution */
    usleep(AD_CMD_DELAY_US);
    /* Read back the response */
    rc = full_read(audience_fd, chipIdCmdResponse, sizeof(chipIdCmdResponse));
    if (rc < 0) {

        printf("Audience command response read failed (Chip ID): %s\n",
               strerror(errno));
        goto close_fd;
    }
    if (chipIdCmdResponse[0] == chipIdCmd[0] && chipIdCmdResponse[1] == chipIdCmd[1]) {

        /* Retrieve the chip ID from the response: */
        chipId = chipIdCmdResponse[2] << 8 | chipIdCmdResponse[3];

        /* Check the chip ID */
        if (chipId == AD_CHIP_ID_ES325) {

            streaming_cmd->ad_chip_version = AUDIENCE_ES325;
        } else if (chipId == AD_CHIP_ID_ES305) {

            streaming_cmd->ad_chip_version = AUDIENCE_ES305;
        } else {

            /* Unknown ID means unsupported chip */
            printf("Unsupported Audience chip ID: 0x%04x\n", chipId);
            streaming_cmd->ad_chip_version = AUDIENCE_UNKNOWN;
            goto close_fd;
        }
    } else {

        printf("Audience chip ID command failed. Chip returned 0x%02x%02x%02x%02x\n",
               chipIdCmdResponse[0],
                chipIdCmdResponse[1],
                chipIdCmdResponse[2],
                chipIdCmdResponse[3]);
        /* Unable to detect the chip */
        streaming_cmd->ad_chip_version = AUDIENCE_UNKNOWN;
        goto close_fd;
    }


    printf("Audience chip selected: '%s'.\n", chipVersion2String[streaming_cmd->ad_chip_version]);
    status = 0;

close_fd:
    close (audience_fd);
done:
    return status;
}

static void display_cmd_help(void)
{

    printf("Format: [-t seconds] [-f /path/filename] [-c chip] channelId [channelId]\n");
    printf("\nArguments Details\n");
    printf("\tchannelId:\tAudience channel to be captured (see channel IDs)\n");
    printf("\t[channelId]:\tOptionnal second Audience channel to be captured (see channel IDs)\n");
    printf("\tf (optional):\tOutput file path (default: %s)\n", DEFAULT_OUTPUT_FILE);
    printf("\tt (optional):\tCapture duration in seconds (default: %d)\n",
           DEFAULT_CAPTURE_DURATION_IN_SEC);
    printf("\tc (optional) (deprecated):\tforce chip selection in <%s|%s> (default: chip"
           "autodetection)\n\n",
           chipVersion2String[AUDIENCE_ES305],
           chipVersion2String[AUDIENCE_ES325]);

    printf("\nChannel IDs for %s\n", chipVersion2String[AUDIENCE_ES305]);
    printf("\tPrimary Mic   = %d\n", ES305_CH_PRI_MIC);
    printf("\tSecondary Mic = %d\n", ES305_CH_SEC_MIC);
    printf("\tClean Speech  = %d\n", ES305_CH_CLEAN_SPEECH);
    printf("\tFar End In    = %d\n", ES305_CH_FAR_END_IN);
    printf("\tFar End out   = %d\n", ES305_CH_FAR_END_OUT);
    printf("\nChannel IDs for %s\n", chipVersion2String[AUDIENCE_ES325]);
    printf("\tPrimary Mic   = %d\n", ES325_CH_PRI_MIC);
    printf("\tSecondary Mic = %d\n", ES325_CH_SEC_MIC);
    printf("\tThird Mic     = %d\n", ES325_CH_THIRD_MIC);
    printf("\tFourth Mic    = %d\n", ES325_CH_FOURTH_MIC);
    printf("\tFar End In    = %d\n", ES325_CH_FAR_END_INPUT);
    printf("\tAudio in 1    = %d\n", ES325_CH_AUDIO_INPUT_1);
    printf("\tAudio in 2    = %d\n", ES325_CH_AUDIO_INPUT_2);
    printf("\tAudio in 3    = %d\n", ES325_CH_AUDIO_INPUT_3);
    printf("\tAudio in 4    = %d\n", ES325_CH_AUDIO_INPUT_4);
    printf("\tUI tone 1     = %d\n", ES325_CH_UI_TONE_1);
    printf("\tUI tone 2     = %d\n", ES325_CH_UI_TONE_2);
    printf("\tSpk ref 1     = %d\n", ES325_CH_SPK_REF_1);
    printf("\tSpk ref 2     = %d\n", ES325_CH_SPK_REF_2);
    printf("\tClean Speech  = %d\n", ES325_CH_CLEAN_SPEECH);
    printf("\tFar End out 1 = %d\n", ES325_CH_FAR_END_OUT_1);
    printf("\tFar End out 2 = %d\n", ES325_CH_FAR_END_OUT_2);
    printf("\tAudio out 1   = %d\n", ES325_CH_AUDIO_OUTPUT_1);
    printf("\tAudio out 2   = %d\n", ES325_CH_AUDIO_OUTPUT_2);
    printf("\tAudio out 3   = %d\n", ES325_CH_AUDIO_OUTPUT_3);
    printf("\tAudio out 4   = %d\n", ES325_CH_AUDIO_OUTPUT_4);
    printf("\nExample\n");
    printf("\tad_streamer -t 10 -f /sdcard/aud_stream.bin 1 2\n\n\tCapture Primary Mic and "
           "Secondary Mic for 10 seconds to /sdcard/aud_stream.bin\n");
    printf("\nNotes\n");
    printf("\t* Dual channels capture requires the I2C bus to be overclocked to have enough "
           "bandwidth (refer to ad_streamer user guide).\n");
    printf("\t* This tool shall be used only while in call (CSV or VOIP) (refer to ad_streamer "
           "user guide).\n");
}

/** Initialize a streaming cmd structure with default values */
static void init_streaming_cmd(ad_streamer_cmd_t *streaming_cmd)
{
    unsigned int channel;

    if (streaming_cmd == NULL) {

        return;
    }

    streaming_cmd->ad_chip_version = AUDIENCE_UNKNOWN;
    streaming_cmd->captureChannelsNumber = 0;

    for (channel = 0; channel < MAX_CAPTURE_CHANNEL; channel++) {

        streaming_cmd->capture_channels[channel] = 0;
    }

    streaming_cmd->duration_in_sec = DEFAULT_CAPTURE_DURATION_IN_SEC;

    strlcpy(streaming_cmd->fname, DEFAULT_OUTPUT_FILE, MAX_FILE_PATH_SIZE);
}

/** Check if the streaming cmd is valid or not. */
static int is_valid_streaming_cmd(const ad_streamer_cmd_t *streaming_cmd)
{
    if (streaming_cmd == NULL) {

        return -1;
    }

    if (streaming_cmd->ad_chip_version >= AUDIENCE_UNKNOWN ||
            streaming_cmd->captureChannelsNumber == 0 ||
            streaming_cmd->captureChannelsNumber > MAX_CAPTURE_CHANNEL ||
            streaming_cmd->duration_in_sec == 0) {

        return -1;
    }
    return 0;
}

int streamer_cmd_from_command_line(int argc,
                                   char *argv[],
                                   ad_streamer_cmd_t *streaming_cmd)
{
    char* cvalue = NULL;
    char* string;
    int index;
    int c;

    /* Reset getopt() API */
    opterr = 0;
    optind = 0;

    if (streaming_cmd == NULL || argv == NULL) {

        return -1;
    }

    init_streaming_cmd(streaming_cmd);

    /* Parse optional parameters */
    while ((c = getopt(argc, argv, "t:f:c:")) != -1) {

        switch (c)
        {
            case 'f':
                cvalue = optarg;
                strlcpy(streaming_cmd->fname, cvalue, MAX_FILE_PATH_SIZE);
                break;
            case 'c':
                cvalue = optarg;
                if (strcmp(cvalue, "es305") == 0) {

                    streaming_cmd->ad_chip_version = AUDIENCE_ES305;
                } else if (strcmp(cvalue, "es325") == 0) {

                    streaming_cmd->ad_chip_version = AUDIENCE_ES325;
                } else {
                    display_cmd_help();
                    printf("\n*Please use valid chip value\n");
                    return -1;
                }
                break;
            case 't':
                // Cap the duration_in_sec to capture samples (in seconds)
                streaming_cmd->duration_in_sec = strtoul(optarg, &string, 10);
                if (*string != '\0') {

                    display_cmd_help();
                    printf("\n*Please use valid numeric value for seconds\n");
                    return -1;
                }
                break;
            default:
                display_cmd_help();
                return -1;
        }
    }

    /* Non-optional arguments: channel ID to be captured */
    for (index = optind; index < argc; index++) {

        // Set streaming channel flag
        if (streaming_cmd->captureChannelsNumber < MAX_CAPTURE_CHANNEL) {

            streaming_cmd->capture_channels[streaming_cmd->captureChannelsNumber] =
                    strtoul(argv[index], &string, 10);

            if (*string != '\0') {

                printf("\nERROR: please use valid numeric value for channel flag\n");
                display_cmd_help();
                return -1;
            }

            streaming_cmd->captureChannelsNumber++;
        } else {

            printf("ERROR: Too many arguments\n");
            display_cmd_help();
            return -1;
        }
    }
    /* Check that at least one channel has to be recorded */
    if (streaming_cmd->captureChannelsNumber == 0) {

        printf("ERROR: Missing channel ID\n");
        display_cmd_help();
        return -1;
    }

    /* If no chip selected from command line, update it with chip detected */
    if (streaming_cmd->ad_chip_version >= AUDIENCE_UNKNOWN) {

        /* Get the Audience FW version */
        if (update_audience_chip_version(streaming_cmd) < 0) {

            return -1;
        }
    } else {

        printf("Deprecated: Audience chip forced to '%s'.\n",
               chipVersion2String[streaming_cmd->ad_chip_version]);
    }

    return 0;
}

static int setup_streaming(const ad_streamer_cmd_t *streaming_cmd, int audience_fd)

{
    int rc;
    unsigned char setChannelCmd[AD_CMD_SIZE];

    if (streaming_cmd == NULL || audience_fd < 0) {

        return -1;
    }

    /* Set streaming channels */
    switch(streaming_cmd->ad_chip_version) {

        case AUDIENCE_ES305:
            memcpy(setChannelCmd, setChannelCmdes305, sizeof(setChannelCmd));
            /* es305: A single command with a bit field of channels to be recorder
             * In case of single channel capture, channels[1] is equal to zero.
             * Maximum number of channels: MAX_CAPTURE_CHANNEL
             */
            setChannelCmd[ES3X5_CHANNEL_SELECT_FIELD] =
                    streaming_cmd->capture_channels[0] | streaming_cmd->capture_channels[1];

            rc = full_write(audience_fd, setChannelCmd, sizeof(setChannelCmd));
            if (rc < 0) {

                printf("audience set channel command failed: %d\n", rc);
                return -1;
            }
            break;

        case AUDIENCE_ES325:
            memcpy(setChannelCmd, setChannelCmdes325, sizeof(setChannelCmd));
            /* es325: As much command(s) as channel(s) to be recorded
             * Maximum number of channels: MAX_CAPTURE_CHANNEL
             */
            unsigned int channel;

            for (channel = 0; channel < streaming_cmd->captureChannelsNumber; channel++) {

                setChannelCmd[ES3X5_CHANNEL_SELECT_FIELD] = streaming_cmd->capture_channels[channel];
                rc = full_write(audience_fd, setChannelCmd, sizeof(setChannelCmd));
                if (rc < 0) {

                    printf("audience set channel command failed: %d\n", rc);
                    return -1;
                }
            }
            break;

        default:
            return -1;
    }

    return 0;
}

int process_ad_streamer_cmd(const ad_streamer_cmd_t *streaming_cmd)
{
    int rc;
    int audience_fd = -1;
    int outfile_fd = -1;
    ad_streamer_handler *handler = NULL;

    if (streaming_cmd == NULL) {

        return -1;
    }

    /* Initialize */
    audience_fd = open(ES3X5_DEVICE_PATH, O_RDWR);

    if (audience_fd < 0) {

        printf("Cannot open %s: %s (%d)\n", ES3X5_DEVICE_PATH, strerror(errno), errno);
        return -1;
    }

    if (is_valid_streaming_cmd(streaming_cmd) == -1) {

        cleanup(audience_fd, outfile_fd, handler);
        return -1;
    }

    outfile_fd = open(streaming_cmd->fname, O_CREAT | O_WRONLY,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (outfile_fd < 0) {

        printf("Cannot open output file %s: %s\n", streaming_cmd->fname, strerror(errno));
        cleanup(audience_fd, outfile_fd, handler);
        return -1;
    }
    printf("Outputing raw streaming data to %s\n", streaming_cmd->fname);

    rc = setup_streaming(streaming_cmd, audience_fd);
    if (rc < 0) {

        cleanup(audience_fd, outfile_fd, handler);
        return -1;
    }
    handler = alloc_streaming_handler(audience_fd, outfile_fd);
    if (handler == NULL) {

        printf("Allocate streaming handler has failed.\n");
        cleanup(audience_fd, outfile_fd, handler);
        return -1;
    }

    /* Let start recording */
    rc = start_streaming(handler);
    if (rc < 0) {

        printf("Start streaming has failed.\n");

        cleanup(audience_fd, outfile_fd, handler);
        return -1;
    }

    printf("\nAudio streaming started, capturing for %d seconds\n", streaming_cmd->duration_in_sec);
    sleep(streaming_cmd->duration_in_sec);

    /* Stop Streaming */
    rc = stop_streaming(handler);
    if (rc < 0) {

        printf("audience stop stream command failed: %d\n", rc);

        cleanup(audience_fd, outfile_fd, handler);
        return -1;
    }

    cleanup(audience_fd, outfile_fd, handler);
    return 0;
}
