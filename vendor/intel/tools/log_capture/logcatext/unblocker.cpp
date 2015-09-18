/* * Copyright (C) Intel 2014
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

#include <pthread.h>
#include <log/log.h>
#include <log/logger.h>
#include "klogger.h"
#include <stdio.h>

static pthread_mutex_t mtx_msg_proc = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_kmsg_available = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_unblock_run = PTHREAD_MUTEX_INITIALIZER;
static int log_id = 1;
static int log_prio = 1;

#define UBL_LOG_TAG "lcx_sync"

#define UBL_LOG_MSG "Log Buffer Sync."

static void *unblock_thread_proc(void *ptr) {
    (void *)ptr;
    int ret, mtx_status;
    while (1) {
        pthread_mutex_lock(&mtx_unblock_run);
        /*make shure we have something from kernel */
        ret = klogger_wait();
        mtx_status = pthread_mutex_trylock(&mtx_msg_proc);
        if (ret > 0 && mtx_status == 0) {
            if (pthread_mutex_trylock(&mtx_kmsg_available) == 0) {
                __android_log_buf_write(log_id, log_prio, UBL_LOG_TAG,
                                        UBL_LOG_MSG);
            }
        } else
            pthread_mutex_unlock(&mtx_unblock_run);
        pthread_mutex_unlock(&mtx_msg_proc);
    }
    return NULL;
}

void unblock_start(int logid, int logprio) {
    pthread_t th;
    log_id = logid;
    log_prio = logprio;
    pthread_mutex_trylock(&mtx_unblock_run);
    pthread_create(&th, NULL, unblock_thread_proc, NULL);
}

void unblock_resume() {
    pthread_mutex_unlock(&mtx_unblock_run);
    pthread_mutex_trylock(&mtx_msg_proc);
    pthread_mutex_unlock(&mtx_msg_proc);
}

void unblock_ack() {
    pthread_mutex_trylock(&mtx_msg_proc);
}

int unblock_is_sync(struct log_msg *msg) {
    if (msg->id() == log_id && msg->msg()[0] == log_prio) {
        if (!strncmp(&msg->msg()[1], UBL_LOG_TAG, strlen(UBL_LOG_TAG)))
            return 1;
    }
    return 0;
}

void unblock_kmsg_consumed() {
    pthread_mutex_trylock(&mtx_kmsg_available);
    pthread_mutex_unlock(&mtx_kmsg_available);
}
