/*
 * Copyright (c) 2014 Intel Corporation.
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

#ifndef CAMERA3_HAL_RAWBYPASSTASK_H_
#define CAMERA3_HAL_RAWBYPASSTASK_H_

#include "ProcUnitTask.h"

namespace android {
namespace camera2 {
/**
 * \class RawBypassTask
 * Currently returns RAW buffers in the request to the Framework
 *
 */
class RawBypassTask : public ProcUnitTask{
public:
    RawBypassTask();
    virtual ~RawBypassTask();

    virtual status_t executeTask(ProcTaskMsg &msg);
    virtual void getTaskOutputType(void);

private:
    virtual status_t handleExecuteTask(ProcTaskMsg &msg);

};

}  // namespace camera2
}  // namespace android

#endif  // CAMERA3_HAL_RAWBYPASSTASK_H_
