#ifndef DROIDBOOT_UTIL_H
#define DROIDBOOT_UTIL_H

int named_file_write(const char *filename, const unsigned char *what,
		size_t sz);
int named_file_write_decompress_gzip(const char *filename,
		unsigned char *what, size_t sz);
int named_file_write_ext4_sparse(const char *filename,
		unsigned char *what, size_t sz);
int write_ext4_sparse(const char *filename,
		unsigned char *data, size_t sz);

unsigned int getfreememsize(void);
int kexec_linux(char *basepath);
int is_valid_blkdev(const char *node);
/* Attribute specification and -Werror prevents most security shenanigans with
 * this function */
int execute_command(const char *fmt, ...) __attribute__((format(printf,1,2)));
int execute_command_data(void *data, unsigned sz, const char *fmt, ...)
		__attribute__((format(printf,3,4)));
void die(void);
void import_kernel_cmdline(void (*callback)(char *name));

#endif // DROIDBOOT_UTIL_H
