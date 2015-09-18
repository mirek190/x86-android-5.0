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
 * @file iptrak.c
 * @brief File containing functions used to handle operations on
 * IPTRAK file.
 *
 */

#include <cutils/properties.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "iptrak.h"
#include "privconfig.h"
#include "fsutils.h"

#define PMIC_DEBUG_DIR          "/sys/kernel/pmic_debug/pmic_debug"

#define PMIC_ADDRESS_FILE       PMIC_DEBUG_DIR "/addr"
#define PMIC_OPERATION_FILE     PMIC_DEBUG_DIR "/ops"
#define PMIC_DATA_FILE          PMIC_DEBUG_DIR "/data"

/**
 * @brief Writes the Vendor and Stepping values to the
 * given buffers.
 * While no robust and portable solution has been found
 * for all board types, vendor and stepping will be
 * set to "unknown".
 *
 */
static int retrieve_pmic_values(char *vendor, char *stepping) {
    char pmic_vendor[PROPERTY_VALUE_MAX];
    char pmic_stepping[PROPERTY_VALUE_MAX];
    /* Retrieve the vendor value */
    strncpy(vendor, "unknown", PROPERTY_VALUE_MAX);
    /* Retrieve the stepping value */
    strncpy(stepping, "unknown", PROPERTY_VALUE_MAX);
    /* Tell the caller that everything went OK */
    return 1;
}

/*
 * Writes the IPTRAK file.
 * Returns a value >= 0 if the operation was OK and a negative value otherwise.
 *
 */
static int write_iptrak_file(
        const char* ifwi_version,
        const char* os_version,
        const char* bkc_version,
        const char* pmic_version,
        const char* soc_version,
        const char* uptime) {
    /* Variables used to write/read from files */
    FILE *fd;
    int ret;
    /* Write iptrak file */
    fd = fopen(IPTRAK_FILE, "w");
    if (fd == NULL) {
        LOGE("[IPTRAK] %s: Cannot write IPTRAK file %s - %s\n", __FUNCTION__, IPTRAK_FILE,
            strerror(errno));
        return -1;
    }

    fprintf(fd, "%s=%s\n", "IFWI Version", ifwi_version);
    fprintf(fd, "%s=%s\n", "OS Version", os_version);
    fprintf(fd, "%s=%s\n", "BKC Version", bkc_version);
    fprintf(fd, "%s=%s\n", "PMIC Version", pmic_version);
    fprintf(fd, "%s=%s\n", "SOC Version", soc_version);
    fprintf(fd, "%s=%s\n", "Uptime", uptime);
    fclose(fd);
    LOGI("[IPTRAK] Updating uptime value: %s.\n", uptime);
    /* Change file permissions */
    ret = do_chmod(IPTRAK_FILE, "640");
    if (ret) {
        LOGE("[IPTRAK] %s: Cannot change IPTRAK file permissions %s - %s\n",
             __FUNCTION__, IPTRAK_FILE, strerror(errno));
        return -1;
    }
    ret = do_chown(IPTRAK_FILE, PERM_USER, PERM_GROUP);
    if (ret) {
        LOGE("[IPTRAK] %s: Cannot change IPTRAK file owner %s - %d\n",
             __FUNCTION__, IPTRAK_FILE, ret);
        return -1;
    }

    return 0;
}

static void retrieve_iptrak_properties(
        char* current_ifwi_version,
        char* current_os_version,
        char* current_bkc_version,
        char* current_pmic,
        char* current_soc_version) {
    /* Variables used to write/read from files */
    FILE *fd;
    char name[32];
    /* Reset output buffers */
    memset(current_ifwi_version, '\0', PROPERTY_VALUE_MAX);
    memset(current_os_version, '\0', PROPERTY_VALUE_MAX);
    memset(current_bkc_version, '\0', PROPERTY_VALUE_MAX);
    memset(current_pmic, '\0', PROPERTY_VALUE_MAX);
    memset(current_soc_version, '\0', PROPERTY_VALUE_MAX);
    /* Read current property values from IPTRAK file */
    fd = fopen(IPTRAK_FILE, "r");
    if (fd == NULL) {
        /* Fill current properties with dummy values */
        strncpy(current_ifwi_version, "unknown", PROPERTY_VALUE_MAX);
        strncpy(current_os_version, "unknown", PROPERTY_VALUE_MAX);
        strncpy(current_bkc_version, "unknown", PROPERTY_VALUE_MAX);
        strncpy(current_pmic, "unknown", PROPERTY_VALUE_MAX);
        strncpy(current_soc_version, "unknown", PROPERTY_VALUE_MAX);
    } else {
        /* Read values from IPTRAK file */
        /* First line: IFWI */
        fscanf(fd, "%s=%s\n", name, current_ifwi_version);
        if(current_ifwi_version[0] == '\0') {
            strncpy(current_ifwi_version, "unknown", PROPERTY_VALUE_MAX);
        }
        /* Second line: OS */
        fscanf(fd, "%s=%s\n", name, current_os_version);
        if(current_os_version[0] == '\0') {
            strncpy(current_os_version, "unknown", PROPERTY_VALUE_MAX);
        }
        /* Third line: BKC */
        fscanf(fd, "%s=%s\n", name, current_bkc_version);
        if(current_bkc_version[0] == '\0') {
            strncpy(current_bkc_version, "unknown", PROPERTY_VALUE_MAX);
        }
        /* Fourth line: PMIC */
        fscanf(fd, "%s=%s\n", name, current_pmic);
        if(current_pmic[0] == '\0') {
            strncpy(current_pmic, "unknown", PROPERTY_VALUE_MAX);
        }
        /* Fifth line: SOC */
        fscanf(fd, "%s=%s\n", name, current_soc_version);
        if(current_soc_version[0] == '\0') {
            strncpy(current_soc_version, "unknown", PROPERTY_VALUE_MAX);
        }
        fclose(fd);
    }
}

static void get_int_values_from_uptime(
        int *hours,
        int *minutes,
        int *seconds,
        const char* uptime_str) {

    char *hours_str, *minutes_str, *seconds_str;
    char *token;
    char temp[PROPERTY_VALUE_MAX];

    /* Check the buffer lengths */
    if(UPTIME_MAX_LENGTH >= PROPERTY_VALUE_MAX) {
        *hours = 0;
        *minutes = 0;
        *seconds = 0;
        LOGE(
            "[IPTRAK] %s: invalid buffer sizes (%d, %d).\n",
            __FUNCTION__,
            sizeof(temp),
            UPTIME_MAX_LENGTH);
        return;
    }

    /* Copy string to temporary buffer because of strtok modifying parameters*/
    strncpy(temp, uptime_str, UPTIME_MAX_LENGTH);
    /* Parse the string */
    hours_str = strtok(temp, ":");
    minutes_str = strtok(NULL, ":");
    seconds_str = strtok(NULL, ":");
    /* Check retrieved values */
    if(NULL == hours_str || NULL == minutes_str || NULL == seconds_str) {
        LOGE(
            "[IPTRAK] %s: Cannot read hours: %s, minutes: %s, seconds: %s from string %s\n",
            __FUNCTION__,
            hours_str,
            minutes_str,
            seconds_str,
            uptime_str);
        return;
    }
    *hours = atoi(hours_str);
    *minutes = atoi(minutes_str);
    *seconds = atoi(seconds_str);
}

static void get_total_uptime(char* uptime) {
    /* Variables used to write/read from files */
    FILE *fd;
    char name[32];
    char line_beginning[7];
    char lastbootuptime[UPTIME_MAX_LENGTH] = "0000:00:00";

    /* Read uptime from history file */
    fd = fopen(HISTORY_FILE, "r");
    if (fd == NULL) {
        LOGE("[IPTRAK] %s: Cannot read uptime value from file %s - %s\n", __FUNCTION__, HISTORY_FILE,
            strerror(errno));
        return;
    }
    fscanf(fd, "#V1.0 %16s%24s\n", name, lastbootuptime);
    /* Check that the uptime line in history file was not currupted*/
    if (memcmp(name, "CURRENTUPTIME", sizeof("CURRENTUPTIME"))) {
        LOGE("[IPTRAK] %s: Bad first line in %s; cannot continue\n",
            __FUNCTION__, HISTORY_FILE);
        return;
    }

    /* Close history file */
    fclose(fd);

    /* Copy the value to the output string */
    strncpy(uptime, lastbootuptime, UPTIME_MAX_LENGTH);
}

/*
 * Updates the iptrak file.
 * Returns 1 if everything went OK and 0 otherwise.
 */
static int update_iptrak_file() {
    /* Variables to store current property values in IPTRAK file */
    char current_ifwi_version[PROPERTY_VALUE_MAX];
    char current_os_version[PROPERTY_VALUE_MAX];
    char current_bkc_version[PROPERTY_VALUE_MAX];
    char current_soc_version[PROPERTY_VALUE_MAX];
    char current_pmic[PROPERTY_VALUE_MAX];
    /* New property values */
    char ifwi_version[PROPERTY_VALUE_MAX];
    char os_version[PROPERTY_VALUE_MAX];
    char bkc_version[PROPERTY_VALUE_MAX];
    char soc_version[PROPERTY_VALUE_MAX];
    char pmic_version[PROPERTY_VALUE_MAX];
    /* Other variables used to write/read from files */
    FILE *fd;
    char name[32];
    char total_uptime[UPTIME_MAX_LENGTH] = "0000:00:00";
    char vendor[PROPERTY_VALUE_MAX];
    char stepping[PROPERTY_VALUE_MAX];
    /* Properties and IPTRAK file operation statuses */
    int iptrak_file_write_result = 0;
    int iptrak_properties_result = 0;
    int need_update_next_time = -1;

    LOGI("[IPTRAK] %s: Generating new IPTRAK file.\n",
        __FUNCTION__);
    /* Read current property values from IPTRAK file */
    retrieve_iptrak_properties(
        current_ifwi_version,
        current_os_version,
        current_bkc_version,
        current_pmic,
        current_soc_version);

    /* Retrieve the new property values */
    property_get(IFWI_FIELD, ifwi_version, current_ifwi_version);
    property_get(PROP_RELEASE_VERSION, os_version, current_os_version);
    property_get(PROP_SYS_BKC_VERSION, bkc_version, current_bkc_version);
    property_get(PROP_SOC_VERSION, soc_version, current_soc_version);

    /* Retrive the total uptime */
    get_total_uptime(total_uptime);

    /* Retrieve PMIC values */
    if(retrieve_pmic_values(vendor, stepping)) {
        /* Format the pmic values */
        sscanf(vendor, "0x%s", vendor);
        sscanf(stepping, "0x%s", stepping);
        LOGI("[IPTRAK] %s: Got PMIC vendor '%s' and stepping '%s'.\n",
            __FUNCTION__,
            vendor,
            stepping);
        snprintf(pmic_version, sizeof(pmic_version), "%s:%s", vendor, stepping);
    } else {
        /* Keep the previous value */
        strncpy(pmic_version, current_pmic, PROPERTY_VALUE_MAX);
    }

    /* Check whether we got some valid property values */
    /* For now, we consider that iptrak does not require  */
    /* a iptrak file re-generation.                       */
    if(strncmp(ifwi_version, "unknown", 7) == 0 ||
            strncmp(os_version, "unknown", 7) == 0 ||
            strncmp(bkc_version, "unknown", 7) == 0 ||
            strncmp(soc_version, "unknown", 7) == 0) {
        iptrak_properties_result = -1;
    }

    /* Actually write the IPTRAK file */
    iptrak_file_write_result = write_iptrak_file(
        ifwi_version,
        os_version,
        bkc_version,
        pmic_version,
        soc_version,
        total_uptime);

    /* Check whether an update is necessary or not. */
    if(iptrak_file_write_result < 0 || iptrak_properties_result < 0) {
        /* Tell the caller that an update should be made next time. */
        need_update_next_time = 0;
    } else {
        /* Tell the call that no update is needed next time.        */
        need_update_next_time = 1;
    }

    /* Return a value that can be used as a boolean. */
    return need_update_next_time;
}

/*
 * Check whether the iptrak file needs to be updated or not.
 * @param[in] uptade: one of iptrak_generation_policy values:
 *                          - ONLY_IF_REQUIRED: do not force generation
 *                          - FORCE_GENERATION: force generation
 *                          - RETRY_ONCE: retry file generation on next call if an error occurred.
 *
 */
void check_iptrak_file(e_iptrak_gen_policy_t update){
    static int need_update = 1;
    int update_success = 0;

    LOGI("[IPTRAK] %s: Initial value of 'need_update' %d.\n",
        __FUNCTION__,
        need_update);
    LOGI("[IPTRAK] %s: Force update value %d.\n",
        __FUNCTION__,
        update);
    /* Handle the case when update is forced */
    if (update == FORCE_GENERATION) {
        need_update = 1;
    }
    LOGI("[IPTRAK] %s: 'need_update' value %d.\n",
        __FUNCTION__,
        need_update);
    if (need_update) {
        update_success = update_iptrak_file();
        LOGI("[IPTRAK] %s: Update status %d.\n",
            __FUNCTION__,
            update_success);
        /* Check whether we will need to update the file next time or not.    */
        if(update == RETRY_ONCE && !update_success) {
            /* If retry was requested we will trigger an update on next call  */
            /* if an error occured or if some data were missing.              */
            need_update = 1;
            LOGI("[IPTRAK] %s: RETRY_ONCE requested, 'need_update' value %d.\n",
                __FUNCTION__,
                need_update);
        } else {
            /* If we were not asked to retry, then assume we will not need    */
            /* any update regardless of the update result.                    */
            need_update = 0;
        }
    }
}
