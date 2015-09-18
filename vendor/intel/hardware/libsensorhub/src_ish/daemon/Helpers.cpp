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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "../include/iio-hal/Helpers.h"

int PathOps::write(const std::string &path, const std::string &buf)
{
    std::string p = mBasePath + path;
    int fd = ::open(p.c_str(), O_WRONLY);
    if (fd < 0)
        return -errno;

    int ret = ::write(fd, buf.c_str(), buf.size());
    close(fd);

    return ret;
}

int PathOps::write(const std::string &path, unsigned int data)
{
    std::ostringstream os;
    os << data;
    return PathOps::write(path, os.str());
}

int PathOps::read(const std::string &path, char *buf, int len)
{
    std::string p = mBasePath + path;
    int fd = ::open(p.c_str(), O_RDONLY);
    if (fd < 0)
        return -errno;

    int ret = ::read(fd, buf, len);
    close(fd);

    return ret;
}

int PathOps::read(const std::string &path, std::string &buf)
{
    std::string p = mBasePath + path;
    /* Using fstream for reading stuff into std::string */
    std::ifstream f(p.c_str(), std::fstream::in);
    if (f.fail())
        return -EINVAL;

    int ret = 0;
    f >> buf;
    if (f.bad())
        ret = -EIO;
    f.close();

    return ret;
}

bool PathOps::exists(const std::string &path)
{
    struct stat s;
    return (bool) (stat((mBasePath + path).c_str(), &s) == 0);
}

bool PathOps::exists()
{
    return PathOps::exists("");
}
