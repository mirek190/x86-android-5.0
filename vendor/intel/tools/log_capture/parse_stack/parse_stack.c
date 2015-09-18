#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include "backtrace.h"



////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	int opt;
	char option[256];
	char path[256];
	pid_t pid = -2, p = -1;
	DIR * dp;
	struct dirent *filename;
	bool find = false;
	void* pp = NULL;
	unsigned int flag = 0;

	printf("#########Enter parse_stack##########\n");
	if (argc < 2) {
		printf("Usage: parse_stack [-a] [-h] [-p process_id] [-f anr_file] [-t tombstone_file]\n");
		return -1;
	}
	while ((opt = getopt(argc, argv,"p:f:t:ha")) != -1) {
		switch(opt) {
			case 'p':
				strncpy(option, optarg, sizeof(option));
				pid = strtoul(option, NULL, 10);
				break;

			case 'h':
				printf("Usage: parse_stack [-a] [-h] [-p process_id] [-f anr_file] [-t tombstone_file]\n");
				return 0;

			case 'f':
				strncpy(option, optarg, sizeof(option));
				flag = 1;
				break;
			case 't':
				strncpy(option, optarg, sizeof(option));
				flag = 2;
				break;
			case 'a':
				printf("parse all stack");
				pid = -1;
				break;
			default:
				printf("Invalid option\n");
				exit(1);
				break;
		}
	}

	option[255] = '\0';
	if (flag == 1) { //parse anr file
		backtrace_parse_anr_file(option);
		return 0;
	}
	else if (flag == 2) { //parse tombstone file
	printf("#########Enter backtrace_parse_tombstone_file: %s##########\n", option);
		backtrace_parse_tombstone_file(option);
		return 0;
	}

	dp = opendir("/proc/");
	if (!dp)
	{
		printf("open /proc directory error\n");
		return -1;
	}
	filename = readdir(dp);
	while (filename)
	{
		if(filename->d_name[0] > '0' &&  filename->d_name[0] <= '9')
		{
			p = atoi(filename->d_name);
			if (p == pid)
				find = true;
		}
		filename = readdir(dp);
	}
	closedir(dp);

	if (pid != -1 && !find) {
		printf("The process %d doesn't exist.\n", pid);
		return -1;
	}

	if (find){


		backtrace_single_process(pid);

	}

	if (pid == -1 && !find)
		backtrace_android_whole();
	return 0;
}

