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
 * @file ramdump.h
 * @brief File containing functions for ram dump mode processing.
 */

#ifndef RAMDUMP_H_
#define RAMDUMP_H_

#ifdef CRASHLOGD_MODULE_RAMDUMP
int do_ramdump_checks(int test);
#else
static inline int do_ramdump_checks(int test __unused) { return 0; }
#endif

#endif /* RAMDUMP_H_ */
