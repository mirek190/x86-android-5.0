/*
 **
 ** Copyright 2012 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **      http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */
#ifndef __IGTI_PROXY__

#define __IGTI_PROXY__

#if defined(__cplusplus)
extern "C" {
#endif
#include <stdbool.h>

/**
 * Handle a AT command in gti
 *
 * This function takes a AT command from an input string and returns an answer
 * in the response string. In case of failure, the function return false.
 *
 * @param[in] at_cmd        AT command to parse
 * @param[out] rsp_buf      buffer where to store the at answer
 * @param[in] rsp_buf_size  size of the buffer given for output.
 *
 * @return true in case of success false otherwise. The function will return
 *         false is one of input buffer is NULL.
 */
bool gti_handle_at_command(const char *at_cmd, char *rsp_buf, size_t rsp_buf_size);

#if defined(__cplusplus)
}
#endif

#endif /* __IGTI_PROXY__ */
