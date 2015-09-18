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

/**
 * @file eventintegrity.c
 * @brief File containing functions used to check event files integrity.
 *
 * This file contains the functions used to check event files integrity.
 */


#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "privconfig.h"
#include "ct_eventintegrity.h"

/* Defined in the TP2E probe declaration */
#define DATA_MAX_LEN 128        /* max length of data attachments */

static int check_integrity_Fabric_Recov(struct ct_event* ev, char* event_dir) {
    struct ct_attchmt* att = NULL;    /* KCT attachment structure */
    int err = 0;

    int foundAttachedDW0 = 0;
    int foundFileDW0 = 0;

    char data1[DATA_MAX_LEN];
    char fline[DATA_MAX_LEN];
    char filename[PATHMAX];

    FILE *file = NULL;

    /* Step 1:                      */
    /* Find DW0 as data1 attachment */
    foreach_attchmt(ev, att) {
        switch (att->type) {
        case CT_ATTCHMT_DATA1:
            strncpy(data1, att->data, DATA_MAX_LEN);
            foundAttachedDW0 = 1;
            break;
        default:
            break;
        }
    }

    if (!foundAttachedDW0) {
        LOGE("%s: Cannot locate data1 attachment.\n", __FUNCTION__);
        return 1;
    }

    /* Step 2:                                                  */
    /* Find DW0 in fresh copy of /proc/ipanic_fabric_recv_err   */
    snprintf(filename, sizeof(filename), "%s%s", event_dir, FABRIC_RECOV_NAME);

    file = fopen(filename, "r");
    if (!file) {
        LOGE("%s: Cannot open '%s' : %s\n", __FUNCTION__, filename,
                strerror(errno));
        return -1;
    }
    /* Lines can be longer but we only need to read */
    /* the first 'DATA_MAX_LEN' chars to locate     */
    /* the strings we are looking for               */
    while (fgets(fline,DATA_MAX_LEN,file)) {
        if (!strncmp(data1, fline, sizeof("DW0:0x12345678")-1)) {
            foundFileDW0 = 1;
            break;
        }
    }
    fclose(file);

    /* Step 3:                                                  */
    /* If DW0 from attachment and form file match, we're good   */
    /* Else, we don't keep the file                             */
    if (!foundFileDW0) {
        LOGI("%s: Attached file is obsolete: %s\n", __FUNCTION__, filename);
        if (!remove(filename)) {
            err = 2;
        } else {
            LOGE("%s: Failed to delete obsolete file from event dir\n",
                    __FUNCTION__);
            err = -2;
        }
    }

exitvalue:
    /* 0 means everything went well */
    /* Positive error values are harmless, logic anomalies*/
    /* Negative error values are file-related errors */
    return err;
}

int check_event_integrity(struct ct_event* ev, char* event_dir) {

    if (!strncmp(ev->submitter_name, "Fabric", MAX_SB_N)
            && !strncmp(ev->ev_name, "Recov", MAX_EV_N)) {
        return check_integrity_Fabric_Recov(ev, event_dir);
    }

    /* 0 means everything went well */
    /* Positive error values are harmless, logic anomalies*/
    /* Negative error values are file-related errors */
    return 0;
}
