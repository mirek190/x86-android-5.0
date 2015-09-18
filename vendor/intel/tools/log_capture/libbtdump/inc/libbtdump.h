/* Copyright (C) Intel 2014
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

#ifndef LIBBTDUMP_H
#define LIBBTDUMP_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Dump Back-traces for all the threads
 * @param output FILE *
 * @return 0 on success
 */
int bt_all(FILE *output);

/**
 * Dump Back-traces for all the threads associated with the
 * provided PID
 * @param pid
 * @param output
 * @return 0 on success
 */
int bt_process(int pid, FILE *output);

#ifdef __cplusplus
}
#endif

#endif /*LIBBTDUMP_H*/
