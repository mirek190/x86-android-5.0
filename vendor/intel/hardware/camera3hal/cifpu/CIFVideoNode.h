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

#ifndef CIFVIDEONODE_H_
#define CIFVIDEONODE_H_

#include "v4l2device.h"

namespace android {
namespace camera2 {

class CIFVideoNode : public V4L2VideoNode {
public:
    CIFVideoNode(const char *name,
                 VideNodeDirection nodeDirection = INPUT_VIDEO_NODE,
                 VideoNodeType nodeType = VIDEO_GENERIC);
    virtual ~CIFVideoNode() {}

    virtual status_t setFormat(FrameInfo &aConfig);
    virtual int stop(bool keepBuffers);
    virtual void destroyBufferPool();

// prevent copy constructor and assignment operator
private:
    CIFVideoNode(const CIFVideoNode& other);
    CIFVideoNode& operator=(const CIFVideoNode& other);
};

} /* namespace camera2 */
} /* namespace android */
#endif /* CIFVIDEONODE_H_ */
