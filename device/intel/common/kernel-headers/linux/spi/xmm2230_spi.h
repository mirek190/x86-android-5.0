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
#ifndef LINUX_XMM2230_SPI_H
#define LINUX_XMM2230_SPI_H
#define SPI_IOCTRL_MAGIC 0x78
#define SPI_IOC_FRAME_LEN_DOWNLOAD _IO(SPI_IOCTRL_MAGIC, 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SPI_IOC_FRAME_LEN_NORMAL _IO(SPI_IOCTRL_MAGIC, 2)
#define SPI_IOC_READ_FRAME_SIZE _IO(SPI_IOCTRL_MAGIC, 3)
#endif

