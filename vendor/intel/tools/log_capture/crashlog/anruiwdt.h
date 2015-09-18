/* Copyright (C) Intel 2010
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
 * @file anruiwdt.h
 * @brief File containing definition of functions for anr and uiwdt events processing.
 *
 * This file contains functions used to process ANR and UIWDT events.
 */

#ifndef __ANRUIWDT_H__
#define __ANRUIWDT_H__

#include "inotify_handler.h"

int process_anruiwdt_event(struct watch_entry *, struct inotify_event *);

#endif /* __ANRUIWDT_H__ */
