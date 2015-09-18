/*
 * Copyright (C) Intel 2014
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __AP_PROXY_BRIDGE_HEADER__
#define __AP_PROXY_BRIDGE_HEADER__

#include <stdbool.h>
#define APB_SOCKET_NAME "atproxy-apb"

typedef void *apb_handle_t;

/**
 * Initializes the APB module
 *
 * @return a valid apb_handle_t pointer if successful
 * @return NULL otherwise
 */
apb_handle_t *apb_init(void);

/**
 * Disposes the APB module
 *
 * @param [in] hdle APB handler
 *
 */
void apb_dispose(apb_handle_t *hdle);

/**
 * Returns a file descriptor. This fd needs to be polled
 * by at-proxy to receive events from APB. An event is raised by APB
 * when a message is received
 *
 * @param [in] hdle APB handler
 *
 * @return -1 if any client is registered or if the handler is incorrect
 * @return a valid fd otherwise
 */
int apb_get_fd(apb_handle_t *hdle);

/**
 * Forward the AT command to AP
 *
 * @param [in] hdle APB handler
 * @param [in] msg AT command to forward
 * @param [in] size size of AT command
 *
 * @return 0 if the message has been forwarded
 * @return -1 otherwise
 */
int apb_fwd_msg(apb_handle_t *hdle, const char *msg, ssize_t size);

/**
 * Returns the client answer. This function must be called when an event has
 * been raised by APB
 *
 * @param [in] hdle APB handler
 *
 * @return char* containing the answer. This buffer needs to be freed by the
 *         client
 * @return NULL otherwise
 */
char *apb_get_answer(apb_handle_t *hdle);

#endif /* __AP_PROXY_BRIDGE_HEADER__ */
