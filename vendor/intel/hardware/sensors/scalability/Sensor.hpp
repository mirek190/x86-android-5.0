#ifndef _SENSOR_HPP_
#define _SENSOR_HPP_
#include "SensorDevice.hpp"
#include "utils.hpp"
#include <queue>
#include <stdatomic.h>

#define SENSOR_NOPOLL   0x7fffffff
#define NS_TO_MS 1000000
#define US_TO_MS 1000
#define SENSOR_BATCH_MODE_FLUSH_DONE 1

class Sensor {
protected:
        SensorDevice device;
        sensors_event_t event;
        sensors_meta_data_event_t metaEvent;
        int pollfd;
        class HardwareState {
                bool activated;
                bool batchModeEnabled;
                bool flushSuccess;
                int dataRate;
                int delay;
                /*
                 record flush times. increased by flush()
                 and decreased when send a SENSOR_TYPE_META_DATA type event
                */
                atomic_long flush;
        public:
                HardwareState()
                {
                        activated = false;
                        batchModeEnabled = false;
                        flushSuccess = false;
                        dataRate = 5;
                        delay = 200;
                        atomic_init(&flush, 0);
                }
                bool getActivated() { return activated; }
                void setActivated(bool new_activated) {  activated = new_activated; }
                bool getBatchModeEnabled() { return batchModeEnabled; }
                void setBatchModeEnabled(bool new_batchModeEnabled) {  batchModeEnabled = new_batchModeEnabled; }
                bool getFlushSuccess() { return flushSuccess; }
                void setFlushSuccess(bool new_flushSuccess) {  flushSuccess = new_flushSuccess; }
                int getDataRate() { return dataRate; }
                void setDataRate(int new_dataRate) {  dataRate = new_dataRate; }
                int getDelay() { return delay; }
                void setDelay(int new_delay) {  delay = new_delay; }
                long increaseFlush() { return atomic_fetch_add(&flush, 1); } /* return the value before add */
                long decreaseFlush() { return atomic_fetch_sub(&flush, 1); } /* return the value before sub */

        };
        HardwareState state;
public:
        Sensor();
        Sensor(SensorDevice &device);
        virtual ~Sensor() {}
        SensorDevice& getDevice() { return device; }
        virtual void resetEventHandle();
        virtual int getPollfd() = 0;
        virtual int activate(int handle, int enabled) { return 0; }
        virtual int setDelay(int handle, int64_t ns) { return 0; }
        virtual int getData(std::queue<sensors_event_t> &eventQue) = 0;
        virtual int batch(int handle, int flags, int64_t period_ns, int64_t timeout);
        virtual int flush(int handle);
        virtual bool selftest() = 0;
        virtual int hardwareSet(bool activated) { return 0; }
};

#endif
