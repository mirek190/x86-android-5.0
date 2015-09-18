/* Copyright (C) Intel 2014
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
 * @file ingredients.c
 * @brief File containing functions used to handle operations on
 * ingredients.txt file.
 *
 */

#include "ingredients.h"
#include "privconfig.h"
#include "fsutils.h"
#include "config.h"

#ifdef CONFIG_EFILINUX
#include <libdmi.h>
#endif

#define UNDEF_INGR "unknown"
#define INGREDIENT_VALUE_MAX_SIZE   PROPERTY_VALUE_MAX

static bool ingredients_disabled = FALSE;

static pconfig_handle parse_ingredients_file(const char *file_path) {
    pconfig_handle pc_handle;

    pc_handle = malloc(sizeof(struct config_handle));
    if (!pc_handle)
        return NULL;

    pc_handle->first = NULL;
    pc_handle->current = NULL;

    if (init_config_file(file_path, pc_handle) < 0) {
        free_config_file(pc_handle);
        free(pc_handle);
        LOGE("[INGR] Cannot load ingredients config, path = %s", file_path);
        return NULL;
    }

    return pc_handle;
}

static void check_config(pconfig_handle handle) {
    psection c_psection;
    pkv c_kv;

    c_psection = handle->first;
    while (c_psection) {
        c_kv = c_psection->kvlist;
        while (c_kv) {
            if (strncmp(c_kv->value, "true", strlen("true")) &&
                strncmp(c_kv->value, "false", strlen("false")))
                LOGW("[INGR] Invalid key value (%s) for %s",
                     c_kv->value, c_kv->key);
            c_kv = c_kv->next;
        }
        c_psection = c_psection->next;
    }
}

#ifdef CONFIG_EFILINUX
static int fetch_dmi_properties(psection section) {
    int status = 1;
    char *property;
    pkv kv = section->kvlist;
    while (kv) {
        property = libdmi_getfield(INTEL_SMBIOS, kv->key);

        free(kv->value);

        if (!property) {
            kv->value = strdup(UNDEF_INGR);
            status = 0;
        } else {
            kv->value = strdup(property);
        }
        kv = kv->next;
    }
    return status;
}
#else
static int fetch_dmi_properties(psection section __unused) {
    return 0;
}
#endif

static int fetch_android_properties(psection section) {
    int status = 1;
    char buffer[INGREDIENT_VALUE_MAX_SIZE];
    pkv kv = section->kvlist;
    while (kv) {
        if (property_get(kv->key, buffer, UNDEF_INGR) < 0) {
            status = 0;
        }
        free(kv->value);
        kv->value = strdup(buffer);

        kv = kv->next;
    }
    return status;
}

static int get_modem_property(char *property_name, char *value) {
    int status = 0;
    /*make shure we start from UNDEF_INGR */
    strcpy(value, UNDEF_INGR);
    if (!strncmp(property_name, "Modem", strlen("Modem") + 1)) {
        read_file_prop_uid(MODEM_FIELD, MODEM_UUID, value, UNDEF_INGR);
        if (!strcmp(value, UNDEF_INGR))
            status = -1;
    }

    if (!strncmp(property_name, "ModemExt", strlen("ModemExt") + 1)) {
        read_file_prop_uid(MODEM_FIELD2, MODEM_UUID2, value, UNDEF_INGR);
        if (!strcmp(value, UNDEF_INGR))
            status = -1;
    }

    if (status < 0)
        LOGW("Cannot get modem prop - %s", property_name);

    return status;
}

static int fetch_modem_properties(psection section) {
    char buffer[INGREDIENT_VALUE_MAX_SIZE+1];
    pkv kv = section->kvlist;
    LOGW("[INGR]: modem Props ");
    while (kv) {
        get_modem_property(kv->key, buffer);
        buffer[INGREDIENT_VALUE_MAX_SIZE] = 0;
        free(kv->value);
        kv->value = strdup(buffer);

        kv = kv->next;
    }
    return 1;
}

static int fetch_ingredients(pconfig_handle handle) {
    psection c_psection;
    int fetch_result = 1;

    c_psection = handle->first;
    while (c_psection) {
        if (!strncmp(c_psection->name, "LIBDMI", strlen("LIBDMI"))
            && fetch_dmi_properties(c_psection))
            fetch_result = 0;

        if (!strncmp(c_psection->name, "GETPROP", strlen("GETPROP"))
            && fetch_android_properties(c_psection) <= 0)
            fetch_result = 0;

        if (!strncmp(c_psection->name, "MODEM", strlen("MODEM"))
            && fetch_modem_properties(c_psection) <= 0)
            fetch_result = 0;

        c_psection = c_psection->next;
    }
    return fetch_result;
}

void check_ingredients_file() {
    static pconfig_handle old_values = NULL;
    pconfig_handle new_values = NULL;

    if (ingredients_disabled)
        return;

    if (!file_exists(INGREDIENTS_CONFIG)) {
        LOGE("[INGR]: File '%s' not found, disable 'ingredients' feature\n",
             INGREDIENTS_CONFIG);
        ingredients_disabled = TRUE;
        return;
    }

    if (!old_values) {
        /*first run, load last ingredients.txt */
        old_values = parse_ingredients_file(INGREDIENTS_FILE);
    }

    new_values = parse_ingredients_file(INGREDIENTS_CONFIG);

    if (!new_values) {
        LOGW("[INGR]: Cannot load config file");
        return;
    }

    check_config(new_values);

    fetch_ingredients(new_values);

    /*old vs new */
    if (cmp_config(old_values, new_values)) {

        LOGI("[INGR]: Updating %s", INGREDIENTS_FILE);
        if (dump_config(INGREDIENTS_FILE, new_values)) {
            LOGI("[INGR]: Cannot update %s", INGREDIENTS_FILE);
            if (new_values) {
                free_config_file(new_values);
                free(new_values);
            }
        } else {
            if (old_values) {
                free_config_file(old_values);
                free(old_values);
            }
            old_values = new_values;
            do_chmod(INGREDIENTS_FILE, "644");
            do_chown(INGREDIENTS_FILE, PERM_USER, PERM_GROUP);
        }
    } else if (new_values) {
        LOGI("[INGR]: No diff between %p and %p", old_values, new_values);
        free_config_file(new_values);
        free(new_values);
    }
}
