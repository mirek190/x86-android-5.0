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
 * @file usercrash.c
 * @brief File containing functions to process 'generic' events (javacrash,
 * tombstone, hprof and apcore events).
 */

#include "inotify_handler.h"
#include "crashutils.h"
#include "usercrash.h"
#include "dropbox.h"
#include "fsutils.h"

#include "log.h"
#include <stdlib.h>

typedef struct {
    int eventtype;
    char *keyword;
    char *tail;
    char *suffix;
} crashevent_type;

crashevent_type crashevent_fakes_array[] = {
    {TOMBSTONE_TYPE, "bionic-unit-tests-cts", "<<<", "_FAKE"}
};

static void backup_apcoredump(unsigned int dir, char* name, char* path) {

    char des[512] = { '\0', };
    snprintf(des, sizeof(des), "%s%d/%s", CRASH_DIR, dir, name);
    int status = do_copy_tail(path, des, 0);
    if (status < 0)
        LOGE("backup ap core dump status: %d.\n",status);
    else
        remove(path);
}

static char * priv_filter_crashevent(int eventtype, char* path) {
    int entries = sizeof(crashevent_fakes_array) / sizeof(crashevent_fakes_array[0]);

    while (entries--) {
        if ((crashevent_fakes_array[entries].eventtype == eventtype)
                && (find_str_in_file(path, crashevent_fakes_array[entries].keyword, crashevent_fakes_array[entries].tail) == 1)) {
            return crashevent_fakes_array[entries].suffix;
        }
    }
    return "\0";
}

/*
* Name          : __process_usercrash_event
* Description   : processes hprof, apcoredump, javacrash and tombstones events
* Parameters    :
* entry: watch_entry which triggered the notification
* event: inotify_event received
*/
static int priv_process_usercrash_event(struct watch_entry *entry, struct inotify_event *event) {
    char path[PATHMAX];
    char destion[PATHMAX];
    char eventname[PATHMAX];
    char *key;
    int dir;
    /* Check for duplicate dropbox event first */
    if ((entry->eventtype == JAVACRASH_TYPE || entry->eventtype == JAVACRASH_TYPE2 || entry->eventtype == JAVATOMBSTONE_TYPE )
            && manage_duplicate_dropbox_events(event) )
        return 1;

    dir = find_new_crashlog_dir(MODE_CRASH);
    snprintf(path, sizeof(path),"%s/%s", entry->eventpath, event->name);
    if (dir < 0 || !file_exists(path)) {
        if (dir < 0)
            LOGE("%s: Cannot get a valid new crash directory...\n", __FUNCTION__);
        else
            LOGE("%s: Cannot access %s\n", __FUNCTION__, path);
        key = raise_event(CRASHEVENT, entry->eventname, NULL, NULL);
        LOGE("%-8s%-22s%-20s%s\n", CRASHEVENT, key, get_current_time_long(0), entry->eventname);
        free(key);
        return -1;
    }

    snprintf(eventname, sizeof(eventname),"%s%s", entry->eventname, priv_filter_crashevent(entry->eventtype, path));
    snprintf(destion,sizeof(destion),"%s%d/%s", CRASH_DIR, dir, event->name);
    do_copy_tail(path, destion, MAXFILESIZE);
    switch (entry->eventtype) {
        case APCORE_TYPE:
            backup_apcoredump(dir, event->name, path);
            do_log_copy(eventname, dir, get_current_time_short(1), APLOG_TYPE);
            break;
        case TOMBSTONE_TYPE:
        case JAVATOMBSTONE_TYPE:
        case JAVACRASH_TYPE2:
        case JAVACRASH_TYPE:
            usleep(TIMEOUT_VALUE);
            do_log_copy(eventname, dir, get_current_time_short(1), APLOG_TYPE);
            break;
        case HPROF_TYPE:
            remove(path);
            break;
        default:
            LOGE("%s: Unexpected type of event(%d)\n", __FUNCTION__, entry->eventtype);
            break;
    }
    snprintf(destion, sizeof(destion), "%s%d", CRASH_DIR, dir);
    key = raise_event(CRASHEVENT, eventname, NULL, destion);
    LOGE("%-8s%-22s%-20s%s %s\n", CRASHEVENT, key, get_current_time_long(0), entry->eventname, destion);
    switch (entry->eventtype) {
    case TOMBSTONE_TYPE:
    case JAVACRASH_TYPE2:
    case JAVACRASH_TYPE:
#ifdef FULL_REPORT
        if ( start_dumpstate_srv(CRASH_DIR, dir, key) <= 0 )
            /* Didn't start the dumpstate server (already running or failed) */
            free(key);
        break;
#endif
    default:
        /* Event is nor JAVACRASH neither TOMBSTONE : no dumpstate necessary*/
        free(key);
        break;
    }
    return 1;
}

int process_usercrash_event(struct watch_entry *entry, struct inotify_event *event) {

    return priv_process_usercrash_event(entry , event);
}

int process_hprof_event(struct watch_entry *entry, struct inotify_event *event) {

    return priv_process_usercrash_event(entry , event);
}

int process_apcore_event(struct watch_entry *entry, struct inotify_event *event) {

    return priv_process_usercrash_event(entry , event);
}
