/*
 **
 ** Copyright 2013 Intel Corporation
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
 **
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start the Audience Proxy. The proxy state machine works forever in an inifinite loop
 * to execute commands from AuViD. On error, the proxy stops itself and the function returns a
 * negative value.
 *
 * @return -1 which means the proxy stops on error
 *            and errno set to indicate the error
 */
int start_proxy(void);

#ifdef __cplusplus
}
#endif
