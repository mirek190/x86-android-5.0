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
 * @file mmgr_source.c
 * @brief File containing functions to handle modem manager source and events
 * coming from this source.
 *
 * This file contains the functions to handle the modem manager source (init,
 * closure, events reading from socket) and the functions to process the
 * different kinds of events read from this source.
 */

#include "mmgr_source.h"
#include "crashutils.h"
#include "privconfig.h"
#include "fsutils.h"
#include "log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <cutils/properties.h>

/* private structure */
struct mmgr_data {
    char string[MMGRMAXSTRING];     /* main string representing mmgr data content */
    int  extra_int;                 /* optional integer data (mailly used for error code) */
    char extra_string[MMGRMAXEXTRA];/* optional string that could be used for any purpose */
    int  extra_nb_causes;                 /* optional integer data that could be used to store number of optionnal data */
    char extra_tab_string[5][MMGRMAXEXTRA];/* optional string that could be used to store data1 to data5 */
};

mmgr_cli_handle_t *mmgr_hdl = NULL;
static int mmgr_monitor_fd[2];

static bool is_mmgr_fake_event() {
    char prop_mmgr[PROPERTY_VALUE_MAX];

    if(property_get(PROP_REPORT_FAKE, prop_mmgr, NULL)){
        if (!strcmp(prop_mmgr, "modem")) {
            return 1;
        }
    }
    return 0;
}

static void write_mmgr_monitor_with_extras(char *chain, char *extra_string, int extra_string_len, int extra_int) {
    struct mmgr_data cur_data;
    strncpy(cur_data.string, chain, sizeof(cur_data.string));
    if (extra_string_len > 0) {
        int size = MIN(extra_string_len+1, (int)sizeof(cur_data.extra_string));
        strncpy(cur_data.extra_string, extra_string, size);
        cur_data.extra_string[size-1] = 0;
    }
    else cur_data.extra_string[0] = 0;
    cur_data.extra_int = extra_int;
    write(mmgr_monitor_fd[1], &cur_data, sizeof( struct mmgr_data));
}

static void write_mmgr_monitor_with_extras_apimr(char *chain, char *extra_string[6], int nb_values,  int extra_string_len[6]) {
    struct mmgr_data cur_data;
    int i,size;
    strncpy(cur_data.string, chain, sizeof(cur_data.string));

    if (nb_values > 0) {
        size = MIN(extra_string_len[0]+1, (int)sizeof(cur_data.extra_string));
        strncpy(cur_data.extra_string, extra_string[0], size);
        cur_data.extra_string[size-1] = 0;
        for(i=1; i < nb_values; i++){
            size = MIN(extra_string_len[i]+1, (int)sizeof(cur_data.extra_tab_string[i-1]));
            strncpy(cur_data.extra_tab_string[i-1], extra_string[i], size);
            cur_data.extra_tab_string[i-1][size-1] = 0;
        }

    }
    cur_data.extra_nb_causes = nb_values - 1;
    write(mmgr_monitor_fd[1], &cur_data, sizeof( struct mmgr_data));
}

int mdm_SHUTDOWN(mmgr_cli_event_t __attribute__((unused))*ev) {
    LOGD("Received E_MMGR_NOTIFY_MODEM_SHUTDOWN");
    write_mmgr_monitor_with_extras("MMODEMOFF", NULL, 0, 0);
    return 0;
}

int mdm_REBOOT(mmgr_cli_event_t __attribute__((unused))*ev)
{
    LOGD("Received E_MMGR_NOTIFY_PLATFORM_REBOOT");
    write_mmgr_monitor_with_extras("MSHUTDOWN", NULL, 0, 0);
    return 0;
}


int mdm_OUT_OF_SERVICE(mmgr_cli_event_t __attribute__((unused))*ev)
{
    LOGD("Received E_MMGR_EVENT_MODEM_OUT_OF_SERVICE");
    write_mmgr_monitor_with_extras("MOUTOFSERVICE", NULL, 0, 0);
    return 0;
}

int mdm_MRESET(mmgr_cli_event_t __attribute__((unused))*ev)
{
    LOGD("Received E_MMGR_NOTIFY_SELF_RESET");
    write_mmgr_monitor_with_extras("MRESET", NULL, 0, 0);
    return 0;
}

int mdm_CORE_DUMP_COMPLETE(mmgr_cli_event_t *ev)
{
    LOGD("Received E_MMGR_NOTIFY_CORE_DUMP_COMPLETE");
    int extra_int = 0, extra_string_len = 0;
    char *extra_string = NULL;
    int copy_cd = 0;

    mmgr_cli_core_dump_t *cd = (mmgr_cli_core_dump_t *)ev->data;
    if (cd == NULL) {
        LOGE("%s: empty data", __FUNCTION__);
        return -1;
    }

    switch (cd->state) {
    case E_CD_TIMEOUT:
        LOGW("COREDUMP_TIMEOUT");
        // use 1 value to indicate timeout error
        extra_int = 1;
        break;
    case E_CD_LINK_ERROR:
        LOGW("COREDUMP_LINK_ERROR");
        // use 2 value to indicate link error
        extra_int = 2;
        break;
    case E_CD_PROTOCOL_ERROR:
        LOGD("COREDUMP_PROTOCOL_ERROR");
        // use 2 value to indicate protocole error
        extra_int = 3;
        break;
    case E_CD_SELF_RESET:
        LOGD("COREDUMP_SELF_RESET");
        // use 2 value to indicate self reset
        extra_int = 4;
        break;
    case E_CD_SUCCEED:
        extra_int = 0;
        copy_cd = 1;
        break;
    default:
        LOGE("Unknown core dump state");
        // use 5 value to indicate an unknown state to crashlogd
        extra_int = 5;
        break;
    }
    if ((cd->path_len < MMGRMAXEXTRA) && (copy_cd == 1) ) {
        extra_string_len = cd->path_len;
        extra_string = cd->path;
        LOGD("core dump path: %s", extra_string);
    }else if ((cd->reason_len < MMGRMAXEXTRA) && (copy_cd == 0) ){
        extra_string_len = cd->reason_len;
        if(cd->reason_len > 0) {
            extra_string = cd->reason;
            LOGE("mdm_CORE_DUMP error : %s", extra_string);
        }
    }

    write_mmgr_monitor_with_extras("MPANIC", extra_string, extra_string_len, extra_int);
    return 0;
}

int mdm_AP_RESET(mmgr_cli_event_t *ev)
{
    LOGD("Received E_MMGR_NOTIFY_AP_RESET");
    char *extra_string[6];
    int extra_string_len[6] = {0,};
    int i;
    mmgr_cli_ap_reset_t *ap = (mmgr_cli_ap_reset_t *)ev->data;

    if (ap == NULL) {
        LOGE("%s: empty data", __FUNCTION__);
    } else {
        LOGD("%s: AP reset asked by: %s (len: %lu)", __FUNCTION__, ap->name, (unsigned long)ap->len);
        if (ap->len < MMGRMAXEXTRA) {
            extra_string[0] = ap->name;
            extra_string_len[0] = ap->len;
            if(ap->num_causes > 0) {
                for(i=0; i < (int)ap->num_causes; i++) {
                    if(ap->recovery_causes[i].len < MMGRMAXEXTRA) {
                        extra_string_len[i+1] = ap->recovery_causes[i].len;
                        extra_string[i+1] = ap->recovery_causes[i].cause;
                    }
                    else {
                        LOGE("%s: cause length error : %lu", __FUNCTION__, (unsigned long)ap->recovery_causes[i].len);
                    }
                }
            }
        } else {
            LOGE("%s: length error : %lu", __FUNCTION__, (unsigned long)ap->len);
        }
    }
    write_mmgr_monitor_with_extras_apimr("APIMR", extra_string, ap->num_causes + 1,extra_string_len);
    return 0;
}

int mdm_CORE_DUMP(mmgr_cli_event_t *ev)
{
    LOGD("Received E_MMGR_NOTIFY_CORE_DUMP");
    struct mmgr_data cur_data;
    strncpy(cur_data.string,"START_CD\0",sizeof(cur_data.string));
    //TO DO update with write_mmgr_monitor_with_extras
    write(mmgr_monitor_fd[1], &cur_data, sizeof( struct mmgr_data));
    return 0;
}

int mdm_TFT_EVENT(mmgr_cli_event_t *ev)
{
    LOGD("Received E_MMGR_NOTIFY_TFT_EVENT");
    struct mmgr_data cur_data;
    mmgr_cli_tft_event_t * tft_ev = (mmgr_cli_tft_event_t *)ev->data;
    size_t i;

    // Add "TFT" in top of cur_data.string to recognize the type of event in the next
    snprintf(cur_data.string, sizeof(cur_data.string), "TFT%s", tft_ev->name);

    // cur_data.extra_int is the merge of the type and the logging rules
    cur_data.extra_int = (tft_ev->type & 0xFF) + (tft_ev->log << 8);

    if (tft_ev->num_data > 0) {
        if (tft_ev->data[0].len > 0) {
            LOGD("len of data 0: %d", tft_ev->data[0].len);
            strncpy(cur_data.extra_string, tft_ev->data[0].value, sizeof(cur_data.extra_string));
            cur_data.extra_string[MIN(tft_ev->data[0].len, sizeof(cur_data.extra_string))] = '\0';
        } else {
            cur_data.extra_string[0] = '\0';
        }

        for (i=0; i<tft_ev->num_data-1; i++) {
            LOGD("len of data %d: %d", i+1, tft_ev->data[i+1].len);
            if (tft_ev->data[i+1].len > 0) {
                strncpy(cur_data.extra_tab_string[i], tft_ev->data[i+1].value,
                        sizeof(cur_data.extra_tab_string[i]));
                cur_data.extra_tab_string[i][MIN(tft_ev->data[i+1].len,
                        sizeof(cur_data.extra_tab_string[i]))] = '\0';
            } else {
                cur_data.extra_tab_string[i][0] = '\0';
            }
        }

        cur_data.extra_nb_causes = tft_ev->num_data - 1;
    } else {
        cur_data.extra_nb_causes = 0;
    }

    write(mmgr_monitor_fd[1], &cur_data, sizeof(struct mmgr_data));
    return 0;
}


int mmgr_get_fd()
{
    return mmgr_monitor_fd[0];
}

void init_mmgr_cli_source(void){
    int ret = 0;
    if (!CRASHLOG_MODE_MMGR_ENABLED(g_crashlog_mode)) {
        LOGI("%s: MMGR source state is disabled", __FUNCTION__);
        mmgr_monitor_fd[0] = 0;
        mmgr_monitor_fd[1] = 0;
        return;
    }

    LOGD("%s : initializing MMGR source...", __FUNCTION__);
    if (mmgr_hdl){
        close_mmgr_cli_source();
    }
    mmgr_hdl = NULL;
    mmgr_cli_create_handle(&mmgr_hdl, "crashlogd", NULL);
    mmgr_cli_subscribe_event(mmgr_hdl, mdm_CORE_DUMP, E_MMGR_NOTIFY_CORE_DUMP);
    mmgr_cli_subscribe_event(mmgr_hdl, mdm_SHUTDOWN, E_MMGR_NOTIFY_MODEM_SHUTDOWN);
    mmgr_cli_subscribe_event(mmgr_hdl, mdm_REBOOT, E_MMGR_NOTIFY_PLATFORM_REBOOT);
    mmgr_cli_subscribe_event(mmgr_hdl, mdm_OUT_OF_SERVICE, E_MMGR_EVENT_MODEM_OUT_OF_SERVICE);
    mmgr_cli_subscribe_event(mmgr_hdl, mdm_MRESET, E_MMGR_NOTIFY_SELF_RESET);
    mmgr_cli_subscribe_event(mmgr_hdl, mdm_CORE_DUMP_COMPLETE, E_MMGR_NOTIFY_CORE_DUMP_COMPLETE);
    mmgr_cli_subscribe_event(mmgr_hdl, mdm_AP_RESET, E_MMGR_NOTIFY_AP_RESET);
    mmgr_cli_subscribe_event(mmgr_hdl, mdm_TFT_EVENT, E_MMGR_NOTIFY_TFT_EVENT);

    uint32_t iMaxTryConnect = MAX_WAIT_MMGR_CONNECT_SECONDS * 1000 / MMGR_CONNECT_RETRY_TIME_MS;

    while (iMaxTryConnect-- != 0) {

        /* Try to connect */
        ret = mmgr_cli_connect (mmgr_hdl);

        if (ret == E_ERR_CLI_SUCCEED) {

            break;
        }

        LOGE("%s: Delaying mmgr_cli_connect %d", __FUNCTION__, ret);

        /* Wait */
        usleep(MMGR_CONNECT_RETRY_TIME_MS * 1000);
    }
    // pipe init
    if (pipe(mmgr_monitor_fd) == -1) {
        LOGE("%s: MMGR source init failed : Can't create the pipe - error is %s\n",
             __FUNCTION__, strerror(errno));
        mmgr_monitor_fd[0] = 0;
        mmgr_monitor_fd[1] = 0;
    }
}

void close_mmgr_cli_source(void){
    if (!CRASHLOG_MODE_MMGR_ENABLED(g_crashlog_mode))
        return;

    mmgr_cli_disconnect(mmgr_hdl);
    mmgr_cli_delete_handle(mmgr_hdl);
}

/**
 * @brief Compute mmgr parameter
 *
 * Called when parameter are needed for a mmgr callback
 *
 * @param parameters needed to create event (logd, name,...)
 *
 * @return 0 on success, -1 on error.
 */
static int compute_mmgr_param(char *type, e_dir_mode_t *mode, char *name, int *aplog, int *bplog, int *log_mode) {

    if (strstr(type, "MODEMOFF" )) {
        //CASE MODEMOFF
        *mode = MODE_STATS;
        sprintf(name, "%s", INFOEVENT);
    } else if (strstr(type, "MSHUTDOWN" )) {
        //CASE MSHUTDOWN
        *mode = MODE_CRASH;
        sprintf(name, "%s", CRASHEVENT);
        *aplog = 1;
        *log_mode = APLOG_TYPE;
    } else if (strstr(type, "MOUTOFSERVICE" )) {
        //CASE MOUTOFSERVICE
        *mode = MODE_CRASH;
        sprintf(name, "%s", CRASHEVENT);
        *aplog = 1;
        *log_mode = APLOG_TYPE;
    } else if (strstr(type, "MPANIC" )) {
        //CASE MPANIC
        *mode = MODE_CRASH;
        sprintf(name, "%s",CRASHEVENT);
        *aplog = 1;
        *log_mode = APLOG_TYPE;
        *bplog = check_running_modem_trace();
    } else if (strstr(type, "APIMR" )) {
        //CASE APIMR
        *mode = MODE_CRASH;
        sprintf(name, "%s", CRASHEVENT);
        *aplog = 1;
        *log_mode = APLOG_TYPE;
        *bplog = check_running_modem_trace();
    } else if (strstr(type, "MRESET" )) {
        //CASE MRESET
        *mode = MODE_CRASH;
        sprintf(name, "%s",CRASHEVENT);
        *log_mode = APLOG_TYPE;
        *aplog = 1;
    } else  if (!strstr(type, "START_CD" )){
        //unknown event name
        LOGE("%s: wrong type found in mmgr_get_fd : %s.\n", __FUNCTION__, type);
        return -1;
    }
    return 0;
}

/**
 * @brief Handle mmgr call back
 *
 * Called when a call back function is triggered
 * depending on init subscription
 *
 * @param files nb max of logs destination directories (crashlog,
 * aplogs, bz... )
 *
 * @return 0 on success, -1 on error.
 */
int mmgr_handle(void) {
    e_dir_mode_t event_mode = MODE_CRASH;
    int aplog_mode, bplog_mode = BPLOG_TYPE, dir;
    char *event_dir, *key;
    char event_name[MMGRMAXSTRING];
    char data0[MMGRMAXEXTRA];
    char data1[MMGRMAXEXTRA];
    char data2[MMGRMAXEXTRA];
    char data3[MMGRMAXEXTRA];
    char data4[MMGRMAXEXTRA];
    char data5[MMGRMAXEXTRA];
    char cd_path[MMGRMAXEXTRA];
    char destion[PATHMAX];
    char destion2[PATHMAX];
    char type[20];
    int nbytes, copy_aplog = 0, copy_bplog = 0;
    struct mmgr_data cur_data;
    const char *dateshort = get_current_time_short(1);

    // Initialize stack strings to empty strings
    event_name[0] = 0;
    data0[0] = 0;
    data1[0] = 0;
    data2[0] = 0;
    data3[0] = 0;
    data4[0] = 0;
    data5[0] = 0;
    cd_path[0] = 0;
    destion[0] = 0;
    destion2[0] = 0;
    type[0] = 0;

    // get data from mmgr pipe
    nbytes = read(mmgr_get_fd(), &cur_data, sizeof( struct mmgr_data));
    strcpy(type,cur_data.string);

    if (nbytes == 0) {
        LOGW("No data found in mmgr_get_fd.\n");
        return 0;
    }
    if (nbytes < 0) {
        nbytes = -errno;
        LOGE("%s: Error while reading mmgr_get_fd - %s.\n", __FUNCTION__, strerror(errno));
        return nbytes;
    }
    //find_dir should be done before event_dir is set
    LOGD("Received string from mmgr: %s  - %d bytes", type,nbytes);
    // For "TFT" event, parameters are given by the data themselves
    if (strstr(type, "TFT")) {
        switch (cur_data.extra_int & 0xFF) {
             case E_EVENT_ERROR:
                 sprintf(event_name, "%s", ERROREVENT);
                 break;
             case E_EVENT_STATS:
                 sprintf(event_name, "%s", STATSEVENT);
                 break;
             case E_EVENT_INFO:
             default:
                 sprintf(event_name, "%s", INFOEVENT);
        }
        event_mode = MODE_STATS;
        aplog_mode = APLOG_STATS_TYPE;
        bplog_mode = BPLOG_STATS_TYPE;
        copy_aplog = (cur_data.extra_int >> 8) & MMGR_CLI_TFT_AP_LOG_MASK;
        copy_bplog = ((cur_data.extra_int >> 8) & MMGR_CLI_TFT_BP_LOG_MASK)
                && check_running_modem_trace();
    } else {
       if (compute_mmgr_param(type, &event_mode, event_name, &copy_aplog, &copy_bplog,
               &aplog_mode) < 0) {
           return -1;
       }
    }
    //set DATA0/1 value
    if (strstr(type, "MPANIC" )) {
        LOGD("Extra int value : %d ",cur_data.extra_int);
        if (cur_data.extra_int == 0){
            snprintf(data0,sizeof(data0),"%s", "CD_SUCCEED");
            snprintf(cd_path,sizeof(cd_path),"%s", cur_data.extra_string);
        }else if(cur_data.extra_int >= 1 && cur_data.extra_int <= 5) {
            if (cur_data.extra_int == 1){
                snprintf(data0,sizeof(data0),"%s", "CD_TIMEOUT");
            }else if (cur_data.extra_int == 2){
                snprintf(data0,sizeof(data0),"%s", "CD_LINK_ERROR");
            }else if (cur_data.extra_int == 3){
                snprintf(data0,sizeof(data0),"%s", "CD_PROTOCOL_ERROR");
            }else if (cur_data.extra_int == 4){
                snprintf(data0,sizeof(data0),"%s", "CD_SELF_RESET");
            }else if (cur_data.extra_int == 5){
                snprintf(data0,sizeof(data0),"%s", "OTHER");
            }
            snprintf(data1,sizeof(data1),"%s", cur_data.extra_string);
        }
        LOGD("Extra string value : %s ",cur_data.extra_string);
        if (file_exists(MCD_PROCESSING))
            remove(MCD_PROCESSING);
    } else if (strstr(type, "APIMR" )){
        if(!cur_data.extra_nb_causes) {
            LOGD("Extra string value : %s ",cur_data.extra_string);
            //need to put it in DATA3 to avoid conflict with parser
            snprintf(data3,sizeof(data3),"%s", cur_data.extra_string);
        }
        else if(cur_data.extra_nb_causes > 0) {
            LOGD("Extra string value : %s ",cur_data.extra_string);
            snprintf(data0,sizeof(data0),"%s", cur_data.extra_string);
            LOGD("Extra tab string value 0: %s ",cur_data.extra_tab_string[0]);
            snprintf(data1,sizeof(data1),"%s", cur_data.extra_tab_string[0]);
            if(cur_data.extra_nb_causes > 1) {
                LOGD("Extra tab string value 1: %s ",cur_data.extra_tab_string[1]);
                snprintf(data2,sizeof(data2),"%s", cur_data.extra_tab_string[1]);
            }
            if(cur_data.extra_nb_causes > 2) {
                LOGD("Extra tab string value 2: %s ",cur_data.extra_tab_string[2]);
                snprintf(data3,sizeof(data3),"%s", cur_data.extra_tab_string[2]);
            }
            if(cur_data.extra_nb_causes > 3) {
                LOGD("Extra tab string value 3: %s ",cur_data.extra_tab_string[3]);
                snprintf(data4,sizeof(data4),"%s", cur_data.extra_tab_string[3]);
            }
            if(cur_data.extra_nb_causes > 4) {
                LOGD("Extra tab string value 4 %s ",cur_data.extra_tab_string[4]);
                snprintf(data5,sizeof(data5),"%s", cur_data.extra_tab_string[4]);
            }
        }
    } else if (strstr(type, "TFT")) {
        LOGD("Extra string value : %s ",cur_data.extra_string);
        snprintf(data0,sizeof(data0),"%s", cur_data.extra_string);
        if(cur_data.extra_nb_causes > 0) {
            LOGD("Extra tab string value 0: %s ",cur_data.extra_tab_string[0]);
            snprintf(data1,sizeof(data1),"%s", cur_data.extra_tab_string[0]);
            if(cur_data.extra_nb_causes > 1) {
                LOGD("Extra tab string value 1: %s ",cur_data.extra_tab_string[1]);
                snprintf(data2,sizeof(data2),"%s", cur_data.extra_tab_string[1]);
            }
            if(cur_data.extra_nb_causes > 2) {
                LOGD("Extra tab string value 2: %s ",cur_data.extra_tab_string[2]);
                snprintf(data3,sizeof(data3),"%s", cur_data.extra_tab_string[2]);
            }
            if(cur_data.extra_nb_causes > 3) {
                LOGD("Extra tab string value 3: %s ",cur_data.extra_tab_string[3]);
                snprintf(data4,sizeof(data4),"%s", cur_data.extra_tab_string[3]);
            }
            if(cur_data.extra_nb_causes > 4) {
                LOGD("Extra tab string value 4 %s ",cur_data.extra_tab_string[4]);
                snprintf(data5,sizeof(data5),"%s", cur_data.extra_tab_string[4]);
            }
        }
        // Remove the "TFT" tag added in top of cur_data.string
        strcpy(type, &cur_data.string[3]);
    } else if (strstr(type, "START_CD" )){
        FILE *fp = fopen(MCD_PROCESSING,"w");
        if (fp == NULL){
            LOGE("can not create file: %s\n", MCD_PROCESSING);
        }
        else fclose(fp);
        return 0;
    }

    dir = find_new_crashlog_dir(event_mode);
    if (dir < 0) {
        LOGE("%s: Cannot get a valid new crash directory...\n", __FUNCTION__);
        key = raise_event(CRASHEVENT, event_name, NULL, NULL);
        LOGE("%-8s%-22s%-20s%s\n", CRASHEVENT, key, get_current_time_long(0), event_name);
        free(key);
        return -1;
    }

    // update event_dir should be done after find_dir call
    event_dir = (event_mode == MODE_STATS ? STATS_DIR : CRASH_DIR);

    if (copy_aplog > 0) {
        do_log_copy(type, dir, dateshort, aplog_mode);
    }
    if (copy_bplog > 0) {
        do_log_copy(type, dir, dateshort, bplog_mode);
    }
    snprintf(destion, sizeof(destion), "%s%d/", event_dir, dir);
    // copying file (if required)
    if (strlen(cd_path) > 0){
        char *basename;
        basename = strrchr ( cd_path, '/' );
        if (basename) basename++;
        else basename = "core_dump";
        snprintf(destion2, sizeof(destion2),"%s/%s", destion, basename);
        do_copy_tail(cd_path, destion2, 0);
        rmfr(cd_path);
    }
    // generating extra DATA (if required)
    if ((strlen(data0) > 0) || (strlen(data1) > 0) || (strlen(data2) > 0) || (strlen(data3) > 0)){
        FILE *fp;
        char fullpath[PATHMAX];
        if (strstr(event_name , ERROREVENT)) {
            snprintf(fullpath, sizeof(fullpath)-1, "%s/%s_errorevent", destion,type );
        }else if (strstr(event_name , INFOEVENT)) {
            snprintf(fullpath, sizeof(fullpath)-1, "%s/%s_infoevent", destion,type );
        }else if (strstr(event_name , STATSEVENT)) {
            snprintf(fullpath, sizeof(fullpath)-1, "%s/%s_trigger", destion,type );
        }else{
            snprintf(fullpath, sizeof(fullpath)-1, "%s/%s_crashdata", destion,type );
        }

        fp = fopen(fullpath,"w");
        if (fp == NULL) {
            LOGE("%s: Cannot create file %s - %s\n", __FUNCTION__, fullpath, strerror(errno));
        } else {
            //Fill DATA fields
            if (strlen(data0) > 0)
                fprintf(fp,"DATA0=%s\n", data0);
            if (strlen(data1) > 0)
                fprintf(fp,"DATA1=%s\n", data1);
            if (strlen(data2) > 0)
                fprintf(fp,"DATA2=%s\n", data2);
            if (strlen(data3) > 0)
                fprintf(fp,"DATA3=%s\n", data3);
            if (strlen(data4) > 0)
                fprintf(fp,"DATA4=%s\n", data4);
            if (strlen(data5) > 0)
                fprintf(fp,"DATA5=%s\n", data5);
            fprintf(fp,"_END\n");
            fclose(fp);
            do_chown(fullpath, PERM_USER, PERM_GROUP);
        }
    }
    //last step : need to check if this event should be reported as FAKE
    if (is_mmgr_fake_event()){
        //add _FAKE suffix
        strcat(type,"_FAKE");
    }
    key = raise_event(event_name, type, NULL, destion);
    LOGE("%-8s%-22s%-20s%s %s\n", event_name, key, get_current_time_long(0), type, destion);
    free(key);
    return 0;
}
