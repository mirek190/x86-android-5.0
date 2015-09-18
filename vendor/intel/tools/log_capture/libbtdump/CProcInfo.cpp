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


 /*Indent with "indent -npro -kr -i4 -ts0  -ss -ncs -cp1"*/

#include "CProcInfo.h"
#include "CThreadInfo.h"
#include <stdlib.h>             /* malloc, free, rand */
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
/*try values close to debuggerd*/
#define SIG_RETRY_CNT 2000      /*total of 10s */
#define SIG_RETRY_INT 5000      /*every 5 ms */
#endif

#define NS_IN_S (1000*1000*1000)

CProcInfo::CProcInfo(unsigned int pid)
{
    this->pid = pid;
    cmdline = NULL;
#ifdef USE_LIBUNWIND
    as = unw_create_addr_space(&_UPT_accessors, 0);
#endif
    clock_gettime(CLOCK_REALTIME, &start_ts);
    get_cmdline();
    get_threads();
#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
    attach();
    for (VpThreadInfo::iterator it = threads.begin();
         it != threads.end(); it++) {
        (*it)->readUserStack();
    }
    detach();
#endif
    clock_gettime(CLOCK_REALTIME, &end_ts);
}

void CProcInfo::get_cmdline()
{
    char temp[PATH_MAX];
    FILE *fp;
    size_t len = 0;
    snprintf(temp, PATH_MAX, "/proc/%d/cmdline", pid);
    fp = fopen(temp, "r");
    if (fp) {
        len = fread(temp, 1, PATH_MAX, fp);
        temp[len] = 0;
        cmdline = strdup(temp);
        fclose(fp);
    }
}

void CProcInfo::get_threads()
{

    char path[PATH_MAX];
    DIR *d;
    struct dirent *drnt;
    unsigned int t_tid;
    snprintf(path, PATH_MAX, "/proc/%d/task", pid);
    d = opendir(path);
    if (d) {
        while ((drnt = readdir(d))) {
            t_tid = strtol(drnt->d_name, NULL, 10);
            if (t_tid)
                threads.push_back(new CThreadInfo(this, t_tid));
        }
        closedir(d);
    }

}

unsigned int CProcInfo::getPid() const
{
    return pid;
}

CProcInfo::~CProcInfo()
{
    free(cmdline);
    for (VpThreadInfo::iterator it = threads.begin();
         it != threads.end(); it++) {
        delete *it;
    }
#ifdef USE_LIBUNWIND
    unw_destroy_addr_space(as);
#endif
}

#ifdef USE_LIBUNWIND
unw_addr_space_t CProcInfo::getAs() const
{
    return as;
}
#endif
/**
 * Print the process info
 * @param output
 */
void CProcInfo::print(FILE * output)
{
    char time_str[128];
    time_t nowtime;
    struct tm *nowtm;
    nowtime = start_ts.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(time_str, 128, "%Y-%m-%d %H:%M:%S", nowtm);

    fprintf(output, "\n----- pid %d at %s.%09ld -----\n", pid, time_str,
            start_ts.tv_nsec);
    fprintf(output, "Cmd line: %s\n", cmdline);
    for (VpThreadInfo::iterator it = threads.begin();
         it != threads.end(); it++) {
        (*it)->print(output);
    }
    fprintf(output, "----- end %d (%ld ns)-----\n", pid,
            timeDiff(start_ts, end_ts));
}

#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)

/**
 * Detach all the threads
 */
void CProcInfo::detach()
{
    for (VpThreadInfo::iterator it =
         threads.begin(); it != threads.end(); it++) {
        (*it)->ptraceDetach();
    }
}

/**
 * Attach to all the threads associated with this process
 */
void CProcInfo::attach()
{
    int remaining_cnt = threads.size(), remaining_retry = SIG_RETRY_CNT;

    for (VpThreadInfo::iterator it =
         threads.begin(); it != threads.end(); it++) {
        (*it)->ptraceAttach();
    }
    /*now wait for sig */
    while (remaining_cnt && remaining_retry) {
        remaining_cnt = 0;
        remaining_retry--;
        for (VpThreadInfo::iterator it =
             threads.begin(); it != threads.end(); it++) {
            if ((*it)->ptraceWaitSig())
                remaining_cnt++;

        }
        usleep(SIG_RETRY_INT);
    }
}

#endif

long CProcInfo::timeDiff(struct timespec start_ts, struct timespec end_ts)
{
    long time_dif;
    time_dif = ((end_ts.tv_sec - start_ts.tv_sec) * NS_IN_S);
    time_dif += end_ts.tv_nsec - start_ts.tv_nsec;
    return time_dif;
}
