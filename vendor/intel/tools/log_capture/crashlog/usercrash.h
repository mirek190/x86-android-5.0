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
 * @file usercrash.h
 * @brief File containing functions to process 'generic' events (javacrash,
 * tombstone, hprof and apcore events).
 */

#ifndef __USERCRASH__H__
#define __USERCRASH__H__

#include "inotify_handler.h"

int process_usercrash_event(struct watch_entry *entry, struct inotify_event *event);
int process_hprof_event(struct watch_entry *entry, struct inotify_event *event);
int process_apcore_event(struct watch_entry *entry, struct inotify_event *event);

#endif
