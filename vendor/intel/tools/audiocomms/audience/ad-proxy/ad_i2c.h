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

#ifndef _AD_I2C_H_
#define _AD_I2C_H_

#define AD_DEV_NODE  "/dev/audience_es305"

#define AD_IOCTL_MAGIC 'u'
#define AD_ENABLE_CLOCK   _IO(AD_IOCTL_MAGIC, 0x03)

/**
 * Initialize i2c module sending SYNC command to the audience chip.
 *
 * @return int value of the file descriptor
 */
int ad_i2c_init(void);
int ad_i2c_read(unsigned char *buf, int len);
int ad_i2c_write(unsigned char *buf, int len);

#endif // _AD_I2C_H_
