#include <utils/AndroidThreads.h>
#include "PSHSensor.hpp"

class CalibrationSensor : public PSHSensor
{
public:
    CalibrationSensor(SensorDevice &device);
    virtual ~CalibrationSensor();
    virtual int getData(std::queue<sensors_event_t> &eventQue);
    virtual int activate(int handle, int enabled);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int getPollfd();
    virtual bool selftest();

private:
    int           mEnabled;
    psh_sensor_t  mSensorType;
    int           mResultPipe[2];
    int           mWakeupPipe[2];
    pthread_t     mWorkerThread;
    static int    workerThread(void* data);
};
