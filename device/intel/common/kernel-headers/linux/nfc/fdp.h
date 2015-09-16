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
#include <linux/ioctl.h>
#define FIELDSPEAK_MAJOR 0
#define FIELDSPEAK_IOC_MAGIC 'o'
#define FIELDSPEAK_IOC_RESET _IO(FIELDSPEAK_IOC_MAGIC, 0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FIELDSPEAK_MAX_IOCTL_VALUE 0
#define NFC_HOST_INT_GPIO "NFC-intr"
#define NFC_RESET_GPIO "NFC-enable"
struct fdp_i2c_platform_data {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int irq_gpio;
 unsigned int rst_gpio;
 unsigned int max_i2c_xfer_size;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */

