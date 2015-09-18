/* Copyright (C) Intel 2014
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

#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "privconfig.h"

int uefivar_get_int(const char *name, const char *guid, unsigned int *value) {
    int ret = 0;
    char *cmd;
    FILE *fp;
    char out[BUFSIZ];

    if (!name || !guid || !value)
        return -EINVAL;

    ret = asprintf(&cmd, "uefivar -n %s -g %s -t int 2> /dev/null", name, guid);
    if (ret == -1) {
        LOGE("%s: failed to build EFI variable cmd for: %s-%s\n",
             __func__, name, guid);
        return -ENOMEM;
    }

    fp = popen(cmd, "r");
    if (!fp) {
        ret = -errno;
        LOGE("%s: popen failed : (%d) %s\n", __func__, errno, strerror(errno));
        goto free_cmd;
    }

    int file_cnt = 0;
    while (fgets(out, BUFSIZ, fp) != NULL)
        file_cnt++;

    ret = pclose(fp);
    if (ret == -1) {
        ret = -errno;
        LOGE("%s: pclose failed\n", __func__);
        goto free_cmd;
    }

    ret = WEXITSTATUS(ret);
    if (ret) {
        LOGE("%s: uefivar command returned error (%d)\n", __func__, ret);
        goto free_cmd;
    }

    if (file_cnt != 1) {
        LOGE("%s: uefivar command returned too many data, last: %s\n",
             __func__, out);
        ret = -EFBIG;
        goto free_cmd;
    }

    char *endptr;
    unsigned long int l = strtoul(out, &endptr, 0);
    if (out[0] == '\0' || *endptr != '\n' || l == ULONG_MAX) {
        LOGE("%s: Failed to read integer %s: %s\n",
             __func__, out, strerror(errno));
        ret = -EINVAL;
        goto free_cmd;
    }

    *value = (int)l;
    if (*value != l) {
        LOGE("%s: read value is too large: 0x%lx (must be lower than 0x%x)\n",
             __func__, l, INT_MAX);
        ret = -EINVAL;
    }

free_cmd:
    free(cmd);

    return ret;
}
