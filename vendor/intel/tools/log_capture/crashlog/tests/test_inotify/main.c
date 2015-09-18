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

#include <inotify_handler.h>
#include <config.h>

#include "test_framework.h"

/*
 * int init_inotify_handler();
 * int set_watch_entry_callback(unsigned int watch_type, inotify_callback pcallback);
 * int handle_inotify_events(int inotify_fd);
*/

int finalize_dropbox_pending_event(const struct inotify_event __attribute__((unused)) *event) {
    return 0;
}

int gevdetected = -1;

int dummy_callback(struct watch_entry *entry, struct inotify_event *event) {
    printf("%s: entry(%s, %s), event(%s)\n", __FUNCTION__,
        entry->eventname, entry->eventpath, event->name);
    gevdetected = entry->eventtype;
    return 0;
}

void test_set_watch_entry_callback(unsigned int watch_type, inotify_callback pcallback, int expect) {
    int res;

    res = set_watch_entry_callback(watch_type, pcallback);
    if (res == expect) printf("%s (%d) succeeded\n", __FUNCTION__, watch_type);
    else printf("%s (%d) failed; returned %d\n", __FUNCTION__, watch_type, res);
}

void test_handle_inotify_events(int fd, int expect, int evdetected) {
    int res;
    
    gevdetected = -1;
    res = receive_inotify_events(fd);
    if (res == expect && gevdetected == evdetected)
        printf("%s (%d) succeeded\n", __FUNCTION__, fd);
    else printf("%s (%d) failed; returned %d and event detected is %d\n", 
        __FUNCTION__, fd, res, gevdetected);
}

int main(int __attribute__((unused)) argc, char __attribute__((unused)) **argv) {
    int fd = init_inotify_handler();
    if ( fd < 0 ) {
        printf("init_inotify_handler failed; returned %d\n", fd);
        return -1;
    }
    
    /* We're monitoring DROPBOX_DIR_MASK */
    test_set_watch_entry_callback(SYSSERVER_TYPE, dummy_callback, 0);
    test_handle_inotify_events(fd, -EAGAIN, -1);
    system("touch " DROPBOX_DIR "/toto");
    test_handle_inotify_events(fd, 0, -1);
    system("touch " DROPBOX_DIR "/system_server_watchdog@jshdfkgj.txt");
    test_handle_inotify_events(fd, 0, SYSSERVER_TYPE);
    system("touch " DROPBOX_DIR "/anr@jshdfkgj.txt");
    test_handle_inotify_events(fd, 0, -1);
    test_set_watch_entry_callback(ANR_TYPE, dummy_callback, 0);
    system("touch " DROPBOX_DIR "/anr@jshdfkgj2.txt");
    test_handle_inotify_events(fd, 0, ANR_TYPE);
    system("rm " DROPBOX_DIR "/anr@jshdfkgj2.txt");
    test_handle_inotify_events(fd, -EAGAIN, -1);
    
    /* Cleanup the tmp files */
    system("rm -fr " DROPBOX_DIR "/*");
    
    return 0;
}
