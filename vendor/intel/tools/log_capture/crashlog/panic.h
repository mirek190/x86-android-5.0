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
 * @file panic.h
 * @brief File containing functions to detect and process ipanic events.
 */

#ifndef __PANIC_H__
#define __PANIC_H__

int crashlog_check_panic(char *reason, int test);
int crashlog_check_ram_panic(char *reason);
int crashlog_check_panic_header(char *reason);
int crashlog_check_kdump(char *reason, int test);
void crashlog_check_panic_events(char *reason, char *watchdog, int test);

#endif /* __PANIC_H__ */
