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
 * @file config_handler.c
 * @brief File containing functions to handle crashlogd configs.
 *
 * This file contains functions used for operations on crashlogds configs such
 * as loading and storing configs or using configs.
 */

#include "inotify_handler.h"
#include "crashutils.h"
#include "fsutils.h"
#include "privconfig.h"
#include "panic.h"
#include "fabric.h"
#include "config_handler.h"
#include "modem.h"
#include "tcs_wrapper.h"

#include <stdlib.h>

pconfig g_first_modem_config = NULL; /* Points to first modem_config in list */
pconfig g_current_modem_config = NULL; /* Points to current modem_config in list */

/* Global variables set from loaded config and used accross crashlogd source code */
extern int  gcurrent_uptime_hour_frequency;
extern long current_sd_size_limit;
int g_current_serial_device_id = 0; /* Specifies where serial ID should be retrieved (from emmc or from properties )*/
static int check_modem_version = 0;

//to get pconfig if it exists
pconfig get_generic_config(char* event_name, pconfig config_to_match) {
    pconfig result = NULL;
    pconfig tmp_config = config_to_match;
    while (tmp_config) {
        if (strstr(event_name, tmp_config->matching_pattern)){
            result = tmp_config;
            break;
        }
        tmp_config = tmp_config->next;
    }
    return result;
}

//to get pconfig if it exists with path argument
pconfig get_generic_config_by_path(char* path_searched, pconfig config_to_match) {
    pconfig result = NULL;
    pconfig tmp_config = config_to_match;
    if (path_searched){
        while (tmp_config) {
            if(strncmp(path_searched, tmp_config->path, strlen(path_searched))==0) {
                result = tmp_config;
                break;
            }
            tmp_config = tmp_config->next;
        }
    }
    return result;
}

//to check if process should be done on generic events
int generic_match(char* event_name, pconfig config_to_match) {
    int result = 0;
    pconfig tmp_comfig = get_generic_config(event_name,config_to_match);
    if (tmp_comfig) {
        result =1;
    }
    return result;
}

//to check if process should be done on generic events with a WD paramer
pconfig generic_match_by_wd(char* event_name, pconfig config_to_match, int wd) {
    pconfig result = NULL;
    pconfig tmp_config = get_generic_config(event_name,config_to_match);
    if (tmp_config) {
        if (tmp_config->wd_config.wd == wd){
            result = tmp_config;
        }
    }
    return result;
}
/*
* Name          : generic_add_watch
* Description   : This function adds watcher for generic config loaded
*/
void generic_add_watch(pconfig config_to_watch, int fd){
    pconfig tmp_config = config_to_watch;
    while (tmp_config) {
        if (strlen( tmp_config->path)>0){
            //add watch and store it
            tmp_config->wd_config.wd = inotify_add_watch(fd, tmp_config->path, VBCRASH_DIR_MASK);
            LOGI("generic_add_watch : %s\n", tmp_config->path);
            if (tmp_config->wd_config.wd < 0) {
                LOGE("Can't add watch for %s - %s.\n", tmp_config->path, strerror(errno));
            }else{
                //store WD in config WD
                tmp_config->wd_config.eventmask = VBCRASH_DIR_MASK;
                tmp_config->wd_config.eventpath = tmp_config->path;
                tmp_config->wd_config.eventname = EXTRA_NAME;
            }
        }
        tmp_config = tmp_config->next;
    }
}

/*
* Name          : free_config
* Description   : This function free the config structure created
*/
void free_config(pconfig first)
{
    pconfig nextconfig;
    pconfig current = first;
    while (current){
        nextconfig = current->next;
        free(current->eventname);
        free(current->matching_pattern);
        free(current);
        current = nextconfig;
    }
    first=NULL;
}

//this function requires a init_config_file before to work properly
void store_config(char *section, struct config_handle a_conf_handle){
    //for the moment, only modem is supported
    char *module;
    char *tmp;

    module = get_value_def(section,"module","UNDEFINED",&a_conf_handle);
    if (strcmp(module,"modem")){
        //for the moment, only modem is valid, code to removed when more modules are managed
        LOGE("extra configuration not supported for : %s\n",module);
        return;
    }else{
        LOGI("storing configuration : %s\n",section);
        //pconfig INIT
        pconfig newconf = malloc(sizeof(struct config));
        if(!newconf) {
            LOGE("%s: newconf malloc failed\n", __FUNCTION__);
            return;
        }
        newconf->next   = NULL;
        newconf->eventname = NULL;
        newconf->matching_pattern = NULL;
        //TO IMPROVE replace harcoded value with array in parameter
        //Event name
        tmp = get_value(section,"eventname",&a_conf_handle);
        if (tmp){
            newconf->eventname = malloc(strlen(tmp)+1);/* add 1 for \0 */
            if(!newconf->eventname) {
                LOGE("%s:malloc failed\n", __FUNCTION__);
                free_config(newconf);
                return;
            }
            strncpy(newconf->eventname,tmp,strlen(tmp));
            newconf->eventname[strlen(tmp)]= '\0';
        }else{
            LOGE("wrong configuration for %s on %s \n",section,"eventname");
            free_config(newconf);
            return;
        }
        //matching_pattern
        tmp = get_value(section,"matching_pattern",&a_conf_handle);
        if (tmp){
            newconf->matching_pattern = malloc(strlen(tmp)+1);/* add 1 for \0 */
            if(!newconf->matching_pattern) {
                LOGE("%s: matching pattern malloc failed\n", __FUNCTION__);
                free_config(newconf);
                return;
            }
            strncpy(newconf->matching_pattern,tmp,strlen(tmp));
            newconf->matching_pattern[strlen(tmp)]= '\0';
        }else{
            LOGE("wrong configuration for %s on %s \n",section,"matching_pattern");
            free_config(newconf);
            return;
        }
        //type
        tmp = get_value_def(section,"type","file",&a_conf_handle);
        if (tmp){
            if (!strcmp(tmp,"dir")){
                newconf->type = 1;
            }else{
                newconf->type = 0;
            }
        }else{
            LOGE("wrong configuration for %s on %s \n",section,"type");
            free_config(newconf);
            return;
        }
        //path
        tmp = get_value(section,"path_trigger",&a_conf_handle);
        if (tmp){
            snprintf(newconf->path, sizeof(newconf->path), "%s", tmp);
            LOGI("path loaded :  %s \n",newconf->path);
        }else{
            LOGW("missing configuration for %s on %s \n",section,"path_trigger");
            //path is not mandatory, config is still valid
        }
        //path_linked
        tmp = get_value(section,"path_linked",&a_conf_handle);
        if (tmp){
            snprintf(newconf->path_linked, sizeof(newconf->path_linked), "%s", tmp);
            LOGI("path_linked loaded :  %s \n",newconf->path_linked);
        }else{
        //path_linked is not mandatory, config is still valid
            newconf->path_linked[0] = '\0';
        }

        //event_class
        tmp = get_value_def(section,"event_class","CRASH",&a_conf_handle);
        if (tmp){
            if (!strcmp(tmp,"CRASH")){
                newconf->event_class = 0;
            }else if (!strcmp(tmp,"ERROR")){
                newconf->event_class = 1;
            }else if (!strcmp(tmp,"INFO")){
                newconf->event_class = 2;
            }else{
                newconf->event_class = 0;
            }
        }else{
            //default value is CRASH
            newconf->event_class = 0;
        }
        if (g_first_modem_config==NULL){
            g_first_modem_config = newconf;
        }else{
            g_current_modem_config->next = newconf;
        }
        g_current_modem_config = newconf;
    }
}

void load_config_by_pattern(char *section_pattern, char *key_pattern, struct config_handle a_conf_handle){
    char *cur_section_name;
    LOGI("checking : %s\n",section_pattern );

    cur_section_name = get_first_section_name( section_pattern,&a_conf_handle);
    while (cur_section_name &&(sk_exists(cur_section_name, key_pattern, &a_conf_handle))){
        LOGI("storing config for :%s\n", cur_section_name);
        store_config(cur_section_name, a_conf_handle);
        cur_section_name = get_next_section_name( section_pattern,&a_conf_handle);
    }
}

void load_config(){
    struct stat info;
    char cur_extra_section[PATHMAX];
    struct config_handle my_conf_handle;
    int i_tmp;
    long l_tmp;
    //Check if config file exists
    if (stat(CRASHLOG_CONF_PATH, &info) == 0) {
        LOGI("Loading specific crashlog config\n");

        my_conf_handle.first=NULL;
        my_conf_handle.current=NULL;
        if (init_config_file(CRASHLOG_CONF_PATH, &my_conf_handle)>=0){
            //General config - uptime
            //TO IMPROVE : general config strategy to define properly
            if (sk_exists(GENERAL_CONF_PATTERN,"uptime_frequency",&my_conf_handle)){
                char *tmp = get_value(GENERAL_CONF_PATTERN,"uptime_frequency",&my_conf_handle);
                if (tmp){
                    i_tmp = atoi(tmp);
                    if (i_tmp > 0){
                        gcurrent_uptime_hour_frequency = i_tmp;
                    }
                }
            }
            if (sk_exists(GENERAL_CONF_PATTERN,"sd_size_limit",&my_conf_handle)){
                char *tmp = get_value(GENERAL_CONF_PATTERN,"sd_size_limit",&my_conf_handle);
                if (tmp){
                    l_tmp = atol(tmp);
                    if (l_tmp > 0){
                        current_sd_size_limit = l_tmp;
                    }
                }
            }
            if (sk_exists(GENERAL_CONF_PATTERN,"serial_device_id",&my_conf_handle)){
                char *tmp = get_value(GENERAL_CONF_PATTERN,"serial_device_id",&my_conf_handle);
                if (tmp){
                    i_tmp = atoi(tmp);
                    if (i_tmp > 0){
                        g_current_serial_device_id = 1;
                    }
                }
            }
            if (sk_exists(GENERAL_CONF_PATTERN,"check_modem_version",&my_conf_handle)){
                char *tmp = get_value(GENERAL_CONF_PATTERN,"check_modem_version",&my_conf_handle);
                if (tmp){
                    i_tmp = atoi(tmp);
                    if (i_tmp > 0){
                        check_modem_version = 1;
                    } else {
                        check_modem_version = 0;
                    }
                    LOGI("Check modem version: %d", check_modem_version);
                }
            }
            load_config_by_pattern(NOTIFY_CONF_PATTERN,"matching_pattern",my_conf_handle);
            //ADD other config pattern HERE
            free_config_file(&my_conf_handle);
        }else{
            LOGI("specific crashlog config not found\n");
        }
    }
}

/*
 * Return the value of the property
 *  check_modem_version
 * This function avoids the use of global variables.
 */
int cfg_check_modem_version() {
    return check_modem_version;
}
