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

#include "CalibrationSensor.hpp"

#include <sys/types.h>
#include <unistd.h>
#include <libsensorhub.h>
#include <sys/time.h>

CalibrationSensor::CalibrationSensor(SensorDevice &device) :
        PSHSensor(device), mEnabled(0), mSensorType(SENSOR_INVALID) {

    pipe(mResultPipe);
//    pipe(mWakeupPipe);
    mWakeupPipe[0] = mWakeupPipe[1] = -1;
}

CalibrationSensor::~CalibrationSensor() {
    if (mWakeupPipe[0] != -1)
        close(mWakeupPipe[0]);

    if (mWakeupPipe[1] != -1)
        close(mWakeupPipe[1]);

    if (mResultPipe[0] != -1)
        close(mResultPipe[0]);

    if (mResultPipe[1] != -1)
        close(mResultPipe[1]);
}

int CalibrationSensor::getPollfd() {
    return mResultPipe[0];
}

/* We use different data rate to indicate the calibration of different sensor types
   10000000: SENSOR_GYRO
   20000000: SENSOR_COMP */
int CalibrationSensor::setDelay(int32_t handle, int64_t ns) {
    ALOGD("setDelay, %lld\n", ns);

    if (ns == 10000000)
        mSensorType = SENSOR_GYRO;
    else if (ns == 20000000)
        mSensorType = SENSOR_COMP;

    return 0;
}

int CalibrationSensor::activate(int32_t handle, int en) {
    ALOGD("activate(), %d; sensor type, %d\n", en, mSensorType);

    if (mSensorType == SENSOR_INVALID) {
        ALOGI("setDelay() not called \n");
        return -1;
    }

    if (mEnabled == en) {
        ALOGI("Duplicate request");
        return -1;
    }

    mEnabled = en;

    if (mEnabled == 1) {
        if (mWakeupPipe[0] != -1) {
              close(mWakeupPipe[0]);
              mWakeupPipe[0] = -1;
        }
        if (mWakeupPipe[1] != -1) {
              close(mWakeupPipe[1]);
              mWakeupPipe[1] = -1;
        }

        pipe(mWakeupPipe);
        if ((mWorkerThread = androidCreateThread(workerThread, this)) > 0) {
            ALOGD("mWorkerThread = %ld", mWorkerThread);
        } else {
            ALOGE("thread created error");
        }
    } else if (mEnabled == 0) {
        mSensorType = SENSOR_INVALID;
        write(mWakeupPipe[1], "a", 1);
        ALOGD("write to wakeup pipe \n");
    }

    return 0;
}

int CalibrationSensor::workerThread(void* data) {
    CalibrationSensor *src = (CalibrationSensor *)data;
    handle_t handle;
    struct cmd_calibration_param param;
    struct cmd_calibration_param saved_param;
    error_t error;
    struct timeval tv;

    handle = methods.psh_open_session(src->mSensorType);
    if (handle == NULL) {
        ALOGE("psh_open_session() failed \n");
        return -1;
    }

    param.sub_cmd = SUBCMD_CALIBRATION_START;
    error = methods.psh_set_calibration(handle, &param);
    if (error != ERROR_NONE) {
        ALOGE("psh_set_calibration() failed \n");
        return -1;
    }

    saved_param.calibrated = 0;

    while (1) {
        unsigned int result;
        fd_set read_fds;
        int ret;

        FD_ZERO(&read_fds);
        FD_SET(src->mWakeupPipe[0], &read_fds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(src->mWakeupPipe[0] + 1, &read_fds, NULL, NULL, &tv);
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            else
                break;
        } else if (FD_ISSET(src->mWakeupPipe[0], &read_fds)) {
            if (saved_param.calibrated != 0) {
                saved_param.sub_cmd = SUBCMD_CALIBRATION_SET;
                methods.psh_set_calibration(handle, &saved_param);
            }
            ALOGD("mWakeupPipe is received \n");
            break;
        }

        // ret == 0
        memset(&param, 0, sizeof(struct cmd_calibration_param));
        error = methods.psh_get_calibration(handle, &param);

        if (error != ERROR_NONE) {
            ALOGE("psh_get_calibration() failed \n");
            return -1;
        }

        result = param.calibrated;
        write(src->mResultPipe[1], &result, sizeof(result));

        ALOGD("result is %d \n", result);
        if (SUBCMD_CALIBRATION_TRUE == result) {
              param.sub_cmd = SUBCMD_CALIBRATION_SET;
              methods.psh_set_calibration(handle, &param);
              break;
        } else if (result > saved_param.calibrated) {
              saved_param = param;

        }
    }

    param.sub_cmd = SUBCMD_CALIBRATION_STOP;
    methods.psh_set_calibration(handle, &param);

    methods.psh_close_session(handle);

    return 0;
}

int CalibrationSensor::getData(std::queue<sensors_event_t> &eventQue) {
    int size, numEventReceived = 0;
    unsigned int *p_result;
    char buf[32], *p;

    if (state.getFlushSuccess() == true) {
        eventQue.push(metaEvent);
        state.setFlushSuccess(false);
        ALOGI("metaEvent reported");
    }

    size = read(mResultPipe[0], buf, 32);

    p = buf;
    while (size > 0) {
        p_result = (unsigned int *)p;
        event.data[0] = *p_result;
        event.timestamp = getTimestamp();
        eventQue.push(event);
        numEventReceived++;

        size = size - sizeof(*p_result);
        p = p + sizeof(*p_result);
    }

    return numEventReceived;
}

bool CalibrationSensor::selftest() {

    return true;
}

