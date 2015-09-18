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
 * @file ct_utils.h
 * @brief File containing basic operations for crashlog to kernel and
 * crashlog to user space communication.
 */

#ifndef __CT_UTILS_H__
#define __CT_UTILS_H__

#include <linux/kct.h>

int event_pass_filter(struct ct_event *ev);
void process_msg(struct ct_event *ev);
int dump_binary_attchmts_in_file(struct ct_event* ev, char* file_path);
int dump_data_in_file(struct ct_event* ev, char* file_path);
void convert_name_to_upper_case(char * name);

#endif /* __CT_UTILS_H__ */
