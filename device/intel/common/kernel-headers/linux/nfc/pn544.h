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
#define PN544_MAGIC 0xE9
#define PN544_SET_PWR _IOW(PN544_MAGIC, 0x01, unsigned int)
#define NFC_HOST_INT_GPIO "NFC-intr"
#define NFC_ENABLE_GPIO "NFC-enable"
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define NFC_FW_RESET_GPIO "NFC-reset"
struct pn544_i2c_platform_data {
 unsigned int irq_gpio;
 unsigned int ven_gpio;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int firm_gpio;
 unsigned int max_i2c_xfer_size;
};

