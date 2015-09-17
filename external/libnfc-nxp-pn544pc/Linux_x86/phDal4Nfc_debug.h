/*
 * Copyright (C) 2010-2013 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*-----------------------------------------------------------------------------------
                                   DEBUG CORNER
------------------------------------------------------------------------------------*/
#ifdef DAL_TRACE
#include <stdio.h>

#define MAX_TRACE_BUFFER    150

#define DAL_PRINT( str )  phOsalNfc_DbgString(str)
#define DAL_DEBUG(str, arg)     \
{                                       \
    char        trace[MAX_TRACE_BUFFER];                    \
    snprintf(trace,MAX_TRACE_BUFFER,str,arg);   \
    phOsalNfc_DbgString(trace);                 \
}

#define DAL_PRINT_BUFFER(msg,buf,len)       \
{                                       \
    uint16_t    i = 0;                  \
    char        trace[MAX_TRACE_BUFFER];                    \
    snprintf(trace,MAX_TRACE_BUFFER,"\n\t %s:",msg);    \
    phOsalNfc_DbgString(trace);                 \
    phOsalNfc_DbgTrace(buf,len);            \
    phOsalNfc_DbgString("\r");              \
}

#define DAL_ASSERT_STR(x, str)   { if (!(x)) { phOsalNfc_DbgString(str); while(1); } }

#else
#define DAL_PRINT( str )
#define DAL_DEBUG(str, arg)
#define DAL_PRINT_BUFFER(msg,buf,len)
#define DAL_ASSERT_STR(x, str)

#endif
