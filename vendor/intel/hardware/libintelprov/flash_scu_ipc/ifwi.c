#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cutils/properties.h>

#include "util.h"
#include "fw_version_check.h"

#define DNX_SYSFS_INT		"/sys/devices/ipc/intel_fw_update.0/dnx"
#define DNX_SYSFS_INT_ALT   "/sys/kernel/fw_update/dnx"

#define IFWI_SYSFS_INT		"/sys/devices/ipc/intel_fw_update.0/ifwi"
#define IFWI_SYSFS_INT_ALT  "/sys/kernel/fw_update/ifwi"

#define BUF_SIZ	4096

#define IPC_DEVICE_NAME		"/dev/mid_ipc"
#define DEVICE_FW_UPGRADE	0xA4

#define PTI_ENABLE_BIT		(1<<7)	/* For testing if PTI is enabled or disabled. */

#define pr_perror(x)	fprintf(stderr, "update_ifwi_image: %s failed: %s\n", \
		x, strerror(errno))

#define CLVT_MINOR_CHECK 0x80	/* Mask applied to check IFWI compliance */

struct update_info {
	uint32_t ifwi_size;
	uint32_t reset_after_update;
	uint32_t reserved;
};

static int ifwi_downgrade_allowed(const char *ifwi)
{
	uint8_t pti_field;

	if (crack_update_fw_pti_field(ifwi, &pti_field)) {
		fprintf(stderr, "Coudn't crack ifwi file to get PTI field!\n");
		return -1;
	}

	/* The SMIP offset 0x30C bit 7 indicates if the PTI is enabled/disabled. */
	/* If PTI is enabled, DEV/DBG IFWI: IFWI downgrade allowed.              */
	/* If PTI is disabled, end user/PROD IFWI: IFWI downgrade not allowed.   */

	if (pti_field & PTI_ENABLE_BIT)
		return 1;

	return 0;
}

static int retry_write(char *buf, int cont, FILE * f)
{
	int w_bytes = 0, retry = 0;

	while (w_bytes < cont && retry++ < 3) {
		w_bytes += fwrite(buf + w_bytes, 1, cont - w_bytes, f);
		if (w_bytes < cont)
			sleep(1);
	}

	if (w_bytes < cont) {
		fprintf(stderr, "retry_write error!\n");
		return -1;
	}
	return 0;
}

int update_ifwi_file_scu_ipc(const char *dnx, const char *ifwi)
{
	int ret = 0;
	int ifwi_allowed;
	size_t cont;
	char buff[BUF_SIZ];
	FILE *f_src, *f_dst;
	struct fw_version img_ifwi_rev;
	struct firmware_versions dev_fw_rev;

	if (crack_update_fw(ifwi, &img_ifwi_rev)) {
		fprintf(stderr, "Coudn't crack ifwi file!\n");
		return -1;
	}
	if (get_current_fw_rev(&dev_fw_rev)) {
		fprintf(stderr, "Couldn't query existing IFWI version\n");
		return -1;
	}

	/* Check if this IFWI file can be updated. */
	ifwi_allowed = ifwi_downgrade_allowed(ifwi);

	if (ifwi_allowed == -1) {
		fprintf(stderr, "Couldn't get PTI information from ifwi file\n");
		return -1;
	}

	if (img_ifwi_rev.major != dev_fw_rev.ifwi.major) {
		fprintf(stderr,
			"IFWI FW Major version numbers (file=%02X current=%02X) don't match, Update abort.\n",
			img_ifwi_rev.major, dev_fw_rev.ifwi.major);

		/* Not an error case. Let update continue to next IFWI versions. */
		goto end;
	}
#ifdef CLVT
	if ((img_ifwi_rev.minor & CLVT_MINOR_CHECK) != (dev_fw_rev.ifwi.minor & CLVT_MINOR_CHECK)) {
		fprintf(stderr,
			"IFWI FW Minor version numbers (file=%02X current=%02X mask=%02X) don't match, Update abort.\n",
			img_ifwi_rev.minor, dev_fw_rev.ifwi.minor, CLVT_MINOR_CHECK);

		/* Not an error case. Let update continue to next IFWI versions. */
		goto end;
	}
#endif

	if (img_ifwi_rev.minor < dev_fw_rev.ifwi.minor) {
		if (!ifwi_allowed) {
			fprintf(stderr,
				"IFWI FW Minor downgrade not allowed (file=%02X current=%02X). Update abort.\n",
				img_ifwi_rev.minor, dev_fw_rev.ifwi.minor);

			/* Not an error case. Let update continue to next IFWI versions. */
			goto end;
		} else {
			fprintf(stderr, "IFWI FW Minor downgrade allowed (file=%02X current=%02X).\n",
				img_ifwi_rev.minor, dev_fw_rev.ifwi.minor);
		}
	}

	if (img_ifwi_rev.minor == dev_fw_rev.ifwi.minor) {
		fprintf(stderr,
			"IFWI FW Minor is not new than board's existing version (file=%02X current=%02X), Update abort.\n",
			img_ifwi_rev.minor, dev_fw_rev.ifwi.minor);

		/* Not an error case. Let update continue to next IFWI versions. */
		goto end;
	}

	fprintf(stderr, "Found IFWI to be flashed (maj=%02X min=%02X)\n", img_ifwi_rev.major,
		img_ifwi_rev.minor);

	f_src = fopen(dnx, "rb");
	if (f_src == NULL) {
		fprintf(stderr, "open %s failed\n", dnx);
		ret = -1;
		goto end;
	}

	f_dst = fopen(DNX_SYSFS_INT, "wb");
	if (f_dst == NULL) {
		f_dst = fopen(DNX_SYSFS_INT_ALT, "wb");
		if (f_dst == NULL) {
			fprintf(stderr, "open %s failed\n", DNX_SYSFS_INT_ALT);
			ret = -1;
			goto err;
		}
	}

	while ((cont = fread(buff, 1, sizeof(buff), f_src)) > 0) {
		if (retry_write(buff, cont, f_dst) == -1) {
			fprintf(stderr, "DNX write failed\n");
			fclose(f_dst);
			ret = -1;
			goto err;
		}
	}

	fclose(f_src);
	fclose(f_dst);

	f_src = fopen(ifwi, "rb");
	if (f_src == NULL) {
		fprintf(stderr, "open %s failed\n", ifwi);
		ret = -1;
		goto end;
	}

	f_dst = fopen(IFWI_SYSFS_INT, "wb");
	if (f_dst == NULL) {
		f_dst = fopen(IFWI_SYSFS_INT_ALT, "wb");
		if (f_dst == NULL) {
			fprintf(stderr, "open %s failed\n", IFWI_SYSFS_INT_ALT);
			ret = -1;
			goto err;
		}
	}

	while ((cont = fread(buff, 1, sizeof(buff), f_src)) > 0) {
		if (retry_write(buff, cont, f_dst) == -1) {
			fprintf(stderr, "IFWI write failed\n");
			fclose(f_dst);
			ret = -1;
			goto err;
		}
	}

	fclose(f_dst);

err:
	fclose(f_src);
end:
	fprintf(stderr, "IFWI flashed\n");
	return ret;
}

int update_ifwi_image_scu_ipc(void *data, size_t size, unsigned reset_flag)
{
	struct update_info *packet;
	int ret = -1;
	int fd;
	struct firmware_versions img_fw_rev;
	struct firmware_versions dev_fw_rev;

	/* Sanity check: If the Major version numbers do not match
	 * refuse to install; the versioning scheme in use encodes
	 * the device type in the major version number. This is not
	 * terribly robust but there isn't any additional metadata
	 * encoded within the IFWI image that can help us */
	if (get_image_fw_rev(data, size, &img_fw_rev)) {
		fprintf(stderr, "update_ifwi_image: Coudn't extract FW " "version data from image\n");
		return -1;
	}
	if (get_current_fw_rev(&dev_fw_rev)) {
		fprintf(stderr, "update_ifwi_image: Couldn't query existing " "IFWI version\n");
		return -1;
	}
	if (img_fw_rev.ifwi.major != dev_fw_rev.ifwi.major) {
		fprintf(stderr, "update_ifwi_image: IFWI FW Major version "
			"numbers (file=%02X current=%02X don't match. "
			"Abort.\n", img_fw_rev.ifwi.major, dev_fw_rev.ifwi.major);
		return -1;
	}

	packet = malloc(size + sizeof(struct update_info));
	if (!packet) {
		pr_perror("malloc");
		return -1;
	}

	memcpy(packet + 1, data, size);
	packet->ifwi_size = size;
	packet->reset_after_update = reset_flag;
	packet->reserved = 0;

	printf("update_ifwi_image -- size: %d reset: %d\n", packet->ifwi_size, packet->reset_after_update);
	fd = open(IPC_DEVICE_NAME, O_RDWR);
	if (fd < 0) {
		pr_perror("open");
		goto out;
	}
	sync();			/* reduce the chance of EMMC contention */
	ret = ioctl(fd, DEVICE_FW_UPGRADE, packet);
	close(fd);
	if (ret < 0)
		pr_perror("DEVICE_FW_UPGRADE");
out:
	free(packet);
	return ret;
}

#define BIN_DNX  "/tmp/__dnx.bin"
#define BIN_IFWI "/tmp/__ifwi.bin"

int flash_dnx_scu_ipc(void *data, unsigned sz)
{
	if (file_write(BIN_DNX, data, sz)) {
		error("Couldn't write dnx file to %s\n", BIN_DNX);
		return -1;
	}

	return 0;
}

int flash_ifwi_scu_ipc(void *data, unsigned sz)
{
	struct firmware_versions img_fw_rev;

	if (access(BIN_DNX, F_OK)) {
		error("dnx binary must be flashed to board first\n");
		return -1;
	}

	if (get_image_fw_rev(data, sz, &img_fw_rev)) {
		error("Coudn't extract FW version data from image");
		return -1;
	}

	printf("Image FW versions:\n");
	dump_fw_versions(&img_fw_rev);

	if (file_write(BIN_IFWI, data, sz)) {
		error("Couldn't write ifwi file to %s\n", BIN_IFWI);
		return -1;
	}

	if (update_ifwi_file_scu_ipc(BIN_DNX, BIN_IFWI)) {
		error("IFWI flashing failed!");
		return -1;
	}
	return 0;
}

bool is_scu_ipc(void)
{
        /* MIA component does not exist on cvt based boards */
        char value[PROPERTY_VALUE_MAX];
        return property_get("sys.mia.version", value, "") == 0;
}
