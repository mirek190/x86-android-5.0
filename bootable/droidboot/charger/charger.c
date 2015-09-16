/*
 * Copyright (C) 2011 The Android Open Source Project
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

//#define DEBUG_UEVENTS

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <cutils/android_reboot.h>
#include <inttypes.h>
#include <cutils/klog.h>
#include <cutils/list.h>
#include <cutils/misc.h>
#include <cutils/uevent.h>
#include <hardware_legacy/power.h>

#include "minui/minui.h"
#include "charger.h"

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#define MSEC_PER_SEC            (1000LL)
#define NSEC_PER_MSEC           (1000000LL)

#define LAST_KMSG_PATH          "/proc/last_kmsg"
#define LAST_KMSG_MAX_SZ        (32 * 1024)

#define LOGE(x...) do { KLOG_ERROR("charger", x); } while (0)
#define LOGI(x...) do { KLOG_INFO("charger", x); } while (0)
#define LOGV(x...) do { KLOG_DEBUG("charger", x); } while (0)

#define SPID_FMLY_FILE_NAME	"/sys/spid/platform_family_id"
#define MOOR_PF_ID		"0008"

static struct charger charger_state = {0};

static pthread_t charger_thread;

/* current time in milliseconds */
static int64_t curr_time_ms(void)
{
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return tm.tv_sec * MSEC_PER_SEC + (tm.tv_nsec / NSEC_PER_MSEC);
}

#define MAX_KLOG_WRITE_BUF_SZ 256

static void dump_last_kmsg(void)
{
    char *buf;
    char *ptr;
    unsigned sz = 0;
    int len;

    LOGI("\n");
    LOGI("*************** LAST KMSG ***************\n");
    LOGI("\n");
    buf = load_file(LAST_KMSG_PATH, &sz);
    if (!buf || !sz) {
        LOGI("last_kmsg not found. Cold reset?\n");
        goto out;
    }

    len = min(sz, LAST_KMSG_MAX_SZ);
    ptr = buf + (sz - len);

    while (len > 0) {
        int cnt = min(len, MAX_KLOG_WRITE_BUF_SZ);
        char yoink;
        char *nl;

        nl = memrchr(ptr, '\n', cnt - 1);
        if (nl)
            cnt = nl - ptr + 1;

        yoink = ptr[cnt];
        ptr[cnt] = '\0';
        klog_write(6, "<6>%s", ptr);
        ptr[cnt] = yoink;

        len -= cnt;
        ptr += cnt;
    }

    free(buf);

out:
    LOGI("\n");
    LOGI("************* END LAST KMSG *************\n");
    LOGI("\n");
}

static int read_file(const char *path, char *buf, size_t sz)
{
    int fd;
    size_t cnt;

    fd = open(path, O_RDONLY, 0);
    if (fd < 0)
        goto err;

    cnt = read(fd, buf, sz - 1);
    if (cnt <= 0)
        goto err;
    buf[cnt] = '\0';
    if (buf[cnt - 1] == '\n') {
        cnt--;
        buf[cnt] = '\0';
    }

    close(fd);
    return cnt;

err:
    if (fd >= 0)
        close(fd);
    return -1;
}

static int read_file_int(const char *path, int *val)
{
    char buf[32];
    int ret;
    int tmp;
    char *end;

    ret = read_file(path, buf, sizeof(buf));
    if (ret < 0)
        return -1;

    tmp = strtol(buf, &end, 0);
    if (end == buf ||
        ((end < buf+sizeof(buf)) && (*end != '\n' && *end != '\0')))
        goto err;

    *val = tmp;
    return 0;

err:
    return -1;
}

int get_battery_capacity(struct charger *charger)
{
    int ret;
    int batt_cap = -1;

    if (!charger->battery)
        return -1;

    ret = read_file_int(charger->battery->cap_path, &batt_cap);
    if (ret < 0 || batt_cap > 100)
        return -1;

    return batt_cap;
}

/* This function allows to get the battery level from droidboot */
int get_battery_level(void)
{
    struct charger *charger = &charger_state;

    // returns -1 in case of error
    return get_battery_capacity(charger);
}

static struct power_supply *find_supply(struct charger *charger,
                                        const char *name)
{
    struct listnode *node;
    struct power_supply *supply;

    list_for_each(node, &charger->supplies) {
        supply = node_to_item(node, struct power_supply, list);
        if (!strncmp(name, supply->name, sizeof(supply->name)))
            return supply;
    }
    return NULL;
}

static struct power_supply *add_supply(struct charger *charger,
                                       const char *name, const char *type,
                                       const char *path, bool online)
{
    struct power_supply *supply;

    supply = calloc(1, sizeof(struct power_supply));
    if (!supply)
        return NULL;

    strlcpy(supply->name, name, sizeof(supply->name));
    strlcpy(supply->type, type, sizeof(supply->type));
    snprintf(supply->cap_path, sizeof(supply->cap_path),
             "/sys/%s/capacity", path);
    supply->online = online;
    list_add_tail(&charger->supplies, &supply->list);
    charger->num_supplies++;
    LOGV("... added %s %s %d\n", supply->name, supply->type, online);
    return supply;
}

static void remove_supply(struct charger *charger, struct power_supply *supply)
{
    if (!supply)
        return;
    list_remove(&supply->list);
    charger->num_supplies--;
    free(supply);
}

static void parse_uevent(const char *msg, struct uevent *uevent)
{
    uevent->action = "";
    uevent->path = "";
    uevent->subsystem = "";
    uevent->ps_name = "";
    uevent->ps_online = "";
    uevent->ps_type = "";

    /* currently ignoring SEQNUM */
    while (*msg) {
#ifdef DEBUG_UEVENTS
        LOGV("uevent str: %s\n", msg);
#endif
        if (!strncmp(msg, "ACTION=", 7)) {
            msg += 7;
            uevent->action = msg;
        } else if (!strncmp(msg, "DEVPATH=", 8)) {
            msg += 8;
            uevent->path = msg;
        } else if (!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            uevent->subsystem = msg;
        } else if (!strncmp(msg, "POWER_SUPPLY_NAME=", 18)) {
            msg += 18;
            uevent->ps_name = msg;
        } else if (!strncmp(msg, "POWER_SUPPLY_ONLINE=", 20)) {
            msg += 20;
            uevent->ps_online = msg;
        } else if (!strncmp(msg, "POWER_SUPPLY_TYPE=", 18)) {
            msg += 18;
            uevent->ps_type = msg;
        }

        /* advance to after the next \0 */
        while (*msg++)
            ;
    }

    LOGV("event { '%s', '%s', '%s', '%s', '%s', '%s' }\n",
         uevent->action, uevent->path, uevent->subsystem,
         uevent->ps_name, uevent->ps_type, uevent->ps_online);
}

static int process_ps_uevent(struct charger *charger, struct uevent *uevent)
{
    int online;
    char ps_type[32];
    struct power_supply *supply = NULL;
    bool was_online = false;
    bool battery = false;
    int old_supplies_online = charger->num_supplies_online;
    int ret = -1;

    if (uevent->ps_type[0] == '\0') {
        char *path;

        if (uevent->path[0] == '\0')
            goto error;
        ret = asprintf(&path, "/sys/%s/type", uevent->path);
        if (ret <= 0) {
            ret = -1;
            goto error;
        }
        ret = read_file(path, ps_type, sizeof(ps_type));
        free(path);
        if (ret < 0) {
            LOGE("Failed to read /sys/%s/type\n", uevent->path);
            goto error;
        }
    } else {
        strlcpy(ps_type, uevent->ps_type, sizeof(ps_type));
    }

    if (!strncmp(ps_type, "Battery", 7))
        battery = true;

    online = atoi(uevent->ps_online);
    supply = find_supply(charger, uevent->ps_name);
    if (supply) {
        was_online = supply->online;
        supply->online = online;
    }

    if (!strcmp(uevent->action, "add")) {
        if (!supply) {
            supply = add_supply(charger, uevent->ps_name, ps_type, uevent->path,
                                online);
            if (!supply) {
                LOGE("cannot add supply '%s' (%s %d)\n", uevent->ps_name,
                     uevent->ps_type, online);
                ret = -1;
                goto error;
            }
            /* only pick up the first battery for now */
            if (battery && !charger->battery)
                charger->battery = supply;
        } else {
            LOGE("supply '%s' already exists..\n", uevent->ps_name);
        }
    } else if (!strcmp(uevent->action, "remove")) {
        if (supply) {
            if (charger->battery == supply)
                charger->battery = NULL;
            remove_supply(charger, supply);
            supply = NULL;
        }
    } else if (!strcmp(uevent->action, "change")) {
        if (!supply) {
            LOGE("power supply '%s' not found ('%s' %d)\n",
                 uevent->ps_name, ps_type, online);
            ret = -1;
            goto error;
        }
    } else {
        LOGE("Unknown uevent action \"%s\"\n", uevent->action);
        ret = -1;
        goto error;
    }

    /* allow battery to be managed in the supply list but make it not
     * contribute to online power supplies. */
    if (!battery) {
        if (was_online && !online)
            charger->num_supplies_online--;
        else if (supply && !was_online && online)
            charger->num_supplies_online++;
    }

    /* If supply list changes, start animating again */
    if (charger->num_supplies_online != old_supplies_online)
        kick_animation();

    LOGI("power supply %s (%s) %s (action=%s num_online=%d num_supplies=%d)\n",
         uevent->ps_name, ps_type, battery ? "" : online ? "online" : "offline",
         uevent->action, charger->num_supplies_online, charger->num_supplies);

    return 0;

error:
    LOGE("Failed to process event { '%s', '%s', '%s', '%s', '%s', '%s' }\n",
         uevent->action, uevent->path, uevent->subsystem,
         uevent->ps_name, uevent->ps_type, uevent->ps_online);

    return ret;

}

static int process_uevent(struct charger *charger, struct uevent *uevent)
{
    if (!strcmp(uevent->subsystem, "power_supply"))
        return process_ps_uevent(charger, uevent);
    return 0;
}

#define UEVENT_MSG_LEN  1024
static int handle_uevent_fd(struct charger *charger, int fd)
{
    char msg[UEVENT_MSG_LEN+2];
    int n;
    int ret = 0;

    if (fd < 0)
        return -1;

    while (true) {
        struct uevent uevent;

        n = uevent_kernel_multicast_recv(fd, msg, UEVENT_MSG_LEN);
        if (n <= 0)
            break;
        if (n >= UEVENT_MSG_LEN)   /* overflow -- discard */
            continue;

        msg[n] = '\0';
        msg[n+1] = '\0';

        parse_uevent(msg, &uevent);
        ret = process_uevent(charger, &uevent);
        if (ret < 0 ) {
            LOGE("Did not process event %s %s\n", uevent.subsystem, uevent.action);
            break;
        }
    }

    return ret;
}

static int uevent_callback(int fd, uint32_t revents, void *data)
{
    struct charger *charger = data;

    if (!(revents & POLLIN))
        return -1;
    return handle_uevent_fd(charger, fd);
}

/* force the kernel to regenerate the change events for the existing
 * devices, if valid */
static void do_coldboot(struct charger *charger, DIR *d, const char *event,
                        bool follow_links, int max_depth)
{
    struct dirent *de;
    int dfd, fd, nbwr;

    dfd = dirfd(d);
    if (dfd >= 0) {
        fd = openat(dfd, "uevent", O_WRONLY);
        if (fd >= 0) {
            nbwr = write(fd, event, strlen(event));
            close(fd);
            if (nbwr <= 0) {
                LOGE("Failed to write event %s\n", event);
                goto close_dfd;
            }
            handle_uevent_fd(charger, charger->uevent_fd);
        }
    }
    else {
        LOGE("Failed to get dir file descriptor %d\n", dfd);
        goto out;
    }

    while ((de = readdir(d)) && max_depth > 0) {
        DIR *d2;

        LOGV("looking at '%s'\n", de->d_name);

        if ((de->d_type != DT_DIR && !(de->d_type == DT_LNK && follow_links)) ||
           de->d_name[0] == '.') {
            LOGV("skipping '%s' type %d (depth=%d follow=%d)\n",
                 de->d_name, de->d_type, max_depth, follow_links);
            continue;
        }
        LOGV("can descend into '%s'\n", de->d_name);

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if (fd < 0) {
            LOGE("cannot openat %d '%s' (%d: %s)\n", dfd, de->d_name,
                errno, strerror(errno));
            continue;
        }

        d2 = fdopendir(fd);
        if (d2 != 0) {
            LOGV("opened '%s'\n", de->d_name);
            do_coldboot(charger, d2, event, follow_links, max_depth - 1);
            closedir(d2);
        }
        close(fd);
    }
close_dfd:
    close(dfd);
out:
    return;
}

static void coldboot(struct charger *charger, const char *path,
                     const char *event)
{
    char str[256];

    LOGV("doing coldboot '%s' in '%s'\n", event, path);
    DIR *d = opendir(path);
    if (d) {
        str[255] = 0;
        snprintf(str, sizeof(str) - 1, "%s\n", event);
        do_coldboot(charger, d, str, true, 1);
        closedir(d);
    }
}

static int set_key_callback(int code, int value, void *data)
{
    struct charger *charger = data;
    int64_t now = curr_time_ms();
    int down = !!value;

    if (code > KEY_MAX)
        return -1;

    /* ignore events that don't modify our state */
    if (charger->keys[code].down == down)
        return 0;

    /* only record the down even timestamp, as the amount
     * of time the key spent not being pressed is not useful */
    if (down)
        charger->keys[code].timestamp = now;
    charger->keys[code].down = down;
    charger->keys[code].pending = true;
    if (down) {
        LOGV("[%"PRId64"] key[%d] down\n", now, code);
    } else {
        int64_t duration = now - charger->keys[code].timestamp;
        int64_t secs = duration / 1000;
        int64_t msecs = duration - secs * 1000;
        LOGV("[%"PRId64"] key[%d] up (was down for %"PRId64".%"PRId64"sec)\n", now,
            code, secs, msecs);
    }

    return 0;
}

static void update_input_state(struct charger *charger,
                               struct input_event *ev)
{
    if (ev->type != EV_KEY)
        return;
    set_key_callback(ev->code, ev->value, charger);
}

static void set_next_key_check(struct charger *charger,
                               struct key_state *key,
                               int64_t timeout)
{
    int64_t then = key->timestamp + timeout;

    if (charger->next_key_check == -1 || then < charger->next_key_check)
        charger->next_key_check = then;
}

static void process_key(struct charger *charger, int code, int64_t now)
{
    struct key_state *key = &charger->keys[code];

    if (code == KEY_POWER) {
        int64_t proceed_timeout = key->timestamp + charger->power_key_ms;
        if (key->down && charger->power_key_ms >= 0) {
            if (now < proceed_timeout) {
                /* if the key is pressed but timeout hasn't expired,
                 * make sure we wake up at the right-ish time to check
                 */
                set_next_key_check(charger, key, charger->power_key_ms);
                set_screen_state(1);
                kick_animation();
            }
        } else {
            /* if the power key got released, force screen state cycle.
             * check if key was hold down longer than power on time
             * to decide to turn on the device. We do the check when key
             * is relaesed to avoid race condition between cold reset
             * and forced shutdown when key is hold down for 7 seconds
             */
            if (!key->down && key->pending) {
                if (now >= proceed_timeout && charger->power_key_ms >= 0) {
                    if (get_battery_capacity(charger) >= charger->min_charge) {
                        LOGI("[%"PRId64"] power button press+hold, exiting\n", now);
                        charger->state = CHARGER_PROCEED;
                    } else {
                        LOGI("[%"PRId64"] ignore press+hold power, battery level "
                                "less than minimum\n", now);
                    }
                }
                set_screen_state(1);
                kick_animation();
            }
        }
    }

    key->pending = false;
}

static void handle_input_state(struct charger *charger, int64_t now)
{
    process_key(charger, KEY_POWER, now);

    if (charger->next_key_check != -1 && now > charger->next_key_check)
        charger->next_key_check = -1;
}

static void handle_power_supply_state(struct charger *charger, int64_t now)
{
    if (charger->unplug_shutdown_ms < 0)
        return;

    if (charger->num_supplies_online == 0) {
        if (charger->next_pwr_check == -1) {
            set_screen_state(1);
            charger->next_pwr_check = now + charger->unplug_shutdown_ms;
            LOGI("[%"PRId64"] device unplugged: shutting down in %"PRId64" (@ %"PRId64")\n",
                 now, charger->unplug_shutdown_ms, charger->next_pwr_check);
        } else if (now >= charger->next_pwr_check) {
            LOGI("[%"PRId64"] shutting down (no online supplies)\n", now);
            /* Subsequent battery level check may change this to CHARGER_PROCEED */
            charger->state = CHARGER_SHUTDOWN;
        } else {
            /* otherwise we already have a shutdown timer scheduled */
        }
    } else {
        /* online supply present, reset shutdown timer if set */
        if (charger->next_pwr_check != -1) {
            LOGI("[%"PRId64"] device plugged in: shutdown cancelled\n", now);
            kick_animation();
        }
        charger->next_pwr_check = -1;
    }
}

/* Periodic check of current battery level to see if it exceeds the
 * minimum threshold */
static void handle_capacity_state(struct charger *charger, int64_t now)
{
    int charge_pct;

    if (!charger->min_charge || charger->mode == MODE_CHARGER)
        return; /* We don't care */

    if (!charger->battery) {
        LOGE("Told to wait until battery is at %d%%, but no "
                "battery detected at all. Exiting.\n", charger->min_charge);
        charger->state = CHARGER_PROCEED;
        return;
    }

    charge_pct = get_battery_capacity(charger);
    if (charge_pct >= charger->min_charge) {
        LOGI("[%"PRId64"] battery capacity %d%% >= %d%%, exiting\n", now,
                charge_pct, charger->min_charge);
        charger->state = CHARGER_PROCEED;
    } else
        LOGV("[%"PRId64"] battery capacity %d%% < %d%%\n", now,
                charge_pct, charger->min_charge);

    if (charger->num_supplies_online == 0)
        charger->next_cap_check = -1;
    else
        charger->next_cap_check = now + charger->cap_poll_ms;
}

static int get_max_temp(void)
{
    int max_temp = -1;
    char buf[256];
    char *p;
    FILE *fd;

    fd = fopen(TEMP_CONF_FILE, "r");
    if (fd == NULL) {
        LOGE("Unable to open thermal config file, "
             "setting default threshold(%d)\n", DEFAULT_TEMP_THRESH);
        return DEFAULT_TEMP_THRESH;
    }

    while (fgets(buf, 256, fd) && strncmp(buf, TEMP_ZONE, strlen(TEMP_ZONE)));
    while (fgets(buf, 256, fd)) {
        p = strstr(buf, TEMP_NAME);
        if (p != NULL) {
            p += strlen(TEMP_NAME)+1;
            max_temp = atoi(p);
            LOGV("max-temp from config-file: %d\n", max_temp);
            break;
        }
    }

    fclose(fd);
    return max_temp;
}

static bool is_platform(const char *data)
{
    int ret;
    char spid_val[4];
    FILE *spid_fd;

    spid_fd = fopen(SPID_FMLY_FILE_NAME, "r");
    if (spid_fd == NULL) {
        LOGE("Unable to open file %s\n", SPID_FMLY_FILE_NAME);
        goto out;
    }

    ret = fseek(spid_fd, 0, SEEK_SET);
    if (ret != 0) {
        LOGE("Unable to set file position %d\n", ret);
        goto close_spid_fd;
    }

    ret = fscanf(spid_fd, "%s\n", spid_val);
    if (ret <= 0) {
        LOGE("Unable to read file %d\n", ret);
        goto close_spid_fd;
    }
    /* check the platform family id */
    if (!strcmp(spid_val, data))
	return true;
    else
	return false;

close_spid_fd:
    fclose(spid_fd);
out:
    return false;
}

static void handle_temperature_state(struct charger *charger)
{
    int temp, ret;
    FILE *temp_fd;

    if (charger->max_temp == -1)
        goto out;

    temp_fd = fopen(SYS_TEMP_INT, "r");
    if (temp_fd == NULL) {
        LOGE("Unable to open file %s\n", SYS_TEMP_INT);
        goto out;
    }

    ret = fseek(temp_fd, 0, SEEK_SET);
    if (ret != 0) {
        LOGE("Unable to set file position %d\n", ret);
        goto close_temp_fd;
    }

    ret  = fscanf(temp_fd, "%d\n", &temp);
    if (ret <= 0) {
        LOGE("Unable to read file %d\n", ret);
        goto close_temp_fd;
    }

    /* TODO: This is workaround for BZ: 165821. This will be removed later. */
    /* For Moorefield-Check platform family id */
    if (!is_platform(MOOR_PF_ID)) {

	if (temp >= charger->max_temp) {
            set_screen_state(1);
            LOGI("Temperature(%d) is higher than threshold(%d), "
                 "shutting down system.\n", temp, charger->max_temp);
            charger->state = CHARGER_SHUTDOWN;
            system("echo 1 > /sys/module/intel_mid_osip/parameters/force_shutdown_occured");
        }
    }

close_temp_fd:
    fclose(temp_fd);
out:
    return;
}

int write_alarm_to_osnib(int mode)
{
    int devfd, errNo, ret;

    devfd = open(IPC_DEVICE_NAME, O_RDWR);
    if (devfd < 0) {
        LOGE("unable to open the DEVICE %s\n", IPC_DEVICE_NAME);
        ret = -1;
        goto err1;
    }

    errNo = ioctl(devfd, IPC_WRITE_ALARM_TO_OSNIB, &mode);
    if (errNo < 0) {
        LOGE("ioctl for DEVICE %s, returns error-%d\n",
                        IPC_DEVICE_NAME, errNo);
        ret = -1;
        goto err2;
    }
    ret = 0;

err2:
    close(devfd);
err1:
    return ret;
}

void *handle_rtc_alarm_event(void *arg)
{
    struct charger *charger = (struct charger *) arg;
    unsigned long data;
    int rtc_fd, ret;
    int batt_cap;
    struct rtc_wkalrm alarm;

    write_alarm_to_osnib(ALARM_CLEAR);

    rtc_fd = open(RTC_FILE, O_RDONLY, 0);
    if (rtc_fd < 0) {
        LOGE("Unable to open the DEVICE %s\n", RTC_FILE);
        goto err1;
    }

    /* RTC alarm set ? */
    ret = ioctl(rtc_fd, RTC_WKALM_RD, &alarm);
    if (ret == -1) {
        LOGE("ioctl(RTC_WKALM_RD) failed\n");
        goto err2;
    }

    if (!alarm.enabled)
        LOGI("no RTC wake-alarm set\n");
    else {
        LOGI("RTC wake-alarm set: %04d-%02d-%02d %02d:%02d:%02d\n",
                alarm.time.tm_year+1900,
                alarm.time.tm_mon+1,
                alarm.time.tm_mday,
                alarm.time.tm_hour,
                alarm.time.tm_min,
                alarm.time.tm_sec);

        /* Enable alarm interrupts */
        ret = ioctl(rtc_fd, RTC_AIE_ON, 0);
        if (ret == -1) {
             LOGE("rtc ioctl RTC_AIE_ON error\n");
             goto err2;
        }
    }

    if (!alarm.pending)
        LOGI("no RTC wake-alarm pending\n");
    else
        LOGI("RTC wake-alarm pending\n");

    /* This blocks until the alarm ring causes an interrupt */
    ret = read(rtc_fd, &data, sizeof(unsigned long));
    if (ret < 0) {
        LOGE("rtc read error\n");
        goto err2;
    }

    batt_cap = get_battery_capacity(charger);
    if (batt_cap >= charger->min_charge) {
        LOGI("RTC alarm rang, Rebooting to MOS");

        if (write_alarm_to_osnib(ALARM_SET))
            LOGE("Error in setting alarm-flag to OSNIB");

        android_reboot(ANDROID_RB_RESTART2, 0, "android");
    } else {
        LOGI("RTC alarm rang, capacity:%d less than minimum threshold:%d, "
             "cannot boot to MOS", batt_cap, charger->min_charge);
    }

err2:
    close(rtc_fd);
err1:
    return NULL;
}

static void wait_next_event(struct charger *charger, int64_t now)
{
    int64_t next_event = INT64_MAX;
    int64_t timeout;
    int ret;

    LOGV("[%"PRId64"] next screen: %"PRId64" next key: %"PRId64" next pwr: %"PRId64" "
            "next cap: %"PRId64"\n", now, charger->next_screen_transition,
            charger->next_key_check, charger->next_pwr_check,
            charger->next_cap_check);

    if (charger->next_screen_transition != -1)
        next_event = charger->next_screen_transition;
    if (charger->next_key_check != -1 && charger->next_key_check < next_event)
        next_event = charger->next_key_check;
    if (charger->next_pwr_check != -1 && charger->next_pwr_check < next_event)
        next_event = charger->next_pwr_check;
    if (charger->next_cap_check != -1 && charger->next_cap_check < next_event)
        next_event = charger->next_cap_check;

    if (next_event != -1 && next_event != INT64_MAX)
        timeout = max(0, next_event - now);
    else
        timeout = -1;
    LOGV("[%"PRId64"] blocking (%"PRId64")\n", now, timeout);
    ret = ev_wait((int)timeout);
    if (!ret)
        ev_dispatch();
}

static int input_callback(int fd, uint32_t revents, void *data)
{
    struct charger *charger = data;
    struct input_event ev;
    int ret;

    ret = ev_get_input(fd, revents, &ev);
    if (ret)
        return -1;
    update_input_state(charger, &ev);
    return 0;
}

static enum charger_exit_state charger_event_loop()
{
    struct charger *charger = &charger_state;

    while (true) {
        int64_t now = curr_time_ms();

        LOGV("[%"PRId64"] event_loop()\n", now);
        handle_input_state(charger, now);
        handle_power_supply_state(charger, now);
        handle_capacity_state(charger, now);
        handle_temperature_state(charger);

        if (charger->state != CHARGER_CHARGE)
            return charger->state;

        /* do screen update last in case any of the above want to start
         * screen transitions (animations, etc)
         */
        update_screen_state(charger, now);
        wait_next_event(charger, now);
    }
}

enum charger_exit_state charger_run(int min_charge, int mode, int power_key_ms,
        int batt_unknown_ms, int unplug_shutdown_ms, int cap_poll_ms)
{
    int fd;
    int64_t now = curr_time_ms() - 1;
    struct charger *charger = &charger_state;
    enum charger_exit_state out_state;

    dump_last_kmsg();

    if (mode == MODE_CHARGER)
        LOGI("--------------- STARTING CHARGER MODE FOR COS ---------------\n");
    else
        LOGI("--------------- STARTING CHARGER MODE TEMPORARILY ---------------\n");

    init_font_size();

    list_init(&charger->supplies);
    charger->min_charge = min_charge;
    charger->mode = mode;
    charger->state = CHARGER_CHARGE;
    charger->power_key_ms = power_key_ms;
    charger->batt_unknown_ms = batt_unknown_ms;
    charger->unplug_shutdown_ms = unplug_shutdown_ms;
    charger->cap_poll_ms = cap_poll_ms;

    charger->max_temp = get_max_temp();
    if (charger->max_temp == -1)
        LOGE("Error in getting maximum temperature threshold");

    if (pthread_create(&charger_thread, NULL, handle_rtc_alarm_event, charger) != 0)
        LOGE("Error in creating rtc-alarm thread\n");

    ev_exit();
    ev_init(input_callback, charger);

    fd = uevent_open_socket(64*1024, true);
    if (fd >= 0) {
        fcntl(fd, F_SETFL, O_NONBLOCK);
        ev_add_fd(fd, uevent_callback, charger);
    }
    else
        LOGE("Failed to create uevent socket\n");

    charger->uevent_fd = fd;
    coldboot(charger, "/sys/class/power_supply", "add");

    init_surfaces();

    ev_sync_key_state(set_key_callback, charger);

    charger->next_screen_transition = now - 1;
    charger->next_key_check = -1;
    charger->next_pwr_check = -1;
    charger->next_cap_check = -1;

    reset_animation();
    kick_animation();
    out_state = charger_event_loop();
    clean_surfaces(out_state, min_charge);
    ev_exit();
    set_screen_state(1);
    return out_state;
}

