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
 * @file startupreason.h
 * @brief File containing functions to read startup reason and to detect and
 * process wdt event.
 */

#ifndef __STARTUP_REASON_H__
#define __STARTUP_REASON_H__

#ifdef CONFIG_EFILINUX
int read_resettype(char *resettype);
int read_resetsource(char *resetsource);
int read_wakesource(char *wakesource);
int read_shutdownsource(char *shutdownsource);
#else
static inline int read_resettype(char *resettype __unused) {return -1;}
static inline int read_resetsource(char *resetsource __unused) {return -1;}
static inline int read_wakesource(char *wakesource __unused) {return -1;}
static inline int read_shutdownsource(char *shutdownsource __unused) {return -1;}
#endif

void read_startupreason(char *startupreason);
int crashlog_check_startupreason(char *reason, char *watchdog);

#endif /* __STARTUP_REASON_H__ */
