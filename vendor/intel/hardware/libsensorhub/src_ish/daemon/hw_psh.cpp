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

#undef LOG_TAG
#define LOG_TAG "Libsensorhub"

#define MAX_STRING_SIZE 256
#define MAX_PROP_VALUE_SIZE 58

#define MAX_FW_VERSION_STR_LEN 256
#define BYT_FW "/system/etc/firmware/psh_bk.bin"
#define FWUPDATE_SCRIPT "/system/bin/fwupdate_script.sh "
#define MERRIFIELD 0
#define BAYTRAIL 1

char platform;

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
				tran_id, cmd_type_to_cmd_id[cmd_id], hw_sensor_id, data_rate_byte[0],
				data_rate_byte[1], buffer_delay_byte[0],
				buffer_delay_byte[1], bit_cfg_byte[0], bit_cfg_byte[1]);
	if (size <= 0)
		return -1;

	log_message(DEBUG, "cmd to sysfs is: %s\n", cmd_string);
	/* TODO: error handling if (size <= 0 || size > MAX_STRING_SIZE) */
	ret = write(ctlfd, cmd_string, size);
	log_message(DEBUG, "cmd return value is %d\n", ret);
	if (ret < 0)
		return -1;

	return 0;
}

/* 0 on success; -1 on fail */
int internal_send_control_cmd(int tran_id, int cmd_id, int hw_sensor_id,
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
	/* TODO: error handling if (size <= 0 || size > MAX_STRING_SIZE) */
	ret = write(ctlfd, cmd_string, size);
	log_message(DEBUG, "cmd return value is %d\n", ret);
	if (ret < 0)
		return -1;

	return 0;
}

static void dispatch_data()
{
	static char *buf = NULL;
	char *p;
	int ret, data_size, left = 0;
	struct cmd_resp *p_cmd_resp;
	struct timeval tv, tv1;
	char datasize_buf[8];

	if (buf == NULL)
		buf = (char *)malloc(128 * 1024);

	if (buf == NULL) {
		ALOGE("dispatch_data(): malloc failed \n");
		return;
	}

	gettimeofday(&tv, NULL);

	log_message(DEBUG, "Got a packet, timestamp is %d, %d \n",
					tv.tv_sec, tv.tv_usec);

	/* read data_size node */
	lseek(datasizefd, 0, SEEK_SET);
	ret = read(datasizefd, datasize_buf, 8);
	if (ret <= 0)
		return;

	datasize_buf[7] = '\0';
	sscanf(datasize_buf, "%d", &data_size);

	if ((data_size <= 0) || (data_size > 128 * 1024))
		return;

	left = data_size;

	/* sysfs has limitation that max 4K data can be returned at once */
	while (left > 0) {
		ret = read(datafd, buf + data_size - left, left);
		if (ret <= 0)
			return;

		left = left - ret;
	}

	ret = data_size;

//	log_message(DEBUG, "data_size is: %d, read() return value is %d \n",
//							data_size, ret);

	p_cmd_resp = (struct cmd_resp *) buf;
	p = buf;
	while (ret > 0) {
		log_message(DEBUG, "tran_id: %d, cmd_type: %d, sensor_id: %d, "
				"data_len: %d \n", p_cmd_resp->tran_id,
				p_cmd_resp->cmd_type, p_cmd_resp->hw_sensor_id,
				p_cmd_resp->data_len);

		if ((p_cmd_resp->cmd_type == RESP_GET_SINGLE) || (p_cmd_resp->cmd_type == RESP_BIST_RESULT))
			dispatch_get_single(p_cmd_resp);
		else if (p_cmd_resp->cmd_type == RESP_STREAMING) {
			dispatch_streaming(p_cmd_resp);
		} else if (p_cmd_resp->cmd_type == RESP_COMP_CAL_RESULT){
			struct cmd_calibration_param param;
			struct resp_compasscal *p = (struct resp_compasscal*)p_cmd_resp->buf;

			if (p_cmd_resp->data_len != sizeof(struct resp_compasscal))
				log_message(DEBUG, "Get a calibration_get response with wrong data_len:%d\n",
											p_cmd_resp->data_len);

			param.sensor_type = SENSOR_CALIBRATION_COMP;
			param.sub_cmd = SUBCMD_CALIBRATION_GET;
			param.calibrated = p->calib_result;
			param.cal_param.compass = p->info;
			handle_calibration(&param);
		} else if (p_cmd_resp->cmd_type == RESP_GYRO_CAL_RESULT){
			struct cmd_calibration_param param;
			struct resp_gyrocal *p = (struct resp_gyrocal*)p_cmd_resp->buf;

			if (p_cmd_resp->data_len != sizeof(struct resp_gyrocal))
				log_message(DEBUG, "Get a calibration_get response with wrong data_len:%d\n",
											p_cmd_resp->data_len);

			param.sensor_type = SENSOR_CALIBRATION_GYRO;
			param.sub_cmd = SUBCMD_CALIBRATION_GET;
			param.calibrated = p->calib_result;
			param.cal_param.gyro = p->info;
			handle_calibration(&param);
		} else if (p_cmd_resp->cmd_type == RESP_ADD_EVENT) {
			/* TODO: record event id, match client with trans id, send return value to client */
			handle_add_event_resp(p_cmd_resp);
		} else if (p_cmd_resp->cmd_type == RESP_CLEAR_EVENT) {
			handle_clear_event_resp(p_cmd_resp);
		} else if (p_cmd_resp->cmd_type == RESP_EVENT) {
			/* dispatch event */
			dispatch_event(p_cmd_resp);
		} else
			log_message(DEBUG,
			    "%d: not support this response currently\n", p_cmd_resp->cmd_type);

		ret = ret - (sizeof(struct cmd_resp) + p_cmd_resp->data_len);
//		log_message(DEBUG, "remain data len is %d\n", ret);

		p = p + sizeof(struct cmd_resp) + p_cmd_resp->data_len;
		p_cmd_resp = (struct cmd_resp *)p;
	}

	gettimeofday(&tv1, NULL);
	log_message(DEBUG, "Done with dispatching a packet, latency is "
					"%d \n", tv1.tv_usec - tv.tv_usec);
}


void ProcessHW(int fd)
{
	dispatch_data();
}

/* get data from data node and dispatch it to clients, begin */
int FillHWDescriptors(int maxfd, void *read_fds, void *exception_fds, int *hw_fds, int *hw_fds_num, bool *toException)
{
	fd_set *datasize_fds = (fd_set *) exception_fds;
	int new_max_fd = (maxfd < datasizefd) ? datasizefd : maxfd;


	FD_ZERO(datasize_fds);
	FD_SET(datasizefd, datasize_fds);
	*toException = true;
	hw_fds[0] = datasizefd;
	*hw_fds_num = 1;

	return new_max_fd;
}

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
			sensor_list = (sensor_state_t *) realloc(sensor_list, sizeof(sensor_state_t) * (current_sensor_index + 1));
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
		sensor_list = (sensor_state_t *) realloc(sensor_list, sizeof(sensor_state_t) * (current_sensor_index + 1));
		if (sensor_list == NULL)
			exit(EXIT_FAILURE);
		memset(&sensor_list[current_sensor_index], 0, sizeof(sensor_state_t));
	}

	memcpy(sensor_list[current_sensor_index].name, "EVENT", SNR_NAME_MAX_LEN);
	current_sensor_index++;

	gettimeofday(&tv1, NULL);
	ALOGI("latency of is get_status() is "
		"%ld \n", tv1.tv_usec - tv.tv_usec);

	if (ret < 0)
		 exit(EXIT_FAILURE);
}

static void setup_psh()
{
	// setup DDR
	internal_send_control_cmd(0, 1, 0, 0, 0, 0);
	// reset
	internal_send_control_cmd(0, 0, 0, 0, 0, 0);

	return;
}

static int get_fw_version(char *fw_version_str)
{
	FILE *fp;
	int filelen;
	char *ver_p = NULL;
	int n = 0;
	int ret = 0;

	fp = fopen(BYT_FW, "r");
	if (fp == NULL) {
		log_message(CRITICAL,"Failed to open firmware file !!!\r\n");
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	filelen = ftell(fp);

	char *buf = (char*)  malloc(filelen + 1);
	if (buf == NULL) {
		fclose(fp);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	fread(buf, 1, filelen, fp);
	buf[filelen]='\0';

	while(ver_p == NULL) {
		ver_p = strstr(buf+n, "VERSION:");
		if (ver_p == NULL) {
			while ((n<(filelen-8))&&(buf[n]!='\0'))
				n++;

			while ((n<(filelen-8))&&(buf[n]=='\0'))
				n++;
		}
		if (n>=(filelen-8)) break;
	}

	if (ver_p!=NULL) {
		/* filter string"VERSION:" */
		strncpy(fw_version_str, ver_p+8, MAX_FW_VERSION_STR_LEN-1);
		ret = 0;
	} else {
		ret = -1;
	}

	fclose(fp);
	free(buf);

	return ret;
}

static int fw_verion_compare()
{
	char version_str_running[MAX_FW_VERSION_STR_LEN];
	char version_str[MAX_FW_VERSION_STR_LEN];
	int length = 0;

	if (get_fw_version(version_str)) {
		log_message(CRITICAL, "can not get the bin fw version!!!\n");
		return -2;
	}

	version_str[MAX_FW_VERSION_STR_LEN-1] = '\0';

	length = strlen(version_str);
	if (length<8) {
		log_message(CRITICAL, "fw version string is error!!!\n");
		return -1;
	}

	lseek(fwversionfd, 0, SEEK_SET);
	if (read(fwversionfd, version_str_running, MAX_FW_VERSION_STR_LEN) <= 0) {
		log_message(CRITICAL, "can not get the running fw version!!!\n");
		return -1;
	}

	version_str_running[length] = '\0';

	if (strcmp(version_str, version_str_running)) {
		log_message(CRITICAL, "psh firmware versions are not same!!!\n");
		return -1;
	}

	return 0;
}

void reset_hw_layer()
{
	char node_path[MAX_STRING_SIZE];
	DIR * dirp;
	struct dirent * entry;
	int found = 0;

	if (ctlfd != -1) {
		close(ctlfd);
		ctlfd = -1;
	}

	if (datafd != -1) {
		close(datafd);
		datafd = -1;
	}

	if (datasizefd != -1) {
		close(datasizefd);
		datasizefd = -1;
	}

	if (fwversionfd != -1) {
		close(fwversionfd);
		fwversionfd = -1;
	}

		/* detect the device node */
	dirp = opendir("/sys/class/hwmon");
	if (dirp == NULL) {
		ALOGE("can't find device node \n");
		exit(EXIT_FAILURE);
	}

	while ((entry = readdir(dirp)) != NULL) {
		int fd;
		char magic_string[32];

		if ((strcmp((const char *)entry->d_name, ".") == 0) ||(strcmp((const char *)entry->d_name, "..") == 0))
			continue;

		snprintf(node_path, MAX_STRING_SIZE, "/sys/class/hwmon/%s/device/modalias", entry->d_name);

		fd = open(node_path, O_RDONLY);
		if (fd == -1)
			continue;

		read(fd, magic_string, 32);
		magic_string[31] = '\0';
		close(fd);

		log_message(DEBUG, "magic_string is %s \n", magic_string);

		if (strstr(magic_string, "11A4") != NULL) {
			platform = MERRIFIELD;
			found = 1;
			break;
		}

		if ((strstr(magic_string, "psh") != NULL)
			|| (strstr(magic_string, "SMO91D0:00") != NULL)) {
			platform = BAYTRAIL;
			found = 1;
			break;
		}
	}

	if (found == 0) {
		ALOGE("can't find device node \n");
		exit(EXIT_FAILURE);
	}

	log_message(DEBUG, "detected node dir is %s \n", entry->d_name);

	/* open control node */
	snprintf(node_path, MAX_STRING_SIZE, "/sys/class/hwmon/%s/device/control", entry->d_name);
	ctlfd = open(node_path, O_WRONLY);
	if (ctlfd == -1) {
		ALOGE("open %s failed, errno is %d\n",
				node_path, errno);
		exit(EXIT_FAILURE);
	}

	/* TODO: send 'reset' cmd */

	/* open data node */
	snprintf(node_path, MAX_STRING_SIZE, "/sys/class/hwmon/%s/device/data", entry->d_name);
	datafd = open(node_path, O_RDONLY);
	if (datafd == -1) {
		ALOGE("open %s failed, errno is %d\n",
				node_path, errno);
		exit(EXIT_FAILURE);
	}

	/* open data_size node */
	snprintf(node_path, MAX_STRING_SIZE, "/sys/class/hwmon/%s/device/data_size", entry->d_name);
	datasizefd = open(node_path, O_RDONLY);
	if (datasizefd == -1) {
		ALOGE("open %s failed, errno is %d\n",
				node_path, errno);
		exit(EXIT_FAILURE);
	}

	closedir(dirp);

	
	/* open fwversion node */
	snprintf(node_path, MAX_STRING_SIZE, "/sys/class/hwmon/%s/device/fw_version", entry->d_name);
	fwversionfd = open(node_path, O_RDONLY);
	if (fwversionfd == -1) {
		ALOGE("open %s failed, errno is %d\n",
			node_path, errno);
		exit(EXIT_FAILURE);
	}

	if (platform == BAYTRAIL)
		/* if fwupdat.flag is not exist or update failed, update fw */
		system(FWUPDATE_SCRIPT BYT_FW);
//		sleep(60);
	setup_psh();

	if (platform == BAYTRAIL) {
		/* fw version is not same or can't get version, update firmware */
		if (fw_verion_compare() == -1) {
			internal_send_control_cmd(0, 255, 0, 0, 0, 0);
			system(FWUPDATE_SCRIPT BYT_FW " force");
			setup_psh();
		}
	}

	get_status();
}


/*
 * 60 bytes can be transfered once maximumly, including property value length and property type.
 * So actually 58 bytes of property value can be sent once.
 *
 * send_set_property() will split property value buffer larger than 58 bytes into segments and
 * send them one by one.
 *
 * User should handle start/end flags, by themselves in their *value buffer.
 * User should handle segments concatenation by themselves in virtual sensor.
 */
int send_set_property(sensor_state_t *p_sensor_state, property_type prop_type, int len, unsigned char *value)
{
	char cmd_string[MAX_STRING_SIZE];
	int size, ret, i, j, set_len, left_len;
	unsigned char *p = value;
	unsigned char prop_type_byte = (unsigned char )prop_type;

	if (p_sensor_state == NULL) {
		log_message(CRITICAL, "%s: p_sensor_state is NULL \n", __func__);
		return -1;
	}

	log_message(CRITICAL, "prop type: %d len: %d value: %d %d %d %d\n", prop_type_byte, len,
			value[0], value[1], value[2], value[3]);

	left_len = len;
	for (j = 0; j < (len + MAX_PROP_VALUE_SIZE - 1) / MAX_PROP_VALUE_SIZE; j++) {
		set_len = left_len > MAX_PROP_VALUE_SIZE ? MAX_PROP_VALUE_SIZE : left_len;
		size = snprintf(cmd_string, MAX_STRING_SIZE, "%d %d %d %d %d", 0,
				cmd_type_to_cmd_id[CMD_SET_PROPERTY], p_sensor_state->hw_sensor_id, set_len + 1,
				prop_type_byte);
		for (i = 0; i < set_len; i++) {
			size += snprintf(cmd_string + size, MAX_STRING_SIZE - size, " %d", p[i]);
		}

		log_message(DEBUG, "cmd to sysfs is: %s\n", cmd_string);
		ret = write(ctlfd, cmd_string, size);
		log_message(DEBUG, "cmd return value is %d\n", ret);

                while (ret < 0)
			return -1;

		p += set_len;
		left_len -= set_len;
	}

	return 0;
}

void get_calibration(sensor_state_t *p_sensor_state,
				session_state_t *p_session_state)
{
	char cmdstring[MAX_STRING_SIZE];
	int ret, len;

	if (p_sensor_state == NULL) {
		log_message(DEBUG, "%s: p_sensor_state is NULL \n", __func__);
		return;
	}

	if (strncmp(p_sensor_state->name, "COMPC", SNR_NAME_MAX_LEN) != 0 &&
		strncmp(p_sensor_state->name, "GYROC", SNR_NAME_MAX_LEN) != 0) {
		log_message(DEBUG, "Get calibration with confused parameter,"
					"sensor type is not for calibration, is %s\n",
					p_sensor_state->name);
		return;
	}

	len = snprintf (cmdstring, MAX_STRING_SIZE, "%d %d %d %d",
			0,			// tran_id
			cmd_type_to_cmd_id[CMD_GET_CALIBRATION],	// cmd_id
			p_sensor_state->hw_sensor_id,	// sensor_id
			SUBCMD_CALIBRATION_GET);		// sub_cmd

	if (len <= 0) {
		log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
		return;
	}

	log_message(DEBUG, "Send to control node :%s\n", cmdstring);
	ret = write(ctlfd, cmdstring, len);
	if (ret != len)
		log_message(DEBUG, "[%s]Error in sending cmd:%d\n", __func__, ret);

	if (p_session_state)
		p_session_state->get_calibration = 1;
}

void set_calibration(sensor_state_t *p_sensor_state,
				struct cmd_calibration_param *param)
{
	char cmdstring[MAX_STRING_SIZE];
	int ret, len = 0;

	if (p_sensor_state == NULL) {
		log_message(DEBUG, "%s: p_sensor_state is NULL \n", __func__);
		return;
	}

	/* Too bad we cannot use send_control_cmd, send cmd by hand. */
	// echo trans cmd sensor [sensor subcmd calibration [struct parameter info]]

	log_message(DEBUG, "Begin to form command string.\n");
	len = snprintf (cmdstring, MAX_STRING_SIZE, "%d %d %d %d ",
			0,			// tran_id
			cmd_type_to_cmd_id[CMD_SET_CALIBRATION],	// cmd_id
			p_sensor_state->hw_sensor_id,	// sensor_id
			param->sub_cmd);		// sub_cmd

	if (len <= 0) {
		log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
		return;
	}

	if (param->sub_cmd == SUBCMD_CALIBRATION_SET) {
		// 30 parameters, horrible Orz...
		unsigned char* p;
		log_message(DEBUG, "Set calibration, sensor_type %s\n", p_sensor_state->name);

		if (param->sub_cmd != SUBCMD_CALIBRATION_SET) {
			log_message(DEBUG, "Set calibration with confused parameter,"
						"sub_cmd is %d\n",param->sub_cmd);

			return;
		}

		if (param->calibrated != SUBCMD_CALIBRATION_TRUE) {
			log_message(DEBUG, "Set calibration with confused parameter,"
						"calibrated is %u\n",param->calibrated);

			return;
		}

		if (strncmp(p_sensor_state->name, "COMPC", SNR_NAME_MAX_LEN) == 0) {
			/* For compass_cal, the parameter is:
			 * offset[3], w[3][3], bfield
			 */
			int i, j;

			for (i = 0; i < 3; i++) {
				p = (unsigned char*)&param->cal_param.compass.offset[i];
				len += snprintf (cmdstring + len, MAX_STRING_SIZE - len, "%d %d %d %d ",
					p[0], p[1], p[2], p[3]); // offset[i]
			}

			if (len <= 0) {
				log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
				return;
			}

			for (i = 0; i < 3; i++)
				for (j = 0; j < 3; j++) {
				p = (unsigned char*)&param->cal_param.compass.w[i][j];
				len += snprintf (cmdstring + len, MAX_STRING_SIZE - len, "%d %d %d %d ",
					p[0], p[1], p[2], p[3]); // w[i][j]
			}

			if (len <= 0) {
				log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
				return;
			}

			p = (unsigned char*)&param->cal_param.compass.bfield;
			len += snprintf (cmdstring + len, MAX_STRING_SIZE - len, "%d %d %d %d",
				p[0], p[1], p[2], p[3]); // bfield

			if (len <= 0) {
				log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
				return;
			}
		} else if (strncmp(p_sensor_state->name, "GYROC", SNR_NAME_MAX_LEN) == 0) {
			/* For gyro_cal, the parameter is:
			 * x, y, z
			 */
			p = (unsigned char*)&param->cal_param.gyro.x;
			len += snprintf (cmdstring + len, MAX_STRING_SIZE - len, "%d %d ",
				p[0], p[1]); // x

			if (len <= 0) {
				log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
				return;
			}

			p = (unsigned char*)&param->cal_param.gyro.y;
			len += snprintf (cmdstring + len, MAX_STRING_SIZE - len, "%d %d ",
				p[0], p[1]); // y

			if (len <= 0) {
				log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
				return;
			}

			p = (unsigned char*)&param->cal_param.gyro.z;
			len += snprintf (cmdstring + len, MAX_STRING_SIZE - len, "%d %d",
				p[0], p[1]); // z

			if (len <= 0) {
				log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
				return;
			}

		}
	} else if (param->sub_cmd == SUBCMD_CALIBRATION_START) {
		log_message(DEBUG, "Calibration START, sensor_type %s\n", p_sensor_state->name);

	} else if (param->sub_cmd == SUBCMD_CALIBRATION_STOP) {
		log_message(DEBUG, "Calibration STOP, sensor_type %s\n", p_sensor_state->name);

	}
	if (len <= 0) {
		log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
		return;
	}

	log_message(DEBUG, "Send to control node :%s\n", cmdstring);
	ret = write(ctlfd, cmdstring, len);
	if (ret != len)
		log_message(DEBUG, "[%s]Error in sending cmd:%d\n", __func__, ret);
}

 void handle_add_event(session_state_t *p_session_state, cmd_event* p_cmd)
{
	unsigned char trans_id;
	char cmd_string[MAX_STRING_SIZE];
	int len, ret;

	log_message(DEBUG, "[%s] Ready to handle command %d\n", __func__, p_cmd->cmd);

	/* allocate a tran_id, send cmd to PSH */
	struct cmd_event_param *evt_param = (struct cmd_event_param *)p_cmd->buf;
	int i, num = evt_param->num;

	trans_id = allocate_trans_id();

	p_session_state->trans_id = trans_id;

	/* struct ia_cmd { u8 tran_id; u8 cmd_id; u8 sensor_id; char param[] }; */
	len = snprintf(cmd_string, MAX_STRING_SIZE, "%d %d %d ", trans_id, cmd_type_to_cmd_id[CMD_ADD_EVENT], 0);
	if (len <= 0) {
		log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
		return;
	}
	/* struct cmd_event_param {u8 num; u8 op; struct sub_event evts[] }; */
	len += snprintf(cmd_string + len, MAX_STRING_SIZE - len, "%d %d ", num, evt_param->relation);
	if (len <= 0) {
		log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
		return;
	}
	/* struct sub_event {u8 sensor_id; u8 chan_id; opt_id; u32 param1; u32 param2; }; */
	for (i = 0; i < num; i++) {
		struct sub_event *sub_event = &(evt_param->evts[i]);
		unsigned char* p;
		sensor_state_t *p_sensor_state;

		p_sensor_state = get_sensor_state_with_name(sensor_type_to_name_str[sub_event->sensor_id].name);
		if (p_sensor_state == NULL) {
			log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
			return;
		}

		len += snprintf(cmd_string + len, MAX_STRING_SIZE - len, "%d %d %d ", p_sensor_state->hw_sensor_id, sub_event->chan_id, sub_event->opt_id);
		if (len <= 0) {
			log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
			return;
		}
		p = (unsigned char*)&sub_event->param1;
		len += snprintf(cmd_string + len, MAX_STRING_SIZE - len, "%d %d %d %d ", p[0], p[1], p[2], p[3]);
		if (len <= 0) {
			log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
			return;
		}
		p = (unsigned char*)&sub_event->param2;
		len += snprintf(cmd_string + len, MAX_STRING_SIZE - len, "%d %d %d %d ", p[0], p[1], p[2], p[3]);
		if (len <= 0) {
			log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
			return;
		}
	}

	if (len <= 0) {
		log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
		return;
	}

	ret = write(ctlfd, cmd_string, len);
	if (ret != len)
		log_message(DEBUG, "[%s]Error in sending cmd:%d\n", __func__, ret);

	return;
}

void handle_clear_event(session_state_t *p_session_state)
{
	char cmd_string[MAX_STRING_SIZE];
	int len, ret;

	if (p_session_state->event_id == 0)
		return;

	/* struct ia_cmd { u8 tran_id; u8 cmd_id; u8 sensor_id; char param[] }; */
	len = snprintf(cmd_string, MAX_STRING_SIZE, "%d %d %d ", 0, cmd_type_to_cmd_id[CMD_CLEAR_EVENT], 0);
	/* struct clear_evt_param {u8 evt_id}; */
	len += snprintf(cmd_string + len, MAX_STRING_SIZE - len, "%d ", p_session_state->event_id);

	p_session_state->event_id = 0;
	p_session_state->trans_id = 0;

	if (len <= 0) {
		log_message(CRITICAL, "[%s] failed to compose cmd_string \n", __func__);
		return;
	}

	ret = write(ctlfd, cmd_string, len);
	if (ret != len)
		log_message(DEBUG, "[%s]Error in sending cmd:%d\n", __func__, ret);

	return;
}

