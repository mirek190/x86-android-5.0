/* Copyright (C) Intel 2013
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file dropbox.c
 * @brief File containing functions used to handle dropbox events.
 *
 * This file contains the functions used to handle various operations linked to
 * dropbox events such as 'lost' event processing, dropbox duplicate events processing,
 * dumpstate server management...
 */
#include <sys/inotify.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <openssl/sha.h>

#include "crashutils.h"
#include "privconfig.h"
#include "fsutils.h"
#include "dropbox.h"

static char gcurrent_key[2][SHA_DIGEST_LENGTH+1] = {{0,},{0,}};
static int index_prod = 0;
static int index_cons = 0;
static int  gfile_monitor_fd = -1;

struct arg_cmd {
    char key[SHA_DIGEST_LENGTH+1];
};

void dropbox_set_file_monitor_fd(int file_monitor_fd) {
    gfile_monitor_fd = file_monitor_fd;
}

int start_dumpstate_srv(char* crash_dir, int crashidx, char *key) {
    char dumpstate_dir[PROPERTY_VALUE_MAX];
    if ( !crash_dir || !key ) return 0;

    /* Check if a dumpstate is already running */
    property_get(PROP_LOGSYSTEMSTATE, dumpstate_dir, "stopped");
    if(strcmp(dumpstate_dir,"running") == 0) {
        LOGI("%s: Can't launch dumpstate for %s%d, already running.\n", __FUNCTION__, crash_dir, crashidx);
        return 0;
    }

    snprintf(dumpstate_dir, sizeof(dumpstate_dir), "%s%d/", crash_dir, crashidx);
    property_set("crashlogd.storage.path", dumpstate_dir);
#ifdef __TEST__
{
    char command[PATHMAX];
    snprintf(command, PATHMAX, "touch %s/dumpstate-0-0-0-0-0-0.txt %s/dropbox-0-0-0-0-0-0.txt", dumpstate_dir, dumpstate_dir);
    system(command);
}
#else
    start_daemon("logsystemstate");
#endif
    if (inotify_add_watch(gfile_monitor_fd, dumpstate_dir, IN_CLOSE_WRITE) < 0) {
        LOGE("%s: Can't add watch for %s - %s.\n", __FUNCTION__,
            dumpstate_dir, strerror(errno));
        return -1;
    }
    strncpy(gcurrent_key[index_prod],key,SHA_DIGEST_LENGTH+1);
    index_prod = (index_prod + 1) % 2;
    return 1;
}

void finalize_dropbox_system_thread(void *param) {
    char cmd[512];
    struct arg_cmd *args = (struct arg_cmd *)param;

    snprintf(cmd,sizeof(cmd)-1,"am broadcast -n com.intel.crashreport"
        "/.specific.NotificationReceiver -a com.intel.crashreport.intent.CRASH_LOGS_COPY_FINISHED "
        "-c android.intent.category.ALTERNATIVE --es com.intel.crashreport.extra.EVENT_ID %s",
        args->key);
    free(args);
    int status = system(cmd);
    if (status == -1) {
        LOGI("%s: Notify crashreport failed for command \"%s\".\n", __FUNCTION__, cmd);
    } else {
        LOGI("%s: Notify crashreport status(%d) for command \"%s\".\n", __FUNCTION__, status, cmd);
    }

}

/* TODO, change the current key with a list of keys and compare with the
 * event to retreive the one */
int finalize_dropbox_pending_event(const struct inotify_event __attribute__((unused)) *event) {
    char boot_state[PROPERTY_VALUE_MAX];
    int ret = 0;
    pthread_t thread;

    /* gcurrent_key is in provision */
    if (gcurrent_key[index_cons][0] == 0) {
        LOGE("%s: Received a dropbox event but no key is pending, drop it...\n", __FUNCTION__);
        return -1;
    }

    property_get(PROP_BOOT_STATUS, boot_state, "-1");
    if (strcmp(boot_state, "1"))
        return -1;

    struct arg_cmd * args_thread =  malloc(sizeof(struct arg_cmd));
    strncpy(args_thread->key,gcurrent_key[index_cons],sizeof(args_thread->key));
    ret = pthread_create(&thread, NULL, (void *)finalize_dropbox_system_thread, args_thread);
    if (ret < 0) {
        LOGE("%s: finalize_dropbox thread error", __FUNCTION__);
        free(args_thread);
    }

    gcurrent_key[index_cons][0] = 0;
    index_cons = (index_cons + 1) % 2;
    return 0;
}

/**
 * @brief convert the timestamp from a dropbox log filename
 *
 * @param[in] dropboxname is the name of the log file containing the timestamp to convert
 *
 * @return a negative in case of failure, 0 if ok.
 */
int convert_dropbox_timestamp(char* dropboxname, char *timestamp) {
    int res;
    int year, month, day, hours, minutes, seconds;
    LOGD("%s\n", __FUNCTION__);
    if (dropboxname == NULL) return -EINVAL;

    res = sscanf(dropboxname, "dropbox.%04d-%02d-%02d-%02d-%02d-%02d.txt",
        &year, &month, &day, &hours, &minutes, &seconds);
    if (res < 6) {
        /* Cannot extract the date, get a fresh one */
        const char* newts = get_current_time_long(1);
        return (strncpy(timestamp, newts, strlen(newts)) != NULL ? 0 : -1);
    }
    return (sprintf(timestamp, "%04d-%02d-%02d/%02d:%02d:%02d",
        year, month, day, hours, minutes, seconds) > 0 ? 0 : -1);
}

/**
 * @brief Extract the timestamp from a dropbox log filename
 *
 * Extract the timestamp value contained in the name of a log file generated by the
 * dropbox manager. The timestamp max value that could be extracted is equal to max value for
 * 'long' type (2 147 483 647 => 19 Jan 2038)
 *
 * @param[in] filename is the name of the log file containing the timestamp to extract
 *
 * @return a long equal to the extracted timestamp value. -1 in case of failure.
 */
long extract_dropbox_timestamp(char* filename)
{
    char *ptr_timestamp_start,*ptr_timestamp_end;
    char timestamp[TIMESTAMP_MAX_SIZE];
    unsigned int i;
    LOGD("%s\n", __FUNCTION__);
    //DropBox log filename format is : 'error/log type' + '@' + 'timestamp' + '.' + 'file suffix'
    //Examples : system_app_anr@1350992829414.txt or system_app_crash@1266667499976.txt
    //So we look for '@' then we look for '.' then extract characters between those two characters
    //and finally we strip the 3 last characters corresponding to millisecs to have an UNIX style timestamp
    ptr_timestamp_start = strchr(filename, '@');
    if (ptr_timestamp_start) {
        //Point to timestamp 1st char
        ptr_timestamp_start++;
        ptr_timestamp_end = strchr(ptr_timestamp_start, '.');
        if (ptr_timestamp_end) {
            int size_timestamp = ptr_timestamp_end - ptr_timestamp_start - 3;
            //checks timestamp size
            if ( (size_timestamp <= TIMESTAMP_MAX_SIZE ) && (size_timestamp > 0 )) {
                strncpy(timestamp, ptr_timestamp_start, size_timestamp);
                //checks timestamp consistency
                while(size_timestamp--) {
                    if (!isdigit(timestamp[size_timestamp]))
                        return -1;
                }
                //checks timestamp value compatibility with 'long' type
                if ( atoll(timestamp) > LONG_MAX )
                    return -1;
                return atol(timestamp);
            }
        }
    }
    return -1;
}

 /**
 * @brief allows to avoid processing a duplicate dropbox event as a real event
 *
 * This function allows to not process twice a same dropbox log file that
 * has been only renamed (notably because of the DropBoxManager service)
 *
 * @param[in] event : current inotify event detected by the file system watcher
 * @param[in] files : nb max of logs destination directories
 *
 * @retval -1 the input event is DUPLICATE and shall NOT be processed
 * @retval 0 the input event shall be processed as usual
 */
int manage_duplicate_dropbox_events(struct inotify_event *event)
{
    static uint32_t previous_event_cookie = 0;
    static char previous_filename[PATHMAX] = { '\0', };
    char info_filename[PATHMAX] = { '\0',};
    char destination[PATHMAX] = { '\0', };
    char origin[PATHMAX] = { '\0', };
    struct stat info;
    long timestamp_value;
    //char timestamp[32] = "timestamp_extract_failed"; //initialized to default value
    char human_readable_date[32] = "timestamp_extract_failed"; //initialized to default value
    /*
     * If a file is moved from the dropbox directory, it shall
     * not be processed as an event but the inotify event cookie
     * and name shall be saved
     */
    if (event->mask & IN_MOVED_FROM) {
        previous_event_cookie = event->cookie;
        strncpy(previous_filename, event->name, MIN(event->len, PATHMAX));
        return -1;
    }

    /*
     * If a log file is moved to the dropbox directory, it could
     * be the log file previously moved from the dropbox so we check
     * it
     *
     * NOTE : we can reasonably assume kernel behaviour is reliable
     * and then move events are always emitted as contiguous pairs
     * with IN_MOVED_FROM immediately followed by IN_MOVED_TO
     */
    if ( (event->mask & IN_MOVED_TO) && (previous_event_cookie != 0)
        && (previous_event_cookie == event->cookie) && event->len) {

        /*
         * the log file is temporaly copied from dropbox directory
         * to /logs and is renamed so it could be detected and
         * processed as an infoevent data
         */
        if (strstr(event->name, "anr")) {
            snprintf(destination,sizeof(destination),"%s/%s", LOGS_DIR, ANR_DUPLICATE_DATA);
            strcpy(info_filename, ANR_DUPLICATE_INFOERROR);
        }
        else if ( strstr(event->name, "system_server_watchdog") ) {
            snprintf(destination,sizeof(destination),"%s/%s", LOGS_DIR, UIWDT_DUPLICATE_DATA);
            strcpy(info_filename, UIWDT_DUPLICATE_INFOERROR);
        }
        else { /* event->name contains "crash" */
            snprintf(destination,sizeof(destination),"%s/%s", LOGS_DIR, JAVACRASH_DUPLICATE_DATA);
            strcpy(info_filename, JAVACRASH_DUPLICATE_INFOERROR);
        }
        snprintf(origin,sizeof(origin),"%s/%s", DROPBOX_DIR, event->name);

        /* manages compressed log file */
        if ( !strcmp(".gz", &origin[strlen(origin) - 3]) )
            strcat(destination,".gz");

        if((stat(origin, &info) == 0) && (info.st_size != 0))
            do_copy_tail(origin, destination, MAXFILESIZE);

        //Fetch the timestamp from the original log filename and write it in infoevent as a human readable date
        timestamp_value = extract_dropbox_timestamp(previous_filename);

        if (timestamp_value != -1) {
            struct tm *time;
            memset(&time, 0, sizeof(time));
            time = localtime(&timestamp_value);
            if (time) {
                PRINT_TIME(human_readable_date, DUPLICATE_TIME_FORMAT , time);
            } else {
                LOGE("%s Could not print human readable timestamp\n", __FUNCTION__);
            }
        }
        /*
         * Generates the INFO event with the previous and the new
         * filename as DATA0 and DATA1 and with the date previously
         * fetched set in DATA2
         */
        create_infoevent(info_filename, previous_filename, event->name, human_readable_date);

        /* re-initialize variables for next event */
        previous_event_cookie = 0;
        strcpy(previous_filename, "");
        return -1;
    }
    return 0;
}

int process_lost_event(struct watch_entry __attribute__((unused)) *entry, struct inotify_event *event) {
    int dir;
    char destination[PATHMAX], path[PATHMAX];
    char lostevent[32];
    char lostevent_subtype[32];
    char *key;
    if (strstr(event->name, "anr"))
        strcpy(lostevent, ANR_EVNAME);
    else if (strstr(event->name, "crash"))
        strcpy(lostevent, JAVACRASH_EVNAME);
    else if (strstr(event->name, "watchdog"))
        strcpy(lostevent, SYSSERVER_EVNAME);
    else /* unknown full dropbox event */
        return 0;

    /* Check for duplicate dropbox event first */
    /* Note : can't currently work because because when a system time update occurs, *.lost dropbox files
     * are not simply renamed by the DropBox Manager but deleted and re-created with the current
     * system timestamp. */
    if ( manage_duplicate_dropbox_events(event) )
        return 1;

    snprintf(lostevent_subtype, sizeof(lostevent_subtype), "%s_%s", LOST_EVNAME, lostevent);

    dir = find_new_crashlog_dir(MODE_CRASH_NOSD);
    if (dir < 0) {
        LOGE("%s: Find dir for lost dropbox failed\n", __FUNCTION__);
        key = raise_event(CRASHEVENT, lostevent, lostevent_subtype, NULL);
        LOGE("%-8s%-22s%-20s%s\n", CRASHEVENT, key, get_current_time_long(0), lostevent);
        free(key);
        return -1;
    }
    /* Copy the *.lost dropbox file */
    snprintf(path, sizeof(path),"%s/%s", entry->eventpath, event->name);
    snprintf(destination,sizeof(destination),"%s%d/%s", CRASH_DIR,dir,event->name);
    do_copy(path, destination, 0);
    usleep(TIMEOUT_VALUE);
    snprintf(destination,sizeof(destination),"%s%d/",CRASH_DIR,dir);
    do_log_copy(lostevent, dir, get_current_time_short(1), APLOG_TYPE);
    key = raise_event(CRASHEVENT, lostevent, lostevent_subtype, destination);
    LOGE("%-8s%-22s%-20s%s %s\n", CRASHEVENT, key, get_current_time_long(0), lostevent, destination);
    free(key);
    return 0;
}
