#ifndef _PSH_UNCALIBRATED_SENSOR_H_
#define _PSH_UNCALIBRATED_SENSOR_H_

#include "PSHCommonSensor.hpp"
#include <pthread.h>

class PSHUncalibratedSensor : public PSHCommonSensor {
        float bias[3];
        char calibration_file[17]; // "/data/"(6 byte) + name(5 byte) + ".conf" (5byte) + 0
        int calibration_fd;
        struct cmd_calibration_param param;
        pthread_t tid;
        bool thread_exit;
        int thread_wakeup_fds[2];
        char thread_wakeup_buf;
        static void* calibration_monitor(void* arg);
public:
        PSHUncalibratedSensor(SensorDevice &mDevice);
        ~PSHUncalibratedSensor();
        int getData(std::queue<sensors_event_t> &eventQue);
        bool selftest();
};

#endif
