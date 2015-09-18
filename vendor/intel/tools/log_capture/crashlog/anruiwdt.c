/* Copyright (C) Intel 2010
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
 * @file anruiwdt.c
 * @brief File containing functions for anr and uiwdt events processing.
 *
 * This file contains functions used to process ANR and UIWDT events.
 */

#include <sys/sendfile.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#include <cutils/properties.h>

#ifdef CONFIG_BTDUMP
#include <libbtdump.h>
#endif

#include "crashutils.h"
#include "privconfig.h"
#include "anruiwdt.h"
#include "dropbox.h"
#include "fsutils.h"

#ifdef FULL_REPORT
static void priv_prepare_anruiwdt(char *destion)
{
    char cmd[PATHMAX];
    int len = strlen(destion);
    if (len < 4) return;

    if ( destion[len-3] == '.' && destion[len-2] == 'g' && destion[len-1] == 'z') {
        /* extract gzip file */
        snprintf(cmd, sizeof(cmd), "gunzip %s", destion);
        system(cmd);
        destion[len-3] = 0;
        if (do_chown(destion, PERM_USER, PERM_GROUP)!=0) {
            LOGE("%s: do_chown failed : status=%s...\n", __FUNCTION__, strerror(errno));}
    }
}
#else
static void priv_prepare_anruiwdt(char *destion)
{
    char cmd[PATHMAX];
    int len = strlen(destion);
    if (len < 4) return;

    if ( destion[len-3] == '.' && destion[len-2] == 'g' && destion[len-1] == 'z') {
        /* extract gzip file */
        do_copy_tail(destion, LOGS_DIR "/tmp_anr_uiwdt.gz",0);
        system("gunzip " LOGS_DIR "/tmp_anr_uiwdt.gz");
        do_chown(LOGS_DIR "/tmp_anr_uiwdt", PERM_USER, PERM_GROUP);
        destion[strlen(destion) - 3] = 0;
        do_copy_tail(LOGS_DIR "/tmp_anr_uiwdt",destion,0);
        remove(LOGS_DIR "/tmp_anr_uiwdt");
    }
}
#endif

#ifdef FULL_REPORT
static void process_anruiwdt_tracefile(char *destion, int dir, int removeunparsed)
{
    char cmd[PATHMAX];
    int src, dest;
    char dest_path[PATHMAX] = {'\0'};
    char dest_path_symb[PATHMAX];
    struct stat stat_buf;
    char *tracefile;
    FILE *fp;
    int i;

    fp = fopen(destion, "r");
    if (fp == NULL) {
        LOGE("%s: Failed to open file %s:%s\n", __FUNCTION__, destion, strerror(errno));
        return;
    }
    /* looking for "Trace file:" from the first 100 lines */
    for (i = 0; i < 100; i++) {
        if ( fgets(cmd, sizeof(cmd), fp) && !strncmp("Trace file:", cmd, 11) ) {
            tracefile = cmd + 11;
            if (!strlen(tracefile)) {
                LOGE("%s: Found lookup pattern, but without tracefile\n", __FUNCTION__);
                break;
            }
            tracefile[strlen(tracefile) - 1] = 0; /* eliminate trailing \n */
            if ( !file_exists(tracefile) ) {
                LOGE("%s: %s lists a trace file (%s) but it does not exist...\n", __FUNCTION__, destion, tracefile);
                break;
            }
            // copy
            src = open(tracefile, O_RDONLY);
            if (src < 0) {
                LOGE("%s: Failed to open trace file %s:%s\n", __FUNCTION__, tracefile, strerror(errno));
                break;
            }
            snprintf(dest_path, sizeof(dest_path), "%s%d/trace_all_stack.txt", CRASH_DIR, dir);
            fstat(src, &stat_buf);
            dest = open(dest_path, O_WRONLY|O_CREAT, 0600);
            if (dest < 0) {
                LOGE("%s: Failed to create dest file %s:%s\n", __FUNCTION__, dest_path, strerror(errno));
                close(src);
                break;
            }
            close(dest);
            do_chown(dest_path, PERM_USER, PERM_GROUP);
            dest = open(dest_path, O_WRONLY, stat_buf.st_mode);
            if (dest < 0) {
                LOGE("%s: Failed to open dest file %s after setting the access rights:%s\n", __FUNCTION__, dest_path, strerror(errno));
                close(src);
                break;
            }
            sendfile(dest, src, NULL, stat_buf.st_size);
            close(src);
            close(dest);
            // remove src file
            if (unlink(tracefile) != 0) {
                LOGE("%s: Failed to remove tracefile %s:%s\n", __FUNCTION__, tracefile, strerror(errno));
            }
            if ( removeunparsed && unlink(dest_path)) {
                LOGE("Failed to remove unparsed tracefile %s:%s\n", dest_path, strerror(errno));
            }
            break;
        }
    }
    fclose(fp);

    if (dest_path[0] == '\0') {
        LOGE("%s: Destination path not set\n", __FUNCTION__);
        return;
    }
    do_chown(dest_path, PERM_USER, PERM_GROUP);
    snprintf(dest_path_symb, sizeof(dest_path_symb), "%s_symbol", dest_path);
    do_chown(dest_path_symb, PERM_USER, PERM_GROUP);
}

static void backtrace_anruiwdt(char *dest, int dir) {
    char value[PROPERTY_VALUE_MAX];

    property_get(PROP_ANR_USERSTACK, value, "0");
    if (strncmp(value, "1", 1)) {
        process_anruiwdt_tracefile(dest, dir, 0);
    }
}
#else
static inline void backtrace_anruiwdt(char *dest __unused, int dir __unused) {}
#endif

#define PATH_LENGTH			256
void do_copy_pvr(char * src, char * dest) {
   char *token = NULL;
   char *str = NULL;
   char buf[PATH_LENGTH] = {0, };
   char path[PATH_LENGTH] = {0, };
   FILE * fs = NULL;
   FILE * fd = NULL;
   fs = fopen(src,"r");
   fd = fopen(dest,"w");
   if (fs && fd) {
		while(fgets(buf, PATH_LENGTH, fs)) {
		    fputs(buf ,fd);
		 }
   }
   if (fs)
      fclose(fs);
   if (fd)
      fclose(fd);

   return;
}

#ifdef CONFIG_BTDUMP

struct bt_dump_arg {
    int eventtype;
    int dir;
    char *destion;
    char *eventname;
};

void *dump_bt_all(void *param) {
    struct bt_dump_arg *args = (struct bt_dump_arg *) param;
    char destion_btdump[PATHMAX];
    FILE *f_btdump;
    char *key;
    char cmd[512];
    static pthread_mutex_t run_once = PTHREAD_MUTEX_INITIALIZER;

    LOGV("Full process backtrace dump started");
    snprintf(destion_btdump, sizeof(destion_btdump), "%s/all_back_traces.txt",
             args->destion);
    f_btdump = fopen(destion_btdump, "w");
    if (f_btdump) {
        if(pthread_mutex_trylock(&run_once)==0) {
            bt_all(f_btdump);
            pthread_mutex_unlock(&run_once);
        } else {
            fprintf(f_btdump,"Another instance of bt_dump is running\n");
            fprintf(f_btdump,"Check previous ANR/UIWDT events\n");
        }
        fclose(f_btdump);
    }

    key = raise_event(CRASHEVENT, args->eventname, NULL, args->destion);
    LOGE("%-8s%-22s%-20s%s %s\n", CRASHEVENT, key, get_current_time_long(0),
         args->eventname, args->destion);
#ifdef FULL_REPORT
    if (args->eventtype != ANR_TYPE
        || start_dumpstate_srv(CRASH_DIR, args->dir, key) <= 0) {
#endif
        /*done */
        snprintf(cmd, sizeof(cmd) - 1, "am broadcast -n com.intel.crashreport"
                 "/.specific.NotificationReceiver -a com.intel.crashreport.intent.CRASH_LOGS_COPY_FINISHED "
                 "-c android.intent.category.ALTERNATIVE --es com.intel.crashreport.extra.EVENT_ID %s",
                 key);

        int status = system(cmd);
        if (status != 0)
            LOGI("%s: Notify crashreport status(%d) for command \"%s\".\n",
                 __FUNCTION__, status, cmd);
        free(key);
#ifdef FULL_REPORT
    }
#endif

    free(args->eventname);
    free(args->destion);
    free(param);
    return NULL;
}
#endif

/*
* Name          :
* Description   :
* Parameters    :
*/
int process_anruiwdt_event(struct watch_entry *entry, struct inotify_event *event) {
    char path[PATHMAX];
    char destion[PATHMAX];
    const char *dateshort = get_current_time_short(1);
    char *key = NULL;
    int dir;
#ifdef CONFIG_BTDUMP
    struct bt_dump_arg *btd_param;
    pthread_t btd_thread;
    char bt_dis_prop[PROPERTY_VALUE_MAX];
#endif
    /* Check for duplicate dropbox event first */
    if ( manage_duplicate_dropbox_events(event) )
        return 1;

    dir = find_new_crashlog_dir(MODE_CRASH);
    snprintf(destion,sizeof(destion),"%s%d/%s", CRASH_DIR, dir, "pvr_debug_dump.txt");
    do_copy_pvr("/d/pvr/debug_dump", destion);
    snprintf(destion,sizeof(destion),"%s%d/%s", CRASH_DIR, dir, "fence_sync.txt");
    do_copy_pvr("/d/sync", destion);
    snprintf(path, sizeof(path),"%s/%s", entry->eventpath, event->name);
    if (dir < 0 || !file_exists(path)) {
        if (dir < 0)
            LOGE("%s: Cannot get a valid new crash directory...\n", __FUNCTION__);
        else
            LOGE("%s: Cannot access %s\n", __FUNCTION__, path);
        key = raise_event(CRASHEVENT, entry->eventname, NULL, NULL);
        LOGE("%-8s%-22s%-20s%s\n", CRASHEVENT, key, get_current_time_long(0), entry->eventname);
        free(key);
        return -1;
    }

    snprintf(destion,sizeof(destion),"%s%d/%s", CRASH_DIR, dir, event->name);
    do_copy_tail(path, destion, MAXFILESIZE);
    priv_prepare_anruiwdt(destion);
    usleep(TIMEOUT_VALUE);
    do_log_copy(entry->eventname, dir, dateshort, APLOG_TYPE);
    backtrace_anruiwdt(destion, dir);
    restart_profile_srv(1);
    snprintf(destion, sizeof(destion), "%s%d", CRASH_DIR, dir);
#ifdef CONFIG_BTDUMP
    property_get(PROP_ANR_USERSTACK, bt_dis_prop, "0");
    if (!strcmp(bt_dis_prop, "0")) {
        /*alloc arguments */
        btd_param = malloc(sizeof(struct bt_dump_arg));
        btd_param->destion = strdup(destion);
        btd_param->eventname = strdup(entry->eventname);
        btd_param->eventtype = entry->eventtype;
        btd_param->dir = dir;

        if (pthread_create(&btd_thread, NULL, dump_bt_all, (void *) btd_param)) {
            LOGE("Cannot start full process list backtrace dump.");
        } else {
            /*Everything will end on the new thread */
            return 1;
        }
        free(btd_param->eventname);
        free(btd_param->destion);
        free(btd_param);
    }
#endif
    key = raise_event(CRASHEVENT, entry->eventname, NULL, destion);
    LOGE("%-8s%-22s%-20s%s %s\n", CRASHEVENT, key, get_current_time_long(0),
         entry->eventname, destion);

    switch (entry->eventtype) {
#ifdef FULL_REPORT
        case ANR_TYPE:
            if (start_dumpstate_srv(CRASH_DIR, dir, key) <= 0) {
                /* Finally raise the event as the dumpstate server is busy or failed to be started */
                free(key);
            }
            break;
#endif
        default:
            free(key);
            break;
    }
    return 1;
}
