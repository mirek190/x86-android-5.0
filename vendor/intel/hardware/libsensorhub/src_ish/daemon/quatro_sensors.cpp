
#include <dirent.h>
#include <fcntl.h>
#include "../include/iio-hal/quatro_sensors.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "../include/iio-hal/Helpers.h"
#include "../include/iio-hal/sensors.h"
#include "../include/iio-hal/SensorConfig.h"

#include "../include/utils.h"

int PathOps::write(const std::string &path, const std::string &buf)
{
    std::string p = mBasePath + path;
    int fd = ::open(p.c_str(), O_WRONLY);
    if (fd < 0)
        return -errno;

    int ret = ::write(fd, buf.c_str(), buf.size());
    close(fd);

    return ret;
}

int PathOps::write(const std::string &path, unsigned int data)
{
    std::ostringstream os;
    os << data;
    return PathOps::write(path, os.str());
}

int PathOps::read(const std::string &path, char *buf, int len)
{
    std::string p = mBasePath + path;
    int fd = ::open(p.c_str(), O_RDONLY);
    if (fd < 0)
        return -errno;

    int ret = ::read(fd, buf, len);
    close(fd);

    return ret;
}

int PathOps::read(const std::string &path, std::string &buf)
{
    std::string p = mBasePath + path;
    /* Using fstream for reading stuff into std::string */
    std::ifstream f(p.c_str(), std::fstream::in);
    if (f.fail())
        return -EINVAL;

    int ret = 0;
    f >> buf;
    if (f.bad())
        ret = -EIO;
    f.close();

    return ret;
}

bool PathOps::exists(const std::string &path)
{
    struct stat s;
    return (bool) (stat((mBasePath + path).c_str(), &s) == 0);
}

bool PathOps::exists()
{
    return PathOps::exists("");
}

static const std::string IIO_DIR = "/sys/bus/iio/devices";
static const int DEF_BUFFER_LEN = 2;
static const int DEF_HYST_VALUE = 0;

SensorIIODev::SensorIIODev(const std::string& dev_name, const std::string& units,
                           const std::string& exponent,
                           const std::string& channel_prefix, int retry_cnt)
             : initialized(false),
               unit_expo_str(exponent),
               unit_str(units),
               device_name(dev_name),
               channel_prefix_str(channel_prefix),
               unit_expo_value(0),
               units_value(0),
               retry_count(retry_cnt),
               raw_buffer(NULL)
{

    log_message(DEBUG, "%s", __func__);
}

SensorIIODev::SensorIIODev(const std::string& dev_name, const std::string& units,
                           const std::string& exponent,
                           const std::string& channel_prefix)
             : initialized(false),
               unit_expo_str(exponent),
               unit_str(units),
               device_name(dev_name),
               channel_prefix_str(channel_prefix),
               unit_expo_value(0),
               units_value(0),
               retry_count(1),
               raw_buffer(NULL)
{

    log_message(DEBUG, "%s", __func__);
}

int SensorIIODev::discover()
{
    int cnt;
    int status;
    int ret;

    log_message(DEBUG, ">>%s discover\n", __func__);
    for (cnt = 0; cnt < retry_count; cnt++) {
        status = ParseIIODirectory(device_name);
		log_message(DEBUG, "after ParseIIODirectory\n");
        if (status >= 0){
            std::stringstream filename;
            initialized = true;
            filename << "/dev/iio:device" << device_number;
            mDevPath = filename.str();
            log_message(DEBUG, "mDevPath %s\n", mDevPath.c_str());
            ret = 0;
            break;
        }
        else{
            log_message(DEBUG, "Sensor IIO Init failed, retry left:%d\n", retry_count-cnt);
            mDevPath = "";
            ret = -1;
        }
        // Device not enumerated yet, wait and retry
        sleep(1);
    }
    return ret;
}

int SensorIIODev::AllocateRxBuffer()
{
    log_message(DEBUG, ">>%s Allocate:%d", __func__, datum_size * buffer_len);
    raw_buffer = new unsigned char[datum_size * buffer_len];
    if (!raw_buffer) {
        log_message(DEBUG, "AllocateRxBuffer: Failed\n");
        return -ENOMEM;
    }
    return 0;
}

int SensorIIODev::FreeRxBuffer()
{
    log_message(DEBUG, ">>%s", __func__);
    delete []raw_buffer;
    raw_buffer = NULL;
    return 0;
}

int SensorIIODev::enable(int enabled)
{
    int ret =0;

    log_message(DEBUG, ">>%s enabled:%d", __func__, enabled);

    if (mEnabled == enabled) {
        return 0;
    }
    if (enabled){
        // QUIRK: some sensor hubs need to be turned on and off and on
        // before sending any information. So we turn it on and off first
        // before enabling again later in this function.
        //EnableBuffer(1);
        //EnableBuffer(0);

        if (ReadHIDExponentValue(&unit_expo_value) < 0)
            goto err_ret;
		log_message(DEBUG, "After ReadHIDExponentValue\n");
        if (ReadHIDMeasurmentUnit(&units_value) < 0)
            goto err_ret;
		log_message(DEBUG, "After ReadHIDMeasurmentUnit\n");
        if (SetDataReadyTrigger(GetDeviceNumber(), true) < 0)
            goto err_ret;
		log_message(DEBUG, "After SetDataReadyTrigger\n");
        if (EnableBuffer(1) < 0)
            goto err_ret;
		log_message(DEBUG, "After EnableBuffer\n");
        if (DeviceActivate(GetDeviceNumber(), 1) < 0)
            goto err_ret;
		log_message(DEBUG, "After DeviceActivate\n");
        if (DeviceSetSensitivity( DEF_HYST_VALUE) < 0)
            goto err_ret;
		log_message(DEBUG, "After DeviceSetSensitivity\n");
        if (AllocateRxBuffer() < 0)
            goto err_ret;
        mEnabled = enabled;
    }
    else{
        mEnabled = enabled;
        if (SetDataReadyTrigger(GetDeviceNumber(), false) < 0)
            goto err_ret;
        if (EnableBuffer(0) < 0)
            goto err_ret;
        if (DeviceActivate(GetDeviceNumber(), 0) < 0)
            goto err_ret;
        if (DeviceSetSensitivity(0))
            goto err_ret;
        if (FreeRxBuffer() < 0)
            goto err_ret;
        mDevPath = "";
    }
    return 0;

err_ret:
    log_message(DEBUG, "SesnorIIO: Enable failed\n");
    return -1;
}

int SensorIIODev::setDelay(int64_t delay_ns){
    int ms = nsToMs(delay_ns);
    int r;

    log_message(DEBUG, ">>%s %ld", __func__, delay_ns);
    if (IsDeviceInitialized() == false){
        log_message(DEBUG, "Device was not initialized \n");
        return  -EFAULT;
    }
    if (ms){
        if ((r = SetSampleDelay(ms)) < 0)
            return r;
    }
    log_message(DEBUG, "<<%s", __func__);
    return 0;
}

int SensorIIODev::setInitialState(){
    mHasPendingEvent = false;
    return 0;
}

long SensorIIODev::GetUnitValue()
{
    return units_value;
}

long SensorIIODev::GetExponentValue()
{
    return unit_expo_value;
}

bool SensorIIODev::IsDeviceInitialized(){
    return initialized;
}

int SensorIIODev::GetDeviceNumber(){
    return device_number;
}

int SensorIIODev::GetDir(const std::string& dir, std::vector < std::string >& files){
    DIR *dp;
    struct dirent *dirp;

    if ((dp = opendir(dir.c_str())) == NULL){
        log_message(DEBUG, "Error opening directory %s\n", (char*)dir.c_str());
        return errno;
    }
    while ((dirp = readdir(dp)) != NULL){
        files.push_back(std::string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

void SensorIIODev::ListFiles(const std::string& dir){
    std::vector < std::string > files = std::vector < std::string > ();

    GetDir(dir, files);

    for (unsigned int i = 0; i < files.size(); i++){
        log_message(DEBUG, "File List.. %s\n", (char*)files[i].c_str());
    }
}

// This function returns a device number for IIO device
// For example if the device is "iio:device1", this will return 1
int SensorIIODev::FindDeviceNumberFromName(const std::string& name, const std::string&
    prefix){
    int dev_number;
    std::vector < std::string > files = std::vector < std::string > ();
    std::string device_name;

    GetDir(std::string(IIO_DIR), files);

    for (unsigned int i = 0; i < files.size(); i++){
        std::stringstream ss;
        if ((files[i] == ".") || (files[i] == "..") || files[i].length() <=
            prefix.length())
            continue;

        if (files[i].compare(0, prefix.length(), prefix) == 0){
            sscanf(files[i].c_str() + prefix.length(), "%d", &dev_number);
        }
        ss << IIO_DIR << "/" << prefix << dev_number;
        std::ifstream ifs(ss.str().c_str(), std::ifstream::in);
        if (!ifs.good()){
            dev_number =  -1;
            ifs.close();
            continue;
        }
        ifs.close();

        std::ifstream ifn((ss.str() + "/" + "name").c_str(), std::ifstream::in);

        if (!ifn.good()){
            dev_number =  -1;
            ifn.close();
            continue;
        }
        std::getline(ifn, device_name);
        if (name.compare(device_name) == 0){
            log_message(DEBUG, "matched %s\n", device_name.c_str());
            ifn.close();
            return dev_number;
        }
        ifn.close();

    }
    return  -EFAULT;
}

// Setup Trigger
int SensorIIODev::SetUpTrigger(int dev_num){
    std::stringstream trigger_name;
    int trigger_num;

    trigger_name << device_name << "-dev" << dev_num;
    if (trigger_name.str().length()){
        log_message(DEBUG, "trigger_name %s\n", (char*)trigger_name.str().c_str());
        trigger_num = FindDeviceNumberFromName(trigger_name.str(), "trigger");
        if (trigger_num < 0){
            log_message(DEBUG, "Failed to find trigger\n");
            return  -EFAULT;
        }
    }
    else{
        log_message(DEBUG, "trigger_name not found \n");
        return  -EFAULT;
    }
    return 0;
}

// Setup buffer length in terms of datum size
int SensorIIODev::SetUpBufferLen(int len){
    std::stringstream filename;
    std::stringstream len_str;

    log_message(DEBUG, "%s: len:%d\n", __func__, len);

    filename << buffer_dir_name.str() << "/" << "length";

    PathOps path_ops;
    len_str << len;
    int ret = path_ops.write(filename.str(), len_str.str());
    if (ret < 0) {
        log_message(DEBUG, "Write Error %s\n", filename.str().c_str());
        return ret;
    }
    buffer_len = len;
    return 0;
}

// Enable buffer
int SensorIIODev::EnableBuffer(int status){
    std::stringstream filename;
    std::stringstream status_str;

    log_message(DEBUG, "%s: len:%d\n", __func__, status);

    filename << buffer_dir_name.str() << "/" << "enable";
    PathOps path_ops;
    status_str << status;
    int ret = path_ops.write(filename.str(), status_str.str());
    if (ret < 0) {
        log_message(DEBUG, "Write Error %s\n", filename.str().c_str());
        return ret;
    }
    enable_buffer = status;
    return 0;
}

int SensorIIODev::EnableChannels(){
    int ret = 0;
    int count;
    int counter = 0;
    std::string dir = std::string(IIO_DIR);
    std::vector < std::string > files = std::vector < std::string > ();
    const std::string FORMAT_SCAN_ELEMENTS_DIR = "/scan_elements";
    unsigned char signchar, bits_used, total_bits, shift, unused;
    SensorIIOChannel iio_channel;

    log_message(DEBUG, ">>%s\n", __func__);
    scan_el_dir.str(std::string());
    scan_el_dir << dev_device_name.str() << FORMAT_SCAN_ELEMENTS_DIR;
    GetDir(scan_el_dir.str(), files);

    for (unsigned int i = 0; i < files.size(); i++){
        int len = files[i].length();
        if (len > 3 && files[i].compare(len - 3, 3, "_en") == 0){
            std::stringstream filename, file_type;
            PathOps path_ops;

            filename << scan_el_dir.str() << "/" << files[i];
            ret = path_ops.write(filename.str(), "1");
            if (ret < 0)
              log_message(DEBUG, "\nChannel Enable Error %s:%d\n", filename.str().c_str(), ret);
        }
    }
    return ret;
}

int SensorIIODev::BuildChannelList(){
    int ret;
    int count;
    int counter = 0;
    std::string dir = std::string(IIO_DIR);
    std::vector < std::string > files = std::vector < std::string > ();
    const std::string FORMAT_SCAN_ELEMENTS_DIR = "/scan_elements";
    unsigned char signchar, bits_used, total_bits, shift, unused;
    SensorIIOChannel iio_channel;
    std::string type_name;

    log_message(DEBUG, ">>%s", __func__);
    scan_el_dir.str(std::string());
    scan_el_dir << dev_device_name.str() << FORMAT_SCAN_ELEMENTS_DIR;
    GetDir(scan_el_dir.str(), files);

    // File ending with _en will specify a channel
    // it contains the count. If we add all these count
    // those many channels present
    unsigned int filesNum = files.size();
    for (unsigned int i = 0; i < filesNum; i++){
      log_message(DEBUG, "iterating file %d out of %d: %s\n", i, filesNum, files[i].c_str());
      
        int len = files[i].length();
	log_message(DEBUG, "the length is %d\n", len);
        // At the least the length should be more than 3 for "_en"
        if (len > 3 && files[i].compare(len - 3, 3, "_en") == 0){
	    log_message(DEBUG, "after compare\n");
            std::stringstream filename, file_type;
            filename << scan_el_dir.str() << "/" << files[i];
            std::ifstream ifs(filename.str().c_str(), std::ifstream::in);
	    log_message(DEBUG, "after ifs %s\n", filename.str().c_str());

            if (!ifs.good()){
                ifs.close();
                continue;
            }
			log_message(DEBUG, "before get\n");
            count = ifs.get() - '0';
            if (count == 1)
                counter++;
            ifs.close();
			log_message(DEBUG, "after close\n");

            iio_channel.enabled = 1;
            iio_channel.scale = 1.0;
            iio_channel.offset = 0;

            iio_channel.name = files[i].substr(0, files[i].length() - 3);

            log_message(DEBUG, "IIO channel name:%s\n", (char*)iio_channel.name.c_str());
            file_type << scan_el_dir.str() << "/" << iio_channel.name <<
                "_type";

			total_bits = 32;
            bits_used = 16;
			signchar = 's';
            iio_channel.bytes = total_bits / 8;
            iio_channel.real_bytes = bits_used / 8;
            iio_channel.shift = 0;
            iio_channel.is_signed = signchar;

            // Add to list
            info_array.push_back(iio_channel);
			log_message(DEBUG, "check6\n");

        }
	log_message(DEBUG, "compare said NO\n");
    }
    log_message(DEBUG, "ooof blin\n");
    //log_message(DEBUG, "<<%s", __func__);
    return counter;
}

int SensorIIODev::GetSizeFromChannels(){
    int size = 0;
    for (unsigned int i = 0; i < info_array.size(); i++){
        SensorIIOChannel channel = info_array[i];
        size += channel.bytes;
    }
    log_message(DEBUG, "%s:%d:%d\n", __func__, info_array.size(), size);
    return size;
}

int SensorIIODev::GetChannelBytesUsedSize(unsigned int channel_no){
    if (channel_no < info_array.size()){
        SensorIIOChannel channel = info_array[channel_no];
        return channel.real_bytes;
    }
    else
        return 0;
}

int SensorIIODev::ParseIIODirectory(const std::string& name){
    int dev_num;
    int ret;
    int size;

    log_message(DEBUG, ">>%s\n", __func__);

    dev_device_name.str(std::string());
    buffer_dir_name.str(std::string());

    device_number = dev_num = FindDeviceNumberFromName(name, "iio:device");
    if (dev_num < 0){
        log_message(DEBUG, "Failed to  find device %s\n", (char*)name.c_str());
        return  -EFAULT;
    }

    // Construct device directory Eg. /sys/devices/iio:device0
    dev_device_name << IIO_DIR << "/" << "iio:device" << dev_num;

    // Construct device directory Eg. /sys/devices/iio:device0:buffer0
    buffer_dir_name << dev_device_name.str() << "/" << "buffer";

    // Setup Trigger
    ret = SetUpTrigger(dev_num);
    if (ret < 0){
        log_message(DEBUG, "ParseIIODirectory Failed due to Trigger\n");
        return  -EFAULT;
    }

    // Setup buffer len. This is in the datum units, not an absolute number
    SetUpBufferLen(DEF_BUFFER_LEN);

    // Set up channel masks
    ret = EnableChannels();
    if (ret < 0){
        log_message(DEBUG, "ParseIIODirectory Failed due Enable Channels failed\n");
		if (ret == -EACCES) {
            // EACCES means the nodes do not have current owner.
            // Need to retry, or else sensors won't power on.
            // Other errors can be ignored, as these nodes are
            // set once, and will return error when set again.
            return ret;
        }
    }

    // Parse the channels and build a list
    ret = BuildChannelList();
    log_message(DEBUG, "moj motek\n");
    if (ret < 0){
        log_message(DEBUG, "ParseIIODirectory Failed due BuildChannelList\n");
        return  -EFAULT;
    }

    datum_size = GetSizeFromChannels();
    log_message(DEBUG, "Datum Size %d\n", datum_size);
    log_message(DEBUG, "<<%s\n", __func__);
    return 0;
}

// Set Data Ready
int SensorIIODev::SetDataReadyTrigger(int dev_num, bool status){
    std::stringstream filename;
    std::stringstream trigger_name;
    int trigger_num;

    log_message(DEBUG, "%s: status:%d\n", __func__, status);

    filename << dev_device_name.str() << "/trigger/current_trigger";
    trigger_name << device_name << "-dev" << dev_num;

    PathOps path_ops;
    int ret;
    if (status)
        ret = path_ops.write(filename.str(), trigger_name.str());
    else
        ret = path_ops.write(filename.str(), " ");
    if (ret < 0) {
        log_message(DEBUG, "Write Error %s:%d", filename.str().c_str(), ret);
        // Ignore error, as this may be due to
        // Trigger was active during resume
    }
	log_message(DEBUG, "%s: done\n", __func__);

    return 0;
}

// Activate device
int SensorIIODev::DeviceActivate(int dev_num, int state){
    std::stringstream filename;
    std::stringstream activate_str;

    log_message(DEBUG, "%s: Device Activate\n", __func__);
    return 0;
}

// Set sensitivity in absolute terms
int SensorIIODev::DeviceSetSensitivity(int value){
    std::stringstream filename;
    std::stringstream sensitivity_str;

    log_message(DEBUG, "%s: Sensitivity :%d\n", __func__, value);

    filename << IIO_DIR << "/" << "iio:device" << device_number << "/" << channel_prefix_str << "hysteresis";

    PathOps path_ops;
    sensitivity_str << value;
    int ret = path_ops.write(filename.str(), sensitivity_str.str());
    if (ret < 0) {
        log_message(DEBUG, "Write Error %s\n", filename.str().c_str());
        // Don't bail out as this  field may be absent
    }
   log_message(DEBUG, "%s done\n", __func__);

    return 0;
}

int SensorIIODev::DeviceGetSensitivity(int &value)
{
	std::stringstream filename;

    log_message(DEBUG, "%s\n", __func__);

    filename << IIO_DIR << "/" << "iio:device" << device_number << "/" << channel_prefix_str << "hysteresis";
    PathOps path_ops;
	std::string valueS;
    int ret = path_ops.read(filename.str(),valueS);
    if (ret < 0) {
        log_message(DEBUG, "Read Error %s\n", filename.str().c_str());
        // Don't bail out as this  field may be absent
    }
	value = atoi(valueS.c_str());

    return 0;
}


// Set the sample period delay: units ms
int SensorIIODev::SetSampleDelay(int period){
    std::stringstream filename;
    std::stringstream sample_rate_str;

    log_message(DEBUG, "%s: sample_rate:%d\n", __func__, period);

    filename << IIO_DIR << "/" << "iio:device" << device_number << "/" << channel_prefix_str << "sampling_frequency";
    if (period <= 0) {
        log_message(DEBUG, "%s: Invalid_rate:%d\n", __func__, period);
    }
    PathOps path_ops;
    sample_rate_str << 1000/period;
    int ret = path_ops.write(filename.str(), sample_rate_str.str());
    if (ret < 0) {
        log_message(DEBUG, "Write Error %s\n", filename.str().c_str());
        // Don't bail out as this  field may be absent
    }
    return 0;
}

 int SensorIIODev::GetSampleDelay(int &rate)
 {
	std::stringstream filename;

    log_message(DEBUG, "%s\n", __func__);

    filename << IIO_DIR << "/" << "iio:device" << device_number << "/" << channel_prefix_str << "sampling_frequency";
    PathOps path_ops;
	std::string value;
    int ret = path_ops.read(filename.str(),value);
    if (ret < 0) {
        log_message(DEBUG, "Read Error %s\n", filename.str().c_str());
        // Don't bail out as this  field may be absent
    }
	rate = 1000/atoi(value.c_str());

    return 0;
 }

int SensorIIODev::readEvents(sensors_event_t *data, int count){
    ssize_t read_size;
    int numEventReceived;

    if (count < 1)
        return  - EINVAL;

    if (mFd < 0)
        return  - EBADF;

    if (!raw_buffer)
        return - EAGAIN;

	log_message(DEBUG, "readEvents: after bad things\n");

    if (mHasPendingEvent){
        mHasPendingEvent = false;
        //mPendingEvent.timestamp = getTimestamp();
        //*data = mPendingEvent;
        return 1;
    }
    read_size = read(mFd, raw_buffer, datum_size * buffer_len);
    numEventReceived = 0;
	if ((raw_buffer[1] == 255) && (raw_buffer[2] == 255))
        numEventReceived++;
	else
	{
	    log_message(DEBUG, "Before processEvent\n");
		if (processEvent(raw_buffer, datum_size) >= 0){
		  //mPendingEvent.timestamp = getTimestamp();
		  // mPendingEvent.timestamp = getTimestamp();
		  //*data = mPendingEvent;
			numEventReceived++;
			log_message(DEBUG, "processEvent suceeded\n");

		}
	}
    return numEventReceived;
}

int SensorIIODev::ReadHIDMeasurmentUnit(long *unit){
    std::stringstream filename;
    int size;
    std::string long_str;

    filename << IIO_DIR << "/" << "iio:device" << device_number << "/" << unit_str;

    std::ifstream its(filename.str().c_str(), std::ifstream::in);
    if (!its.good()){
        log_message(DEBUG, "%s: Can't Open :%s\n",
                __func__, filename.str().c_str());
        its.close();
        return -EINVAL;
    }
    std::getline(its, long_str);
    its.close();

    if (long_str.length() > 0){
        *unit = atol(long_str.c_str());
        return 0;
    }
    log_message(DEBUG, "ReadHIDMeasurmentUnit failed\n");
    return  -EINVAL;
}

int SensorIIODev::ReadHIDExponentValue(long *exponent){
    std::stringstream filename;
    int size;
    std::string long_str;

    filename << IIO_DIR << "/" << "iio:device" << device_number << "/" << unit_expo_str;

    std::ifstream its(filename.str().c_str(), std::ifstream::in);
    if (!its.good()){
        log_message(DEBUG, "%s: Can't Open :%s\n",
               __func__, filename.str().c_str());
        its.close();
        return -EINVAL;
    }
    std::getline(its, long_str);
    its.close();

    if (long_str.length() > 0){
        *exponent = atol(long_str.c_str());
        return 0;
    }
    log_message(DEBUG, "ReadHIDExponentValue failed\n");
    return  -EINVAL;
}


#define CHANNEL_X 0
#define CHANNEL_Y 1
#define CHANNEL_Z 2

struct accel_3d_sample{
    unsigned int accel_x;
    unsigned int accel_y;
    unsigned int accel_z;
};;

// const struct sensor_t AccelSensor::sSensorInfo_accel3D = {
//     "HID_SENSOR Accelerometer 3D", "Intel", 1, SENSORS_ACCELERATION_HANDLE,
//         SENSOR_TYPE_ACCELEROMETER, RANGE_A, RESOLUTION_A, 0.23f, 16666, {}
// }
//  ;

const long HID_USAGE_SENSOR_UNITS_G = 0x1A;
const long HID_USAGE_SENSOR_UNITS_METERS_PER_SEC_SQRD = (0x11, 0xE0);
const int retry_cnt = 30;

AccelSensor::AccelSensor(): SensorIIODev("accel_3d", "in_accel_scale", "in_accel_offset", "in_accel_", retry_cnt){
    log_message(DEBUG, ">>AccelSensor 3D: constructor!\n");
    // mPendingEvent.version = sizeof(sensors_event_t);
    //mPendingEvent.sensor = ID_A;
    //mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    //memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

    log_message(DEBUG, "<<AccelSensor 3D: constructor!");
}

int AccelSensor::processEvent(unsigned char *raw_data, size_t raw_data_len){
    struct accel_3d_sample *sample;

    log_message(DEBUG, ">>%s", __func__);

    if (IsDeviceInitialized() == false){
        log_message(DEBUG, "Device was not initialized \n");
        return  - 1;
    } if (raw_data_len < sizeof(struct accel_3d_sample)){
        log_message(DEBUG, "Insufficient length \n");
        return  - 1;
    }

    log_message(DEBUG, "Accel:%2x:%2x:%2x:%2x:%2x:%2x",  *raw_data, *(raw_data + 1), *
        (raw_data + 2), *(raw_data + 3), *(raw_data + 4), *(raw_data + 5));
    sample = (struct accel_3d_sample*)raw_data;
    // mPendingEvent.data[0] = mPendingEvent.acceleration.x =
    //     CONVERT_A_G_VTF16E14_X(GetChannelBytesUsedSize(CHANNEL_X), GetExponentValue(), sample->accel_x);
    // mPendingEvent.data[1] = mPendingEvent.acceleration.y =
    //     CONVERT_A_G_VTF16E14_Y(GetChannelBytesUsedSize(CHANNEL_Y), GetExponentValue(), sample->accel_y);
    // mPendingEvent.data[2] = mPendingEvent.acceleration.z =
    //     CONVERT_A_G_VTF16E14_Z(GetChannelBytesUsedSize(CHANNEL_Z), GetExponentValue(), sample->accel_z);

    // log_message(DEBUG, "ACCEL 3D Sample %fm/s2 %fm/s2 %fm/s2\n", mPendingEvent.acceleration.x,
    //     mPendingEvent.acceleration.y, mPendingEvent.acceleration.z);
    log_message(DEBUG, "<<%s\n", __func__);
    return 0;
}

struct gyro_3d_sample{
    unsigned int gyro_x;
    unsigned int gyro_y;
    unsigned int gyro_z;
};

//const struct sensor_t GyroSensor::sSensorInfo_gyro3D = {
//    "HID_SENSOR Gyro 3D", "Intel", 1, SENSORS_GYROSCOPE_HANDLE,
//        SENSOR_TYPE_GYROSCOPE, RANGE_GYRO, RESOLUTION_GYRO, 6.10f, 16666, {}
//};
const int HID_USAGE_SENSOR_UNITS_DEGREES_PER_SECOND = 0x15;
const int HID_USAGE_SENSOR_UNITS_RADIANS_PER_SECOND = 0xF012;
//const int retry_cnt = 10;

GyroSensor::GyroSensor(): SensorIIODev("gyro_3d", "in_anglvel_scale", "in_anglvel_offset", "in_anglvel_", retry_cnt){
    log_message(DEBUG, "GyroSensor: constructor\n");
 //   mPendingEvent.version = sizeof(sensors_event_t);
 //   mPendingEvent.sensor = ID_GY;
 //   mPendingEvent.type = SENSOR_TYPE_GYROSCOPE;
 //   memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

}

int GyroSensor::processEvent(unsigned char *raw_data, size_t raw_data_len){
    struct gyro_3d_sample *sample;
	float x,y,z;

    log_message(DEBUG, ">>%s\n", __func__);
    if (IsDeviceInitialized() == false){
        log_message(DEBUG, "Device was not initialized \n");
        return  - 1;
    } if (raw_data_len < sizeof(struct gyro_3d_sample)){
        log_message(DEBUG, "Insufficient length \n");
        return  - 1;
    }
    sample = (struct gyro_3d_sample*)raw_data;
    if (GetUnitValue() == HID_USAGE_SENSOR_UNITS_DEGREES_PER_SECOND){
       x = CONVERT_G_D_VTF16E14_X
            (GetChannelBytesUsedSize(CHANNEL_X), GetExponentValue(), sample->gyro_x)
            ;
        y = CONVERT_G_D_VTF16E14_Y
            (GetChannelBytesUsedSize(CHANNEL_Y), GetExponentValue(), sample->gyro_y)
            ;
        z = CONVERT_G_D_VTF16E14_Z
            (GetChannelBytesUsedSize(CHANNEL_Z), GetExponentValue(), sample->gyro_z)
            ;
    }
    else if (GetUnitValue() == HID_USAGE_SENSOR_UNITS_RADIANS_PER_SECOND){
        log_message(DEBUG, "radians\n");
		x = CONVERT_FROM_VTF16
            (GetChannelBytesUsedSize(CHANNEL_X), GetExponentValue(), sample->gyro_x)
            ;
        y = CONVERT_FROM_VTF16
            (GetChannelBytesUsedSize(CHANNEL_Y), GetExponentValue(), sample->gyro_y)
            ;
        z = CONVERT_FROM_VTF16
            (GetChannelBytesUsedSize(CHANNEL_Z), GetExponentValue(), sample->gyro_z)
            ;
    }
    else{
        log_message(DEBUG, "Gyro Unit is not supported\n");
    }

    log_message(DEBUG, "GYRO 3D Sample %fm/s2 %fm/s2 %fm/s2\n", x, y, z);
    log_message(DEBUG, "<<%s\n", __func__);
    return 0;
}


struct als_sample{
    unsigned short illum;
};


ALSSensor::ALSSensor(): SensorIIODev("als", "in_intensity_scale", "in_intensity_offset", "in_intensity_", retry_cnt){
//    log_message(DEBUG, ">>ALSSensor 3D: constructor!");
 //   mPendingEvent.version = sizeof(sensors_event_t);
 //   mPendingEvent.sensor = ID_L;
 //   mPendingEvent.type = SENSOR_TYPE_LIGHT;
  //  memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
   // log_message(DEBUG, "<<ALSSensor 3D: constructor!");
}

int ALSSensor::processEvent(unsigned char *raw_data, size_t raw_data_len){
    struct als_sample *sample;

    log_message(DEBUG, ">>%s\n", __func__);
    if (IsDeviceInitialized() == false){
        log_message(DEBUG, "Device was not initialized \n");
        return  - 1;
    } if (raw_data_len < sizeof(struct als_sample)){
        log_message(DEBUG, "Insufficient length \n");
        return  - 1;
    }
    sample = (struct als_sample*)raw_data;
    float light = (float)sample->illum;

    log_message(DEBUG, "ALS %fm/s2\n", light);
    log_message(DEBUG, "<<%s\n", __func__);
    return 0;
}


struct compass_3d_sample{
    unsigned int compass_x;
    unsigned int compass_y;
    unsigned int compass_z;
};



CompassSensor::CompassSensor(): SensorIIODev("magn_3d", "in_magn_scale", "in_magn_offset", "in_magn_", retry_cnt){
 /*   log_message(DEBUG, ">>ComassSensor 3D: constructor!");
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_M;
    mPendingEvent.type = SENSOR_TYPE_MAGNETIC_FIELD;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
    log_message(DEBUG, "<<ComassSensor 3D: constructor!");*/
}

int CompassSensor::processEvent(unsigned char *raw_data, size_t raw_data_len){
    struct compass_3d_sample *sample;

    log_message(DEBUG, ">>%s\n", __func__);
    if (IsDeviceInitialized() == false){
        log_message(DEBUG, "Device was not initialized \n");
        return  - 1;
    } if (raw_data_len < sizeof(struct compass_3d_sample)){
        log_message(DEBUG, "Insufficient length \n");
        return  - 1;
    }
    sample = (struct compass_3d_sample*)raw_data;
	float x,y,z;
    x = CONVERT_M_MG_VTF16E14_X
        (GetChannelBytesUsedSize(CHANNEL_X), GetExponentValue(), sample->compass_x);
    y = CONVERT_M_MG_VTF16E14_Y
        (GetChannelBytesUsedSize(CHANNEL_Y), GetExponentValue(), sample->compass_y);
    z = CONVERT_M_MG_VTF16E14_Z
        (GetChannelBytesUsedSize(CHANNEL_Z), GetExponentValue(), sample->compass_z);

    log_message(DEBUG, "COMPASS 3D Sample %fuT %fuT %fuT\n", x, y, z);
    log_message(DEBUG, "<<%s\n", __func__);
    return 0;
}

