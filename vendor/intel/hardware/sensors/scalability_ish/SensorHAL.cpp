#include <hardware/hardware.h>
#include "InputEventSensor.hpp"
#include "MiscSensor.hpp"
#include "PSHCommonSensor.hpp"
#include "PlatformConfig.hpp"
#include "PedometerSensor.hpp"
#include "PhysicalActivitySensor.hpp"
#include "GestureSensor.hpp"
#include "AudioClassifierSensor.hpp"
#include <poll.h>
#include "utils.hpp"
static int open(const struct hw_module_t* module, const char* id,
                struct hw_device_t** device);

static struct hw_module_methods_t sensors_module_methods = {
open: open,
};

struct SensorModule {
        struct sensor_t* list;
        std::vector<Sensor*> sensors;
        struct pollfd *pollfds;
        int count;
};

static struct SensorModule mModule;

static int get_sensors_list(struct sensors_module_t* module, struct sensor_t const** list)
{
        *list = mModule.list;
        return mModule.count;
}

struct sensors_module_t HAL_MODULE_INFO_SYM = {
common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: SENSORS_HARDWARE_MODULE_ID,
        name: "Intel Sensor module",
        author: "Han, He <he.han@intel.com>, Intel Inc.",
        methods: &sensors_module_methods,
        dso: 0,
        reserved: { 0 },
},
get_sensors_list: get_sensors_list,
};

static bool initSensors()
{
		log_message(DEBUG,"initSensors started\n");
        PlatformConfig mConfig;
        SensorDevice mDevice;
        struct PlatformData mData;
        Sensor* mSensor = NULL;
        unsigned int size;

        size = mConfig.size();
        mModule.sensors.reserve(size);
        int newId = 0;
		log_message(DEBUG,"initSensors before for with size =%d\n", size);

        for (unsigned int i = 0; i < size; i++) {
  			    log_message(DEBUG,"iteration %d\n", i);

				if (!mConfig.getSensorDevice(i, mDevice)) {
                        log_message(CRITICAL,"Sensor Device config error\n");
                        return false;
                }
				log_message(DEBUG,"after getSensorDevice %d\n", mDevice.getType());

                if (mDevice.getCategory() == LIBSENSORHUB) {
						log_message(DEBUG,"in SENSORHUB\n");

                        switch (mDevice.getType()) {
                        case SENSOR_TYPE_ACCELEROMETER:
                        case SENSOR_TYPE_MAGNETIC_FIELD:
                        case SENSOR_TYPE_GYROSCOPE:
                        case SENSOR_TYPE_PRESSURE:
                        case SENSOR_TYPE_LIGHT:
                        case SENSOR_TYPE_PROXIMITY:
                        case SENSOR_TYPE_TEMPERATURE:
                        case SENSOR_TYPE_RELATIVE_HUMIDITY:
                        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
                        case SENSOR_TYPE_ORIENTATION:
                        case SENSOR_TYPE_GRAVITY:
                        case SENSOR_TYPE_LINEAR_ACCELERATION:
                        case SENSOR_TYPE_ROTATION_VECTOR:
                        case SENSOR_TYPE_GESTURE_FLICK:
                        case SENSOR_TYPE_TERMINAL:
                        case SENSOR_TYPE_SHAKE:
                        case SENSOR_TYPE_SIMPLE_TAPPING:
                        case SENSOR_TYPE_MOVE_DETECT:
                        case SENSOR_TYPE_STEP_DETECTOR:
                        case SENSOR_TYPE_STEP_COUNTER:
                        case SENSOR_TYPE_SIGNIFICANT_MOTION:
                        case SENSOR_TYPE_GAME_ROTATION_VECTOR:
                        case SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
                                mSensor = new PSHCommonSensor(mDevice);
                                break;
                        case SENSOR_TYPE_PEDOMETER:
                                mSensor = new PedometerSensor(mDevice);
                                break;
                        case SENSOR_TYPE_PHYSICAL_ACTIVITY:
                                mSensor = new PhysicalActivitySensor(mDevice);
                                break;
                        case SENSOR_TYPE_GESTURE:
                                mSensor = new GestureSensor(mDevice);
                                break;
                        case SENSOR_TYPE_AUDIO_CLASSIFICATION:
                                mSensor = new AudioClassifierSensor(mDevice);
                                break;
                        default:
                                log_message(CRITICAL,"%s Unsupported sensor type: %d\n", __FUNCTION__, mDevice.getType());
                                return false;
                        }
                } else {
						log_message(DEBUG,"in NOT SENSORHUB\n");

                        if (!mConfig.getPlatformData(i, mData)) {
                                log_message(CRITICAL,"Get Platform Data config error\n");
                                return false;
                        }
						log_message(DEBUG,"after  getPlatformData %d\n", mDevice.getType());

                        switch (mDevice.getType()) {
                        case SENSOR_TYPE_ACCELEROMETER:
                        case SENSOR_TYPE_MAGNETIC_FIELD:
                        case SENSOR_TYPE_GYROSCOPE:
                        case SENSOR_TYPE_PRESSURE:
                        case SENSOR_TYPE_LIGHT:
                        case SENSOR_TYPE_PROXIMITY:
                        case SENSOR_TYPE_TEMPERATURE:
                        case SENSOR_TYPE_RELATIVE_HUMIDITY:
                        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
                                if (mData.driverNodeType == MISC)
                                        mSensor = new MiscSensor(mDevice, mData);
                                else
                                        mSensor = new InputEventSensor(mDevice, mData);
                                break;
                        default:
                                log_message(CRITICAL,"%s Unsupported sensor type: %d\n", __FUNCTION__, mDevice.getType());
                                return false;
                        }
						log_message(DEBUG,"after second switch\n");


                }
				log_message(DEBUG,"after big if\n");

                if (mSensor) {
                        if (mSensor->selftest()) {
								log_message(DEBUG,"after selftest\n");

                                // Need to reset ids and handles, since some unfunctional sensors are removed
                                mSensor->getDevice().setId(newId);
                                mSensor->getDevice().setHandle(SensorDevice::idToHandle(newId));
                                mSensor->resetEventHandle();
                                mModule.sensors.push_back(mSensor);
                                newId++;
                        } else {
							log_message(DEBUG,"before  delete\n");

                                delete mSensor;
                        }
                        mSensor = NULL;
                }
        }
		log_message(DEBUG,"after for\n");

        mModule.count = mModule.sensors.size();
        mModule.list = new sensor_t[mModule.count];

        for (int i = 0; i < mModule.count; i++) {
                mModule.sensors[i]->getDevice().copyItem(mModule.list + i);
        }

        mModule.pollfds = new struct pollfd[mModule.count];
        for (int i = 0; i < mModule.count; i++) {
                mModule.pollfds[i].fd = mModule.sensors[i]->getPollfd();
                mModule.pollfds[i].events = POLLIN;
                mModule.pollfds[i].revents = 0;
        }
		log_message(DEBUG,"initSensors finished well\n");

        return true;
}

int sensorActivate(struct sensors_poll_device_t *dev, int handle, int enabled)
{
        int id = SensorDevice::handleToId(handle);
        if (id < 0) {
                log_message(CRITICAL,"%s: line:%d Invalid handle: handle: %d; id: %d",
                     __FUNCTION__, __LINE__, handle, id);
                return -1;
        }

        return mModule.sensors[id]->activate(handle, enabled);
}

int sensorSetDelay(struct sensors_poll_device_t *dev, int handle, int64_t ns)
{
        int id = SensorDevice::handleToId(handle);
        if (id < 0) {
                log_message(CRITICAL,"%s: line:%d Invalid handle: handle: %d; id: %d",
                     __FUNCTION__, __LINE__, handle, id);
                return -1;
        }
        return mModule.sensors[id]->setDelay(handle, ns);

}

int sensorPoll(struct sensors_poll_device_t *dev, sensors_event_t* data, int count)
{
        static std::queue<sensors_event_t> eventQue;
        int eventNum = 0;
        int num, err;

        while (true) {
                while (eventQue.size() > 0 && eventNum <= count) {
                        data[eventNum] = eventQue.front();
                        eventQue.pop();
                        eventNum++;
                }

                if (eventNum > 0)
                        return eventNum;

                num = poll(mModule.pollfds, mModule.count, -1);
                if (num <= 0) {
                        err = errno;
                        log_message(CRITICAL,"%s: line: %d poll error: %d %s", __FUNCTION__, __LINE__, err, strerror(err));
                        return -err;
                }
                for (int i = 0; i < mModule.count; i++) {
                        if (mModule.pollfds[i].revents & POLLIN)
                                mModule.sensors[i]->getData(eventQue);
                        else if (mModule.pollfds[i].revents != 0)
                                log_message(CRITICAL,"%s: line: %d poll error: %d fd: %d type: %d", __FUNCTION__, __LINE__, mModule.pollfds[i].revents, mModule.pollfds[i].fd, mModule.sensors[i]->getDevice().getType());
                        mModule.pollfds[i].revents = 0;
                }
        }

        return -1;
}

int close(struct hw_device_t* device)
{
        if (mModule.list != NULL)
                delete mModule.list;
        for (unsigned int i = 0; i < mModule.sensors.size(); i++) {
                if (mModule.sensors[i])
                        delete mModule.sensors[i];
        }
        if (mModule.pollfds)
                delete [] mModule.pollfds;

        return 0;
}


static int open(const struct hw_module_t* module, const char* id,
                struct hw_device_t** device)
{
        static struct sensors_poll_device_t dev;

		log_message(DEBUG,"open started for id = %s\n", id);

        dev.common.tag = HARDWARE_DEVICE_TAG;
        dev.common.version = 0;
        dev.common.module  = const_cast<hw_module_t*>(module);
        dev.common.close   = close;
        dev.activate       = sensorActivate;
        dev.setDelay       = sensorSetDelay;
        dev.poll           = sensorPoll;

        *device = &dev.common;

        if(initSensors())
                return 0;

        return -1;
}

