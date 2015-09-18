/*
**
** Copyright 2010, The Android Open Source Project
** Copyright (C) 2014 Intel Corporation
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
*/

#ifndef _ANDROID_MEMORYDUMPER_H_
#define _ANDROID_MEMORYDUMPER_H_
#include <utils/String8.h>

namespace android {
namespace camera2 {

#ifdef MEM_LEAK_CHECK
#define STUB
#define STAT_STUB
#else
#define STUB {}
#define STAT_STUB {return false;}
#endif


class MemoryDumper
{
 public:
    MemoryDumper()STUB;
    ~MemoryDumper()STUB;
    bool dumpHeap()STAT_STUB;
    bool saveMaps()STAT_STUB;

 private:
    bool copyfile(const char* sourceFile, const char* destFile);
    String8 m_fileName;
    int m_dumpNo;
}; //  namespace camera2
}; //  namespace android
};
#endif

