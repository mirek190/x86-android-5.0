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
 * @file dropbox.h
 * @brief File containing functions used to handle dropbox events.
 *
 * This file contains the functions used to handle various operations linked to
 * dropbox events such as 'lost' event processing, dropbox duplicate events processing,
 * dumpstate server management...
 */
#ifndef __DROPBOX_H__
#define __DROPBOX_H__

#include <inotify_handler.h>

void dropbox_set_file_monitor_fd(int file_monitor_fd);
int start_dumpstate_srv(char* crash_dir, int crashidx, char *key);
long extract_dropbox_timestamp(char* filename);
int manage_duplicate_dropbox_events(struct inotify_event *event);
int process_lost_event(struct watch_entry *entry, struct inotify_event *event);
int finalize_dropbox_pending_event(const struct inotify_event *event);

#endif /* __DROPBOX_H__ */
