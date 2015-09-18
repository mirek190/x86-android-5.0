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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "privconfig.h"
#include "fsutils.h"

#define SYS_SPID_DIR  RESDIR "/sys/spid"
#define SYS_SPID_1    SYS_SPID_DIR "/vendor_id"
#define SYS_SPID_2    SYS_SPID_DIR "/manufacturer_id"
#define SYS_SPID_3    SYS_SPID_DIR "/customer_id"
#define SYS_SPID_4    SYS_SPID_DIR "/platform_family_id"
#define SYS_SPID_5    SYS_SPID_DIR "/product_line_id"
#define SYS_SPID_6    SYS_SPID_DIR "/hardware_id"

static void spid_read_concat(const char *path, char *complete_value) {
    FILE *fd;
    char temp_spid[5]="XXXX";

    fd = fopen(path, "r");
    if (fd != NULL && fscanf(fd, "%s", temp_spid) == 1)
        fclose(fd);
    else
        LOGE("%s: Cannot read %s - %s\n", __FUNCTION__, path, strerror(errno));

    strncat(complete_value, "-", 1);
    strncat(complete_value, temp_spid, sizeof(temp_spid));
}

void read_sys_spid(const char *filename) {
    FILE *fd;
    char complete_spid[256];
    char temp_spid[5]="XXXX";

    if (filename == 0)
        return;

    fd = fopen(SYS_SPID_1, "r");
    if (fd != NULL && fscanf(fd, "%s", temp_spid) == 1)
        fclose(fd);
    else
        LOGE("%s: Cannot read SPID from %s - %s\n", __FUNCTION__, SYS_SPID_1, strerror(errno));

    snprintf(complete_spid, sizeof(complete_spid), "%s", temp_spid);

    spid_read_concat(SYS_SPID_2, complete_spid);
    spid_read_concat(SYS_SPID_3, complete_spid);
    spid_read_concat(SYS_SPID_4, complete_spid);
    spid_read_concat(SYS_SPID_5, complete_spid);
    spid_read_concat(SYS_SPID_6, complete_spid);

    fd = fopen(filename, "w");
    if (!fd) {
        LOGE("%s: Cannot write SPID to %s - %s\n", __FUNCTION__, filename, strerror(errno));
    } else {
        fprintf(fd, "%s", complete_spid);
        fclose(fd);
    }
    do_chown(filename, PERM_USER, PERM_GROUP);
}
