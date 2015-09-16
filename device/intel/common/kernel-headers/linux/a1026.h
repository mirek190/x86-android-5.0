/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef __LINUX_A1026_H
#define __LINUX_A1026_H
#include <linux/ioctl.h>
#include <linux/i2c.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A1026_MAX_FW_SIZE (256*1024)
struct a1026img {
 unsigned char *buf;
 unsigned img_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define A1026_IOCTL_MAGIC 'u'
#define A1026_BOOTUP_INIT _IO(A1026_IOCTL_MAGIC, 0x01)
#define A1026_SUSPEND _IO(A1026_IOCTL_MAGIC, 0x02)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A1026_ENABLE_CLOCK _IO(A1026_IOCTL_MAGIC, 0x03)
#endif

