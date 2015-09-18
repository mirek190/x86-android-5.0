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
#ifndef _ASM_X86_PAGE_DEFS_H
#define _ASM_X86_PAGE_DEFS_H
#include <linux/const.h>
#include <linux/types.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define PAGE_SHIFT 12
#define PAGE_SIZE (_AC(1,UL) << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE-1))
#define __PHYSICAL_MASK ((phys_addr_t)((1ULL << __PHYSICAL_MASK_SHIFT) - 1))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define __VIRTUAL_MASK ((1UL << __VIRTUAL_MASK_SHIFT) - 1)
#define PHYSICAL_PAGE_MASK (((signed long)PAGE_MASK) & __PHYSICAL_MASK)
#define PMD_PAGE_SIZE (_AC(1, UL) << PMD_SHIFT)
#define PMD_PAGE_MASK (~(PMD_PAGE_SIZE-1))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define HPAGE_SHIFT PMD_SHIFT
#define HPAGE_SIZE (_AC(1,UL) << HPAGE_SHIFT)
#define HPAGE_MASK (~(HPAGE_SIZE - 1))
#define HUGETLB_PAGE_ORDER (HPAGE_SHIFT - PAGE_SHIFT)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define HUGE_MAX_HSTATE 2
#define PAGE_OFFSET ((unsigned long)__PAGE_OFFSET)
#define VM_DATA_DEFAULT_FLAGS   (((current->personality & READ_IMPLIES_EXEC) ? VM_EXEC : 0 ) |   VM_READ | VM_WRITE | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC)
#include <asm/page_32_types.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#ifndef __ASSEMBLY__
#endif
#endif

