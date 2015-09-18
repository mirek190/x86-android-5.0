/* Copyright (C) Intel 2014
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
 * @file checksum.h
 * @brief File containing basic functions to calculate a checksum on a
 * buffer or a file.
 */

#include "fsutils.h"
#include "checksum.h"

#include <ftw.h>
#include <fcntl.h>

/* used by calculate_checksum_directory! */
static SHA_CTX g_sha1_context_dir_sum;
static calculate_checksum_callback callback_dir = NULL;

int calculate_checksum_buffer(const char *buffer, ssize_t buffer_size, unsigned char *result) {
    ssize_t nread;
    unsigned char *l_buffer = (unsigned char *)buffer;
    SHA_CTX sha1_context;

    if (!buffer || !result || buffer_size <= 0)
        return -EINVAL;

    SHA1_Init(&sha1_context);

    while (buffer_size) {

        nread = (buffer_size > CRASHLOG_CHECKSUM_SIZE) ?
                 CRASHLOG_CHECKSUM_SIZE : buffer_size;
        buffer_size -= nread;

        SHA1_Update(&sha1_context, l_buffer, nread);

        l_buffer += nread;
    }

    SHA1_Final(result, &sha1_context);

    return 0;
}

int calculate_checksum_file(const char *filename, unsigned char *result) {
    ssize_t nread;
    unsigned char buffer[CRASHLOG_CHECKSUM_SIZE];
    SHA_CTX sha1_context;
    int fd = -1;

    if (!filename || !result)
        return -EINVAL;

    /* Open file for reading */
    fd = open(filename, O_RDONLY);

    if (fd < 0)
        return -errno;

    SHA1_Init(&sha1_context);

    while ((nread = do_read(fd, buffer, CRASHLOG_CHECKSUM_SIZE)) != 0) {

        if (nread < 0)
            return -errno;

        SHA1_Update(&sha1_context, buffer, nread);

        if (nread < CRASHLOG_CHECKSUM_SIZE)
            break;
    }

    SHA1_Final(result, &sha1_context);

    return 0;
}

static int calculate_checksum_file_feed(const char *filename) {
    ssize_t nread;
    unsigned char buffer[CRASHLOG_CHECKSUM_SIZE];
    int fd = -1;

    if (!filename)
        return -EINVAL;

    /* Open file for reading */
    fd = open(filename, O_RDONLY);

    if (fd < 0)
        return -errno;

    while ((nread = do_read(fd, buffer, CRASHLOG_CHECKSUM_SIZE)) != 0) {

        if (nread < 0)
            return -errno;

        SHA1_Update(&g_sha1_context_dir_sum, buffer, nread);

        if (nread < CRASHLOG_CHECKSUM_SIZE)
            break;
    }

    return 0;
}

static int calculate_checksum_buffer_feed(const char *buffer, int buffer_size) {
    ssize_t nread;
    unsigned char *l_buffer = (unsigned char *)buffer;

    if (!buffer || buffer_size <= 0)
        return -EINVAL;

    while (buffer_size) {

        nread = (buffer_size > CRASHLOG_CHECKSUM_SIZE) ?
                 CRASHLOG_CHECKSUM_SIZE : buffer_size;
        buffer_size -= nread;

        SHA1_Update(&g_sha1_context_dir_sum, l_buffer, nread);

        l_buffer += nread;
    }

    return 0;
}

static int calculate_checksum_path(const char *fpath, const struct stat *sb, int tflag, struct FTW __unused *ftwbuf) {

    /* if file, calculate the checksum */
    if (tflag == FTW_F) {
        if ((sb->st_mode & S_IFMT) == S_IFREG) {
            calculate_checksum_file_feed(fpath);
        } else if (callback_dir != NULL) {
            /* return the reason why a path tagged as a file was skipped */
            callback_dir(fpath, sb->st_mode & S_IFMT);
        }
    }

    /* calculate also the checksum on the file/folder path */
    /* for the case with no access or entry is a directory */
    calculate_checksum_buffer_feed(fpath, strlen(fpath));

    return 0;
}

int calculate_checksum_directory(const char *path, unsigned char *result, calculate_checksum_callback callback) {
    /* mutex used to protect g_sha1_context_dir_sum */
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    callback_dir = callback;

    pthread_mutex_lock(&lock);
    SHA1_Init(&g_sha1_context_dir_sum);

    if (nftw(path, calculate_checksum_path, CRASHLOG_CHECKSUM_SIZE, 0) == -1) {
        pthread_mutex_unlock(&lock);
        return -1;
    }

    SHA1_Final(result, &g_sha1_context_dir_sum);
    pthread_mutex_unlock(&lock);

    return 0;
}
