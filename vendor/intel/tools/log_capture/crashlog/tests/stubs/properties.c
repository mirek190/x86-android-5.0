#include "cutils/properties.h"
#include "cutils/log.h"

#include "fsutils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

typedef struct {
        char key[PROPERTY_KEY_MAX];
        char value[PROPERTY_VALUE_MAX];
} stubproperty;

/* 0ed initialize array of properties */
static stubproperty cache[MAX_PROPERTIES] = { {{0,}, {0,}}, };

int file_to_cache() {
    int fd, res, idx = 0, len;
    char tmpbuf[MAXLINESIZE];
    char *pvalue;
    
    fd = open(DEFAULT_PROPS, O_RDONLY);
    if (fd < 0) return -errno;
    
    while ( idx < MAX_PROPERTIES && (res = readline(fd, tmpbuf)) > 0) {
        len = strlen(tmpbuf);
		if (tmpbuf[len-1] == '\n') tmpbuf[len-1] = 0;
        pvalue = strchr(tmpbuf, '=');
        if (pvalue == NULL) {
            /* bad line or format, ignore it! */
            continue;
        }
        /* Split the line at the first = and shift the pointer */
        pvalue[0] = 0;pvalue++;
        strncpy(cache[idx].key, tmpbuf, PROPERTY_KEY_MAX);
        strncpy(cache[idx].value, pvalue, PROPERTY_VALUE_MAX);
        /*printf("%s: cache[%d]=[key=\"%s\", value=\"%s\"]\n",
            __FUNCTION__, idx, cache[idx].key, cache[idx].value);*/
        idx++;
    }
    return res;
}

int cache_to_file() {
    int res = 1, idx = 0;
    FILE *fd;
    
    fd = fopen(DEFAULT_PROPS, "w");
    if (fd == NULL) return -errno;
    
    while (res > 0 && idx < MAX_PROPERTIES && cache[idx].key[0] != 0) {
        res = fprintf(fd, "%s=%s\n", cache[idx].key, cache[idx].value);
        idx++;
    }
    return (res > 0 ? 0 : -errno); 
}

int reset_property_cache() {
    int idx = 0;
    
    /* 0 the cache keys */
    while (idx < MAX_PROPERTIES && cache[idx].key != NULL) {
        cache[idx].key[0] = 0;
        idx++;
    }
    return file_to_cache();
}

int property_get(char *name, char *value, char *def) {

    int idx = 0;
    
    if (!name || !value) return -EINVAL;
    
    if (cache[0].key == NULL && file_to_cache() < 0) return -errno;
    
    while (idx < MAX_PROPERTIES && cache[idx].key[0] != 0) {
        /*printf("%s: Compare %s to cache[%d]=[key=\"%s\", value=\"%s\"]\n",
            __FUNCTION__, name, idx, cache[idx].key, cache[idx].value);*/
        if ( !strncmp(cache[idx].key, name, PROPERTY_KEY_MAX) ) {
            strncpy(value, cache[idx].value, PROPERTY_VALUE_MAX);
            return strlen(value);
        }
        idx++;
    }
    if (def != NULL) {
        strncpy(value, def, PROPERTY_VALUE_MAX);
        return strlen(value);
    }
    return -1;
}

int property_set(char *name, char *value) {

    int idx = 0;
    
    if (!name || !value) return -EINVAL;
    
    if (cache[0].key == NULL && file_to_cache() < 0) return -errno;
    
    while (idx < MAX_PROPERTIES && cache[idx].key[0] != 0) {
        if ( !strncmp(cache[idx].key, name, PROPERTY_KEY_MAX) ) {
            strncpy(cache[idx].value, value, PROPERTY_VALUE_MAX);
            break;
        }
    }
    if (idx < MAX_PROPERTIES && cache[idx].key[0] != 0) {
        return cache_to_file();
    }
    /* property not found */
    return -1;
}
