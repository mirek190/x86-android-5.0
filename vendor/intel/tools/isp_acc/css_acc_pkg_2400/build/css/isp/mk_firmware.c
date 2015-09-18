/*
 * INTEL CONFIDENTIAL
 *
 * Copyright (C) 2010 - 2013 Intel Corporation.
 * All Rights Reserved.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or licensors. Title to the Material remains with Intel
 * Corporation or its licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No License under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or
 * delivery of the Materials, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights
 * must be express and approved by Intel in writing.
 */

#include "assert_support.h"
#include "mk_firmware.h"

#if defined(TARGET_crun)
#define C_RUN
#endif

#include "sh_css_defs.h"

bool verbose;

static bool
supports_output_format(const struct ia_css_binary_xinfo *info,
		       enum ia_css_frame_format format)
{
	int i;

	for (i = 0; i < info->num_output_formats; i++) {
		if (info->output_formats[i] == format)
			return true;
	}
	return false;
}

static size_t
ffill(const size_t N,
	FILE *fid)
{
	size_t n;

	char zero = 0;
	for (n = 0; n < N; n++) {
		fwrite(&zero, sizeof(char), 1, fid);
	}

	return n;
}

static void
write_firmware(const char *filename,
	       const struct ia_css_blob_info *blob_original)
{
	FILE *binary;
	const char name[] = STR(APPL);

	binary = fopen(filename, "wb");

	if (!binary) {
		fprintf(stderr, "Cannot open firmware file %s\n", filename);
		exit(1);
	}
	if (verbose) {
		fprintf(stdout, "FW name : %s\n\n", name);
	}

	fwrite(&firmware_header, 1, sizeof(firmware_header), binary);
	if (mem_offsets)
		fwrite(mem_offsets, 1, mem_offsets_size, binary);
	if (conf_mem_offsets)
		fwrite(conf_mem_offsets, 1, conf_mem_offsets_size, binary);
	if (state_mem_offsets)
		fwrite(state_mem_offsets, 1, state_mem_offsets_size, binary);
	fwrite(name, 1, strlen(name) + 1, binary);
	if (blob != NULL) {
		struct ia_css_blob_info	*blob_aligned = &(firmware_header.blob);
		size_t size = 0;

		if (firmware_header.blob.padding_size == 0) {
		    /* MW: This path is redundant */
		    size += fwrite(blob, 1, firmware_header.blob.size,
				   binary);
		} else {
		    /* Again assume a fixed order of sections */
		    /* Fill to an aligned boundary */
		    size += ffill(blob_aligned->icache_padding, binary);
		    size += fwrite(blob + blob_original->icache_source,
				   1, blob_original->icache_size, binary);

		    size += ffill(blob_aligned->text_padding, binary);
		    size += fwrite(blob + blob_original->text_source,
				   1, blob_original->text_size, binary);

		    size += ffill(blob_aligned->data_padding, binary);
		    size += fwrite(blob + blob_original->data_source,
				   1, blob_original->data_size, binary);
		}
		assert(blob_aligned->size == size);
	}

	fclose(binary);
}

int
main(unsigned argc, char **argv)
{
    struct ia_css_blob_info	blob_original;
    const char *filename;

    if (argc != 2) {
	fprintf(stderr, "Incorrect number of arguments\n");
	exit(1);
    }
    filename = argv[1];

    /* verbose = (strcomp(argv[2], "verbose") == 0); */
    verbose = true;

    blob_original = fill_header();

    write_firmware(filename, &blob_original);

    return 0;
}
