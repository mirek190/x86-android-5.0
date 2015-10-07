/* Telephony Config Selector (TCS) - mmgr data
**
** Copyright (C) Intel 2013
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
*/

#ifndef __TCS_MMGR_HEADER__
#define __TCS_MMGR_HEADER__

#include <limits.h>
#include "stdbool.h"
#include "tcs_config.h"
#include "linux/mdm_ctrl.h"

#define SYSFS_CMD_LEN 16

#define MCDR_PROTOCOL \
    X(UNKNOWN), \
    X(LCDP), \
    X(YMODEM), \

#define MCD_CTRL \
    X(UNKNOWN), \
    X(IOCTL), \
    X(NETLINK), \

typedef enum e_mcdr_protocol {
#undef X
#define X(a) E_MCDR_ ## a
    MCDR_PROTOCOL
} e_mcdr_protocol_t;

typedef enum e_mdm_ctrl {
#undef X
#define X(a) E_MCD_ ## a
    MCD_CTRL
} e_mdm_ctrl_t;

typedef struct link_uart {
    char device[PATH_MAX];
    int baudrate;
} link_uart_t;

typedef struct link_hsi {
    char device[PATH_MAX];
    char ctrl[PATH_MAX];
    char cmd[SYSFS_CMD_LEN];
} link_hsi_t;

typedef struct link_usb {
    char device[PATH_MAX];
    int pid;
    int vid;
} link_usb_t;

typedef struct link_shm {
    char device[PATH_MAX];
} link_shm_t;

typedef struct link_spi {
    char device[PATH_MAX];
    bool high_speed;
} link_spi_t;

typedef struct power {
    char device[PATH_MAX];
    char on[SYSFS_CMD_LEN];
    char off[SYSFS_CMD_LEN];
} power_t;

typedef struct link_ctrl {
    char device[PATH_MAX];
    char on[SYSFS_CMD_LEN];
    char off[SYSFS_CMD_LEN];
    char reset[SYSFS_CMD_LEN];
} link_ctrl_t;

typedef struct mux {
    int frame_size;
    int retry;
} mux_t;

typedef struct link {
    e_link_t type;
    union {
        link_uart_t uart;
        link_hsi_t hsi;
        link_usb_t usb;
        link_shm_t shm;
        link_spi_t spi;
    };
} link_t;

typedef struct mcdr_gnl {
    bool enable;
    e_mcdr_protocol_t protocol;
    char path[PATH_MAX];
    int timeout;
    bool enable_cd_log;
} mcdr_gnl_t;

typedef struct mcdr_info {
    mcdr_gnl_t gnl;
    link_t link;
    power_t power;
    link_ctrl_t ctrl;
} mcdr_info_t;

typedef struct mmgr_mdm_link {
    link_t flash_ebl;
    link_t flash_fw;
    link_t baseband;
    link_t reconfig_usb;
    power_t power;
    link_ctrl_t ctrl;
} mmgr_mdm_link_t;

typedef struct mmgr_fw_folders {
    char input[PATH_MAX];
    char runtime[PATH_MAX];
    char factory[PATH_MAX];
    char injection[PATH_MAX];
} mmgr_fw_folders_t;

typedef struct mmgr_fw_nvm {
    char sta[NAME];
    char dyn[NAME];
    char cal[NAME];
} mmgr_fw_nvm_t;

typedef struct mmgr_fw_rnd {
    char cert[NAME];
} mmgr_fw_rnd_t;

typedef struct mmgr_fw {
    mmgr_fw_folders_t folders;
    mmgr_fw_nvm_t nvm;
    mmgr_fw_rnd_t rnd;
} mmgr_fw_t;

typedef struct mmgr_com {
    /* @TODO: for future platform without MUX, an enum and union should be used
     * here */
    mux_t mux;
} mmgr_com_t;

typedef struct mmgr_recovery {
    bool enable;
    int cold_reset;
    int reboot;
    int reset_delay;
    int reboot_delay;
    int cold_timeout;
    int shtdwn_timeout;
} mmgr_recovery_t;

typedef struct mmgr_timings {
    int ipc_ready;
    int cd_ipc_reset;
    int cd_ipc_ready;
    int mdm_flash;
    int fmmo;
} mmgr_timings_t;

typedef struct mmgr_clients {
    int max;
} mmgr_clients_t;

typedef struct mmgr_mcd {
    e_mdm_ctrl_t interface;
    enum mdm_ctrl_board_type board;
    char path[PATH_MAX];
} mmgr_mcd_t;

typedef struct mmgr_info {
    mmgr_mdm_link_t mdm_link;
    mmgr_mcd_t mcd;
    mmgr_fw_t fw;
    mmgr_com_t com;
    mmgr_timings_t timings;
    mmgr_recovery_t recov;
    mmgr_clients_t cli;
    mcdr_info_t mcdr;
} mmgr_info_t;

#endif                          // __TCS_MMGR_HEADER__
