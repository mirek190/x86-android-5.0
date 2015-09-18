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
/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include "atdebug.h"
#include "atcmd.h"
#include "atp_util.h"
#include "serial.h"
#include "mmgr_cli.h"
#include <stdbool.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <hardware_legacy/power.h>
#include <sys/reboot.h>
#include <poll.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/netlink.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "apb.h"

#ifdef USE_FRAMEWORK_GTI
#include "IGtiProxy.h"
#endif

static int pc_fd     = -1;
static int md_fd     = -1;
static int device_fd = -1;

/* forward to modem (may be processed on AP first and then forwarded to modem; or may be just forwarded to modem) */
#define PROXY_TO_MODEM 0
/* implemented on AP (nothing forwarded to modem) */
#define APE_COMMAND 1

#ifdef USE_FRAMEWORK_GTI
/* forward to AP GTI */
#define PROXY_TO_GTI 2

#define GTI_RSP_BUFFER_NB_OF_ELEMENT 2048
#endif

/* filtered by AT command proxy as part of the commands that are considered as not supported by the platform */
#define APE_FILTERED -1

/* atproxy need to do nothing for this cmd & just forward */
#define TRANSPARENT_CMD -1
/* atproxy need to do nothing for this response */
#define TRANSPARENT_RESPONSE -1

#define UEVENT_MSG_LEN  1024

#define ATPROXY_NAME "atproxy"
#define NAME_LEN 16

enum {
    ROUTE_TO_MODEM,
    ROUTE_TO_OTHER_1
};

/* switch_route_command is a manual switch that allows at@ commands to be sent
 * at alternative components. */
static int switch_route_command = ROUTE_TO_MODEM;
static volatile int ate_flag    = 0;
static volatile int cur_atcmd   = AT_MAX;
static volatile unsigned char ats0_value        = 0;
static volatile unsigned char ats0_set_value    = 0;
static volatile int thread_created  = 0;
static volatile int modem_hangup_flag = 0;

mmgr_cli_handle_t *mmgr_hdl;
e_mmgr_events_t modem_status = E_MMGR_EVENT_MODEM_DOWN;
static pthread_t s_host_modem_state_machine;


/* receive & send data buffer size */
#define BUF_SIZE                1024
#define CONFIG_MSG_MAX_LEN      128

#define RETRY_MAX               30      /* retry to open tty node max times */
#define POLL_RETRY_MAX          10      /* retry to send when poll timeout or failure */
#define RESEND_ATCMD_MAX        10      /* retry to send AT command max times */

static char buf[BUF_SIZE]   =   {0};

#define ATP_TTY_TYPE_PC         0
#define ATP_TTY_TYPE_MODEM      1

const char *tty_pc          = "/dev/ttyGS0";
const char *tty_modem   = "/dev/gsmtty10";
const char *devpath     = "gsmtty10";
static char rild_prop[PROPERTY_VALUE_MAX];

const char *tunnel_switch   = "off";

const char *modem_config_file       = "/modem_config_file";
const char *usb_sys_file            = "/sys/devices/pci0000:00/0000:00:02.3/hsm";

enum {
    USB_CONNECTED,
    USB_DISCONNECTED,
};
enum {
    MODEM_ACCESSED,
    MODEM_DISACCESSED,
};

static int host_state = USB_DISCONNECTED;
static int modem_state = MODEM_DISACCESSED;

struct uevent {
    const char *action;
    const char *path;
    const char *subsystem;
    const char *firmware;
    const char *partition_name;
    const char *usb_state;
    int partition_num;
    int major;
    int minor;
};

static int open_uevent_socket(void)
{
    struct sockaddr_nl addr;
    int sz = 64*1024; // XXX larger? udev uses 16MB!
    int on = 1;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(s < 0)
        return -1;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));
    setsockopt(s, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));

    if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }

    return s;
}

static void parse_event(const char *msg, struct uevent *uevent)
{
    uevent->action = "";
    uevent->path = "";
    uevent->subsystem = "";
    uevent->firmware = "";
    uevent->major = -1;
    uevent->minor = -1;
    uevent->partition_name = NULL;
    uevent->usb_state = NULL;
    uevent->partition_num = -1;
    /* currently ignoring SEQNUM */
    while(*msg) {
        if(!strncmp(msg, "ACTION=", 7)) {
            msg += 7;
            uevent->action = msg;
        } else if(!strncmp(msg, "DEVPATH=", 8)) {
            msg += 8;
            uevent->path = msg;
        } else if(!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            uevent->subsystem = msg;
        } else if(!strncmp(msg, "FIRMWARE=", 9)) {
            msg += 9;
            uevent->firmware = msg;
        } else if(!strncmp(msg, "MAJOR=", 6)) {
            msg += 6;
            uevent->major = atoi(msg);
        } else if(!strncmp(msg, "MINOR=", 6)) {
            msg += 6;
            uevent->minor = atoi(msg);
        } else if(!strncmp(msg, "PARTN=", 6)) {
            msg += 6;
            uevent->partition_num = atoi(msg);
        } else if(!strncmp(msg, "PARTNAME=", 9)) {
            msg += 9;
            uevent->partition_name = msg;
        } else if(!strncmp(msg, "USB_STATE=", 10)) {
            msg += 10;
            uevent->usb_state = msg;
        }

        /* advance to after the next \0 */
        while(*msg++)
            ;
    }
}

static void fdclose()
{
    if (md_fd != -1) {
        close(md_fd);
        md_fd = -1;
    }
    if (pc_fd != -1) {
        /* Add errno trace in case that "unable write usb value" problem of BZ4161 is reproduced.*/
        if (close(pc_fd) < 0) {
            ALOGE("close(pc_fd:%d) failed, error: %d, %s\n", pc_fd, errno, strerror(errno));
        }
        pc_fd = -1;
    }
}

static void close_all()
{
    fdclose();
}

/*
 * Main signal handler to end correctly atproxy
 *
 * input:
 *      int sig: signal id
 * output:
 *      none
 */
static void sighandler(int sig)
{
    ALOGD(TAG"signal %d\n", sig);

    mmgr_cli_disconnect(mmgr_hdl);
    mmgr_cli_delete_handle(mmgr_hdl);
    /* Close all file descriptors */
    ALOGD(TAG"close all file descriptors and exit.");
    close_all();

    exit(0);
}

static int check_atcmd_format(int atcmd, const char *line)
{
    char buf_tmp[BUF_SIZE] = {0};
    int i = 0;
    for (; *line; line++) {
        while (' ' == *line)
            line++;
        buf_tmp[i++] = *line;
    }

    i = strlen(atcmd_name[atcmd]);
    if (buf_tmp[i] == '\r' || buf_tmp[i] == '\n' || buf_tmp[i] == ';')
        return 0;

    return -1;
}

#ifdef USE_FRAMEWORK_GTI
static void fw_at_to_gti(const char *line)
{
    char rsp_buf[GTI_RSP_BUFFER_NB_OF_ELEMENT] = "";

    if (!gti_handle_at_command(line, rsp_buf, sizeof(rsp_buf))) {
        return;
    }

    if (write(pc_fd, rsp_buf, strlen(rsp_buf)) < 0) {
        ALOGE(TAG"write to %s: %s(%d)\n", tty_pc, strerror(errno), errno);
    }
}
#endif


static int  get_atcmd_value(const char *buf)
{
    char *p = strstr(buf, "=");
    if (p == NULL || p+1 == NULL)
        return -1;

    return strtoul(p+1, 0, 10);
}

static void power_shutdown() {
    if (md_fd != -1) {
        const char* SHUT_DOWN_CMD = "at+cfun=0\r\n";
        write(md_fd, SHUT_DOWN_CMD, strlen(SHUT_DOWN_CMD) );

        /* Need to give modem time to send detach request and cleanly shutdown */
        sleep( 5 );
    }
    sync();
    reboot(RB_POWER_OFF);
}

static int open_tty_port(int* return_fd, const char * tty_name, int tty_type)
{
    int (*p_func)(const char*, int);
    int fd = -1;
    int retry_count = 0;

    if (tty_type == ATP_TTY_TYPE_PC) {
        p_func = open_pc_serial;
    } else if (tty_type == ATP_TTY_TYPE_MODEM) {
        p_func = open_serial;
    } else {
        return -1;
    }

    fd = (*p_func)(tty_name, B115200);
    while (fd < 0) {
        if ((EAGAIN == errno || ENODEV == errno
          || ENOENT == errno) && retry_count < RETRY_MAX) {
            sleep(3);
            ALOGD(TAG"Retry open %s\n", tty_name);
            fd = (*p_func)(tty_name, B115200);
            retry_count++;
        } else {
            ALOGD(TAG"Cannot open %s\n", tty_name);
            *return_fd = fd;
            return fd;
        }
    }
    ALOGD(TAG"TTY %s opened\n", tty_name);
    *return_fd = fd;
    return fd;
}

static void modem_state_detect()
{
    if (modem_status == E_MMGR_EVENT_MODEM_UP) {
        {
            ALOGD(TAG"Modem is accessed\n");
        }
        modem_state = MODEM_ACCESSED;
    }
    else {
        if (modem_status == E_MMGR_EVENT_MODEM_DOWN) {
            ALOGD(TAG"Modem cannot be accessed\n");
        }
        modem_state = MODEM_DISACCESSED;
    }
}

int mdm_up(mmgr_cli_event_t *ev)
{
    ALOGD("Received MODEM_UP");
    modem_status = ev->id;
    modem_state_detect();
    return 0;
}

int mdm_dwn(mmgr_cli_event_t *ev)
{
    ALOGD("Received MODEM_DOWN");
    modem_status = ev->id;
    modem_state_detect();
    return 0;
}

static void* host_modem_state_machine()
{
    ALOGD(TAG"host state machine is launched\n");

    device_fd = open_uevent_socket();
    if(device_fd < 0)
        return NULL;

    fcntl(device_fd, F_SETFD, FD_CLOEXEC);

    /* Init the host_state and modem_state */
    modem_state_detect();

    for(;;) {
        char msg[UEVENT_MSG_LEN+2];
        char cred_msg[CMSG_SPACE(sizeof(struct ucred))];
        struct iovec iov = {msg, sizeof(msg)};
        struct sockaddr_nl snl;
        struct msghdr hdr = {&snl, sizeof(snl), &iov, 1, cred_msg, sizeof(cred_msg), 0};

        ssize_t n = recvmsg(device_fd, &hdr, 0);
        if (n <= 0) {
            ALOGD(TAG"uevent loop exit\n");
            break;
        }

        if ((snl.nl_groups != 1) || (snl.nl_pid != 0)) {
            /* ignoring non-kernel netlink multicast message */
            continue;
        }

        struct cmsghdr * cmsg = CMSG_FIRSTHDR(&hdr);
        if (cmsg == NULL || cmsg->cmsg_type != SCM_CREDENTIALS) {
            /* no sender credentials received, ignore message */
            continue;
        }

        struct ucred * cred = (struct ucred *)CMSG_DATA(cmsg);
        if (cred->uid != 0) {
            /* message from non-root user, ignore */
            continue;
        }

        if(n >= UEVENT_MSG_LEN)   /* overflow -- discard */
            continue;

        msg[n] = '\0';
        msg[n+1] = '\0';

        struct uevent uevent;
        parse_event(msg, &uevent);
        if(strncmp(uevent.subsystem, "android_usb", strlen("android_usb")) == 0) {
            if(strncmp(uevent.usb_state, "CONFIGURED", 10) == 0) {
                host_state = USB_CONNECTED;
                ALOGD(TAG"Host is Connected\n");
            } if(strncmp(uevent.usb_state, "DISCONNECTED", 13) == 0) {
                host_state = USB_DISCONNECTED;
                ALOGD(TAG"Host is Disconnected\n");
            }
        }
        if(strstr(uevent.path, devpath) != NULL) {
            modem_state_detect();
        }
    }
    return NULL;
}

void reset_modem() {
    mmgr_cli_requests_t request;
    mmgr_cli_restart_t restart;

    MMGR_CLI_INIT_REQUEST(request, E_MMGR_REQUEST_MODEM_RESTART);
    memset(&restart, 0, sizeof(restart));
    request.data = &restart;
    mmgr_cli_send_msg(mmgr_hdl, &request);
}

static int atcmd_dispatch(int atcmd, const char *line)
{
    int ret;
    unsigned long value = 0;
    char *p;

    ret = -1;

    switch (atcmd) {

    case ATD:
        ALOGD(TAG"ATD\n");
        ret = PROXY_TO_MODEM;
        break;

        /* FIXME */
    case ATS7:
        ALOGD(TAG"ATS7\n");
        if (write(pc_fd, "\r\nOK\r\n", 6) < 0) {
            ALOGE(TAG"write to %s\n", tty_pc);
            return -1;
        }
        ret = APE_COMMAND;
        break;

    case AT_AND:
        ALOGD(TAG"AT&\n");
        if (write(pc_fd, "\r\nOK\r\n", 6) < 0) {
            ALOGE(TAG"write to %s\n", tty_pc);
            return -1;
        }
        ret = APE_COMMAND;
        break;

    case ATE:
        ALOGD(TAG"ATE\n");
        if ((p = strstr(line, "E")) == NULL)
            p = strstr(line, "e");
        if (p != NULL)
            value = strtoul(p+1, 0, 10);
        if (value == 1) {
            ate_flag = 1;
            if (write(pc_fd, "\r\nOK\r\n", 6) < 0) {
                ALOGE(TAG"write to %s\n", tty_pc);
                return -1;
            }
        } else if (value == 0 && str_starts_with(line, "ATE0") != 0) {
            ate_flag = 0;
            if (write(pc_fd, "\r\nOK\r\n", 6) < 0) {
                ALOGE(TAG"write to %s\n", tty_pc);
                return -1;
            }
        } else {
            if (write(pc_fd, "\r\nERROR\r\n", 9) < 0) {
                ALOGE(TAG"write to %s\n", tty_pc);
                return -1;
            }
        }
        ret = APE_COMMAND;
        break;

    case ATA:
        ALOGD(TAG"ATA\n");
        ret = PROXY_TO_MODEM;
        break;


    case ATH:
        ALOGD(TAG"ATH\n");
        ret = PROXY_TO_MODEM;
        break;

    case AT_CHLD:
        ALOGD(TAG"AT_CHLD\n");
        ret = PROXY_TO_MODEM;
        break;

    case AT_CHUP:
        ALOGD(TAG"AT+CHUP\n");
        ret = PROXY_TO_MODEM;
        break;

    case AT_CBC:
        ALOGD(TAG"AT+CBC\n");
        /* FIXME: at+cbc not supported yet (+cmee error: 4) */
        if (write(pc_fd, "\r\nERROR\r\n", 9) < 0) {
            ALOGE(TAG"write to %s\n", tty_pc);
            return -1;
        }
        ret = APE_COMMAND;
        break;

    case AT_CLAC:
        ALOGD(TAG"AT+CLAC\n");
        ret = check_atcmd_format(AT_CLAC, line);
        if (ret == -1) {
            if (write(pc_fd, "\r\n\r\nERROR\r\n\r\n", 13) < 0) {
                ALOGE(TAG"write to %s\n", tty_pc);
                return -1;
            }
            return ret;
        }
        ret = PROXY_TO_MODEM;
        break;

    case ATS0:
        ALOGD(TAG"ATS0\n");
        ret = get_atcmd_value(line);
        if(ret >= 0) {
            /* The ATS0 value range is 0-255 according to AT Function Specification */
            ats0_set_value = (unsigned char)ret;
            ALOGD(TAG"ats0 is %d\n", (int)ats0_set_value);
        }
        ret = PROXY_TO_MODEM;
        break;
    case AT_CFUN:
        ALOGD(TAG"AT+CFUN\n");
        ret = get_atcmd_value(line);
        if (ret == 0 && strstr(line, "?") == NULL) {
            if (write(pc_fd, "\r\nOK\r\n", 6) < 0) {
                ALOGE(TAG"write to %s\n", tty_pc);
                return -1;
            }
            reset_modem();
            ret = APE_COMMAND;
        } else {
            ret = PROXY_TO_MODEM;
        }
        break;
#ifdef USE_FRAMEWORK_GTI
    case AT_AT:
        ALOGD(TAG"AT@\n");
        if (strncasecmp("AT@MODEM", line, 8) == 0) {
            switch_route_command = ROUTE_TO_MODEM;
            if (write(pc_fd, "\r\nOK\r\n", 6) < 0) {
                ALOGE(TAG"write to %s\n", tty_pc);
                return -1;
            }
            ret = APE_COMMAND;
        } else if (strncasecmp("AT@MEDIA", line, 8) == 0) {
            switch_route_command = ROUTE_TO_OTHER_1;
            if (write(pc_fd, "\r\nOK\r\n", 6) < 0) {
                ALOGE(TAG"write to %s\n", tty_pc);
                return -1;
            }
            ret = APE_COMMAND;
        } else {
            switch (switch_route_command) {
            case ROUTE_TO_OTHER_1:
                ret = PROXY_TO_GTI;
                break;
            case ROUTE_TO_MODEM:
            default:
                ret = PROXY_TO_MODEM;
                break;
            }
        }
        break;
#endif
    default:
        if (strstr(line, "+++ATH") != NULL || strstr(line, "at#ud") != NULL) {
            if (write(pc_fd, "\r\nOK\r\n", 6) < 0) {
                ALOGE(TAG"write to %s\n", tty_pc);
                return -1;
            }
            ret = APE_COMMAND;
        } else
            ret = PROXY_TO_MODEM;
        break;
    }
    cur_atcmd = atcmd;
    return ret;
}

static int atcmd_parse(const char *buf)
{
    unsigned int i = 0;
    for (i = 0; i < sizeof(atcmd_name) / sizeof(atcmd_name[0]); i++) {
        if (1 == str_starts_with(buf, atcmd_name[i]))
            return i;
    }
    return TRANSPARENT_CMD;
}

static int write_at_command(int fd, const char* tty_name, const char * at_cmd, char* buf, int buf_size)
{
    int send_cnt = 0, poll_cnt = 0;
    int result;

    if (buf_size <= 1)
        return -1;

    memset(buf, 0, buf_size);
    do {
        send_cnt++;
        if (send_cnt > 1) {
            if ((send_cnt > RESEND_ATCMD_MAX) || \
                (modem_state == MODEM_DISACCESSED) || \
                (host_state == USB_DISCONNECTED)) {
                return -1;
            } else {
                sleep(3);
            }
        }

        /*FIXME: writing too early on the tty provokes EAGAIN
         this needs a proper fix
         for now, just sleep a bit and try again!
         */
tryagain:
        ALOGD(TAG"Write %s to %s\n", at_cmd, tty_name);
        if (write(fd, at_cmd, strlen(at_cmd)) < 0 || write(fd, "\r\n", 2) < 0) {
            ALOGE(TAG"Write %s error : %s !\n", tty_name, strerror(errno));
            if(errno == EAGAIN)
            {
                sleep(1);
                goto tryagain;
            }
            return -1;
        }

        result = wait_for_data (fd, 3000 );
        if (result == AT_CMD_OK ) {
            if (read(fd, buf, buf_size-1) < 0) {
                ALOGE(TAG"Read response from %s error, error num is %d\n", tty_name, errno);
                return -1;
            }
        } else if (result == AT_CMD_RESEND) {
            poll_cnt++;
            ALOGD(TAG"Wait %s result, poll %s failed %d times\n", at_cmd, tty_name, poll_cnt);
            sleep(2);
            if (poll_cnt >= POLL_RETRY_MAX)
                return -1;
        } else {
            ALOGD(TAG"Wait %s result, poll %s failed and need reopen.\n", at_cmd, tty_name);
            modem_hangup_flag = 1;
            return -1;
        }

        buf[buf_size - 1]='\0';
        ALOGD(TAG"Response of %s << %s\n", at_cmd, buf);
    } while (result != AT_CMD_OK);
    ALOGD(TAG"write %s to modem successfully.\n", at_cmd);
    return 0;
}

/*
 * Main
 *
 * AT-Proxy main function.
 *
 */
int main(int argc, char *argv[])
{
    const char *line = NULL;
    struct pollfd fds[3];
    unsigned int num_fds = sizeof(fds)/sizeof(fds[0]);
    apb_handle_t *apb = apb_init();
    char client_name[NAME_LEN] = {0};
    int tunnel_mode = 0, count = 0, poll_cnt = 0, i = 0, data_mode = 0, saved_tunnel_mode = 0;
    int inst_id = 1;
    int atcmd, ret;

    struct sigaction sigact;

    if (!apb)
        ALOGI(TAG"APB is disabled\n");

    /* Parse arguments */
    for (i = 1; i < argc ;) {
        if (0 == strcmp(argv[i], "-m") && (argc - i > 1)) {
            tty_modem = argv[i + 1];
            //devpath = strstr(tty_modem, "gsmtty");
            //if (NULL == devpath) {
            //    goto err;
            //}
            i += 2;
        } else if (0 == strcmp(argv[i], "-p") && (argc - i > 1)) {
            tty_pc = argv[i + 1];
            i += 2;
        } else if (0 == strcmp(argv[i], "-d") && (argc - i > 1)) {
            i += 2;
        } else if (0 == strcmp(argv[i], "-f") && (argc - i > 1)) {
            modem_config_file = argv[i + 1];
            i += 2;
        } else if (0 == strcmp(argv[i], "-t") && (argc - i > 1)) {
            tunnel_switch = argv[i + 1];
            if (0 == strncmp(tunnel_switch, "on", 2)) {
                tunnel_mode = 1;
            }
            i += 2;
        } else if (0 == strcmp(argv[i], "-i") && (argc - i > 1)) {
            inst_id = strtol(argv[i + 1], NULL, 0);

            ALOGD(TAG"Proxy main inst_id=%d\n", inst_id);
            i += 2;
        }
    }

    if (1 == inst_id) {
        snprintf(client_name, NAME_LEN, "%s", ATPROXY_NAME);
    } else {
        snprintf(client_name, NAME_LEN, "%s%d", ATPROXY_NAME, inst_id);
    }

    /* Set signal handler */
    if (sigemptyset(&sigact.sa_mask) == -1)
        exit(-1);

    sigact.sa_flags = 0;
    sigact.sa_handler = sighandler;

    if (sigaction(SIGINT, &sigact, NULL) == -1)
        exit(-1);
    if (sigaction(SIGHUP, &sigact, NULL) == -1)
        exit(-1);
    if (sigaction(SIGTERM, &sigact, NULL) == -1)
        exit(-1);

    if (tunnel_mode)
        ALOGD(TAG"Run in tunnel mode\n");

    /* Start the host and modem state machine thread */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&s_host_modem_state_machine, &attr, host_modem_state_machine, NULL);
    if (ret < 0) {
        ALOGE(TAG"pthread_create failed for host state machine : [%s]\n",
                strerror(errno));
        sighandler(SIGTERM);
    }

    mmgr_hdl = NULL;
    mmgr_cli_create_handle(&mmgr_hdl, client_name, NULL);
    mmgr_cli_set_instance(mmgr_hdl, inst_id);
    mmgr_cli_subscribe_event(mmgr_hdl, mdm_up, E_MMGR_EVENT_MODEM_UP);
    mmgr_cli_subscribe_event(mmgr_hdl, mdm_dwn, E_MMGR_EVENT_MODEM_DOWN);

    while (mmgr_cli_connect(mmgr_hdl) != E_ERR_CLI_SUCCEED) {
        sleep(1);
        ALOGD(TAG"Retry to connect to MMGR");
    }
    //mmgr_cli_lock(mmgr_hdl);

    /*
     * Waiting loop before start at-proxy
     *
     */
    for (;;) {
        /* Waiting for Modem and USB */
        /* wait for dev.bootcompleted sets to "1" */
        /* So that we do not start the at-proxy too early */
        /* According to bz 28989 */
        char boot_status[PROPERTY_VALUE_MAX];
        char prop_proxy[PROPERTY_VALUE_MAX];
        char prop_usb[PROPERTY_VALUE_MAX];
        int ats0_ret = 0;
        property_get("dev.bootcomplete", boot_status, "-1");
        if ( strcmp(boot_status, "1") ||
             modem_state == MODEM_DISACCESSED || host_state == USB_DISCONNECTED) {
            sleep(1);
            continue;
        }
        modem_hangup_flag = 0;

        /* Verify that we have set the usb.config properly, */
        if ( property_get("persist.system.at-proxy.mode", prop_proxy, NULL) &&
                property_get("sys.usb.config", prop_usb, NULL) ) {
            if ((0 == strcmp(prop_proxy, "1")) || (0 == strcmp(prop_proxy, "3"))) {
                ALOGD(TAG"Start atproxy,prop_proxy = 1 or 3\n");
                system("am startservice com.intel.atproxy/com.intel.atproxy.ATService");
            }

            /* at-proxy mode <> "0" and usb config <> "rndis,acm,adb" */
            if (strcmp(prop_proxy, "0") && strcmp(prop_usb, "rndis,acm,adb")) {

                /* persist.system.at-proxy.mode set to 1 or 2 and
                   sys.usb.config not in correct mode */
                sleep(1);
                ALOGD(TAG"re-setting usb.config to rndis,acm.adb\n");
                property_set("sys.usb.config", "rndis,acm,adb");
                sleep(1);
            }
        }

        /* Open PC host tty port */
        if (open_tty_port(&pc_fd, tty_pc, ATP_TTY_TYPE_PC) < 0) {
            goto err;
        }

        if (!tunnel_mode) {
            /* Waiting for first AT command on PC host */
            for (;;) {
                ALOGD(TAG"Wait AT commands from %s.\n", tty_pc);
                count = 0;
                poll_cnt = 0;
                do {
                    ret = wait_for_data (pc_fd, -1);
                    if (ret == AT_CMD_OK ) {
                        if ((count = read(pc_fd, buf, sizeof(buf))) < 0) {
                            ALOGE(TAG"read from %s\n", tty_pc);
                            goto err;
                        }
                    } else if (ret == AT_CMD_RESEND) {
                        poll_cnt++;
                        ALOGD(TAG"Poll %s failed %d times.\n", tty_pc, poll_cnt);
                        sleep(2);
                        if (poll_cnt >= POLL_RETRY_MAX)
                            goto err;

                    } else {
                        ALOGD(TAG"Poll %s failed and need reopen.\n", tty_pc);
                        goto err;
                    }
                } while (count <= 0);

                line = readline(buf, count, 0);
                if (line != NULL) {
                    ALOGD(TAG"send %zu bytes:>> %s\n", strlen(line), line);
                    if (strncmp(line, "AT", 2) == 0 || strncmp(line, "at", 2) == 0) {
                        break;
                    }
                }
            }
        }

        /* Open modem tty  port */
        if ((modem_state == MODEM_DISACCESSED) ||
            (open_tty_port(&md_fd, tty_modem, ATP_TTY_TYPE_MODEM) < 0)) {
            modem_hangup_flag = 1;
            goto err;
        }

        if (!tunnel_mode) {
            /* first,write ate0 to modem for we need to readline & echo at the same time */
            if (write_at_command(md_fd, tty_modem, "ate0", buf, BUF_SIZE) < 0 ) {
                goto err;
            }

            /* we read ats0 current value for restarting this app sometimes */
            if (write_at_command(md_fd, tty_modem, "ats0?", buf, BUF_SIZE) < 0 ) {
                goto err;
            }

            if ((strlen(buf) > 0) && (sscanf(buf, "%d", &ats0_ret) > 0)) {
                    ats0_value = (unsigned char)ats0_ret;
                    ALOGD(TAG"ats0_value is %d\n", (int)ats0_value);
            }
            else {
                ats0_value = 0;
                ALOGE(TAG"ats0_value can't be read, set to %d\n", (int)ats0_value);
            }

            /* Send the first command that kicked this off to modem or apb */
            if (apb_fwd_msg(apb, line, strlen(line)) == -1) {
                ALOGD(TAG"send %zu bytes:>> %s\n", strlen(line), line);
                atcmd = atcmd_parse(line);
                ret = atcmd_dispatch(atcmd, line);
                if (ret == PROXY_TO_MODEM) {
                    if (write(md_fd, line, strlen(line)) < 0) {
                        ALOGE(TAG"write to %s\n", tty_modem);
                        goto err;
                    }
                }
            }
        }

        fds[0].fd = pc_fd;
        fds[0].events = POLLIN;
        fds[1].fd = md_fd;
        fds[1].events = POLLIN;
        fds[2].fd = apb_get_fd(apb);
        fds[2].events = POLLIN;

        /* Main loop for pooling PC and Modem file descriptors */
        for (;;) {

            if (modem_state == MODEM_DISACCESSED) {
                    ALOGE(TAG"Main loop: device %s hung up\n", tty_modem);
                    modem_hangup_flag = 1;
                    goto err;
            }
            fds[0].revents = 0;
            fds[1].revents = 0;
            fds[2].revents = 0;
            memset(buf, 0, sizeof(buf));

            ret = poll(fds, num_fds, -1);

            /* interrupted by signal */
            if ((ret == -1) && (errno == EINTR)) {
                ALOGD(TAG"Main loop: receive signal.\n");
                if (fds[0].revents & SIGINT) {
                    sighandler(SIGINT);
                } else if (fds[0].revents & SIGTERM) {
                    sighandler(SIGTERM);
                } else {
                    ALOGD(TAG"Main loop: receive signal other than SIGINT or SIGTERM, continue.\n");
                    continue;
                }
            } else if (ret <= 0) {
                ALOGD(TAG"Main loop: poll failed, goto error.\n");
                goto err;
            }

            /* * * process data from pc_fd * * */
            if (fds[0].revents) {
                if (fds[0].revents & (POLLHUP | POLLERR)) {
                    ALOGE(TAG"Main loop: device %s hung up\n", tty_pc);
                    goto err;

                } else if (fds[0].revents & POLLIN) {

                    if ((count = read(pc_fd, buf, sizeof(buf)-1)) <= 0) {
                        ALOGE(TAG"read from %s\n", tty_pc);
                        goto err;
                    }

                    if (tunnel_mode == 1) {
                        if ((count = write(md_fd, buf, count)) < 0) {
                            ALOGE(TAG"write to %s\n", tty_modem);
                            goto err;
                        }
                    } else {
                        line = readline(buf, count, 0);
                        if (line != NULL) {

                            if (!apb_fwd_msg(apb, line, strlen(line)))
                                continue;

                            ALOGD(TAG"send %zu bytes:>> %s\n", strlen(line), line);
                            atcmd = atcmd_parse(line);
                            ret = atcmd_dispatch(atcmd, line);
                            if (ret == PROXY_TO_MODEM) {
                                if (write(md_fd, line, strlen(line)) < 0) {
                                    ALOGE(TAG"write to %s\n", tty_modem);
                                    goto err;
                                }
#ifdef USE_FRAMEWORK_GTI
                            } else if (ret == PROXY_TO_GTI) {
                                ALOGD(TAG"write to GTI\n");
                                fw_at_to_gti(line);
#endif
                            }
                        }

                        if (ate_flag == 1) {
                            if (write(pc_fd, buf, count) < 0) {
                                ALOGE(TAG"write to %s\n", tty_pc);
                                goto err;
                            }
                        }
                    }
                } else {
                    /* not sure we will ever get here */
                    ALOGE(TAG"Main loop: unexpected %s poll event (%d).\n", tty_pc, fds[0].revents);
                    continue;
                }
            }

            /* * * process data from md_fd * * */
            if (fds[1].revents) {
                if (fds[1].revents & POLLHUP) {
                    ALOGE(TAG"Main loop: device %s hung up\n", tty_modem);
                    modem_hangup_flag = 1;
                    goto err;

                } else if (fds[1].revents & POLLIN) {

                    if ((count = read(md_fd, buf, sizeof(buf)-1)) <= 0) {
                        ALOGE(TAG"error reading from %s: count= %d, errno=%d\n", tty_modem,
                              count, errno);
                        goto err;
                    }

                    if (tunnel_mode == 1) {
                        if (data_mode == 1) {
                            /* In data mode, if we receive NO CARRIER, this means that we need to
                               switch back to control mode.*/
                            if (check_no_carrier(buf, count)) {
                                ALOGD(TAG"return to control mode");
                                tunnel_mode = saved_tunnel_mode;
                                data_mode = 0;
                            }
                        }
                        if ((count = write(pc_fd, buf, count)) < 0) {
                            ALOGE(TAG"write to %s\n", tty_pc);
                            goto err;
                        }
                    } else {
                        line = readline(buf, count, 1);
                        if (line != NULL) {
                            ALOGD(TAG"receive %zu bytes:<< %s\n", strlen(line), line);
                            /* If we receive connect, this means that we switch to data mode,
                               fallback to tunnel mode remembering data_mode in order to return
                               to non_tunnel mode at end of data session.*/
                            if (strstr(line, "CONNECT")) {
                                ALOGD(TAG"switch to data mode");
                                saved_tunnel_mode = tunnel_mode;
                                tunnel_mode = 1;
                                data_mode = 1;
                            }
                            if (write(pc_fd, line,  strlen(line)) < 0) {
                                ALOGE(TAG"write to %s\n", tty_pc);
                                goto err;
                            }
                        }
                    }
                } else {
                    /* not sure we will ever get here */
                    ALOGE(TAG"Main loop: unexpected %s poll event (%d).\n", tty_modem,
                          fds[1].revents);
                    continue;
                }
            }

            /* handle APB notification */
            if (fds[2].revents) {
                /* consume event */
                char c;
                read(fds[2].fd, &c, sizeof(c));

                /* get answer */
                char *msg = apb_get_answer(apb);
                if (msg) {
                    write(pc_fd, msg, strlen(msg));
                    free(msg);
                }
            }
        }
err:
        fdclose();

        if (modem_hangup_flag) {
            ALOGD(TAG"modem hangup, just close %s.\n", tty_modem);
            modem_hangup_flag = 0;
        }

        sleep(3);
    }

    close_all();
    apb_dispose(apb);
    return 0;
}
