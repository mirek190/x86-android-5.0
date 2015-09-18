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
 * @file history.h
 * @brief File containing every functions for operations on the history file and
 * for uptime management.
 *
 * This file contains the functions to handle the history file and the uptime event.
 * A circular buffer locally defined has its content synchronized on the history file
 * content.
 */

#ifndef __HISTORY__H__
#define __HISTORY__H__

#include "inotify_handler.h"

#include <sys/inotify.h>

struct history_entry {
    char *event;
    char *type;
    char *log;
    const char* lastuptime;
    char* key;
    char* eventtime;
};

int get_lastboot_uptime(char lastbootuptime[24]);
int get_uptime_string(char newuptime[24], int *hours);
int update_history_file(struct history_entry *entry);
int reset_uptime_history();
int uptime_history();
int history_has_event(char *eventdir);
int reset_history_cache();
int add_uptime_event();
int update_history_on_cmd_delete(char *events);
int process_uptime_event(struct watch_entry *entry, struct inotify_event *event);

#endif /* __HISTORY__H__ */
