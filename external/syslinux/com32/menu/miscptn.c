
#include <com32.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslinux/rawio.h>

#include "miscptn.h"

#define error(...) fprintf(stderr, __VA_ARGS__)
#define BCB_MAGIC	0xFEEDFACE

/* Persistent area written by Android recovery console and Linux bcb driver
 * reboot hook for communication with the bootloader */
struct bootloader_message {
    /* Directive to the bootloader on what it needs to do next.
     * Possible values:
     *   boot-NNN - Automatically boot into label NNN
     *   bootonce-NNN - Automatically boot into label NNN, clearing this
     *     field afterwards
     *   anything else / garbage - Boot default label */
    char command[32];

    /* Storage area for error codes when using the BCB to boot into special
     * boot targets, e.g. for baseband update. */
    char status[32];

    /* Area for recovery console to stash its command line arguments
     * in case it is reset and the cache command file is erased.
     * Not used here. */
    char recovery[1024];

    /* Magic sentinel value written by the bootloader; kernel/userspace won't
     * update this if not equalto to BCB_MAGIC */
    uint32_t magic;
};


static struct bootloader_message *get_bootloader_message(
	    struct disk_part_iter *cur_part)
{
    struct bootloader_message *msg;

    msg = rawio_read_sectors(cur_part->lba_data, size_to_sectors(sizeof(*msg)));
    if (!msg)
	return NULL;

    /* Null terminate everything, could be garbage */
    msg->command[sizeof(msg->command) - 1] = '\0';
    msg->status[sizeof(msg->status) - 1] = '\0';
    msg->recovery[sizeof(msg->recovery) - 1] = '\0';

    return msg;
}

static int put_bootloader_message(struct disk_part_iter *cur_part,
	    struct bootloader_message *bcb)
{
    unsigned char *out_sect;
    int ret = 0;
    unsigned int i;

    out_sect = malloc(size_to_sectors(sizeof(*bcb)) * SECTOR);
    memcpy(out_sect, bcb, sizeof(*bcb));

    for(i = 0; i < size_to_sectors(sizeof(*bcb)); i++) {
	ret |= rawio_write_sector(cur_part->lba_data + i, out_sect);
	out_sect += SECTOR;
    }
    return ret;
}

char *read_misc_partition(int disk, int partnum)
{
    struct disk_part_iter *cur_part;
    struct bootloader_message *bcb = NULL;
    char *boot_target = NULL;
    int dirty = 0;

    cur_part = rawio_get_partition(disk, partnum);
    if (!cur_part) {
	error("rawio_get_partition(%d,%d) failed\n", disk, partnum);
	return NULL;
    }

    bcb = get_bootloader_message(cur_part);
    if (!bcb) {
	error("failed to get bootloader_message");
	return NULL;
    }

    if (!strncmp(bcb->command, "boot-", 5)) {
	boot_target = strdup(bcb->command + 5);
    } else if (bcb && !strncmp(bcb->command, "bootonce-", 9)) {
	boot_target = strdup(bcb->command + 9);
	bcb->command[0] = '\0';
	dirty = 1;
    }
    if (bcb->magic != BCB_MAGIC) {
	bcb->magic = BCB_MAGIC;
	dirty = 1;
    }

    /* Historical: 'reboot bootloader' means enter Fastboot mode
     * (which is not implemented in the bootloader but another boot
     * image) */
    if (!strcmp(boot_target, "bootloader"))
	strcpy(boot_target, "fastboot");

    if (dirty && put_bootloader_message(cur_part, bcb))
	error("failed to update bootloader message");

    if (bcb)
	free(bcb);
    if (cur_part) {
	free(cur_part->block);
	free(cur_part);
    }
    return boot_target;
}
