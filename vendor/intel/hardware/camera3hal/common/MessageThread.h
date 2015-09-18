/*
 * Copyright (C) 2013 Intel Corporation
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
#ifndef CAMERA3_HAL_IMESSAGETHREAD_H_
#define CAMERA3_HAL_IMESSAGETHREAD_H_
#include <utils/threads.h>
#include <utils/String8.h>

#define PRIORITY_CAMERA -10

namespace android {
namespace camera2 {

/* Abstraction of MessageThread */
class IMessageHandler
{
public:
    virtual ~IMessageHandler() { };
    virtual void messageThreadLoop(void) = 0;
};

class MessageThread : public Thread {
public:
    MessageThread(IMessageHandler* runner, const char* name,
                         int priority = PRIORITY_URGENT_DISPLAY + PRIORITY_LESS_FAVORABLE) :
        Thread(false), mRunner(runner), mName(name), mPriority(priority) { }
    virtual void onFirstRef() {
        run(mName.string(), mPriority);
    }
    virtual bool threadLoop() {
        mRunner->messageThreadLoop();
        return false;
    };
    virtual void messageThreadLoop(void) {};
private:
    IMessageHandler* mRunner;
    String8 mName;
    int mPriority;
};

} /* namespace camera2 */
} /* namespace android */
#endif /* CAMERA3_HAL_IMESSAGETHREAD_H_ */

