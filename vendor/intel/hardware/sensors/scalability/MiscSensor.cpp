#include "MiscSensor.hpp"
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define IO_CMD_ACTIVATE 0
#define IO_CMD_SETDELAY 1

typedef struct {
        union {
                int value;
                uint64_t config;
                int64_t timestamp;
        };
        axis_t axis;
} sensors_misc_event_t;

int MiscSensor::getPollfd()
{
        if (pollfd >= 0)
                return pollfd;

        pollfd = open(data.dataInterface.c_str(), O_RDONLY);
        if (pollfd < 0)
                ALOGE("%s: line: %d: open %s error!", __FUNCTION__, __LINE__, data.activateInterface.c_str());

        return pollfd;
}

int MiscSensor::hardwareSet(bool activated)
{
        int fd, ret;

        fd = getPollfd();
        if (fd < 0) {
                ALOGE("%s: line: %d: %s getPollfd error!", __FUNCTION__, __LINE__, data.name.c_str());
                return -1;
        }

        /* activate sensor */
        if (activated) {
                /* hardware is inactive now */
                if (!state.getActivated()) {
                        if (Calibration != NULL)
                                Calibration(&event, READ_DATA, data.calibrationFile.c_str());
                        ret = ioctl(fd, IO_CMD_ACTIVATE, 1);
                        if (ret) {
                                ALOGE("%s line:%d %s real active hardware sensor error! %d",
                                     __FUNCTION__, __LINE__, device.getName(), ret);
                                return -1;
                        }
                        state.setActivated(true);
                        if (DriverCalibration != NULL)
                                DriverCalibration(&event, CALIBRATION_DATA, data.driverCalibrationFile.c_str(), data.driverCalibrationInterface.c_str());
                }

                /* only change the delay */
                /* If the sensor is interrupt mode */
                if (device.getMinDelay() != 0 && data.setDelayInterface.length() != 0) {
                        ret = ioctl(fd, IO_CMD_SETDELAY, state.getDelay());
                        if (ret) {
                                ALOGE("%s: line: %d: %s set delay error! delay: %d", __FUNCTION__, __LINE__, data.name.c_str(), state.getDelay());
                                return -1;
                        }
                }
                return 0;
        }

        state.setActivated(false);
        if (Calibration != NULL)
                Calibration(&event, STORE_DATA, data.calibrationFile.c_str());

        ret = ioctl(fd, IO_CMD_ACTIVATE, 0);
        if (ret) {
                ALOGE("%s line:%d %s real deactive hardware sensor error! %d",
                     __FUNCTION__, __LINE__, device.getName(), ret);
                return -1;
        }
        return 0;
}

int MiscSensor::activate(int handle, int enabled) {
        if (handle != device.getHandle()) {
                ALOGE("%s: line: %d: %s handle not match! handle: %d required handle: %d",
                     __FUNCTION__, __LINE__, data.name.c_str(), device.getHandle(), handle);
                return -1;
        }

        return hardwareSet(enabled == 0 ? false : true);
}

int MiscSensor::setDelay(int handle, int64_t period_ns)
{
        int64_t delay;
        int minDelay = device.getMinDelay() / US_TO_MS;

        if (handle != device.getHandle()) {
                ALOGE("%s: line: %d: %s handle not match! handle: %d required handle: %d",
                     __FUNCTION__, __LINE__, data.name.c_str(), device.getHandle(), handle);
                return -1;
        }

        if (period_ns / 1000 == SENSOR_NOPOLL)
                delay = 0;
        else
                delay = period_ns / NS_TO_MS;

        if (delay < minDelay)
                delay = minDelay;

        if (state.getActivated() && delay != state.getDelay()) {
                state.setDelay(delay);
                return hardwareSet(true);
        }
        state.setDelay(delay);

        return 0;
}

int MiscSensor::getData(std::queue<sensors_event_t> &eventQue) {
        int count, ret;
        sensors_misc_event_t miscEvent[32];

        if (state.getFlushSuccess() == true) {
                eventQue.push(metaEvent);
                state.setFlushSuccess(false);
        }

        ret = read(pollfd, miscEvent, sizeof(miscEvent));

        if (ret == sizeof(int)) {
                event.data[0] = static_cast<float>(miscEvent[0].value) * device.getScale(0);
                event.timestamp = getTimestamp();
                if (Calibration != NULL)
                        Calibration(&event, CALIBRATION_DATA, NULL);
                /* auto disable one-shot sensor */
                if ((device.getFlags() & ~SENSOR_FLAG_WAKE_UP) == SENSOR_FLAG_ONE_SHOT_MODE)
                        activate(device.getHandle(), 0);
                eventQue.push(event);
                if (state.getFlushSuccess() == true) {
                        eventQue.push(metaEvent);
                        state.setFlushSuccess(false);
                }
                return 0;
        } else if (ret < 0 || ret % sizeof(sensors_misc_event_t) != 0) {
                ALOGE("%s line: %d, name: %s ret: %d",
                     __FUNCTION__, __LINE__, data.name.c_str(), ret);
                return -1;
        }

        count = ret / sizeof(sensors_misc_event_t);
        for (int i = 0; i < count; i++) {
                float value = static_cast<float>(miscEvent[i].value);
                switch(miscEvent[i].axis) {
                case AXIS_X:
                        event.data[device.getMapper(AXIS_X)] = value * device.getScale(AXIS_X);
                        break;
                case AXIS_Y:
                        event.data[device.getMapper(AXIS_Y)] = value * device.getScale(AXIS_Y);
                        break;
                case AXIS_Z:
                        event.data[device.getMapper(AXIS_Z)] = value * device.getScale(AXIS_Z);
                        break;
                case AXIS_OTHER:
                        if (miscEvent[i].config == SENSOR_BATCH_MODE_FLUSH_DONE || state.getFlushSuccess() == true) {
                                eventQue.push(metaEvent);
                                state.setFlushSuccess(false);
                        } else {
                                event.timestamp = miscEvent[i].timestamp;
                                if (Calibration != NULL)
                                        Calibration(&event, CALIBRATION_DATA, NULL);
                                else if (device.getEventProperty() == VECTOR)
                                        event.acceleration.status = SENSOR_STATUS_ACCURACY_MEDIUM;
                                /* auto disable one-shot sensor */
                                if ((device.getFlags() & ~SENSOR_FLAG_WAKE_UP) == SENSOR_FLAG_ONE_SHOT_MODE)
                                        activate(device.getHandle(), 0);
                                eventQue.push(event);
                        }
                default:
                        ALOGW("%s line: %d unknown axis: %d", __FUNCTION__, __LINE__, miscEvent[i].axis);
                        break;
                }
        }

        return 0;
}

bool MiscSensor::selftest() {
        if (getPollfd() >= 0)
                return true;
        return false;
}
