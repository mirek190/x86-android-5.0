/*
 * Copyright (C) 2011-2014 Intel Corporation
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "recovery"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#include <cutils/log.h>
#include <mincrypt/sha.h>
#include <applypatch.h>

#include <bootimg.h>

#include "flash.h"
#include "update_osip.h"
#include "util.h"

/* Needs to agree with ota_from_target_files.MakeRecoveryPatch() */
#define LOGPERROR(x)	ALOGE("%s failed: %s", x, strerror(errno))

#define TGT_SIZE_MAX    (LBA_SIZE * OS_MAX_LBA)

/* Compare the first SIG_SIZE bytes of the current recovery.img
 * with the SHA1 passed in, to determine if we need to update it.
 * These last bytes have the digital signature from LFSTK and would
 * not be the same. This is much faster than hashing the entire image */
static int check_recovery_header(const char *tgt_sha1, int *needs_patching)
{
	int sig_size;
	void *buf;
	uint8_t src_digest[SHA_DIGEST_SIZE];
	uint8_t tgt_digest[SHA_DIGEST_SIZE];

	if (ParseSha1(tgt_sha1, tgt_digest)) {
		ALOGE("Bad SHA1 digest passed in");
		return -1;
	}

	sig_size = read_image_signature(&buf, RECOVERY_OS_NAME);
	if (sig_size == -1) {
		ALOGE("Failed to read boot signature\n");
		return -1;
	}
	SHA_hash(buf, sig_size, src_digest);
	free(buf);

	*needs_patching = memcmp(src_digest, tgt_digest, SHA_DIGEST_SIZE);
	return 0;
}

static int check_recovery_image(const char *tgt_sha1, int *needs_patching)
{
	void *data;
	int sz;
	uint8_t tgt_digest[SHA_DIGEST_SIZE];
	uint8_t expected_tgt_digest[SHA_DIGEST_SIZE];

	if (ParseSha1(tgt_sha1, expected_tgt_digest)) {
		ALOGE("Bad SHA1 digest passed in");
		return -1;
	}
	printf("Good SHA1 digest passed in\n");

	if ((sz = read_image(RECOVERY_OS_NAME, &data)) == -1) {
		ALOGE("failed to read recovery image");
		*needs_patching = 1;
		return 0;
	}
	printf("read recovery image success\n");
	SHA_hash(data, sz, tgt_digest);

	*needs_patching = memcmp(tgt_digest, expected_tgt_digest, SHA_DIGEST_SIZE);
	free(data);
	return 0;
}

typedef struct {
	unsigned char *buffer;
	ssize_t size;
	ssize_t pos;
} MemorySinkInfo;

static ssize_t MemorySink(unsigned char *data, ssize_t len, void *token)
{
	MemorySinkInfo *msi = (MemorySinkInfo *) token;
	if (msi->size - msi->pos < len) {
		return -1;
	}
	memcpy(msi->buffer + msi->pos, data, len);
	msi->pos += len;
	return len;
}

static int patch_recovery(const char *src_sha1, const char *tgt_sha1,
			  unsigned int tgt_size, const char *patchfile)
{
	MemorySinkInfo msi;
	void *src_data;
	void *recovery_data;
	int src_size;
	int recovery_size;
	uint8_t expected_src_digest[SHA_DIGEST_SIZE];
	uint8_t src_digest[SHA_DIGEST_SIZE];
	uint8_t expected_tgt_digest[SHA_DIGEST_SIZE];
	uint8_t tgt_digest[SHA_DIGEST_SIZE];
	SHA_CTX ctx;

	Value patchval;

	msi.buffer = NULL;
	src_data = NULL;
	patchval.data = NULL;
	SHA_init(&ctx);

	if (ParseSha1(src_sha1, expected_src_digest)) {
		ALOGE("Bad SHA1 src SHA1 digest %s passed in", src_sha1);
		return -1;
	}
        printf("src_sha1 : %s\n",src_sha1);
        printf("expected_digest_src_sha1 : %x\n",expected_src_digest);

	if (ParseSha1(tgt_sha1, expected_tgt_digest)) {
		ALOGE("Bad SHA1 tgt SHA1 digest %s passed in", tgt_sha1);
		return -1;
	}

	src_size = read_image(ANDROID_OS_NAME, &src_data);
	recovery_size = read_image(RECOVERY_OS_NAME, &recovery_data);
        printf("read size : %d\n",src_size);
	if (src_size == -1) {
		ALOGE("Failed to read image %s\n", ANDROID_OS_NAME);
		return -1;
	}

	SHA_hash(src_data, src_size, src_digest);
        printf("digests : %x vs %x\n",src_digest,expected_src_digest);
	if (memcmp(src_digest, expected_src_digest, SHA_DIGEST_SIZE)) {
		ALOGE("boot image digests don't match!");
		goto out;
	}

	msi.pos = 0;
	if (tgt_size > TGT_SIZE_MAX) {
		ALOGE("tgt_size is too big!");
		goto out;
	}
	msi.size = tgt_size;
	msi.buffer = malloc(tgt_size);
	if (!msi.buffer) {
		LOGPERROR("malloc");
		goto out;
	}

	if (file_read(patchfile, (void **)&patchval.data, (size_t *) & patchval.size)) {
		ALOGE("Coudln't read patch data");
		goto out;
	}
	patchval.type = VAL_BLOB;

        printf("before applyimagepatch\n");
	if (ApplyImagePatch(src_data, src_size, &patchval, MemorySink, &msi, &ctx, NULL)) {
		ALOGE("Patching process failed");
		goto out;
	}
        printf("after applyimagepatch\n");
	SHA_hash(msi.buffer, msi.size, tgt_digest);
	if (memcmp(tgt_digest, expected_tgt_digest, SHA_DIGEST_SIZE)) {
		ALOGE("output recovery image digest mismatch %s vs %s",tgt_digest,expected_tgt_digest);
		goto out;
	}
	if (flash_recovery_kernel(msi.buffer, msi.size)) {
		ALOGE("error writing patched recovery image");
		goto out;
	}
	ALOGI("Recovery sucessfully patched from boot");

out:
	free(src_data);
	free(patchval.data);
	free(msi.buffer);
	return 0;
}

static void usage(void)
{
	printf("-s | --src-sha1     Expected sha1 of boot image\n");
	printf("-t | --tgt-sha1     Expected sha1 of patched recovery image\n");
	printf("-z | --tgt-size     Size of patched recovery image\n");
	printf("-c | --check-sha1   Expected SHA1 of signature of recovery\n");
	printf("                    If the actual value does not match, patch it\n");
	printf("-p | --patch        Patch file, applied to boot, which creates recovery\n");
	printf("-h | --help         Show this message\n");
}

static struct option long_options[] = {
	{"src-sha1", 1, NULL, 's'},
	{"tgt-sha1", 1, NULL, 't'},
	{"tgt-size", 1, NULL, 'z'},
	{"check-sha1", 1, NULL, 'c'},
	{"patch", 1, NULL, 'p'},
	{"help", 0, NULL, 'h'},
	{0, 0, 0, 0}
};

int main(int argc, char **argv)
{
	int c;
	char *src_sha1 = NULL;
	char *tgt_sha1 = NULL;
	char *check_sha1 = NULL;
	char *patch_file = NULL;
	unsigned int tgt_size = 0;
	int needs_patching;
	int signed_image;

	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "s:t:z:c:p:h", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 's':
			src_sha1 = strdup(optarg);
			break;
		case 't':
			tgt_sha1 = strdup(optarg);
			break;
		case 'z':
			tgt_size = atoi(optarg);
			break;
		case 'c':
			check_sha1 = strdup(optarg);
			break;
		case 'p':
			patch_file = strdup(optarg);
			break;
		case 'h':
			usage();
			exit(0);
		default:
			usage();
			exit(1);
		}
	}

	signed_image = is_image_signed(RECOVERY_OS_NAME);
	if (signed_image == -1) {
		ALOGE("Failed to know if recovery is signed\n");
		exit(EXIT_FAILURE);
	}

	if (!signed_image || !check_sha1) {
		/* We can't do any quick checks if unsigned images
		 * are used. Examine the whole image */
		ALOGI("Checking full recovery image");
		if (tgt_sha1 != NULL) {
			if (check_recovery_image(tgt_sha1, &needs_patching)) {
				ALOGE("Can't examine current recovery console SHA1");
				exit(EXIT_FAILURE);
			}
		} else {
			ALOGE("SHA1 option was not detected correctly");
			exit(EXIT_FAILURE);
		}
	} else {
		ALOGI("Checking recovery image's signature");
		if (check_recovery_header(check_sha1, &needs_patching)) {
			ALOGE("Can't compare recovery console SHA1 sums");
			exit(EXIT_FAILURE);
		}
	}

	if ((src_sha1 != NULL) && (tgt_sha1 != NULL) && (patch_file != NULL) && (tgt_size != 0)) {
		if (needs_patching) {
			ALOGI("Installing new recovery image");
			if (patch_recovery(src_sha1, tgt_sha1, tgt_size, patch_file)) {
				ALOGE("Couldn't patch recovery image");
				exit(EXIT_FAILURE);
			}
		} else {
			ALOGI("Recovery image already installed");
		}
	} else {
		ALOGI("Update options were not successfully detected");
	}
	return 0;
}
