/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_SENSOR_HELPERS_H
#define ANDROID_SENSOR_HELPERS_H

#include <string>

class PathOps {
    std::string mBasePath;

public:
        PathOps():mBasePath("") {};
        PathOps(const char *basePath):mBasePath(basePath) {}

    /* write data to base path (dir) + provided path */
    int write(const std::string &path, const std::string &buf);
    int write(const std::string &path, unsigned int data);

    /* read data from base path (dir) + provided path */
    int read(const std::string &path, char *buf, int len);
    int read(const std::string &path, std::string &buf);

    const char *basePath() { return mBasePath.c_str(); }

    bool exists(const std::string &path);
    bool exists();
};

inline unsigned int nsToMs(int64_t ns) { return ns/1000000; }

#endif
