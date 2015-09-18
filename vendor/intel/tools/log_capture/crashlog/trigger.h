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
 * @file trigger.h
 * @brief File containing functions to process aplog events, bz events and stats
 * events.
 */

#ifndef __TRIGGER_H__
#define __TRIGGER_H__

#include <sys/inotify.h>
#include "inotify_handler.h"

int process_stat_event(struct watch_entry *entry, struct inotify_event *event);
int process_aplog_event(struct watch_entry *entry, struct inotify_event *event);
int process_log_event(char *rootdir, char *triggername, int mode);

#endif /* __TRIGGER_H__ */
