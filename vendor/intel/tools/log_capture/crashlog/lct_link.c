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
 * @file lct_link.c
 * @brief File containing functions for getting user space events.
 */

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <resolv.h>
#include <linux/kct.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <lct.h>
#include "cutils/properties.h"
#include "privconfig.h"
#include "crashutils.h"
#include "fsutils.h"
#include "ct_utils.h"

int sock_fd = -1;

static int lct_server_init(void)
{
    const struct sockaddr_un addr = {
            .sun_family = AF_UNIX,
            .sun_path = SK_NAME,
    };

    int s, e;
    s = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (s >= 0)
    if (bind(s, (struct sockaddr* const) &addr, sizeof(addr))) {
        e = errno; close(s); errno = e;
        s = -1;
    }

    return s;
}

static struct ct_event *lct_link_get_event(int sock_fd)
{
    struct ct_event *ev;
    char buf[sizeof(*ev)];
    ssize_t n;
    size_t len;

    do {
            n = recv(sock_fd, buf, sizeof(buf), MSG_PEEK);
    } while (n < 0 && errno == EINTR);

    if (n < 0)
        return NULL;

    len = ((struct ct_event*)buf)->attchmt_size + sizeof(*ev);

    ev = malloc(len);
    if (ev == NULL)
        return NULL;

    do {
        n = recv(sock_fd, ev, len, 0);
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        free(ev);
        return NULL;
    }

    return ev;
}

void lct_link_init_comm(void)
{
    if ((sock_fd = lct_server_init()) < 0) {
        ALOGE("can't open userland socket: %s", strerror(errno));
    }
}

int lct_link_get_fd() {
    return sock_fd;
}

void lct_link_handle_msg(void)
{
    struct ct_event *ev;
    ev = lct_link_get_event(sock_fd);

    if (ev == NULL) {
        ALOGE("Could not receive userland packet: %s", strerror(errno));
        return;
    }

    if (event_pass_filter(ev) == TRUE) {
        /* Process Kernel user space message */
        process_msg(ev);
    }
    free(ev);
}
