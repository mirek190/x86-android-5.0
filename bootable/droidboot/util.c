/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <zlib.h>
#include <cutils/android_reboot.h>

#include "fastboot.h"
#include "droidboot.h"
#include "droidboot_ui.h"
#include "droidboot_util.h"

#include <sparse_format.h>
#include <sparse/sparse.h>

void die(void)
{
	pr_error("droidboot has encountered an unrecoverable problem, exiting!\n");
	exit(1);
}

#define CHUNK 1024 * 256

int named_file_write_decompress_gzip(const char *filename,
	unsigned char *what, size_t sz)
{
	int data_in_mem, ret;
	unsigned int have;
	z_stream strm;
	FILE *src = NULL, *dest = NULL;
	unsigned char in[CHUNK], out[CHUNK];

	dest = fopen(filename, "w");
	if (!dest) {
		pr_perror("fopen dest");
		return -1;
	}

	if (strncmp((const char*)what, FASTBOOT_DOWNLOAD_TMP_FILE, sz) == 0) {
		data_in_mem = 0;
		src = fopen(FASTBOOT_DOWNLOAD_TMP_FILE, "r");
		if (src == NULL) {
			pr_perror("fopen src");
			ret = -1;
			goto out;
		}
	} else
		data_in_mem = 1;

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit2(&strm, 15 + 32);
	if (ret != Z_OK) {
		pr_error("zlib inflateInit error");
		fclose(dest);
		return ret;
	}

	do {
		if (data_in_mem) {
			strm.avail_in = (sz > CHUNK) ? CHUNK : sz;
			if (strm.avail_in == 0)
				break;
			strm.next_in = what;

			what += strm.avail_in;
			sz -= strm.avail_in;
		} else {
			strm.avail_in = fread(in, 1, CHUNK, src);
			if (strm.avail_in == 0)
				break;
			strm.next_in = in;
		}

		/* run inflate() on input until output buffer not full */
		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			if (ret == Z_STREAM_ERROR) {
				pr_error("zlib state clobbered");
				die();
			}
			switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;     /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				pr_perror("zlib memory/data/corruption error");
				goto out;
			}
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have ||
					ferror(dest)) {
				pr_perror("fwrite");
				ret = -1;
				goto out;
			}
		} while (strm.avail_out == 0);
		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	if (ret == Z_STREAM_END)
		ret = 0;
	else
		pr_error("zlib data error");
out:
	/* clean up and return */
	(void)inflateEnd(&strm);
        if (src)
		fclose(src);
	fclose(dest);
	return ret;
}

static int simg2img(const char * inC, const char * outC)
{
	int in;
	int out;
	int ret;
	struct sparse_file *s;

	out = open(outC, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	if (out < 0) {
		pr_error("Cannot open output file %s\n", outC);
		return -1;
	}

	in = open(inC, O_RDONLY);
	if (in < 0) {
		pr_error("Cannot open input file %s\n", inC);
		 ret = -1;
                 goto closeout;
	}

	s = sparse_file_import(in, true, false);
	if (!s) {
		pr_error("Failed to read sparse file\n");
		ret =-1;
		goto closein;
	}

	ret = lseek(out, 0, SEEK_SET);
	if (ret < 0) {
		pr_error("Cannot set pointer to offset\n");
		goto closein;
	}


	ret = sparse_file_write(s, out, false, false, false);
	if (ret < 0) {
		pr_error("Cannot write output file\n");
	}
	sparse_file_destroy(s);

closein:
	close(in);
closeout:
	close(out);
        return ret;

}

#define SPARSE_TEMP_FILE "/tmp/sparse.img"
int named_file_write_ext4_sparse(const char *filename,
        unsigned char *what, size_t sz)
{
	int ret;
        char * sparseFileName;

	if (strncmp((const char*)what, FASTBOOT_DOWNLOAD_TMP_FILE, sz) == 0) {
		sparseFileName = FASTBOOT_DOWNLOAD_TMP_FILE;
	} else {
		ret = named_file_write(SPARSE_TEMP_FILE, what, sz);
		if (ret) {
			pr_error("writing sparse ext4 image to temporary file\n");
			return -1;
		}
		sparseFileName = SPARSE_TEMP_FILE;
	}

	ret = simg2img(sparseFileName, filename);

	if (ret) {
		pr_error("writing sparse ext4 image failed\n");
		return -1;
	}

	unlink(sparseFileName);

	return 0;
}

int named_file_write(const char *filename, const unsigned char *what,
		size_t sz)
{
	int fd;
	int ret;
	/* By convention, if "what" is equal to FASTBOOT_DOWNLOAD_TMP_FILE
	   then the scratch was not big enough for containing the download,
	   and the file was copied to filesystem directly.
	   We then just rename the file.
	   Probably its not a good idea for flash time to allow cross filesystem renaming.
	   So let's say it is a feature not to handle errno == EXDEV
	*/
	if (strncmp((const char*)what, FASTBOOT_DOWNLOAD_TMP_FILE, sz) == 0) {
		ret = rename((const char*)what, filename);
		if (ret) {
			pr_error("unable to rename file %s: %s\n",
				 filename, strerror(errno));
			return -1;
		}
		return 0;
	}
	fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		pr_error("file_write: Can't open file %s: %s\n",
				filename, strerror(errno));
		return -1;
	}

	while (sz) {
		pr_verbose("write() %zu bytes to %s\n", sz, filename);
		ret = write(fd, what, sz);
		if (ret <= 0 && errno != EINTR) {
			pr_error("file_write: Failed to write to %s: %s\n",
					filename, strerror(errno));
			close(fd);
			return -1;
		}
		what += ret;
		sz -= ret;
	}
	close(fd);
	return 0;
}

int write_ext4_sparse(const char *filename, unsigned char *data, size_t sz) {
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	int dest;
	unsigned int chunk;
	unsigned long chunk_data_sz;
	long position;
	uint32_t total_blocks = 0;
	int ret = 0;

	dest = open(filename, O_RDWR);
	if (dest == -1) {
		pr_error("open %s fail : %s\n", filename, strerror(errno));
		return -1;
	}

	/* Read and skip over sparse image header */
	sparse_header = (sparse_header_t *) data;
	data += sizeof(sparse_header_t);
	if(sparse_header->file_hdr_sz > sizeof(sparse_header_t)) {
		/* Skip the remaining bytes in a header that is longer than
		* we expected. */
		data += (sparse_header->file_hdr_sz - sizeof(sparse_header_t));
	}

	/* Start processing chunks */
	for (chunk=0; chunk<sparse_header->total_chunks; chunk++) {
		/* Read and skip over chunk header */
		chunk_header = (chunk_header_t *) data;
		data += sizeof(chunk_header_t);

		if(sparse_header->chunk_hdr_sz > sizeof(chunk_header_t)) {
			/* Skip the remaining bytes in a header that is longer than
			* we expected.
			*/
			data += (sparse_header->chunk_hdr_sz - sizeof(chunk_header_t));
		}

		chunk_data_sz = sparse_header->blk_sz * chunk_header->chunk_sz;
		switch (chunk_header->chunk_type) {
			case CHUNK_TYPE_RAW:
				if(chunk_header->total_sz != (sparse_header->chunk_hdr_sz + chunk_data_sz)) {
					pr_perror("Bogus chunk size for chunk type Raw\n");
					ret = -1;
					goto exit;
				}

				if (write(dest, data, chunk_data_sz) != (long)chunk_data_sz) {
					pr_perror("write failure for chunk type Raw\n");
					ret = -1;
					goto exit;
				}

				total_blocks += chunk_header->chunk_sz;
				data += chunk_data_sz;
				break;

			case CHUNK_TYPE_DONT_CARE:
				total_blocks += chunk_header->chunk_sz;
				if (((position = lseek64(dest, chunk_data_sz, SEEK_CUR)) < 0) &&
					(position != (long)(total_blocks * sparse_header->blk_sz))) {
					perror("Bogus chunk size for chunk type Don't care\n ");
					ret = -1;
					goto exit;
				}
				break;

			case CHUNK_TYPE_CRC32:
				if(chunk_header->total_sz != sparse_header->chunk_hdr_sz) {
					pr_error("Bogus chunk size for chunk type CRC32\n");
					ret = -1;
					goto exit;
				}
				total_blocks += chunk_header->chunk_sz;
				data +=  4;
				break;

			default:
				pr_perror("Unknown chunk type\n");
				ret =-1;
				goto exit;
		}
	}
	if (total_blocks != sparse_header->total_blks) {
		pr_perror("Sparse image write failure\n");
		ret = -1;
	}

exit:
	close (dest);
	return ret;
}


#ifdef DROIDBOOT_SHELL_UTILS
int execute_command(const char *fmt, ...)
{
	int ret = -1;
	va_list ap;
	char *cmd;

	va_start(ap, fmt);
	if (vasprintf(&cmd, fmt, ap) < 0) {
		pr_perror("vasprintf");
		return -1;
	}
	va_end(ap);

	pr_debug("Executing: '%s'\n", cmd);
	ret = system(cmd);

	if (ret < 0) {
		pr_error("Error while trying to execute '%s': %s\n",
			cmd, strerror(errno));
		goto out;
	}
	ret = WEXITSTATUS(ret);
	pr_debug("Done executing '%s' (retval=%d)\n", cmd, ret);
out:
	free(cmd);
	return ret;
}

int execute_command_data(void *data, unsigned sz, const char *fmt, ...)
{
	int ret = -1;
	va_list ap;
	char *cmd;
	FILE *fp;
	size_t bytes_written;

	va_start(ap, fmt);
	if (vasprintf(&cmd, fmt, ap) < 0) {
		pr_perror("vasprintf");
		return -1;
	}
	va_end(ap);

	pr_debug("Executing: '%s'\n", cmd);
	fp = popen(cmd, "w");
	free(cmd);
	if (!fp) {
		pr_perror("popen");
		return -1;
	}

	bytes_written = fwrite(data, 1, sz, fp);
	if (bytes_written != sz) {
		pr_perror("fwrite");
		pclose(fp);
		return -1;
	}

	ret = pclose(fp);
	if (ret < 0) {
		pr_perror("pclose");
		return -1;
	}
	ret = WEXITSTATUS(ret);
	pr_debug("Execution complete, retval=%d\n", ret);

	return ret;
}

#endif

int is_valid_blkdev(const char *node)
{
	struct stat statbuf;
	if (stat(node, &statbuf)) {
		pr_perror("stat");
		return 0;
	}
	if (!S_ISBLK(statbuf.st_mode)) {
		pr_error("%s is not a block device\n", node);
		return 0;
	}
	return 1;
}

/* Taken from Android init, which also pulls runtime options
 * out of the kernel command line
 * FIXME: params can't have spaces */
void import_kernel_cmdline(void (*callback)(char *name))
{
	char cmdline[1024];
	char *ptr;
	int fd;

	fd = open("/proc/cmdline", O_RDONLY);
	if (fd >= 0) {
		int n = read(fd, cmdline, 1023);
		if (n < 0) n = 0;

		/* get rid of trailing newline, it happens */
		if (n > 0 && cmdline[n-1] == '\n')
			n--;

		cmdline[n] = 0;
		close(fd);
	} else {
		cmdline[0] = 0;
	}

	ptr = cmdline;
	while (ptr && *ptr) {
		char *x = strchr(ptr, ' ');
		if (x != 0)
			*x++ = 0;
		callback(ptr);
		ptr = x;
	}
}

#define FREE_MEM 	"MemFree:"
#define FREE_MEM_SIZE 	sizeof(FREE_MEM) -1
unsigned int getfreememsize(void)
{
	FILE *f = NULL;
	long size = 0;
	char str[256];

	f = fopen("/proc/meminfo", "r");

	if (f == NULL) {
		pr_perror("fopen meminfo file");
		return 0;
	}

	while (NULL != fgets(str, sizeof(str), f)) {
		str[sizeof(str)-1] = 0;
		if (0 == memcmp(str, FREE_MEM, FREE_MEM_SIZE)) {
			if (1 == sscanf(str + FREE_MEM_SIZE, "%ld", &size)) {
				break;
			}
		}
	}
	fclose(f);

	return size;
}
