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

#ifndef __INOTIFY_HANDLER_H__
#define __INOTIFY_HANDLER_H__

#include <sys/inotify.h>
#include "fsutils.h"

/* Define inotify mask values for watched directories */
#define BASE_DIR_MASK       (IN_CLOSE_WRITE|IN_DELETE_SELF|IN_MOVE_SELF)
#define DROPBOX_DIR_MASK    (BASE_DIR_MASK|IN_MOVED_FROM|IN_MOVED_TO)
#define TOMBSTONE_DIR_MASK  (BASE_DIR_MASK)
#define CORE_DIR_MASK       (BASE_DIR_MASK)
#define STAT_DIR_MASK       (BASE_DIR_MASK)
#define APLOG_DIR_MASK      (BASE_DIR_MASK)
#define UPTIME_MASK         IN_CLOSE_WRITE
#define MDMCRASH_DIR_MASK   (BASE_DIR_MASK)
#define VBCRASH_DIR_MASK    (BASE_DIR_MASK|IN_CREATE)

struct watch_entry;

/* The callback API is:
 * returns a negative value if any error occurred
 * returns 0 if the event was not handled
 * returns a positive value if the event was handled properly
 */
typedef int (*inotify_callback) (struct watch_entry *,
    struct inotify_event *);

struct watch_entry {
    int wd;
    int eventmask;
    int eventtype;
    int inotify_error;
    char *eventname;
    char *eventpath;
    char *eventpattern;
    inotify_callback pcallback;
};

int init_inotify_handler();
void handle_missing_watched_dir();
int get_missing_watched_dir_nb();
void build_crashenv_dir_list_option( char crashenv_param[PATHMAX] );
int set_watch_entry_callback(unsigned int watch_type, inotify_callback pcallback);
int receive_inotify_events(int inotify_fd);

#endif /* __INOTIFY_HANDLER_H__ */
