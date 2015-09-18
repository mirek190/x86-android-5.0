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
 * @file crashutils.h
 * @brief File containing functions used to handle various operations on events.
 *
 * This file contains the functions used to handle various operations linked to
 * events such as :
 *  - general event raising (crash, bz...)
 *  - info event creation/raising
 *  - crashfile creation
 */

#ifndef __CRASHUTILS_H__
#define __CRASHUTILS_H__

#include "inotify_handler.h"

/* Define time formats */
enum time_format {
    DATE_FORMAT_SHORT = 0,
    TIME_FORMAT_SHORT,
    TIME_FORMAT_LONG,
};

/* Define error type */
enum partition_error {
    ERROR_LOGS_MISSING = 0,
    ERROR_PARTITIONS_MISSING,
    ERROR_LOGS_RO,
};

#define TIME_FORMAT_LENGTH  32
#define DUPLICATE_TIME_FORMAT    "%Y-%m-%d/%H:%M:%S"

#define PRINT_TIME(var_tmp, format_time, local_time) {              \
    strftime(var_tmp, TIME_FORMAT_LENGTH, format_time, local_time); \
    var_tmp[TIME_FORMAT_LENGTH-1]=0;                                \
    }

char *get_time_formated(char *format, char *dest);

const char *get_current_time_long(int refresh);
const char *get_current_time_short(int refresh);
const char *get_current_date_short(int refresh);

unsigned long long get_uptime(int refresh, int *error);

char **commachain_to_fixedarray(char *chain,
        unsigned int recordsize, unsigned int maxrecords, int *res);
int do_screenshot_copy(char* bz_description, char* bzdir);

void do_wdt_log_copy(int dir);
void do_last_kmsg_copy(int dir);
void do_last_fw_msg_copy(int dir);
void clean_crashlog_in_sd(char *dir_to_search, int max);
void check_crashlog_died();
int raise_infoerror(char *type, char *subtype);
int create_rebootfile(char* key, int data_ready);
int reboot_reason_files_present();
void get_data_from_boot_file(char *file, char* data, FILE* fp);
char *raise_event(char *event, char *type, char *subtype, char *log);
char *raise_event_nouptime(char *event, char *type, char *subtype, char *log);
char *raise_event_wdt(char *event, char *type, char *subtype, char *log);
char *raise_event_bootuptime(char *event, char *type, char *subtype, char *log);
char *raise_event_dataready(char *event, char *type, char *subtype, char *log, int data_ready);
char *raise_event_dataready012(char *event, char *type, char *subtype, char *log,
                            int data_ready,char* data0, char* data1, char* data2);
void create_infoevent(char* filename, char* data0, char* data1,
    char* data2);
void notify_crashreport();
char *create_crashdir_move_crashfile(char *origpath, char *crashfile, int copylogs);

void start_daemon(const char *daemonname);
void restart_profile_srv(int serveridx);
void check_running_power_service();
int check_running_modem_trace();


int get_build_board_versions(char *filename, char *buildver, char *boardver);
const char *get_build_footprint();

void build_crashenv_parameters( char * crashenv_param );
void monitor_crashenv();

int process_info_and_error_inotify_callback(struct watch_entry *entry, struct inotify_event *event);
int process_info_and_error(char *filename, char *name);
int notify_partition_error(enum partition_error type);

int get_cmdline_bootreason(char *bootreason);

#endif /* __CRASHUTILS_H__ */
