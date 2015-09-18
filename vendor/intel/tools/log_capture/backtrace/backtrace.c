
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
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <elf.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/syscall.h>

#include "symbols.h"



typedef struct stack_info {
	int pid;
	int ppid;//parent pid
	char p_name[NAME_LENGTH]; // thread/process name
	int stack_depth;
	int is_thread;
	int child_count;
	int p_index;
	unsigned long ebp[MAX_STACK_LENGTH];//user stack for this process
	char kernel_stack[MAX_KERNELSTACK_SIZE];
} stack_info;

stack_info StackAll[TASK_NUM];
int g_index = 0;
long read_all_stack_time = 0;
bool is64OS = false;
static inline long calc_timediff(struct timeval *t0, struct timeval *t1)
{
	return (t1->tv_sec - t0->tv_sec) * 1000 +
		(t1->tv_usec - t0->tv_usec) / 1000;
}

int judge_64_OS()
{
	if( sizeof(long) == 4)
		return 0;
	else if( sizeof(long) == 8)
		return 1;
	else
		return -1;
}

mapinfo *parse_map_line(char *line)
{
	mapinfo *mi;
	int len = strlen(line);

	if(len < 1)
		return 0;
	line[--len] = 0;

	if(len < 50)
		return 0;

	if(line[20] != 'x')
		return 0;

	mi = malloc(sizeof(mapinfo) + (len - 47));
	if(mi == NULL)
		return 0;

	mi->start = strtoul(line, 0, 16);
	mi->end = strtoul(line + 9, 0, 16);
	mi->exidx_start = mi->exidx_end = 0;
	mi->symbols = 0;
	mi->next = 0;
	strncpy(mi->name, line + 49, sizeof(mi->name));

	return mi;
}

mapinfo *parse_map_line_64(char *line)
{
	mapinfo *mi;
	char * token = NULL;
	char * p = NULL;
	int len = strlen(line);
	unsigned long start_addr = 0;
	unsigned long end_addr = 0;
	if (len < 2)
		return 0;
	line[--len] = 0;

	token = strtok(line, " ");
	if (token == NULL)
		return NULL;
	char buf1[200] = {0,};

	p = strchr(token, '-');
	if (p == NULL)
		return NULL;
	strncpy(buf1, token, sizeof(buf1));
	start_addr = strtoull(buf1, NULL, 16);
	end_addr  = strtoull(p+1, NULL, 16);
	//rwx
	token = strtok(NULL, " ");

	p = strchr(token, 'x');
	if (p == NULL)
		return NULL;

	//map file
	int counter = 0;
	while((token = strtok(NULL, " "))){
		p = token;
		counter++;
	}
	if (counter != 4)
		return NULL;

	mi = malloc(sizeof(mapinfo));
	if (mi == NULL)
		return 0;

	//fill struct
	mi->start = start_addr;
	mi->end = end_addr;
	mi->exidx_start = mi->exidx_end = 0;
	mi->symbols = 0;
	mi->next = 0;
	strncpy(mi->name, p, sizeof(mi->name));
	return mi;
}

static void parse_elf32(mapinfo *milist)
{
	mapinfo *mi;
	struct symbol_table *table = NULL;
	//int leon_parse_elf32= 0, hit = 0;
	for (mi = milist; mi != NULL; mi = mi->next) {
		Elf32_Ehdr  ehdr;
		Elf32_Ehdr  *ehdr1;
		memset(&ehdr, 0, sizeof(Elf32_Ehdr));
		/*
		 * Read in sizeof(Elf32_Ehdr) worth of data from
		 * the beginning of mapped section.
		 */
		struct stat sb;
		int length;
		char *base;
		int fd = open(mi->name, O_RDONLY);
		if(fd < 0) {
			continue ;
		}

		fstat(fd, &sb);
		length = sb.st_size;
		base = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
		if(!base) {

			goto out_close;
		}

		// Parse the file header
		ehdr1 = ((Elf32_Ehdr*)base);
		memcpy(&ehdr,ehdr1,sizeof(Elf32_Ehdr));
out_unmap:
		munmap(base, length);
out_close:
		close(fd);
		//hit++;
		if (IS_ELF(ehdr)) {
			/* Try to load symbols from this file */
			//fprintf(stderr, "+++==leon, loading from:%s\n", mi->name);
			mi->symbols = symbol_tables_create(mi->name);
		}
	}
	//fprintf(stderr, "+++++++++++==leon, loop2:%d,hit:%d\n", leon_parse_elf32, hit);
}

static void parse_elf64(mapinfo *milist)
{
	mapinfo *mi;
	struct symbol_table *table = NULL;
	for (mi = milist; mi != NULL; mi = mi->next) {
		Elf64_Ehdr  ehdr;
		Elf64_Ehdr  *ehdr1;
		memset(&ehdr, 0, sizeof(Elf64_Ehdr));
		/*
		 * Read in sizeof(Elf64_Ehdr) worth of data from
		 * the beginning of mapped section.
		 */
		struct stat sb;
		int length;
		char *base;
		int fd = -1;
	        fd = open(mi->name, O_RDONLY);
		if(fd < 0) {
			continue ;
		}
		fstat(fd, &sb);
		length = sb.st_size;
		base = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
		if(!base) {

			goto out_close;
		}
		char * p = (char *)base+18;
		bool  is_Elf32 = false;
		if (*p != 0x3e)
			is_Elf32 = true;
		// Parse the file header
		ehdr1 = ((Elf64_Ehdr*)base);
		memcpy(&ehdr,ehdr1,sizeof(Elf64_Ehdr));
		if (IS_ELF(ehdr)) {
			/* Try to load symbols from this file */
			//			LOGV("load symbols from file...\n");
			//			LOGV("symbol_tables_create64\n");
			if(is_Elf32) {
				mi->symbols = symbol_tables_create(mi->name);
			}
			else
				mi->symbols = symbol_tables_create64(mi->name);
		}

out_unmap:
		munmap(base, length);
out_close:
		close(fd);

	}
}

void parse_elf(mapinfo *milist)
{
   if(judge_64_OS()) {
	   parse_elf64(milist);
   }
   else  {
	   parse_elf32(milist);
   }
}


static bool read_single_stack(char * data, int  index)
{
	unsigned long tmp;
	FILE* fp = NULL;
	int iCount = 0,i = 0;
	bool access_ok = false;
	char *pKernel = NULL;

	if(index >= TASK_NUM)
		return access_ok;

	sprintf(data, "%s/stack", data);
	fp = fopen(data, "r");
	if (fp) {
		pKernel = StackAll[index].kernel_stack;
		while(fgets(data, PATH_LENGTH, fp)) {
			if( strncmp("userspace", data, 9)==0 ) {
				iCount++;
			} else {
				if (iCount == 0) {
					memcpy(pKernel,data,strlen(data));
					pKernel = pKernel + strlen(data);
				}
			}

			if(iCount > 0 && i <32) {
				if (iCount !=1)	{
					tmp = strtoul(data,0,16);
					StackAll[index].ebp[i] = tmp;
					i++;
				}
				iCount++;
			}
			access_ok = true;
		}

		fclose(fp);
	}

	if(access_ok) {
		/* update the stack depth for this process*/
		StackAll[index].stack_depth = i;
	}

	return access_ok;
}

/* Return true if some thread is not detached cleanly */
static bool dump_sibling_thread_stack(unsigned pid, int parent_index)
{
	char task_path[PATH_LENGTH];
	char data[PATH_LENGTH];
	unsigned int data1[MAX_STACK_LENGTH];
	sprintf(task_path, "/proc/%d/task", pid);
    DIR *d;
    struct dirent *de;
    int need_cleanup = 0;
    d = opendir(task_path);
    /* Bail early if cannot open the task directory */
    if (d == NULL) {
        printf("Cannot open /proc/%d/task\n", pid);
        return false;
    }
    while ((de = readdir(d)) != NULL) {
        unsigned new_tid;
        /* Ignore "." and ".." */
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        new_tid = atoi(de->d_name);
        /* The main thread at fault has been handled individually */
        if (new_tid != pid)
         {
		snprintf(data, PATH_LENGTH, "%s/%d", task_path,new_tid);
		if ( read_single_stack(data, g_index) ) {
			StackAll[g_index].pid  = new_tid;
			StackAll[g_index].ppid = pid;
			StackAll[g_index].is_thread = 1;
			StackAll[g_index].p_index = parent_index;
			StackAll[parent_index].child_count++;
			g_index++;
		}

	}
        /* Skip this thread if cannot ptrace it */
     }
    closedir(d);
    return need_cleanup != 0;
}


int read_all_stack()
{
	pid_t traced_process;
	long ins;
	FILE *fp;
	DIR * dp;
	struct dirent *filename;
	struct timeval currentTime, stillafstarttime;
	char data[PATH_LENGTH];
	int stack_depth = 0;
	int index_tmp = 0;

	gettimeofday(&stillafstarttime,0);
	printf("parse_stack \n");

	dp = opendir("/proc/");

	if (!dp)
	{
		printf("open /proc directory error\n");
		return 0;
	}

	while ((filename=readdir(dp)))
	{
		if( filename->d_name[0] > '0' &&  filename->d_name[0] < '9' )
		{
			traced_process = atoi(filename->d_name);
			/* Clear stack pointer records */
			StackAll[g_index].p_index = -1;
			// read parent stack
			snprintf(data, PATH_LENGTH, "/proc/%s", filename->d_name);
			if (read_single_stack(data,g_index))
			{
				/*
				 * if we can access the stack of this
				 * process, continue to parse the child
				 * thread
				 */
				StackAll[g_index].pid = traced_process;
				g_index++;
				// read child stack
				//unwind to get the child stack,
				dump_sibling_thread_stack(atoi(filename->d_name), g_index - 1);
			}

		}
	}
	closedir(dp);
	gettimeofday(&currentTime,0);
	read_all_stack_time = calc_timediff(&stillafstarttime, &currentTime);
	return 0;
}

void parse_kernel(int index)
{
	char *pKernel = StackAll[index].kernel_stack;
	printf("\nKernel Stack:\n");
	printf("%s", pKernel);
}

void *parse_all(void *arg)
{
	pid_t traced_process;
	FILE *fp = NULL;
	mapinfo *milist = 0;
	int stack_depth = 0;
	int index = 0;
	struct timeval currentTime, stillafstarttime;
	//int leon_create = 0, leon_free = 0, hit_1 = 0, hit_2 = 0, hit_3 = 0;

	gettimeofday(&stillafstarttime, 0);
	printf("\n\nparse_all, the stack whole size is %d\n", g_index);

	while (index <= g_index) {
		char data[PATH_LENGTH];
		unsigned long data1[STACK_DEPTH];
		int iElfCount = 0;

		/* Clear stack pointer records */
		traced_process = StackAll[index].pid;
		if (traced_process < 1)
			break;

		printf("=========\n");
		if (StackAll[index].is_thread) {
			sprintf(data, "/proc/%d/task/%d/comm",
				StackAll[index].ppid,
				StackAll[index].pid);
		}
		else {
			sprintf(data, "/proc/%d/comm",
				StackAll[index].pid);
		}

		fp = fopen(data,"r");
		if (!fp) {
			index++;
			printf("can't read %s\n", data);
			continue;
		}

		if(fgets(data, PATH_LENGTH, fp)) {
			strncpy(StackAll[index].p_name, data, sizeof(StackAll[index].p_name));
			if (StackAll[index].is_thread) {
				int pp_index = StackAll[index].p_index;

				printf("tid : %d, name: %s", StackAll[index].pid, data);
				printf("ppid: %d, name: %s\n", StackAll[index].ppid,
					StackAll[index].ppid > 0 ? StackAll[pp_index].p_name : " ");
			}
			else {
				printf("pid : %d, name: %s", StackAll[index].pid, data);
				int pp_index = StackAll[index].p_index;
				printf("ppid: %d, name: %s\n",
					StackAll[index].ppid,
					StackAll[index].ppid > 0 ? StackAll[pp_index].p_name : " ");
				printf("child count is %d\n",StackAll[index].child_count);
			}

		}
		fclose(fp);
		fp = NULL;

		if (StackAll[index].is_thread)	{
			sprintf(data, "/proc/%d/task/%d/maps",
				StackAll[index].ppid,StackAll[index].pid);
		} else {
			sprintf(data, "/proc/%d/maps", StackAll[index].pid);
		}
		parse_kernel(index);

		//parse the map info of this process, create milist in here, need to free
		if(!StackAll[index].is_thread) {
			/* free the list of previous process */
			//leon_free = 0;
			while(milist) {
				mapinfo *next = milist->next;
				symbol_tables_free(milist->symbols);
				free(milist);
				milist = next;
				//leon_free++;
			}
			//fprintf(stderr, "+++++++++++==leon, loop3:%d\n", leon_free);
			fp = fopen(data, "r");
			if(!fp) {
				index++;
				continue;
			}
			//leon_create=0;
			while(fgets(data, PATH_LENGTH, fp)) {
				mapinfo *mi = parse_map_line_64(data);
				if(mi) {
					//fprintf(stderr, "+++==leon, parsing:%s\n", mi->name);
					//leon_create++;
					mi->next = milist;
					milist = mi;
					iElfCount ++;
				}
			}
			fclose(fp);
			fp = NULL;
			if ((iElfCount < 1)) {
				printf("can't access file /proc/%d/maps\n\n\n",traced_process);
				index++;
				continue;
			}
			//fprintf(stderr, "+++++++++++==leon, loop1:%d\n", leon_create);
			parse_elf(milist);
		}


		// parse parent stack
		printf("\nUser Stack:\n");
		if(judge_64_OS())
			unwind_backtrace_with_stack64(StackAll[index].ebp,
					StackAll[index].stack_depth,
					milist);
		else
			unwind_backtrace_with_stack(StackAll[index].ebp,
					StackAll[index].stack_depth,
					milist);

		printf("\n\n\n");

		index++;
	}
	while(milist) {
		mapinfo *next = milist->next;
		symbol_tables_free(milist->symbols);
		free(milist);
		milist = next;
	}
	g_index = 0;

	gettimeofday(&currentTime,0);
	printf("read all stack time: %ldms\n", read_all_stack_time);
	printf("parse_all time: %ldms\n",
		calc_timediff(&stillafstarttime, &currentTime));
//	printf(" +++++++++++==leon, create count:%d, free count:%d, table count:%d, hit_1:%d, hit_2:%d, hit_3:%d\n",
//		   leon_create, leon_free, leon_parse_elf32, hit_1, hit_2, hit_3);
	return NULL;
}
 int backtrace_android_whole()
{
	struct timeval currentTime, stillafstarttime;
	pid_t traced_process;
	long ins;
	FILE *fp;
	mapinfo *milist = 0;
	int stack_depth = 0;
	int index = 0;
	g_index = 0;

	gettimeofday(&stillafstarttime,0);
	memset(StackAll,0,sizeof(StackAll));

	read_all_stack();
	parse_all(NULL);

	gettimeofday(&currentTime,0);
	printf("Whole time: %ldms\n",
			calc_timediff(&stillafstarttime, &currentTime));
	return 0;
}


int backtrace_android_current()
{
	struct timeval currentTime, stillafstarttime;
	char path[PATH_LENGTH] = {0, };
	pid_t traced_process;
	long ins;
	FILE *fp;
	mapinfo *milist = 0;
	int stack_depth = 0;
	int index = 0;
	g_index = 0;
	int tid;

	gettimeofday(&stillafstarttime,0);
	memset(StackAll,0,sizeof(StackAll));
#if 0
	tid = syscall(224/*SYS_gettid*/);
#else
	pthread_t thread = pthread_self();
	tid = __pthread_gettid(thread);
#endif
	sprintf(path, "/proc/%d", tid);
	if (read_single_stack(path, g_index)) {
		StackAll[g_index].pid = tid;
		StackAll[g_index].p_index = -1;
		g_index++;
		parse_all(NULL);
	}
	
	gettimeofday(&currentTime,0);
	printf("Whole time: %ldms\n",
			calc_timediff(&stillafstarttime, &currentTime));

	return 0;
}
////////////////////////////////////////////////////////////
void backtrace_parse_anr_file( char *filename)
{
	FILE *fp = NULL;
	FILE *fp_copy = NULL;
	mapinfo *milist = 0;
	unsigned int stack_depth = 0;
	char data[PATH_LENGTH] = {0,};
	char* result[STACK_DEPTH] = {0,};
	unsigned long  userstack[STACK_DEPTH] = {0,};
	unsigned int i = 0;

	snprintf(data, PATH_LENGTH, "%s_symbol",filename);
	fp_copy  = fopen(data,"w");
	if (fp_copy == NULL) {
		printf("failed to open file %s: %s.\n", data, strerror(errno));
		return;
	}

	fp  = fopen(filename,"r");
	if (fp == NULL) {
		printf("failed to open file %s: %s.\n", data, strerror(errno));
		fclose(fp_copy);
		return;
	}
	// parse maps
	while (fgets(data,PATH_LENGTH, fp) ) {
		if (strlen(data) < 2)
			continue;

		if (!strncmp(data, "[<", 2)) {
			/* this is kernel stack */
			fputs(data,fp_copy);
			continue;
		}
		
		if (!strncmp(data, "userspace", 9)) {
			break;
		}
		
		mapinfo *mi = parse_map_line_64(data);
		if(mi) {
			mi->next = milist;
			milist = mi;
		}
	}
	parse_elf(milist);
	
	/* userspace stack */
	while(fgets(data, PATH_LENGTH, fp)) {
		if (strlen(data) < 2) continue;
		userstack[stack_depth++] = strtoul(data, NULL, 16);
	}
	unwind_backtrace_with_stack_file(userstack,stack_depth,  milist, result);
	for ( i = 0; i < stack_depth; i++) {
		if(result[i] != NULL) {
			fwrite(result[i], strlen(result[i]), 1, fp_copy);
			free(result[i]);
			result[i] = NULL;
		}
	}
	/* free memory */
	while(milist) {
		mapinfo *next = milist->next;
		symbol_tables_free(milist->symbols);
		free(milist);
		milist = next;
	}
	fclose(fp);
	fclose(fp_copy);
	return ;
}

////////////////////////////////////////////////////////////
void backtrace_parse_tombstone_file( char *filename)
{
	FILE *fp = NULL;
	mapinfo *milist = 0;
	unsigned int stack_depth = 0;
	char *str = NULL;
	int farther = false;
	FILE * fp_copy = NULL;
	int father_pid = -1;

	char data[PATH_LENGTH] = {0,};
	char title[PATH_LENGTH] = {0,};
	char* result[STACK_DEPTH] = {NULL,};
	unsigned long  userstack[STACK_DEPTH] = {0,};
	unsigned int i = 0;
	int is_thread = 0;

	snprintf(data, PATH_LENGTH, "%s_symbol",filename);
	fp_copy  = fopen(data,"w");

	fp  = fopen(filename,"r");
	while ( fp_copy && fp && fgets(data,PATH_LENGTH, fp) ) {
		int iElfCount = 0;
		if (strlen(data) < 2) {
			fputs(data,fp_copy);
			continue;
		}
		if (( str = strstr(data, "PID START")) )  // got the farther task id,
			{
				str += 10;
				father_pid  = atoi(str);
				farther = true;
				fputs(data,fp_copy);
			}
		else {
			farther = false;
			fputs(data,fp_copy);
			continue;
		}
		if (farther == true) //parse map info, skip write map info to new file. only read map info for farther task
		{
			while ( fgets(data,PATH_LENGTH,fp))  {
				if (strlen(data) < 2) {
					fputs(data,fp_copy);
					continue;
				}
				if ( (str = strstr(data, "maps start")) )  // got the farther task id,
					{
						while(fgets(data, PATH_LENGTH, fp)) {
							if ( strstr(data,"maps end"))
								goto f;

							mapinfo *mi = parse_map_line_64(data);
							if(mi) {
								mi->next = milist;
								milist = mi;
								iElfCount ++;
							}
						}
					}

				else    {
					fputs(data,fp_copy);
				}
			}
		}
f:      	if (farther == true)  {
			parse_elf32(milist);
			farther = false;
		}
		while(fgets(data, PATH_LENGTH, fp)) {
			if (strlen(data) < 2) {
				fputs(data,fp_copy);
				continue;
			}
			if ((strstr(data, "process stack start") || strstr(data, "thread stack start"))) {
				fputs(data,fp_copy);//copy title
				while(fgets(data, PATH_LENGTH, fp)) {
					if (strlen(data) < 2) continue;
					if ((str = strstr(data,"userspace"))) {
						while(fgets(data, PATH_LENGTH, fp)) {
							if (strlen(data) < 2) continue;
							if (stack_depth > (STACK_DEPTH-2) )
								break;

							if ((strstr(data, "process stack end") || strstr(data, "thread stack end"))) {
								strcpy(title, data);
								if (strstr(data, "thread stack end"))
									is_thread = 1;
								break;
							}
							userstack[i++] = strtoul(data, NULL, 16);
							stack_depth++;
						}
						unwind_backtrace_with_stack_file(userstack,stack_depth,  milist, result);
						for ( i = 0; i < stack_depth; i++) {
							if (result[i] == NULL || i > (STACK_DEPTH-2)) {
								continue;
							}
							if (is_thread) {
								if (result[i] != NULL){
									strcpy(data, "         	");
									strncat(data, result[i], strlen(result[i]));
									fputs(data, fp_copy);}
							}
							else
                                                           {
								   strcpy(data, "");
								   if( result[i] != NULL){
									   strncat(data, result[i], strlen(result[i]));
									   fputs(data, fp_copy);}
							   }
							free(result[i]);
							result[i] = NULL;
						}
						//copy titile end
						fputs(title, fp_copy);

						is_thread = 0;
						stack_depth = 0;
						i = 0;
						break;

					}
					//write kernel stack infomation
					fwrite(data, strlen(data), 1, fp_copy);
				}
			} //end "process stack start" || "thread stack start"
			else  {
				fputs(data,fp_copy);
				if ((str = strstr(data, "PID END"))) {
					while(milist) {
						mapinfo *next = milist->next;
						symbol_tables_free(milist->symbols);
						free(milist);
						milist = next;
					}
					milist = 0;
					break;
				}
			}
		}
	}
	while(milist) {
		mapinfo *next = milist->next;
		symbol_tables_free(milist->symbols);
		free(milist);
		milist = next;
	}
	if (fp)
		fclose(fp);
	if (fp_copy)
		fclose(fp_copy);
	return ;
}

void backtrace_single_process(int pid) {
	char path[PATH_LENGTH] = {0, };
	g_index = 0;
	memset(StackAll,0,sizeof(StackAll));
	sprintf(path, "/proc/%d", pid);
	if (read_single_stack(path, g_index)) {
		StackAll[g_index].pid = pid;
		StackAll[g_index].p_index = -1;
		g_index++;
		dump_sibling_thread_stack(pid,  g_index-1);
		parse_all(NULL);
	}
}


