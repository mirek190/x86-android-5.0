
#include <iostream>
#include <string>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <poll.h>
#include <dirent.h>
#include <errno.h>
#include "sensors.h"
#include "Helpers.h"

// Used by SensorIIO device containers
struct SensorIIOChannel{
    std::string name;
    float scale;
    float offset;
    unsigned index;
    unsigned real_bytes;
    unsigned bytes;
    unsigned shift;
    unsigned mask;
    unsigned is_signed;
    unsigned enabled;
};

/**
 * Input device based sensors must inherit this class.
 * The readEvents loop is already defined by this base class,
 * inheritors need to define only processEvent for doing sensor-specific
 * event computations.
 */
class SensorIIODev{

public:
    bool initialized;
    int device_number;
    std::stringstream dev_device_name;
    std::stringstream scan_el_dir;
    std::stringstream buffer_dir_name;
    std::vector < SensorIIOChannel > info_array;
    int num_channels;
    int buffer_len;
    int enable_buffer;
    int file_id;
    int datum_size;
    std::string unit_expo_str;
    std::string unit_str;
    std::string device_name;
    std::string channel_prefix_str;
    std::string mDevPath;
    bool mHasPendingEvent;
    bool mEnabled;
    int mFd;
    long unit_expo_value;
    long units_value;
    int retry_count;
    unsigned char *raw_buffer;

    int discover();
    int EnableIIODevice();
    int GetDir(const std::string& dir, std::vector < std::string >& files);
    void ListFiles(const std::string& dir);
    int FindDeviceNumberFromName(const std::string& name, const std::string& prefix);

    int BuildChannelList();
    int SetUpTrigger(int dev_num);
    int SetUpBufferLen(int len);
    int GetSizeFromChannels();
    int ParseIIODirectory(const std::string& name);
    int EnableChannels();
    int AllocateRxBuffer();
    int FreeRxBuffer();

    int SetSampleDelay(int rate);
    int GetSampleDelay(int &rate);
    int DeviceSetSensitivity(int value);
    int DeviceGetSensitivity(int &value);

    bool IsDeviceInitialized();
    int GetDeviceNumber();
    int SetDataReadyTrigger(int dev_num, bool status);
    int EnableBuffer(int status);
    int DeviceActivate(int dev_num, int state);
    long GetUnitValue();
    long GetExponentValue();
    int ReadHIDMeasurmentUnit(long *unit);
    int ReadHIDExponentValue(long *exponent);
    int GetChannelBytesUsedSize(unsigned int channel_no);
    virtual int processEvent(unsigned char *raw_data, size_t raw_data_len)
        = 0;
    virtual int readEvents(sensors_event_t *data, int count);
    virtual int enable(int enabled);
    virtual int setDelay(int64_t delay_ns);
    virtual int setInitialState();

public:
    SensorIIODev(const std::string& dev_name, const std::string& units, const std::string& exponent, const std::string& channel_prefix);
    SensorIIODev(const std::string& dev_name, const std::string& units, const std::string& exponent, const std::string& channel_prefix, int retry_cnt);
    virtual ~SensorIIODev() {}

};

class AccelSensor: public SensorIIODev{

public:
    AccelSensor();
    virtual int processEvent(unsigned char *raw_data, size_t raw_data_len);
    static const struct sensor_t sSensorInfo_accel3D;
};

class GyroSensor: public SensorIIODev{

public:
    GyroSensor();
    virtual int processEvent(unsigned char *raw_data, size_t raw_data_len);
    static const struct sensor_t sSensorInfo_gyro3D;
};

class ALSSensor : public SensorIIODev {

public:
    ALSSensor();
    virtual int processEvent(unsigned char *raw_data, size_t raw_data_len);
    static const struct sensor_t sSensorInfo_als;
};

class CompassSensor : public SensorIIODev {

public:
    CompassSensor();
    virtual int processEvent(unsigned char *raw_data, size_t raw_data_len);
    static const struct sensor_t sSensorInfo_compass3D;
};




#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ID_A  (0)
#define ID_M  (1)
#define ID_O  (2)
#define ID_L  (3)
#define ID_P  (4)
#define ID_GY (5)
#define ID_PR (6)
#define ID_T  (7)
#define MAX_SENSOR_CNT (ID_T+1)

#define SENSORS_ACCELERATION     (1<<ID_A)
#define SENSORS_MAGNETIC_FIELD   (1<<ID_M)
#define SENSORS_ORIENTATION      (1<<ID_O)
#define SENSORS_LIGHT            (1<<ID_L)
#define SENSORS_PROXIMITY        (1<<ID_P)
#define SENSORS_GYROSCOPE        (1<<ID_GY)
#define SENSORS_PRESSURE         (1<<ID_PR)
#define SENSORS_TEMPERATURE      (1<<ID_T)

#define SENSORS_ACCELERATION_HANDLE     0
#define SENSORS_MAGNETIC_FIELD_HANDLE   1
#define SENSORS_ORIENTATION_HANDLE      2
#define SENSORS_LIGHT_HANDLE            3
#define SENSORS_PROXIMITY_HANDLE        4
#define SENSORS_GYROSCOPE_HANDLE        5
#define SENSORS_PRESSURE_HANDLE         6
#define SENSORS_TEMPERATURE_HANDLE      7


