/* Ultra Simple Partitionning library written from:

http://en.wikipedia.org/wiki/Master_boot_record
http://en.wikipedia.org/wiki/Extended_boot_record

to test:
 gcc -D TEST -I ../../../../bootable/recovery microfdisk.c -o microfdisk -g -Wall
*/
#define _LARGEFILE64_SOURCE
#include <linux/fs.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <linux/hdreg.h>

#include "roots.h"
#include "droidboot_ui.h"

#ifdef TEST
#define ui_print printf
#endif

#ifndef STORAGE_BASE_PATH
#define STORAGE_BASE_PATH "/dev/block/mmcblk0"
#endif

/*
  useful ioctls:
 BLKRRPART: re-read partition table
 BLKGETSIZE: return device size /512 (long *arg)
*/
#define MAX_PART 32
struct block_dev {
	int fd;
	long lba_start[MAX_PART];
	long lba_count[MAX_PART];
	int type[MAX_PART];
	int num_partitions;
	int max_lba;
	int sectors;
	int heads;
	int extended_partition_offset;
};
#define MBR_PART_TABLE_START 0x1BE


#define BUF_SIZE		512
#define NUM_PRIMARY_PARTITIONS  3

/* partition types
   http://en.wikipedia.org/wiki/Partition_type
*/
#define EMPTY_PART 0x00
#define EXT_PART 0x05
#define VFAT_PART 0x0C
#define HIDDEN_PART -1
#define LINUX_PART 0x83

#define MB_TO_LBA(x) (x*1024*(1024/BUF_SIZE))
#define LBA_TO_MB(x) (x/(1024*(1024/BUF_SIZE)))

#define DEVICE_CREATION_TIMEOUT		5


/* gap reserved for partition table. (one track)
   http://en.wikipedia.org/wiki/Extended_boot_record#cite_note-fn_2-3
*/
#define EBR_GAP (s->sectors)

struct mbr_part {
	__u8 status;
	__u8 C1;
	__u8 H1;
	__u8 S1;
	__u8 type;
	__u8 C2;
	__u8 H2;
	__u8 S2;
	__u32 lba_start;
	__u32 lba_count;
};
static unsigned char buf[BUF_SIZE];

/* write  the MBRs
   CHS is not supported. It is ignored anyway by linux
 */
static int write_mbr(struct block_dev *s)
{
	struct mbr_part *parts = (struct mbr_part *)(buf + MBR_PART_TABLE_START);
	int i, ret = 0;

	ret = lseek(s->fd, 0, SEEK_SET);
	if (ret == -1) {
		LOGE("lseek error (%s)!\n", strerror(errno));
		return ret;
	}

	ret = read(s->fd, buf, BUF_SIZE);

	if (ret == -1) {
	    LOGE("read error on MBR (%s)\n", strerror(errno));
	    return ret;
	}

	for (i = 0; i < NUM_PRIMARY_PARTITIONS; i++) {
		memset(&parts[i], 0, sizeof(parts[i]));
		parts[i].type = s->type[i];
		parts[i].lba_start = s->lba_start[i];
		parts[i].lba_count = s->lba_count[i];
	}
	memset(&parts[NUM_PRIMARY_PARTITIONS], 0, sizeof(parts[NUM_PRIMARY_PARTITIONS]));
	if(s->num_partitions > NUM_PRIMARY_PARTITIONS)
	{
		parts[NUM_PRIMARY_PARTITIONS].type = EXT_PART;
		parts[NUM_PRIMARY_PARTITIONS].lba_start = s->lba_start[NUM_PRIMARY_PARTITIONS] - EBR_GAP;
		s->extended_partition_offset = parts[NUM_PRIMARY_PARTITIONS].lba_start;
		parts[NUM_PRIMARY_PARTITIONS].lba_count = s->lba_start[s->num_partitions-1]
				     + s->lba_count[s->num_partitions-1]
				     - parts[NUM_PRIMARY_PARTITIONS].lba_start;
	}
	ret = lseek(s->fd, 0, SEEK_SET);
	if (ret == -1) {
		LOGE("lseek position error (%s)!\n", strerror(errno));
		return ret;
	}

	ret = write(s->fd, buf, BUF_SIZE);

	if (ret == -1) {
	    LOGE("failed to write MBR (%s)\n", strerror(errno));
	    return ret;
	}

	return 0;
}
/* write one of the EBRs
   CHS is not supported. It is ignored anyway by linux
 */
static int write_ebr(struct block_dev *s, int i)
{
	int ret = 0;
	struct mbr_part *parts = (struct mbr_part *)(buf + MBR_PART_TABLE_START);

	ret = lseek64(s->fd, (long long)(s->lba_start[i]-EBR_GAP)*BUF_SIZE, SEEK_SET);

	if (ret == -1) {
		LOGE("lseek error on ebr (%s)!\n", strerror(errno));
		return ret;
	}

	ret = read(s->fd, buf, BUF_SIZE);

	if (ret == -1) {
	    LOGE("read error on EBR (%s)\n", strerror(errno));
	    return ret;
	}

	memset(buf, 0, BUF_SIZE);
	parts[0].status = 0;
	parts[0].type = s->type[i];
	parts[0].lba_start = EBR_GAP;
	parts[0].lba_count = s->lba_count[i];
	if (i + 1 < s->num_partitions) {
		parts[1].status = 0;
		parts[1].type = EXT_PART;
		parts[1].lba_start = s->lba_start[i+1]
				     - s->extended_partition_offset
				     - EBR_GAP;
		parts[1].lba_count = s->lba_count[i+1] + EBR_GAP;
	}
	buf[0x1FE]=0x55;
	buf[0x1FF]=0xAA;

	ret = lseek64(s->fd, (long long)(s->lba_start[i]-EBR_GAP)*BUF_SIZE, SEEK_SET);

	if (ret == -1) {
		LOGE("next lseek error on ebr (%s)!\n", strerror(errno));
		return ret;
	}

	ret = write(s->fd, buf, BUF_SIZE);

	if (ret == -1) {
	    LOGE("failed to write EBR (%s)\n", strerror(errno));
	    return ret;
	}

	return 0;
}
/*
   declare a new partition
   @param: size in MB
   @param: type: the filesystem type as defined in:
   http://en.wikipedia.org/wiki/Partition_type
*/
static int new_partition(struct block_dev *s, long size, int type)
{
	static int gap = 0;
	if (type == HIDDEN_PART) {
		gap += MB_TO_LBA(size);
	} else {
		int i = s->num_partitions++;
		s->lba_count[i] = MB_TO_LBA(size);
		s->type[i] = type;
		if (i == 0)
			s->lba_start[i] = EBR_GAP;
		else if (i<NUM_PRIMARY_PARTITIONS)
			s->lba_start[i] = s->lba_start[i-1]
				+ s->lba_count[i-1];
		else
			s->lba_start[i] = s->lba_start[i-1]
				+ s->lba_count[i-1]
				+ EBR_GAP;
		s->lba_start[i] += gap;
		if ( s->max_lba < s->lba_start[i] + s->lba_count[i]) {
			LOGE("no space left for new partition...\n");
			return -ENOSPC;
		}
		gap = 0;
	}
	return 0;
}
/* open the disk, and fetch its info
   only 512b sector disk is supported...
*/
static struct block_dev *open_disk(char *disk)
{
	struct block_dev *ret = NULL;
	int sec_size, r, num_sec;
	struct hd_geometry geo;

	int fd = open(disk,O_RDWR);

	/* Take care to close fd only in case of error next lines. The disk */
	/* shall remain open for use by other functions in this file. It is */
	/* closed at the end of ufdisk_create_partition() once the process  */
	/* has completed.                                                   */
	if (fd < 0) {
		LOGE("unable to open file (%s)\n", strerror(errno));
		return NULL;
	}
	r = ioctl(fd,BLKSSZGET,&sec_size);
	if (r < 0 || sec_size != BUF_SIZE)
	{
		LOGE("sec_size != 512 (%s). What kind of disk is this?\n", strerror(errno));
		close(fd);
		goto end;
	}
	r = ioctl(fd,BLKGETSIZE, &num_sec);
	if (r < 0)
	{
		LOGE("unable to get disk size (%s)\n", strerror(errno));
		close(fd);
		goto end;
	}

        r = ioctl(fd, HDIO_GETGEO, &geo);
	if (r < 0)
	{
		LOGE("unable to get disk geometry (%s)\n", strerror(errno));
		close(fd);
		goto end;
	}
	ret = calloc(1, sizeof(struct block_dev));
	if (ret == NULL) {
		LOGE("calloc error (%s)!\n", strerror(errno));
		close(fd);
		goto end;
	}
	ret->fd = fd;
	ret->max_lba = num_sec;
	ret->sectors = geo.sectors;
	ret->heads = geo.heads;

end:
	return ret;
}
/* write the mbr and ebrs, and ask linux to reload the partitions table */
static int write_partitions(struct block_dev *dev)
{
	int i, r = 0;
	if ( dev->max_lba < dev->lba_start[dev->num_partitions-1]
		+ dev->lba_count[dev->num_partitions-1]){
		LOGE("no space left to write partition...\n");
		return -ENOSPC;
	}
	r = write_mbr(dev);
	if (r < 0) {
		LOGE("write_mbr error %d !\n", r);
		return r;
	}
	for (i=NUM_PRIMARY_PARTITIONS; i<dev->num_partitions; i++) {
	    r = write_ebr(dev, i);

	    if (r < 0) {
		LOGE("unable to write EBRs - error %d\n", r);
		return r;
	    }
	}

	r = ioctl(dev->fd,BLKRRPART,NULL);
	if (r < 0)
	    LOGE("unable to load partition table (%s)\n", strerror(errno));
	return r;
}
extern int num_volumes;
extern Volume* device_volumes;
#define _EMMC_BASEDEVICE STORAGE_BASE_PATH

#ifdef TEST
char *EMMC_BASEDEVICE;
#else
#define EMMC_BASEDEVICE _EMMC_BASEDEVICE
#endif
#define MMC_BLOCK_MAJOR 179
#define BLOCK_EXT_MAJOR 259

int is_emmc(Volume *v)
{
	if (v->device == 0)
		return 0;

#ifdef TEST
	if (EMMC_BASEDEVICE == NULL) {
		LOGE("failed to check if device is emmc. Device is NULL\n");
		return -1;
	}

	if (memcmp(v->device, EMMC_BASEDEVICE, strlen(EMMC_BASEDEVICE)) != 0)
		return 0;
#else
	struct stat stat_buf;
	if (stat(v->device, &stat_buf) == -1)
		return 0;

	if (!S_ISBLK(stat_buf.st_mode) || (major(stat_buf.st_rdev) != MMC_BLOCK_MAJOR &&
		major(stat_buf.st_rdev) != BLOCK_EXT_MAJOR))
		return 0;
#endif

	return 1;

}
void ufdisk_umount_all(void)
{
	int i;
	/* ensure everybody is unmounted */
	for (i = 0; i < num_volumes; ++i) {
		Volume* v = device_volumes+i;
		if (!is_emmc(v))
			continue;
		ensure_path_unmounted(v->mount_point);
	}
}

static int wait_device_creation_timeout(const char *device, int times)
{
	int i, ret = -1;

	for (i = 0; i < times; i++) {
		if (access(device, R_OK) == 0) {
			ret = 0;
			break;
		}
		sleep(1);
	}

	return ret;
}

static int get_partition_size(void)
{
	int num_vol = 0;

	FILE* fstab = fopen("/etc/recovery.fstab", "r");
	if (fstab == NULL) {
		LOGE("failed to open /etc/recovery.fstab (%s)\n", strerror(errno));
		return -1;
	}

	char buffer[32];
	int i;
	while (fgets(buffer, sizeof(buffer)-1, fstab)) {
		for (i = 0; buffer[i] && isspace(buffer[i]); ++i);
		if (buffer[i] == '\0' || buffer[i] != '#') continue;

		char* original = strdup(buffer);

		// read size_hint specified as a comment in recovery.fstab
		char* size_hint = strtok(buffer+i, " \t\n");

		Volume* v = device_volumes+num_vol;

		v->size_hint = 0;
		if (size_hint == NULL || strncmp(size_hint, "#size_hint=", 11) != 0) {
			LOGE("skipping malformed recovery.fstab line: %s\n", original);
		} else {
			v->size_hint = atoi(size_hint+11);
			++num_vol;
		}

		free(original);
	}

	fclose(fstab);

	return 0;
}

int ufdisk_need_create_partition(void)
{
	int i, ret = 0;

	if(get_partition_size() != 0)
		return -1;

	for (i = 0; i < num_volumes; ++i) {
		Volume* v = device_volumes+i;
		if (!is_emmc(v) || strcmp(v->fs_type, "hidden") == 0 || v->size_hint < 0)
			continue;
		if (access(v->device, R_OK) != 0)
			ret = -1;
	}
	return ret;
}

int ufdisk_create_partition(void)
{
	int i;
	int num_auto_part = 0;
	int allocated_space = 0;
	int max_space;
	int auto_size = 1;
	int r;
	struct block_dev *dev;

	dev = open_disk(EMMC_BASEDEVICE);
	if (dev == NULL)
		return -ENODEV;

	if(get_partition_size() != 0)
		return -EINVAL;

	/* first pass to calculate remaining space,
	   and check if we need to partition */
	for (i = 0; i < num_volumes; ++i) {
		Volume* v = device_volumes+i;
		if (!is_emmc(v))
			continue;

		if (v->size_hint > 0) /* +1 for EBR_GAPS */
			allocated_space += v->size_hint + 1;
		else if (v->size_hint == 0)
			num_auto_part++;
	}

	ufdisk_umount_all();

	max_space = LBA_TO_MB(dev->max_lba);
	if (max_space < allocated_space) {
		LOGE("emmc is too small for this partition table! %dM VS %dM\n",
			 max_space, allocated_space);
		r = -EINVAL;
		goto exit;
	}
	if (num_auto_part)
		auto_size = ((max_space - allocated_space) / num_auto_part);
	/* start allocating */
	for (i = 0; i < num_volumes; ++i) {
		Volume* v = device_volumes+i;
		int type = LINUX_PART;
		if (!is_emmc(v))
			continue;
		if (v->size_hint == 0)
			v->size_hint = auto_size;
		if (strcmp(v->fs_type, "vfat") == 0)
			type = VFAT_PART;
		if (strcmp(v->fs_type, "hidden") == 0)
			type = HIDDEN_PART;
		LOGI("create new partition %s type %s\n", v->device, v->fs_type);
		r = new_partition(dev, v->size_hint, type);
		if (r) {
			goto exit;
		}
	}
	r = write_partitions(dev);
	if (r) {
		goto exit;
	}

	/* check if everything is good */
	for (i = 0; i < num_volumes; ++i) {
		Volume* v = device_volumes+i;
		if (!is_emmc(v))
			continue;
		if (strcmp(v->fs_type, "hidden") == 0)
			continue;
		if (wait_device_creation_timeout(v->device, DEVICE_CREATION_TIMEOUT) != 0) {
			LOGE("fatal: unable to create partition: %s\n", v->device);
			r = -ENODEV;
			goto exit;
		}
	}

	/* format every body */
	for (i = 0; i < num_volumes; ++i) {
		Volume* v = device_volumes+i;
		if (!is_emmc(v))
			continue;
		if (strcmp(v->fs_type, "ext4") == 0) {
			LOGI("formatting %s\n", v->mount_point);
			if(format_volume(v->mount_point)) {
				LOGE("Could not format %s\n", v->mount_point);
				r = -EINVAL;
			}
		}
	}

exit:
	close(dev->fd);
	return r;
}

#ifdef TEST
int num_volumes;
Volume* device_volumes;
int ensure_path_unmounted(const char* path)
{
	LOGI("would ensure %s is unmounted\n", path);
	return 0;
}
int format_volume(const char* volume)
{
	LOGI("would format %s\n", volume);
	return 0;
}
/* stub version from roots. replaces /dev/block/mmcblk0 by /dev/sdb
   for testing purpose */
void load_volume_table(char *filename) {
    int alloc = 2;
    Volume* tmp_volumes = NULL;
    device_volumes = malloc(alloc * sizeof(Volume));

    if (device_volumes == NULL) {
	LOGE("failed to allocate device volumes (%s)\n", strerror(errno));
        return;
    }

    // Insert an entry for /tmp, which is the ramdisk and is always mounted.
    device_volumes[0].mount_point = "/tmp";
    device_volumes[0].fs_type = "ramdisk";
    device_volumes[0].device = NULL;
    device_volumes[0].length = 0;
    num_volumes = 1;

    FILE* fstab = fopen(filename, "r");
    if (fstab == NULL) {
	LOGE("failed to open /etc/recovery.fstab (%s)\n", strerror(errno));
        return;
    }

    char buffer[1024];
    int i;
    while (fgets(buffer, sizeof(buffer)-1, fstab)) {
        for (i = 0; buffer[i] && isspace(buffer[i]); ++i);
        if (buffer[i] == '\0' || buffer[i] == '#') continue;

        char* original = strdup(buffer);

        char* mount_point = strtok(buffer+i, " \t\n");
        char* fs_type = strtok(NULL, " \t\n");
        char* device = strtok(NULL, " \t\n");
        // lines may optionally have a partition size (in MB) hint to
	// be used by the partitionner
	// if ommitted, the remaining space will be evenly affected
        char* length = strtok(NULL, " \t\n");

        if (mount_point && fs_type && device) {
            while (num_volumes >= alloc) {
                alloc *= 2;
                tmp_volumes = realloc(device_volumes, alloc*sizeof(Volume));

		if (tmp_volumes == NULL) {
		    LOGE("failed to reallocate device volumes (%s)\n", strerror(errno));
		    goto exit;
                }

		device_volumes = tmp_volumes;
            }
            device_volumes[num_volumes].mount_point = strdup(mount_point);
            device_volumes[num_volumes].fs_type = strdup(fs_type);
	    if (memcmp(device, _EMMC_BASEDEVICE, sizeof(_EMMC_BASEDEVICE)-1) == 0) {
		     char *buf = malloc(100);

                     if (buf == NULL)
			    LOGE("failed to allocate buffer (%s)\n", strerror(errno));
			    goto exit;
                     }

		     snprintf(buf, 100,
			     "%s%s",EMMC_BASEDEVICE, device + sizeof(_EMMC_BASEDEVICE));
		     device_volumes[num_volumes].device = buf;
	    } else {
		     device_volumes[num_volumes].device = strdup(device);
	    }

	    device_volumes[num_volumes].length =
		length ? atoi(length) : 0;
	    ++num_volumes;
        } else {
	    LOGE("skipping malformed recovery.fstab line: %s\n", original);
        }
        free(original);
    }

    LOGI("recovery filesystem table\n");
    LOGI("=========================\n");
    for (i = 0; i < num_volumes; ++i) {
        Volume* v = &device_volumes[i];
	LOGI("  %d %s %s %s\n", i, v->mount_point, v->fs_type,
               v->device);
    }
    LOGI("\n");

exit:

    fclose(fstab);

    if (original)
	free original;

    free(device_volumes);
}

int main(int argc, char ** argv)
{
	if (argc != 3) {
		LOGE("usage:\n# sudo %s /dev/sdb /path/to/recovery.fstab\n",argv[0]);
		return 1;
	}
	EMMC_BASEDEVICE = argv[1];
	load_volume_table(argv[2]);
	if (num_volumes == 1)
		return 1;
	ufdisk_ensure_partition_created();
	return 0;
}
#endif
