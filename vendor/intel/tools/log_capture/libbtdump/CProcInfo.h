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

#ifndef CPROCINFO_H_
#define CPROCINFO_H_
#include <vector>
#ifdef USE_LIBUNWIND
#include <libunwind-ptrace.h>
#endif
#include <stdio.h>
#include <time.h>

class CThreadInfo;
typedef std::vector<CThreadInfo *> VpThreadInfo;
class CProcInfo {
    unsigned int pid;
    char *cmdline;
    struct timespec start_ts, end_ts;
    VpThreadInfo threads;
#ifdef USE_LIBUNWIND
    unw_addr_space_t as;
#endif
    void get_cmdline();
    void get_threads();
#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
    void detach();
    void attach();
#endif
  public:
    CProcInfo(unsigned int pid);
    virtual ~ CProcInfo();
    unsigned int getPid() const;
    void print(FILE * output);
#ifdef USE_LIBUNWIND
    unw_addr_space_t getAs() const;
#endif
    static long timeDiff(struct timespec start_ts, struct timespec end_ts);
};

#endif /* CPROCINFO_H_ */
