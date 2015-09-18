#include "PSHCommonSensor.hpp"

int PSHCommonSensor::getPollfd()
{
        if (pollfd >= 0)
                return pollfd;

        if (methods.psh_open_session == NULL || methods.psh_get_fd == NULL) {
                ALOGE("psh_open_session/psh_get_fd not initialized!");
                return -1;
        }

        psh_sensor_t PSHType = SensorHubHelper::getType(device.getType(), device.getSubname());
        sensorHandle = methods.psh_open_session(PSHType);
        if (sensorHandle == NULL) {
                ALOGE("psh_open_session error!");
                return -1;
        }

        pollfd = methods.psh_get_fd(sensorHandle);

        return pollfd;
}

int PSHCommonSensor::hardwareSet(bool activated)
{
        int dataRate = state.getDataRate();
        int ret = 0;

        if (!activated) {
                state.setActivated(activated);
                error_t err = methods.psh_stop_streaming(sensorHandle);
                if (err != ERROR_NONE) {
                        ALOGE("psh_stop_streaming error %d", err);
                        return -1;
                }
        } else {
                if (device.getFlags() & SENSOR_FLAG_WAKE_UP)
                        flag = NO_STOP_WHEN_SCREEN_OFF;
                /* Some PSH session need to set different rate and delay */
                SensorHubHelper::getStartStreamingParameters(device.getType(), dataRate, bufferDelay, flag);

                if (dataRate < 0) {
                        ALOGE("Invalid data rate: %d", dataRate);
                        return -1;
                }
                if (!state.getActivated()) {
                        /* Some PSH session need to set property */
                        if ((ret = SensorHubHelper::setPSHPropertyIfNeeded(device.getType(), methods, sensorHandle)) != ERROR_NONE) {
                                ALOGE("Set property failed for sensor type %d ret: %d", device.getType(), ret);
                                return -1;
                        }
                }

                error_t err;
                err = methods.psh_start_streaming_with_flag(sensorHandle, dataRate, bufferDelay, flag);
                if (err != ERROR_NONE) {
                        ALOGE("psh_start_streaming(_with_flag) error %d name:%s handle: %x %d %d",
                             err, device.getName(), sensorHandle, dataRate, flag);
                        return -1;
                }

                state.setActivated(activated);
                state.setDataRate(dataRate);
        }

        return 0;
}

int PSHCommonSensor::batch(int handle, int flags, int64_t period_ns, int64_t timeout) {
        int delay = period_ns / NS_TO_MS;
        int ret = -EINVAL;
        static int oldDataRate = -1;
        static int oldBufferDelay = -1;
        static streaming_flag oldFlag;

        if (handle != device.getHandle()) {
                ALOGE("%s: line: %d: %s handle not match! handle: %d required handle: %d",
                     __FUNCTION__, __LINE__, device.getName(), device.getHandle(), handle);
                return -EINVAL;
        }

        if (period_ns < 0 || timeout < 0)
                return -EINVAL;

        if (timeout == 0) {
                ret = setDelay(handle, period_ns);
                bufferDelay = 0;
                oldBufferDelay = 0;
                return ret;
        }

        bufferDelay = timeout / NS_TO_MS;

        ret = setDelay(handle, period_ns);

        if (oldDataRate == -1) {
                oldDataRate = state.getDataRate();
                oldBufferDelay = bufferDelay;
                oldFlag = flag;
                return ret;
        }

        if (oldDataRate != state.getDataRate() || oldBufferDelay != bufferDelay || oldFlag != flag) {
                oldDataRate = state.getDataRate();
                oldBufferDelay = bufferDelay;
                oldFlag = flag;
                if (state.getActivated())
                        ret = hardwareSet(true);
        }

        return ret;
}

int PSHCommonSensor::activate(int handle, int enabled) {
        if (methods.psh_start_streaming == NULL || methods.psh_stop_streaming == NULL || sensorHandle == NULL) {
                ALOGE("psh_start_streaming/psh_stop_streaming/sensorHandle not initialized!");
                return -1;
        }

        if(enabled == 0) {
                if (state.getActivated())
                        return hardwareSet(false);
                return 0;
        }

        if (!state.getActivated())
                return hardwareSet(true);
        return 0;
}

int PSHCommonSensor::setDelay(int handle, int64_t period_ns) {
        int dataRate = 5;
        int delay = 200;
        int minDelay = device.getMinDelay() / US_TO_MS;
        int maxDelay = device.getMaxDelay() / US_TO_MS;

        if (period_ns / 1000 != SENSOR_NOPOLL)
                delay = period_ns / NS_TO_MS;
        else
                delay = 200;

        if (delay < minDelay)
                delay = minDelay;

        if (delay > maxDelay)
                delay = maxDelay;

        if (delay != 0)
                dataRate = 1000 / delay;

        if (dataRate <= 0)
                dataRate = 1;

        if (state.getActivated() && dataRate != state.getDataRate()) {
                state.setDataRate(dataRate);
                return hardwareSet(true);
        }
        state.setDataRate(dataRate);

        return 0;
}

int PSHCommonSensor::getData(std::queue<sensors_event_t> &eventQue) {
        int count = 32;

        if (state.getFlushSuccess() == true) {
                eventQue.push(metaEvent);
                state.setFlushSuccess(false);
        }

        count = SensorHubHelper::readSensorhubEvents(pollfd, sensorhubEvent, count, device.getType());
        for (int i = 0; i < count; i++) {
                if (sensorhubEvent[i].timestamp == PSH_BATCH_MODE_FLUSH_DONE) {
                        /*   sensors_meta_data_event_t.version must be META_DATA_VERSION
                         *   sensors_meta_data_event_t.sensor must be 0
                         *   sensors_meta_data_event_t.type must be SENSOR_TYPE_META_DATA
                         *   sensors_meta_data_event_t.reserved must be 0
                         *   sensors_meta_data_event_t.timestamp must be 0
                         */
                        eventQue.push(metaEvent);
                        state.setFlushSuccess(false);
                } else {
                        if (device.getType() == SENSOR_TYPE_STEP_COUNTER) {
                                event.u64.step_counter = sensorhubEvent[i].step_counter;
                        } else {
                                event.data[device.getMapper(AXIS_X)] = sensorhubEvent[i].data[0] * device.getScale(AXIS_X);
                                event.data[device.getMapper(AXIS_Y)] = sensorhubEvent[i].data[1] * device.getScale(AXIS_Y);
                                event.data[device.getMapper(AXIS_Z)] = sensorhubEvent[i].data[2] * device.getScale(AXIS_Z);
                                event.data[device.getMapper(AXIS_W)] = sensorhubEvent[i].data[3] * device.getScale(AXIS_W);
                                if (sensorhubEvent[i].accuracy != 0)
                                        event.acceleration.status = sensorhubEvent[i].accuracy;
                        }
                        event.timestamp = sensorhubEvent[i].timestamp;
                        /* auto disable one-shot sensor */
                        if ((device.getFlags() & ~SENSOR_FLAG_WAKE_UP) == SENSOR_FLAG_ONE_SHOT_MODE)
                                activate(device.getHandle(), 0);
                        eventQue.push(event);
                }
        }

        return 0;
}

bool PSHCommonSensor::selftest() {
        if (getPollfd() >= 0)
                return true;
        return false;
}
