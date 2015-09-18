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
 * @file config_handler.h
 * @brief File containing functions to handle crashlogd configs.
 *
 * This file contains functions used for operations on crashlogds configs such
 * as loading and storing configs or using configs.
 */

#ifndef CONFIG_HANDLER_H_
#define CONFIG_HANDLER_H_

#include "privconfig.h"
#include "inotify_handler.h"
#include "config.h"

#include <sys/types.h>

typedef struct config * pconfig;

struct config {
    int type;  /* 0 => file 1 => directory */
    int event_class; /* 0 => CRASH 1 => ERROR  2=> INFO */
    char *matching_pattern; /* pattern to check when notified */
    char *eventname; /* event name to generate when pattern found */
    pconfig next;
    char path[PATHMAX];
    char path_linked[PATHMAX];
    struct watch_entry wd_config;
};

pconfig get_generic_config(char* event_name, pconfig config_to_match);
pconfig get_generic_config_by_path(char* path_searched, pconfig config_to_match);
int generic_match(char* event_name, pconfig config_to_match);
void generic_add_watch(pconfig config_to_watch, int fd);
pconfig generic_match_by_wd(char* event_name, pconfig config_to_match, int wd);
void free_config(pconfig first);
void store_config(char *section, struct config_handle a_conf_handle);
void load_config_by_pattern(char *section_pattern, char *key_pattern, struct config_handle a_conf_handle);
void load_config();

int cfg_check_modem_version();

#endif /* CONFIG_HANDLER_H_ */
