/* HALAudioDump.cpp
 **
 ** Copyright 2013 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **      http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include "HALAudioDump.h"

#define LOG_TAG "HALAudioDump"

#include <cutils/log.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <utils/Errors.h>

using namespace android;
using namespace std;

const char *CHALAudioDump::streamDirections[] = { "in", "out" };
const char *CHALAudioDump::dumpDirPath = "/logs/audio_dumps";
const uint32_t CHALAudioDump::MAX_NUMBER_OF_FILES = 4;

CHALAudioDump::CHALAudioDump()
    : dumpFile(0), fileCount(0)
{
    uint32_t status;

    status = mkdir(dumpDirPath, S_IRWXU | S_IRGRP | S_IROTH);

    if ((status != EEXIST) && (status != NO_ERROR)) {
      LOGE("Error in creating the audio dumps directory at %s.", dumpDirPath);
    }
}

CHALAudioDump::~CHALAudioDump()
{
    if (dumpFile) {
        Close();
    }
}

void CHALAudioDump::dumpAudioSamples(const void *buffer,
                                     ssize_t bytes,
                                     bool isOutput,
                                     uint32_t sRate,
                                     uint32_t chNb,
                                     const std::string &nameContext)
{
    int32_t write_status;

    if (dumpFile == 0) {
        char *audio_file_name;

        /**
         * A new dump file is created for each stream relevant to the dump needs
         */
        asprintf(&audio_file_name,
                 "%s/audio_%s_%dKhz_%dch_%s_%d.pcm",
                 dumpDirPath,
                 streamDirectionStr(isOutput),
                 sRate,
                 chNb,
                 nameContext.c_str(),
                 ++fileCount);

        if (!audio_file_name) {
            return;
        }

        dumpFile = fopen(audio_file_name, "wb");

        if (dumpFile == NULL) {
            ALOGE("Cannot open dump file %s, errno %d, reason: %s",
                  audio_file_name,
                  errno,
                  strerror(errno));
            free(audio_file_name);
            return;
        }

        ALOGI("Audio %put stream dump file %s, fh %p opened.", streamDirectionStr(isOutput),
              audio_file_name,
              dumpFile);
        free(audio_file_name);
    }

    write_status = writeDumpFile(buffer, bytes);

    if (write_status == BAD_VALUE) {

        /**
         * Max file size reached. Close file to open another one
         * Up to 4 dump files are dumped. When 4 dump files are
         * reached the last file is reused circularly.
         */
        Close();
    }

    /**
     * Roll on 4 files, to keep at least the last audio dumps
     * and to split dumps for more convenience.
     */
    if (write_status == INVALID_OPERATION) {
        char *fileToRemove;

        asprintf(&fileToRemove,
                 "%s/audio_%s_%dKhz_%dch_%s_%d.pcm",
                 dumpDirPath,
                 streamDirectionStr(isOutput),
                 sRate,
                 chNb,
                 nameContext.c_str(),
                 fileCount);

        if (!fileToRemove) {
            return;
        }

        fileCount--;

        remove(fileToRemove);
        free(fileToRemove);
        Close();
    }
}

void CHALAudioDump::Close()
{
    fclose(dumpFile);
    dumpFile = NULL;
}

const char *CHALAudioDump::streamDirectionStr(bool isOutput) {

    return streamDirections[isOutput];
}


int32_t CHALAudioDump::writeDumpFile(const void *buffer,
                                  ssize_t bytes)
 {
    struct stat stDump;

    /**
     * Check that file size does not exceed its maximum size
     **/
    if (dumpFile && fstat(fileno(dumpFile), &stDump) == 0
        && (stDump.st_size + bytes) < MAX_DUMP_FILE_SIZE) {
        if (fwrite(buffer, bytes, 1, dumpFile) != 1) {
            ALOGE("Error writing PCM in audio dump file : %s", strerror(errno));
            return BAD_VALUE;
        }
    } else if (fileCount >= MAX_NUMBER_OF_FILES) {
        return INVALID_OPERATION;
    }
    /**
     * Max file size reached
     */
    else {
        return BAD_VALUE;
    }

    return NO_ERROR;
}
