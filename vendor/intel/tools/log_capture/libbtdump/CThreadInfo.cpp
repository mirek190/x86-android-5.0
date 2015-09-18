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

#include "CThreadInfo.h"

#include <stddef.h>
#include <stdlib.h>
#include <cstring>
#include <iterator>
#include <limits.h>
#include <errno.h>

#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
#include <sys/ptrace.h>
#endif

#ifdef USE_LIBUNWIND
#include <libunwind-ptrace.h>
#endif

#ifdef USE_LIBBACKTRACE
#include <backtrace/Backtrace.h>
#include <UniquePtr.h>
#endif

#include "CFrameInfo.h"
#include "CProcInfo.h"

#define KTHREADD_PID 2

CThreadInfo::CThreadInfo(CProcInfo * parent, unsigned int tid)
{
    this->parent = parent;
    this->tid = tid;
    name = NULL;
    kstack = NULL;
    state = 'U';
    ppid = 0;
#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
    attach = ATT_DETACH;
    f_reason = FR_NONE;
    ar_ts.tv_nsec = 0;
    ar_ts.tv_sec = 0;
    ac_ts.tv_nsec = 0;
    ac_ts.tv_sec = 0;
    d_ts.tv_nsec = 0;
    d_ts.tv_sec = 0;
#endif
    readState();
    readKstack();
}

unsigned int CThreadInfo::getTid() const
{
    return tid;
}

CThreadInfo::~CThreadInfo()
{
    free(name);
    free(kstack);
#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
    for (VpFrameInfo::iterator it = uframes.begin();
         it != uframes.end(); it++) {
        delete *it;
    }
#endif
}

void CThreadInfo::readState()
{
    char temp[PATH_MAX], *name_start, *name_end;
    FILE *fp;
    size_t len = 0;
    snprintf(temp, PATH_MAX, "/proc/%d/task/%d/stat", parent->getPid(),
             tid);
    fp = fopen(temp, "r");
    if (fp) {
        len = fread(temp, 1, PATH_MAX, fp);
        temp[len] = 0;
        name_start = strchr(temp, '(') + 1;
        name_end = strchr(temp, ')');
        /*null terminate the name */
        name_end[0] = 0;
        name = strdup(name_start);
        state = name_end[2];
        sscanf(&name_end[3], "%d", &ppid);
        fclose(fp);
    }
}

void CThreadInfo::readKstack()
{
    char temp[PATH_MAX];
    FILE *fp;
    size_t len = 0;
    snprintf(temp, PATH_MAX, "/proc/%d/task/%d/stack", parent->getPid(),
             tid);
    fp = fopen(temp, "r");
    if (fp) {
        len = fread(temp, 1, PATH_MAX, fp);
        temp[len] = 0;
        kstack = strdup(temp);
        fclose(fp);
    }
}

#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
/**
 * Trigger the user space back trace dump
 */
void CThreadInfo::readUserStack()
{
    if (attach != ATT_STOPPED)
        return;
#ifdef USE_LIBUNWIND
    struct UPT_info *ui;
    char temp[PATH_MAX];
    unw_cursor_t c;
    unw_proc_info_t pinfo;
    unw_word_t ip, sp, offset;
    struct CFrameInfo *frame;
    ui = (struct UPT_info *)_UPT_create(tid);
    if (unw_init_remote(&c, parent->getAs(), ui) < 0)
        return;
    do {

        if (unw_get_reg(&c, UNW_REG_IP, &ip) < 0
            || unw_get_reg(&c, UNW_REG_SP, &sp) < 0)
            break;

        temp[0] = '\0';
        unw_get_proc_name(&c, temp, PATH_MAX, &offset);
        if (temp[0])
            uframes.push_back(new CFrameInfo(ip, sp, temp, offset));

        if (unw_step(&c) < 0) {
            break;
        }

    } while (1);

    _UPT_destroy(ui);
#endif
#ifdef USE_LIBBACKTRACE
    UniquePtr <Backtrace>
        backtrace(Backtrace::Create(tid, BACKTRACE_CURRENT_THREAD));
    if (!backtrace.get()) {
        uframes.push_back(new CFrameInfo("ERROR: Cannot get backtrace context"));
    } else if (!backtrace->GetMap()) {
        uframes.push_back(new CFrameInfo("ERROR: Cannot get backtrace map context"));
    } else if (backtrace->Unwind(0)) {
        for (size_t i = 0; i < backtrace.get()->NumFrames(); i++) {
            uframes.push_back(new CFrameInfo(backtrace->FormatFrameData(i).c_str()));
        }
    }
#endif
}
#endif

void CThreadInfo::print(FILE * output)
{
    fprintf(output, "\n-- \"%s\" sysTid=%d(%d) [%c]--\n", name, tid, ppid,
            state);
    fprintf(output, "Kernel Stack:\n%s", kstack);
#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
    switch(f_reason) {
    case FR_ATT_REQ:
        fprintf(output,"Ptrace attach failed. No Userspace Stack\n");
        break;
    case FR_ATT_CNF:
        fprintf(output,"Cannot attach. No Userspace Stack\n");
        break;
    case FR_NONE:
        if (uframes.size()) {
            fprintf(output, "Wait attach: %ld ns, Trace int.: %ld ns\n",
                    CProcInfo::timeDiff(ar_ts, ac_ts),
                    CProcInfo::timeDiff(ac_ts, d_ts));
            fprintf(output, "Userspace Stack:\n");
            for (VpFrameInfo::iterator it = uframes.begin();
                 it != uframes.end(); it++) {
                (*it)->print(output);
            }
        }
    }
#endif

    fprintf(output, "-- end sysTid=%d --\n", tid);
}

#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
bool CThreadInfo::canAttach()
{
    /*only S,D and R states */
    if (attach != ATT_DETACH)
        return false;
    if (state != 'S' && state != 'D' && state != 'R')
        return false;
    if (ppid == KTHREADD_PID)
        return false;
    return true;

}

void CThreadInfo::ptraceAttach()
{
    if (canAttach()) {
        ptrace(PTRACE_ATTACH, tid, 0, 0);
        attach = ATT_WAIT_SIG;
        f_reason = FR_ATT_CNF;
        clock_gettime(CLOCK_REALTIME, &ar_ts);
    }
    f_reason = FR_ATT_REQ;
}

bool CThreadInfo::ptraceWaitSig()
{
    if (attach != ATT_WAIT_SIG)
        return false;
    if ((ptrace(PTRACE_GETSIGINFO, tid, 0, &si) < 0)
        && (errno == ESRCH || errno == EINTR))
        return true;
    else {
        attach = ATT_STOPPED;
        f_reason = FR_NONE;
        clock_gettime(CLOCK_REALTIME, &ac_ts);
    }
    return false;
}

void CThreadInfo::ptraceDetach()
{
    if (attach == ATT_STOPPED)
        ptrace(PTRACE_DETACH, tid, 0, 0);
    attach = ATT_DETACH;
    clock_gettime(CLOCK_REALTIME, &d_ts);
}
#endif
