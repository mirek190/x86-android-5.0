#include "Sensor.hpp"

Sensor::Sensor()
{
        pollfd = -1;
        memset(&event, 0, sizeof(sensors_event_t));
        event.version = sizeof(sensors_event_t);

        memset(&metaEvent, 0, sizeof(metaEvent));
        metaEvent.version = META_DATA_VERSION;
        metaEvent.type = SENSOR_TYPE_META_DATA;
        metaEvent.meta_data.sensor = device.getHandle();
        metaEvent.meta_data.what = META_DATA_FLUSH_COMPLETE;
}

Sensor::Sensor(SensorDevice &mDevice)
{
        pollfd = -1;
        device = mDevice;
        memset(&event, 0, sizeof(sensors_event_t));
        event.version = sizeof(sensors_event_t);
        event.sensor = device.getHandle();
        event.type = device.getType();

        memset(&metaEvent, 0, sizeof(metaEvent));
        metaEvent.version = META_DATA_VERSION;
        metaEvent.type = SENSOR_TYPE_META_DATA;
        metaEvent.meta_data.sensor = device.getHandle();
        metaEvent.meta_data.what = META_DATA_FLUSH_COMPLETE;
}

int Sensor::batch(int handle, int flags, int64_t period_ns, int64_t timeout)
{
        if (handle != device.getHandle()) {
                ALOGE("%s: line: %d: %s handle not match! handle: %d required handle: %d",
                     __FUNCTION__, __LINE__, device.getName(), device.getHandle(), handle);
                return -EINVAL;
        }

        if (period_ns < 0 || timeout < 0)
                return -EINVAL;

        return setDelay(handle, period_ns);
}

int Sensor::flush(int handle)
{
        if (handle != device.getHandle()) {
                ALOGE("%s: line: %d: %s handle not match! handle: %d required handle: %d",
                     __FUNCTION__, __LINE__, device.getName(), device.getHandle(), handle);
                return -EINVAL;
        }


        if (!state.getActivated()) {
                ALOGW("%s line: %d %s not activated", __FUNCTION__, __LINE__, device.getName());
                return -EINVAL;
        }

        if ((device.getFlags() & ~SENSOR_FLAG_WAKE_UP) == SENSOR_FLAG_ONE_SHOT_MODE) {
                ALOGE("%s line: %d error: one-shot sensor: %s", __FUNCTION__, __LINE__, device.getName());
                return -EINVAL;
        }

        if (!state.getBatchModeEnabled()) {
                ALOGW("%s line: %d %s batch not enabled", __FUNCTION__, __LINE__, device.getName());
                state.setFlushSuccess(true);
                if (state.increaseFlush() > 0) {
                        ALOGW("%s line: %d %s flush before SENSOR_TYPE_META_DATA event",
                                __FUNCTION__, __LINE__, device.getName());
                }
                return 0;
        }

        return -EINVAL;
}

void Sensor::resetEventHandle()
{
        event.sensor = device.getHandle();
        metaEvent.meta_data.sensor = device.getHandle();
}
