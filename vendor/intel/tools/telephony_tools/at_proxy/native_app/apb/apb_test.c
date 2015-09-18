/*
 * Copyright (C) Intel 2014
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

#include <cutils/sockets.h>
#include <stdio.h>
#include <unistd.h>
#include "apb.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int main()
{
    int ret = EXIT_FAILURE;
    int fd = socket_local_client(APB_SOCKET_NAME,
            ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);

    if (fd < 0) {
        fprintf(stderr, "failed to connect to APB\n");
        goto out;
    }

    printf("connected to at-proxy\n");
    while (1) {
        char answer[] = "ATE0\r\n";
        char buf[1024];
        ssize_t err = read(fd, buf, sizeof(buf));
        if (err > 0) {
            ssize_t len = MIN(err, (ssize_t) sizeof(buf) - 1);
            buf[len] = '\0';
            printf("AT command received: \"%s\"\n", buf);
            write(fd, answer, strlen(answer));
        }
    }

out:
    return ret;
}
