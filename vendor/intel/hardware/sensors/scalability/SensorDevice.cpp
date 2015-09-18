#include "SensorDevice.hpp"

void SensorDevice::copyDevice(const SensorDevice &device)
{
        char* str;
        int size;

        memset(&dev, 0, sizeof(dev));
        setName(device.dev.name);
        setVendor(device.dev.vendor);
        dev.version = device.dev.version;
        dev.handle = device.dev.handle;
        dev.type = device.dev.type;
        dev.maxRange = device.dev.maxRange;
        dev.resolution = device.dev.resolution;
        dev.power = device.dev.power;
        dev.minDelay = device.dev.minDelay;

        dev.fifoReservedEventCount = device.dev.fifoReservedEventCount;
        dev.fifoMaxEventCount = device.dev.fifoMaxEventCount;
        setStringType(device.dev.stringType);
        setRequiredPermission(device.dev.requiredPermission);
        dev.maxDelay = device.dev.maxDelay;
        dev.flags = device.dev.flags;

        id = device.id;
        category = device.category;
        subname = device.subname;
        eventProperty = device.eventProperty;
        for (int i = 0; i < AXIS_MAX; i++) {
                mapper[i] = device.mapper[i];
                scale[i] = device.scale[i];
        }
}

SensorDevice::SensorDevice(const SensorDevice &device)
{
        copyDevice(device);
}

SensorDevice& SensorDevice::operator=(const SensorDevice &device)
{
        if (this == &device)
                return *this;

        copyDevice(device);

        return *this;
}

void SensorDevice::setName(const char* name) {
        if (name != NULL) {
                if (dev.name != NULL)
                        delete[] dev.name;
                int size = strlen(name);
                char* str = new char[size + 1];
                strcpy(str, name);
                dev.name = str;
        }
}

void SensorDevice::setVendor(const char* vendor)
{
        if (vendor != NULL) {
                if (dev.vendor != NULL)
                        delete[] dev.vendor;
                int size = strlen(vendor);
                char* str = new char[size + 1];
                strcpy(str, vendor);
                dev.vendor = str;
        }
}

void SensorDevice::setStringType(const char* stringType)
{
        if (stringType != NULL) {
                if (dev.stringType != NULL)
                        delete[] dev.stringType;
                int size = strlen(stringType);
                char* str = new char[size + 1];
                strcpy(str, stringType);
                dev.stringType = str;
        }
}

void SensorDevice::setRequiredPermission(const char* requiredPermission)
{
        if (requiredPermission != NULL) {
                if (dev.requiredPermission != NULL)
                        delete[] dev.requiredPermission;
                int size = strlen(requiredPermission);
                char* str = new char[size + 1];
                strcpy(str, requiredPermission);
                dev.requiredPermission = str;
        }
}
