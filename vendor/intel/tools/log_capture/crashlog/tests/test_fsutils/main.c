#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include <cutils/properties.h>

#include <crashutils.h>
#include <fsutils.h>

#include "test_framework.h"

/*
int cache_file(char *file, char **records, int maxrecords, int cachemode);
int file_exists(char *filename);
int get_file_size(char *filename);
int do_chown(char *file, char *uid, char *gid);
int do_copy_tail(char *src, char *dest, int limit);
int find_matching_file(char *dir_to_search, char *pattern, char *filename_found);
int readline(int fd, char buffer[MAXLINESIZE]); -- tested with cache_file
int find_str_in_file(char *file, char *keyword, char *tail);
int find_oneofstrings_in_file(char *file, char **keywords, int nbkeywords);
int append_file(char *filename, char *text);
void get_sdcard_paths(int mode);
int find_new_crashlog_dir(int mode);
*/

int raise_infoerror(char *type, char *subtype) {
    printf("LOGE: %s: type:%s subtype:%s\n", __FUNCTION__, type, subtype);
    return 0;
}

#define CACHE_NBROWS  12

void dump_cachefile(char **cachefile, int nbrecords) {
    int idx;
    
    for (idx = 0 ; idx < nbrecords ; idx++) {
        if (cachefile[idx] == NULL) printf("line %d: empty\n", idx);
        else if (cachefile[idx][0] == 0)  printf("line %d: null string\n", idx);
        else printf("line %d: %s", idx, cachefile[idx]);
    }
}

void test_cache_file(char *testfile, int cachemode, int expect) {
    char *cachefile[CACHE_NBROWS];
    int res;

    res = cache_file(testfile, (char**)cachefile, CACHE_NBROWS, cachemode, 0);
    if (res == expect) printf("%s with (%s, %s) succeeded\n",
		__FUNCTION__, testfile,
			(cachemode == CACHE_START ? "CACHE_START" : "CACHE_TAIL"));
    else printf("%s with (%s, %s) failed; returned %d\n",
		__FUNCTION__, testfile,
			(cachemode == CACHE_START ? "CACHE_START" : "CACHE_TAIL"), res);
    //if (res > 0) dump_cachefile((char**)cachefile, CACHE_NBROWS);    
}

void test_file_exists(char *filename, int expect) {
    int res;

    res = file_exists(filename);
    if (res == expect) printf("%s with %s succeeded\n", __FUNCTION__, filename);
    else printf("%s with %s failed; returned %d\n", __FUNCTION__, filename, res);
}

void test_file_size(char *filename, int expect) {
    int res;

    res = get_file_size(filename);
    if (res == expect) printf("%s with %s succeeded\n", __FUNCTION__, filename);
    else printf("%s with %s failed; returned %d\n", __FUNCTION__, filename, res);
}

void test_do_chown(char *filename, char *uid, char *gid, int expect) {
	int res;
	
	res = do_chown(filename, uid, gid);
    if (res == expect) printf("%s with (%s, %s, %s) succeeded\n", __FUNCTION__, filename, uid, gid);
    else printf("%s with (%s, %s, %s) failed; returned %d\n", __FUNCTION__, filename, uid, gid, res);
}

void test_do_copy_tail(char *src, int limit, int expect) {
	int res;
	char *dest;
	
	if (src) {
		dest = (char*)malloc(strlen(src)+5);
		sprintf(dest, "%s_copy", src);
	}
	else
		dest = "dont matter";
	if (! dest) {
		printf("%s with (%s, %d) cannot be tested; malloc failed\n",
			__FUNCTION__, src, limit);
		return;
	}
	
	res = do_copy_tail(src, dest, limit);
    if (res == expect) printf("%s with (%s, %d) succeeded\n",
			__FUNCTION__, src, limit);
    else printf("%s with (%s, %d) failed; returned %d\n",
			__FUNCTION__, src, limit, res);
	
	if (src)
		free(dest);
}

void test_find_matching_file(char *dir, char *pattern, int expect) {
	int res;
	char buffer[64];
	
	res = find_matching_file(dir, pattern, buffer);
    if (res == expect) {
		if (res == 1)
			printf("%s with (%s, %s) succeeded and returned %s\n", __FUNCTION__, dir, pattern, buffer);
		else printf("%s with (%s, %s) succeeded\n", __FUNCTION__, dir, pattern);
	}
    else printf("%s with (%s, %s) failed; returned %d\n", __FUNCTION__, dir, pattern, res);
}

void test_find_str_in_file(char *file, char *keyword, char *tail, int expect) {
	int res;
	
	res = find_str_in_file(file, keyword, tail);
    if (res == expect) {
		printf("%s with (%s, %s, %s) succeeded\n", __FUNCTION__, file, keyword, tail);
	}
    else printf("%s with (%s, %s, %s) failed; returned %d\n", __FUNCTION__, file, keyword, tail, res);
}

void test_find_oneofstrings_in_file(char *file, char **keywords,
		int nbkeywords, int expect) {
	int res;
	
	res = find_oneofstrings_in_file(file, keywords, nbkeywords);
    if (res == expect) {
		printf("%s with (%s, %s, %d) succeeded\n", __FUNCTION__, file, (keywords ? "[keywords]" : NULL), nbkeywords);
	}
    else printf("%s with (%s, %s, %d) failed; returned %d\n", __FUNCTION__, file, (keywords ? "[keywords]" : NULL), nbkeywords, res);
}

void test_append_file(char *filename, char *text, int expect) {
    int res;

    res = append_file(filename, text);
    if (res == expect) printf("%s with (%s, %s) succeeded\n", __FUNCTION__, filename, text);
    else printf("%s with (%s, %s) failed; returned %d\n", __FUNCTION__, filename, text, res);
}

void test_get_sdcard_paths(int mode, int expect) {
    int res;
    
	res = get_sdcard_paths(mode);
    if (res == expect) printf("%s with (%d) succeeded\n", __FUNCTION__, mode);
    else printf("%s with (%d) failed; returned %d\n", __FUNCTION__, mode, res);
}

void test_find_new_crashlog_dir(int mode, int expect) {
    int res;
    
	res = find_new_crashlog_dir(mode);
    if (res == expect) printf("%s with (%d, %d) succeeded\n", __FUNCTION__, gmaxfiles, mode);
    else printf("%s with (%d, %d) failed; returned %d\n", __FUNCTION__, gmaxfiles, mode, res);
}

void test_file_read_string(const char *file, char *buffer, int expect) {
    int res;

    res = file_read_string(file, buffer);
    if (res == expect)
        printf("%s with (%s, %s) : %d : succeeded\n", __FUNCTION__, file, buffer, res);
    else
        printf("%s with (%s, %s) failed; returned %d\n", __FUNCTION__, file, buffer, res);
}

int main(int __attribute__((unused)) argc, char __attribute__((unused)) **argv) {

    char *missing_2props[] = {"testprop45", "testprop451"};
    char *one_missing_3props[] = {"testprop45","testprop0","testprop1",};
    char *one_in_3props[] = {"testprop451", "testprop0", "testprop452"};
    char *in_3props[] = {"testprop0", "testprop1", "testprop2"};
    char buffer[64] = {'\0',};

    test_cache_file("res/cache_file_missing", CACHE_START, -ENOENT);
    test_cache_file("res/cache_file_empty", CACHE_START, 0);
    test_cache_file("res/cache_file_tooshort", CACHE_START, 7);
    test_cache_file("res/cache_file_longer", CACHE_START, 12);
    test_cache_file(NULL, CACHE_START, -EINVAL);
    test_cache_file("res/cache_file_missing", CACHE_TAIL, -ENOENT);
    test_cache_file("res/cache_file_empty", CACHE_TAIL, 0);
    test_cache_file("res/cache_file_tooshort", CACHE_TAIL, 7);
    test_cache_file("res/cache_file_longer", CACHE_TAIL, 12);
    test_cache_file("res/cache_file_twicelines", CACHE_TAIL, 12);
    test_cache_file(NULL, CACHE_TAIL, -EINVAL);

    test_file_exists("res/cache_file_missing", 0);
    test_file_exists("res/cache_file_empty", 1);
    test_file_exists("/root/kjsdfjksdfgj", 0);
    test_file_exists(NULL, 0);

    test_file_size("res/cache_file_missing", -ENOENT);
    test_file_size("res/cache_file_empty", 0);
    test_file_size("res/cache_file_tooshort", 180);
    test_file_size(NULL, -ENOENT);

    /* The following tests are not fully checked under linux
    * except if you run it as root and change the stub in fsutils.c
    */
    /* TODO(jer) fix it */
    /* test_do_chown("res/cache_file_missing", "system", "log", -ENOENT); */
    /* test_do_chown("res/cache_file_empty", "system", "log", 0); */
    /* test_do_chown(NULL, "system", "log", -ENOENT); */
    /* test_do_chown("res/cache_file_empty", NULL, "log", -EINVAL); */
    /* test_do_chown("res/cache_file_empty", "system", NULL, -EINVAL); */

    test_do_copy_tail("res/cache_file_empty", 10, 0);
    test_do_copy_tail("res/cache_file_tooshort", 10, 10);
    test_do_copy_tail("res/cache_file_tooshort", 1000, 180);
    test_do_copy_tail(NULL, 1000, -EINVAL);
    test_do_copy_tail("res/cache_file_tooshort", 0, 180);
    test_do_copy_tail("res/cache_fissle_tooshort", 10, -ENOENT);

    test_find_matching_file("res", "tooshort", 1);
    test_find_matching_file("res", "missing", 0);
    test_find_matching_file("missing", "missing", -ENOENT);
    test_find_matching_file("res", NULL, -EINVAL);
    test_find_matching_file(NULL, "missing", -EINVAL);

    test_find_str_in_file("res/content_str_in_file.txt", "testprop0", NULL, 1);
    test_find_str_in_file("res/content_str_in_file.txt", "testprop0", "lateteatoto", 1);
    test_find_str_in_file("res/content_str_in_file.txt", "testprop0", "lateteatiti", 0);
    test_find_str_in_file(NULL, "testprop0", "lateteatoto", -EINVAL);
    test_find_str_in_file("res/content_str_in_file.txt", NULL, "lateteatiti", -EINVAL);
    test_find_str_in_file("res/content_str_in_file.txt", "testprop1", "lateteatoto", 0);
    test_find_str_in_file("res/content_str_in_file.txt", "testprop1", "\"lateteatoto\"", 1);
    test_find_str_in_file("res/content_str_in_file.txt", "testprop2", "\"lateteatoto\"", 0);
    test_find_str_in_file("res/content_str_in_file.txt", "testprop3", NULL, 1);
    test_find_str_in_file("res/content_str_in_file.txt", "testprop3", "\"lateteatoto\"", 0);
    test_find_str_in_file("res/propertiess.txt", "testprop3", "\"lateteatoto\"", -ENOENT);

    test_find_oneofstrings_in_file("res/content_str_in_file.txt", missing_2props, 2, 0);
    test_find_oneofstrings_in_file("res/content_str_in_file.txt", one_missing_3props, 3, 1);
    test_find_oneofstrings_in_file("res/content_str_in_file.txt", one_in_3props, 3, 1);
    test_find_oneofstrings_in_file("res/content_str_in_file.txt", in_3props, 3, 1);
    test_find_oneofstrings_in_file("res/content_str_in_file.txt", NULL, 0, -EINVAL);
    test_find_oneofstrings_in_file(NULL, NULL, 0, -EINVAL);
    test_find_oneofstrings_in_file(NULL, in_3props, 0, -EINVAL);
    test_find_oneofstrings_in_file("res/proddperties.txt", in_3props, 3, -ENOENT);
    test_find_oneofstrings_in_file("res/content_str_in_file.txt", in_3props, 0, -EINVAL);

    system("rm -f res/file_to_append && touch res/file_to_append");
    test_append_file("res/file_to_append", "text", 4);
    test_append_file("res/file_to_appen", "text", -ENOENT);
    test_append_file("res/file_to_append", NULL, -EINVAL);
    test_append_file(NULL, "text", -EINVAL);

    /* TODO(jer) fix it */
    /* system("rm -fr res/mnt/sdcard && rm -f res/properties.txt"); */
    /* test_get_sdcard_paths(MODE_CRASH, -ENOENT); */
    /* system("echo persist.sys.crashlogd.mode=lowmemory > res/properties.txt"); */
    /* reset_property_cache(); */
    /* test_get_sdcard_paths(MODE_CRASH, 0); */
    /* system("mkdir -p res/mnt/sdcard/logs"); */
    /* test_get_sdcard_paths(MODE_CRASH, 0); */
    /* test_get_sdcard_paths(MODE_CRASH_NOSD, 0); */
    /* system("echo persist.sys.crashlogd.mode=lowmemorrry > res/properties.txt"); */
    /* reset_property_cache(); */
    /* test_get_sdcard_paths(MODE_CRASH, 0); */

    system("rm -f res/logs/currentcrashlog");
    gmaxfiles = 10;
    test_find_new_crashlog_dir(MODE_CRASH, 0);
    test_find_new_crashlog_dir(MODE_CRASH, 1); /* increment the counter */
    system("echo 5 > res/logs/currentcrashlog"); /* reset the counter to something else */
    test_find_new_crashlog_dir(MODE_CRASH, 5);
    test_find_new_crashlog_dir(MODE_CRASH, 6);
    system("rm -f res/logs/currentcrashlog");
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 0);
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 1);
    system("echo 5 > res/logs/currentcrashlog"); /* reset the counter to something else */
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 5);
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 6);
    gmaxfiles = 5;
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 7); /* Returns 7 because it did not take gmaxfiles into account before next call */
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 3);
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 4);
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 0);
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 1);
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 2);
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 3);
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 4);
    test_find_new_crashlog_dir(MODE_CRASH_NOSD, 0);
    system("rm -f res/logs/currentaplogslog");
    test_find_new_crashlog_dir(MODE_APLOGS, 0);
    system("echo 3 > res/logs/currentaplogslog");
    test_find_new_crashlog_dir(MODE_APLOGS, 3);
    system("rm -f res/logs/currentbzlog");
    test_find_new_crashlog_dir(MODE_BZ, 0);
    system("echo 4 > res/logs/currentbzlog");
    test_find_new_crashlog_dir(MODE_BZ, 4);
    system("rm -f res/logs/currentstatslog");
    test_find_new_crashlog_dir(MODE_BZ+1, -1);
    system("echo 4 > res/logs/currentstatslog");
    test_find_new_crashlog_dir(MODE_BZ+1, -1);

    test_file_read_string("res/file_to_append", buffer, 4); // data is 'text'
    test_file_read_string("res/cache_file_empty", buffer, 0);
    test_file_read_string("res/file_do_not_exists", buffer, -ENOENT);
    test_file_read_string(NULL, buffer, -EINVAL);
    test_file_read_string("res/file_to_append", NULL, -EINVAL);

    return 0;
}
