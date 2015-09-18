/* Copyright (C) Intel 2013
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file kct_netlink.c
 * @brief File containing functions for getting Kernel events.
 */

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <resolv.h>
#include <linux/kct.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <cutils/properties.h>
#include "privconfig.h"
#include "kct_netlink.h"
#include "ct_utils.h"

#define CTM_MAX_NL_MSG 4096
int sock_nl_fd = -1;

static struct kct_packet *netlink_get_packet(int fd) {

    struct kct_packet *pkt;
    struct sockaddr_nl nladdr;
    char buf[sizeof(*pkt)];
    socklen_t nladdrlen;
    ssize_t len;

    assert(fd >= 0);

    nladdrlen = sizeof(nladdr);

    /* MSG_PEEK let the pkt in queue; MSG_TRUNK return full pkt length */
    len = recvfrom(fd, buf, sizeof(buf), MSG_PEEK|MSG_TRUNC,
            (struct sockaddr*)&nladdr, &nladdrlen);
    if ((size_t)len < sizeof(buf)) {
        if (len >= 0) {
            /* pop invalid packet from queue so we don't retrieve it later */
            recvfrom(fd, buf, sizeof(buf), MSG_TRUNC,
                    (struct sockaddr*)&nladdr, &nladdrlen);
            errno = EBADE;
        }
        return NULL;
    }

    LOGI("Packet received of size: %d\n", (int)len);

    pkt = malloc(len);
    if (pkt == NULL)
        return NULL;

    /* read full pkt now */
    len = recvfrom(fd, pkt, len, 0, (struct sockaddr*)&nladdr, &nladdrlen);
    if (len > 0)
        /* unless someone else read from the socket, len != 0 */
        if (NLMSG_OK(&pkt->nlh, (size_t)len))
            return pkt;

    free(pkt);
    errno = EBADE;

    return NULL;
}

int netlink_sendto_kct(int fd, int type, const void *data,
        unsigned int size) {

    struct sockaddr_nl addr;
    static int sequence = 0;
    struct kct_packet req;
    int retval;

    if (NLMSG_SPACE(size) > CTM_MAX_NL_MSG) {
        errno = EMSGSIZE;
        return -1;
    }

    /* when about to overflow, start over */
    if (++sequence < 0)
        sequence = 1;

    memset(&req, 0, sizeof(req));
    req.nlh.nlmsg_len = NLMSG_SPACE(size);
    req.nlh.nlmsg_type = type;
    req.nlh.nlmsg_flags = NLM_F_REQUEST;
    req.nlh.nlmsg_seq = sequence;

    if (size && data)
        memcpy(NLMSG_DATA(&req.nlh), data, size);

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;

    do {
        retval = sendto(fd, &req, req.nlh.nlmsg_len, 0,
                (struct sockaddr*)&addr, sizeof(addr));
    } while ((retval < 0) && (errno == EINTR));

    if (retval < 0)
        return -1;

    return sequence;
}

int netlink_init(void) {

    int fd;

    if ((fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_CRASHTOOL)) < 0) {
        ALOGE("socket: %s", strerror(errno));
        return -1;
    }

    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        ALOGE("fcntl: %s", strerror(errno));
        close(fd);
        return -1;
    }

    if (netlink_sendto_kct(fd, KCT_SET_PID, NULL, 0) < 0) {
        ALOGE("ctm_nl_sendto_kct : %s", strerror(errno));
        close(fd);
        return -1;
    }

    LOGD("%s: Netlink intialization succeed.\n", __FUNCTION__);

    return fd;
}

void kct_netlink_init_comm(void) {

    unsigned int connect_try = KCT_MAX_CONNECT_TRY;

    while (connect_try-- != 0) {
        /* Try to connect */
        if ((sock_nl_fd = netlink_init()) != -1)
            break;

        LOGE("%s: Delaying kct_netlink_init_comm\n", __FUNCTION__);

        /* Wait */
        sleep(KCT_CONNECT_RETRY_TIME_S);
    }
}

int kct_netlink_get_fd() {

    return sock_nl_fd;
}

void kct_netlink_handle_msg(void) {

    struct kct_packet *msg;

    msg = netlink_get_packet(sock_nl_fd);
    if (msg == NULL) {
        LOGE("Could not receive kernel packet: %s", strerror(errno));
        return;
    }

    /* Process Kernel message */
    process_msg(&msg->event);
    free(msg);
}

