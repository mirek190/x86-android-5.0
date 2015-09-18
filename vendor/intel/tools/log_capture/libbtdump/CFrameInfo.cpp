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

#include "CFrameInfo.h"
#include <stdlib.h>
#include <string.h>

CFrameInfo::~CFrameInfo()
{
    free(sym);
}

#if defined(USE_LIBUNWIND) || defined (USE_LIBBACKTRACE)
/**
 * Dump the full textual description of the frame in
 * the provided output file.
 * @param output
 */
void CFrameInfo::print(FILE *output)
{
#else
void CFrameInfo::print(FILE __attribute__ ((unused)) * output)
{
#endif
#ifdef USE_LIBUNWIND
    fprintf(output, "%p %s+%x\n", (void *)ip, sym, (unsigned int)offset);
#elif defined (USE_LIBBACKTRACE)
    fprintf(output, "%s\n", sym);
#endif
}

#ifdef	USE_LIBUNWIND
/**
 * New frame info
 * @param ip the frame program counter
 * @param sp the stack pointer
 * @param sym textual description of the current procedure
 * @param offset
 */
CFrameInfo::CFrameInfo(unw_word_t ip, unw_word_t sp, const char *sym,
                       unw_word_t offset)
{
    this->ip = ip;
    this->sp = sp;
    this->offset = offset;
    if (sym && *sym)
        this->sym = strdup(sym);
    else
        this->sym = NULL;
}
#endif

#ifdef	USE_LIBBACKTRACE
/**
 * New frame info
 * @param sym full textual description of the current frame
 */
CFrameInfo::CFrameInfo(const char *sym)
{
    if (sym && *sym)
        this->sym = strdup(sym);
    else
        this->sym = NULL;
}
#endif
