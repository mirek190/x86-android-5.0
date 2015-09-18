/* * Copyright (C) Intel 2010
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
 * @file modem.c
 * @brief File containing functions to handle the processing of modem events.
 *
 * This file contains the functions to handle the processing of modem events and
 * modem shutdown events.
 */

#include "inotify_handler.h"
#include "crashutils.h"
#include "fsutils.h"
#include "privconfig.h"
#include "config_handler.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

extern pconfig g_first_modem_config;

static int copy_modemcoredump(char *spath, char *dpath) {
    char src[PATHMAX];
    char des[PATHMAX];
    struct stat st;
    DIR *d;
    struct dirent *de;

    if (stat(spath, &st))
        return -errno;
    if (stat(dpath, &st))
        return -errno;

    src[0] = 0;
    des[0] = 0;

    d = opendir(spath);
    if (d == 0) {
        LOGE("%s: opendir failed - %s\n", __FUNCTION__, strerror(errno));
        return -errno;
    }
    while ((de = readdir(d)) != 0) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        if (de->d_name[0] == 'c' && de->d_name[1] == 'd'
                && strstr(de->d_name, ".tar.gz")) {
            /* file form is cd_xxx.tar.gz */
            snprintf(src, sizeof(src), "%s/%s", spath, de->d_name);
            snprintf(des, sizeof(des), "%s/%s", dpath, de->d_name);
            do_copy_tail(src, des, 0);
            remove(src);
        }
    }
    if (closedir(d) < 0){
        LOGE("%s: closedir failed - %s\n", __FUNCTION__, strerror(errno));
        return -errno;
    }
    return 0;

}

int process_modem_event(struct watch_entry *entry, struct inotify_event *event) {
    int dir;
    char path[PATHMAX];
    char destion[PATHMAX];
    const char *dateshort = get_current_time_short(1);
    char *key;

    snprintf(path, sizeof(path),"%s/%s", entry->eventpath, event->name);
    dir = find_new_crashlog_dir(MODE_CRASH);
    if (dir < 0) {
        LOGE("%s: find_new_crashlog_dir failed\n", __FUNCTION__);
        key = raise_event(CRASHEVENT, entry->eventname, NULL, NULL);
        LOGE("%-8s%-22s%-20s%s\n", CRASHEVENT, key, get_current_time_long(0), entry->eventname);
        rmfr(path);
        free(key);
        return -1;
    }

    snprintf(destion,sizeof(destion),"%s%d", CRASH_DIR,dir);
    /*Copy Coredump only if event is a modem crash*/
    if (entry->eventtype == MDMCRASH_TYPE ) {
        int status = copy_modemcoredump(entry->eventpath, destion);
        if (status != 0)
            LOGE("backup modem core dump status: %d.\n", status);
    }
    snprintf(destion,sizeof(destion),"%s%d/%s", CRASH_DIR, dir, event->name);
    do_copy_tail(path, destion, MAXFILESIZE);
    snprintf(destion,sizeof(destion),"%s%d", CRASH_DIR, dir);
    usleep(TIMEOUT_VALUE);
    do_log_copy(entry->eventname, dir, dateshort, APLOG_TYPE);
    do_log_copy(entry->eventname, dir, dateshort, BPLOG_TYPE);
    key = raise_event(CRASHEVENT, entry->eventname, NULL, destion);
    LOGE("%-8s%-22s%-20s%s %s\n", CRASHEVENT, key, get_current_time_long(0), entry->eventname, destion);
    rmfr(path);
    free(key);
    return 0;
}

int crashlog_check_modem_shutdown() {
    const char *dateshort = get_current_time_short(1);
    char destion[PATHMAX];
    int dir;
    char *key;

    if ( !file_exists(MODEM_SHUTDOWN_TRIGGER) ) {
        /* Nothing to do */
        return 0;
    }

    dir = find_new_crashlog_dir(MODE_CRASH);
    if (dir < 0) {
        LOGE("%s: find_new_crashlog_dir failed\n", __FUNCTION__);
        key = raise_event(CRASHEVENT, MODEM_SHUTDOWN, NULL, NULL);
        LOGE("%-8s%-22s%-20s%s\n", CRASHEVENT, key, get_current_time_long(0), MODEM_SHUTDOWN);
        remove(MODEM_SHUTDOWN_TRIGGER);
        free(key);
        return -1;
    }

    destion[0] = '\0';
    snprintf(destion, sizeof(destion), "%s%d/", CRASH_DIR, dir);

    usleep(TIMEOUT_VALUE);
    do_log_copy(MODEM_SHUTDOWN, dir, dateshort, APLOG_TYPE);
    do_last_kmsg_copy(dir);
    key = raise_event(CRASHEVENT, MODEM_SHUTDOWN, NULL, destion);
    LOGE("%-8s%-22s%-20s%s %s\n", CRASHEVENT, key, get_current_time_long(0), MODEM_SHUTDOWN, destion);
    free(key);
    remove(MODEM_SHUTDOWN_TRIGGER);

    return 0;
}

int crashlog_check_mpanic_abort(){
    char destion[PATHMAX];
    int dir;
    char *key;
    const char *dateshort = get_current_time_short(1);

    if (file_exists(MCD_PROCESSING)) {
        remove(MCD_PROCESSING);

        dir = find_new_crashlog_dir(MODE_CRASH);
        if (dir < 0) {
            LOGE("%s: find_new_crashlog_dir failed\n", __FUNCTION__);
            key = raise_event(CRASHEVENT, MDMCRASH_EVNAME, NULL, NULL);
            LOGE("%-8s%-22s%-20s%s\n", CRASHEVENT, key, get_current_time_long(0), MDMCRASH_EVNAME);
            free(key);
            return -1;
        }

        do_log_copy(MDMCRASH_EVNAME,dir,dateshort,APLOG_TYPE);
        do_log_copy(MDMCRASH_EVNAME,dir,dateshort,BPLOG_TYPE_OLD);

        snprintf(destion,sizeof(destion),"%s%d/", CRASH_DIR,dir);

        FILE *fp;
        char fullpath[PATHMAX];
        snprintf(fullpath, sizeof(fullpath)-1, "%s/%s_crashdata", destion, MDMCRASH_EVNAME );

        fp = fopen(fullpath,"w");
        if (fp == NULL){
            LOGE("%s: can not create file: %s\n", __FUNCTION__, fullpath);
        }else{
            fprintf(fp,"DATA0=%s\n", MPANIC_ABORT);
            fprintf(fp,"_END\n");
            fclose(fp);
            do_chown(fullpath, PERM_USER, PERM_GROUP);
        }

        key = raise_event(CRASHEVENT, MDMCRASH_EVNAME, NULL, destion);
        LOGE("%-8s%-22s%-20s%s %s\n", CRASHEVENT, key, get_current_time_long(0), MDMCRASH_EVNAME, destion);
        free(key);
    }
    return 0;
}

/*
* Name          : process_modem_generic
* Description   : processes modem generic events
* Parameters    :
*   char *filename        -> path of watched directory/file
*   char *eventname       -> name of the watched event
*   char *name            -> name of the file inside the watched directory that has triggered the event
*   unsigned int files    -> nb max of logs destination directories (crashlog, aplogs, bz... )
*   int fd                -> file descriptor referring to the inotify instance */
int process_modem_generic(struct watch_entry *entry, struct inotify_event *event, int fd __unused) {

    const char *dateshort = get_current_time_short(1);
    char *key;
    int dir;
    char path[PATHMAX];
    char path_linked[PATHMAX];
    char name_linked[PATHMAX];
    char destion[PATHMAX];
    char event_class[PATHMAX];
    FILE *fp;
    char fullpath[PATHMAX];
    int event_mode;
    int wd;
    int ret = 0, data_ready = 1;
    int generate_data = 0;
    pthread_t thread;
    pconfig linkedConfig=NULL;

    pconfig curConfig = get_generic_config(event->name, g_first_modem_config);
    if(!curConfig){
        LOGE("%s: no generic configuration found\n",  __FUNCTION__);
        return -1;
    }
    //select event type
    if (curConfig->event_class == 0){
        strncpy(event_class,CRASHEVENT, sizeof(event_class));
        event_mode = MODE_CRASH;
    }else if (curConfig->event_class == 1){
        strncpy(event_class,ERROREVENT, sizeof(event_class));
        event_mode = MODE_STATS;
    }else if (curConfig->event_class == 2){
        strncpy(event_class,INFOEVENT, sizeof(event_class));
        event_mode = MODE_STATS;
    }else{
        //default mode = crash mode
        strncpy(event_class,CRASHEVENT, sizeof(event_class));
        event_mode = MODE_CRASH;
    }
    //adding security NULL character
    event_class[sizeof(event_class)-1] = '\0';

    snprintf(path, sizeof(path),"%s/%s", entry->eventpath, event->name);

    dir = find_new_crashlog_dir(event_mode);
    if (dir < 0) {
        LOGE("%s: find_new_crashlog_dir failed\n", __FUNCTION__);
        key = raise_event(CRASHEVENT, curConfig->eventname, NULL, NULL);
        LOGE("%-8s%-22s%-20s%s\n", event_class, key, get_current_time_long(0), curConfig->eventname);
        rmfr(path);
        free(key);
        return -1;
    }

    //update event_dir should be done after find_dir call
    if (event_mode == MODE_STATS){
        snprintf(destion,sizeof(destion),"%s%d/", STATS_DIR,dir);
    }else if (event_mode == MODE_CRASH) {
        snprintf(destion,sizeof(destion),"%s%d/", CRASH_DIR,dir);
    }
    usleep(TIMEOUT_VALUE);

    //massive copy of directory found for type "directory"
    do_log_copy(curConfig->eventname, dir, dateshort, APLOG_TYPE);
    if (curConfig->type ==1){
        struct arg_copy * args =  malloc(sizeof(struct arg_copy));
        if(!args) {
            LOGE("%s: malloc failed\n", __FUNCTION__);
            return -1;
        }
        //time in seconds( should be less than phone doctor timer)
        args->time_val = 100 ;
        strncpy(args->orig,path,sizeof(args->orig));
        strncpy(args->dest,destion,sizeof(args->dest));
        ret = pthread_create(&thread, NULL, (void *)copy_dir, (void *)args);
        if (ret < 0) {
            LOGE("%s: pthread_create copy dir error", __FUNCTION__);
            free(args);
            //if ret >=0 free is done inside copy_dir
        }else{
            //backgroung thread is running. Event should be tagged not ready
            data_ready = 0;
        }
        if (strlen(curConfig->path_linked)>0){
            //now copy linked data
            linkedConfig = get_generic_config_by_path(curConfig->path_linked, g_first_modem_config);
            if (linkedConfig){
                strncpy(name_linked, event->name, sizeof(name_linked));
                //adding security NULL character
                name_linked[sizeof(name_linked)-1] = '\0';
                if (!str_simple_replace(name_linked,curConfig->matching_pattern,linkedConfig->matching_pattern)){
                    //only do the copy if name has been replaced
                    struct arg_copy * args_linked =  malloc(sizeof(struct arg_copy));
                    if(!args_linked) {
                        LOGE("%s: args_linked malloc failed\n", __FUNCTION__);
                        return -1;
                    }
                    //time in seconds( should be less than phone doctor timer)
                    args_linked->time_val = 100 ;
                    snprintf(path_linked, sizeof(path_linked),"%s/%s",curConfig->path_linked,name_linked);
                    strncpy(args_linked->orig,path_linked,sizeof(args_linked->orig));
                    strncpy(args_linked->dest,destion,sizeof(args_linked->dest));
                    ret = pthread_create(&thread, NULL, (void *)copy_dir, (void *)args_linked);
                    if (ret < 0) {
                        LOGE("pthread_create copy linked dir error");
                        free(args_linked);
                        //if ret >=0 free is done inside copy_dir
                    }
                }
            }
        }
        //generating DATAREADY (if required)
        if (strstr(event_class , ERROREVENT)) {
            snprintf(fullpath, sizeof(fullpath)-1, "%s/%s_errorevent", destion,curConfig->eventname );
            generate_data = 1;
        }else if (strstr(event_class , INFOEVENT)) {
            snprintf(fullpath, sizeof(fullpath)-1, "%s/%s_infoevent", destion,curConfig->eventname );
            generate_data = 1;
        }else{
            //we don't need to generate data
            generate_data = 0;
        }

        if (generate_data == 1){
            fp = fopen(fullpath,"w");
            if (fp == NULL){
                LOGE("can not create file: %s\n", fullpath);
            }else{
                //Fill DATAREADY field
                fprintf(fp,"DATAREADY=%d\n", data_ready);
                fprintf(fp,"_END\n");
                fclose(fp);
                do_chown(fullpath, PERM_USER, PERM_GROUP);
            }
        }
    }
    key = raise_event_dataready(event_class, curConfig->eventname, NULL, destion, data_ready);
    LOGE("%-8s%-22s%-20s%s %s\n", event_class, key, get_current_time_long(0), curConfig->eventname, destion);
    //rmfr(path); //TO DO : define when path should be removed
    free(key);
    return 0;
}
