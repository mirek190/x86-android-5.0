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

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <utils/Log.h>
#include <cutils/sockets.h>

#include "apb.h"

enum {
    READ = 0,
    WRITE,
};

enum {
    FD_MAIN = 0,
    FD_CLIENT,
    FD_NUM,
};

typedef struct apb_ctx {
    pthread_t id;
    pthread_mutex_t mtx;
    size_t nb_at;
    char **ats;
    /* main socket and client socket */
    int fd_sock[FD_NUM];
    /* self-pipe used to notify internal (APB) events */
    int fd_in_pipe[2];
    /* self-pipe used to notify external events (to ap-proxy) */
    int fd_out_pipe[2];
    char *msg;
} apb_ctx_t;

#undef LOG_TAG
#define LOG_TAG "proxy_apb"

#define LOG_ERROR(format, args ...) \
    do { ALOGE("%s - " format, __FUNCTION__, ## args); } while (0)
#define LOG_DEBUG(format, args ...) \
    do { ALOGD("%s - " format, __FUNCTION__, ## args); } while (0)

#define ARRAY_SIZE(e) (sizeof(e) / sizeof(*e))

#define xstr(s) str(s)
#define str(s) #s

#define ASSERT(exp) do { \
    if (!(exp)) { \
        LOG_ERROR("%s:%d Assertion '" xstr(exp) "'", __FILE__, __LINE__); \
        abort(); \
    } \
} while (0)

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define INFINITE_TIMEOUT -1

#define APB_CONFIG_FILE "/local_cfg/telephony_config/apb_config.txt"

static void at_get_list(apb_ctx_t *ctx)
{
    FILE *fd = NULL;
    size_t size = 0;
    char line[1024];

    ASSERT(ctx != NULL);

    ctx->nb_at = 0;

    if ((fd = fopen(APB_CONFIG_FILE, "r"))) {
        while (fgets(line, sizeof(line), fd)) {
            if (ctx->nb_at >= size) {
                /* To reduce memory fragmentation, let's allocate by chunks
                 * of (char* x 64) bytes */
                size += 64;
                ctx->ats = realloc(ctx->ats, sizeof(char*) * size);
                ASSERT(ctx->ats != NULL);
            }
            /* \n removal */
            line[strlen(line) - 1] = '\0';
            ctx->ats[ctx->nb_at++] = strdup(line);
        }
        fclose(fd);
        LOG_DEBUG("%zu AT commands are handled by APB", ctx->nb_at);
    } else
        LOG_ERROR("No configuration file found - AT command list is empty");
}

static bool is_at_supported(apb_ctx_t *ctx, const char *at)
{
    bool ret = false;

    ASSERT(ctx != NULL);
    ASSERT(at != NULL);

    for (size_t i = 0; i < ctx->nb_at; i++) {
        if (strcasestr(at, ctx->ats[i])) {
            ret = true;
            break;
        }
    }

    char* at_upper = strdup(at);
    if (at_upper != NULL) {
        for (size_t i = 0; i < strlen(at_upper); i++) {
           at_upper[i] = toupper(at_upper[i]);
        }

        if (strcasestr(at_upper, "AT+CGDCONT=")) {
            int cid;
            sscanf(at_upper, "AT+CGDCONT=%d", &cid);
            if (cid == 1) {
                ret = true;
            }
        } else if (strcasestr(at_upper, "AT+CGACT=")) {
            int cid, state;
            sscanf(at_upper, "AT+CGACT=%d,%d", &state, &cid);
            if (cid == 1) {
                ret = true;
            }
        } else if (strcasestr(at_upper, "AT+CFUN=")) {
            int mode;
            sscanf(at_upper, "AT+CFUN=%d", &mode);
            if ((strlen(at_upper) == 11) && (mode == 1 || mode == 4)) {
                ret = true;
            }
        }
        free(at_upper);
    }
    return ret;
}

static int socket_open(apb_ctx_t *ctx)
{
    int ret = 0;

    ASSERT(ctx != NULL);

    if (ctx->fd_sock[FD_MAIN] != -1) {
        LOG_ERROR("socket already opened");
        ret = -1;
        goto out;
    }

    LOG_DEBUG("configure socket: %s", APB_SOCKET_NAME);
    ctx->fd_sock[FD_MAIN] = android_get_control_socket(APB_SOCKET_NAME);

    if (listen(ctx->fd_sock[FD_MAIN], 1) < 0) {
        LOG_ERROR("listen failed (%s)", strerror(errno));
        ret = -1;
    }

out:
    return ret;
}

static int socket_close(int fd)
{
    int ret = 0;

    if (fd > 0) {
        shutdown(fd, SHUT_RDWR);
        if (close(fd) < 0) {
            LOG_ERROR("(fd=%d) reason: (%s)", fd, strerror(errno));
            ret = -1;
        }
    }

    return ret;
}

static inline void notify(int fd)
{
    char c = '0';
    ASSERT(write(fd, &c, sizeof(c)) == sizeof(c));
}

static inline int client_get_fd(apb_ctx_t *ctx)
{
    int fd;
    pthread_mutex_lock(&ctx->mtx);
    fd = ctx->fd_sock[FD_CLIENT];
    pthread_mutex_unlock(&ctx->mtx);

    return fd;
}

static inline void client_set_fd(apb_ctx_t *ctx, int fd)
{
    pthread_mutex_lock(&ctx->mtx);
    ctx->fd_sock[FD_CLIENT] = fd;
    pthread_mutex_unlock(&ctx->mtx);
}

static inline void poll_add_fd(struct pollfd *pollfd, int fd)
{
    pollfd->fd = fd;
    pollfd->events = POLLIN;
    pollfd->revents = 0;
}

static void client_communication(apb_ctx_t *ctx)
{
    struct pollfd fds[3];

    ASSERT(ctx != NULL);

    poll_add_fd(&fds[0], ctx->fd_sock[FD_MAIN]);
    poll_add_fd(&fds[1], ctx->fd_in_pipe[READ]);

    for (;;) {
        /* update of fd client */
        poll_add_fd(&fds[2], client_get_fd(ctx));

        int err = poll(fds, ARRAY_SIZE(fds), INFINITE_TIMEOUT);
        if (err < 0) {
            LOG_ERROR("An error happened: %s", strerror(errno));
            continue;
        }

        if (fds[0].revents & POLLIN) {
            /* Important note: APB only supports one client */
            int fd = accept(ctx->fd_sock[FD_MAIN], NULL, NULL);
            if (client_get_fd(ctx) != -1) {
                LOG_ERROR("A client is already connected. Connection rejected");
                socket_close(fd);
            } else {
                client_set_fd(ctx, fd);
                LOG_DEBUG("client connected");
            }
        }

        if (fds[1].revents & POLLIN) {
            LOG_DEBUG("APB shutdown on-going");
            break;
        }

        if (fds[2].revents & POLLIN) {
            char buf[1024];
            ssize_t len = read(fds[2].fd, buf, sizeof(buf));

            if (len <= 0) {
                socket_close(ctx->fd_sock[FD_CLIENT]);
                ctx->fd_sock[FD_CLIENT] = -1;
                LOG_DEBUG("client disconnected");
            } else {
                LOG_DEBUG("message received from client");
                /* @TODO: a FIFO might be implemented here to handle
                 * multiple answers */
                if (ctx->msg) {
                    LOG_ERROR("queue not empty. Message trashed");
                } else {
                    ssize_t max = MIN(len, (ssize_t) sizeof(buf) - 1);
                    buf[max] = '\0';
                    ctx->msg = strdup(buf);
                    /* Notify at proxy */
                    notify(ctx->fd_out_pipe[WRITE]);
                }
            }
        }
    }
}

/**
 * @see apb.h
 */
apb_handle_t *apb_init(void)
{
    apb_ctx_t *ctx = calloc(1, sizeof(apb_ctx_t));
    if (!ctx) {
        LOG_ERROR("memory allocation failure");
        goto err;
    }

    for (int i = 0; i < FD_NUM; i++)
        ctx->fd_sock[i] = -1;

    ASSERT(pipe(ctx->fd_in_pipe) == 0);
    ASSERT(pipe(ctx->fd_out_pipe) == 0);

    at_get_list(ctx);

    if (socket_open(ctx)) {
        LOG_ERROR("failed to open the socket");
        goto err;
    }

    pthread_mutex_init(&ctx->mtx, NULL);

    /* Client communication is handled in a separate thread. This could be done
     * in the main at-proxy thread. But, regarding the current code, no needs
     * to add more complexity in an already spaghetti code... */
    if(pthread_create(&ctx->id, NULL, (void *)client_communication, ctx)) {
        LOG_ERROR("failed to create thread");
        goto err;
    }

    LOG_DEBUG("APB initialized successfully");
    return (apb_handle_t*) ctx;

err:
    apb_dispose((apb_handle_t*) ctx);
    return NULL;
}

/**
 * @see apb.h
 */
void apb_dispose(apb_handle_t *hdle)
{
    apb_ctx_t *ctx = (apb_ctx_t*) hdle;

    if (ctx) {
        /* Stop client thread */
        if (ctx->fd_in_pipe[WRITE] > 0)
            notify(ctx->fd_in_pipe[WRITE]);
        if (ctx->id > 0)
            pthread_join(ctx->id, NULL);

        for (size_t i = 0; i < ctx->nb_at; i++)
            free(ctx->ats[i]);
        free(ctx->ats);

        for (int i = 0; i < FD_NUM; i++) {
            if (ctx->fd_sock[i] > 0)
                socket_close(ctx->fd_sock[i]);
        }

        for (int i = 0; i < 2; i++) {
            if (ctx->fd_in_pipe[i] > 0)
                close(ctx->fd_in_pipe[i]);
            if (ctx->fd_out_pipe[i] > 0)
                close(ctx->fd_out_pipe[i]);
        }

        free(ctx);
        LOG_DEBUG("APB disposed");
    }
}

/**
 * @see apb.h
 */
int apb_get_fd(apb_handle_t *hdle)
{
    int fd = -1;
    apb_ctx_t *ctx = (apb_ctx_t*) hdle;

    if (ctx)
        fd = ctx->fd_out_pipe[READ];

    return fd;
}

/**
 * @see apb.h
 */
int apb_fwd_msg(apb_handle_t *hdle, const char *msg, ssize_t size)
{
    int ret = -1;
    apb_ctx_t *ctx = (apb_ctx_t*) hdle;

    if (ctx == NULL)
        goto out;

    int fd = client_get_fd(ctx);
    if (fd < 0) {
        LOG_DEBUG("No client connected");
        goto out;
    }

    if (is_at_supported(ctx, msg)) {
        LOG_DEBUG("AT command forwarded: %s", msg);
        ASSERT(write(fd, msg, size) == size);
        ret = 0;
    }

out:
    return ret;
}

/**
 * @see apb.h
 */
char* apb_get_answer(apb_handle_t *hdle)
{
    char *msg = NULL;
    apb_ctx_t *ctx = (apb_ctx_t*) hdle;

    if (ctx && ctx->msg) {
        msg = ctx->msg;
        /* It's client responsibility to free the buffer.
         * So we can dereference this memory area */
        ctx->msg = NULL;
    }

    return msg;
}
