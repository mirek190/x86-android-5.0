/*
 * INTEL CONFIDENTIAL
 *
 * Copyright (C) 2010 - 2013 Intel Corporation.
 * All Rights Reserved.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or licensors. Title to the Material remains with Intel
 * Corporation or its licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No License under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or
 * delivery of the Materials, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights
 * must be express and approved by Intel in writing.
 */

#ifndef _ISPTD_ISP_H
#define _ISPTD_ISP_H

#include "assert_support.h"

#ifndef __HIVECC
#define ENABLETD
#endif

#define TDBASE 770000
#define TDPRINTTIME 1
#define TDPRINTVAL  2
#define TDPRINTSTR  3

// tags
#define TDNAM 20
#define TDDNM 40
#define TDSTA 60
#define TDSTO 80
#define TDDSC 100
#define TDVAL 120

#define GET_TAG_STR(tag)  (tag == TDNAM) ? "NAM" : \
                          (tag == TDDNM) ? "DNM" : \
                          (tag == TDSTA) ? "STA" : \
                          (tag == TDSTO) ? "STO" : \
                          (tag == TDDSC) ? "DSC" : \
                          (tag == TDVAL) ? "VAL" : \
                          ""

// types for STA STO CRE DEL 
#define TDTASK 0
#define TDISR 1
#define TDSEM 2
#define TDQUEUE 3
#define TDEVENT 4
#define TDAGENT 8
#define TDPORT 11

// types for OCC
#define TDNOTE 7

// types for VAL
#define TDVALUE 5
#define TDCYCLES 6
#define TDMEMCYCL 9

// types for DSC
#define TDSTRING 0
#define TDNUM 1
#define TDTIME 2
#define TDCOLOR 3


#ifdef ENABLETD

#ifndef __HIVECC
static int time = 0;
static FILE* tdfile = NULL;
#endif

// --------------------
// Internal fucntions
// --------------------
static inline void
td_print_tag(int tag, int type, int id)
{
#ifdef __HIVECC
  OP___dump(TDBASE + tag + type, id);
#else
  OP___assert(tdfile);
  fprintf(tdfile, "%s %d %d ", GET_TAG_STR(tag), type, id);
#endif
}

static inline void
td_print_time(int delta)
{
#ifdef __HIVECC
  OP___dump(TDBASE + TDPRINTTIME, OP___cycles());
#else
  OP___assert(tdfile);
  fprintf(tdfile, "%d\n", time + delta);
#endif
}

static inline void
td_print_val(int val)
{
#ifdef __HIVECC
  OP___dump(TDBASE + TDPRINTVAL, val);
#else
  OP___assert(tdfile);
  fprintf(tdfile, "%d\n", val);
#endif
}


static inline void
td_print_str(const char *str)
{
#ifdef __HIVECC
//  OP___dump(TDBASE + TDPRINTSTR, "bla");
#else
  OP___assert(tdfile);
  fprintf(tdfile, "%s\n", str);
#endif
}

// --------------------
// public fucntions
// --------------------

static inline void
td_register_name(int type, int id, const char *name)
{
  td_print_tag(TDNAM, type, id);
  td_print_str(name);
}

static inline void
td_register_descriptor(int type, int id, const char *name)
{
  td_print_tag(TDDNM, 0, id);
  td_print_str(name);
}

static inline void
td_start_block(int type, int id)
{
  td_print_tag(TDSTA, type, id);
  td_print_time(0);
}

static inline void
td_stop_block(int type, int id)
{
  td_print_tag(TDSTO, type, id);
  td_print_time(0);
}

static inline void
td_start_agent(int id)
{
  td_print_tag(TDSTA, TDAGENT, id);
  td_print_time(0);
}

static inline void
td_stop_agent(int id)
{
  td_print_tag(TDSTO, TDAGENT, id);
  td_print_time(0);
}

static inline void
td_delayed_stop_agent(int id, int delay)
{
  td_print_tag(TDSTO, TDAGENT, id);
  td_print_time(delay);
}

static inline void
td_log_descriptor(int type, int id, int val)
{
  td_print_tag(TDDSC, type, id);
  td_print_val(val);
}

static inline void
td_log_descriptor_str(int type, int id, const char *str)
{
  td_print_tag(TDDSC, type, id);
  td_print_str(str);
}

static inline void
td_init(void)
{
#ifndef __HIVECC
  printf("opening tdfile\n");
  tdfile = fopen("lineloop.tdi","w");
  if (tdfile == NULL) printf("fopen tdfile failed\n");
#endif
}

static inline void
td_init_fname(const char *filename)
{
#ifndef __HIVECC
  printf("opening tdfile %s\n", filename);
  tdfile = fopen(filename,"w");
  if (tdfile == NULL) printf("fopen tdfile failed\n");
#endif
}

static inline void
td_close(void)
{
#ifndef __HIVECC
  fclose(tdfile);
  printf("closing tdfile\n");
#endif
}

static inline void
td_increment_time(unsigned t)
{
// time increment function is only for crun, because in crun there is no notion of time, so the programmer should put time increments with estimated times in the code.
#ifndef __HIVECC
  time += t;
#endif
}
#else
static inline void
td_register_name(int type, int id, const char *name)
{
  (void) type;
  (void) id;
  (void) name;
}

static inline void
td_register_descriptor(int type, int id, const char *name)
{
  (void) type;
  (void) id;
  (void) name;
}

static inline void
td_start_block(int type, int id)
{
  (void) type;
  (void) id;
}

static inline void
td_stop_block(int type, int id)
{
  (void) type;
  (void) id;
}

static inline void
td_start_agent(int id)
{
  (void) id;
}

static inline void
td_stop_agent(int id)
{
  (void) id;
}

static inline void
td_log_descriptor(int type, int id, int val)
{
  (void) type;
  (void) id;
  (void) val;
}

static inline void
td_log_descriptor_str(int type, int id, const char *str)
{
  (void) type;
  (void) id;
  (void) str;
}

static inline void
td_init(void)
{
}

static inline void
td_init_fname(const char *filename)
{
}

static inline void
td_close(void)
{
}

static inline void
td_increment_time(unsigned t)
{
  (void) t;
}

#endif
#endif /* _ISPTD_ISP_H */
