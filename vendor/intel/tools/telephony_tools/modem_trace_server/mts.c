/*
 * mts.c (Modem Trace Server):- Server application that routes
 *                              data coming from a tty location to
 *                              another location like a socket.
 *
 * ---------------------------------------------------------------------------
 * Copyright (c) 2011, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * Neither the name of the Intel Corporation nor the
 * names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES,
 * (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ---------------------------------------------------------------------------
 */

/*
 *This program is unit tested only for following use cases in TEST mode:
 *
 *  1) File System (-f) option
 *      - Used a bin file in place of ttyGSM18
 *      - program was able to read and generate same size log file
 *  2) RNDIS / Socket (-p) optind
 *      - Used a bin file in place of ttyGSM18
 *      - Program waits for usb0 interface to become available
 *      - Open socket
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <linux/tty.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <cutils/sockets.h>
#include <cutils/log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cutils/properties.h>
#include <string.h>
#include <stdbool.h>
#include <sys/inotify.h>
#include <time.h>
#include "mmgr_cli.h"

/* Variables to configure MTS */
#undef MTS_INTERTHREAD_LOGGING
#undef MTS_LOGGING_FRAMEWORK
#undef MTS_DROP_DATA

/* Usage text of mts displayed with -h toggle */
#define USAGE_TXT "Usage:\n" \
    "mts -i <input> -t <out_type> -o <output>" \
    "[-r <kbytes>] [-n <count>]]\n" \
    "\n" \
    "\t-i <input>      Input file, usually a tty (/dev/gsmtty18)\n" \
    "\t-t <out_type>   Output type, f for file, p for socket and k for PTI\n" \
    "\t-o <output>     Output target.\n" \
    "\t\t\t type p: port number\n" \
    "\t\t\t type k: tty supporting kernel routing\n" \
    "\t\t\t type f: file name\n" \
    "\t-r <kbytes>     Rotate log every kbytes (10MB if " \
    "unspecified). Requires type f\n" \
    "\t-n <count>      Sets max number of rotated logs to " \
    "<count>, default 4\n" \
    "\t-s <interface>     interface name\n" \
    "\t-b <mbytes>     Size of the buffer between read and write threads\n" \
    "\t-m <instance>   Specify which instance is handled now, the default value is 1\n"

/* Log Size in KBytes : 10Megs */
#define DEFAULT_LOG_SIZE 10*(1<<10)
/* Number of Allowed file rotations will create bplog and bplog.{1-N} where N is
 * DEFAULT_MAX_ROTATED_NR */
#define DEFAULT_MAX_ROTATED_NR 4
/* Check Interval used in parameters of sleep in various loops */
#define CHECK_INTERVAL 2
/* aplog TAG */
#undef LOG_TAG
#define LOG_TAG "BPLOG"
/* Maximum buffer size for read / write system calls */
#define MAXDATA_R 65536
#define MAXDATA_D 8192
#define MAXDATA_W 8192
/* Signal used to un-configure the ldiscs */
#define MY_SIG_UNCONF_PTI SIGUSR1
/* Inotify node to watch */
#define FILE_NAME_SZ 256
#define INOT_USB_DEV_NODE "/dev/bus/usb/001"
#define INOT_EVENT_QUEUE 512
#define INOT_EVENT_SZ sizeof(struct inotify_event)
#define INOT_BUF_SZ INOT_EVENT_QUEUE * (INOT_EVENT_SZ + FILE_NAME_SZ)
/* USB log port */
#define USB_LOG_DEV "/dev/ttyACM1"
/* Poll FDs */
#define POLL_FD_NBR 3
/* Note: FD_LOG needs to be lowest entry in the fd array to ensure that messages from the read
 *       thread are handled before any others as this fd might get flushed in case of modem events
 *       (not doing so might block the write thread when trying to read on this file descriptor
 *       whereas the fd was already flushed).
 */
#define POLL_FD_LOG 0
#define POLL_FD_COM 1
#define POLL_FD_NOT 2

#define MDM_UP "MDM_UP\n"
#define MDM_DW "MDM_DW\n"
#define MDM_SW "MDM_SW\n"
#define MDM_CP "MDM_CP\n"
#define MDM_CT "MDM_CT\n"
#define MDM_MSG_SZ 7

#define ATTEMPTI(fun, predicate, msg, ret) do { \
    if ((*ret = fun)predicate) { \
        LOG_INFO("%s:%d " msg " FAILED when calling %s : returned %d!", \
              __FUNCTION__, __LINE__, #fun, *ret); \
    }}while(0)

#define ATTEMPTE(fun, predicate, msg, ret, label) do { \
    if ((*ret = fun)predicate) { \
        LOG_ERROR("%s:%d " msg " FAILED when calling %s : returned %d!", \
              __FUNCTION__, __LINE__, #fun, *ret); \
        goto label; \
    }}while(0)

/* Length of the full PATH in a file expressed in bytes */
#define PATH_LEN 256

/* BP log file extension */
#define BPLOG_EXT "istp"

/* FALLOC_FL_KEEP_SIZE is not exported by bionic yet, so defining it here until bionic exports it */
#define FALLOC_FL_KEEP_SIZE 0x01
#define PROP_HEAD "persist.service.mts"
#define PROP_HEAD_NON_DEFAULT "persist.sys.mts"

#define MAX_WAIT_MMGR_CONNECT_SECONDS  5
#define MMGR_CONNECT_RETRY_TIME_MS     200

/* Variables needed for communication between read and write threads */
#define BUF_RTOW_IPC_DATA 0
#define BUF_RTOW_IPC_RECOVERABLE_ERROR 1
#define BUF_RTOW_IPC_UNRECOVERABLE_ERROR 2

#define BUF_WTOR_UNLOCK 0
#define BUF_WTOR_STOP 1

#define DEFAULT_BUF_SIZE 4

static char pattern[10] = { "\0" };

static inline void logs_init(int id)
{
    snprintf(pattern, sizeof(pattern), "(MTS_%d)", id);
}

static inline const char *logs_get_pattern()
{
    return pattern;
}

/* define debug LOG functions */
#define LOG_ERROR(format, args ...) \
    do { ALOGE("%s %s - " format, logs_get_pattern(), \
              __FUNCTION__, ## args); } while (0)
#define LOG_DEBUG(format, args ...) \
    do { ALOGD("%s %s - " format, logs_get_pattern(), \
              __FUNCTION__, ## args); } while (0)
#define LOG_VERBOSE(format, args ...) \
    do { ALOGV("%s %s - " format, logs_get_pattern(), \
              __FUNCTION__, ## args); } while (0)
#define LOG_INFO(format, args ...) \
    do { ALOGI("%s %s - " format, logs_get_pattern(), \
              __FUNCTION__, ## args); } while (0)

/* 'Constants' that are updated only at MTS start */
unsigned char *rw_buffer;
unsigned char *rw_buffer_end;
int rw_buffer_size = DEFAULT_BUF_SIZE;

unsigned char *buf_r_ptr; /* Position of next read in the buffer, updated by write thread,
                           * read by read thread */
unsigned char *buf_w_ptr; /* Position of next write to the buffer, updated by read thread,
                           * read by write thread */

/* Wake-up mechanism */
unsigned int buf_ipc_w_pending;
unsigned int buf_ipc_r_locked;
pthread_mutex_t buf_ipc_lock;
int buf_ipc_rtow_fds[2];
int buf_ipc_wtor_fds[2];

/* Read thread */
pthread_t r_thread;

typedef struct args_s
{
    char *name;
    char *key;
    char *type_conv;
    void *storage;
} args;

typedef struct comm_mdm_s
{
    pthread_cond_t modem_online;
    pthread_mutex_t cond_mtx;
    int intercom[2];            /* 0 read - 1 write */
    bool modem_available;
    bool usb_logging;
    int ttyfd;
    int inotfd;
    int inotwd;
    char *p_input;
    mmgr_cli_handle_t *mmgr_hdl;
} comm_mdm;

#ifdef MTS_LOGGING_FRAMEWORK
/* Common */
#define LOG_BUFSIZE   0
#define LOG_READTIME  1
#define LOG_WRITETIME 2
#define LOG_DATARATE  3
#define LOG_DROPRATE  4

int start_log_time;

#define LOGGING_SIZE (65536)
static int logging_buffer[LOGGING_SIZE];
static int log_index = 0;
static pthread_mutex_t log_lock;

/* Buffer size */
#define DIFF_TO_LOG (32 * 1024)

int last_log_time = -1;
int last_log_value = 0;

/* Syscall duration */
#define READ  0
#define WRITE 1

long long syscall_acc[2];
int syscall_nb[2];
int syscall_last[2] = { -1, -1 };

/* Datarate */
int first_sample[2] = { -1, -1 };
long long data_accumulator[2] = { 0, 0 };


static inline void add_datarate_logging(int type, int bytesread)
{
    struct timespec ts;
    int cur_time;

    clock_gettime(CLOCK_BOOTTIME, &ts);
    cur_time = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    cur_time -= start_log_time;

    if (first_sample[type - LOG_DATARATE] < 0) first_sample[type - LOG_DATARATE] = cur_time;

    if (cur_time < (first_sample[type - LOG_DATARATE] + 100))
    {
        data_accumulator[type - LOG_DATARATE] += bytesread;
    }
    else
    {
        pthread_mutex_lock(&log_lock);
        logging_buffer[log_index] = (first_sample[type - LOG_DATARATE] & 0x0FFFFFFF) | (type << 28);
        logging_buffer[log_index + 1] = data_accumulator[type - LOG_DATARATE];
        log_index = (log_index + 2) % LOGGING_SIZE;
        pthread_mutex_unlock(&log_lock);

        first_sample[type - LOG_DATARATE] += 100;
        if ((first_sample[type - LOG_DATARATE] + 100) <= cur_time)
        {
            int stored = first_sample[type - LOG_DATARATE];

            pthread_mutex_lock(&log_lock);
            logging_buffer[log_index] =
                    (first_sample[type - LOG_DATARATE] & 0x0FFFFFFF) | (type << 28);
            logging_buffer[log_index + 1] = 0;
            log_index = (log_index + 2) % LOGGING_SIZE;
            pthread_mutex_unlock(&log_lock);

            first_sample[type - LOG_DATARATE] =
                    cur_time - ((cur_time - first_sample[type - LOG_DATARATE]) % 100);
            if ((first_sample[type - LOG_DATARATE] - 100) > stored)
            {
                pthread_mutex_lock(&log_lock);
                logging_buffer[log_index] =
                        ((first_sample[type - LOG_DATARATE] - 100) & 0x0FFFFFFF) | (type << 28);
                logging_buffer[log_index + 1] = 0;
                log_index = (log_index + 2) % LOGGING_SIZE;
                pthread_mutex_unlock(&log_lock);
            }
        }
        data_accumulator[type - LOG_DATARATE] = bytesread;
    }
}

static inline void add_bufsize_logging(void)
{
    struct timespec ts;
    int cur_time;
    int diff;
    unsigned char *buf_r_ptr_copy, *buf_w_ptr_copy;
    int buf_size;

    pthread_mutex_lock(&log_lock);

    buf_r_ptr_copy = buf_r_ptr;
    buf_w_ptr_copy = buf_w_ptr;

    if (buf_r_ptr_copy > buf_w_ptr_copy)
        buf_size = buf_r_ptr_copy - buf_w_ptr_copy;
    else
        buf_size = (buf_r_ptr_copy + rw_buffer_size) - buf_w_ptr_copy;

    clock_gettime(CLOCK_BOOTTIME, &ts);
    cur_time = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    cur_time -= start_log_time;

    diff = buf_size - last_log_value;
    if ((diff > DIFF_TO_LOG) || (diff < -DIFF_TO_LOG) ||
            (last_log_time < 0) || ((cur_time - last_log_time) > 100))
    {
        logging_buffer[log_index] = (cur_time & 0x0FFFFFFF) | (LOG_BUFSIZE << 28);
        logging_buffer[log_index + 1] = buf_size;
        log_index = (log_index + 2) % LOGGING_SIZE;

        last_log_time = cur_time;
        last_log_value = buf_size;
    }

    pthread_mutex_unlock(&log_lock);
}

static inline void add_syscall_logging(int type, int duration)
{
    struct timespec ts;
    int cur_time;

    clock_gettime(CLOCK_BOOTTIME, &ts);
    cur_time = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    cur_time -= start_log_time;

    if (syscall_last[type] < 0) syscall_last[type] = cur_time;

    if (cur_time >= (syscall_last[type] + 100))
    {
        pthread_mutex_lock(&log_lock);
        logging_buffer[log_index] = (cur_time & 0x0FFFFFFF) | ((LOG_READTIME + type) << 28);
        logging_buffer[log_index + 1] = syscall_acc[type] / syscall_nb[type];
        log_index = (log_index + 2) % LOGGING_SIZE;
        pthread_mutex_unlock(&log_lock);

        syscall_acc[type] = duration;
        syscall_nb[type] = 1;
    }
    else
    {
        syscall_acc[type] += duration;
        syscall_nb[type] += 1;
    }
}

static void *logging_thread(void *param)
{
    int last_log_index = 0;
    int log_fd;

    unlink("/data/MTS_logging.bin");
    log_fd = open("/data/MTS_logging.bin", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (log_fd < 0) return NULL;

    while (1)
    {
        int log_index_cpy;

        usleep(250000);

        add_bufsize_logging();
        log_index_cpy = log_index;
        if (log_index_cpy > last_log_index)
        {
            write(log_fd, &logging_buffer[last_log_index],
                  (log_index_cpy - last_log_index) * sizeof(int));
        }
        else
        {
            write(log_fd, &logging_buffer[last_log_index],
                  (LOGGING_SIZE - last_log_index) * sizeof(int));
            write(log_fd, &logging_buffer[0], log_index_cpy * sizeof(int));
        }
        last_log_index = log_index_cpy;
    }
}

#endif // MTS_LOGGING_FRAMEWORK

static void *log_traces_read_thread(void *param);

static int
open_gsmtty (char *p_portname, comm_mdm *ctx)
{
    struct termios t_termios;
    int tty_fd;
    int ret;

    LOG_INFO ("%s:%d Opening %s", __FUNCTION__, __LINE__, p_portname);
    ATTEMPTE (open (p_portname, O_RDONLY | O_NONBLOCK), <0, "Opening tty",
              &tty_fd, out);
    /*
     * Setting up for tty raw mode, which is based
     * of what ttyIFX0 diagnostic apps developed.
     * I don't think we should quit in the event of an
     * error, but we should inform.
     */
    ATTEMPTI (tcgetattr (tty_fd, &t_termios), !=0, "getting attr's", &ret);
    if (ret == 0)
    {
        cfmakeraw (&t_termios);
        ATTEMPTI (tcsetattr (tty_fd, TCSANOW, &t_termios), !=0,
                  "setting attr's", &ret);
    }
    LOG_INFO ("%s:%d %s opened", __FUNCTION__, __LINE__, p_portname);

    if (tty_fd >= 0)
        pthread_create(&r_thread, NULL, log_traces_read_thread, ctx);

out:
    return tty_fd;
}

static int
iface_up (char *ip_addr_if, char *p_interface)
{
    int fd;
    int if_status = 0;
    struct ifreq ifr;

    fd = socket (AF_INET, SOCK_DGRAM, 0);
    int flags = fcntl (fd, F_GETFL, 0);
    if (fcntl (fd, F_SETFL, flags | O_NONBLOCK) != 0)
        LOG_INFO ("%s:%d Couldn't set O_NONBLOCK on %s socket ... proceeding anyway",
               __FUNCTION__, __LINE__, p_interface);
    memset (&ifr, 0, sizeof (struct ifreq));
    if (IFNAMSIZ > strlen(p_interface))
    {
        strncpy (ifr.ifr_name, p_interface, strlen(p_interface));
        ifr.ifr_addr.sa_family = AF_INET;

        /* Read Flag for interface p_interface, Bit#0 indicates Interface Up/Down */
        if (ioctl (fd, SIOCGIFFLAGS, &ifr) < 0) if_status = 0;
        /* if usb0 is up get its ip address */
        else if (ifr.ifr_flags & 0x01)
        {
            if_status = 1;
            ioctl (fd, SIOCGIFADDR, &ifr);
            strncpy (ip_addr_if, (char*)(inet_ntoa (((struct sockaddr_in *) &ifr.ifr_addr)->
                    sin_addr)), 16);
            LOG_INFO ("%s:%d IPAddress: %s", __FUNCTION__, __LINE__, ip_addr_if);
        }
    }
    else
    {
        ALOGE ("%s:%d ifr.ifr_name buffer (%d) is not big enough to copy p_interface (%zu)",
               __FUNCTION__,__LINE__, IFNAMSIZ, strlen(p_interface));
    }

    close (fd);
    return if_status;
}

static int
init_new_file (char *p_input, int rotate_size)
{
    struct stat st;
    int tfd = -1;
    int ret;
    ret = stat (p_input, &st);
    if (ret == -1 && errno == ENOENT)
    {
        if ((tfd = open (p_input, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) > 0)
            if (fallocate (tfd, FALLOC_FL_KEEP_SIZE, 0, rotate_size * 1024) <
                0)
                LOG_ERROR ("%s:%d Error allocating %d KBytes: %s", __FUNCTION__,
                       __LINE__, rotate_size, strerror (errno));
    }
    else
    {
        LOG_DEBUG ("%s:%d file %s exists, skipping creation", __FUNCTION__,
               __LINE__, p_input);
        tfd = open (p_input, O_WRONLY);
    }

    return tfd;
}

static char *
get_names (char *p_input, int rotate_size, int rotate_num)
{
    static char *fnames = NULL;

    if (rotate_num != 0 && fnames == NULL)
    {
        /* Initialize fnames and allocates .x files if they don't exists */
        fnames = malloc (rotate_num * (PATH_LEN + 1));
        if (fnames == NULL)
            return NULL;
        memset (fnames, '\0', rotate_num * PATH_LEN + 1);
        snprintf (&fnames[0], PATH_LEN, "%s.%s", p_input, BPLOG_EXT);
        close (init_new_file (&fnames[0], rotate_size));
        int i;
        for (i = 1; i < rotate_num; i++)
        {
            snprintf (&fnames[i * PATH_LEN], PATH_LEN, "%s.%d.%s", p_input,
                      i, BPLOG_EXT);
            LOG_DEBUG ("%s:%d \t %s", __FUNCTION__, __LINE__,
                   &fnames[i * PATH_LEN]);
            close (init_new_file (&fnames[i * PATH_LEN], rotate_size));
        }
    }
    return fnames;
}

static int
rotateFile (char *p_input, int fs_fd, int rotate_size, int rotate_num)
{
    int err;
    int i;
    char *fnames = get_names (p_input, rotate_size, rotate_num);

    if (p_input == NULL)
        goto out_err;
    if (fnames == NULL)
        goto out;

    /* close fs_fd in a thread to avoid waiting for a flush on filp_close */
    pthread_t thr_close;
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    pthread_create (&thr_close, &attr, (void *) close, (void*)(intptr_t)fs_fd);
    pthread_attr_destroy (&attr);

    for (i = rotate_num - 2; i >= 0; i--)
    {
        err = rename (&fnames[i * PATH_LEN], &fnames[(i + 1) * PATH_LEN]);
        LOG_DEBUG ("%s:%d renaming log file %s to %s", __FUNCTION__, __LINE__,
               &fnames[i * PATH_LEN], &fnames[(i + 1) * PATH_LEN]);
        if (err < 0 && errno != ENOENT)
            LOG_ERROR ("%s:%d \t error: %s", __FUNCTION__, __LINE__,
                   strerror (errno));
    }

out:
    return init_new_file (&fnames[0], rotate_size);
out_err:
    return -1;
}

static int
get_ldisc_id (const char *ldisc_name)
{
    int val;
    int ret = -1;
    char name[255] = { 0 };
    FILE *fldisc = fopen ("/proc/tty/ldiscs", "r");

    if (fldisc == NULL)
    {
        goto out_noclose;
    }

    while (fscanf (fldisc, "%254s %d", name, &val) == 2)
    {
        /*early return if the ldisc_name is found */
        if (strncmp (ldisc_name, name, strlen (ldisc_name)) == 0)
        {
            ret = val;
            goto out;
        }
    }
out:
    fclose (fldisc);
out_noclose:
    return ret;
}

static int
log_to_pti (char *gsmtty, char *sink, comm_mdm * ctx)
{
    int ldisc_id_sink;
    int ldisc_id_router;
    int ret = -1;
    int rcvd = 0;
    int fs_gsmtty, fs_sink;
    sigset_t sig_mask;

    /* Wait modem ready to start traces */
    if (pthread_mutex_lock (&ctx->cond_mtx) != 0)
    {
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error taking mutex: %s. Exiting wait loop and MTS.",
               __FUNCTION__, __LINE__, strerror (errno));
        goto out;
    }

    while (!ctx->modem_available)
        if (pthread_cond_wait (&ctx->modem_online, &ctx->cond_mtx) != 0)
        {
            LOG_ERROR ("%s:%d PTHREAD_COND_WAIT Error: %s. Exiting.", __FUNCTION__,
                   __LINE__, strerror (errno));
            if (pthread_mutex_unlock (&ctx->cond_mtx) != 0)
                ALOGE ("%s:%d PTHREAD_COND_MUTEX Error releasing mutex: %s.\
                       Exiting wait loop and MTS.",
                       __FUNCTION__, __LINE__, strerror (errno));
            goto out;
        }

    if (pthread_mutex_unlock (&ctx->cond_mtx) != 0)
    {
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error releasing mutex: %s. Exiting wait loop and MTS.",
               __FUNCTION__, __LINE__, strerror (errno));
        goto out;
    }

    LOG_INFO ("%s:%d opening devices", __FUNCTION__, __LINE__);
    ATTEMPTE (open (gsmtty, O_RDWR), <0, "Opening tty", &fs_gsmtty, out);
    ATTEMPTE (open (sink, O_RDWR), <0, "Opening pti", &fs_sink, out_close_tty);

    LOG_INFO ("%s:%d getting ldiscs", __FUNCTION__, __LINE__);
    /*get ldisc ids */
    ATTEMPTE (get_ldisc_id ("n_tracesink"), == -1, "Getting sink ldisc id",
              &ldisc_id_sink, out_close);
    ATTEMPTE (get_ldisc_id ("n_tracerouter"), == -1, "Getting router ldisc id",
              &ldisc_id_router, out_close);

    LOG_INFO ("%s:%d setting ldiscs", __FUNCTION__, __LINE__);
    /*finally set the ldisc to the devices */
    ATTEMPTE (ioctl (fs_gsmtty, TIOCSETD, &ldisc_id_router), <0,
              "Setting router ldisc", &ret, out_close);
    ATTEMPTE (ioctl (fs_sink, TIOCSETD, &ldisc_id_sink), <0,
              "Setting sink ldisc", &ret, out_rm_ldisc_tty);

    LOG_INFO ("%s:%d waiting for MY_SIG_UNCONF_PTI", __FUNCTION__, __LINE__);
    /*blocks the program, waiting for MY_SIG_UNCONF_PTI signal to happen */
    sigemptyset (&sig_mask);
    sigaddset (&sig_mask, MY_SIG_UNCONF_PTI);
    sigprocmask (SIG_BLOCK, &sig_mask, NULL);
    if (sigwait (&sig_mask, &rcvd) == -1)
        LOG_DEBUG ("%s:%d ERROR waiting for signal", __FUNCTION__, __LINE__);
    LOG_INFO ("%s:%d Unconfiguring PTI", __FUNCTION__, __LINE__);

    /*deconfigure ldiscs */

    ioctl (fs_sink, TIOCSETD, (int[])
           {
               0
           });
out_rm_ldisc_tty:
    ioctl (fs_gsmtty, TIOCSETD, (int[])
           {
               0
           });
out_close:
    close (fs_sink);
out_close_tty:
    close (fs_gsmtty);
out:
    return ret;
}

static int
init_file (char *p_input, int *rotate_size, int rotate_num)
{
    int ret;
    int fs_fd = -2;
    struct stat st;
    int err;
    char new_fname[PATH_LEN] = {'\0'};
    bool rotate = false;

    ret = stat (p_input, &st);
    if (ret == 0 && S_ISCHR (st.st_mode))
    {
        /* We output on a character device, just disable file rotation */
        LOG_INFO ("%s:%d file %s is a Characater device, disabling file rotation...",
               __FUNCTION__, __LINE__, p_input);
        fs_fd = open (p_input, O_WRONLY);
        *rotate_size = 0;
    }
    else
    {
        snprintf (new_fname, PATH_LEN, "%s.%s", p_input, BPLOG_EXT);
        if (ret == 0 && S_ISREG (st.st_mode))
        {
            /* bplog exists without extension so rename it */
            ALOGI ("%s:%d file %s exists, renaming....", __FUNCTION__, __LINE__,
                   p_input);
            err = rename (p_input, new_fname);
            if (err < 0 && errno != ENOENT)
                ALOGE ("%s:%d \t error: %s", __FUNCTION__, __LINE__,
                       strerror (errno));
            else
                ALOGD ("%s:%d log file %s renamed to %s", __FUNCTION__, __LINE__,
                       p_input, new_fname);
            rotate = true;
        }
        else
        {
            /* search for new file name, with extension */
            ret = stat (new_fname, &st);
            if (ret == 0 && S_ISREG (st.st_mode))
            {
                LOG_INFO ("%s:%d file %s exists", __FUNCTION__, __LINE__,
                   new_fname);
                rotate = true;
            }
        }

        if (rotate)
        {
            /* a bplog was found so we rotate to start with a fresh one */
            LOG_INFO ("%s:%d Rotating...", __FUNCTION__, __LINE__);
            fs_fd = rotateFile (p_input, fs_fd, *rotate_size, rotate_num);
        }
        else if (ret == -1 && errno == ENOENT)
        {
            char *end_dir = strrchr (p_input, '/');
            if (end_dir != NULL)
            {
                *end_dir = '\0';
                if (stat (p_input, &st) == -1 && errno == ENOENT)
                {
                    *end_dir = '/';
                    /* Creates the missing directories */
                    ALOGI ("%s:%d Path for file %s does not exists",
                           __FUNCTION__, __LINE__, p_input);
                    char path[PATH_LEN];
                    char *p = path;
                    strncpy (path, p_input, PATH_LEN - 1);
                    path[strnlen (p_input, PATH_LEN)] = '\0';
                    /* Skips first / to avoid trying to create nothing */
                    if (p[0] == '/')
                        p++;
                    while ((p = strchr (p, '/')) != NULL)
                    {
                        *p = '\0';
                        ret = stat (path, &st);
                        if (ret != 0 && errno == ENOENT)
                        {
                            ret = mkdir (path, 0755);
                            if (ret != 0)
                            {
                                LOG_ERROR ("%s:%d Can't create path %s : %s",
                                       __FUNCTION__, __LINE__, path,
                                       strerror (errno));
                                goto out_error;
                            }
                            LOG_INFO ("%s:%d Path %s ... created", __FUNCTION__,
                                   __LINE__, path);
                        }
                        *p = '/';
                        p++;
                    }
                }
                *end_dir = '/';
            }
            fs_fd = init_new_file (new_fname, *rotate_size);
        }
        else
        {
            ALOGI ("%s:%d Won't be able to log on %s : %s", __FUNCTION__, __LINE__,
                   p_input, strerror (errno));
            goto out_error;
        }
    }

    if (rotate_num > 0
        && get_names (p_input, *rotate_size, rotate_num) == NULL)
    {
        LOG_ERROR ("%s:%d can't init file list", __FUNCTION__, __LINE__);
        goto out_error;
    }

    if (fs_fd < 0)
    {
        if (new_fname[0] == '\0')
            LOG_ERROR ("%s:%d Couldn't open %s : %s", __FUNCTION__, __LINE__, p_input,
                   strerror (errno));
        else
            LOG_ERROR ("%s:%d Couldn't open %s : %s", __FUNCTION__, __LINE__, new_fname,
                   strerror (errno));
        goto out_error;
    }

    return fs_fd;
out_error:
    return -1;
}

static int
init_socket (char *ip_addr_if, int port_no)
{
    int connect_fd = -1;
    int sock_stream_fd = -1;
    int ret;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    LOG_INFO ("%s:%d Open Stream Socket", __FUNCTION__, __LINE__);
    ATTEMPTE (socket (AF_INET, SOCK_STREAM, 0), <0, "Opening stream socket",
              &sock_stream_fd, out);

    memset (&serv_addr, 0, sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_aton (ip_addr_if, &serv_addr.sin_addr);
    serv_addr.sin_port = htons (port_no);

    LOG_INFO ("%s:%d Bind to port %d", __FUNCTION__, __LINE__, port_no);
    ATTEMPTE (bind (sock_stream_fd, (struct sockaddr *) &serv_addr,
              sizeof (serv_addr)), <0, "Binding to port", &ret, out);

    ALOGI ("%s:%d Listening()...", __FUNCTION__, __LINE__);
    ATTEMPTE (listen (sock_stream_fd, 5), <0, "Listening on sock", &ret, out);

    clilen = sizeof (cli_addr);

    LOG_INFO ("%s:%d Wait for client to connect...", __FUNCTION__, __LINE__);
    ATTEMPTE (accept (sock_stream_fd, (struct sockaddr *) &cli_addr, &clilen),
              <0, "Accepting socket", &connect_fd, out);
    LOG_INFO ("%s:%d Socket %d accepted...", __FUNCTION__, __LINE__, port_no);
out:
    return connect_fd;
}

#ifdef MTS_INTERTHREAD_LOGGING
static const char const *DEST_debug[] = { "TO_READ ", "TO_WRITE" };
static const char const *TOW_debug[] = { "DATA", "REC_ERROR", "UNREC_ERROR" };
static const char const *TOR_debug[] = { "UNLOCK", "STOP" };
static const char const **TOR_array[] = { TOR_debug, TOW_debug };
#endif

static inline void send_msg(int fd, signed char msg)
{
#ifdef MTS_INTERTHREAD_LOGGING
    {
        int dest = (fd == buf_ipc_wtor_fds[1]);
        LOG_DEBUG("%s: %s", DEST_debug[dest], TOR_array[dest][msg]);
    }
#endif
    while (1)
    {
        int ret = write(fd, &msg, sizeof(msg));
        if (ret == sizeof(msg))
        {
            break;
        }
        else if (errno != EINTR)
        {
            LOG_ERROR ("%s:%d Error in write - reason: %s. MTS will exit.",
                   __FUNCTION__, __LINE__, strerror (errno));
            exit(-1);
        }
    }
}

static inline signed char read_msg(int fd)
{
    signed char msg;

    while (1)
    {
        int ret = read(fd, &msg, sizeof(msg));
        if (ret == sizeof(msg))
        {
            break;
        }
        else if (errno != EINTR)
        {
            LOG_ERROR ("%s:%d Error in read - reason: %s. MTS will exit.",
                   __FUNCTION__, __LINE__, strerror (errno));
            exit(-1);
        }
    }

    return msg;
}

static void flush_pipe(int fd)
{
    struct pollfd pfd;
    int ret;

    pfd.fd = fd;
    pfd.events = POLLIN;
    while ((ret = poll(&pfd, 1, 0)))
    {
        if (ret == 1)
        {
            read_msg(fd);
        }
        else if (errno != EINTR)
        {
            LOG_ERROR ("%s:%d Error in poll - reason: %s. MTS will exit.",
                   __FUNCTION__, __LINE__, strerror (errno));
            exit(-1);
        }
    }
}

static void *log_traces_read_thread(void *param)
{
    struct pollfd pfd[2];
    comm_mdm *ctx = param;
#ifdef MTS_DROP_DATA
    unsigned char dummy_buf[MAXDATA_D];
#endif

    pfd[0].fd = ctx->ttyfd;
    pfd[0].events = POLLIN | POLLERR | POLLHUP;
    pfd[1].fd = buf_ipc_wtor_fds[0];
    pfd[1].events = POLLIN;

    buf_ipc_r_locked = 0;

    LOG_DEBUG ("%s:%d read thread started.",
           __FUNCTION__, __LINE__);

    while (1)
    {
        int i;

        int ret = poll(pfd, 2, -1);
        if (ret < 0)
        {
            if (ret == EINTR)
            {
                LOG_DEBUG ("%s:%d Interrupted poll - reason: %s. MTS will continue.",
                       __FUNCTION__, __LINE__, strerror (errno));
                continue;
            }
            else
            {
                LOG_ERROR ("%s:%d Error in poll - reason: %s. MTS will exit.",
                       __FUNCTION__, __LINE__, strerror (errno));
                exit(-1);
            }
        }

        if (pfd[1].revents)
        {
            /* Data on communication fd */
            signed char msg = read_msg(pfd[1].fd);

            if (msg == BUF_WTOR_STOP)
                break;
            else if (msg == BUF_WTOR_UNLOCK)
                pfd[0].fd = ctx->ttyfd;
            else
                LOG_ERROR("%s:%d Received unknown message %d!",
                      __FUNCTION__, __LINE__, msg);
        }
        else if (pfd[0].revents & POLLIN)
        {
            /* Data on log fd */
            unsigned char *buf_r_ptr_copy;
            int size;
            unsigned char *w_ptr;

            w_ptr = buf_w_ptr;

            /* Need to have a lock around copy of read pointer and setting of
             * the lock status of the read thread.
             */
            pthread_mutex_lock(&buf_ipc_lock);
            buf_r_ptr_copy = buf_r_ptr;

            if (buf_r_ptr_copy > buf_w_ptr)
                size = buf_r_ptr_copy - buf_w_ptr;
            else
                size = (buf_r_ptr_copy + rw_buffer_size) - buf_w_ptr;

            /* Need to remove 1 to distinguish between full and empty buffer */
            size -= 1;
            if (size == 0)
            {
#ifdef MTS_DROP_DATA
                /* This is the 'drop at MTS level' mode (where the read thread
                 * never gets blocked but drops data).
                 *
                 * In this case, simply read in a dummy buffer.
                 */
                w_ptr = dummy_buf;
                size = MAXDATA_D;
#else
                /* Need to stop listening on log fd and wait for write thread to
                 * give us space to write in memory.
                 *
                 * Data drop will be done at kernel side.
                 */
                pfd[0].fd = -1;
                buf_ipc_r_locked = 1;
#endif
            }
            pthread_mutex_unlock(&buf_ipc_lock);

            if (buf_ipc_r_locked == 0)
            {
                int n;
                signed char msg = -1;
#ifdef MTS_LOGGING_FRAMEWORK
                struct timespec ts_s, ts_e;
#endif

                size = (size > MAXDATA_R) ? MAXDATA_R : size;
#ifdef MTS_DROP_DATA
                if (w_ptr != dummy_buf)
#endif
                    if ((buf_w_ptr + size) > rw_buffer_end)
                        size = rw_buffer_end - buf_w_ptr;

#ifdef MTS_LOGGING_FRAMEWORK
                clock_gettime(CLOCK_BOOTTIME, &ts_s);
#endif
                n = read(pfd[0].fd, w_ptr, size);
#ifdef MTS_LOGGING_FRAMEWORK
                clock_gettime(CLOCK_BOOTTIME, &ts_e);
                add_syscall_logging(READ,
                                    (((ts_e.tv_sec - ts_s.tv_sec) * 1000000) +
                                     ((ts_e.tv_nsec - ts_s.tv_nsec) / 1000)));
                add_datarate_logging(LOG_DATARATE, n);
#ifdef MTS_DROP_DATA
                if (w_ptr == dummy_buf)
                    add_datarate_logging(LOG_DROPRATE, n);
#endif
#endif

                if (n < 0)
                {
                    msg = BUF_RTOW_IPC_UNRECOVERABLE_ERROR;
                    LOG_ERROR("%s:%d read of modem trace returns %d: error %s. "
                          "Exiting. %p / %p / %p / %08x / %d",
                          __FUNCTION__, __LINE__, n, strerror(errno),
                          w_ptr, buf_r_ptr_copy, rw_buffer, rw_buffer_size, size);
                }
                else if (n == 0)
                {
                    if (ctx->usb_logging)
                        /* In case of non-USB, simply ignore cases where n == 0 */
                        msg = BUF_RTOW_IPC_RECOVERABLE_ERROR;
                }
                else
                {
#ifdef MTS_DROP_DATA
                    if (w_ptr != dummy_buf)
#endif
                    {
                        pthread_mutex_lock(&buf_ipc_lock);
                        buf_w_ptr += n;
                        if (buf_w_ptr == rw_buffer_end)
                            buf_w_ptr = rw_buffer;
                        if (buf_ipc_w_pending == 0)
                        {
                            msg = BUF_RTOW_IPC_DATA;
                            buf_ipc_w_pending = 1;
                        }
                        pthread_mutex_unlock(&buf_ipc_lock);
#ifdef MTS_LOGGING_FRAMEWORK
                        add_bufsize_logging();
#endif
                    }
                }
                if (msg >= 0)
                    send_msg(buf_ipc_rtow_fds[1], msg);
                if ((msg == BUF_RTOW_IPC_RECOVERABLE_ERROR) ||
                    (msg == BUF_RTOW_IPC_UNRECOVERABLE_ERROR))
                    /* Wait for STOP */
                    pfd[0].fd = -1;
            }
        }
        else if (pfd[0].revents & (POLLHUP | POLLERR))
        {
            send_msg(buf_ipc_rtow_fds[1], BUF_RTOW_IPC_RECOVERABLE_ERROR);
            /* Wait for STOP */
            pfd[0].fd = -1;
        }
    }

    flush_pipe(buf_ipc_wtor_fds[0]);

    LOG_DEBUG ("%s:%d read thread stopped.",
           __FUNCTION__, __LINE__);

    return NULL;
}

static void
close_ttyfd(comm_mdm * ctx)
{
    int ret;
    send_msg(buf_ipc_wtor_fds[1], BUF_WTOR_STOP);
    if ((ret = pthread_join(r_thread, NULL)))
    {
        LOG_DEBUG ("%s:%d join returns: %d / %d, exiting to prevent deadlocks",
               __FUNCTION__, __LINE__, ret, errno);
        exit(-1);
    }
    flush_pipe(buf_ipc_rtow_fds[0]);
    close(ctx->ttyfd);
    ctx->ttyfd = -1;

    buf_w_ptr = rw_buffer;
    buf_r_ptr = rw_buffer;
    buf_ipc_w_pending = 0;
    buf_ipc_r_locked = 0;
}

static void
log_traces (int outfd, char *p_input, char *p_output,
            int rotate_size, int rotate_num, comm_mdm * ctx)
{
    off_t cnt_written = 0;
    int ret = 0;
    bool exit_cond = false;
    int i = 0;
    char intercom_msg_buf[MDM_MSG_SZ];
    char inot_event_buf[INOT_BUF_SZ];
    struct pollfd *mtspoll = NULL;

    if ((mtspoll = calloc (POLL_FD_NBR, sizeof (struct pollfd))) == NULL)
    {
        LOG_ERROR ("%s:%d POLL struct calloc failed: %s. Exiting wait loop and MTS.",
               __FUNCTION__, __LINE__, strerror (errno));
        return;
    }

    /* Initialize poll event field */
    for (i = 0; i < POLL_FD_NBR; i++)
        mtspoll[i].events = POLLHUP | POLLIN | POLLERR;

    /* Clean any log fd */
    ctx->ttyfd = -1;

    /* Trace until critical error */
    while (!exit_cond)
    {
        /* Wait modem ready to start traces */
        if (pthread_mutex_lock (&ctx->cond_mtx) != 0)
        {
            LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error taking mutex: %s. Exiting wait loop and MTS.",
                   __FUNCTION__, __LINE__, strerror (errno));
            goto out;
        }

        while (!ctx->modem_available)
        {
            /* Ensure fd is closed - due to previous iteration */
            if (ctx->ttyfd != -1)
                close_ttyfd(ctx);
            if (pthread_cond_wait (&ctx->modem_online, &ctx->cond_mtx) != 0)
            {
                LOG_ERROR ("%s:%d PTHREAD_COND_WAIT Error: %s. Exiting.",
                       __FUNCTION__, __LINE__, strerror (errno));
                if (pthread_mutex_unlock (&ctx->cond_mtx) != 0)
                    LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error releasing mutex: %s.\
                           Exiting wait loop and MTS.",
                           __FUNCTION__, __LINE__, strerror (errno));
                goto out;
            }
        }

        if (pthread_mutex_unlock (&ctx->cond_mtx) != 0)
        {
            LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error releasing mutex: %s.\
                   Exiting wait loop and MTS.",
                   __FUNCTION__, __LINE__, strerror (errno));
            goto out;
        }

        /* So far the modem ready event is received - so open must succeed */
        /* Debug trace */
        if (ctx->ttyfd == -1)
            LOG_DEBUG ("%s:%d Got MODEM_UP, gsmtty to be opened: %s", __FUNCTION__,
                   __LINE__, ctx->p_input);

        if ((ctx->ttyfd == -1)
            && ((ctx->ttyfd = open_gsmtty (ctx->p_input, ctx)) < 0))
        {
            LOG_ERROR ("%s:%d GSMTTY open failure - but modem is notified UP.\
                   MTS needs to wait for USB enumeration.",
                   __FUNCTION__, __LINE__);
            if (!(ctx->usb_logging))
            {
                LOG_ERROR ("%s:%d HSI log port open failure. MTS will exit now.",
                       __FUNCTION__, __LINE__);
                /* For HSI we exit as this is an critical case */
                /* For USB we will get node creation by INOTIFY */
                goto out;
            }
        }

        /* First FD is always the modem log port */
        mtspoll[POLL_FD_LOG].fd = buf_ipc_rtow_fds[0];
        /* 2nd FD pipe read to receive modem event */
        mtspoll[POLL_FD_COM].fd = ctx->intercom[0];
        /* If 3rd FD, this is the USB INOTIFIER */
        if (ctx->usb_logging)
            mtspoll[POLL_FD_NOT].fd = ctx->inotfd;
        else
            mtspoll[POLL_FD_NOT].fd = -1;

        /* Poll FDs */
        ret = poll (mtspoll, POLL_FD_NBR, -1);
        if (ret < 0 && ret != EINVAL)
        {
            LOG_DEBUG ("%s:%d Interrupted poll - reason: %s. MTS will continue.",
                   __FUNCTION__, __LINE__, strerror (errno));
            continue;
        }
        if (ret < 0 && ret == EINVAL)
        {
            LOG_ERROR ("%s:%d Error in poll - reason: %s. MTS will exit.",
                   __FUNCTION__, __LINE__, strerror (errno));
            goto out;
        }

        /* process poll data */
        for (i = 0; i < POLL_FD_NBR; i++)
        {

            if (mtspoll[i].revents & POLLIN)
            {
                int n, n1;
                switch (i)
                {

                /* CASE POLL_FD_LOG */

                case POLL_FD_LOG: {
                    signed char msg = read_msg(buf_ipc_rtow_fds[0]);

                    if (msg == BUF_RTOW_IPC_DATA)
                    {
                        int size;
                        unsigned char *buf_w_ptr_copy;

                        pthread_mutex_lock(&buf_ipc_lock);
                        buf_w_ptr_copy = buf_w_ptr;
                        /* As write pointer has been copied, indicate to read thread that it needs
                         * to resend an event to get it to be processed by write thread.
                         */
                        buf_ipc_w_pending = 0;
                        pthread_mutex_unlock(&buf_ipc_lock);

                        if (buf_w_ptr_copy >= buf_r_ptr)
                            size = buf_w_ptr_copy - buf_r_ptr;
                        else
                            size = (buf_w_ptr_copy + rw_buffer_size) - buf_r_ptr;

                        while (size)
                        {
                            int n1;
                            int lsize = (size > MAXDATA_W) ? MAXDATA_W : size;
#ifdef MTS_LOGGING_FRAMEWORK
                            struct timespec ts_s, ts_e;
#endif
                            if ((buf_r_ptr + lsize) > rw_buffer_end)
                                lsize = rw_buffer_end - buf_r_ptr;

#ifdef MTS_LOGGING_FRAMEWORK
                            clock_gettime(CLOCK_BOOTTIME, &ts_s);
#endif
                            ATTEMPTE (write (outfd, buf_r_ptr, lsize),
                                      <=0
                                      && errno != ENOSPC, "Writing data to output",
                                      &n1, out);
#ifdef MTS_LOGGING_FRAMEWORK
                            clock_gettime(CLOCK_BOOTTIME, &ts_e);
                            add_syscall_logging(WRITE,
                                                (((ts_e.tv_sec - ts_s.tv_sec) * 1000000) +
                                                 ((ts_e.tv_nsec - ts_s.tv_nsec) / 1000)));
#endif
                            if (n1 <= 0)
                            {
                                /* if we end up with no space to write on device, then we have a problem.
                                   we suppose that pre-allocation didn't work (not implemented on FS for
                                   example) mts can't just exit because android will restart the service
                                   and chaos will ensue.
                                   Cleaner solution is to divide rotate_size by 1.5 and
                                   force a file rotation in the hope to free enough space to keep logging.
                                */
                                if (rotate_size > 1024)
                                {
                                    rotate_size /= 1.5;
                                    LOG_DEBUG ("%s:%d Rotate Size adjusted to: %d",
                                           __FUNCTION__, __LINE__, rotate_size);
                                    ATTEMPTE (rotateFile(p_output, outfd, rotate_size,
                                                         rotate_num), <0, "Rotating logs",
                                              &outfd, out);
                                    cnt_written = 0;
                                    continue;
                                }
                                /* if we end up there, there's not much we can do ...
                                   just pause the logging :(
                                */
                                else
                                {
                                    LOG_ERROR ("%s:%d No more space on device!",
                                           __FUNCTION__, __LINE__);
                                    pause ();
                                }
                            }
                            else
                            {
                                signed char msg_to_send = -1;
                                cnt_written += n1;
                                if (rotate_size > 0
                                    && (cnt_written / 1024) >= rotate_size)
                                {
                                    ATTEMPTE (rotateFile(p_output, outfd, rotate_size,
                                                         rotate_num), <0, "Rotating logs",
                                              &outfd, out);
                                    cnt_written = 0;
                                    LOG_INFO ("%s:%d Logs rotated", __FUNCTION__,
                                           __LINE__);
                                }

                                pthread_mutex_lock(&buf_ipc_lock);
                                buf_r_ptr += n1;
                                if (buf_r_ptr == rw_buffer_end)
                                    buf_r_ptr = rw_buffer;
                                /* As read pointer was incremented, unlock reading thread
                                 * if blocked */
                                if (buf_ipc_r_locked)
                                {
                                    buf_ipc_r_locked = 0;
                                    msg_to_send = BUF_WTOR_UNLOCK;
                                }
                                pthread_mutex_unlock(&buf_ipc_lock);

                                if (msg_to_send >= 0)
                                    send_msg(buf_ipc_wtor_fds[1], msg_to_send);
#ifdef MTS_LOGGING_FRAMEWORK
                                add_bufsize_logging();
#endif
                                size -= n1;
                            }
                        }
                    }
                    else if (msg == BUF_RTOW_IPC_UNRECOVERABLE_ERROR)
                    {
                        goto out;
                    }
                    else if (msg == BUF_RTOW_IPC_RECOVERABLE_ERROR)
                    {
                        close_ttyfd(ctx);
                    }
                    else
                    {
                        LOG_ERROR("%s:%d Received unknown message %d!",
                              __FUNCTION__, __LINE__, msg);
                    }
                    break;
                }

                /* CASE POLL_FD_COM */

                case POLL_FD_COM:

                    LOG_DEBUG ("%s:%d Get modem event notification while polling.",
                           __FUNCTION__, __LINE__);
                    memset (intercom_msg_buf, 0,
                            sizeof (intercom_msg_buf));
                    n = n1 = 0;

                    ATTEMPTE (read(mtspoll[POLL_FD_COM].fd, intercom_msg_buf,
                              MDM_MSG_SZ * sizeof (char)), <0,
                              "Reading modem event data", &n, out);

                    if (strncmp(intercom_msg_buf, MDM_UP,
                        MDM_MSG_SZ * sizeof (char)) == 0)
                    {
                        LOG_DEBUG ("%s:%d Get modem event notification - MODEM UP.",
                               __FUNCTION__, __LINE__);
                    }
                    if (strncmp(intercom_msg_buf, MDM_DW,
                        MDM_MSG_SZ * sizeof (char)) == 0)
                    {
                        LOG_DEBUG ("%s:%d Get modem event notification - MODEM DOWN.",
                               __FUNCTION__, __LINE__);
                        /* Close tty */
                        if (ctx->ttyfd != -1)
                        {
                            LOG_DEBUG ("%s:%d Log FD closed.", __FUNCTION__,
                                   __LINE__);
                            close_ttyfd(ctx);
                        }
                    }
                    if (strncmp(intercom_msg_buf, MDM_SW,
                        MDM_MSG_SZ * sizeof (char)) == 0)
                    {
                        LOG_DEBUG ("%s:%d Get modem event notification - MODEM SHUTDOWN.",
                               __FUNCTION__, __LINE__);
                        /* Close tty */
                        if (ctx->ttyfd != -1)
                        {
                            LOG_DEBUG ("%s:%d Log FD closed.", __FUNCTION__,
                                   __LINE__);
                            close_ttyfd(ctx);
                        }
                    }
                    if (strncmp(intercom_msg_buf, MDM_CP,
                        MDM_MSG_SZ * sizeof (char)) == 0)
                    {
                        LOG_DEBUG ("%s:%d Get modem event notification - MODEM COREDUMP.",
                               __FUNCTION__, __LINE__);
                        /* Close tty */
                        if (ctx->ttyfd != -1)
                        {
                            LOG_DEBUG ("%s:%d Log FD closed.", __FUNCTION__,
                                   __LINE__);
                            close_ttyfd(ctx);
                        }
                    }
                    if (strncmp(intercom_msg_buf, MDM_CT,
                        MDM_MSG_SZ * sizeof (char)) == 0)
                    {
                        LOG_DEBUG ("%s:%d Get modem event notification - MODEM COLD RESET.",
                               __FUNCTION__, __LINE__);
                        /* Close tty */
                        if (ctx->ttyfd != -1)
                        {
                            LOG_DEBUG ("%s:%d Log FD closed.", __FUNCTION__,
                                   __LINE__);
                            close_ttyfd(ctx);
                        }
                    }

                    break;

                /* CASE POLL_FD_NOT */

                case POLL_FD_NOT:

                    memset (inot_event_buf, 0, sizeof (inot_event_buf));
                    n = n1 = 0;

                    ATTEMPTE (read(mtspoll[POLL_FD_NOT].fd, inot_event_buf,
                              INOT_BUF_SZ), <0, "Reading inotify data",
                              &n, out);

                    if (n == 0)
                    {
                        LOG_DEBUG ("%s:%d Warning: READ 0 bytes for inotify event.",
                               __FUNCTION__, __LINE__);
                        break;
                    }

                    while (n1 < n)
                    {
                        struct inotify_event *evt =
                            (struct inotify_event *) &inot_event_buf[n1];
                        struct timespec ptim;
                        ptim.tv_sec = 0;
                        ptim.tv_nsec = 50000000L;       /* 50 ms sleep */
                        if (evt->len)
                        {
                            if ((evt->mask & IN_CREATE)
                                && (!(evt->mask & IN_ISDIR)))
                            {
                                int attempt = 5;
                                bool try = true;
                                LOG_DEBUG ("%s:%d USB node %s CREATED.",
                                       __FUNCTION__, __LINE__,
                                       evt->name);
                                /* Give time for ttyACM1 node creation */
                                nanosleep (&ptim, NULL);
                                if (ctx->ttyfd == -1)
                                {
                                    LOG_DEBUG ("%s:%d MTS will loop until we connect on USB.",
                                           __FUNCTION__, __LINE__);
                                    while ((ctx->ttyfd =
                                                open_gsmtty (ctx->
                                                             p_input, ctx)) < 0 && try)
                                    {
                                        if (!attempt)
                                        {
                                            LOG_ERROR ("%s:%d Too many open tentative. \
                                                   MTS will give up until next USB event.",
                                                   __FUNCTION__, __LINE__);
                                            n1 += INOT_EVENT_SZ + evt->len;
                                            try = false;
                                        }
                                        nanosleep (&ptim, NULL);
                                        attempt--;
                                    }
                                }
                                n1 += INOT_EVENT_SZ + evt->len;
                            }
                            if ((evt->mask & IN_DELETE)
                                && (!(evt->mask & IN_ISDIR)))
                            {
                                LOG_DEBUG ("%s:%d USB node %s DELETED.",
                                       __FUNCTION__, __LINE__,
                                       evt->name);
                                if (ctx->ttyfd != -1)
                                {
                                    LOG_DEBUG ("%s:%d USB node FD closed.",
                                           __FUNCTION__, __LINE__);
                                    close_ttyfd(ctx);
                                }
                                n1 += INOT_EVENT_SZ + evt->len;
                            }
                        }
                    }
                    break;
                }
            }
            if (mtspoll[i].revents & POLLERR)
            {
                switch (i)
                {
                case POLL_FD_LOG:
                    LOG_ERROR ("%s:%d Error: POLLERR event captured on communication fd.",
                           __FUNCTION__, __LINE__);
                    exit(-1);
                    break;
                case POLL_FD_COM:
                case POLL_FD_NOT:
                    LOG_ERROR ("%s:%d Error: POLLERR event captured on fd. Exiting MTS.",
                           __FUNCTION__, __LINE__);
                    goto out;
                    break;
                }
            }
            if (mtspoll[i].revents & POLLHUP)
            {
                switch (i)
                {
                case POLL_FD_LOG:
                    LOG_ERROR ("%s:%d Error: POLLHUP event captured on communication fd.",
                           __FUNCTION__, __LINE__);
                    exit(-1);
                    break;
                case POLL_FD_COM:
                case POLL_FD_NOT:
                    LOG_DEBUG ("%s:%d Warning: POLLERR event captured on fd. Exiting MTS.",
                           __FUNCTION__, __LINE__);
                    goto out;
                    break;
                }
            }
        }

        /* Clean up revents fields - everything is processed */
        for (i = 0; i < POLL_FD_NBR; i++)
            mtspoll[i].revents = 0;

    }

out:
    if (ctx->ttyfd != -1)
    {
        close_ttyfd(ctx);
    }
    close (outfd);
    if (mtspoll != NULL)
        free (mtspoll);
    LOG_DEBUG ("%s:%d Closed input: %s and output: %s", __FUNCTION__, __LINE__,
           p_input, p_output);
    return;
}

int
mdm_up (mmgr_cli_event_t * ev)
{
    comm_mdm *ctx = (comm_mdm *) ev->context;
    LOG_DEBUG ("%s:%d Received MODEM_UP", __FUNCTION__, __LINE__);

    if (pthread_mutex_lock (&ctx->cond_mtx) != 0)
    {
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error taking mutex: %s. CRITICAL: PTHREAD_COND not sent!",
               __FUNCTION__, __LINE__, strerror (errno));
        goto out;
    }
    /* Modem up - start tracing */
    ctx->modem_available = true;
    if (write (ctx->intercom[1], MDM_UP, MDM_MSG_SZ * sizeof (char))
        != (MDM_MSG_SZ * sizeof (char)))
        LOG_ERROR ("%s:%d Modem event msg not properly sent. MTS may missbehave.",
               __FUNCTION__, __LINE__);

    if (pthread_mutex_unlock (&ctx->cond_mtx) != 0)
    {
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error releasing mutex: %s.\
               CRITICAL: MTS not expected to work anymore. PTHREAD_COND not sent!",
               __FUNCTION__, __LINE__, strerror (errno));
        goto out;
    }
    /* Send signal to start tracing */
    if (pthread_cond_signal (&ctx->modem_online) != 0)
        LOG_ERROR ("%s:%d PTHREAD_COND_SIGNAL Error: %s.\
               CRITICAL: MTS not expected to work anymore. PTHREAD_COND not sent!",
               __FUNCTION__, __LINE__, strerror (errno));
out:
    return 0;
}

int
mdm_dwn (mmgr_cli_event_t * ev)
{
    comm_mdm *ctx = (comm_mdm *) ev->context;
    LOG_DEBUG ("%s:%d Received MODEM_DOWN", __FUNCTION__, __LINE__);

    if (pthread_mutex_lock (&ctx->cond_mtx) != 0)
    {
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error taking mutex: %s. \
               CRITICAL: MTS not expected to work anymore. Trouble on mdm_down notification !",
               __FUNCTION__, __LINE__, strerror (errno));
        goto out;
    }
    /* Modem down - stop tracing */
    ctx->modem_available = false;
    if (write (ctx->intercom[1], MDM_DW, MDM_MSG_SZ * sizeof (char))
        != (MDM_MSG_SZ * sizeof (char)))
        LOG_ERROR ("%s:%d Modem event msg not properly sent. MTS may missbehave.",
               __FUNCTION__, __LINE__);

    if (pthread_mutex_unlock (&ctx->cond_mtx) != 0)
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error releasing mutex: %s. \
               CRITICAL: MTS not expected to work anymore. Trouble on mdm_down notification t!",
               __FUNCTION__, __LINE__, strerror (errno));

out:
    return 0;
}

int
mdm_cdump (mmgr_cli_event_t * ev)
{
    comm_mdm *ctx = (comm_mdm *) ev->context;
    LOG_DEBUG ("%s:%d Received MODEM_CORE_DUMP", __FUNCTION__, __LINE__);

    if (pthread_mutex_lock (&ctx->cond_mtx) != 0)
    {
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error taking mutex: %s. \
               CRITICAL: MTS not expected to work anymore. Trouble on mdm_cdump notification !",
               __FUNCTION__, __LINE__, strerror (errno));
        goto out;
    }
    /* Modem performing core dump - stop tracing */
    ctx->modem_available = false;
    if (write (ctx->intercom[1], MDM_CP, MDM_MSG_SZ * sizeof (char))
        != (MDM_MSG_SZ * sizeof (char)))
        LOG_ERROR ("%s:%d Modem event msg not properly sent. MTS may missbehave.",
               __FUNCTION__, __LINE__);

    if (pthread_mutex_unlock (&ctx->cond_mtx) != 0)
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error releasing mutex: %s. \
               CRITICAL: MTS not expected to work anymore. Trouble on mdm_cdump notification t!",
               __FUNCTION__, __LINE__, strerror (errno));

out:
    return 0;
}

int
mdm_shtdwn (mmgr_cli_event_t * ev)
{
    comm_mdm *ctx = (comm_mdm *) ev->context;
    LOG_DEBUG ("%s:%d Received MODEM_SHUTDOWN", __FUNCTION__, __LINE__);

    if (pthread_mutex_lock (&ctx->cond_mtx) != 0)
    {
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error taking mutex: %s. \
               CRITICAL: MTS not expected to work anymore. Trouble on mdm_shtdwn notification !",
               __FUNCTION__, __LINE__, strerror (errno));
        goto out;
    }
    /* Modem shutting down - stop tracing */
    ctx->modem_available = false;
    if (write (ctx->intercom[1], MDM_SW, MDM_MSG_SZ * sizeof (char))
        != (MDM_MSG_SZ * sizeof (char)))
        LOG_ERROR ("%s:%d Modem event msg not properly sent. MTS may missbehave.",
               __FUNCTION__, __LINE__);

    if (pthread_mutex_unlock (&ctx->cond_mtx) != 0)
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error releasing mutex: %s. \
               CRITICAL: MTS not expected to work anymore. Trouble on mdm_shtdwn notification t!",
               __FUNCTION__, __LINE__, strerror (errno));

out:
    return 0;
}

int
mdm_crst (mmgr_cli_event_t * ev)
{
    comm_mdm *ctx = (comm_mdm *) ev->context;
    LOG_DEBUG ("%s:%d Received MODEM_COLD_RESET", __FUNCTION__, __LINE__);

    if (pthread_mutex_lock (&ctx->cond_mtx) != 0)
    {
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error taking mutex: %s. \
               CRITICAL: MTS not expected to work anymore. Trouble on mdm_crst notification !",
               __FUNCTION__, __LINE__, strerror (errno));
        goto out;
    }
    /* Modem performing cold reset - stop tracing */
    ctx->modem_available = false;
    if (write (ctx->intercom[1], MDM_CT, MDM_MSG_SZ * sizeof (char))
        != (MDM_MSG_SZ * sizeof (char)))
        LOG_ERROR ("%s:%d Modem event msg not properly sent. MTS may missbehave.",
               __FUNCTION__, __LINE__);

    if (pthread_mutex_unlock (&ctx->cond_mtx) != 0)
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX Error releasing mutex: %s. \
               CRITICAL: MTS not expected to work anymore. Trouble on mdm_crst notification t!",
               __FUNCTION__, __LINE__, strerror (errno));

out:
    return 0;
}

int
main (int argc, char **argv)
{
    char ip_addr_if[16] = "";
    int ret = 0;
    struct comm_mdm_s ctx;
    int outfd = -1;

    char p_output[PATH_LEN] = { 0 };
    char p_out_type = '\0';
    char p_input[PATH_LEN] = { 0 };
    char p_interface[PATH_LEN] = { 0 };
    char module_name[16] = { '\0' };
    int p_rotate_size = DEFAULT_LOG_SIZE;
    int p_rotate_num = DEFAULT_MAX_ROTATED_NR;
    int max_argc_properties = 1;
    int inst_id = 1;
    unsigned int i, j;
#ifdef MTS_LOGGING_FRAMEWORK
    pthread_t l_thread;

    struct timespec ts;
    clock_gettime(CLOCK_BOOTTIME, &ts);
    start_log_time = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);

    pthread_create(&l_thread, NULL, logging_thread, NULL);
#endif

#ifdef MTS_LOGGING_FRAMEWORK
    add_datarate_logging(LOG_DATARATE, 0);
    add_datarate_logging(LOG_DROPRATE, 0);
    add_bufsize_logging();
    add_syscall_logging(READ, 0);
    add_syscall_logging(WRITE, 0);
#endif

    ctx.ttyfd = -1;
    ctx.inotfd = -1;
    ctx.inotwd = -1;
    ctx.modem_available = false;
    ctx.usb_logging = false;
    const struct args_s list[] =
            {{ .name = "output",.key = "-o",.type_conv = "%s",
               .storage = p_output},
             { .name = "output_type",.key = "-t",.type_conv = "%c",
               .storage = &p_out_type},
             { .name = "input",.key = "-i",.type_conv = "%s",
               .storage = p_input},
             { .name = "interface",.key = "-s",.type_conv = "%s",
               .storage = p_interface},
             { .name = "rotate_size",.key = "-r",.type_conv = "%d",
               .storage = &p_rotate_size},
             { .name = "rotate_num",.key = "-n",.type_conv = "%d",
               .storage = &p_rotate_num},
             { .name = "buffer_size",.key = "-b",.type_conv = "%d",
               .storage = &rw_buffer_size }};
    char key_array[sizeof (list) / sizeof (*list)] = { 0 };

    //add -m for mmgr instance id handling(e.g. service mtsp /system/bin/mts -m 1)
    if (argc > 1)
    {
        for (i = 1; i < (unsigned) argc; i++) {
            if (0 == strcmp(argv[i], "-m") && (argc - i > 1)) {
                inst_id = strtol(argv[i + 1], NULL, 0);
                max_argc_properties = 3;
                break;
            }
        }
    }

    if (argc > max_argc_properties)
    {
        for (i = 1; i < (unsigned) argc; i++)
        {
            for (j = 0; j < sizeof (list) / sizeof (*list); j++)
            {
                if (key_array[j] == 1)
                    continue;
                if (strncmp (list[j].key, argv[i], 2) == 0)
                {
                    sscanf (argv[++i], list[j].type_conv, list[j].storage);
                    key_array[j] = 1;
                    break;
                }
            }
        }
    }
    else
    {
        char result[PROPERTY_VALUE_MAX];
        for (i = 0; i < sizeof (list) / sizeof (*list); i++)
        {
            char property[PROPERTY_KEY_MAX] = {'\0'};
            if (inst_id == 1)
                snprintf(property, sizeof(property), "%s.%s", PROP_HEAD, list[i].name);
            else
                snprintf(property, sizeof(property), "%s%d.%s", PROP_HEAD_NON_DEFAULT,
                        inst_id, list[i].name);
            property_get (property, result, "");
            if (result[0] == '\0')
                continue;
            sscanf (result, list[i].type_conv, list[i].storage);
        }
    }

    //initialize log tag as soon as possible
    logs_init(inst_id);

    LOG_INFO ("%s Version:%s%s", argv[0], __DATE__, __TIME__);

    LOG_DEBUG ("%s:%d Parameters: out_type: %c output: %s input: %s interface: %s "
           "rotate_num: %d rotate_size: %d buffer_size: %d M inst_id: %d",
           __FUNCTION__, __LINE__, p_out_type, p_output, p_input, p_interface,
           p_rotate_num, p_rotate_size, rw_buffer_size, inst_id);


    if (pthread_cond_init (&ctx.modem_online, NULL) != 0)
    {
        LOG_ERROR ("%s:%d modem_online PTHREAD_COND cannot be initialized: error %s. Exiting.",
               __FUNCTION__, __LINE__, strerror (errno));
        goto cond_failure;
    }

    if (pthread_mutex_init (&ctx.cond_mtx, NULL) != 0)
    {
        LOG_ERROR ("%s:%d PTHREAD_COND_MUTEX cannot be initialized: error %s. Exiting.",
               __FUNCTION__, __LINE__, strerror (errno));
        goto mtx_failure;
    }

    if (pipe (ctx.intercom) == -1)
    {
        LOG_ERROR ("%s:%d PIPE for intercom cannot be initialized: error %s. Exiting.",
               __FUNCTION__, __LINE__, strerror (errno));
        goto init_failure;
    }

    if (!p_input[0] || !p_output[0])
    {
        puts (USAGE_TXT);
        LOG_ERROR ("Wrong MTS parameters");
        goto init_failure;
    }

    /* Initialize buffer for inter-thread communication */
    rw_buffer_size *= 1024 * 1024; /* Convert buffer size in bytes */

    rw_buffer = malloc(rw_buffer_size);
    if (!rw_buffer) return -1;
    buf_w_ptr = rw_buffer;
    buf_r_ptr = rw_buffer;
    rw_buffer_end = rw_buffer + rw_buffer_size;

    pthread_mutex_init(&buf_ipc_lock, NULL);
    buf_ipc_w_pending = 0;
    if (pipe(buf_ipc_rtow_fds) < 0) return -1;
    if (pipe(buf_ipc_wtor_fds) < 0) return -1;
    buf_ipc_r_locked = 0;

    if (strncmp (p_input, USB_LOG_DEV, sizeof (USB_LOG_DEV)) == 0)
    {
        LOG_DEBUG ("%s:%d: USB logging enabled - activate INOTIFY watch.",
               __FUNCTION__, __LINE__);
        ctx.usb_logging = true;
        /* Create INOTIFY instance */
        if ((ctx.inotfd = inotify_init ()) < 0)
        {
            LOG_ERROR ("%s:%d  INOTIFY cannot be initialized: error %s. Exiting.",
                   __FUNCTION__, __LINE__, strerror (errno));
            goto init_failure;
        }

        /* Add watch for USB device */
        if ((ctx.inotwd =
                 inotify_add_watch (ctx.inotfd, INOT_USB_DEV_NODE,
                                    IN_CREATE | IN_DELETE)) < 0)
        {
            LOG_ERROR ("%s:%d  INOTIFY WATCH cannot be initialized: error %s. Exiting.",
                   __FUNCTION__, __LINE__, strerror (errno));
            goto inotwatch_failure;
        }
    }

    ctx.mmgr_hdl = NULL;

    snprintf(module_name, sizeof(module_name), "mts%d", inst_id);

    mmgr_cli_create_handle (&ctx.mmgr_hdl, module_name, &ctx);
    mmgr_cli_set_instance(ctx.mmgr_hdl, inst_id);
    mmgr_cli_subscribe_event (ctx.mmgr_hdl, mdm_up, E_MMGR_EVENT_MODEM_UP);
    mmgr_cli_subscribe_event (ctx.mmgr_hdl, mdm_dwn, E_MMGR_EVENT_MODEM_DOWN);
    mmgr_cli_subscribe_event (ctx.mmgr_hdl, mdm_cdump, E_MMGR_NOTIFY_CORE_DUMP);
    mmgr_cli_subscribe_event (ctx.mmgr_hdl, mdm_shtdwn, E_MMGR_NOTIFY_MODEM_SHUTDOWN);
    mmgr_cli_subscribe_event (ctx.mmgr_hdl, mdm_crst, E_MMGR_NOTIFY_MODEM_COLD_RESET);
    ctx.p_input = p_input;

    uint32_t iMaxTryConnect = MAX_WAIT_MMGR_CONNECT_SECONDS * 1000 / MMGR_CONNECT_RETRY_TIME_MS;

    while (iMaxTryConnect-- != 0) {

        /* Try to connect */
        ret = mmgr_cli_connect (ctx.mmgr_hdl);

        if (ret == E_ERR_CLI_SUCCEED) {

            break;
        }

        LOG_ERROR("Delaying mmgr_cli_connect %d", ret);

        /* Wait */
        usleep(MMGR_CONNECT_RETRY_TIME_MS * 1000);
    }
    /* Check for unsuccessfull connection */
    if (ret != E_ERR_CLI_SUCCEED) {

        LOG_ERROR("Failed to connect to mmgr %d", ret);

    }

    switch (p_out_type)
    {
    case 'f':
        /* disables rotation if one or the other param is <=0 */
        if (p_rotate_num <= 0 || p_rotate_size <= 0)
            p_rotate_size = p_rotate_num = 0;
        outfd = init_file (p_output, &p_rotate_size, p_rotate_num);
        log_traces (outfd, p_input, p_output, p_rotate_size, p_rotate_num,
                    &ctx);
        break;
    case 'p':
        while (!iface_up (ip_addr_if, p_interface))
        {
            sleep (CHECK_INTERVAL);
            LOG_INFO ("%s:%d wait iface_up(%s)", __FUNCTION__, __LINE__, p_interface);
        }
        LOG_INFO ("%s:%d %s is up, do init_socket()", __FUNCTION__, __LINE__,
               p_interface);
        outfd = init_socket (ip_addr_if, atoi (p_output));
        log_traces (outfd, p_input, NULL, 0, 0, &ctx);
        break;
    case 'k':
        /* log_to_pti will sets pti sink/sources and block until
           a signal MY_SIG_UNCONF_PTI is received
           then it will clean and close all fds
         */
        log_to_pti (p_input, p_output, &ctx);
        break;
    default:
        puts (USAGE_TXT);
    }

    mmgr_cli_disconnect (ctx.mmgr_hdl);
    mmgr_cli_delete_handle (ctx.mmgr_hdl);
    if (ctx.usb_logging)
        inotify_rm_watch (ctx.inotfd, ctx.inotwd);
inotwatch_failure:
    if (ctx.usb_logging)
        close (ctx.inotfd);
    close (ctx.intercom[0]);
    close (ctx.intercom[1]);
init_failure:
    pthread_mutex_destroy (&ctx.cond_mtx);
mtx_failure:
    pthread_cond_destroy (&ctx.modem_online);
cond_failure:
    LOG_INFO ("%s:%d Exiting mts", __FUNCTION__, __LINE__);
    return 0;
}
