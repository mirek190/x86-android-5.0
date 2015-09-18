/*
 **
 ** Copyright 2011 Intel Corporation
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
 ** Author:
 ** Zhang, Dongsheng <dongsheng.zhang@intel.com>
 ** Li, Peizhao <peizhao.li@intel.com>
 **
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <full_rw.h>

#define LOG_TAG "ad_proxy:i2c"
#include "cutils/log.h"

#include "ad_i2c.h"

int ad_i2c_fd = -1;
char ad_path[] = AD_DEV_NODE;

/**
 * Maximum number of SYNC reply read retry.
 */
#define AD_I2C_SYNC_RETRIES 20

/**
 * Delay to wait before to retry to read the reply of a SYNC command.
 */
#define AD_I2C_SYNC_RETRIES_DELAY 20000

int ad_i2c_sync(void)
{
    int i;
    int ret = -1;
    unsigned char sync[4] = {0x80, 0x00, 0x00, 0x00};
    unsigned char res[4] = {0x00, 0x00, 0x00, 0x00};

    if (ad_i2c_write(sync, 4) == 4) {
        for (i=0; i < AD_I2C_SYNC_RETRIES; i++) {
            if (ad_i2c_read(res, 4) == 4) {
                ALOGD("SYNC RES: 0x%02x%02x%02x%02x", res[0], res[1], res[2], res[3]);
                if ((res[0] == sync[0]) && (res[1] == sync[1]) &&
                    (res[2] == sync[2]) && (res[3] == sync[3])) {
                    ret = 0;
                    break;
                }
            }
            /* Chip has not yet a SYNC reply ready, wait before to re-read */
            usleep(AD_I2C_SYNC_RETRIES_DELAY);
        }
    } else {
        ALOGE("%s write sync error", __func__);
    }

    ALOGD("%s [%d]", __func__, ret);
    return ret;
}

int ad_i2c_init(void)
{
    int ret;

    ad_i2c_fd = open(ad_path, O_RDWR);
    if (ad_i2c_fd < 0) {
        ALOGE("%s open error (%s)", __func__, strerror(errno));
        return -1;
    }

    ret = ioctl(ad_i2c_fd, AD_ENABLE_CLOCK);
    if(ret) {
        ALOGE("%s ioctl  ENABLE_CLOCK error (%s)", __func__, strerror(errno));
        return -1;
    }
    close(ad_i2c_fd);

    return ad_i2c_sync();
}

int ad_i2c_read(unsigned char *buf, int len)
{
    int readSize;

    ad_i2c_fd = open(ad_path, O_RDWR);
    if (ad_i2c_fd < 0) {
        ALOGE("%s open error (%s)", __func__, strerror(errno));
        return -1;
    }

    readSize = full_read(ad_i2c_fd, buf, len);
    if (readSize != len) {
        ALOGE("%s: read %d len %d error (%s)", __func__, readSize, len, strerror(errno));
    }

    close(ad_i2c_fd);
    return readSize;
}

int ad_i2c_write(unsigned char *buf, int len)
{
    int ret;
    int writeSize;

    ad_i2c_fd = open(ad_path, O_RDWR);
    if (ad_i2c_fd < 0) {
        ALOGE("%s open error (%s)", __func__, strerror(errno));
        return -1;
    }
    ret = ioctl(ad_i2c_fd, AD_ENABLE_CLOCK);
    if(ret) {
        ALOGE("%s: ioctl  ENABLE_CLOCK error (%s)", __func__, strerror(errno));
        return ret;
    }

    writeSize = full_write(ad_i2c_fd, buf, len);
    if (writeSize != len) {
        ALOGE("%s: write %d len %d error (%s)", __func__, writeSize, len, strerror(errno));
    }


    close(ad_i2c_fd);
    return writeSize;
}
