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
 * @file iptrak.h
 * @brief File containing functions used to handle various operations on
 * IPTRAK file.
 *
 */

#ifndef __IPTRAK_H__
#define __IPTRAK_H__

/*
 * The enumerated type to use in order to control
 * iptrak file generation.
 */
typedef enum e_iptrak_gen_policy {
    /* Do not force generation.                        */
    /* File will be generated only if an error         */
    /* has occurred previously and a retry has been    */
    /* requested.                                      */
    ONLY_IF_REQUIRED,
    /* Force generation.                               */
    FORCE_GENERATION,
    /* File generation will be attempted once more on  */
    /* next call if an error occurred.                 */
    RETRY_ONCE
} e_iptrak_gen_policy_t;

#ifdef CRASHLOGD_MODULE_IPTRAK
void check_iptrak_file(e_iptrak_gen_policy_t update);
#else
static inline void check_iptrak_file(e_iptrak_gen_policy_t update __unused) {}
#endif

#endif /* __IPTRAK_H__ */
