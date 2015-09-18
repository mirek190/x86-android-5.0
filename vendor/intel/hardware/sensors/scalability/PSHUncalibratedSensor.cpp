#include <sys/inotify.h>
#include <fcntl.h>
#include "PSHUncalibratedSensor.hpp"

#define UNCALIBRATED_DEBUG false

void* PSHUncalibratedSensor::calibration_monitor(void* arg)
{
        fd_set rfds;
        int fd_inotify, wd, ret = -1;
        float offset[3];
        bool calibration_changed = false;
        struct cmd_calibration_param param;
        struct inotify_event ievent[16];
        PSHUncalibratedSensor* data = reinterpret_cast<PSHUncalibratedSensor*>(arg);

        fd_inotify = inotify_init();
        if (fd_inotify < 0) {
                ALOGE("%s line: %d Cannot initialize inotify!", __FUNCTION__, __LINE__);
                return NULL;
        }

        wd = inotify_add_watch(fd_inotify, data->calibration_file, IN_MODIFY);
        if (wd < 0) {
                ALOGE("Cannot watch file: %s %d", data->calibration_file, wd);
        }

        int maxfd = data->thread_wakeup_fds[0] > fd_inotify ? data->thread_wakeup_fds[0] + 1 : fd_inotify + 1;

        while(!data->thread_exit) {
                FD_ZERO(&rfds);
                FD_SET(fd_inotify, &rfds);
                FD_SET(data->thread_wakeup_fds[0], &rfds);
                ret = select(maxfd, &rfds, NULL, NULL, NULL);
                if (ret <= 0) {
                        ALOGE("%s line: %d select fd_inotify error: %d", __FUNCTION__, __LINE__, ret);
                        continue;
                }

                if (FD_ISSET(fd_inotify, &rfds)) {
                        ret = read(fd_inotify, ievent, 16 * sizeof(struct inotify_event));
                        if (ret < 0) {
                                ALOGE("%s line: %d read fd_inotify error: %d", __FUNCTION__, __LINE__, ret);
                                continue;
                        }
                        for (int i = 0; i < ret / sizeof(struct inotify_event); i++) {
                                if (ievent[i].wd == wd) {
                                        calibration_changed = true;
                                        break;
                                }
                        }
                }

                if (calibration_changed) {
                        pread(data->calibration_fd, &param, sizeof(param), 0);
                        if (SensorHubHelper::getCalibrationOffset(data->device.getType(), &param, offset)) {
                                if(offset[0] != 0 || offset[1] != 0 || offset[2] != 0) {
                                        offset[0] *= data->device.getScale(0);
                                        offset[1] *= data->device.getScale(1);
                                        offset[2] *= data->device.getScale(2);
                                        if (offset[0] != data->bias[0])
                                                data->bias[0] = offset[0];
                                        if (offset[1] != data->bias[1])
                                                data->bias[1] = offset[1];
                                        if (offset[2] != data->bias[2])
                                                data->bias[2] = offset[2];
                                        ALOGD_IF(UNCALIBRATED_DEBUG, "%s offset changed! %f %f %f",
                                                data->device.getName(), data->bias[0],
                                                data->bias[1], data->bias[2]);
                                }
                        }
                        calibration_changed= false;
                }
        }

        if (fd_inotify >= 0)
                close(fd_inotify);

        return NULL;
}

PSHUncalibratedSensor::PSHUncalibratedSensor(SensorDevice &mDevice)
        :PSHCommonSensor(mDevice)
{
        int err;

        for (int i = 0; i < 3; i++) {
                bias[i] = 0;
        }
        calibration_file[16] = 0;
        calibration_fd = -1;
        tid = -1;
        thread_exit = false;
        thread_wakeup_buf = 'w';
        memset(&param, 0, sizeof(param));

        if (SensorHubHelper::getCalibrationFileName(device.getType(), calibration_file)) {
                calibration_fd = open(calibration_file, O_RDONLY);
                if (calibration_fd < 0) {
                        ALOGE("%s line: %d Cannot open file: %s fd: %d",
                             __FUNCTION__, __LINE__, calibration_file, calibration_fd);
                        return;
                }
                pread(calibration_fd, &param, sizeof(param), 0);

                if (SensorHubHelper::getCalibrationOffset(device.getType(), &param, bias)) {
                        bias[AXIS_X] *= device.getScale(AXIS_X);
                        bias[AXIS_Y] *= device.getScale(AXIS_Y);
                        bias[AXIS_Z] *= device.getScale(AXIS_Z);
                }
                ALOGD_IF(UNCALIBRATED_DEBUG, "%s bias %f %f %f", device.getName(), bias[AXIS_X], bias[AXIS_Y], bias[AXIS_Z]);

                if (pipe(thread_wakeup_fds)) {
                        ALOGE("%s line: %d %s pipe error: %d", __FUNCTION__, __LINE__, device.getName(), errno);
                        return;
                }

                err = pthread_create(&tid, NULL, PSHUncalibratedSensor::calibration_monitor, reinterpret_cast<void*>(this));
                if (err) {
                        ALOGE("%s calibration_monitor thread create error: %d", device.getName(), err);
                        tid = -1;
                        return;
                }
        }
}

PSHUncalibratedSensor::~PSHUncalibratedSensor()
{
        int ret;
        thread_exit = true;
        if (thread_wakeup_fds[1] >= 0) {
                ret = write(thread_wakeup_fds[1], &thread_wakeup_buf, 1);
                if (tid > 0 && ret > 0)
                        pthread_join(tid, NULL);
                close(thread_wakeup_fds[1]);
        }
        if (thread_wakeup_fds[0] >= 0)
                close(thread_wakeup_fds[0]);
        if (calibration_fd >= 0)
                close(calibration_fd);
}

int  PSHUncalibratedSensor::getData(std::queue<sensors_event_t> &eventQue)
{
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
                        for (int j = 0; j < 3 ; j++) {
                                event.uncalibrated_gyro.uncalib[j] = sensorhubEvent[i].data[j] * device.getScale(j) + bias[j];
                                event.uncalibrated_gyro.bias[j] = bias[j];
                                ALOGD_IF(UNCALIBRATED_DEBUG, "%s %s: %d %f %f", device.getName(), __FUNCTION__, j, event.uncalibrated_gyro.uncalib[j], bias[j]);
                        }
                        event.timestamp = sensorhubEvent[i].timestamp;
                        eventQue.push(event);
                }
        }
        return 0;
}

bool  PSHUncalibratedSensor::selftest()
{
        if (getPollfd() >= 0 && calibration_fd >= 0)
                return true;
        return false;
}
