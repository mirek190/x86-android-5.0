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
#ifndef _HSI_DLP_H
#define _HSI_DLP_H
#include <linux/ioctl.h>
struct hsi_dlp_stats {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned long long data_sz;
 unsigned int pdus_cnt;
 unsigned int overflow_cnt;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define HSI_DLP_MAGIC 0x77
#define HSI_DLP_RESET_TX_STATS _IO(HSI_DLP_MAGIC, 32)
#define HSI_DLP_GET_TX_STATS _IOR(HSI_DLP_MAGIC, 32,   struct hsi_dlp_stats)
#define HSI_DLP_RESET_RX_STATS _IO(HSI_DLP_MAGIC, 33)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define HSI_DLP_GET_RX_STATS _IOR(HSI_DLP_MAGIC, 33,   struct hsi_dlp_stats)
#define HSI_DLP_SET_FLASHING_MODE _IOW(HSI_DLP_MAGIC, 42, unsigned int)
#endif

