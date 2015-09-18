/* tools/mkbootimg/mkbootimg.c
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include "mincrypt/sha.h"
#include "bootimg.h"

static void *load_file(const char *fn, unsigned *_sz)
{
    char *data;
    int sz;
    int fd;

    data = 0;
    fd = open(fn, O_RDONLY);
    if(fd < 0) return 0;

    sz = lseek(fd, 0, SEEK_END);
    if(sz < 0) goto oops;

    if(lseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) malloc(sz);
    if(data == 0) goto oops;

    if(read(fd, data, sz) != sz) goto oops;
    close(fd);

    if(_sz) *_sz = sz;
    return data;

oops:
    close(fd);
    if(data != 0) free(data);
    return 0;
}

int usage(void)
{
    fprintf(stderr,"usage: mkbootimg\n"
            "       --kernel <filename>\n"
            "       --ramdisk <filename>\n"
            "       [ --signsize <size in bytes of digital signature> ]\n"
            "       [ --signexec <image signing tool with args> ]\n"
            "       [ --second <2ndbootloader-filename> ]\n"
            "       [ --cmdline <kernel-commandline> ]\n"
            "       [ --board <boardname> ]\n"
            "       [ --base <address> ]\n"
            "       [ --pagesize <pagesize> ]\n"
            "       -o|--output <filename>\n"
            );
    return 1;
}


static size_t pagealign(size_t sz, unsigned pagesize)
{
    return ((sz + (pagesize - 1)) / pagesize) * pagesize;
}

/* caller must check return value to determine if output is validly set
 * return: 0 successful conversion, output holds a valid value
 *         1 conversion failed, output holds an invalid value
 */
static int string_to_long(long int *output, const char *input, int base)
{
    char *endptr = NULL;
    long int val;

    errno = 0;

    val = strtol(input, &endptr, base);

    /* don't care which kind of error*/
    if(errno) {
        perror("strtol");
        return 1;
    }

    if(endptr == input) {
        fprintf(stderr, "no digits\n");
        return 1;
    }

    *output = val;

    return 0;
}

/* return:
 * 1  - child exits successfully
 * -1 - child exits without success or errors happen
 * 0  - child has not exited, as for non-block
 */
static int wait_child_exit(pid_t pid, int block)
{
    int status = 0;
    int ret = -1;
    pid_t w;

    do {
        w = waitpid(pid, &status, block ? 0 : WNOHANG);

        if(w == -1) {
            fprintf(stderr, "Wait child %d failed\n", pid);
            ret = -1;
            goto wait_done;
        } else if(!w) { /* non-block check and no change on child's state */
            ret = 0;
            goto wait_done;
        } else if(WIFEXITED(status)) { /* child exits */
            if(WEXITSTATUS(status) != EXIT_SUCCESS)
                ret = -1;
            else
                ret = 1;
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

wait_done:
    return ret;
}

static int read_write_data(void *outdata, size_t outsz,
        void *indata, size_t insz, int outfd, int infd)
{
    pid_t pid;
    FILE *fp;
    unsigned char *pos;
    size_t remaining;
    size_t ret;
    int status;
    int w;
    int result = 0; /* for parent */

    pid = fork();
    switch (pid) {
    case -1:
        perror("fork");
        exit(EXIT_FAILURE);
    case 0: /* child, writes outgoing data */
        close(infd);

        fp = fdopen(outfd, "w");
        if(!fp) {
            perror("fdopen");
            exit(EXIT_FAILURE);
        }
        remaining = outsz;
        pos = outdata;

        while (remaining) {
            ret = fwrite(pos, 1, remaining, fp);
            if(ferror(fp)) {
                perror("fwrite");
                fclose(fp);
                exit(EXIT_FAILURE);
            }
            remaining -= ret;
            pos += ret;
        }
        fclose(fp);
        exit(EXIT_SUCCESS);
    default: /* parent, read incoming data */
        close(outfd);

        fp = fdopen(infd, "r");
        if(!fp) {
            perror("fdopen");
            result = 1;
            goto rw_done;
        }
        remaining = insz;
        pos = indata;

        while (remaining) {
            if(wait_child_exit(pid, 0)) {
                fprintf(stderr, "child %d exits too early\n", pid);
                result = 1;
                goto rw_done;
            }
            ret = fread(pos, 1, remaining, fp);
            if(ferror(fp)) {
                perror("fread");
                result = 1;
                goto rw_done;
            }
            remaining -= ret;
            pos += ret;

        }

        if(wait_child_exit(pid, 1) != 1) {
            fprintf(stderr, "wait child %d after reading\n",pid);
            result = 1;
            goto rw_done;
        }

rw_done:
        fclose(fp);
        return result;
    }
}

static unsigned char padding[16384] = { 0, };

static int sign_blob(void **sigdata_ptr, void *data, size_t datasize,
            const char *signexec, size_t signsize)
{
    pid_t pid;
    int to_pipe[2];
    int from_pipe[2];
    int ret = 0; /* for parent */

    if(pipe(to_pipe) || pipe(from_pipe)) {
        perror("pipe");
        return 1;
    }

    pid = fork();

    switch (pid) {
    case -1:
        perror("fork");
        return 1;
    case 0: /* child */
        close(to_pipe[1]);
        close(from_pipe[0]);
        /* Redirect appropriate pipe ends to the child process'
         * stdin and stdout */
        if(dup2(to_pipe[0], STDIN_FILENO) < 0 ||
                    dup2(from_pipe[1], STDOUT_FILENO) < 0) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(to_pipe[0]);
        close(from_pipe[1]);
        execlp("sh", "sh", "-c", signexec, NULL);

        /* should not reach here unless something wrong happens */
        perror("execlp");
        exit(EXIT_FAILURE);
    default: /* parent */
        close(to_pipe[0]);
        close(from_pipe[1]);

        ret = read_write_data(data, datasize, *sigdata_ptr, signsize,
                to_pipe[1], from_pipe[0]);

        if(wait_child_exit(pid, 1) != 1)
            fprintf(stderr, "wait child %d to exit\n", pid);
    }

    return ret;
}



static int create_image(unsigned char **image, ssize_t *imgsize,
        struct boot_img_hdr *hdr, void *kernel_data,
        void *ramdisk_data, void *second_data)
{
    unsigned char *bootimg, *pos;
    size_t total_size;
    size_t ps;

    ps = hdr->page_size;

    total_size = pagealign(sizeof(hdr), ps) + pagealign(hdr->kernel_size, ps) +
            pagealign(hdr->ramdisk_size, ps) +
            pagealign(hdr->second_size, ps);
    bootimg = malloc(total_size);
    if(!bootimg) {
        perror("malloc");
        return 1;
    }
    memset(bootimg, 0, total_size);

    pos = bootimg;
    memcpy(pos, hdr, sizeof(*hdr));

    pos += ps;
    memcpy(pos, kernel_data, hdr->kernel_size);

    pos += pagealign(hdr->kernel_size, ps);
    memcpy(pos, ramdisk_data, hdr->ramdisk_size);

    if(second_data) {
        pos += pagealign(hdr->ramdisk_size, ps);
        memcpy(pos, second_data, hdr->second_size);
    }

    *image = bootimg;
    *imgsize = total_size;

    return 0;
}


int main(int argc, char **argv)
{
    boot_img_hdr hdr;

    char *kernel_fn = 0;
    void *kernel_data = 0;
    char *ramdisk_fn = 0;
    void *ramdisk_data = 0;
    char *second_fn = 0;
    void *second_data = 0;
    char *cmdline = "";
    char *bootimg = 0;
    char *board = "";
    char *signexec = 0;
    unsigned pagesize = 2048;
    long int signsize = 0;
    int fd = -1;
    SHA_CTX ctx;
    const uint8_t* sha;
    unsigned base           = 0x10000000;
    unsigned kernel_offset  = 0x00008000;
    unsigned ramdisk_offset = 0x01000000;
    unsigned second_offset  = 0x00f00000;
    unsigned tags_offset    = 0x00000100;
    size_t cmdlen;
    unsigned char *outimg;
    unsigned char *sigdata = NULL;
    ssize_t outsize;
    ssize_t sig_size_aligned = 0;
    int ret = 1;

    argc--;
    argv++;

    memset(&hdr, 0, sizeof(hdr));

    while(argc > 0){
        char *arg = argv[0];
        char *val = argv[1];
        if(argc < 2) {
            return usage();
        }
        argc -= 2;
        argv += 2;
        if(!strcmp(arg, "--output") || !strcmp(arg, "-o")) {
            bootimg = val;
        } else if(!strcmp(arg, "--kernel")) {
            kernel_fn = val;
        } else if(!strcmp(arg, "--ramdisk")) {
            ramdisk_fn = val;
        } else if(!strcmp(arg, "--second")) {
            second_fn = val;
        } else if(!strcmp(arg, "--cmdline")) {
            cmdline = val;
        } else if(!strcmp(arg, "--base")) {
            base = strtoul(val, 0, 16);
        } else if(!strcmp(arg, "--kernel_offset")) {
            kernel_offset = strtoul(val, 0, 16);
        } else if(!strcmp(arg, "--ramdisk_offset")) {
            ramdisk_offset = strtoul(val, 0, 16);
        } else if(!strcmp(arg, "--second_offset")) {
            second_offset = strtoul(val, 0, 16);
        } else if(!strcmp(arg, "--tags_offset")) {
            tags_offset = strtoul(val, 0, 16);
        } else if(!strcmp(arg, "--board")) {
            board = val;
        } else if(!strcmp(arg,"--pagesize")) {
            pagesize = strtoul(val, 0, 10);
            if ((pagesize != 2048) && (pagesize != 4096)
                && (pagesize != 8192) && (pagesize != 16384)) {
                fprintf(stderr,"error: unsupported page size %d\n", pagesize);
                return ret;
            }
        } else if(!strcmp(arg,"--signexec")) {
            signexec = val;
        } else if(!strcmp(arg,"--signsize")) {
            if(string_to_long(&signsize, val, 0) || signsize <= 0) {
                fprintf(stderr, "error: unsupported signature size\n");
                return ret;
            }

        } else {
            fprintf(stderr,"error: unsupported arg:  %s\n", arg);
            return usage();
        }
    }

    hdr.page_size = pagesize;
    hdr.kernel_addr =  base + kernel_offset;
    hdr.ramdisk_addr = base + ramdisk_offset;
    hdr.second_addr =  base + second_offset;
    hdr.tags_addr =    base + tags_offset;

    if(bootimg == 0) {
        fprintf(stderr,"error: no output filename specified\n");
        return usage();
    }

    if(kernel_fn == 0) {
        fprintf(stderr,"error: no kernel image specified\n");
        return usage();
    }

    if(ramdisk_fn == 0) {
        fprintf(stderr,"error: no ramdisk image specified\n");
        return usage();
    }

    if(strlen(board) >= BOOT_NAME_SIZE) {
        fprintf(stderr,"error: board name too large\n");
        return usage();
    }

    if(!!signexec != !!signsize) {
        fprintf(stderr,"error: --signexec and --signsize must be used together\n");
        return usage();
    } else if(signexec) {
        sig_size_aligned = pagealign(signsize, hdr.page_size);

        sigdata = malloc(sig_size_aligned);

        if(!sigdata) {
            perror("malloc sigdata");
            goto fail;
        }
        memset(sigdata, 0, sig_size_aligned);
    }

    hdr.sig_size = signsize;

    strcpy((char *) hdr.name, board);


    memcpy(hdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

    cmdlen = strlen(cmdline);
    if(cmdlen > (BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE - 2)) {
        fprintf(stderr,"error: kernel commandline too large\n");
        goto fail;
    }
    /* Even if we need to use the supplemental field, ensure we
     * are still NULL-terminated */
    strncpy((char *)hdr.cmdline, cmdline, BOOT_ARGS_SIZE - 1);
    hdr.cmdline[BOOT_ARGS_SIZE - 1] = '\0';
    if (cmdlen >= (BOOT_ARGS_SIZE - 1)) {
        cmdline += (BOOT_ARGS_SIZE - 1);
        strncpy((char *)hdr.extra_cmdline, cmdline, BOOT_EXTRA_ARGS_SIZE);
    }

    kernel_data = load_file(kernel_fn, &hdr.kernel_size);
    if(kernel_data == 0) {
        fprintf(stderr,"error: could not load kernel '%s'\n", kernel_fn);
        goto fail;
    }

    if(!strcmp(ramdisk_fn,"NONE")) {
        ramdisk_data = 0;
        hdr.ramdisk_size = 0;
    } else {
        ramdisk_data = load_file(ramdisk_fn, &hdr.ramdisk_size);
        if(ramdisk_data == 0) {
            fprintf(stderr,"error: could not load ramdisk '%s'\n", ramdisk_fn);
            goto fail;
        }
    }

    if(second_fn) {
        second_data = load_file(second_fn, &hdr.second_size);
        if(second_data == 0) {
            fprintf(stderr,"error: could not load secondstage '%s'\n", second_fn);
            goto fail;
        }
    }

    /* put a hash of the contents in the header so boot images can be
     * differentiated based on their first 2k.
     */
    SHA_init(&ctx);
    SHA_update(&ctx, kernel_data, hdr.kernel_size);
    SHA_update(&ctx, &hdr.kernel_size, sizeof(hdr.kernel_size));
    SHA_update(&ctx, ramdisk_data, hdr.ramdisk_size);
    SHA_update(&ctx, &hdr.ramdisk_size, sizeof(hdr.ramdisk_size));
    SHA_update(&ctx, second_data, hdr.second_size);
    SHA_update(&ctx, &hdr.second_size, sizeof(hdr.second_size));
    sha = SHA_final(&ctx);
    memcpy(hdr.id, sha,
           SHA_DIGEST_SIZE > sizeof(hdr.id) ? sizeof(hdr.id) : SHA_DIGEST_SIZE);

    if(create_image(&outimg, &outsize, &hdr, kernel_data, ramdisk_data,
                second_data))
        goto fail;
    if(signexec) {
        if(sign_blob((void **)&sigdata, outimg, outsize, signexec, signsize)) {
            fprintf(stderr,"error: couldn't sign boot image\n");
            goto fail;
        }
    }
    fd = open(bootimg, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if(fd < 0) {
        fprintf(stderr,"error: could not create '%s'\n", bootimg);
        goto fail;
    }
    if(write(fd, outimg, outsize) != outsize)
        goto fail;
    if(signexec) {
        if(write(fd, sigdata, sig_size_aligned) != sig_size_aligned)
            goto fail;
    }

    ret = 0;

fail:
    free(sigdata);
    if(!(fd < 0))
        close(fd);
    if(ret)
        unlink(bootimg);
    return ret;
}
