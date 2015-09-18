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
 * @file kct_netlink.h
 * @brief File containing functions for getting Kernel events.
 */

#ifndef __KCT_NETLINK_H__
#define __KCT_NETLINK_H__

#ifdef CRASHLOGD_MODULE_KCT
void kct_netlink_init_comm(void);
int kct_netlink_get_fd(void);
void kct_netlink_handle_msg(void);
#else
static inline void kct_netlink_init_comm(void) {}
static inline int kct_netlink_get_fd(void) { return 0; }
static inline void kct_netlink_handle_msg(void) {}
#endif

#endif /* __KCT_NETLINK_H__ */
