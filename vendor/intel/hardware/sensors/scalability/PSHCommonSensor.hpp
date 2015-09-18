#ifndef _PSH_COMMON_SENSOR_HPP_
#define _PSH_COMMON_SENSOR_HPP_

#include "PSHSensor.hpp"

class PSHCommonSensor : public PSHSensor {
protected:
        struct sensorhub_event_t sensorhubEvent[32];
        int bufferDelay;
        streaming_flag flag;
public:
        PSHCommonSensor(SensorDevice &mDevice) :PSHSensor(mDevice)
        {
                memset(sensorhubEvent, 0, 32 * sizeof(struct sensorhub_event_t));
                bufferDelay = 0;
                flag = STOP_WHEN_SCREEN_OFF;
        }
        virtual ~PSHCommonSensor()
        {
                if (sensorHandle != NULL)
                        methods.psh_close_session(sensorHandle);
        }
        int getPollfd();
        int activate(int handle, int enabled);
        int setDelay(int handle, int64_t ns);
        virtual int getData(std::queue<sensors_event_t> &eventQue);
        int batch(int handle, int flags, int64_t period_ns, int64_t timeout);
        virtual bool selftest();
        int hardwareSet(bool activated);
};

#endif
