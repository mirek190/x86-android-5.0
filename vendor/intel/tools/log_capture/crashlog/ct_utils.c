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
 * @file ct_utils.c
 * @brief File containing basic operations for crashlog to kernel and
 * crashlog to user space communication
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <resolv.h>
#include "cutils/properties.h"
#include "privconfig.h"
#include "crashutils.h"
#include "trigger.h"
#include "fsutils.h"
#include "ct_utils.h"
#include "ct_eventintegrity.h"

#define BINARY_SUFFIX  ".bin"
#define PROP_PREFIX    "dev.log"

static const char *suffixes[] = {
    [CT_EV_STAT]    = "_trigger",
    [CT_EV_INFO]    = "_infoevent",
    [CT_EV_ERROR]   = "_errorevent",
    [CT_EV_CRASH]   = "_errorevent",
    [CT_EV_LAST]    = "_ignored"
};

static bool copy_file_to_crashdir(char* sourcefile, const char* destpath) {

    char *ptr_filename;               /* Pointer to file in filepath string */
    char destination[PATHMAX];        /* Absolute path to copy input files */

    if (!sourcefile || !destpath)
        return 0;

    ptr_filename = strrchr(sourcefile, '/');

    if (!ptr_filename || !ptr_filename[0])
        return 0;

    /* Start from the character following '/' */
    ptr_filename++;

    /* Destination file name */
    snprintf(destination, sizeof(destination), "%s%s",
             destpath, ptr_filename);
    /* Copy the file */
    return !do_copy_eof(sourcefile, destination);
}

#define FILELIST_MAX_LEN    256    /* max length of filelist string */
#define FILELIST_SEPARATOR  ";,"
static int copy_attached_files(struct ct_event* ev, const char* copy_dst) {

    struct ct_attchmt* att = NULL;    /* KCT attachment structure */
    char filelist[FILELIST_MAX_LEN];  /* Copy of the filelist field string */
    char *ptr_filepath;               /* Pointer for  each file path */
    char *savedstate;                 /* Pointer to save state of strtok_r */

    foreach_attchmt(ev, att) {
        switch (att->type) {
        case CT_ATTCHMT_FILELIST:
            if (!att->size || !att->data)
                break;

            /* Copy the file list into a new array */
            /* because of strtok_r mechanism       */
            strncpy(filelist, att->data, FILELIST_MAX_LEN);

            /* Using comma or semicolon to split the list */
            ptr_filepath = strtok_r(filelist, FILELIST_SEPARATOR, &savedstate);

            while (ptr_filepath != NULL) {
                copy_file_to_crashdir(ptr_filepath, copy_dst);

                /* Get next token */
                ptr_filepath = strtok_r(NULL, FILELIST_SEPARATOR, &savedstate);
            }
            break;
        default:
            break;
        }
    }
    return 0;
}

static int do_additionl_steps(struct ct_event* ev, const char * destination) {
    struct ct_attchmt* att = NULL;
    uint32_t steps;
    char destion[PATHMAX];

    foreach_attchmt(ev, att) {
        if (att->type == CT_ATTCHMT_ADDITIONAL && att->data
                && (att->size == sizeof(uint32_t))) {
            memcpy(&steps, att->data, att->size);
            if ((steps & CT_ADDITIONAL_APLOG) && file_exists(APLOG_FILE_0)) {
                snprintf(destion, sizeof(destion), "%saplog", destination);
                do_copy_tail(APLOG_FILE_0, destion, MAXFILESIZE);
            }

            if (steps & CT_ADDITIONAL_LAST_KMSG) {
                if (file_exists(LAST_KMSG)) {
                    snprintf(destion, sizeof(destion), "%s%s", destination,
                        LAST_KMSG_FILE);
                    do_copy_tail(LAST_KMSG, destion, MAXFILESIZE);
                } else if (file_exists(CONSOLE_RAMOOPS)) {
                    snprintf(destion, sizeof(destion), "%s%s", destination,
                            CONSOLE_RAMOOPS_FILE);
                    do_copy_tail(CONSOLE_RAMOOPS, destion, MAXFILESIZE);
                }
                if (file_exists(FTRACE_RAMOOPS)) {
                    snprintf(destion, sizeof(destion), "%s%s", destination,
                            FTRACE_RAMOOPS_FILE);
                    do_copy_tail(FTRACE_RAMOOPS, destion, MAXFILESIZE);
                }
            }

            if ((steps & CT_ADDITIONAL_FWMSG) &&
                    file_exists(CURRENT_PROC_ONLINE_SCU_LOG_NAME)) {
                snprintf(destion, sizeof(destion), "%s%s.txt", destination,
                        ONLINE_SCU_LOG_NAME);
                do_copy_eof(CURRENT_PROC_ONLINE_SCU_LOG_NAME, destion);
            }
        }
    }
    return 0;
}

/*
* Replace all occurrences of 'from' char with
* 'to' within the given null terminated 'buff'
*/
static void strrplc(char *buff, char from, char to) {
    char *ptr = buff;
    while (*ptr) {
        if (*ptr == from)
            *ptr = to;
        ptr++;
    }
}

int event_pass_filter(struct ct_event *ev) {

    char submitter[PROPERTY_KEY_MAX];
    char propval[PROPERTY_VALUE_MAX];

    if (ev->type >= CT_EV_LAST) {
        LOGE("Unknown event type '%d', discarding", ev->type);
        return FALSE;
    }

    snprintf(submitter, sizeof(submitter), "%s.%s",
            PROP_PREFIX, ev->submitter_name);

    /*
     * to reduce confusion:
     * property can be either ON/OFF for a given submitter.
     * if it's ON, we want event not to be filtered
     * if it's OFF, we want event to be filtered
     * event should be flagged to be manage by this property.
     */
    if (ev->flags & EV_FLAGS_PRIORITY_LOW) {
        if (!property_get(submitter, propval, NULL)) {
            LOGI("Filter out %s, cannot get prop", ev->submitter_name);
            return FALSE;
        }
        if (strcmp(propval, "ON")) {
            LOGI("Filter out %s, OFF", ev->submitter_name);
            return FALSE;
        }
    }
    return TRUE;
}

void process_msg(struct ct_event *ev)
{
    char destination[PATHMAX];
    char name[MAX_SB_N+MAX_EV_N+2];
    char name_event[20];
    e_dir_mode_t mode;
    char *dir_mode;
    int dir;
    char *key;

    /* Temporary implementation: Crashlog handles Kernel CRASH events
     * as if they were Kernel ERROR events
     */
    switch (ev->type) {
    case CT_EV_STAT:
        mode = MODE_STATS;
        dir_mode = STATS_DIR;
        snprintf(name_event, sizeof(name_event), "%s", STATSEVENT);
        break;
    case CT_EV_INFO:
        mode = MODE_STATS;
        dir_mode = STATS_DIR;
        LOGI("%s: Event CT_EV_INFO", __FUNCTION__);
        snprintf(name_event, sizeof(name_event), "%s", INFOEVENT);
        LOGI("%s: Event CT_EV_INFO %s", __FUNCTION__, name_event);
        break;
    case CT_EV_ERROR:
    case CT_EV_CRASH:
        mode = MODE_STATS;
        LOGI("%s: Event CT_EV_CRASH", __FUNCTION__);
        dir_mode = STATS_DIR;
        snprintf(name_event, sizeof(name_event), "%s", ERROREVENT);
        break;
    case CT_EV_LAST:
    default:
        LOGE("%s: unknown event type\n", __FUNCTION__);
        return;
    }

    strrplc(ev->submitter_name, ' ', '_');
    strrplc(ev->ev_name, ' ', '_');
    /* Compute name */
    snprintf(name, sizeof(name), "%s_%s", ev->submitter_name, ev->ev_name);
    /* Convert lower-case name into upper-case name */
    convert_name_to_upper_case(name);

    dir = find_new_crashlog_dir(mode);
    if (dir < 0) {
        LOGE("%s: Cannot get a valid new crash directory...\n", __FUNCTION__);
        key = raise_event(name_event, name, NULL, NULL);
        LOGE("%-8s%-22s%-20s%s\n", name_event, key,
                get_current_time_long(0), name);
        free(key);
        return;
    }

    if (ev->attchmt_size) {
        /* copy binary data into file */
        snprintf(destination, sizeof(destination), "%s%d/%s_%s%s",
                dir_mode, dir,
                ev->submitter_name, ev->ev_name, BINARY_SUFFIX);

        dump_binary_attchmts_in_file(ev, destination);

        snprintf(destination, sizeof(destination), "%s%d/", dir_mode, dir);
        copy_attached_files(ev, destination);
        do_additionl_steps(ev, destination);
        check_event_integrity(ev, destination);
    }

    snprintf(destination, sizeof(destination), "%s%d/%s_%s%s",
            dir_mode, dir,
            ev->submitter_name, ev->ev_name, suffixes[ev->type]);

    /*
     * Here we copy only DATA{0,1,2} in the trig file, because crashtool
     * does not understand any other types. We attach others types in the
     * data file thanks to the function dump_binary_attchmts_in_file();
     */

    dump_data_in_file(ev, destination);

    snprintf(destination, sizeof(destination), "%s%d/", dir_mode, dir);
    key = raise_event(name_event, name, NULL, destination);
    LOGE("%-8s%-22s%-20s%s %s\n", name_event, key,
            get_current_time_long(0), name, destination);
    free(key);
    /* in case of CT crash, create an extra event to upload current logs */
    if (ev->type == CT_EV_CRASH)
        process_log_event(NULL, NULL, MODE_APLOGS);
}

int dump_binary_attchmts_in_file(struct ct_event* ev, char* file_path) {

    struct ct_attchmt* at = NULL;
    char *b64encoded = NULL;
    FILE *file = NULL;
    int nr_binary = 0;

    LOGI("Creating %s\n", file_path);

    file = fopen(file_path, "w+");
    if (!file) {
        LOGE("can't open '%s' : %s\n", file_path, strerror(errno));
        return -1;
    }

    foreach_attchmt(ev, at) {
        switch (at->type) {
        case CT_ATTCHMT_BINARY:
            b64encoded = calloc(1, (at->size+2)*4/3);
            b64_ntop((u_char*)at->data, at->size,
                    b64encoded, (at->size+2)*4/3);
            fprintf(file, "BINARY%d=%s\n", nr_binary, b64encoded);
            ++nr_binary;
            free(b64encoded);
            break;
        case CT_ATTCHMT_DATA0:
        case CT_ATTCHMT_DATA1:
        case CT_ATTCHMT_DATA2:
        case CT_ATTCHMT_DATA3:
        case CT_ATTCHMT_DATA4:
        case CT_ATTCHMT_DATA5:
        case CT_ATTCHMT_FILELIST:
        case CT_ATTCHMT_ADDITIONAL:
        /* Nothing to do */
            break;
        default:
            LOGE("Ignoring unknown attachment type: %d\n", at->type);
            break;
        }
    }

    fclose(file);

    /* No binary data in attachment. File shall be removed */
    if (!nr_binary)
        remove(file_path);

    return 0;
}

int dump_data_in_file(struct ct_event* ev, char* file_path) {

    struct ct_attchmt* att = NULL;
    FILE *file = NULL;

    LOGI("Creating %s\n", file_path);

    file = fopen(file_path, "w+");
    if (!file) {
        LOGE("can't open '%s' : %s\n", file_path, strerror(errno));
        return -1;
    }

    foreach_attchmt(ev, att) {
        switch (att->type) {
        case CT_ATTCHMT_DATA0:
            fprintf(file, "DATA0=%s\n", att->data);
            break;
        case CT_ATTCHMT_DATA1:
            fprintf(file, "DATA1=%s\n", att->data);
            break;
        case CT_ATTCHMT_DATA2:
            fprintf(file, "DATA2=%s\n", att->data);
            break;
        case CT_ATTCHMT_DATA3:
            fprintf(file, "DATA3=%s\n", att->data);
            break;
        case CT_ATTCHMT_DATA4:
            fprintf(file, "DATA4=%s\n", att->data);
            break;
        case CT_ATTCHMT_DATA5:
            fprintf(file, "DATA5=%s\n", att->data);
            break;
        default:
            break;
        }
    }

    fclose(file);

    return 0;
}

void convert_name_to_upper_case(char * name) {

    unsigned int i;

    for (i=0; i<strlen(name); i++) {
        name[i] = toupper(name[i]);
    }
}
