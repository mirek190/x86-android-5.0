/*
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
 **
 */
#include "ad_streamer_cmd.h"


int main(int argc, char *argv[]) {

    ad_streamer_cmd_t cmd;

    if (streamer_cmd_from_command_line(argc, argv, &cmd) == 0) {

        return process_ad_streamer_cmd(&cmd);
    }
    return -1;
}
