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

#ifndef CTHREADINFO_H_
#define CTHREADINFO_H_

#include <cstdio>
#include <vector>
#include <time.h>
#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
#include <signal.h>
#endif
class CFrameInfo;
class CProcInfo;
typedef std::vector<CFrameInfo *> VpFrameInfo;

class CThreadInfo {

#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
    enum attach_state {
        ATT_DETACH,
        ATT_WAIT_SIG,
        ATT_STOPPED
    };

    enum fail_reason {
        FR_NONE,    /**< successfully attached*/
        FR_ATT_REQ, /**< ptrace attach failed*/
        FR_ATT_CNF  /**< no SIG received*/
    };

    VpFrameInfo uframes;
    enum attach_state attach;
    enum fail_reason f_reason;
    struct timespec ar_ts; /**< when attach was requested */
    struct timespec ac_ts; /**< when attach was confirmed */
    struct timespec d_ts;  /**< when detach */
#endif
    CProcInfo *parent;
    unsigned int tid, ppid;
    char state;
    char *name;
    char *kstack;

    void readState();
    void readKstack();
#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
    bool canAttach();
    siginfo_t si;
#endif

  public:
    CThreadInfo(CProcInfo * parent, unsigned int tid);
    virtual ~ CThreadInfo();
    unsigned int getTid() const;
    void print(FILE * output);

#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
    void readUserStack();
    void ptraceAttach();
    void ptraceDetach();
    bool ptraceWaitSig();
#endif

};

#endif
