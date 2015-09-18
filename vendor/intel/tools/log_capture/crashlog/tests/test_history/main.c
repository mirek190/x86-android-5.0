#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <cutils/properties.h>

#include <crashutils.h>
#include <history.h>
#include <fsutils.h>
#include <config.h>

#include "test_framework.h"

#define LOGE printf
#define do_chmod(...)

/*
int update_history_file(struct history_entry *entry);
int reset_uptime_history(char *lastuptime);
int history_has_event(char *eventdir);
int reset_history_cache();
*/
static void history_file_write(char *event, char *type, char *subtype, char *log, char* lastuptime, char* key, char* date_tmp_2);

void test_update_history_file(struct history_entry *entry, int expect) {
    int res;

    res = update_history_file(entry);
    if (res == expect) printf("%s with entry.log=%s succeeded\n", __FUNCTION__, (entry ? entry->log : "(null)"));
    else printf("%s entry.log=%s failed; returned %d\n", __FUNCTION__, (entry ? entry->log : "(null)"), res);
}

void test_history_file_write(struct history_entry *entry, int expect) {
    int res = expect;

    history_file_write(entry->event, entry->type, entry->type,
		entry->log, (char *)entry->lastuptime, entry->key, entry->eventtime);
    if (res == expect) printf("%s with entry.log=%s succeeded\n", __FUNCTION__, (entry ? entry->log : "(null)"));
    else printf("%s entry.log=%s failed; returned %d\n", __FUNCTION__, (entry ? entry->log : "(null)"), res);
}

void test_reset_uptime_history(int expect) {
    int res;

    res = reset_uptime_history();
    if (res == expect) printf("%s succeeded\n", __FUNCTION__);
    else printf("%s failed; returned %d\n", __FUNCTION__, res);
}

void test_history_has_event(char *eventdir, int expect) {
    int res;

    res = history_has_event(eventdir);
    if (res == expect) printf("%s (%s) succeeded\n", __FUNCTION__, eventdir);
    else printf("%s (%s) failed; returned %d\n", __FUNCTION__, eventdir, res);
}

void test_reset_history_cache(int expect) {
    int res;

    res = reset_history_cache();
    if (res == expect) printf("%s succeeded\n", __FUNCTION__);
    else printf("%s failed; returned %d\n", __FUNCTION__, res);
}

static void history_file_write(char *event, char *type, char *subtype, char *log, char* lastuptime, char* key, char* date_tmp_2)
{
    char uptime[32];
    struct stat info;
    long long tm=0;
    int hours, seconds, minutes;
    FILE *to;
    char tmp[PATHMAX];
    char * p;

    // compute subtype
    if (!subtype)
        subtype = type;

    // compute uptime
    tm = get_uptime(1, &hours);
    hours = (int) (tm / 1000000000LL);
    seconds = hours % 60;
    hours /= 60;
    minutes = hours % 60;
    hours /= 60;
    snprintf(uptime,sizeof(uptime),"%04d:%02d:%02d",hours, minutes,seconds);

    if (stat(HISTORY_FILE, &info) != 0) {
        to = fopen(HISTORY_FILE, "w");
        if (to == NULL){
            LOGE("can not open file: %s\n", HISTORY_FILE);
            return;
        }
        do_chmod(HISTORY_FILE, "644");
        do_chown(HISTORY_FILE, PERM_USER, PERM_GROUP);
        fprintf(to, "#V1.0 %-16s%-24s\n", UPTIME_EVNAME, uptime);
        fprintf(to, "#EVENT  ID                    DATE                 TYPE\n");
        fclose(to);
    }

    if (log != NULL) {
        snprintf(tmp, sizeof(tmp), "%s", log);
        if((p = strrchr(tmp,'/'))){
            p[0] = '\0';
        }
        to = fopen(HISTORY_FILE, "a");
        if (to == NULL){
            LOGE("can not open file: %s\n", HISTORY_FILE);
            return;
        }
        fprintf(to, "%-8s%-22s%-20s%s %s\n", event, key, date_tmp_2, type, tmp);
        fclose(to);
        if (!strncmp(event, "CRASH", 5))
            printf("create_minimal_crashfile(subtype, tmp, key, uptime, date_tmp_2);");
        else if(!strncmp(event, "BZ", 2))
            printf("create_minimal_crashfile(event, tmp, key, uptime, date_tmp_2);");
    } else if (type != NULL) {

        to = fopen(HISTORY_FILE, "a");
        if (to == NULL){
            LOGE("can not open file: %s\n", HISTORY_FILE);
            return;
        }
        if (lastuptime != NULL)
            fprintf(to, "%-8s%-22s%-20s%-16s %s\n", event, key, date_tmp_2, type, lastuptime);
        else
            fprintf(to, "%-8s%-22s%-20s%-16s\n", event, key, date_tmp_2, type);
        fclose(to);
    } else {

        to = fopen(HISTORY_FILE, "a");
        if (to == NULL){
            LOGE("can not open file: %s\n", HISTORY_FILE);
            return;
        }
        fprintf(to, "%-8s%-22s%-20s%s\n", event, key, date_tmp_2, lastuptime);
        fclose(to);
    }
    return;
}

int main(int __attribute__((unused)) argc, char __attribute__((unused)) **argv) {
    struct history_entry entry;
    
    entry.event = "my_ev";
    entry.type = "my_type";
    entry.log = strdup("/logs/my_log");
    entry.lastuptime = "my_lastuptime";
    entry.key = "my_key";
    entry.eventtime = "my_time";
    
    test_update_history_file(NULL, -EINVAL);
    system("rm -f res/logs/history_event");
    test_update_history_file(&entry, 0);
    system("touch res/logs/history_event");
    test_update_history_file(&entry, 0);
    free(entry.log);
    entry.log = strdup("my_log");
    test_update_history_file(&entry, 0);
    entry.eventtime = NULL;
    test_update_history_file(&entry, -EINVAL);
    
    test_history_file_write(&entry, 0);
    free(entry.log);
    
    test_reset_uptime_history(0);
    system("rm -f res/logs/history_event");
    test_reset_history_cache(2); // two head lines
    test_reset_uptime_history(0);
    system("cp res/history_event.base res/logs/history_event");
    test_reset_uptime_history(0);
    
    system("cp res/history_event.base res/logs/history_event");
    test_reset_history_cache(45);
    test_history_has_event("e23eb993705d3199db53", 1);
    test_history_has_event("toto", 0);
    system("rm -f res/logs/history_event");
    test_reset_history_cache(2);
    test_history_has_event("toto", 0);
    
    return 0;
}
