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

#include "atdebug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

/* at command max stored length */
#define BUF_LEN 1024
/* buf max number */
#define BUF_MAX_NUM 3

static char cur_line[BUF_MAX_NUM][BUF_LEN];

static const char *find_next_eol(const char *cur, int len)
{
    while (*cur != '\0' && *cur != '\r' && *cur != '\n' && *cur != 0x1A && len) {
        cur++;
        len--;
    }
    if (len > 0) {
        return cur;
    }
    return NULL;
}

/* No carrier may come in two different read() from modem, so we need to
 * create a statefull detector of the NO CARRIER modem answer that comes
 * when the PPP session is ended. */
int check_no_carrier(const char *buf, int len) {
    static const char *NO_CARRIER = "NO CARRIER";
    static unsigned int matches = 0;
    int i;

    for (i = 0; i < len; i++) {
        if (buf[i] == NO_CARRIER[matches]) {
            matches++;
        } else {
            matches = 0;
        }

        if (matches == strlen(NO_CARRIER)) {
            matches = 0;
            return 1;
        }
    }
    return 0;
}

/* returns 1 if line starts with prefix, 0 if it does not */
int str_starts_with(const char *line, const char *prefix)
{
    for ( ; *line != '\0' && *prefix != '\0'; line++, prefix++) {
        while (' ' == *line)
            line++;
        if (*line != *prefix) {
            if (*line != *prefix + ' ' || *line < 'a' || *line > 'z')
                return 0;
        }
    }
    return *prefix == '\0';
}

const char *readline(const char *buf, int len, int num)
{
    static volatile int cur_len = 0;
    const char *cur;
    cur = find_next_eol(buf, len);
    if (cur == NULL) {
        /* if line is bigger than BUF_LEN, we just discard it */
        if (cur_len + len > BUF_LEN)
            cur_len = 0;
        memcpy(cur_line[num] + cur_len, buf, len);
        cur_len += len;
        /* no one line read completely */
        return NULL;
    } else {
        if (cur_len + len > BUF_LEN)
            cur_len = 0;
        memcpy(cur_line[num] + cur_len, buf, len);
        memset(cur_line[num] + cur_len + len, 0, sizeof(cur_line[num]) - cur_len - len);
        cur_len = 0;
    }
    return cur_line[num];
}
