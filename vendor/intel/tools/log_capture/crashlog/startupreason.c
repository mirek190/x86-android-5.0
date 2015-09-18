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
 * @file startupreason.c
 * @brief File containing functions to read startup reason and to detect and
 * process wdt event.
 */

#include "crashutils.h"
#include "fsutils.h"
#include "privconfig.h"
#include "uefivar.h"

#ifdef CONFIG_EFILINUX
#include <efilinux/bootlogic.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>

#ifdef CONFIG_EFILINUX
static int compute_uefi_startupreason(enum reset_sources rs, enum reset_types rt,
                                      enum wake_sources ws, enum shutdown_sources ss __unused,
                                      char *startupreason) {
    static const char *wake_source[] = {
        "WAKE_NOT_APPLICABLE",
        "WAKE_BATT_INSERT",
        "WAKE_USB_CHRG_INSERT",
        "WAKE_ACDC_CHGR_INSERT",
        "WAKE_PWR_BUTTON_PRESS",
        "WAKE_RTC_TIMER",
        "WAKE_BATT_THRESHOLD",
    };
    static const char *reset_source[] = {
        "RESET_NOT_APPLICABLE",
        "RESET_OS_INITIATED",
        "RESET_FORCED",
        "RESET_FW_UPDATE",
        "RESET_KERNEL_WATCHDOG",
        "RESET_SECURITY_WATCHDOG",
        "RESET_SECURITY_INITIATED",
        "RESET_PMC_WATCHDOG",
        "RESET_EC_WATCHDOG",
        "RESET_PLATFORM_WATCHDOG",
    };
    static const char *reset_type[] = {
        "NOT_APPLICABLE",
        "WARM_RESET",
        "COLD_RESET",
        "GLOBAL_RESET",
    };

    char str_wake_source[32] = { '\0', };
    char str_reset_source[32] = { '\0', };
    char str_reset_type[32] = { '\0', };
    char str_shutdown_source[32] = { '\0', };
    unsigned int s;

    s = (unsigned int) ws;
    if  ((s > 0) && (s < (sizeof(wake_source)/sizeof(char*)))) {
        strcpy(str_wake_source, wake_source[s]);
        LOGI("wake source (%d): %s", s, str_wake_source);
    }

    s = (unsigned int) rs;
    if  ((s > 0) && (s < (sizeof(reset_source)/sizeof(char*)))) {
        strcpy(str_reset_source, reset_source[s]);
        LOGI("reset source (%d): %s", s, str_reset_source);
    }

    s = (unsigned int) rt;
    if  ((s > 0) && (s < (sizeof(reset_type)/sizeof(char*)))) {
        strcpy(str_reset_type, reset_type[s]);
        LOGI("reset type (%d): %s", s, str_reset_type);
    }

    if (strlen(str_wake_source))
        strcpy(startupreason, str_wake_source);
    else if (strlen(str_reset_source))
      if (strstr(str_reset_source, "KERNEL_WATCHDOG"))
         strcpy(startupreason, "SWWDT_RESET");
      else if  (strstr(str_reset_source, "WATCHDOG"))
         strcpy(startupreason, "HWWDT_RESET");
      else
         strcpy(startupreason, str_reset_source);
    else
       return -1;

    return 0;
}

static int get_uefi_startupreason(char *startupreason)
{
    int ret = 0;

    const char *var_guid_common = "4a67b082-0a4c-41cf-b6c7-440b29bb8c4f";
    //const char *var_guid_common = "80868086-8086-8086-8086-000000000200";
    const char *var_reset_source = "ResetSource";
    const char *var_reset_type = "ResetType";
    const char *var_wake_source = "WakeSource";
    const char *var_shutdown_source = "ShutdownSource";

    enum reset_sources rs;
    enum reset_types rt;
    enum wake_sources ws;
    enum shutdown_sources ss;

    strcpy(startupreason, "UNKNOWN");

    ret = uefivar_get_int(var_reset_source, var_guid_common, (unsigned int *)&rs);
    if (ret) {
        LOGE("uefivar_get error %s-%s : %d\n",
               var_reset_source, var_guid_common, ret);
        return -1;
    }

    ret = uefivar_get_int(var_reset_type, var_guid_common, (unsigned int *)&rt);
    if (ret) {
        LOGE("uefivar_get error %s-%s : %d\n",
               var_reset_type, var_guid_common, ret);
        return -1;
    }

    ret = uefivar_get_int(var_wake_source, var_guid_common, (unsigned int *)&ws);
    if (ret) {
        LOGE("uefivar_get error %s-%s : %d\n",
               var_wake_source, var_guid_common, ret);
        return -1;
    }

    ret = uefivar_get_int(var_shutdown_source, var_guid_common, (unsigned int *)&ss);
    if (ret) {
        LOGE("uefivar_get error %s-%s : %d\n",
               var_shutdown_source, var_guid_common, ret);
        return -1;
    }

    LOGI("%s: %s:%d, %s:%d, %s:%d, %s:%d\n", __func__, var_reset_source, rs,
         var_reset_type, rt, var_wake_source, ws, var_shutdown_source, ss);

    return compute_uefi_startupreason(rs, rt, ws, ss, startupreason);
}
#endif // CONFIG_EFILINUX

#ifdef CONFIG_FDK
/*
* Name          : get_fdk_startupreason
* Description   : This function returns the decoded startup reason by reading
*                 the wake source from the command line. The wake src is translated
*                 to a crashtool startup reason.
*                 In case of a HW watchdog, the wake sources are translated to a HWWDT
*                 List of wake sources :
*    0x00: WAKE_BATT_INSERT
*    0x01: WAKE_PWR_BUTTON_PRESS
*    0x02: WAKE_RTC_TIMER
*    0x03: WAKE_USB_CHRG_INSERT
*    0x04: Reserved
*    0x05: WAKE_REAL_RESET -> COLD_RESET
*    0x06: WAKE_COLD_BOOT
*    0x07: WAKE_UNKNOWN
*    0x08: WAKE_KERNEL_WATCHDOG_RESET -> SWWDT_RESET
*    0x09: WAKE_SECURITY_WATCHDOG_RESET (Chaabi ou TXE/SEC) -> HWWDT_RESET_SECURITY
*    0x0A: WAKE_WATCHDOG_COUNTER_EXCEEDED -> WDT_COUNTER_EXCEEDED
*    0x0B: WAKE_POWER_SUPPLY_DETECTED
*    0x0C: WAKE_FASTBOOT_BUTTONS_COMBO
*    0x0D: WAKE_NO_MATCHING_OSIP_ENTRY
*    0x0E: WAKE_CRITICAL_BATTERY
*    0x0F: WAKE_INVALID_CHECKSUM
*    0x10: WAKE_FORCED_RESET
*    0x11: WAKE_ACDC_CHGR_INSERT
*    0x12: WAKE_PMIC_WATCHDOG_RESET (PMIC/EC) -> HWWDT_RESET_PMIC
*    0x13: WAKE_PLATFORM_WATCHDOG_RESET -> HWWDT_RESET_PLATFORM
*    0x14: WAKE_SC_WATCHDOG_RESET (SCU/PMC) -> HWWDT_RESET_SC
*
* Parameters    :
*   char *startupreason   -> string containing the translated startup reason
*   */
void get_fdk_startupreason(char *startupreason)
{
    char cmdline[1024] = { '\0', };
    char prop_reason[PROPERTY_VALUE_MAX];

    char *p, *endptr;
    unsigned long reason;
    FILE *fd;
    int res;
    static const char *bootmode_reason[] = {
        "BATT_INSERT",
        "PWR_BUTTON_PRESS",
        "RTC_TIMER",
        "USB_CHRG_INSERT",
        "Reserved",
        "COLD_RESET",
        "COLD_BOOT",
        "UNKNOWN",
        "SWWDT_RESET",
        "HWWDT_RESET_SECURITY",
        "WDT_COUNTER_EXCEEDED",
        "POWER_SUPPLY_DETECTED",
        "FASTBOOT_BUTTONS_COMBO",
        "NO_MATCHING_OSIP_ENTRY",
        "CRITICAL_BATTERY",
        "INVALID_CHECKSUM",
        "FORCED_RESET",
        "ACDC_CHGR_INSERT",
        "HWWDT_RESET_PMIC",
        "HWWDT_RESET_PLATFORM",
        "HWWDT_RESET_SC"};

    strcpy(startupreason, "UNKNOWN");
    fd = fopen(CURRENT_KERNEL_CMDLINE, "r");
    if ( fd == NULL ) {
        LOGE("%s: Cannot open file %s - %s\n", __FUNCTION__,
            CURRENT_KERNEL_CMDLINE, strerror(errno));
        return;
    }
    res = fread(cmdline, 1, sizeof(cmdline)-1, fd);
    fclose(fd);
    if (res <= 0) {
        LOGE("%s: Cannot read file %s - %s\n", __FUNCTION__,
            CURRENT_KERNEL_CMDLINE, strerror(errno));
        return;
    }
    p = strstr(cmdline, STARTUP_STR);
    if(!p) {
        /* No reason in the command line, use property */
        LOGE("%s: no reason in cmdline : %s \n",  __FUNCTION__, cmdline);
        if (property_get("ro.boot.wakesrc", prop_reason, "") > 0) {
            reason = strtoul(prop_reason, NULL, 16);
        } else {
            LOGE("%s: no property found... \n",  __FUNCTION__);
            return;
        }
    } else {

        if (strlen(p) <= strlen(STARTUP_STR)) {
            /* the pattern is found but is incomplete... */
            LOGE("%s: Incomplete startup reason found in cmdline \"%s\"\n",
                __FUNCTION__, cmdline);
            return;
        }
        p += strlen(STARTUP_STR);
        if (isspace(*p)) {
            /* the pattern is found but starting with a space... */
            LOGE("%s: Incorrect startup reason found in cmdline \"%s\"\n",
                __FUNCTION__, cmdline);
            return;
        }

        /* All is fine, decode the reason */
        errno = 0;
        reason = strtoul(p, &endptr, 16);
        if (endptr == p) {
            LOGE("%s: Invalid startup reason found in cmdline \"%s\"\n",
            __FUNCTION__, cmdline);
            return;
        }
    }
    if ((errno != ERANGE) &&
        (reason < (sizeof(bootmode_reason)/sizeof(char*)))) {
        strcpy(startupreason, bootmode_reason[reason]);
    } else {
        /* Hmm! bad value... */
        LOGE("%s: Invalid startup reason found \"%s\"\n",
            __FUNCTION__, startupreason);
    }
}
#endif // CONFIG_FDK

static void get_default_bootreason(char *bootreason) {
    int ret;
    unsigned int i;
    char bootreason_prop[PROPERTY_VALUE_MAX];

    ret = get_cmdline_bootreason(bootreason_prop);
    if (ret <= 0)
        return;

    for (i = 0; i < strlen(bootreason_prop); i++)
        bootreason[i] = toupper(bootreason_prop[i]);
    bootreason[i] = '\0';
}

void read_startupreason(char *startupreason) {
#ifdef CONFIG_EFILINUX
    get_uefi_startupreason(startupreason);
    return;
#endif

#ifdef CONFIG_FDK
    return get_fdk_startupreason(startupreason);
#endif

    get_default_bootreason(startupreason);
}

#ifdef CONFIG_EFILINUX
int read_resetsource(char * resetsource)
{
    static const char *reset_source[] = {
            "RESET_NOT_APPLICABLE",
            "RESET_OS_INITIATED",
            "RESET_FORCED",
            "RESET_FW_UPDATE",
            "RESET_KERNEL_WATCHDOG",
            "RESET_SECURITY_WATCHDOG",
            "RESET_SECURITY_INITIATED",
            "RESET_PMC_WATCHDOG",
            "RESET_EC_WATCHDOG",
            "RESET_PLATFORM_WATCHDOG",
    };

    int ret = 0;
    const char *var_guid_common = "4a67b082-0a4c-41cf-b6c7-440b29bb8c4f";
    const char *var_reset_source = "ResetSource";
    unsigned int rs;

    ret = uefivar_get_int(var_reset_source, var_guid_common, &rs);

    if (ret) {
        LOGE("uefivar_get error %s-%s : %d\n",
                var_reset_source, var_guid_common, ret);
        return -1;
    }

    if ((rs > 0) && (rs < (sizeof(reset_source)/sizeof(char*)))) {
        strcpy(resetsource, reset_source[rs]);
        LOGI("reset source value (%d): %s", rs, resetsource);
        return 1;
    } else {
        return 0;
    }
}


int read_resettype(char * resettype)
{
    static const char *reset_type[] = {
            "NOT_APPLICABLE",
            "WARM_RESET",
            "COLD_RESET",
            "GLOBAL_RESET",
    };

    int ret = 0;
    const char *var_guid_common = "4a67b082-0a4c-41cf-b6c7-440b29bb8c4f";
    const char *var_reset_type = "ResetType";
    unsigned int rt;

    ret = uefivar_get_int(var_reset_type, var_guid_common, &rt);

    if (ret) {
        LOGE("uefivar_get error %s-%s : %d\n",
                var_reset_type, var_guid_common, ret);
        return -1;
    }

    if ((rt > 0) && (rt < (sizeof(reset_type)/sizeof(char*)))) {
        strcpy(resettype, reset_type[rt]);
        LOGI("reset type (%d): %s", rt, resettype);
        return 1;
    } else {
        return 0;
    }
}

int read_wakesource(char * wakesource)
{
    static const char *wake_source[] = {
            "WAKE_NOT_APPLICABLE",
            "WAKE_BATT_INSERT",
            "WAKE_USB_CHRG_INSERT",
            "WAKE_ACDC_CHGR_INSERT",
            "WAKE_PWR_BUTTON_PRESS",
            "WAKE_RTC_TIMER",
            "WAKE_BATT_THRESHOLD",
    };

    int ret = 0;
    const char *var_guid_common = "4a67b082-0a4c-41cf-b6c7-440b29bb8c4f";
    const char *var_wake_source = "WakeSource";
    unsigned int ws;

    ret = uefivar_get_int(var_wake_source, var_guid_common, &ws);

    if (ret) {
        LOGE("uefivar_get error %s-%s : %d\n",
                var_wake_source, var_guid_common, ret);
        return -1;
    }

    if ((ws > 0) && (ws < (sizeof(wake_source)/sizeof(char*)))) {
        strcpy(wakesource, wake_source[ws]);
        LOGI("wake source (%d): %s", ws, wakesource);
        return 1;
    } else {
        return 0;
    }
}


int read_shutdownsource(char * shutdownsource)
{
    static const char *shutdown_source[] = {
            "SHTDWN_NOT_APPLICABLE",
            "SHTDWN_POWER_BUTTON_OVERRIDE",
            "SHTDWN_BATTERY_REMOVAL",
            "SHTDWN_VCRIT",
            "SHTDWN_THERMTRIP",
            "SHTDWN_PMICTEMP",
            "SHTDWN_SYSTEMP",
            "SHTDWN_BATTEMP",
            "SHTDWN_SYSUVP",
            "SHTDWN_SYSOVP",
            "SHTDWN_SECURITY_WATCHDOG",
            "SHTDWN_SECURITY_INITIATED",
            "SHTDWN_PMC_WATCHDOG",
            "SHTDWN_ERROR",
    };

    int ret = 0;
    const char *var_guid_common = "4a67b082-0a4c-41cf-b6c7-440b29bb8c4f";
    const char *var_shutdown_source = "ShutdownSource";
    unsigned int ss;

    ret = uefivar_get_int(var_shutdown_source, var_guid_common, &ss);

    if (ret) {
        LOGE("uefivar_get error %s-%s : %d\n",
                var_shutdown_source, var_guid_common, ret);
        return -1;
    }

    if ((ss > 0) && (ss < (sizeof(shutdown_source)/sizeof(char*)))) {
        strcpy(shutdownsource, shutdown_source[ss]);
        LOGI("shutdown source (%d): %s", ss, shutdownsource);
        return 1;
    } else {
        return 0;
    }
}
#endif // CONFIG_EFILINUX

/**
 * @brief generate WDT crash event
 *
 * If startup reason contains HWWDT or SWWDT, generate a WDT crash event.
 * Copy aplogs and /proc/last_kmsg files in the event.
 *
 * @param reason - string containing the translated startup reason
 */
int crashlog_check_startupreason(char *reason, char *watchdog) {
    const char *dateshort = get_current_time_short(1);
    char destination[PATHMAX];
    int dir;
    char *key;

    /* Nothing to do if the reason :
     *  - doesn't contains "HWWDT" or "SWWDT" or "WDT" */
    /*  - contains "WDT" but with "FAKE" suffix */
    if ( !( strstr(reason, "WDT_") ) || strstr(reason, "FAKE") || !( strstr(watchdog, "UNHANDLED") ) ) {
        /* Nothing to do */
        return 0;
    }

    dir = find_new_crashlog_dir(MODE_CRASH);
    if (dir < 0) {
        LOGE("%s: find_new_crashlog_dir failed\n", __FUNCTION__);
        key = raise_event(CRASHEVENT, watchdog, NULL, NULL);
        LOGE("%-8s%-22s%-20s%s\n", CRASHEVENT, key, get_current_time_long(0), "WDT");
        free(key);
        return -1;
    }

    destination[0] = '\0';
    snprintf(destination, sizeof(destination), "%s%d/", CRASH_DIR, dir);
    key = raise_event_wdt(CRASHEVENT, watchdog, reason, destination);
    LOGE("%-8s%-22s%-20s%s %s\n", CRASHEVENT, key, get_current_time_long(0), "WDT", destination);
    flush_aplog(APLOG_BOOT, "WDT", &dir, dateshort);
    usleep(TIMEOUT_VALUE);
    do_log_copy("WDT", dir, dateshort, APLOG_TYPE);
    do_last_kmsg_copy(dir);
    do_last_fw_msg_copy(dir);
    free(key);

    return 0;
}
