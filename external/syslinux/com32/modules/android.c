/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2011 Intel Corporation; author: Andrew Boie
 *   Significant protions Copyright 2007-2008 H. Peter Anvin
 *   Significant portions copyright (C) 2010 Shao Miller
 *                                        [partition iteration, GPT, "fs"]
 *   Significant portions Copyright 2007, The Android Open Source Project
 *                  [struct boot_img_hdr layout]
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 53 Temple Place Ste 330,
 *   Boston MA 02111-1307, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------- */

/*
 * android.c
 *
 * Add support for loading Android boot partitions.
 *
 * Much code for this derived from chain.c32 and linux.c32
 */

#include <com32.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dprintf.h>
#include <console.h>
#include <syslinux/linux.h>
#include <syslinux/config.h>
#include <syslinux/rawio.h>

#define PAGE_SIZE 2048

typedef struct boot_img_hdr boot_img_hdr;

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512

struct boot_img_hdr
{
    char magic[BOOT_MAGIC_SIZE];

    unsigned kernel_size;  /* size in bytes */
    unsigned kernel_addr;  /* physical load addr */

    unsigned ramdisk_size; /* size in bytes */
    unsigned ramdisk_addr; /* physical load addr */

    unsigned second_size;  /* size in bytes */
    unsigned second_addr;  /* physical load addr */

    unsigned tags_addr;    /* physical addr for kernel tags */
    unsigned page_size;    /* flash page size we assume */
    unsigned unused[2];    /* future expansion: should be 0 */

    char name[BOOT_NAME_SIZE]; /* asciiz product name */

    char cmdline[BOOT_ARGS_SIZE];

    unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};

/*
** +-----------------+
** | boot header     | 1 page
** +-----------------+
** | kernel          | n page
** +-----------------+
** | ramdisk         | m pages
** +-----------------+
** | second stage    | o pages
** +-----------------+
**
** n = (kernel_size + page_size - 1) / page_size
** m = (ramdisk_size + page_size - 1) / page_size
** o = (second_size + page_size - 1) / page_size
**
** 0. all entities are page_size aligned in flash
** 1. kernel and ramdisk are required (size != 0)
** 2. second is optional (second_size == 0 -> no second)
** 3. load each element (kernel, ramdisk, second) at
**    the specified physical address (kernel_addr, etc)
** 4. prepare tags at tag_addr.  kernel_args[] is
**    appended to the kernel commandline in the tags.
** 5. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
** 6. if second_size != 0: jump to second_addr
**    else: jump to kernel_addr
*/


#define error(...) fprintf(stderr, __VA_ARGS__)
#define SECTORS_PER_PAGE (PAGE_SIZE / SECTOR)

/* Given that disk_info is initialized and we know the starting LBA
 * for the partition containing the kernel we want, parse
 * the boot image header, get the kernel, ramdisk, and cmdline data,
 * and boot it */
static void do_boot(uint64_t lba, char *cmdline)
{
    struct boot_img_hdr *boot_img_hdr = NULL;
    struct initramfs *initramfs = NULL;
    void *kernel_data = NULL;
    size_t kernel_len;
    void *initramfs_data = NULL;
    size_t initramfs_len;

    dprintf("Boot image from LBA %llu\n", lba);

    boot_img_hdr = rawio_read_sectors(lba, 1 * SECTORS_PER_PAGE);
    if (!boot_img_hdr) {
        error("reading boot image header\n");
        goto error_out;
    }
    if (strncmp(boot_img_hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
        error("boot magic mismatch\n");
        goto error_out;
    }

    dprintf("Loading Android image (k=%u r=%u) '%s'\n", boot_img_hdr->kernel_size,
            boot_img_hdr->ramdisk_size, boot_img_hdr->name);

    kernel_len = boot_img_hdr->kernel_size;
    lba += 1 * SECTORS_PER_PAGE;
    dprintf("kernel at LBA %llu\n", lba);
    kernel_data = rawio_read_sectors(lba, (kernel_len / SECTOR) + 1);
    if (!kernel_data) {
        error("unable to read kernel data\n");
        goto error_out;
    }

    initramfs_len = boot_img_hdr->ramdisk_size;
    lba += ((kernel_len + PAGE_SIZE - 1) / PAGE_SIZE) * SECTORS_PER_PAGE;
    dprintf("ramdisk at LBA %llu\n", lba);
    initramfs_data = rawio_read_sectors(lba, (initramfs_len / SECTOR) + 1);
    if (!initramfs_data) {
        error("unable to read initramfs data");
        goto error_out;
    }

    initramfs = initramfs_init();
    if (!initramfs) {
        error("initramfs_init failed\n");
        goto error_out;
    }

    if (initramfs_add_data(initramfs, initramfs_data, initramfs_len,
                initramfs_len, 4)) {
        error("initramfs_add_data failed\n");
        goto error_out;
    }

    if (cmdline) {
        /* append user provided cmdline to the existing one */
        int boot_cmdline_len = strlen(boot_img_hdr->cmdline);
        int cmdline_len = strlen(cmdline);
        if (cmdline_len < (BOOT_ARGS_SIZE - boot_cmdline_len - 1)) {
            /* length already checked for */
            strcat(boot_img_hdr->cmdline, " ");
            strcat(boot_img_hdr->cmdline, cmdline);
        } else
            error("kernel cmdline too big, sticking to original cmdline\n");
    }

    dprintf("cmdline: '%s'\n", boot_img_hdr->cmdline);
    syslinux_boot_linux(kernel_data, kernel_len, initramfs,
            boot_img_hdr->cmdline);
error_out:
    if (boot_img_hdr)
        free(boot_img_hdr);
    if (kernel_data)
        free(kernel_data);
    if (initramfs_data)
        free(initramfs_data);
    if (initramfs)
        free(initramfs);
}



/* Stitch together the command line from a set of argv's */
static char *make_cmdline(char **argv)
{
    char **arg;
    size_t bytes;
    char *cmdline, *p;

    bytes = 1;                  /* Just in case we have a zero-entry cmdline */
    for (arg = argv; *arg; arg++) {
        bytes += strlen(*arg) + 1;
    }

    p = cmdline = malloc(bytes);
    if (!cmdline)
        return NULL;

    for (arg = argv; *arg; arg++) {
        int len = strlen(*arg);
        memcpy(p, *arg, len);
        p[len] = ' ';
        p += len + 1;
    }

    if (p > cmdline)
        p--;                    /* Remove the last space */
    *p = '\0';

    return cmdline;
}

static void usage(void)
{
    static const char usage[] = "\
Usage:  android.c32 <disk#> <partition> [kernel cmdline]\n\
use 'current' for disk# to use the device syslinux started from\n\
kernel cmdline will be appended to existing one\n";
    printf(usage);
}

int main(int argc, char *argv[])
{
    int drive = 0;
    int partnum = 1;
    struct disk_part_iter *cur_part = NULL;

    openconsole(&dev_null_r, &dev_stdcon_w);

    if (argc < 3) {
        error("expecting at least 2 parameters\n");
        usage();
        goto bail;
    }

    dprintf("params ok\n");

    if (!strcmp(argv[1], "current")) {
        const union syslinux_derivative_info *sdi;
        sdi = syslinux_derivative_info();
        drive = sdi->disk.drive_number;
    } else {
        drive = 0x80 | strtoul(argv[1], NULL, 0);
    }

    dprintf("drive is %02x\n", drive);

    /* Get the disk geometry and disk access setup */
    if (rawio_get_disk_params(drive)) {
        error("Cannot  get disk parameters\n");
        goto bail;
    }
    dprintf("disk params loaded\n");

    /* Find the specified partiton */
    partnum = strtoul(argv[2], NULL, 0);
    cur_part = rawio_get_first_partition(NULL);
    dprintf("searching for partition %d\n", partnum);
    while (cur_part) {
        if (cur_part->index == partnum)
            break;
        cur_part = cur_part->next(cur_part);
    }
    if (!cur_part) {
        error("requested partition not found!\n");
        goto bail;
    }

    do_boot(cur_part->lba_data, make_cmdline(&argv[3]));

bail:
    error("Unable to boot Android image...\n");
    if (cur_part) {
        free(cur_part->block);
        free(cur_part);
    }
    return 255;
}
