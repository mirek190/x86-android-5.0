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
 * @file crashutils.c
 * @brief File containing functions used to handle various operations on events.
 *
 * This file contains the functions used to handle various operations linked to
 * events such as :
 *  - general event raising (crash, bz...)
 *  - info event creation/raising
 *  - crashfile creation
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <openssl/sha.h>
#include <sys/wait.h>

#include <cutils/properties.h>
#ifndef __TEST__
#include <linux/android_alarm.h>
#endif

#include <crashutils.h>
#include <privconfig.h>
#include <history.h>
#include <fsutils.h>
#include <dropbox.h>
#include "uefivar.h"
#include "startupreason.h"
#include "ingredients.h"

#ifdef CONFIG_EFILINUX
#include <libuefivar.h>
#endif

char gbuildversion[PROPERTY_VALUE_MAX] = {0,};
char gboardversion[PROPERTY_VALUE_MAX] = {0,};
char guuid[256] = {0,};
int gabortcleansd = 0;

/**
 * @brief Returns the date/time under the input format.
 *
 * if refresh requested, returns current date/time.
 * if refresh not requested, returns previous computed time if available.
 *
 * @param[in] refresh : force date/time re-computing or not.
 * @param[in] format : specifies requested date/time format.
 * @retval           : the current system time under with specified formatting.
 */
const char *  __attribute__( ( noinline ) ) get_current_system_time(int refresh, enum time_format format) {

    int format_idx = 0;
    /* Defines different formats type used by crashlog */
    const struct { char *format; } date_time_format [] = {
            [ DATE_FORMAT_SHORT ] = { "%Y%m%d" },
            [ TIME_FORMAT_SHORT ] = { "%Y%m%d%H%M%S" },
            [ TIME_FORMAT_LONG ]  = { "%Y-%m-%d/%H:%M:%S  " },
    };
    /* Array containing the current date/time under different formats */
    static struct { char value[TIME_FORMAT_LENGTH]; } crashlog_time_array[] = {
            [ DATE_FORMAT_SHORT ] = { {0,} },
            [ TIME_FORMAT_SHORT ] = { {0,} },
            [ TIME_FORMAT_LONG ]  = { {0,} },
    };
    /* to NOT refresh and time array already initialized : returns value previously computed */
    if (!refresh && crashlog_time_array[format].value[0] != 0 )
        return crashlog_time_array[format].value;

    /* time array not initialized yet or to refresh : get system time and refresh current date/time array */
    time_t t;
    if (time(&t) == (time_t)-1 )
        LOGE("%s: Can't get current system time : use value previously got - error is %s", __FUNCTION__, strerror(errno));
    else {
        struct tm * time_val = localtime((const time_t *)&t);
        if (!time_val) {
            LOGE("%s: Could not retrieve the local time. Returning previously computed values.", __FUNCTION__);
        } else {
            for ( format_idx = 0 ; format_idx < (int)DIM(crashlog_time_array); format_idx++ ) {
                PRINT_TIME( crashlog_time_array[format_idx].value ,
                            date_time_format[format_idx].format ,
                            time_val);
            }
        }
    }
    return crashlog_time_array[format].value;
}

const char *get_current_time_short(int refresh) {
    return get_current_system_time( refresh, TIME_FORMAT_SHORT);
}
const char *get_current_date_short(int refresh) {
    return get_current_system_time( refresh, DATE_FORMAT_SHORT);
}
const char *get_current_time_long(int refresh) {
    return get_current_system_time( refresh, TIME_FORMAT_LONG);
}

unsigned long long get_uptime(int refresh, int *error)
{
    static long long time_ns = -1;
#ifndef __LINUX__
    struct timespec ts;
    int result;
    int fd;
#else
    struct timeval ts;
#endif
    *error = 0;

    if (!refresh && time_ns != -1) return time_ns;

#ifndef __LINUX__
    fd = open("/dev/alarm", O_RDONLY);
    if (fd <= 0) {
        *error = errno;
        LOGE("%s - Cannot open file: /dev/alarm: %s\n", __FUNCTION__,
            strerror(errno));
        return -1;
    }
    result = ioctl(fd,
        ANDROID_ALARM_GET_TIME(ANDROID_ALARM_ELAPSED_REALTIME), &ts);
    close(fd);
    if (result < 0) {
        *error = errno;
        LOGE("%s - Cannot ioctl /dev/alarm: %s\n", __FUNCTION__,
            strerror(errno));
        return -1;
    }
    time_ns = (((long long) ts.tv_sec) * 1000000000LL) + ((long long) ts.tv_nsec);
#else
#include <sys/time.h>
    gettimeofday(&ts, NULL);
    time_ns = (((long long) ts.tv_sec) * 1000000000LL) + ((long long) ts.tv_usec) * 1000LL;
#endif
    return time_ns;
}

void do_last_kmsg_copy(int dir) {
    char destion[PATHMAX];

    if ( file_exists(LAST_KMSG) ) {
        snprintf(destion, sizeof(destion), "%s%d/%s", CRASH_DIR, dir, LAST_KMSG_FILE);
        do_copy_tail(LAST_KMSG, destion, MAXFILESIZE);
    } else if ( file_exists(CONSOLE_RAMOOPS) ) {
        snprintf(destion, sizeof(destion), "%s%d/%s", CRASH_DIR, dir, CONSOLE_RAMOOPS_FILE);
        do_copy_tail(CONSOLE_RAMOOPS, destion, MAXFILESIZE);
    }
    if ( file_exists(FTRACE_RAMOOPS) ) {
        snprintf(destion, sizeof(destion), "%s%d/%s", CRASH_DIR, dir, FTRACE_RAMOOPS_FILE);
        do_copy_tail(FTRACE_RAMOOPS, destion, MAXFILESIZE);
    }
}

void do_last_fw_msg_copy(int dir) {
    char destion[PATHMAX];

    if ( (dir >= 0) && file_exists(CURRENT_PROC_OFFLINE_SCU_LOG_NAME) ) {
        snprintf(destion, sizeof(destion), "%s%d/%s.txt", CRASH_DIR, dir, OFFLINE_SCU_LOG_NAME);
        do_copy_eof(CURRENT_PROC_OFFLINE_SCU_LOG_NAME, destion);
    }
}

/* Compute key shall be checked only once at the first call to insure the
  memory allocation succeeded */
static int compute_key(char* key, char *event, char *type)
{
    static SHA_CTX *sha = NULL;
    char buf[256] = { '\0', };
    long long time_ns=0;
    char *tmp_key = key;
    unsigned char results[SHA_DIGEST_LENGTH];
    int i;

    if (sha == NULL) {
        sha = (SHA_CTX*)malloc(sizeof(SHA_CTX));
        if (sha == NULL) {
            LOGE("%s - Cannot create SHA_CTX memory... fails to compute the key!\n", __FUNCTION__);
            return -1;
        }
        SHA1_Init(sha);
    }

    if (!key || !event || !type) return -EINVAL;

    time_ns = get_uptime(1, &i);
    snprintf(buf, 256, "%s%s%s%s%lld", gbuildversion, guuid, event, type, time_ns);

    SHA1_Update(sha, (unsigned char*) buf, strlen(buf));
    SHA1_Final(results, sha);
    for (i = 0; i < SHA_DIGEST_LENGTH/2; i++)
    {
        sprintf(tmp_key, "%02x", results[i]);
        tmp_key+=2;
    }
    *tmp_key=0;
    return 0;
}

static void check_prop_modemid(){
    char prop[PROPERTY_VALUE_MAX];
    if(read_file_prop_uid(MODEM_FIELD, MODEM_UUID, prop, "unknown")>0)
        check_ingredients_file();
}

char **commachain_to_fixedarray(char *chain,
        unsigned int recordsize, unsigned int maxrecords, int *res) {
    char *curptr, *copy, *psave, **array;
    int idx;

    if (!chain || !recordsize) {
        *res = -EINVAL;
        return NULL;
    }

    *res = 0;
    if (!maxrecords) return NULL;

    /* First copy the chain because it gets modified by strtok_r */
    copy = strdup(chain);
    if (!copy) {
        *res = -errno;
        return NULL;
    }
    curptr = strtok_r(copy, ";", &psave);
    if (!curptr) {
        free(copy);
        return NULL;
    }

    array = (char**)malloc(maxrecords*sizeof(char*));
    if (!array) {
        *res = -errno;
        LOGE("%s: Cannot allocate %d bytes of temporary memory - %s\n",
            __FUNCTION__, maxrecords*(int)sizeof(char*), strerror(errno));
        free(copy);
        return NULL;
    }
    for (idx = 0 ; (unsigned int)idx < maxrecords ; idx++) {
        array[idx] = (char*)malloc(recordsize*sizeof(char));
        if (!array[idx]) {
            *res = -errno;
            LOGE("%s: Cannot allocate %d bytes of temporary memory to store an aplogs pattern - %s\n",
                __FUNCTION__, recordsize*(int)sizeof(char), strerror(errno));
            for (--idx ; idx >= 0 ; idx--) {
                free(array[idx]);
            }
            free(array);
            free(copy);
            return NULL;
        }
    }

    /* Do the job, finally!! */
    for (idx = 0 ; curptr && (unsigned int)idx < maxrecords ; idx++) {
        strncpy(array[*res], curptr, recordsize-1);
        array[*res][recordsize-1] = 0;
        curptr = strtok_r(NULL, ";", &psave);
        (*res)++;
    }
    /* returns maxrecords + 1 if the chain tokens exceeded
        the array capacity */
    if (curptr != NULL) (*res)++;

    free(copy);
    return array;
}

static const char *get_imei() {
    static char imei[PROPERTY_VALUE_MAX] = { 0, };

    if (imei[0] != 0) return imei;

    property_get(IMEI_FIELD, imei, "");
    return imei;
}

static const char *get_operator() {
    static char operator[PROPERTY_VALUE_MAX] = { 0, };
    property_get(OPERATOR_FIELD, operator, "UNKNOWN");
    return operator;
}

//This function creates a minimal crashfile (DATA0, DATA1 and DATA2 fields)
//Note: for Modem Panic case, DATA are set via file parsing adn parameters
//  Data0,1,2 are ignored
static int create_minimal_crashfile(char * event, const char* type, const char* path, char* key,
      const char* uptime, const char* date, int data_ready, char* data0, char* data1, char* data2)
{
    FILE *fp;
    char fullpath[PATHMAX];
    char mpanicpath[PATHMAX];

    snprintf(fullpath, sizeof(fullpath)-1, "%s/%s", path, CRASHFILE_NAME);

    //Create crashfile
    errno = 0;
    fp = fopen(fullpath,"w");
    if (fp == NULL)
    {
        LOGE("%s: Cannot create %s - %s\n", __FUNCTION__, fullpath, strerror(errno));
        return -errno;
    }
    fclose(fp);
    do_chown(fullpath, PERM_USER, PERM_GROUP);

    fp = fopen(fullpath,"w");
    if (fp == NULL)
    {
        LOGE("%s: can not open file: %s\n", __FUNCTION__, fullpath);
        return -errno;
    }

    //Fill crashfile
    fprintf(fp,"EVENT=%s\n", event);
    fprintf(fp,"ID=%s\n", key);
    fprintf(fp,"SN=%s\n", guuid);
    fprintf(fp,"DATE=%s\n", date);
    fprintf(fp,"UPTIME=%s\n", uptime);
    fprintf(fp,"BUILD=%s\n", get_build_footprint());
    fprintf(fp,"BOARD=%s\n", gboardversion);
    fprintf(fp,"IMEI=%s\n", get_imei());
    fprintf(fp,"TYPE=%s\n", type);
    fprintf(fp,"DATA_READY=%d\n", data_ready);
    fprintf(fp,"OPERATOR=%s\n", get_operator());
    //MPANIC crash : fill DATA field and preempt data012 mechanism
    if (!strcmp(MDMCRASH_EVNAME, type)){
        LOGI("Modem panic detected : generating DATA0\n");
        FILE *fd_panic;
        DIR *d;
        struct dirent* de;
        d = opendir(path);
        if(!d) {
            LOGE("%s: Can't open dir %s\n",__FUNCTION__, path);
            fclose(fp);
            return -1;
        }
        while ((de = readdir(d))) {
            const char *name = de->d_name;
            int ismpanic, iscrashdata = 0;
            ismpanic = (strstr(name, "mpanic") != NULL);
            if (!ismpanic) iscrashdata = (strstr(name, "_crashdata") != NULL);
            if (ismpanic || iscrashdata) {
                snprintf(mpanicpath, sizeof(mpanicpath)-1, "%s/%s", path, name);
                fd_panic = fopen(mpanicpath, "r");
                if (fd_panic == NULL){
                    LOGE("%s: can not open file: %s\n", __FUNCTION__, mpanicpath);
                    break;
                }
                char value[PATHMAX] = "";
                if (ismpanic) {
                    fscanf(fd_panic, "%s", value);
                    fprintf(fp,"DATA0=%s\n", value);
                }
                else { // iscrashdata
                    while (fgets(value,sizeof(value),fd_panic) && !strstr(value,"_END"))
                        fprintf(fp,"%s\n", value);
                }
                fclose(fd_panic);
                break;
            }
        }
        closedir(d);
    } else {
        if (data0)
            fprintf(fp,"DATA0=%s\n", data0);
        if (data1)
            fprintf(fp,"DATA1=%s\n", data1);
        if (data2)
            fprintf(fp,"DATA2=%s\n", data2);
    }
    fprintf(fp,"_END\n");
    fclose(fp);
    return 0;
}

static char *priv_raise_event(char *event, char *type, char *subtype, char *log,
        int add_uptime, int data_ready, char* data0, char* data1, char* data2) {
    struct history_entry entry;
    char key[SHA_DIGEST_LENGTH+1];
    char newuptime[32], *puptime = NULL;
    char lastbootuptime[24];
    int res, hours;
    const char *datelong = get_current_time_long(1);

    //check property of modemid at each event
    check_prop_modemid();


    /* UPTIME      : event uptime value is get from system
     * UPTIME_BOOT : event uptime value get from history file first line
     * NO_UPTIME   : no event uptime value */
    switch(add_uptime) {
    case UPTIME :
        puptime = &newuptime[0];
        if ((res = get_uptime_string(newuptime, &hours)) < 0) {
            LOGE("%s failed: Cannot get the uptime - %s\n",
                __FUNCTION__, strerror(-res));
            return NULL;
        }
        break;
    case UPTIME_BOOT :
        if (!get_lastboot_uptime(lastbootuptime))
            puptime = &lastbootuptime[0];
        break;
    default : /* NO_UPTIME */
        break;
    }

    if (!event) {
        /* event is not set, then use type/subtype */
        event = type;
        type = subtype;
    }
    if (!subtype) subtype = type;
    compute_key(key, event, type);

    entry.event = event;
    entry.type = type;
    entry.log = log;
    entry.lastuptime = puptime;
    entry.key = key;
    entry.eventtime = (char*)datelong;

    errno = 0;
    if ((res = update_history_file(&entry)) != 0) {
        LOGE("%s: Cannot update the history file (%s), drop the event \"%-8s%-8s%-22s%-20s%s\"\n",
            __FUNCTION__, strerror(-res), event, type, key, datelong, type);
        errno = -res;
        return NULL;
    }
    if (log ) {
        if (puptime == NULL) puptime = "\0";
        if (!strncmp(event, CRASHEVENT, sizeof(CRASHEVENT))) {
            res = create_minimal_crashfile( event, subtype, log, key, puptime,
                                    datelong, data_ready, data0, data1, data2);
        }
        else if(!strncmp(event, BZEVENT, sizeof(BZEVENT))) {
            res = create_minimal_crashfile( event, BZMANUAL, log, key, puptime,
                                    datelong, data_ready, data0, data1, data2);
        }
        else if(!strncmp(event, INFOEVENT, sizeof(INFOEVENT)) && !strncmp(type, "FIRMWARE", sizeof("FIRMWARE"))) {
            res = create_minimal_crashfile( event, type, log, key, puptime,
                                datelong, data_ready, data0, data1, data2);
        }
        if ( res != 0 ) {
            LOGE("%s: Cannot create a minimal crashfile in %s - %s.\n", __FUNCTION__,
                entry.log, strerror(-res));
            errno = -res;
            return NULL;
        }
    }

    if (!strncmp(event, SYS_REBOOT, sizeof(SYS_REBOOT))) {
        res = reboot_reason_files_present();
        create_rebootfile(key, res);
    }

    /* Notify CrashReport except for UIWDT events */
    if (strncmp(type, SYSSERVER_EVNAME, sizeof(SYSSERVER_EVNAME))) {
        notify_crashreport();
    }
    return strdup(key);
}

char *raise_event_nouptime(char *event, char *type, char *subtype, char *log) {
    return priv_raise_event(event, type, subtype, log, NO_UPTIME, 1, NULL , NULL, NULL);
}

char *raise_event_wdt(char *event, char *type, char *subtype, char *log) {
    return priv_raise_event(event, type, NULL, log, UPTIME, 1, subtype, NULL, NULL);
}

char *raise_event(char *event, char *type, char *subtype, char *log) {
    return priv_raise_event(event, type, subtype, log, UPTIME, 1, NULL, NULL, NULL);
}

char *raise_event_bootuptime(char *event, char *type, char *subtype, char *log) {
    return priv_raise_event(event, type, subtype, log, UPTIME_BOOT, 1, NULL , NULL, NULL);
}

char *raise_event_dataready(char *event, char *type, char *subtype, char *log, int data_ready) {
    return priv_raise_event(event, type, subtype, log, UPTIME, data_ready, NULL , NULL, NULL);
}

char *raise_event_dataready012(char *event, char *type, char *subtype, char *log, int data_ready,
                                                        char* data0, char* data1, char* data2) {
    return priv_raise_event(event, type, subtype, log, UPTIME, data_ready, data0 , data1, data2);
}

void restart_profile_srv(int serveridx) {
    char value[PROPERTY_VALUE_MAX];
    char expected;
    char *profile_srv;

    switch (serveridx) {
        case 1:
            profile_srv = "profile1_rest";
            expected = '1';
            break;
        case 2:
            profile_srv = "profile2_rest";
            expected = '2';
            break;
        default: return;
    }

    if (property_get(PROP_PROFILE, value, NULL) <= 0) return;
    if ( value[0] == expected )
        start_daemon(profile_srv);
}

int check_running_modem_trace() {
    char modemtrace[PROPERTY_VALUE_MAX];

    property_get("init.svc.mtsp", modemtrace, "");
    if (!strncmp(modemtrace, "running", 7))
        return 1;
    return 0;
}

void check_running_power_service() {
#ifdef FULL_REPORT
    char powerservice[PROPERTY_VALUE_MAX];
    char powerenable[PROPERTY_VALUE_MAX];

    property_get("init.svc.profile_power", powerservice, "");
    property_get("persist.service.power.enable", powerenable, "");
    if (strcmp(powerservice, "running") && !strcmp(powerenable, "1")) {
        LOGE("power service stopped whereas property is set .. restarting\n");
        start_daemon("profile_power");
    }
#endif
}

int get_build_board_versions(char *filename, char *buildver, char *boardver) {
    FILE *fd;
    char buffer[MAXLINESIZE];
    char bldpattern[MAXLINESIZE], brdpattern[MAXLINESIZE];
    int bldfieldlen, brdfieldlen;

    buildver[0] = 0;
    boardver[0] = 0;
    property_get(PROP_BUILD_FIELD, buildver, "");
    property_get(PROP_BOARD_FIELD, boardver, "");
    if (buildver[0] != 0 && boardver[0] != 0) return 0;

    if ((fd = fopen(filename, "r")) == NULL) {
        LOGE("%s - Cannot read %s - %s\n", __FUNCTION__, filename, strerror(errno));
        return -errno;
    }

    /* build the patterns */
    bldfieldlen = strlen(PROP_BUILD_FIELD);
    brdfieldlen = strlen(PROP_BOARD_FIELD);
    memcpy(bldpattern, PROP_BUILD_FIELD, bldfieldlen);
    memcpy(brdpattern, PROP_BOARD_FIELD, brdfieldlen);
    bldpattern[bldfieldlen+3] = '0';bldpattern[bldfieldlen+2] = 's';bldpattern[bldfieldlen+1] = '%';bldpattern[bldfieldlen] = '=';
    brdpattern[brdfieldlen+3] = '0';brdpattern[brdfieldlen+2] = 's';brdpattern[brdfieldlen+1] = '%';brdpattern[brdfieldlen] = '=';

    /* read the file */
    while (freadline(fd, buffer) > 0) {
        if (buildver[0] == 0)
            sscanf(buffer, bldpattern, buildver);
        if (boardver[0] == 0)
            sscanf(buffer, brdpattern, boardver);
        if (buildver[0] != 0 && boardver[0] != 0) break;
    }
    fclose(fd);
    return ((buildver[0] != 0 && boardver[0] != 0) ? 0 : -1);
}

/**
 * @brief This function is only an interface of the process_info_and_error function
 * called as a callback by the inotify-watcher mechanism when a info-error
 * file is triggered.
 *
 * @param[in] entry : watcher entry linked to the inotify event
 * @param[in] event : the initial inotify event
 */
int process_info_and_error_inotify_callback(struct watch_entry *entry, struct inotify_event *event) {

    if ( event->len )
        return process_info_and_error(entry->eventpath, event->name);
    else
        return -1; /* Robustness */
}

/*
* Name          : process_info_and_error
* Description   : This function manages treatment of error and info
*                 When event or error trigger file is detected, it copies data and trigger files
* Parameters    :
*   char *filename        -> path of watched directory/file
*   char *name            -> name of the file inside the watched directory that has triggered the event
*/
int process_info_and_error(char *filename, char *name) {
    int dir;
    char path[PATHMAX];
    char destion[PATHMAX];
    unsigned int j = 0;
    char *p;
    char tmp[32];
    char name_event[10];
    char file_ext[20];
    char type[20] = { '\0', };
    char tmp_data_name[PATHMAX];
    char *key;

    if (strstr(name, "_infoevent" )){
        snprintf(name_event,sizeof(name_event),"%s",INFOEVENT);
        snprintf(file_ext,sizeof(file_ext),"%s","_infoevent");
    }else if (strstr(name, "_errorevent" )){
        snprintf(name_event,sizeof(name_event),"%s",ERROREVENT);
        snprintf(file_ext,sizeof(file_ext),"%s","_errorevent");
    }else{ /*Robustness*/
        LOGE("%s: Can't handle input file\n", __FUNCTION__);
        return -1;
    }
    snprintf(tmp,sizeof(tmp),"%s",name);

    dir = find_new_crashlog_dir(MODE_STATS);
    if (dir < 0) {
        LOGE("%s: Cannot get a valid new crash directory...\n", __FUNCTION__);
        p = strstr(tmp,"trigger");
        if ( p ){
            strcpy(p,"data");
        }
        key = raise_event(name_event, tmp, NULL, NULL);
        LOGE("%-8s%-22s%-20s%s\n", name_event, key, get_current_time_long(0), tmp);
        free(key);
        return -1;
    }
    /*copy data file*/
    p = strstr(tmp,file_ext);
    if ( p ){
        strcpy(p,"_data");
        find_matching_file(filename,tmp, tmp_data_name);
        snprintf(path, sizeof(path),"%s/%s", filename,tmp_data_name);
        snprintf(destion,sizeof(destion),"%s%d/%s", STATS_DIR, dir, tmp_data_name);
        do_copy_tail(path, destion, 0);
        remove(path);
    }
    /*copy trigger file*/
    snprintf(path, sizeof(path),"%s/%s", filename,name);
    snprintf(destion,sizeof(destion),"%s%d/%s", STATS_DIR,dir,name);
    do_copy_tail(path, destion, 0);
    remove(path);
    snprintf(destion,sizeof(destion),"%s%d/", STATS_DIR,dir);
    /*create type */
    snprintf(tmp,sizeof(tmp),"%s",name);
    /*Set to upper case*/
    p = strstr(tmp,file_ext);
    if (p) {
        for (j=0;j<sizeof(type)-1;j++) {
            if (p == (tmp+j))
                break;
            type[j]=toupper(tmp[j]);
        }
    } else
        snprintf(type,sizeof(type),"%s",name);
    key = raise_event(name_event, type, NULL, destion);
    LOGE("%-8s%-22s%-20s%s %s\n", name_event, key, get_current_time_long(0), type, destion);
    free(key);
    return 0;
}

/**
 * @brief Generates an INFO event from an input trigger file and input data
 *
 * This function generates an INFO event by creating an infoevent file (instead of
 * using classical watcher mechanism) and filling it with input data (data0, data1 and data2)
 *
 * @param[in] filename : name of the infoevent file to create (shall contains "_infoevent" pattern)
 * @param[in] data0 : string set to DATA0 field in the infoevent file created
 * @param[in] data1 : string set to DATA1 field in the infoevent file created
 * @param[in] data2 : string set to DATA2 field in the infoevent file created
 */
void create_infoevent(char* filename, char* data0, char* data1, char* data2)
{
    FILE *fp;
    char fullpath[PATHMAX];

    snprintf(fullpath, sizeof(fullpath)-1, "%s/%s", LOGS_DIR, filename);

    fp = fopen(fullpath,"w");
    if (fp == NULL)
    {
        LOGE("can not create file1: %s\n", fullpath);
        return;
    }
    //Fill DATA fields
    if (data0 != NULL)
        fprintf(fp,"DATA0=%s\n", data0);
    if (data1 != NULL)
        fprintf(fp,"DATA1=%s\n", data1);
    if (data2 != NULL)
        fprintf(fp,"DATA2=%s\n", data2);
    fprintf(fp,"_END\n");
    fclose(fp);
    do_chown(fullpath, PERM_USER, PERM_GROUP);

    process_info_and_error(LOGS_DIR, filename);
}

const char *get_build_footprint() {
    static char footprint[SIZE_FOOTPRINT_MAX+1] = {0,};
    char prop[PROPERTY_VALUE_MAX];
    int mdmIdUpdate;

    /* footprint contains:
     * buildId
     * fingerPrint
     * kernelVersion
     * buildUserHostname
     * modemVersion
     * ifwiVersion
     * iafwVersion
     * scufwVersion
     * punitVersion
     * valhooksVersion */
    if (footprint[0] != 0) return footprint;

    snprintf(footprint, SIZE_FOOTPRINT_MAX, "%s,", gbuildversion);

    property_get(FINGERPRINT_FIELD, prop, "");
    strncat(footprint, prop, SIZE_FOOTPRINT_MAX);
    strncat(footprint, ",", SIZE_FOOTPRINT_MAX);

    property_get(KERNEL_FIELD, prop, "");
    strncat(footprint, prop, SIZE_FOOTPRINT_MAX);
    strncat(footprint, ",", SIZE_FOOTPRINT_MAX);

    property_get(USER_FIELD, prop, "");
    strncat(footprint, prop, SIZE_FOOTPRINT_MAX);
    strncat(footprint, "@", SIZE_FOOTPRINT_MAX);

    property_get(HOST_FIELD, prop, "");
    strncat(footprint, prop, SIZE_FOOTPRINT_MAX);
    strncat(footprint, ",", SIZE_FOOTPRINT_MAX);

    mdmIdUpdate = read_file_prop_uid(MODEM_FIELD, MODEM_UUID, prop, "unknown");
    strncat(footprint, prop, SIZE_FOOTPRINT_MAX);
    strncat(footprint, ",", SIZE_FOOTPRINT_MAX);
    if(mdmIdUpdate>0)
    {
        check_ingredients_file();
    }

    property_get(IFWI_FIELD, prop, "");
    strncat(footprint, prop, SIZE_FOOTPRINT_MAX);
    strncat(footprint, ",", SIZE_FOOTPRINT_MAX);

    property_get(IAFW_VERSION, prop, "");
    strncat(footprint, prop, SIZE_FOOTPRINT_MAX);
    strncat(footprint, ",", SIZE_FOOTPRINT_MAX);

    property_get(SCUFW_VERSION, prop, "");
    strncat(footprint, prop, SIZE_FOOTPRINT_MAX);
    strncat(footprint, ",", SIZE_FOOTPRINT_MAX);

    property_get(PUNIT_VERSION, prop, "");
    strncat(footprint, prop, SIZE_FOOTPRINT_MAX);
    strncat(footprint, ",", SIZE_FOOTPRINT_MAX);

    property_get(VALHOOKS_VERSION, prop, "");
    strncat(footprint, prop, SIZE_FOOTPRINT_MAX);
    return footprint;
}


void start_daemon(const char *daemonname) {
    property_set("ctl.start", (char *)daemonname);
}

void notify_crashreport_thread() {
    char boot_state[PROPERTY_VALUE_MAX];

    /* Does current crashlog mode allow notifs to crashreport ?*/
    if ( !CRASHLOG_MODE_NOTIFS_ENABLED(g_crashlog_mode) ) {
        LOGD("%s : Current crashlog mode is %s - crashreport notifs disabled.\n", __FUNCTION__, CRASHLOG_MODE_NAME(g_crashlog_mode) );
        return;
    }
    property_get(PROP_BOOT_STATUS, boot_state, "-1");
    if (strcmp(boot_state, "1"))
        return;

    int status = system("am broadcast -n com.intel.crashreport/.specific.NotificationReceiver -a com.intel.crashreport.intent.CRASH_NOTIFY -c android.intent.category.ALTERNATIVE");
    if (status == -1) {
        LOGI("notify crash report failed: fork\n");
        return;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status)) {
        LOGI("notify crash report status: %d.\n", WEXITSTATUS(status));
        return;
    }
}

void notify_crashreport() {
    int ret = 0;
    pthread_t thread;

    ret = pthread_create(&thread, NULL, (void *)notify_crashreport_thread, NULL);
    if (ret < 0) {
        LOGE("%s: notify_crashreport thread error", __FUNCTION__);
    }
}

/**
 * @brief Notify partition error
 *
 * @return Notify partition error : 1 if successfull
 */
int notify_partition_error(enum partition_error type) {
    char boot_state[PROPERTY_VALUE_MAX];
    int status = 0;

    property_get(PROP_BOOT_STATUS, boot_state, "-1");
    if (strcmp(boot_state, "1"))
        return 0;

    switch(type) {
        case ERROR_LOGS_MISSING :
             status = system("am broadcast -a intel.intent.action.phonedoctor.REPORT_ERROR --es \"intel.intent.extra.phonedoctor.TYPE\" \"PARTITION\" --es \"intel.intent.extra.phonedoctor.DATA0\" \"LOGS MISSING\"");
             break;
        case ERROR_LOGS_RO :
             status = system("am broadcast -a intel.intent.action.phonedoctor.REPORT_ERROR --es \"intel.intent.extra.phonedoctor.TYPE\" \"PARTITION\" --es \"intel.intent.extra.phonedoctor.DATA0\" \"LOGS READ ONLY\"");
             break;
        case ERROR_PARTITIONS_MISSING :
             status = system("am broadcast -a intel.intent.action.phonedoctor.REPORT_ERROR --es \"intel.intent.extra.phonedoctor.TYPE\" \"PARTITION\" --es \"intel.intent.extra.phonedoctor.DATA0\" \"PARTITION MISSING\"");
             break;
        default :
             break;
    }
    if (status == -1) {
        LOGI("notify partition error failed: fork\n");
        return 0;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status)) {
        LOGI("notify partition error status: %d.\n", WEXITSTATUS(status));
        return 0;
    }
    return 1;
}

/**
 * @brief Builds the string containing arguments to pass
 * to monitor_crashenv script.
 *
 * @param crashenv_param : string containing args to pass to crashenv
 */
void build_crashenv_parameters( char * crashenv_param ) {

    int idx = 0;
    char param[PATHMAX] = { 0, };

    /* Checks if some directories need to be listed */
    if ( get_missing_watched_dir_nb() ) {
        build_crashenv_dir_list_option( param );
        strncpy( crashenv_param, param , PATHMAX );
    }
    /* Others arguments to be added...*/

}

/**
 * @brief Performs a call to monitor_crashenv shell script.
 */
void monitor_crashenv()
{
    char cmd[PATHMAX] = { 0, }, parameters[PATHMAX] = { 0, };

    /* Does current crashlog mode allow monitor_crash_env ?*/
    if ( !CRASHLOG_MODE_MONITOR_CRASHENV(g_crashlog_mode) ) {
        LOGD("%s : Current crashlog mode is %s - monitor_crashenv disabled.\n", __FUNCTION__, CRASHLOG_MODE_NAME(g_crashlog_mode));
        return;
    }

    build_crashenv_parameters(parameters);
    snprintf(cmd, sizeof(cmd), "/system/bin/monitor_crashenv %s", parameters);
    LOGD("%s: execute %s ", __FUNCTION__, cmd);
    int status = system(cmd);
    if (status != 0)
        LOGE("monitor_crashenv status: %d.\n", status);
    return ;
}

int raise_infoerror(char *type, char *subtype) {
    char *key;
    int status;

    if ((key = raise_event(NULL, type, subtype, LOGINFO_DIR)) == NULL)
        return -errno;
    LOGE("%-8s%-22s%-20s%s\n", type, key, get_current_time_long(0), subtype);
    free(key);
    unlink(LOGRESERVED);
#ifdef FULL_REPORT
    monitor_crashenv();
#endif
    return 0;
}

/*
 * Name         :   check_crashlog_dead
 * Description  :   Get the token property and had a 1 at the end.
 *                  When at least 2 ones are read, reports that an
 *                  error happenned in crashtool until 4 ones are
 *                  read, then gets silent and stop adding ones
 */
void check_crashlog_died()
{
    char token[PROPERTY_VALUE_MAX];
    property_get("crashlogd.token", token, "");
    if ((strlen(token) < 4)) {
         strcat(token, "1");
         property_set("crashlogd.token", token);
         if (!strncmp(token, "11", 2))
             raise_infoerror(ERROREVENT, CRASHLOG_ERROR_DEAD);
    }
}

int do_screenshot_copy(char* bz_description, char* bzdir) {
    char buffer[PATHMAX];
    char screenshot[PATHMAX];
    char destion[PATHMAX];
    FILE *fd1;
    struct stat info;
    int bz_num = 0;
    int screenshot_len;

    if (stat(bz_description, &info) < 0)
        return -1;

    fd1 = fopen(bz_description,"r");
    if(fd1 == NULL){
        LOGE("%s: can not open file: %s\n", __FUNCTION__, bz_description);
        return -1;
    }

    while(!feof(fd1)){
        if (fgets(buffer, sizeof(buffer), fd1) != NULL){
            if (strstr(buffer,SCREENSHOT_PATTERN)){
                //Compute length of screenshot path
                screenshot_len = strlen(buffer) - strlen(SCREENSHOT_PATTERN);
                if ((screenshot_len > 0) && ((unsigned int)screenshot_len < sizeof(screenshot))) {
                    //Copy file path
                    strncpy(screenshot, buffer+strlen(SCREENSHOT_PATTERN), screenshot_len);
                    //If last character is '\n' replace it by '\0'
                    if ((screenshot[screenshot_len-1]) == '\n')
                        screenshot_len--;
                    screenshot[screenshot_len]= '\0';

                    if(bz_num == 0)
                        snprintf(destion,sizeof(destion),"%s/bz_screenshot.png", bzdir);
                    else
                        snprintf(destion,sizeof(destion),"%s/bz_screenshot%d.png", bzdir, bz_num);
                    bz_num++;
                    do_copy_tail(screenshot,destion,0);
                }
            }
        }
    }

    if (fd1 != NULL)
        fclose(fd1);

    return 0;
}

/*
* Name          : clean_crashlog_in_sd
* Description   : clean legacy crash log folder
* Parameters    :
*   char *dir_to_search       -> name of the folder
*   int max                   -> max number of folder to remove
*/
void clean_crashlog_in_sd(char *dir_to_search, int max) {
    char path[PATHMAX];
    DIR *d;
    struct dirent* de;
    int i = 0;

    d = opendir(dir_to_search);
    if (d) {
        while ((de = readdir(d))) {
            const char *name = de->d_name;
            snprintf(path, sizeof(path)-1, "%s/%s", dir_to_search, name);
            if ( (strstr(path, SDCARD_CRASH_DIR) ||
                 (strstr(path, SDCARD_STATS_DIR)) ||
                 (strstr(path, SDCARD_APLOGS_DIR))) )
                /* If current path is not written in the history file, it's a legacy folder to remove */
                if (!history_has_event(path)) {
                    LOGD("%s : remove legacy crash folder %s", __FUNCTION__, path);
                    if  (rmfr(path) < 0)
                        LOGE("%s: failed to remove folder %s", __FUNCTION__, path);
                    i++;
                    if (i >= max)
                       break;
                }
            }
            closedir(d);
    }
    // abort clean of legacy log folder when there is no more folders expect the ones in history_event
    gabortcleansd = (i < max);
}

#ifdef CONFIG_EFILINUX
struct {
    char * name;
    char * description;
} target_os_boot_array[] = {
    {"", "N/A"},
    {"main", "MOS"},
    {"fastboot", "POS"},
    {"recovery", "ROS"},
    {"charging", "COS"}
};

static void trim_wchar_to_char(char *src, char * dest, int length) {
    int src_count, dest_count;
    for (src_count = 0, dest_count = 0; src_count < length; dest_count++, src_count += 2) {
        dest[dest_count] = (char) src[src_count];
    }
}

static char * check_os_boot_entry_description(const char* name, const char *guid){
    const int max_len = 1024;
    char target_mode[max_len];
    int ret = 0;

    int entries = sizeof(target_os_boot_array) / sizeof(target_os_boot_array[0]);

    if ((ret = libuefivar_read_string(name, guid, target_mode, max_len)<=0)) {
        LOGE("%s: Could not retrieve last target mode name or empty, returned: %d", __FUNCTION__, ret);
        return target_os_boot_array[0].description;
    }
    trim_wchar_to_char(target_mode, target_mode, max_len);

    while (entries--) {
        if (!strcmp(target_mode, target_os_boot_array[entries].name))
            break;
    }
    entries = (entries<0) ? 0 : entries;
    return target_os_boot_array[entries].description;
}

static char * compute_bootmode(char* retVal, int len){
    const char *last_target_mode_name = "LoaderEntryLast";
    const char *previous_target_mode_name = "LoadEntryPrevious";
    const char *guid = "4a67b082-0a4c-41cf-b6c7-440b29bb8c4f";

    snprintf(retVal, len, "%s-%s",
             check_os_boot_entry_description(previous_target_mode_name, guid),
             check_os_boot_entry_description(last_target_mode_name, guid));

    return retVal;
}
#endif

//This function creates a reboot file(DATA0/1 set to RESETSRC0/1).
int create_rebootfile(char* key, int data_ready)
{
    FILE *fp;
    char fullpath[PATHMAX];
    int ret = 0;
    const int bootmode_len = 16;
    char bootmode[bootmode_len];

    if (!file_exists(EVENTS_DIR)) {
        /* Create a fresh directory */
        errno = 0;
        if (mkdir(EVENTS_DIR, 0777) == -1) {
            LOGE("%s: Cannot create dir %s %s\n", __FUNCTION__, EVENTS_DIR, strerror(errno));
            raise_infoerror(ERROREVENT, EVENTS_DIR);
            return -errno;
        }
        do_chown(EVENTS_DIR, PERM_USER, PERM_GROUP);
    }
    snprintf(fullpath, sizeof(fullpath)-1, "%s/%s_%s", EVENTS_DIR, EVENTFILE_NAME, key);

    //Create crashfile
    errno = 0;
    fp = fopen(fullpath,"w");
    if (fp == NULL) {
        LOGE("%s: Cannot create %s - %s\n", __FUNCTION__, fullpath, strerror(errno));
        return -errno;
    }
    fclose(fp);
    do_chown(fullpath, PERM_USER, PERM_GROUP);

    errno = 0;
    fp = fopen(fullpath,"w");
    if (fp == NULL) {
        LOGE("%s: can not open file: %s %s\n", __FUNCTION__, fullpath, strerror(errno));
        return -errno;
    }

    fprintf(fp,"DATA_READY=%d\n", data_ready);

#ifdef CONFIG_EFILINUX
    //generating data0, data1 for edk platforms
    char resetsource[32] = { '\0', };
    char resettype[32] = { '\0', };
    char wakesource[32] = { '\0', };
    char shutdownsource[32] = { '\0', };

    ret = read_resetsource(resetsource);
    if (ret > 0) {
        fprintf(fp,"DATA0=%s\n", resetsource);
    }

    ret = read_resettype(resettype);
    if (ret > 0) {
        fprintf(fp,"DATA1=%s\n", resettype);
    }

    ret = read_wakesource(wakesource);
    if (ret > 0) {
        fprintf(fp,"DATA2=%s\n", wakesource);
    }

    ret = read_shutdownsource(shutdownsource);
    if (ret > 0) {
        fprintf(fp,"DATA3=%s\n", shutdownsource);
    }

    fprintf(fp,"BOOTMODE=%s\n", compute_bootmode(bootmode, bootmode_len));
#endif

#ifdef CONFIG_FDK
    //generating data0, data1 for other platforms
    if (data_ready) {
        LOGI("reset source detected : generating DATA0\n");
        char tmp[PATHMAX] = "";
        if(file_exists(RESET_SOURCE_0))
            snprintf(tmp, sizeof(tmp), RESET_SOURCE_0);
        else if(file_exists(RESET_IRQ_1))
            snprintf(tmp, sizeof(tmp), RESET_IRQ_1);
        get_data_from_boot_file(tmp, "DATA3", fp);

        tmp[0] = '\0';
        if(file_exists(RESET_SOURCE_1))
            snprintf(tmp, sizeof(tmp), RESET_SOURCE_1);
        else if(file_exists(RESET_IRQ_2))
            snprintf(tmp, sizeof(tmp), RESET_IRQ_2);
        get_data_from_boot_file(tmp,"DATA4", fp);
    }
#endif

    fprintf(fp,"_END\n");
    fclose(fp);
    return 0;
}

/**
 * Return 0 if reboot reason files (RESETSRC0/1 or RESETIRQ1/2) are present
 */
int reboot_reason_files_present() {
    if((file_exists(RESET_SOURCE_0) && file_exists(RESET_SOURCE_1)) || (file_exists(RESET_IRQ_1) && file_exists(RESET_IRQ_2)))
        return 1;
    else return 0;

}

void get_data_from_boot_file(char *file, char* data, FILE* fp) {
    char value[PATHMAX] = "";
    FILE *fd_source = fopen(file, "r");
    errno = 0;
    if (fd_source == NULL){
        LOGE("%s: can not open file: %s %s\n", __FUNCTION__, file, strerror(errno));
    }
    else {
        if(fgets(value,sizeof(value),fd_source)) {
            int size = strlen(value);
            if(value[size-1] == '\n')
                value[size-1] = '\0';
            fprintf(fp,"%s=%s\n", data, value);
        }
        fclose(fd_source);
    }
}

void do_wdt_log_copy(int dir) {
    const char *dateshort = get_current_time_short(1);
    char destination[PATHMAX] = "";

    destination[0] = '\0';
    snprintf(destination, sizeof(destination), "%s%d/", CRASH_DIR, dir);
    flush_aplog(APLOG_BOOT, "WDT", &dir, dateshort);
    usleep(TIMEOUT_VALUE);
    do_log_copy("WDT", dir, dateshort, APLOG_TYPE);
    do_last_fw_msg_copy(dir);
}

int get_cmdline_bootreason(char *bootreason) {
    FILE *fd;
    int res;
    char *p, *p1;
    char cmdline[1024] = { '\0', };
    const char key[] = "bootreason=";

    fd = fopen(CURRENT_KERNEL_CMDLINE, "r");
    if (!fd) {
        LOGE("%s: failed to open file %s - %s\n", __FUNCTION__,
             CURRENT_KERNEL_CMDLINE, strerror(errno));
        return -1;
    }

    res = fread(cmdline, 1, sizeof(cmdline)-1, fd);
    if (res <= 0) {
        LOGE("%s: failed to read file %s - %s\n", __FUNCTION__,
             CURRENT_KERNEL_CMDLINE, strerror(errno));
        return -1;
    }
    fclose(fd);

    p = strstr(cmdline, key);
    if (!p)
        return 0;
    p += strlen(key);
    p1 = strstr(p, " ");
    if (p1)
        *p1 = '\0';

    strncpy(bootreason, p, strlen(p) + 1);
    return strlen(bootreason);
}
