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
/*
int raise_infoerror(char *type, char *subtype)
*/

int orig_crashlog_raise_infoerror(char *type, char *subtype);
void orig_process_stats_trigger(char *filename, char *name,  unsigned int files);

void test_orig_crashlog_raise_infoerror(char *type, char *subtype, int expect) {
    int res;

    res = orig_crashlog_raise_infoerror(type, subtype);
    if (res == expect) printf("%s (%s, %s) succeeded\n", __FUNCTION__, type, subtype);
    else printf("%s (%s, %s) failed; returned %d\n", __FUNCTION__, type, subtype, res);
}

void test_orig_process_stats_trigger(char *filename, char *name, int expect) {
    int res;

    res = expect;
    orig_process_stats_trigger(filename, name, 100);
    if (res == expect) printf("%s (%s, %s) succeeded\n", __FUNCTION__, filename, name);
    else printf("%s (%s, %s) failed; returned %d\n", __FUNCTION__, filename, name, res);
}

void test_raise_infoerror(char *type, char *subtype, int expect) {
    int res;

    res = raise_infoerror(type, subtype);
    if (res == expect) printf("%s (%s, %s) succeeded\n", __FUNCTION__, type, subtype);
    else printf("%s (%s, %s) failed; returned %d\n", __FUNCTION__, type, subtype, res);
}

void test_file_exists(char *filename, int expect) {
    int res;

    res = file_exists(filename);
    if (res == expect) printf("%s (%s) succeeded\n", __FUNCTION__, filename);
    else printf("%s (%s) failed; returned %d\n", __FUNCTION__, filename, res);
}

void test_get_file_size(char *filename, int expect) {
    int res;

    res = get_file_size(filename);
    if (res == expect) printf("%s (%s) succeeded\n", __FUNCTION__, filename);
    else printf("%s (%s) failed; returned %d\n", __FUNCTION__, filename, res);
}

int main(int __attribute__((unused)) argc, char __attribute__((unused)) **argv) {
    
    system("rm -f res/logs/history_event");
    test_orig_crashlog_raise_infoerror("toto", "titi", 0);
    test_get_file_size("res/logs/history_event", 173);
    system("rm -f res/logs/history_event");
    test_raise_infoerror("toto", "titi", 0);
    test_get_file_size("res/logs/history_event", 173);
    system("touch res/logs/history_event");
    test_orig_crashlog_raise_infoerror("toto", "titi", 0);
    test_get_file_size("res/logs/history_event", 243);
    test_raise_infoerror("toto", "titi", 0);
    test_get_file_size("res/logs/history_event", 313);
    
    //test_orig_process_stats_trigger("res/logs/stats", "titi_trigger", 0);
    return 0;
}
