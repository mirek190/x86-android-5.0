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

#define error(...) fprintf(stderr, __VA_ARGS__)

/*
 * Call int 13h, but with retry on failure.  Especially floppies need this.
 */
static int int13_retry(const com32sys_t * inreg, com32sys_t * outreg)
{
    int retry = 6;		/* Number of retries */
    com32sys_t tmpregs;

    if (!outreg)
	outreg = &tmpregs;

    while (retry--) {
	__intcall(0x13, inreg, outreg);
	if (!(outreg->eflags.l & EFLAGS_CF))
	    return 0;		/* CF=0, OK */
    }

    return -1;			/* Error */
}

/*
 * Query disk parameters and EBIOS availability for a particular disk.
 */
struct diskinfo {
    int disk;
    int ebios;			/* EBIOS supported on this disk */
    int cbios;			/* CHS geometry is valid */
    int head;
    int sect;
} disk_info;

int rawio_get_disk_params(int disk)
{
    static com32sys_t getparm, parm, getebios, ebios;

    disk_info.disk = disk;
    disk_info.ebios = disk_info.cbios = 0;

    /* Get EBIOS support */
    getebios.eax.w[0] = 0x4100;
    getebios.ebx.w[0] = 0x55aa;
    getebios.edx.b[0] = disk;
    getebios.eflags.b[0] = 0x3;	/* CF set */

    __intcall(0x13, &getebios, &ebios);

    if (!(ebios.eflags.l & EFLAGS_CF) &&
	ebios.ebx.w[0] == 0xaa55 && (ebios.ecx.b[0] & 1)) {
	disk_info.ebios = 1;
    }

    /* Get disk parameters -- really only useful for
       hard disks, but if we have a partitioned floppy
       it's actually our best chance... */
    getparm.eax.b[1] = 0x08;
    getparm.edx.b[0] = disk;

    __intcall(0x13, &getparm, &parm);

    if (parm.eflags.l & EFLAGS_CF)
	return disk_info.ebios ? 0 : -1;

    disk_info.head = parm.edx.b[1] + 1;
    disk_info.sect = parm.ecx.b[0] & 0x3f;
    if (disk_info.sect == 0) {
	disk_info.sect = 1;
    } else {
	disk_info.cbios = 1;	/* Valid geometry */
    }

    return 0;
}

/*
 * Get a disk block and return a malloc'd buffer.
 * Uses the disk number and information from disk_info.
 */
struct ebios_dapa {
    uint16_t len;
    uint16_t count;
    uint16_t off;
    uint16_t seg;
    uint64_t lba;
};


/* Read count sectors from drive, starting at lba.  Return a new buffer */
static int read_sectors_raw(uint64_t lba, uint8_t count, void *data)
{
    com32sys_t inreg;
    struct ebios_dapa *dapa = __com32.cs_bounce;
    void *buf = (char *)__com32.cs_bounce + SECTOR;

    if (!count)
	/* Silly */
	return 1;
    dprintf("read_sectors_raw lba=%llu count=%hhu\n", lba, count);

    memset(&inreg, 0, sizeof inreg);

    if (disk_info.ebios) {
	dapa->len = sizeof(*dapa);
	dapa->count = count;
	dapa->off = OFFS(buf);
	dapa->seg = SEG(buf);
	dapa->lba = lba;

	inreg.esi.w[0] = OFFS(dapa);
	inreg.ds = SEG(dapa);
	inreg.edx.b[0] = disk_info.disk;
	inreg.eax.b[1] = 0x42;	/* Extended read */
    } else {
	unsigned int c, h, s, t;

	if (!disk_info.cbios) {
	    /* We failed to get the geometry */

	    if (lba)
		return 1;	/* Can only read MBR */

	    s = h = c = 0;
	} else {
	    s = lba % disk_info.sect;
	    t = lba / disk_info.sect;	/* Track = head*cyl */
	    h = t % disk_info.head;
	    c = t / disk_info.head;
	}

	if (s >= 63 || h >= 256 || c >= 1024)
	    return 1;

	inreg.eax.b[0] = count;
	inreg.eax.b[1] = 0x02;	/* Read */
	inreg.ecx.b[1] = c;
	inreg.ecx.b[0] = ((c & 0x300) >> 2) | (s + 1);
	inreg.edx.b[1] = h;
	inreg.edx.b[0] = disk_info.disk;
	inreg.ebx.w[0] = OFFS(buf);
	inreg.es = SEG(buf);
    }

    if (int13_retry(&inreg, NULL)) {
	error("int13 call failed\n");
	return 1;
    }

    memcpy(data, buf, count * SECTOR);
    return 0;
}

/* Some BIOSes freak out if you try to do an extended read of more than
 * 127 sectors; play it safe */
#define MAX_SECTORS 127

/* Read count sectors from drive, starting at lba.  Return a new buffer */
void *rawio_read_sectors(uint64_t lba, unsigned int count)
{
    unsigned char *data;
    off_t pos = 0;

    data = malloc(count * SECTOR);
    if (!data) {
	return NULL;
    }
    /* keep reading until all the data is in */
    while (count) {
	uint8_t sectors_to_read = ((count > MAX_SECTORS) ? MAX_SECTORS : count);
	if (read_sectors_raw(lba, sectors_to_read, &data[pos])) {
	    free(data);
	    dprintf("read_sectors_raw failure\n");
	    return NULL;
	}
	pos += sectors_to_read * SECTOR;
	count -= sectors_to_read;
	lba += sectors_to_read;
    }
    return data;
}


int rawio_write_sector(unsigned int lba, const void *data)
{
    com32sys_t inreg;
    struct ebios_dapa *dapa = __com32.cs_bounce;
    void *buf = (char *)__com32.cs_bounce + SECTOR;

    memcpy(buf, data, SECTOR);
    memset(&inreg, 0, sizeof inreg);

    if (disk_info.ebios) {
	dapa->len = sizeof(*dapa);
	dapa->count = 1;	/* 1 sector */
	dapa->off = OFFS(buf);
	dapa->seg = SEG(buf);
	dapa->lba = lba;

	inreg.esi.w[0] = OFFS(dapa);
	inreg.ds = SEG(dapa);
	inreg.edx.b[0] = disk_info.disk;
	inreg.eax.w[0] = 0x4300;	/* Extended write */
    } else {
	unsigned int c, h, s, t;

	if (!disk_info.cbios) {
	    /* We failed to get the geometry */

	    if (lba)
		return -1;	/* Can only write MBR */

	    s = h = c = 0;
	} else {
	    s = lba % disk_info.sect;
	    t = lba / disk_info.sect;	/* Track = head*cyl */
	    h = t % disk_info.head;
	    c = t / disk_info.head;
	}

	if (s >= 63 || h >= 256 || c >= 1024)
	    return -1;

	inreg.eax.w[0] = 0x0301;	/* Write one sector */
	inreg.ecx.b[1] = c;
	inreg.ecx.b[0] = ((c & 0x300) >> 2) | (s+1);
	inreg.edx.b[1] = h;
	inreg.edx.b[0] = disk_info.disk;
	inreg.ebx.w[0] = OFFS(buf);
	inreg.es = SEG(buf);
    }

    if (int13_retry(&inreg, NULL))
	return -1;

    return 0;			/* ok */
}

int rawio_write_verify_sector(unsigned int lba, const void *buf)
{
    char *rb;
    int rv;

    rv = rawio_write_sector(lba, buf);
    if (rv)
	return rv;		/* Write failure */
    rb = rawio_read_sectors(lba, 1);
    if (!rb)
	return -1;		/* Readback failure */
    rv = memcmp(buf, rb, SECTOR);
    free(rb);
    return rv ? -1 : 0;
}



static void mbr_part_dump(const struct part_entry *part)
{
    (void)part;
    dprintf("Partition status _____ : 0x%.2x\n"
	    "Partition CHS start\n"
	    "  Cylinder ___________ : 0x%.4x (%u)\n"
	    "  Head _______________ : 0x%.2x (%u)\n"
	    "  Sector _____________ : 0x%.2x (%u)\n"
	    "Partition type _______ : 0x%.2x\n"
	    "Partition CHS end\n"
	    "  Cylinder ___________ : 0x%.4x (%u)\n"
	    "  Head _______________ : 0x%.2x (%u)\n"
	    "  Sector _____________ : 0x%.2x (%u)\n"
	    "Partition LBA start __ : 0x%.8x (%u)\n"
	    "Partition LBA count __ : 0x%.8x (%u)\n"
	    "-------------------------------\n",
	    part->active_flag,
	    chs_cylinder(part->start),
	    chs_cylinder(part->start),
	    chs_head(part->start),
	    chs_head(part->start),
	    chs_sector(part->start),
	    chs_sector(part->start),
	    part->ostype,
	    chs_cylinder(part->end),
	    chs_cylinder(part->end),
	    chs_head(part->end),
	    chs_head(part->end),
	    chs_sector(part->end),
	    chs_sector(part->end),
	    part->start_lba, part->start_lba, part->length, part->length);
}

/* A DOS MBR */
struct mbr {
    char code[440];
    uint32_t disk_sig;
    char pad[2];
    struct part_entry table[4];
    uint16_t sig;
} __attribute__ ((packed));
static const uint16_t mbr_sig_magic = 0xAA55;




static struct disk_part_iter *next_ebr_part(struct disk_part_iter *part)
{
    const struct part_entry *ebr_table;
    const struct part_entry *parent_table =
	((const struct mbr *)part->private.ebr.parent->block)->table;
    static const struct part_entry phony = {.start_lba = 0 };
    uint64_t ebr_lba;

    /* Don't look for a "next EBR" the first time around */
    if (part->private.ebr.parent_index >= 0)
	/* Look at the linked list */
	ebr_table = ((const struct mbr *)part->block)->table + 1;
    /* Do we need to look for an extended partition? */
    if (part->private.ebr.parent_index < 0 || !ebr_table->start_lba) {
	/* Start looking for an extended partition in the MBR */
	while (++part->private.ebr.parent_index < 4) {
	    uint8_t type = parent_table[part->private.ebr.parent_index].ostype;

	    if ((type == 0x05) || (type == 0x0F) || (type == 0x85))
		break;
	}
	if (part->private.ebr.parent_index == 4)
	    /* No extended partitions found */
	    goto out_finished;
	part->private.ebr.lba_extended =
	    parent_table[part->private.ebr.parent_index].start_lba;
	ebr_table = &phony;
    }
    /* Load next EBR */
    ebr_lba = ebr_table->start_lba + part->private.ebr.lba_extended;
    free(part->block);
    part->block = rawio_read_sectors(ebr_lba, 1);
    if (!part->block) {
	error("Could not load EBR!\n");
	goto err_ebr;
    }
    ebr_table = ((const struct mbr *)part->block)->table;
    dprintf("next_ebr_part:\n");
    mbr_part_dump(ebr_table);

    /*
     * Sanity check entry: must not extend outside the
     * extended partition.  This is necessary since some OSes
     * put crap in some entries.
     */
    {
	const struct mbr *mbr =
	    (const struct mbr *)part->private.ebr.parent->block;
	const struct part_entry *extended =
	    mbr->table + part->private.ebr.parent_index;

	if (ebr_table[0].start_lba >= extended->start_lba + extended->length) {
	    dprintf("Insane logical partition!\n");
	    goto err_insane;
	}
    }
    /* Success */
    part->lba_data = ebr_table[0].start_lba + ebr_lba;
    dprintf("Partition %d logical lba %llu\n", part->index, part->lba_data);
    part->index++;
    part->record = ebr_table;
    return part;

err_insane:

    free(part->block);
    part->block = NULL;
err_ebr:

out_finished:
    free(part->private.ebr.parent->block);
    free(part->private.ebr.parent);
    free(part->block);
    free(part);
    return NULL;
}

static struct disk_part_iter *next_mbr_part(struct disk_part_iter *part)
{
    struct disk_part_iter *ebr_part;
    /* Look at the partition table */
    struct part_entry *table = ((struct mbr *)part->block)->table;

    /* Look for data partitions */
    while (++part->private.mbr_index < 4) {
	uint8_t type = table[part->private.mbr_index].ostype;

	if (type == 0x00 || type == 0x05 || type == 0x0F || type == 0x85)
	    /* Skip empty or extended partitions */
	    continue;
	if (!table[part->private.mbr_index].length)
	    /* Empty */
	    continue;
	break;
    }
    /* If we're currently the last partition, it's time for EBR processing */
    if (part->private.mbr_index == 4) {
	/* Allocate another iterator for extended partitions */
	ebr_part = malloc(sizeof(*ebr_part));
	if (!ebr_part) {
	    error("Could not allocate extended partition iterator!\n");
	    goto err_alloc;
	}
	/* Setup EBR iterator parameters */
	ebr_part->block = NULL;
	ebr_part->index = 4;
	ebr_part->record = NULL;
	ebr_part->next = next_ebr_part;
	ebr_part->private.ebr.parent = part;
	/* Trigger an initial EBR load */
	ebr_part->private.ebr.parent_index = -1;
	/* The EBR iterator is responsible for freeing us */
	return next_ebr_part(ebr_part);
    }
    dprintf("next_mbr_part:\n");
    mbr_part_dump(table + part->private.mbr_index);

    /* Update parameters to reflect this new partition.  Re-use iterator */
    part->lba_data = table[part->private.mbr_index].start_lba;
    dprintf("Partition %d primary lba %llu\n", part->private.mbr_index,
	    part->lba_data);
    part->index = part->private.mbr_index + 1;
    part->record = table + part->private.mbr_index;
    return part;

    free(ebr_part);
err_alloc:

    free(part->block);
    free(part);
    return NULL;
}

/*
 * GUID
 * Be careful with endianness, you must adjust it yourself
 * iff you are directly using the fourth data chunk
 */
struct guid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint64_t data4;
} __attribute__ ((packed));

    /*
     * This walk-map effectively reverses the little-endian
     * portions of the GUID in the output text
     */
static const char guid_le_walk_map[] = {
    3, -1, -1, -1, 0,
    5, -1, 0,
    3, -1, 0,
    2, 1, 0,
    1, 1, 1, 1, 1, 1
};

#if DEBUG
/*
 * Fill a buffer with a textual GUID representation.
 * The buffer must be >= char[37] and will be populated
 * with an ASCII NUL C string terminator.
 * Example: 11111111-2222-3333-4444-444444444444
 * Endian:  LLLLLLLL-LLLL-LLLL-BBBB-BBBBBBBBBBBB
 */
static void guid_to_str(char *buf, const struct guid *id)
{
    unsigned int i = 0;
    const char *walker = (const char *)id;

    while (i < sizeof(guid_le_walk_map)) {
	walker += guid_le_walk_map[i];
	if (!guid_le_walk_map[i])
	    *buf = '-';
	else {
	    *buf = ((*walker & 0xF0) >> 4) + '0';
	    if (*buf > '9')
		*buf += 'A' - '9' - 1;
	    buf++;
	    *buf = (*walker & 0x0F) + '0';
	    if (*buf > '9')
		*buf += 'A' - '9' - 1;
	}
	buf++;
	i++;
    }
    *buf = 0;
}
#endif



/* A GPT partition */
struct gpt_part {
    struct guid type;
    struct guid uid;
    uint64_t lba_first;
    uint64_t lba_last;
    uint64_t attribs;
    char name[72];
} __attribute__ ((packed));

static void gpt_part_dump(const struct gpt_part *gpt_part)
{
#ifdef DEBUG
    unsigned int i;
    char guid_text[37];

    dprintf("----------------------------------\n"
	    "GPT part. LBA first __ : 0x%.16llx\n"
	    "GPT part. LBA last ___ : 0x%.16llx\n"
	    "GPT part. attribs ____ : 0x%.16llx\n"
	    "GPT part. name _______ : '",
	    gpt_part->lba_first, gpt_part->lba_last, gpt_part->attribs);
    for (i = 0; i < sizeof(gpt_part->name); i++) {
	if (gpt_part->name[i])
	    dprintf("%c", gpt_part->name[i]);
    }
    dprintf("'");
    guid_to_str(guid_text, &gpt_part->type);
    dprintf("GPT part. type GUID __ : {%s}\n", guid_text);
    guid_to_str(guid_text, &gpt_part->uid);
    dprintf("GPT part. unique ID __ : {%s}\n", guid_text);
#endif
    (void)gpt_part;
}

/* A GPT header */
struct gpt {
    char sig[8];
    union {
	struct {
	    uint16_t minor;
	    uint16_t major;
	} fields __attribute__ ((packed));
	uint32_t uint32;
	char raw[4];
    } rev __attribute__ ((packed));
    uint32_t hdr_size;
    uint32_t chksum;
    char reserved1[4];
    uint64_t lba_cur;
    uint64_t lba_alt;
    uint64_t lba_first_usable;
    uint64_t lba_last_usable;
    struct guid disk_guid;
    uint64_t lba_table;
    uint32_t part_count;
    uint32_t part_size;
    uint32_t table_chksum;
    char reserved2[1];
} __attribute__ ((packed));
static const char gpt_sig_magic[] = "EFI PART";

#if DEBUG
static void gpt_dump(const struct gpt *gpt)
{
    char guid_text[37];

    dprintf("GPT sig ______________ : '%8.8s'\n"
	   "GPT major revision ___ : 0x%.4x\n"
	   "GPT minor revision ___ : 0x%.4x\n"
	   "GPT header size ______ : 0x%.8x\n"
	   "GPT header checksum __ : 0x%.8x\n"
	   "GPT reserved _________ : '%4.4s'\n"
	   "GPT LBA current ______ : 0x%.16llx\n"
	   "GPT LBA alternative __ : 0x%.16llx\n"
	   "GPT LBA first usable _ : 0x%.16llx\n"
	   "GPT LBA last usable __ : 0x%.16llx\n"
	   "GPT LBA part. table __ : 0x%.16llx\n"
	   "GPT partition count __ : 0x%.8x\n"
	   "GPT partition size ___ : 0x%.8x\n"
	   "GPT part. table chksum : 0x%.8x\n",
	   gpt->sig,
	   gpt->rev.fields.major,
	   gpt->rev.fields.minor,
	   gpt->hdr_size,
	   gpt->chksum,
	   gpt->reserved1,
	   gpt->lba_cur,
	   gpt->lba_alt,
	   gpt->lba_first_usable,
	   gpt->lba_last_usable,
	   gpt->lba_table, gpt->part_count, gpt->part_size, gpt->table_chksum);
    guid_to_str(guid_text, &gpt->disk_guid);
    dprintf("GPT disk GUID ________ : {%s}\n", guid_text);
}
#endif

static struct disk_part_iter *next_gpt_part(struct disk_part_iter *part)
{
    const struct gpt_part *gpt_part = NULL;

    while (++part->private.gpt.index < part->private.gpt.parts) {
	gpt_part =
	    (const struct gpt_part *)(part->block +
				      (part->private.gpt.index *
				       part->private.gpt.size));
	if (!gpt_part->lba_first)
	    continue;
	break;
    }
    /* Were we the last partition? */
    if (part->private.gpt.index == part->private.gpt.parts) {
	goto err_last;
    }
    part->lba_data = gpt_part->lba_first;
    part->private.gpt.part_guid = &gpt_part->uid;
    part->private.gpt.part_label = gpt_part->name;
    /* Update our index */
    part->index = part->private.gpt.index + 1;
    gpt_part_dump(gpt_part);

    /* In a GPT scheme, we re-use the iterator */
    return part;

err_last:
    free(part->block);
    free(part);

    return NULL;
}

struct disk_part_iter *rawio_get_first_partition(struct disk_part_iter *part)
{
    const struct gpt *gpt_candidate;

    /*
     * Ignore any passed partition iterator.  The caller should
     * have passed NULL.  Allocate a new partition iterator
     */
    part = malloc(sizeof(*part));
    if (!part) {
	error("Count not allocate partition iterator!\n");
	goto err_alloc_iter;
    }
    /* Read MBR */
    part->block = rawio_read_sectors(0, 2);
    if (!part->block) {
	error("Could not read two sectors!\n");
	goto err_read_mbr;
    }
    /* Check for an MBR */
    if (((struct mbr *)part->block)->sig != mbr_sig_magic) {
	error("No MBR magic!\n");
	goto err_mbr;
    }
    /* Establish a pseudo-partition for the MBR (index 0) */
    part->index = 0;
    part->record = NULL;
    part->private.mbr_index = -1;
    part->next = next_mbr_part;
    /* Check for a GPT disk */
    gpt_candidate = (const struct gpt *)(part->block + SECTOR);
    if (((struct mbr *)part->block)->table[0].ostype == 0xEE &&
            !memcmp(gpt_candidate->sig, gpt_sig_magic, sizeof(gpt_sig_magic))) {
	/* LBA for partition table */
	uint64_t lba_table;

	/* It looks like one */
	/* TODO: Check checksum.  Possibly try alternative GPT */
#if DEBUG
	puts("Looks like a GPT disk.");
	gpt_dump(gpt_candidate);
#endif
	/* TODO: Check table checksum (maybe) */
	/* Note relevant GPT details */
	part->next = next_gpt_part;
	part->private.gpt.index = -1;
	part->private.gpt.parts = gpt_candidate->part_count;
	part->private.gpt.size = gpt_candidate->part_size;
	lba_table = gpt_candidate->lba_table;
	gpt_candidate = NULL;
	/* Load the partition table */
	free(part->block);
	part->block =
	    rawio_read_sectors(lba_table,
			 ((part->private.gpt.size * part->private.gpt.parts) +
			  SECTOR - 1) / SECTOR);
	if (!part->block) {
	    error("Could not read GPT partition list!\n");
	    goto err_gpt_table;
	}
    }
    /* Return the pseudo-partition's next partition, which is real */
    return part->next(part);

err_gpt_table:

err_mbr:

    free(part->block);
    part->block = NULL;
err_read_mbr:

    free(part);
err_alloc_iter:

    return NULL;
}

struct disk_part_iter *rawio_get_partition(int drive, int partnum)
{
    const union syslinux_derivative_info *sdi;
    struct disk_part_iter *cur_part = NULL;

    if (drive < 0) {
	sdi = syslinux_derivative_info();
	drive = sdi->disk.drive_number;
    }

    dprintf("drive is %02x\n", drive);

    /* Get the disk geometry and disk access setup */
    if (rawio_get_disk_params(drive)) {
	error("Cannot get disk parameters\n");
	return NULL;
    }
    dprintf("disk params loaded\n");

    /* Find the specified partiton */
    cur_part = rawio_get_first_partition(NULL);
    dprintf("searching for partition %d\n", partnum);
    while (cur_part) {
	if (cur_part->index == partnum)
	    break;
	cur_part = cur_part->next(cur_part);
    }
    if (!cur_part) {
	error("requested partition not found!\n");
	return NULL;
    }
    return cur_part;
}

