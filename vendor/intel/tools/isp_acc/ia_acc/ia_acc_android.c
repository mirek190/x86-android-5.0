#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <linux/atomisp.h>

#include "ia_acc.h"

#define ISP_DEV "/dev/video0"

static int fd = -1;

static int
get_file_size(FILE *file)
{
    int len;

    if (file == NULL) return 0;

    if (fseek(file, 0, SEEK_END)) return 0;

    len = ftell(file);

    if (fseek(file, 0, SEEK_SET)) return 0;

    return len;
}

#define xioctl(req, arg) _xioctl(req, arg, #req)

static int
_xioctl(int request, void *arg, const char *req_name)
{
    int ret;

    if (fd == -1) {
        printf("ia_acc: device not opened\n");
        return -1;
    }

    printf("xioctl: request = %s\n", req_name);
    do {
    	if (request == ATOMISP_IOC_ACC_START)
		printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
        ret = ioctl (fd, request, arg);
    } while (-1 == ret && EINTR == errno);

    if (ret < 0)
        printf("%s: Request %s failed: %s\n", __FUNCTION__, req_name, strerror(errno));

    return ret;
}

int
ia_acc_open(const char *csim_target)
{
    (void)csim_target;
    if (fd == -1) {
        fd = open(ISP_DEV, O_RDWR);
        if (fd <= 0) {
            printf("Error opening ISP device (%s)\n", strerror(errno));
            fd = -1;
            return -1;
        }
    }
    printf ("Opened ISP Device %s\n", ISP_DEV);
    return 0;
}

void
ia_acc_close(void)
{
    if (fd != -1) {
        close(fd);
        fd = -1;
	printf ("Closed ISP Device %s\n", ISP_DEV);
    }
}

void *
ia_acc_open_firmware(const char *fw_path, unsigned *size)
{
    FILE *file;
    unsigned len;
    void *fw;

    if (!fw_path)
        return NULL;

    file = fopen(fw_path, "rb");
    if (!file)
        return NULL;

    len = get_file_size(file);

    if (!len) {
        fclose(file);
        return NULL;
    }

    fw = malloc(len);
    if (!fw) {
        fclose(file);
        return NULL;
    }

    if (fread(fw, 1, len, file) != len) {
        fclose(file);
        free(fw);
        return NULL;
    }

    *size = len;

    fclose(file);

    return fw;
}

int
ia_acc_load_firmware(void *fw, unsigned size, unsigned *handle)
{
    int ret;

    struct atomisp_acc_fw_load fwData;
    fwData.size      = size;
    fwData.fw_handle = 0;
    fwData.data      = fw;

    ret = xioctl(ATOMISP_IOC_ACC_LOAD, &fwData);

    //If IOCTRL call was returned successfully, get the firmware handle
    //from the structure and return it to the application.
    if (!ret)
        *handle = fwData.fw_handle;

    printf("%s: Loaded the ACC firmware handle = %p\n", __func__, (void *)*handle);
    return ret;
}

int
ia_acc_unload_firmware(unsigned handle)
{
    return xioctl(ATOMISP_IOC_ACC_UNLOAD, &handle);
}

int
ia_acc_map(void *user_ptr, size_t size, uint32_t *css_ptr)
{
    int ret;
    struct atomisp_acc_map map;

    printf ("%s: Called for user_ptr %p\n", __func__, user_ptr);
    memset(&map, 0, sizeof(map));

    map.length = size;
    map.user_ptr = user_ptr;

    ret = xioctl(ATOMISP_IOC_ACC_MAP, &map);

    *css_ptr = map.css_ptr;
    if (ret == 0)
      printf ("%s: user_ptr %p mapped to %p\n", __func__, user_ptr, (void *)css_ptr);
    return ret;
}

int
ia_acc_unmap(uint32_t css_ptr, size_t size)
{
    struct atomisp_acc_map map;

    memset(&map, 0, sizeof(map));

    map.css_ptr = css_ptr;
    map.length = size;

    return xioctl(ATOMISP_IOC_ACC_UNMAP, &map);
}

int
ia_acc_set_mapped_arg(unsigned handle, unsigned mem, uint32_t css_ptr, size_t size)
{
    struct atomisp_acc_s_mapped_arg arg;

    memset(&arg, 0, sizeof(arg));

    arg.fw_handle = handle;
    arg.memory = mem;
    arg.css_ptr = css_ptr;
    arg.length = size;

    return xioctl(ATOMISP_IOC_ACC_S_MAPPED_ARG, &arg);
}

int
ia_acc_start_firmware(unsigned handle)
{
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    return xioctl(ATOMISP_IOC_ACC_START, &handle);
}

int
ia_acc_wait_for_firmware(unsigned handle)
{
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    return xioctl(ATOMISP_IOC_ACC_WAIT, &handle);
}

void*
ia_acc_alloc(size_t size)
{
#if defined(_WIN32)
    return _aligned_malloc(size, 4096); // FIXME: properly get the page size
#else
    void *ptr;
    posix_memalign(&ptr, sysconf(_SC_PAGESIZE), size);
    return ptr;
#endif
}

void
ia_acc_free(void *ptr)
{
    if (ptr) {
#if defined(_WIN32)
        _aligned_free(ptr);
//#elif defined(ANDROID)
//        _aligned_free(ptr, sysconf(_SC_PAGESIZE));
#else
        free(ptr);
#endif
    }
}


uint32_t
ia_acc_css_alloc(size_t size)
{
    (void)size;
    printf("use alloc+map instead\n");
    return 0;
}

void
ia_acc_css_free(uint32_t css_ptr)
{
    (void)css_ptr;
    printf("use unmap+fre instead\n");
}

ia_acc_buf*
ia_acc_buf_create(void *cpu_ptr, size_t size)
{
    ia_acc_buf *me = malloc(sizeof(*me));
    if (!me)
        return me;
    me->size = size;
    me->cpu_ptr = cpu_ptr;
    me->own_cpu_ptr = 0;
    ia_acc_map(me->cpu_ptr, size, &me->css_ptr);
    return me;
}

ia_acc_buf*
ia_acc_buf_alloc(size_t size)
{
    ia_acc_buf *me = malloc(sizeof(*me));
    if (!me)
        return me;
    me->size = size;
    me->cpu_ptr = ia_acc_alloc(size);
    me->own_cpu_ptr = 0;
    ia_acc_map(me->cpu_ptr, size, &me->css_ptr);
    return me;
}

void
ia_acc_buf_free(ia_acc_buf *buf)
{
    if (buf) {
        ia_acc_unmap(buf->css_ptr, buf->size);
        if (buf->own_cpu_ptr)
            ia_acc_free(buf->cpu_ptr);
        free(buf);
    }
}

void
ia_acc_buf_sync_to_css(ia_acc_buf *buf)
{
    /* Android uses uncached pages, nothing to do */
    (void)buf;
}

void
ia_acc_buf_sync_to_cpu(ia_acc_buf *buf)
{
    /* Android uses uncached pages, nothing to do */
    (void)buf;
}
