/* Telephony Config Selector (TCS) - external include file
**
** Copyright (C) Intel 2013
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
*/

#ifndef __TCS_EXTERNAL_HEADER_FILE__
#define __TCS_EXTERNAL_HEADER_FILE__

#ifdef __cplusplus
extern "C" {
#endif

#include "tcs_config.h"
#include "tcs_mmgr.h"

typedef void *tcs_handle_t;

typedef struct config {
    size_t nb;
    mdm_info_t *mdm;
} tcs_cfg_t;

/**
 * Create Telephony Configuration Selector library handle.
 *
 * It detects the running configuration (via the sysfs) parses
 * all XML files and extracts the parameters related to the config.
 *
 * If the function failed, TCS is broken. There is no recovery.
 *
 * @return NULL if operation failed
 */
tcs_handle_t *tcs_init(void);

/**
 * This function returns a tcs_cfg_t pointer containing all the parameters
 * of the running configuration.
 *
 * @param [in] handle
 *
 * @return NULL if the operation failed
 */
tcs_cfg_t *tcs_get_config(tcs_handle_t *handle);

/**
 * This function returns a mmgr_info_t pointer containing all MMGR parameters
 * of the running configuration.
 *
 * @param [in] handle
 * @param [in] mdm_info Pointer to the information structure of the modem
 * for which MMGR parameters are requested
 *
 * @return NULL if the operation failed
 */
mmgr_info_t *tcs_get_mmgr_config(tcs_handle_t *handle, mdm_info_t *mdm_info);

/**
 * This function prints the current platform configuration.
 * If tcs_get_mmgr_config has not been called before, only the content of
 * tcs_cfg_t is printed.
 * If tcs_get_mmgr_config bas been called before, the content of
 * tcs_cfg_t and mmgr_info_t is printed.
 *
 * @param [in] handle
 *
 * @return 0 if successful
 * @return -1 if the operation failed
 */
int tcs_print(tcs_handle_t *handle);

/**
 * Delete library handle.
 * The pointer provided by tcs_get_config is freed.
 *
 * @param [out] handle
 *
 * @return 0 if successful
 * @return -1 if the operation failed
 */
int tcs_dispose(tcs_handle_t *handle);

#ifdef __cplusplus
}
#endif
#endif                          /* __TCS_EXTERNAL_HEADER_FILE__ */
