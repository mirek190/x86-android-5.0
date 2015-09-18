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

#define LOG_TAG "MemoryDumper"
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include <utils/Errors.h>  // for status_t
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/SystemClock.h>
#include <utils/Vector.h>
#include <cutils/properties.h>
#include <cutils/atomic.h>

#include <utils/misc.h>
#include <binder/IServiceManager.h>
#include "MemoryDumper.h"
#include "LogHelper.h"
#include "UtilityMacros.h"


namespace android {
namespace camera2 {


const size_t READ_BLOCK_SIZE = 4096;

#ifdef MEM_LEAK_CHECK
MemoryDumper::MemoryDumper():
    m_dumpNo(0)
{
    m_fileName.appendFormat("/data/memstatus_%d", getpid());
    LOG1("MemoryDumper created, dump file=%s", m_fileName.string());
}

MemoryDumper::~MemoryDumper()
{
    LOG1("MemoryDumper destructor.");
}

extern "C" void get_malloc_leak_info(uint8_t** info, size_t* overallSize,
        size_t* infoSize, size_t* totalMemory, size_t* backtraceSize);
extern "C" void free_malloc_leak_info(uint8_t* info);

bool MemoryDumper::dumpHeap()
{
    LOG1("enter dumpHeap");
    bool isDumpSuccess = false;
    String8 fileName(m_fileName);
    fileName.appendFormat(".%d", m_dumpNo);
    m_dumpNo++;
    FILE *f = fopen(fileName.string(), "w+");
    if  (f == NULL)  {
        LOGE("Open file failed: %s!", fileName.string());
        return false;
    }
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    typedef struct {
        size_t size;
        size_t dups;
        intptr_t * backtrace;
    } AllocEntry;

    uint8_t *info = NULL;
    size_t overallSize = 0;
    size_t infoSize = 0;
    size_t totalMemory = 0;
    size_t backtraceSize = 0;

    get_malloc_leak_info(&info, &overallSize, &infoSize,
                         &totalMemory, &backtraceSize);
    LOG2("returned from get_malloc_leak_info, info = %p, overallSize = %d,\
          infoSize = %d, totalMemory = %d, backtraceSize = %d",
          info, overallSize, infoSize, totalMemory, backtraceSize);
    if (info) {
        uint8_t *ptr = info;
        size_t count = overallSize / infoSize;

        snprintf(buffer, SIZE, " Allocation count %i\n", count);
        result.append(buffer);
        snprintf(buffer, SIZE, " Total meory %i\n", totalMemory);
        result.append(buffer);

        AllocEntry * entries = new AllocEntry[count];

        for (size_t i = 0; i < count; i++) {
            // Each entry should be size_t, size_t, intptr_t[backtraceSize]
            AllocEntry *e = &entries[i];

            e->size = *reinterpret_cast<size_t *>(ptr);
            ptr += sizeof(size_t);

            e->dups = *reinterpret_cast<size_t *>(ptr);
            ptr += sizeof(size_t);

            e->backtrace = reinterpret_cast<intptr_t *>(ptr);
            ptr += sizeof(intptr_t) * backtraceSize;
        }

        // Now we need to sort the entries.  They come sorted by size but
        // not by stack trace which causes problems using diff.
        bool moved;
        do {
            moved = false;
            for (size_t i = 0; i < (count - 1); i++) {
                AllocEntry *e1 = &entries[i];
                AllocEntry *e2 = &entries[i+1];

                bool swap = e1->size < e2->size;
                if (e1->size == e2->size) {
                    for (size_t j = 0; j < backtraceSize; j++) {
                        if (e1->backtrace[j] == e2->backtrace[j]) {
                            continue;
                        }
                        swap = e1->backtrace[j] < e2->backtrace[j];
                        break;
                    }
                }
                if (swap) {
                    AllocEntry t = entries[i];
                    entries[i] = entries[i+1];
                    entries[i+1] = t;
                    moved = true;
                }
            }
        } while (moved);

        for (size_t i = 0; i < count; i++) {
            AllocEntry *e = &entries[i];

            snprintf(buffer, SIZE, "size %8i, dup %4i, ", e->size, e->dups);
            result.append(buffer);
            for (size_t ct = 0; (ct < backtraceSize) && e->backtrace[ct]; ct++) {
                if (ct) {
                    result.append(", ");
                }
                snprintf(buffer, SIZE, "0x%08x", e->backtrace[ct]);
                result.append(buffer);
            }
            result.append("\n");
        }

        delete[] entries;
        free_malloc_leak_info(info);
        isDumpSuccess = true;
    }
    if (isDumpSuccess) {
        write(fileno(f), result.string(), result.size());
        String8 mapsfile(fileName);
        mapsfile.append(".maps");
        LOG2("save maps file to: %s", (const char*)mapsfile);
        copyfile("/proc/self/maps", (const char*)mapsfile);
        String8 smapsfile(fileName);
        smapsfile.append(".smaps");
        LOG2("save smaps file to: %s", (const char*)smapsfile);
        copyfile("/proc/self/smaps", (const char*)smapsfile);
        String8 statmfile(fileName);
        statmfile.append(".statm");
        LOG2("save statm file to: %s", (const char*)statmfile);
        copyfile("/proc/self/statm", (const char*)statmfile);
    }
    fclose(f);
    LOG1("exit memStatus, isDumpSuccess=%d", isDumpSuccess);
    return isDumpSuccess;
}

bool MemoryDumper::saveMaps()
{
    LOG1("enter saveMaps");
    String8 mapsfile = m_fileName;
    mapsfile.append(".maps");
    return copyfile("/proc/self/maps", mapsfile.string());
}

bool MemoryDumper::copyfile(const char* sourceFile, const char* destFile)
{
    LOG1("copy file, sourceFile=%s, destFile=%s", sourceFile, destFile);
    FILE* src = fopen(sourceFile, "r");
    if (src == NULL) {
        LOGE("Open source file failed: %s!", sourceFile);
        return false;
    }
    FILE* dest = fopen(destFile, "w+");
    if (dest == NULL) {
        LOGE("Open dest file failed: %s!", destFile);
        fclose(src);
        return false;
    }
    char* buffer = new char[READ_BLOCK_SIZE + 1];
    int readNum = 0;
    while (!feof(src)) {
        readNum = fread(buffer, 1, READ_BLOCK_SIZE, src);
        if (readNum > 0) {
            fwrite(buffer, 1, readNum, dest);
        } else {
            LOGE("Read error, readNum=%d, errno=%d", readNum, errno);
            break;
        }
    }
    delete buffer;
    buffer = NULL;
    fclose(src);
    fclose(dest);
    LOG1("exit copy file.");
    return true;
}
#else
/**
 * We need to add the dummy declaration of methods with parameters to avoid
 * the compiler warning about unused parameters
 */
bool copyfile(const char* sourceFile, const char* destFile)
{
    UNUSED(destFile);
    UNUSED(sourceFile);
    return false;
}
#endif

};// namespace camera2
};// namespace android

