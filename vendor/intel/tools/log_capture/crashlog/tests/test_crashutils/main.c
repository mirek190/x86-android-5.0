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
#include <sha1.h>

#include <crashutils.h>
#include <history.h>
#include <config.h>

#include "test_framework.h"

/*
unsigned long long get_uptime(int refresh, int *error); // ignored
void restart_profile_srv(int serveridx);                // ignored
void check_running_power_service();                     // ignored
void notify_crashreport();                              // ignored
char **commachain_to_fixedarray(char *chain,
        unsigned int recordsize, unsigned int maxrecords, int *res);
int raise_event(char *event, char *type, char *subtype,
    char *log);
*/

void test_commachain_to_fixedarray(char *chain, unsigned int recordsize, unsigned int maxrecords, int expect) {
    char **array;
    int res;
    unsigned int idx;

    array = commachain_to_fixedarray(chain, recordsize, maxrecords, &res);
    if (res == expect) printf("%s (%s, %u, %u) succeeded\n", __FUNCTION__, chain, recordsize, maxrecords);
    else printf("%s (%s, %u, %u) failed; returned %d\n", __FUNCTION__, chain, recordsize, maxrecords, res);
    if (!array) return;
    
    for (idx = 0 ; idx < maxrecords ; idx++) {
        free(array[idx]);
    }
    free(array);
}

void test_raise_event(char *event, char *type, char *subtype,
    char *log, int validptr_expected, int errno_expected) {
    char *res;

    res = raise_event(event, type, subtype, log);
    if ( ((!res && !validptr_expected) || (res && validptr_expected)) && errno == errno_expected)
        printf("%s (%s, %s, %s, %s) succeeded\n", __FUNCTION__, event, type, subtype, log);
    else printf("%s (%s, %s, %s, %s) failed; returned %p and errno=%d\n", __FUNCTION__, event, type, subtype, log, res, errno);
    if (res) free(res);
}

int main(int __attribute__((unused)) argc, char __attribute__((unused)) **argv) {
    
    test_commachain_to_fixedarray("lkjlkj;kjlk,iuin", 20, 20, 2);
    test_commachain_to_fixedarray("lkjlkj;kjlk;iuin", 20, 20, 3);
    test_commachain_to_fixedarray("lkjlkj;kjlk;iuin", 20, 1, 2);
    test_commachain_to_fixedarray("lkjlkj;kjlk,iuin;;", 20, 20, 2);
    test_commachain_to_fixedarray(NULL, 20, 20, -EINVAL);
    test_commachain_to_fixedarray("lkjlkj;kjlk,iuin;;", 0, 20, -EINVAL);
    test_commachain_to_fixedarray("lkjlkj;kjlk,iuin;;", 20, 0, 0);
    test_commachain_to_fixedarray("lkjlkj;kjlk,iuin;;", 20, -1, -ENOMEM); /* unsigned int so 0xffffffff as max */
    test_commachain_to_fixedarray("lkjlkj;kjlk,iuin;;", 5, 5, 2); /* len of the first extracted shall be caped to 5*/
    
    system("rm -f res/logs/history_event");
    system("rm -fr res/logs/log1");
    system("rm -fr res/logs/log2");
    test_raise_event("myev1", "mytype1", "mysubtype1", "res/logs/log1", 0, ENOENT);
    system("touch res/logs/history_event");
    test_raise_event("myev2", "mytype2", "mysubtype2", "res/logs/log2", 0, ENOENT);
    
    system("rm -f res/logs/history_event");
    system("mkdir res/logs/log1");
    system("mkdir res/logs/log2");
    test_raise_event("myev1", "mytype1", "mysubtype1", "res/logs/log1", 1, 0);
    system("touch res/logs/history_event");
    test_raise_event("myev2", "mytype2", "mysubtype2", "res/logs/log2", 1, 0);
    system("rm -fr res/logs/log1");
    system("rm -fr res/logs/log2");

    return 0;
}

