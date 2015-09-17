#ifndef _RAWIO_H_
#define _RAWIO_H_

#include <stdint.h>

#define SECTOR 512		/* bytes/sector */
#define size_to_sectors(sz) ((((sz) - 1) / SECTOR) + 1)

/*
 * CHS (cylinder, head, sector) value extraction macros.
 * Taken from WinVBlock.  Does not expand to an lvalue
*/
#define     chs_head(chs) chs[0]
#define   chs_sector(chs) (chs[1] & 0x3F)
#define chs_cyl_high(chs) (((uint16_t)(chs[1] & 0xC0)) << 2)
#define  chs_cyl_low(chs) ((uint16_t)chs[2])
#define chs_cylinder(chs) (chs_cyl_high(chs) | chs_cyl_low(chs))

typedef uint8_t chs[3];

/* A DOS partition table entry */
struct part_entry {
    uint8_t active_flag;        /* 0x80 if "active" */
    chs start;
    uint8_t ostype;
    chs end;
    uint32_t start_lba;
    uint32_t length;
} __attribute__ ((packed));

/* Forward declaration */
struct disk_part_iter;

/* Partition-/scheme-specific routine returning the next partition */
typedef struct disk_part_iter *(*disk_part_iter_func) (struct disk_part_iter *
                                                       part);

/* Contains details for a partition under examination */
struct disk_part_iter {
    /* The block holding the table we are part of */
    char *block;
    /* The LBA for the beginning of data */
    uint64_t lba_data;
    /* The partition number, as determined by our heuristic */
    int index;
    /* The DOS partition record to pass, if applicable */
    const struct part_entry *record;
    /* Function returning the next available partition */
    disk_part_iter_func next;
    /* Partition-/scheme-specific details */
    union {
        /* MBR specifics */
        int mbr_index;
        /* EBR specifics */
        struct {
            /* The first extended partition's start LBA */
            uint64_t lba_extended;
            /* Any applicable parent, or NULL */
            struct disk_part_iter *parent;
            /* The parent extended partition index */
            int parent_index;
        } ebr;
        /* GPT specifics */
        struct {
            /* Real (not effective) index in the partition table */
            int index;
            /* Current partition GUID */
            const struct guid *part_guid;
            /* Current partition label */
            const char *part_label;
            /* Count of entries in GPT */
            int parts;
            /* Partition record size */
            uint32_t size;
        } gpt;
    } private;
};

int rawio_get_disk_params(int disk);
struct disk_part_iter *rawio_get_first_partition(struct disk_part_iter *part);
void *rawio_read_sectors(uint64_t lba, unsigned int count);
int rawio_write_sector(unsigned int lba, const void *data);
int write_verify_sector(unsigned int lba, const void *buf);

/* If drive is -1, assume current boot disk */
struct disk_part_iter *rawio_get_partition(int drive, int partnum);

#endif
