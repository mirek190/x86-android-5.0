#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <string.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <pthread.h>
#include <hardware_legacy/power.h>
#include <cutils/sockets.h>

#include "../include/socket.h"
#include "../include/utils.h"
#include "../include/message.h"
#include "../include/bist.h"
#include "../include/hw.h"
#include "../include/upperHalfHandlers.h"
#include "../include/iio-hal/quatro_sensors.h"

#include <map>
#include <string>

#undef LOG_TAG
#define LOG_TAG "Libsensorhub"

#define MAX_STRING_SIZE 256
#define MAX_PROP_VALUE_SIZE 58

AccelSensor *accel = NULL;
GyroSensor *gyro = NULL;
CompassSensor *compass = NULL;
ALSSensor *als = NULL;


struct cmd_resp *AllocResponse(unsigned int len)
{
	void *p = malloc(len + offsetof(struct cmd_resp, buf));
	return (struct cmd_resp*) p;
}

struct cmd_resp *resp = NULL;

//todo don't forget clear maps when resetting
std::map<int, SensorIIODev *> FdToSensor;
std::map<int, SensorIIODev *> hwIdToSensor;
std::map<int, int> DataSize;

const int SENSORS_NUM = 4;
int sensorFDs[SENSORS_NUM];

std::map<std::string, std::string> psh2iioNames;

void DiscoverSensor(SensorIIODev *sens, int index, int datasize = 12)
{
	if (sens->discover() < 0) 
	{
		log_message(DEBUG, "cannot find accelerometer device\n");
		sensorFDs[index] = -1;
	}
	else
	{
		
		log_message(DEBUG,"Discovered Sensor %s path is %s\n", sens->device_name.c_str(), sens->mDevPath.c_str());

		int trials = 5;
		bool done=false;

		while(!done && trials)
		{
			sensorFDs[index] = open(sens->mDevPath.c_str(), O_RDONLY);
			if (sensorFDs[index]  == -1) 
				log_message(WARNING, "Failed to open; reason: %s\n", strerror(errno));
			else
				done = true;
			trials--;
			sleep(1);
		}

		FdToSensor[sensorFDs[index]] = sens;
		sensor_list[current_sensor_index].hw_sensor_id = sens->GetDeviceNumber();
		sensor_list[current_sensor_index].freq_max = 500;
		memcpy(sensor_list[current_sensor_index].name, psh2iioNames[sens->device_name].c_str(), SNR_NAME_MAX_LEN);
		hwIdToSensor[sensor_list[current_sensor_index].hw_sensor_id] = sens;
		DataSize[sensorFDs[index]] = datasize;
		printf("registered sensor %s\n", sensor_list[current_sensor_index].name);
		current_sensor_index++;
		
	}
}

void reset_hw_layer()
{
	log_message(DEBUG,"reset_hw_layer start\n");
	//todo - add more discuss with MCG guys
	//{{"ACCEL"}, {"GYRO"}, {"COMPS"}, {"BARO"}, {"ALS_P"}, {"PS_P"}, {"TERMC"}, {"LPE_P"}, {"ACC1"}, {"GYRO1"}, {"COMP1"}, {"ALS1"}, {"PS1"}, {"BARO1"}, {"PHYAC"}, {"GSSPT"}, {"GSFLK"}, {"RVECT"}, {"GRAVI"}, {"LACCL"}, {"ORIEN"}, {"COMPC"}, {"GYROC"}, {"9DOF"}, {"PEDOM"}, {"MAGHD"}, {"SHAKI"}, {"MOVDT"}, {"STAP"}, {"BIST"}, {"EVENT"}};

	psh2iioNames["accel_3d"] = "ACCEL";
	psh2iioNames["gyro_3d"] = "GYRO";
	psh2iioNames["magn_3d"] = "COMPS";
	psh2iioNames["als"] = "ALS_P";

	if(!resp)
		resp = AllocResponse(36);

	if (accel) {
		close(accel->mFd);
		delete accel;
	}
	accel = new AccelSensor();
	DiscoverSensor(accel, 0);
	

	if (gyro) {
		close(gyro->mFd);
		delete gyro;
	}
	gyro = new GyroSensor();
	DiscoverSensor(gyro, 1);
	

	if (compass) {
		close(compass->mFd);
		delete compass;
	}
	compass = new CompassSensor();
	DiscoverSensor(compass, 2);
	

	if (als) {
		close(als->mFd);
		delete als;
	}
	als = new ALSSensor();
	DiscoverSensor(als, 3, 4);
	

	log_message(DEBUG,"reset_hw_layer end\n");

	//todo fill sensors as get_status
}

static void dispatch_data()
{
//	static char *buf = NULL;
//	char *p;
//	int ret, data_size, left = 0;
//	struct cmd_resp *p_cmd_resp;
//	struct timeval tv, tv1;
//	char datasize_buf[8];
//
//	if (buf == NULL)
//		buf = (char *)malloc(128 * 1024);
//
//	if (buf == NULL) {
//		ALOGE("dispatch_data(): malloc failed \n");
//		return;
//	}
//
//	gettimeofday(&tv, NULL);
//
//	log_message(DEBUG, "Got a packet, timestamp is %d, %d \n",
//					tv.tv_sec, tv.tv_usec);
//
//	/* read data_size node */
//	lseek(datasizefd, 0, SEEK_SET);
//	ret = read(datasizefd, datasize_buf, 8);
//	if (ret <= 0)
//		return;
//
//	datasize_buf[7] = '\0';
//	sscanf(datasize_buf, "%d", &data_size);
//
//	if ((data_size <= 0) || (data_size > 128 * 1024))
//		return;
//
//	left = data_size;
//
//	/* sysfs has limitation that max 4K data can be returned at once */
//	while (left > 0) {
//		ret = read(datafd, buf + data_size - left, left);
//		if (ret <= 0)
//			return;
//
//		left = left - ret;
//	}
//
//	ret = data_size;
//
////	log_message(DEBUG, "data_size is: %d, read() return value is %d \n",
////							data_size, ret);
//
//	p_cmd_resp = (struct cmd_resp *) buf;
//	p = buf;
//	while (ret > 0) {
//		log_message(DEBUG, "tran_id: %d, cmd_type: %d, sensor_id: %d, "
//				"data_len: %d \n", p_cmd_resp->tran_id,
//				p_cmd_resp->cmd_type, p_cmd_resp->hw_sensor_id,
//				p_cmd_resp->data_len);
//
//		if ((p_cmd_resp->cmd_type == RESP_GET_SINGLE) || (p_cmd_resp->cmd_type == RESP_BIST_RESULT))
//			dispatch_get_single(p_cmd_resp);
//		else if (p_cmd_resp->cmd_type == RESP_STREAMING) {
//			dispatch_streaming(p_cmd_resp);
//		} else if (p_cmd_resp->cmd_type == RESP_COMP_CAL_RESULT){
//			struct cmd_calibration_param param;
//			struct resp_compasscal *p = (struct resp_compasscal*)p_cmd_resp->buf;
//
//			if (p_cmd_resp->data_len != sizeof(struct resp_compasscal))
//				log_message(DEBUG, "Get a calibration_get response with wrong data_len:%d\n",
//											p_cmd_resp->data_len);
//
//			param.sensor_type = SENSOR_CALIBRATION_COMP;
//			param.sub_cmd = SUBCMD_CALIBRATION_GET;
//			param.calibrated = p->calib_result;
//			param.cal_param.compass = p->info;
//			handle_calibration(&param);
//		} else if (p_cmd_resp->cmd_type == RESP_GYRO_CAL_RESULT){
//			struct cmd_calibration_param param;
//			struct resp_gyrocal *p = (struct resp_gyrocal*)p_cmd_resp->buf;
//
//			if (p_cmd_resp->data_len != sizeof(struct resp_gyrocal))
//				log_message(DEBUG, "Get a calibration_get response with wrong data_len:%d\n",
//											p_cmd_resp->data_len);
//
//			param.sensor_type = SENSOR_CALIBRATION_GYRO;
//			param.sub_cmd = SUBCMD_CALIBRATION_GET;
//			param.calibrated = p->calib_result;
//			param.cal_param.gyro = p->info;
//			handle_calibration(&param);
//		} else if (p_cmd_resp->cmd_type == RESP_ADD_EVENT) {
//			/* TODO: record event id, match client with trans id, send return value to client */
//			handle_add_event_resp(p_cmd_resp);
//		} else if (p_cmd_resp->cmd_type == RESP_CLEAR_EVENT) {
//			handle_clear_event_resp(p_cmd_resp);
//		} else if (p_cmd_resp->cmd_type == RESP_EVENT) {
//			/* dispatch event */
//			dispatch_event(p_cmd_resp);
//		} else
//			log_message(DEBUG,
//			    "%d: not support this response currently\n", p_cmd_resp->cmd_type);
//
//		ret = ret - (sizeof(struct cmd_resp) + p_cmd_resp->data_len);
////		log_message(DEBUG, "remain data len is %d\n", ret);
//
//		p = p + sizeof(struct cmd_resp) + p_cmd_resp->data_len;
//		p_cmd_resp = (struct cmd_resp *)p;
//	}
//
//	gettimeofday(&tv1, NULL);
//	log_message(DEBUG, "Done with dispatching a packet, latency is "
//					"%d \n", tv1.tv_usec - tv.tv_usec);
}


void ProcessHW(int fd)
{
	SensorIIODev *sens = FdToSensor[fd];


	int toread = 128;
	int scan_size = DataSize[fd];
	char *data = (char*) malloc(scan_size * toread);
	int read_size = read(fd, data, scan_size * toread);
	int actualSamples = 0;
	if (read_size == -EAGAIN) {
		log_message(WARNING, "nothing available\n");
	}
	else
	{
		if (read_size < 0)
			log_message(WARNING, "Failed to read; reason: %s\n", strerror(errno));
		else
		{
			actualSamples = read_size/scan_size;
			if (actualSamples > 3) read_size = scan_size * 3;
			log_message(DEBUG, "DATA [read_size=%u]:\n", read_size);
			for (int i = 0; i < read_size; ++i)
			{
			  //	log_message(DEBUG, "%02X ", (unsigned)data[i] & 0xFF);
				resp->buf[i] = data[i] ;
			}
			for (int j = 0; j < actualSamples, j < 3; j++)
			{
			  unsigned int val =((unsigned)data[j*3 + 3]) * 4096 + 
			    ((unsigned)data[j*3 + 2]) * 256 + ((unsigned)data[j*3 + 1]) * 16 + ((unsigned)data[j*3]);
			    log_message(DEBUG, "value %d is %X\n", val);
			}
			
			resp->cmd_type = RESP_STREAMING;
			resp->data_len = read_size;
			resp->hw_sensor_id = sens->GetDeviceNumber();
			dispatch_streaming(resp);
		}
	}
	free(data);
	
}



/* 0 on success; -1 on fail */
int send_control_cmd(int tran_id, int cmd_id, int hw_sensor_id,
				unsigned short data_rate, unsigned short buffer_delay, unsigned short bit_cfg)
{
	char cmd_string[MAX_STRING_SIZE];
	int size;
	ssize_t ret;
	unsigned char *data_rate_byte = (unsigned char *)&data_rate;
	unsigned char *buffer_delay_byte = (unsigned char *)&buffer_delay;
	unsigned char *bit_cfg_byte = (unsigned char *)&bit_cfg;

	size = snprintf(cmd_string, MAX_STRING_SIZE, "%d %d %d %d %d %d %d %d %d",
				tran_id, cmd_id, hw_sensor_id, data_rate_byte[0],
				data_rate_byte[1], buffer_delay_byte[0],
				buffer_delay_byte[1], bit_cfg_byte[0], bit_cfg_byte[1]);
	if (size <= 0)
		return -1;

	log_message(DEBUG, "cmd to sysfs is: %s\n", cmd_string);
	log_message(DEBUG, "data rate is %d\n", data_rate);
	switch(cmd_id)
	{
		case CMD_START_STREAMING: 
		{
			SensorIIODev *sens = hwIdToSensor[hw_sensor_id];

			if (sens->enable(1) < 0)
				return -1;
			sens->SetSampleDelay(1000); //todo use data rate
			break;
		}
		case CMD_STOP_STREAMING: //todo - extract sensor
		{
			SensorIIODev *sens = hwIdToSensor[hw_sensor_id];

			if (sens->enable(0) < 0)
				return -1;

			break;
		}
	}
	

	return 0;
}


void set_calibration(sensor_state_t *p_sensor_state,
				struct cmd_calibration_param *param)
{
	return;
}

void get_calibration(sensor_state_t *p_sensor_state,
					session_state_t *p_session_state)
{
	return;
}


int FillHWDescriptors(int maxfd, void *read_fds, void *exception_fds, int *hw_fds, int *hw_fds_num, bool *toException)
{
	static bool firstTime = true;
	fd_set *datasize_fds = (fd_set *) read_fds;
	int new_max_fd = maxfd;
	int i = 0;
	int k = 0;
	for (; i < SENSORS_NUM; i++)
		if (sensorFDs[i] != -1)
		{
			FD_SET(sensorFDs[i], datasize_fds);
			if(firstTime)
				log_message(DEBUG,"added new FD: %d\n", sensorFDs[i]);
			if (sensorFDs[i] > new_max_fd)
				new_max_fd = sensorFDs[i];
			(*hw_fds_num)++;
			hw_fds[k] = sensorFDs[i];
			k++;
		}

	*toException = false;
	if(firstTime)
		firstTime = false;
	return new_max_fd;
}

int send_set_property(sensor_state_t *p_sensor_state, property_type prop_type, int len, unsigned char *value)
{
	return 0;
}

//todo - deal with hysteresis
void handle_clear_event(session_state_t *p_session_state)
{
	return;
}
 void handle_add_event(session_state_t *p_session_state, cmd_event* p_cmd)
{
	return;
}

#if 0

#define IA_BIT_CFG_AS_WAKEUP_SRC ((unsigned short)(0x1 << 0))
struct resp_compasscal {
	unsigned char calib_result;
	struct compasscal_info info;
} __attribute__ ((packed));

struct resp_gyrocal {
	unsigned char calib_result;
	struct gyrocal_info info;
} __attribute__ ((packed));

/* The Unix socket file descriptor */
static int 	ctlfd = -1,     // ctrl bottom half socket
		datafd = -1,     // bottom: data from FW device/data
		datasizefd = -1, //bottom: data size from FW device/data_size
		fwversionfd = -1; //bottom half - just file node to read a version

static int enable_debug_data_rate = 0;

static int cmd_type_to_cmd_id[CMD_MAX] = {2, 3, 4, 5, 6, 9, 9, 12};

/* 0 on success; -1 on fail */





#define CMD_GET_STATUS		11

static void get_status()
{
#define LINK_AS_CLIENT		(0)
#define LINK_AS_MONITOR		(1)
#define LINK_AS_REPORTER	(2)
	struct link_info {
		unsigned char id;
		unsigned char ltype;
		unsigned short slide;
	} __attribute__ ((packed));

	struct sensor_info {
		unsigned char id;
		unsigned char status;
		unsigned short freq;
		unsigned short data_cnt;
		unsigned short slide;
		unsigned short priv;
		unsigned short attri;

		short freq_max;	// -1, means no fixed data rate
		char name[SNR_NAME_MAX_LEN + 1];

		unsigned char health;
		unsigned char link_num;
		struct link_info linfo[0];
	} __attribute__ ((packed));

	char cmd_string[MAX_STRING_SIZE];
	int size, ret;
	struct cmd_resp resp;
	struct sensor_info *snr_info;
	char buf[512];

	struct timeval tv, tv1;
	gettimeofday(&tv, NULL);

	size = snprintf(cmd_string, MAX_STRING_SIZE, "%d %d %d %d %d %d %d",
			0, CMD_GET_STATUS, 0, 0xff, 0xff, 0xff, 0xff);
	if (size <= 0) {
		log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
		exit(EXIT_FAILURE);
	}

	ALOGI("cmd to sysfs is: %s\n", cmd_string);

	ret = write(ctlfd, cmd_string, size);
	ALOGI("cmd return value is %d\n", ret);
	if (ret < 0)
		exit(EXIT_FAILURE);

	while ((ret = read(datafd, &resp, sizeof(resp))) >= 0) {
		if (ret == 0) {
			usleep(100000);
			continue;
		}

		// it's safe to cast ret to unsigned int since ret > 0 at this point
		if ((unsigned int)ret < sizeof(resp))
			exit(EXIT_FAILURE);

		if (resp.cmd_type != CMD_GET_STATUS) {
			ret = read(datafd, buf, resp.data_len);
			if (ret != resp.data_len)
				exit(EXIT_FAILURE);
			continue;
		}

		if (resp.data_len == 0)
			break;

		ret = read(datafd, buf, resp.data_len);
		if (ret != resp.data_len)
			exit(EXIT_FAILURE);

		if (current_sensor_index > MAX_SENSOR_INDEX) {
			sensor_list = realloc(sensor_list, sizeof(sensor_state_t) * (current_sensor_index + 1));
			if (sensor_list == NULL)
				exit(EXIT_FAILURE);
			memset(&sensor_list[current_sensor_index], 0, sizeof(sensor_state_t));
		}

		snr_info = (struct sensor_info *)buf;

		ALOGI("sensor id is %d, name is %s, freq_max is %d, current_sensor_index is %d \n", snr_info->id, snr_info->name, snr_info->freq_max, current_sensor_index);

		sensor_list[current_sensor_index].hw_sensor_id = snr_info->id;
		sensor_list[current_sensor_index].freq_max = snr_info->freq_max;
		memcpy(sensor_list[current_sensor_index].name, snr_info->name, SNR_NAME_MAX_LEN);

		current_sensor_index++;
	}

	if (current_sensor_index > MAX_SENSOR_INDEX) {
		sensor_list = realloc(sensor_list, sizeof(sensor_state_t) * (current_sensor_index + 1));
		if (sensor_list == NULL)
			exit(EXIT_FAILURE);
		memset(&sensor_list[current_sensor_index], 0, sizeof(sensor_state_t));
	}

	memcpy(sensor_list[current_sensor_index].name, "EVENT", SNR_NAME_MAX_LEN);
	current_sensor_index++;

	gettimeofday(&tv1, NULL);
	ALOGI("latency of is get_status() is "
		"%d \n", tv1.tv_usec - tv.tv_usec);

	if (ret < 0)
		 exit(EXIT_FAILURE);
}


int pollEvents(SensorIIODev* sensor, sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int n = 0;

    do 
	{
        int nb = sensor->readEvents(data, count);
		printf("readEvents read %d\n", nb);
        if (nb < count) {
            // no more data for this sensor
           

            if (nb < 0) {
                printf("%s: handle:%d error:%d\n", __func__, nb);
                continue;
            }
        }
        count -= nb;
        nbEvents += nb;
        data += nb;
         // if we have events and space, go read them
		usleep(20000);
    } while (count);

    return nbEvents;
}

#endif
