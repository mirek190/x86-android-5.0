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

#include "inc/libbtdump.h"

#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <errno.h>
#include "CProcInfo.h"
#include <vector>

#define WAIT_RETRY_MAX    20
#define WAIT_RETRY_DELAY  1000

#define MAX_KSTACK_DUMP   512
#define TEMP_BUFF_MAX     128

static void dump_file_header(FILE * output)
{
    fprintf(output, "*************************************************\n");
    fprintf(output, "* Warning: Kernel space stacks are dumped before*\n");
    fprintf(output, "*          attaching to the thread              *\n");
    fprintf(output, "*************************************************\n");
    fprintf(output, "\n");
}

extern "C" int bt_process(int pid, FILE * output)
{
    if (!output)
        return -EINVAL;

    dump_file_header(output);
    CProcInfo pidnfo(pid);
    pidnfo.print(output);
    return 0;
}

typedef std::vector<CProcInfo *> VpProcInfo;

extern "C" int bt_all(FILE * output)
{
    DIR *dp;
    struct dirent *d_entry;
    int pid = -1;
    VpProcInfo processes;
    if (!output)
        return -EINVAL;

    dp = opendir("/proc/");

    if (!dp)
        return -EACCES;

    while ((d_entry = readdir(dp))) {
        if (d_entry->d_name[0] > '0' && d_entry->d_name[0] <= '9') {
            pid = atoi(d_entry->d_name);
            /*if we try to debug, pid != getppid(),
             * otherwise we can end up in a tracing loop */
            if (pid != getpid())
                processes.push_back(new CProcInfo(pid));
        }
    }

    dump_file_header(output);

    for (VpProcInfo::iterator it = processes.begin();
         it != processes.end(); it++) {
        (*it)->print(output);
        delete(*it);
    }

    closedir(dp);
    return 0;
}
