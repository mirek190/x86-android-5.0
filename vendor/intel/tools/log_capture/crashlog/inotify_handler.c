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
 * @file inotify_handler.h
 * @brief File containing functions to handle the crashlogd files/directories
 * watcher.
 *
 * This file contains the functions to handle the crashlogd files/directories
 * watcher.
 * The watcher initialization is performed with a local array containing every
 * kind of watched events. Each event is linked to a directory to be watched and linked
 * to a specific callback processing function.
 * Each time the inotify watcher file descriptor is set, the buffer is read to
 * treat each event it contains. When an event can't be processed normally the
 * buffer content not yet read is dumped and flushed to LOG.
 *
 */

#include "inotify_handler.h"
#include "privconfig.h"
#include "crashutils.h"
#include "dropbox.h"
#include "modem.h"
#include "config_handler.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define LOG_PREFIX "inotify: "

extern pconfig g_first_modem_config;

/**
* @brief structure containing directories watched by crashlogd
*
* This structure contains every watched directories, with the itnotify mask value, and the associated
* filename that should trigger a processing
* note: a watched directory can only have one mask value
*/
struct watch_entry wd_array[] = {
    /* -------------Warning: if table is updated, don't forget to update the  enum in privconfig.h
     * ---          it should be ALWAYS aligned with this enum of event type   */
    {0, DROPBOX_DIR_MASK,   LOST_TYPE,      0,      LOST_EVNAME,        DROPBOX_DIR,        ".lost",                    NULL}, /* for full dropbox */
    {0, DROPBOX_DIR_MASK,   SYSSERVER_TYPE, 0,      SYSSERVER_EVNAME,   DROPBOX_DIR,        "system_server_watchdog",   NULL},
    {0, DROPBOX_DIR_MASK,   ANR_TYPE,       0,      ANR_EVNAME,         DROPBOX_DIR,        "anr",                      NULL},
    {0, TOMBSTONE_DIR_MASK, TOMBSTONE_TYPE, 0,      TOMBSTONE_EVNAME,   TOMBSTONE_DIR,      "tombstone",                NULL},
    {0, DROPBOX_DIR_MASK,   JAVATOMBSTONE_TYPE, 0,  JAVA_TOMBSTONE_EVNAME,   DROPBOX_DIR,   "native_crash",             NULL},
    {0, DROPBOX_DIR_MASK,   JAVACRASH_TYPE, 0,      JAVACRASH_EVNAME,   DROPBOX_DIR,        "app_crash",                NULL},
    {0, DROPBOX_DIR_MASK,   JAVACRASH_TYPE2, 0,     JAVACRASH_EVNAME,   DROPBOX_DIR,        "system_server_crash",      NULL},
    {0, CORE_DIR_MASK,      APCORE_TYPE,    0,      APCORE_EVNAME,      HISTORY_CORE_DIR,   ".core",                    NULL},
    {0, CORE_DIR_MASK,      HPROF_TYPE,     0,      HPROF_EVNAME,       HISTORY_CORE_DIR,   ".hprof",                   NULL},
    {0, STAT_DIR_MASK,      STATTRIG_TYPE,  0,      STATSTRIG_EVNAME,   STAT_DIR,           "_trigger",                 NULL},
    {0, STAT_DIR_MASK,      INFOTRIG_TYPE,  0,      STATSTRIG_EVNAME,   STAT_DIR,           "_infoevent",               NULL},
    {0, STAT_DIR_MASK,      ERRORTRIG_TYPE, 0,      STATSTRIG_EVNAME,   STAT_DIR,           "_errorevent",              NULL},
    {0, APLOG_DIR_MASK,     APLOGTRIG_TYPE, 0,      APLOGTRIG_EVNAME,   APLOG_DIR,          "_trigger",                 NULL},
    {0, APLOG_DIR_MASK,     CMDTRIG_TYPE,   0,      CMDTRIG_EVNAME,     APLOG_DIR,          "_cmd",                     NULL},
    /* -----------------------------above is dir, below is file------------------------------------------------------------ */
    {0, UPTIME_MASK,        UPTIME_TYPE,    0,      UPTIME_EVNAME,      UPTIME_FILE,        NULL,                      NULL},
    /* -------------------------above is AP, below is modem---------------------------------------------------------------- */
    {0, MDMCRASH_DIR_MASK,  MDMCRASH_TYPE,  0,      MDMCRASH_EVNAME,    LOGS_MODEM_DIR,     "mpanic.txt",               NULL},/*for modem crash */
    {0, MDMCRASH_DIR_MASK,  APIMR_TYPE,     0,      APIMR_EVNAME,       LOGS_MODEM_DIR,     "apimr.txt",                NULL},
    {0, MDMCRASH_DIR_MASK,  MRST_TYPE,      0,      MRST_EVNAME,        LOGS_MODEM_DIR,     "mreset.txt",               NULL},
};

int set_watch_entry_callback(unsigned int watch_type, inotify_callback pcallback) {

    if ( watch_type >= DIM(wd_array) ) {
        LOGE(LOG_PREFIX "%s: Cannot set the callback for type %u (max is %lu)\n",
             __FUNCTION__, watch_type, (long unsigned int)DIM(wd_array));
        return -1;
    }

    if ( CRASHLOG_MODE_EVENT_TYPE_ENABLED(g_crashlog_mode, wd_array[watch_type].eventtype) )
        wd_array[watch_type].pcallback = pcallback;
    else
        LOGD(LOG_PREFIX "event type '%s' is disabled : Don't set callback function\n",
             print_eventtype[watch_type] );

    return 0;
}


/**
 * @brief File Monitor module init function
 *
 * Initialize File Monitor module by adding all watched
 * files/directories to the inotify mechanism and expose it in the
 * module FD. The list of watched files/directories is defined by the
 * wd_array global array.
 *
 * @return 0 on success, -1 on failure.
 */
int init_inotify_handler() {

    int fd, i;

#ifndef __TEST__
    fd = inotify_init();
#else
    fd = inotify_init1(IN_NONBLOCK);
#endif
    if (fd < 0) {
        LOGE(LOG_PREFIX "inotify_init failed, %s\n", strerror(errno));
        return -errno;
    }

    for (i = 0; i < (int)DIM(wd_array); i++) {
        int alreadywatched = 0, j;

        if (!CRASHLOG_MODE_EVENT_TYPE_ENABLED(g_crashlog_mode, wd_array[i].eventtype)) {
            LOGI(LOG_PREFIX "event type '%s' disabled : Don't add watch on %s\n",
                 print_eventtype[ wd_array[i].eventtype ], wd_array[i].eventpath);
            continue;
        }

        /* Install watches only for new unwatched paths */
        for (j = 0 ; j < i ; j++) {
            if (!strcmp(wd_array[j].eventpath, wd_array[i].eventpath) ) {
                alreadywatched = 1;
                wd_array[i].wd = wd_array[j].wd;
                LOGV(LOG_PREFIX "Don't duplicate watch operation for %s\n", wd_array[i].eventpath);
                break;
            }
        }
        if (alreadywatched)
            continue;

        wd_array[i].wd = inotify_add_watch(fd, wd_array[i].eventpath,
                                           wd_array[i].eventmask);
        if (wd_array[i].wd < 0) {
            wd_array[i].inotify_error = errno;
            LOGE(LOG_PREFIX "Can't add watch for %s - %s.\n",
                 wd_array[i].eventpath, strerror(wd_array[i].inotify_error));
        } else
            LOGI(LOG_PREFIX "%s, wd=%d has been snooped\n", wd_array[i].eventpath, wd_array[i].wd);
    }
    //add generic watch here
    generic_add_watch(g_first_modem_config, fd);

    return fd;
}

/**
 * @brief Handles treatments to do when one or severals
 * directories that should be watched by crashlogd couldn't
 * have been added to inotify watcher.
 */
void handle_missing_watched_dir() {
    int idx;
    const char * date = get_current_time_long(1);
    /* Raise first an error */
    raise_infoerror(ERROREVENT, CRASHLOG_WATCHER_ERROR);

    /* Then raise an infoevent for each failed watched directory */
    for (idx = 0 ; idx < (int)DIM(wd_array) ; idx++) {
        if (wd_array[idx].wd >= 0 && wd_array[idx].inotify_error == 0) continue; /* Skip if well watched */
        int alreadydone = 0, j;
        /* Skip already treated directories */
        for (j = 0 ; j < idx ; j++) {
            if ( !strcmp(wd_array[j].eventpath, wd_array[idx].eventpath) ) {
                alreadydone = 1;
                break;
            }
        }
        if (alreadydone) continue;
        create_infoevent(CRASHLOG_WATCHER_INFOEVENT, wd_array[idx].eventpath, strerror(wd_array[idx].inotify_error), (char *)date );
    }
}

/**
 * @brief Returns the number of directories that couldn't
 * have been added to inotify watcher.
 *
 * @return : the number of directories not watched.
 */
int get_missing_watched_dir_nb() {
    int idx, res = 0;
    /* Loop on watched directories to */
    for (idx = 0 ; idx < (int)DIM(wd_array) ; idx++) {
        if (wd_array[idx].wd >= 0 && wd_array[idx].inotify_error == 0) continue; /* Skip if well watched */
        int alreadydone = 0, j;
        for (j = 0 ; j < idx ; j++) {
            if ( !strcmp(wd_array[j].eventpath, wd_array[idx].eventpath) ) {
                alreadydone = 1;
                break;
            }
        }
        if (alreadydone) continue; /* Skip if already treated */
        res++;
    }
    return res;
}

/**
 * @brief lists the directories to be listed by crashenv script
 * when one or several directories couldn't have been added to
 * inotify watcher.
 *
 * @param crashenv_param: buffer containing the inotify events
 */
void build_crashenv_dir_list_option( char crashenv_param[PATHMAX] ) {

    int idx = 0, idx_parent = 0;
    char path[PATHMAX] = { 0, };
    char tmp[PATHMAX] = { 0, };
    char parent_array[(int)DIM(wd_array)][PATHMAX];

    crashenv_param[0] = '\0';

    for (idx = 0 ; idx < (int)DIM(wd_array) ; idx++) {
        if (wd_array[idx].wd >= 0 && wd_array[idx].inotify_error == 0) continue; /* Skip if well watched */
        if ( get_parent_dir( wd_array[idx].eventpath, path ) != 0 ) {
            LOGD("%s : error - can't get parent path of %s\n", __FUNCTION__, wd_array[idx].eventpath);
            continue;
        }
        /* Skip path if same parent path already got */
        int alreadydone = 0, j;
        for (j = 0 ; j < idx_parent ; j++) {
            if ( !strcmp(path, parent_array[j]) ) {
                alreadydone = 1;
                break;
            }
        }
        if (alreadydone) continue;
        /* Add path to parent_array to manage duplicates and add path to output param string */
        strncpy(parent_array[idx_parent], path, PATHMAX-1);
        idx_parent++;
        snprintf( tmp, sizeof(tmp), "-l %s ", path);
        if( PATHMAX-1 - strlen(crashenv_param) >= strlen(tmp) )
            strncat( crashenv_param, tmp, PATHMAX-1);
        else
            LOGW("%s : error - can't add path %s in monitor_crashenv -l option : no available space\n",
                    __FUNCTION__, wd_array[idx].eventpath);
    }
    return;
}

static struct watch_entry *get_event_entry(int wd, char *eventname) {
    int idx;
    for (idx = 0 ; idx < (int)DIM(wd_array) ; idx++) {
        if ( wd_array[idx].wd == wd ) {
            if ( !eventname && !wd_array[idx].eventpattern )
                return &wd_array[idx];
            if ( eventname &&
                    strstr(eventname, wd_array[idx].eventpattern) )
                return &wd_array[idx];
        }
    }
    return NULL;
}

/**
 * @brief Show the contents of an array of inotify_events
 * Called when a problem occurred during the parsing of
 * the array to ease the debug
 *
 * @param buffer: buffer containing the inotify events
 * @param len: length of the buffer
 */
static void dump_inotify_events(char *buffer, unsigned int len,
    char *lastevent) {

    struct inotify_event *event;
    int i;

    LOGD("%s: Dump the wd_array:\n", __FUNCTION__);
    for (i = 0; i < (int)DIM(wd_array) ; i++) {
        LOGD("%s: wd_array[%d]: filename=%s, wd=%d\n", __FUNCTION__, i, wd_array[i].eventpath, wd_array[i].wd);
    }

    while (1) {
        if (len == 0) {
            /* End of the buffer */
            return;
        }
        event = (struct inotify_event*)buffer;
        if (len < sizeof(struct inotify_event) ||
            len < (sizeof(struct inotify_event) + event->len)) {
            /* Not enought room the last event,
             * get it from the lastevent */
            event = (struct inotify_event*)lastevent;
            if (!event->wd && !event->mask && !event->cookie && !event->len)
                // no last event
                return;
        } else {
            buffer += sizeof(struct inotify_event) + event->len;
            len -= sizeof(struct inotify_event) + event->len;
        }
        LOGD("%s: event received (name=%s, wd=%d, mask=0x%x, len=%d)\n", __FUNCTION__, event->name, event->wd, event->mask, event->len);
    }
}

/**
 * @brief Handle inotify events
 *
 * Calls the callcbacks
 *
 * @param files nb max of logs destination directories (crashlog,
 * aplogs, bz... )
 *
 * @return 0 on success, -1 on error.
 */
int receive_inotify_events(int inotify_fd) {
    int len = 0, orig_len, idx, wd, missing_bytes;
    char orig_buffer[sizeof(struct inotify_event)+PATHMAX], *buffer, lastevent[sizeof(struct inotify_event)+PATHMAX];
    struct inotify_event *event;
    struct watch_entry *entry = NULL;

    len = read(inotify_fd, orig_buffer, sizeof(orig_buffer));
    if (len < 0) {
        LOGE("%s: Cannot read file_monitor_fd, error is %s\n", __FUNCTION__, strerror(errno));
        return -errno;
    }

    buffer = &orig_buffer[0];
    orig_len = len;
    event = (struct inotify_event *)buffer;

    /* Preinitialize lastevent (in case it was not used so it is not dumped) */
    ((struct inotify_event *)lastevent)->wd = 0;
    ((struct inotify_event *)lastevent)->mask = 0;
    ((struct inotify_event *)lastevent)->cookie = 0;
    ((struct inotify_event *)lastevent)->len = 0;

    while (1) {
        if (len == 0) {
            /* End of the events to read */
            return 0;
        }
        if ((unsigned int)len < sizeof(struct inotify_event)) {
            /* Not enough room for an empty event */
            LOGI("%s: incomplete inotify_event received (%d bytes), complete it\n", __FUNCTION__, len);
            /* copy the last bytes received */
            if( (unsigned int)len <= sizeof(lastevent) )
                memcpy(lastevent, buffer, len);
            else {
                LOGE("%s: Cannot copy buffer\n", __FUNCTION__);
                return -1;
            }
            /* read the missing bytes to get the full length */
            missing_bytes = (int)sizeof(struct inotify_event)-len;
            if(((int) len + missing_bytes) < ((int)sizeof(lastevent))) {
                if (read(inotify_fd, &lastevent[len], missing_bytes) != missing_bytes ){
                    LOGE("%s: Cannot complete the last inotify_event received (structure part) - %s\n", __FUNCTION__, strerror(errno));
                    return -1;
                }
            }
            else {
                LOGE("%s: Cannot read missing bytes, not enought space in lastevent\n", __FUNCTION__);
                return -1;
            }
            event = (struct inotify_event*)lastevent;
            /* now, reads the full last event, including its name field */
            if ( read(inotify_fd, &lastevent[sizeof(struct inotify_event)],
                event->len) != (int)event->len) {
                LOGE("%s: Cannot complete the last inotify_event received (name part) - %s\n",
                    __FUNCTION__, strerror(errno));
                return -1;
            }
            len = 0;
            /* now, the last event is complete, we can continue the parsing */
        } else if ( (unsigned int)len < sizeof(struct inotify_event) + event->len ) {
            int res, missing_bytes = (int)sizeof(struct inotify_event) + event->len - len;
            event = (struct inotify_event*)lastevent;
            /* The event was truncated */
            LOGI("%s: truncated inotify_event received (%d bytes missing), complete it\n", __FUNCTION__, missing_bytes);

            /* Robustness : check 'lastevent' array size before reading inotify fd*/
            if( (unsigned int)len > sizeof(lastevent) ) {
                LOGE("%s: not enough space on array lastevent.\n", __FUNCTION__);
                return -1;
            }
            /* copy the last bytes received */
            memcpy(lastevent, buffer, len);
            /* now, reads the full last event, including its name field */
            res = read(inotify_fd, &lastevent[len], missing_bytes);
            if ( res != missing_bytes ) {
                LOGE("%s: Cannot complete the last inotify_event received (name part2); received %d bytes, expected %d bytes - %s\n",
                    __FUNCTION__, res, missing_bytes, strerror(errno));
                return -1;
            }
            len = 0;
            /* now, the last event is complete, we can continue the parsing */
        } else {
            event = (struct inotify_event *)buffer;
            buffer += sizeof(struct inotify_event) + event->len;
            len -= sizeof(struct inotify_event) + event->len;
        }
        /* Handle the event read from the buffer*/
        /* First check the kind of the subject of this event (file or directory?) */
        if (!(event->mask & IN_ISDIR)) {
            /* event concerns a file into a watched directory */
            entry = get_event_entry(event->wd, (event->len ? event->name : NULL));
            if ( !entry ) {
                /* Didn't find any entry for this event, check for
                 * a dropbox final event... */
                if (event->len > 8 && !strncmp(event->name, "dropbox-", 8)) {
                    /* dumpstate is done so remove the watcher */
                    LOGD("%s: Received a dropbox event(%s)...",
                        __FUNCTION__, event->name);
                    inotify_rm_watch(inotify_fd, event->wd);
                    finalize_dropbox_pending_event(event);
                    continue;
                }
                /* Stray event... */
                LOGD("%s: Can't handle the event \"%s\", no valid entry found, drop it...\n",
                    __FUNCTION__, (event->len ? event->name : "empty event"));
                continue;
            }
        }
        /*event concerns a watched directory itself */
        else {
            entry = NULL;
            /* Manage case where a watched directory is deleted*/
            if ( event->mask & (IN_DELETE_SELF | IN_MOVE_SELF) ) {
                /* Recreate the dir and reinstall the watch */
                for (idx = 0 ; idx < (int)DIM(wd_array) ; idx++) {
                    if ( wd_array[idx].wd == event->wd )
                        entry = &wd_array[idx];
                }
                if ( entry && entry->eventpath ) {
                    mkdir(entry->eventpath, 0777); /* TO DO : restoring previous rights/owner/group ?*/
                    inotify_rm_watch(inotify_fd, event->wd);
                    wd = inotify_add_watch(inotify_fd, entry->eventpath, entry->eventmask);
                    if ( wd < 0 ) {
                        LOGE("Can't add watch for %s.\n", entry->eventpath);
                        return -1;
                    }
                    LOGW("%s: watched directory %s : \'%s\' has been created and snooped",__FUNCTION__,
                            (event->mask & (IN_DELETE_SELF) ? "deleted" : "moved"), entry->eventpath);
                    /* if the watch was duplicated, set it for all the entries */
                    for (idx = 0 ; idx < (int)DIM(wd_array) ; idx++) {
                        if (wd_array[idx].wd == event->wd)
                            wd_array[idx].wd = wd;
                    }
                    /* Do nothing more on directory events */
                    continue;
                }
            }
            else {
                for (idx = 0 ; idx < (int)DIM(wd_array) ; idx++) {
                    if ( wd_array[idx].wd != event->wd )
                        continue;
                    entry = &wd_array[idx];
                    /* for modem generic */
                    /* TO IMPROVE : change flag management and put this in main loop */
                    if(strstr(LOGS_MODEM_DIR, entry->eventpath) && (generic_match(event->name, g_first_modem_config))){
                        process_modem_generic(entry, event, inotify_fd);
                        break;
                    }
                }
                pconfig check_config = generic_match_by_wd(event->name, g_first_modem_config, event->wd);
                if(check_config){
                        process_modem_generic( &check_config->wd_config, event, inotify_fd);
                }else{
                    LOGE("%s: Directory not catched %s.\n", __FUNCTION__, event->name);
                }
                /* Do nothing more on directory events */
                continue;
            }
        }
        if ( entry && entry->pcallback && entry->pcallback(entry, event) < 0 ) {
            LOGE("%s: Can't handle the event %s...\n", __FUNCTION__,
                event->name);
            dump_inotify_events(orig_buffer, orig_len, lastevent);
            return -1;
        }
    }

    return 0;
}
