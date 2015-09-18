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
#include "ad_i2c.h"
#include "ad_usb_tty.h"
#include "ad_proxy.h"

#define LOG_TAG "ad_proxy"
#include "cutils/log.h"

#include <errno.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <full_rw.h>

#define AD_VERSION "V1.3"

#define ACM_TTY "/dev/ttyGS0"

#define AUDIENCE_ACK_US_DELAY  20000 /* in us */

#define AD_WRITE_DATA_BLOCK_OPCODE   0x802F
#define AD_BAUD_RATE_OPCODE          0x7000

typedef enum {
    STATE_WAIT_COMMAND,
    STATE_READ_ACK,
    STATE_WAIT_DATA_BLOCK,
    STATE_WRITE_COMMAND
} protocol_state_t;

/**
 * @brief Type to contain a 4-bytes Audience command
 */
typedef struct {
    unsigned char bytes[4];
} audience_command_t;


/**
 * Returns the Audience command opcode. The opcode is in the upper 16bits of the command.
 *
 * @param[in] cmd audience_command_t
 *
 * @return integer value of the command 16bits opcode
 */
static unsigned int get_command_opcode(audience_command_t* cmd)
{
    // Opcode is 16bits, HI in byte #0, LO in byte #1
    return cmd->bytes[0] << 8 | cmd->bytes[1];
}
/**
 * Returns the Audience command argument. The argument is in the lower 16bits of the command.
 *
 * @param[in] cmd audience_command_t
 *
 * @return integer value of the command 16bits argument
 */
static unsigned int get_command_arg(audience_command_t* cmd)
{
    // Arg is 16bits, HI in byte #2, LO in byte #3
    return cmd->bytes[2] << 8 | cmd->bytes[3];
}

static void ad_proxy_worker(int usb_tty_fd)

{
    protocol_state_t state = STATE_WAIT_COMMAND;

    audience_command_t audience_cmb_buf = {
        { 0, 0, 0, 0 }
    };

    int data_block_session = 0;
    size_t data_block_size = 0;

    int status;
    int quit = 0;

    while (quit == 0) {

        ALOGD("State: %d", state);
        switch (state) {
            default:
            case STATE_WAIT_COMMAND: {
                // Read a command from AuViD
                status = full_read(usb_tty_fd, audience_cmb_buf.bytes, sizeof(audience_cmb_buf.bytes));
                if (status != sizeof(audience_cmb_buf.bytes)) {

                    ALOGE("%s Read CMD from AuViD failed\n", __func__);
                    quit = 1;
                    break;
                }

                if (get_command_opcode(&audience_cmb_buf) == AD_BAUD_RATE_OPCODE) {

                    // In normal operation, the baud rate command is used as handshake command by
                    // AuViV. This command shall be ignored and just returned back as it to AuViD.
                    status = full_write(usb_tty_fd, audience_cmb_buf.bytes,
                                        sizeof(audience_cmb_buf.bytes));
                    if (status != sizeof(audience_cmb_buf.bytes)) {

                        ALOGE("%s Write to AuVid failed\n", __func__);
                        quit = 1;
                        break;
                    }
                    state = STATE_WAIT_COMMAND;
                } else {

                    // Otherwise, we need to forward the command to the chip
                    state = STATE_WRITE_COMMAND;
                }
                break;
            }
            case STATE_WRITE_COMMAND: {
                // Write the command to the chip
                status = ad_i2c_write(audience_cmb_buf.bytes, sizeof(audience_cmb_buf.bytes));
                if (status != sizeof(audience_cmb_buf.bytes)) {

                    ALOGE("%s Write CMD to chip failed\n", __func__);
                    quit = 1;
                    break;
                }
                // Is it a Data Block command ?
                if (get_command_opcode(&audience_cmb_buf) == AD_WRITE_DATA_BLOCK_OPCODE) {

                    data_block_session = 1;
                    // Block Size is in 16bits command's arg
                    data_block_size = get_command_arg(&audience_cmb_buf);

                    ALOGD("Starting Data Block session for %d bytes", data_block_size);
                }
                // Need to handle the chip ACK response
                state = STATE_READ_ACK;
                break;
            }
            case STATE_READ_ACK: {
                audience_command_t audience_cmb_ack_buf;
                // Wait before to read ACK
                usleep(AUDIENCE_ACK_US_DELAY);
                status = ad_i2c_read(audience_cmb_ack_buf.bytes,
                                     sizeof(audience_cmb_ack_buf.bytes));
                if (status != sizeof(audience_cmb_ack_buf.bytes)) {

                    ALOGE("%s Read ACK from chip failed\n", __func__);
                    quit = 1;
                    break;
                }
                // Send back the ACK to AuViD
                status = full_write(usb_tty_fd, audience_cmb_ack_buf.bytes,
                               sizeof(audience_cmb_ack_buf.bytes));
                if (status != sizeof(audience_cmb_ack_buf.bytes)) {

                    ALOGE("%s Write ACK to AuVid failed\n", __func__);
                    quit = 1;
                    break;
                }
                if (data_block_session == 1) {

                    // Next step is Data Block to handle
                    state = STATE_WAIT_DATA_BLOCK;
                } else {

                    // Next step is to handle a next command
                    state = STATE_WAIT_COMMAND;
                }
                break;
            }
            case STATE_WAIT_DATA_BLOCK: {
                unsigned char* data_block_payload = (unsigned char *) malloc(data_block_size);
                if (data_block_payload == NULL) {

                    ALOGE("%s Read Data Block memory allocation failed.\n", __func__);
                    quit = 1;
                    break;
                }
                // Read data block from AuViD
                status = full_read(usb_tty_fd, data_block_payload, data_block_size);
                if (status != (int)data_block_size) {

                    ALOGE("%s Read Data Block failed\n", __func__);
                    free(data_block_payload);
                    quit = 1;
                    break;
                }

                // Forward these bytes to the chip
                status = ad_i2c_write(data_block_payload, data_block_size);
                free(data_block_payload);

                if (status != (int)data_block_size) {

                    ALOGE("%s Write Data Block failed\n", __func__);
                    quit = 1;
                    break;
                }
                // No more in data block session
                data_block_session = 0;
                // Need to handle ACK of datablock session
                state = STATE_READ_ACK;
                break;
            }
        }
    }
}

int start_proxy(void)
{
    int usb_tty_fd = -1;

    ALOGD("-->ad_proxy startup (Version %s)", AD_VERSION);

    // open the acm tty.
    usb_tty_fd = ad_open_tty(ACM_TTY,  B115200);
    if (usb_tty_fd < 0) {

        ALOGE("%s open %s failed (%s)\n",  __func__, ACM_TTY, strerror(errno));
        return -1;
    }

    // open the audience device node.
    if (ad_i2c_init() < 0) {

        ALOGE("%s open %s failed (%s)\n", __func__, AD_DEV_NODE, strerror(errno));
    } else {

        // Infinite Work
        ad_proxy_worker(usb_tty_fd);
        // Only reached on error
        ALOGE("Exit");
    }

    // close the acm tty
    close(usb_tty_fd);

    return -1;
}
