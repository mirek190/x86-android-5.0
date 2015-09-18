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
 * @file fw_update.h
 * @brief File containing the function to process the firmware update status.
 */

#ifndef __FW_UPDATE_H__
#define __FW_UPDATE_H__

#ifdef CRASHLOGD_MODULE_FW_UPDATE
int crashlog_check_fw_update_status(void);
#else
static inline int crashlog_check_fw_update_status(void) { return 0; }
#endif

#endif /* __FW_UPDATE_H__ */
