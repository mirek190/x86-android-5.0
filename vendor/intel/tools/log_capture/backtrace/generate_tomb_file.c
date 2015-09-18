/*
 * * backtrace dump tool
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
 * */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>


#define PATH_LENGTH 256


int dump_binder_info(FILE* f, unsigned int pid)
{
   char *token = NULL;
   char *str = NULL;
   char buf[PATH_LENGTH] = {0, };
   char path[PATH_LENGTH] = {0, };
   FILE * fp = NULL;
   sprintf(path, "/d/binder/proc/%d", pid);
   fp = fopen(path,"r");
   if (fp && f) {
		fprintf(f, "\n----binder info: pid %d----\n",  pid);
		while(fgets(buf, PATH_LENGTH, fp)) {
		    fputs(buf ,f);
		 }

		fprintf(f,"----binder info end: pid %d----\n\n",  pid);
   }
   if (fp)
	   fclose(fp);

   return 0;
}
int dump_binder_transaction(FILE* f)
{
   char *token = NULL;
   char *str = NULL;
   char buf[PATH_LENGTH] = {0, };
   char path[PATH_LENGTH] = {0, };
   FILE * fp = NULL;
   sprintf(path, "/d/binder/transactions");
   fp = fopen(path,"r");
   if (fp && f) {
		while(fgets(buf, PATH_LENGTH, fp)) {
		    fputs(buf ,f);
		 }
   }
   if (fp)
      fclose(fp);

   return 0;
}

int read_process_maps(FILE *f, unsigned int pid)
{
	char buf[PATH_LENGTH] = {0, };
	char path[PATH_LENGTH] = {0, };
	FILE * fp = NULL;

	sprintf(path, "/proc/%d/maps", pid);

	fp = fopen(path, "r");
	if (fp && f) {
		fprintf(f,"----maps start: pid %d ----\n",  pid);
		while(fgets(buf, PATH_LENGTH, fp)) {
		    fputs(buf,f);
		 }
		fprintf(f,"----maps end: pid %d----\n\n",  pid);
	}
	if (fp)
		fclose(fp);

	return 0;
}
int read_process_stack(FILE *f, unsigned int pid)
{
	char buf[PATH_LENGTH] = {0, };
	char path[PATH_LENGTH] = {0, };
	FILE * fp = NULL;
	DIR * dp = NULL;
	int tid = -1;
	struct dirent *filename = NULL;

	sprintf(path, "/proc/%d/stack", pid);
	fp = fopen(path, "r");
	if (fp && f) {
		fprintf(f,"----process stack start: pid %d----\n",  pid);
		while(fgets(buf, PATH_LENGTH, fp)) {
		    fputs(buf ,f);
		 }
		fprintf(f,"----process stack end: pid %d----\n\n", pid);
	}
	if (fp)
		fclose(fp);

	sprintf(path, "/proc/%d/task/", pid);
	dp = opendir(path);
	if ( !dp )
		printf("open /proc/%d/task/ failed\n", pid);
	else
		while((filename = readdir(dp))) {
			if (filename->d_name[0] > '0' && filename->d_name[0] <= '9') {
				tid = atoi(filename->d_name);
				if (tid != pid)
					read_thread_stack(f, pid, tid);
			}

		}
        if (dp)
           closedir(dp);
	return 0;
}

int read_thread_stack(FILE *f, unsigned int pid, int tid)
{
	char buf[PATH_LENGTH] = {0, };
	char path[PATH_LENGTH] = {0, };
	char str[PATH_LENGTH] = {0, };
	FILE * fp = NULL;

	char comm[PATH_LENGTH] = {0,};
	char name[PATH_LENGTH] = {0,};
	sprintf(name,"/proc/%d/comm",tid);
	FILE *fpfile = fopen(name,"r");
	if (fpfile)
	   fgets(comm,PATH_LENGTH,fpfile);
	if( fpfile )
	   fclose(fpfile);

	sprintf(path, "/proc/%d/task/%d/stack", pid, tid);

	fp = fopen(path, "r");
	if (fp && f) {
		fprintf(f,"		----thread stack start: pid %d tid %d, thread name %s\n",  pid, tid,comm);
		while(fgets(buf, PATH_LENGTH, fp)) {
		    strcpy(str, "		");
		    strncat(str, buf, sizeof(str)-2);
		    fputs(str ,f);
		 }
		fprintf(f,"		----thread stack end: pid %d tid %d, thread name %s\n\n",  pid, tid, comm);
	}
	if (fp)
		fclose(fp);

	return 0;
}

int read_process_info(FILE *f, int pid)
{
        char comm[PATH_LENGTH] = {0,};
        char name[PATH_LENGTH] = {0,};
        sprintf(name,"/proc/%d/comm",pid);
        FILE *fp = fopen(name,"r");
        if (fp)
           fgets(comm,PATH_LENGTH,fp);
	fprintf(f, "--------PID START: %d ,process name %s--------\n", pid,comm);
        if( fp )
           fclose(fp);
	dump_binder_info(f, pid);
	read_process_maps(f, pid);
	read_process_stack(f, pid);
	fprintf(f, "--------PID END: %d --------\n\n\n", pid);
	return 0;

}

int read_all_process_info(FILE *f, int pid ,int tid)
{
	DIR * dp = NULL;
	struct dirent *filename = NULL;
	char path[PATH_LENGTH] = {0, };
	int p = -1;
	int flag = 0;
/*
	if (f == NULL) {
		flag = 1;
		sprintf(path, "/data/tombstones/tombstones_%d_%d_raw", pid, tid);
		f = fopen(path, "w");
		if (f == NULL) {
			printf("Create file: %s failed\n", path);
			return -1;
		}
	}
*/
	dump_binder_transaction(f);
	dp = opendir("/proc/");
	if ( !dp )
		printf("open /proc/ failed\n");
	else
		while((filename = readdir(dp))) {
			if (filename->d_name[0] > '0' && filename->d_name[0] <= '9') {
				p = atoi(filename->d_name);
				read_process_info(f, p);
			}

		}
        if (dp)
           closedir(dp);
/*
	if (flag)
		fclose(f);
*/
	return 0;
}

