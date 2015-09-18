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
#include <strings.h>
#include <string>

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


#define WAKE_NODE "/sys/power/wait_for_fb_wake"
#define SLEEP_NODE "/sys/power/wait_for_fb_sleep"


static const char *log_file = "/data/sensorhubd.log";
static const char *WAKE_LOCK_ID = "Sensorhubd";

#define IA_BIT_CFG_AS_WAKEUP_SRC ((unsigned short)(0x1 << 0))

/* The Unix socket file descriptor */
static int sockfd = -1, // main upper half socket  (sensorhubd)
		wakefd = -1, 
		sleepfd = -1,
		notifyfd = -1;  //upper half - S3/S0 Notify -union of both above

static int screen_state;
static int enable_debug_data_rate = 0;

#define COMPASS_CALIBRATION_FILE "/data/compass.conf"
#define GYRO_CALIBRATION_FILE "/data/gyro.conf"

/* definition and struct for calibration used by psh_fw */
#define CMD_CALIBRATION_SET	(0x1)
#define CMD_CALIBRATION_GET	(0x2)
#define CMD_CALIBRATION_START	(0x3)
#define CMD_CALIBRATION_STOP	(0x4)
struct cmd_compasscal_param {
	unsigned char sub_cmd;
	struct compasscal_info info;
} __attribute__ ((packed));

struct cmd_gyrocal_param {
	unsigned char sub_cmd;
	struct gyrocal_info info;
} __attribute__ ((packed));

#define CALIB_RESULT_NONE	(0)
#define CALIB_RESULT_USER 	(1)
#define CALIB_RESULT_AUTO 	(2)

/* Calibration status */
#define	CALIBRATION_INIT	(1 << 1)	// Init status
#define	CALIBRATION_RESET	(1 << 2)	// Reset status
#define	CALIBRATION_IN_PROGRESS (1 << 3)	// Calibration request has been sent but get no reply
#define	CALIBRATION_DONE	(1 << 4)	// Get CALIBRATION_GET response
#define	CALIBRATION_NEED_STORE	(1 << 31)	// Additional sepcial status for parameters stored to file

sensor_state_t *sensor_list = NULL;

int current_sensor_index = 0;	// means the current empty slot of sensor_list[]


/* Daemonize the sensorhubd */
static void daemonize()
{
	pid_t sid, pid = fork();

	if (pid < 0) {
		log_message(CRITICAL, "error in fork daemon. \n");
		exit(EXIT_FAILURE);
	} else if (pid > 0)
		exit(EXIT_SUCCESS);

	/* new SID for daemon process */
	sid = setsid();
	if (sid < 0) {
		log_message(CRITICAL, "error in setsid(). \n");
		exit(EXIT_FAILURE);
	}
#ifdef GCOV_DAEMON
	/* just for gcov test, others can ignore it */
	gcov_thread_start();
#endif

	/* close STD fds */
	freopen("/data/my0001.log", "wt", stdout);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	struct passwd *pw;

	pw = getpwnam("system");
	if (pw == NULL) {
		log_message(CRITICAL, "daemonize(): getpwnam return NULL \n");
		return;
	}
	setuid(pw->pw_uid);

	log_message(DEBUG, "sensorhubd is started in daemon mode. \n");
}
#if 0

sensor_state_t* get_sensor_state_with_name(const char *name)
{
        int i;
		char* hwName = NULL;
		hwName = GetHwSensorName(name);
		
		if (hwName)
        {
			log_message(DEBUG, "iio sensors vs %s \n", hwName);

			int len = strlen(hwName);
			for (i = 0; i < current_sensor_index; i ++) {
				
                if (strncmp(sensor_list[i].name, hwName, len) == 0) {
                //if (strncasecmp(sensor_list[i].name, name, len) == 0) {
                      return &sensor_list[i];
				}
            }
		}
        return NULL;
}


#endif

static sensor_state_t* get_sensor_state_with_id(unsigned char hw_sensor_id)
{
	int i;

	for (i = 0; i < current_sensor_index; i ++) {
		if (sensor_list[i].hw_sensor_id == hw_sensor_id) {
			return &sensor_list[i];
		}
	}

	return NULL;
}

sensor_state_t* get_sensor_state_with_name(const char *name)
{
        int i;

        for (i = 0; i < current_sensor_index; i ++) {
                if (strncmp(sensor_list[i].name, name, SNR_NAME_MAX_LEN) == 0) {
                        return &sensor_list[i];
                }
        }

        return NULL;
}

static session_state_t* get_session_state_with_session_id(
					session_id_t session_id)
{
	int i;

	log_message(DEBUG, "get_session_state_with_session_id(): "
				"session_id %d\n", session_id);

	for (i = 0; i < current_sensor_index; i ++) {
		session_state_t *p_session_state = sensor_list[i].list;

		while (p_session_state != NULL) {
			if (p_session_state->session_id == session_id)
				return p_session_state;

			p_session_state = p_session_state->next;
		}
        }

	log_message(WARNING, "get_session_state_with_session_id(): "
				"NULL is returned\n");

	return NULL;
}

#define MAX_UNSIGNED_INT 0xffffffff

static session_id_t allocate_session_id()
{
        /* rewind session ID */
	static session_id_t session_id = 0;
	static int rewind = 0;

	session_id++;

	if (rewind) {
		while (get_session_state_with_session_id(session_id) != NULL)
			session_id++;
	}

	if (session_id == MAX_UNSIGNED_INT)
		rewind = 1;

	return session_id;
}

/* return 0 on success; -1 on fail */
static int get_sensor_state_session_state_with_fd(int ctlfd,
					sensor_state_t **pp_sensor_state,
					session_state_t **pp_session_state)
{
	int i;

	 for (i = 0; i < current_sensor_index; i ++) {
		session_state_t *p_session_state = sensor_list[i].list;

		while (p_session_state != NULL) {
			if (p_session_state->ctlfd == ctlfd) {
				*pp_sensor_state = &sensor_list[i];
				*pp_session_state = p_session_state;

				return 0;
			}

			p_session_state = p_session_state->next;
		}

	}

	return -1;
}

/* flag: 1 means start_streaming, 0 means stop_streaming */
static int data_rate_arbiter(sensor_state_t *p_sensor_state,
				int data_rate,
				session_state_t *p_session_state,
				char flag)
{
	session_state_t *p_session_state_tmp;
	int max_data_rate = 0;

	if (p_sensor_state == NULL) {
		log_message(CRITICAL, "%s: p_sensor_state is NULL \n", __func__);
		return 0;
	}

	if (flag == 1) {
		if (p_sensor_state->data_rate == 0)
			return data_rate;
		else if (data_rate > p_sensor_state->data_rate)
			return data_rate;
		else
			max_data_rate = data_rate;

	}

	/* flag == 0 */
	p_session_state_tmp = p_sensor_state->list;

	while (p_session_state_tmp != NULL) {
		if ((p_session_state_tmp != p_session_state)
			&& ((p_session_state_tmp->state == ACTIVE)
			|| (p_session_state_tmp->state == ALWAYS_ON))) {
			if (max_data_rate < p_session_state_tmp->data_rate) {
				max_data_rate = p_session_state_tmp->data_rate;
			}
		}

		p_session_state_tmp = p_session_state_tmp->next;
	}

	return max_data_rate;
}

/* flag: 1 means add a new buffer_delay, 0 means remove a buffer_delay */
static int buffer_delay_arbiter(sensor_state_t *p_sensor_state,
				int buffer_delay,
				session_state_t *p_session_state,
				char flag)
{
	if (p_sensor_state == NULL) {
		log_message(CRITICAL, "%s: p_sensor_state is NULL \n", __func__);
		return 0;
	}

	if (flag == 1) {
		int r;
		if (buffer_delay == 0)
			return 0;

		/* a new buffer_delay and not the first one */
		if (p_sensor_state->buffer_delay == 0) {
			session_state_t *tmp = p_sensor_state->list;
			while (tmp != NULL) {
				if ((tmp != p_session_state)
					&& ((tmp->state == ACTIVE)
					|| (tmp->state == ALWAYS_ON)))
					return 0;
				tmp = tmp->next;
			}
		}
		r = max_common_factor(p_sensor_state->buffer_delay,
							buffer_delay);
		return r;
	}
	/* flag == 0 */
	int data1 = 0, data2;
	session_state_t *p_session_state_tmp = p_sensor_state->list;

	if ((p_sensor_state->buffer_delay == 0) && (buffer_delay != 0))
		return 0;

	while (p_session_state_tmp != NULL) {
		if ((p_session_state_tmp != p_session_state)
			&& ((p_session_state_tmp->state == ACTIVE)
			|| (p_session_state_tmp->state == ALWAYS_ON))) {
			if (p_session_state_tmp->buffer_delay == 0)
				return 0;

			data2 = p_session_state_tmp->buffer_delay;
			data1 = max_common_factor(data1, data2);
		}

		p_session_state_tmp = p_session_state_tmp->next;
	}

	return data1;
}

/* return as long as anyone is ALWAYS_ON */
static unsigned short bit_cfg_arbiter(sensor_state_t *p_sensor_state,
                                unsigned short bit_cfg,
                                session_state_t *p_session_state)
{
	session_state_t *p_session_state_tmp;

	if (bit_cfg != 0)
		return bit_cfg;

	p_session_state_tmp = p_sensor_state->list;
	while (p_session_state_tmp != NULL) {
		if (p_session_state_tmp != p_session_state) {
			if (p_session_state_tmp->state == ALWAYS_ON) {
				return IA_BIT_CFG_AS_WAKEUP_SRC;
			}
		}

		p_session_state_tmp = p_session_state_tmp->next;
	}

	return 0;
}
void stop_callback(int signum)
{
	log_message(DEBUG, "Caught signal %d\n",signum);

	for (int i = 0; i < current_sensor_index; i ++) 
	{
			send_control_cmd(0, CMD_STOP_STREAMING,
							sensor_list[i].hw_sensor_id, 0, 0, 0);
	}

}

void start_callback(int signum)
{
 	int data_rate_arbitered = 0;
	int buffer_delay_arbitered = 0;
	unsigned short bit_cfg = 0;

	log_message(DEBUG, "Caught signal %d\n",signum);

	for (int i = 0; i < current_sensor_index; i ++) 
	{
			send_control_cmd(0, CMD_START_STREAMING,
							sensor_list[i].hw_sensor_id,
							data_rate_arbitered, buffer_delay_arbitered, bit_cfg);
	}

}
 
 

/* flag: 2 means no_stop_no_report; 1 means no_stop when screen off; 0 means stop when screen off */
static void start_streaming(sensor_state_t *p_sensor_state,
				session_state_t *p_session_state,
				int data_rate, int buffer_delay, int flag)
{
	int data_rate_arbitered, buffer_delay_arbitered;
	unsigned short bit_cfg = 0;

	if (p_sensor_state == NULL) {
		log_message(CRITICAL, "%s: p_sensor_state is NULL \n", __func__);
		return;
	}

//	log_message(CRITICAL, "requested data rate is %d \n", data_rate);

	if (data_rate != -1)
		data_rate_arbitered = data_rate_arbiter(p_sensor_state, data_rate,
						p_session_state, 1);
	else
		data_rate_arbitered = -1;

	buffer_delay_arbitered = buffer_delay_arbiter(p_sensor_state, buffer_delay,
						p_session_state, 1);

	log_message(CRITICAL, "requested data rate is %d, arbitered is %d \n", data_rate, data_rate_arbitered);

	log_message(DEBUG, "CMD_START_STREAMING, data_rate_arbitered "
				"is %d, buffer_delay_arbitered is %d \n",
				data_rate_arbitered, buffer_delay_arbitered);

	p_sensor_state->data_rate = data_rate_arbitered;
	p_sensor_state->buffer_delay = buffer_delay_arbitered;

	/* send CMD_START_STREAMING to psh_fw no matter data rate changes or not,
	   this is requested by Dong for terminal context sensor on 2012/07/17 */
//	if ((sensor_list[sensor_type].data_rate != data_rate_arbitered)
//	||(sensor_list[sensor_type].buffer_delay != buffer_delay_arbitered)) {
		/* send cmd to sysfs control node to change data_rate.
		   NO NEED to send CMD_STOP_STREAMING
		   before CMD_START_STREAMING according to Dong's enhancement
		   on 2011/12/29 */
//		send_control_cmd(0, cmd_type_to_cmd_id[CMD_STOP_STREAMING],
//				sensor_type_to_sensor_id[sensor_type], 0, 0);

	if ((flag == 1) || (flag == 2))
		bit_cfg = IA_BIT_CFG_AS_WAKEUP_SRC;
	else
		bit_cfg = bit_cfg_arbiter(p_sensor_state, 0, p_session_state);

	send_control_cmd(0, CMD_START_STREAMING,
			p_sensor_state->hw_sensor_id,
			data_rate_arbitered, buffer_delay_arbitered, bit_cfg);
//	}

	if (data_rate != 0) {
		p_session_state->data_rate = data_rate;
		p_session_state->buffer_delay = buffer_delay;
		p_session_state->tail = 0;
		if (flag == 0)
			p_session_state->state = ACTIVE;
		else if (flag == 1)
			p_session_state->state = ALWAYS_ON;
		else if (flag == 2) {
			p_session_state->state = ALWAYS_ON;
			p_session_state->flag = 1;
		}
	}
}

static void stop_streaming(sensor_state_t *p_sensor_state,
				session_state_t *p_session_state)
{
	int data_rate_arbitered, buffer_delay_arbitered;
	unsigned short bit_cfg_arbitered;

	if (p_sensor_state == NULL) {
		log_message(CRITICAL, "%s: p_sensor_state is NULL \n", __func__);
		return;
	}

	if (p_session_state->state == INACTIVE)
		return;

	if (p_session_state->state == NEED_RESUME) {
		p_session_state->state = INACTIVE;
		return;
	}

	data_rate_arbitered = data_rate_arbiter(p_sensor_state,
						p_session_state->data_rate,
						p_session_state, 0);
	buffer_delay_arbitered = buffer_delay_arbiter(p_sensor_state,
						p_session_state->buffer_delay,
						p_session_state, 0);

	p_session_state->state = INACTIVE;

	if ((p_sensor_state->data_rate == data_rate_arbitered)
	&& (p_sensor_state->buffer_delay == buffer_delay_arbitered))
		return;

	p_sensor_state->data_rate = data_rate_arbitered;
	p_sensor_state->buffer_delay = buffer_delay_arbitered;

	if (data_rate_arbitered != 0) {
		/* send CMD_START_STREAMING to sysfs control node to
		   re-config the data rate */
		bit_cfg_arbitered = bit_cfg_arbiter(p_sensor_state, 0, p_session_state);
		send_control_cmd(0, CMD_START_STREAMING,
				p_sensor_state->hw_sensor_id,
				data_rate_arbitered, buffer_delay_arbitered, bit_cfg_arbitered);
	} else {
		/* send CMD_STOP_STREAMING cmd to sysfs control node */
		send_control_cmd(0, CMD_STOP_STREAMING,
			p_sensor_state->hw_sensor_id, 0, 0, 0);
	}

//	sensor_list[sensor_type].count = 0;

	log_message(DEBUG, "CMD_STOP_STREAMING, data_rate_arbitered is %d, "
			"buffer_delay_arbitered is %d \n", data_rate_arbitered,
			buffer_delay_arbitered);
}


static void get_single(sensor_state_t *p_sensor_state,
				session_state_t *p_session_state)
{
	log_message(DEBUG, "get_single is called with sensor_type %s \n",
							p_sensor_state->name);

	if (p_sensor_state == NULL) {
		log_message(CRITICAL, "%s: p_sensor_state is NULL \n", __func__);
		return;
	}

	log_message(DEBUG, "get_single is called with %s \n", p_sensor_state->name);

	/* send CMD_GET_SINGLE to sysfs control node */
	p_session_state->get_single = 1;
	send_control_cmd(0, CMD_GET_SINGLE,
			p_sensor_state->hw_sensor_id, 0, 0, 0);
}


static void load_calibration_from_file(sensor_state_t *p_sensor_state,
					struct cmd_calibration_param *param)
{
	FILE *conf;
	const char *file_name;

	if (p_sensor_state == NULL) {
		log_message(DEBUG, "%s: p_sensor_state is NULL \n", __func__);
		return;
	}

	memset(param, 0, sizeof(struct cmd_calibration_param));

	if (strncmp(p_sensor_state->name, "COMPC", SNR_NAME_MAX_LEN) == 0)
		file_name = COMPASS_CALIBRATION_FILE;
	else if (strncmp(p_sensor_state->name, "GYROC", SNR_NAME_MAX_LEN) == 0)
		file_name = GYRO_CALIBRATION_FILE;
	else
		return;

	conf = fopen(file_name, "r");

	if (!conf) {
		/* calibration file doesn't exist, creat a new file */
		log_message(DEBUG, "Open file %s failed, create a new one!\n", file_name);
		conf = fopen(file_name, "w");
		if (conf == NULL) {
			log_message(CRITICAL, "load_calibration_from_file(): failed to open file \n");
			return;
		}
		fwrite(param, sizeof(struct cmd_calibration_param), 1, conf);
		fclose(conf);

		return;
	}

	fread(param, sizeof(struct cmd_calibration_param), 1, conf);
	fclose(conf);
}

static void store_calibration_to_file(sensor_state_t *p_sensor_state,
					struct cmd_calibration_param *param)
{
	FILE *conf;
	const char *file_name;

	if (p_sensor_state == NULL) {
		log_message(DEBUG, "%s: p_sensor_state is NULL \n", __func__);
		return;
	}

	if (strncmp(p_sensor_state->name, "COMPC", SNR_NAME_MAX_LEN) == 0)
		file_name = COMPASS_CALIBRATION_FILE;
	else if (strncmp(p_sensor_state->name, "GYROC", SNR_NAME_MAX_LEN) == 0)
		file_name = GYRO_CALIBRATION_FILE;
	else
		return;

	conf = fopen(file_name, "w");

	if (!conf) {
		/* calibration file doesn't exist, creat a new file */
		log_message(DEBUG, "Can not Open file %s for writing\n", file_name);
		return;
	}

	fwrite(param, sizeof(struct cmd_calibration_param), 1, conf);
	fclose(conf);
}

static inline int check_calibration_status(sensor_state_t *p_sensor_state, int status)
{
	if (p_sensor_state == NULL)
		return 0;

	if (p_sensor_state->calibration_status & status)
		return 1;
	else
		return 0;
}

static int set_calibration_status(sensor_state_t *p_sensor_state, int status,
					struct cmd_calibration_param *param)
{
	if (p_sensor_state == NULL) {
		log_message(DEBUG, "%s: p_sensor_state is NULL \n", __func__);
		return -1;
	}

	if (strncmp(p_sensor_state->name, "COMPC", SNR_NAME_MAX_LEN) != 0 &&
		strncmp(p_sensor_state->name, "GYROC", SNR_NAME_MAX_LEN) != 0)
		return -1;

	if (status & CALIBRATION_NEED_STORE)
		p_sensor_state->calibration_status |=
						CALIBRATION_NEED_STORE;

	if (status & CALIBRATION_INIT) {
		struct cmd_calibration_param param_temp;

		/* If current status is calibrated, calibration init isn't needed*/
		if (p_sensor_state->calibration_status &
				CALIBRATION_DONE)
			return p_sensor_state->calibration_status;

		load_calibration_from_file(p_sensor_state, &param_temp);

		if (param_temp.calibrated == SUBCMD_CALIBRATION_TRUE) {
			/* Set calibration parameter to psh_fw */
			param_temp.sub_cmd = SUBCMD_CALIBRATION_SET;
			set_calibration(p_sensor_state, &param_temp);

			/* Clear 0~30bit, 31bit is for CALIBRATION_NEED_STORE,
			 * 31bit can coexist with other bits */
			p_sensor_state->calibration_status &=
							CALIBRATION_NEED_STORE;
			/* Then set the status bit */
			p_sensor_state->calibration_status |=
							CALIBRATION_DONE;
		} else {
			memset(&param_temp, 0, sizeof(struct cmd_calibration_param));
			/* clear the parameter file */
			store_calibration_to_file(p_sensor_state, &param_temp);
			/* If no calibration parameter, just set status */
			p_sensor_state->calibration_status &=
							CALIBRATION_NEED_STORE;
			p_sensor_state->calibration_status |=
							CALIBRATION_IN_PROGRESS;
		}
	} else if (status & CALIBRATION_RESET) {
		/* Start the calibration */
		if (param && param->sub_cmd == SUBCMD_CALIBRATION_START) {
			set_calibration(p_sensor_state, param);

			memset(param, 0, sizeof(struct cmd_calibration_param));
			/* clear the parameter file */
			store_calibration_to_file(p_sensor_state, param);

			p_sensor_state->calibration_status &=
							CALIBRATION_NEED_STORE;
			p_sensor_state->calibration_status |=
							CALIBRATION_IN_PROGRESS;
		}
	} else if (status & CALIBRATION_IN_PROGRESS) {
		/* CALIBRATION_IN_PROGRESS just a self-changed middle status,
		 * This status can not be set
		 * In this status, can only send SUBCMD_CALIBRATION_STOP
		 */
		if (param && param->sub_cmd == SUBCMD_CALIBRATION_STOP)
			set_calibration(p_sensor_state, param);
	} else if (status & CALIBRATION_DONE) {
		if (param && param->calibrated == SUBCMD_CALIBRATION_TRUE && param->sub_cmd == SUBCMD_CALIBRATION_SET) {
			/* Set calibration parameter to psh_fw */
			set_calibration(p_sensor_state, param);

			p_sensor_state->calibration_status &=
							CALIBRATION_NEED_STORE;
			p_sensor_state->calibration_status |=
							CALIBRATION_DONE;

			/* There is a chance to check if needs store param to file
			 * If so, then do it, and clear the CALIBRATION_NEED_STORE bit
			 */
			if (p_sensor_state->calibration_status &
				CALIBRATION_NEED_STORE) {
				store_calibration_to_file(p_sensor_state, param);

				p_sensor_state->calibration_status &=
							(~CALIBRATION_NEED_STORE);
			}
		}
	}

	return p_sensor_state->calibration_status;
}

static session_state_t* get_session_state_with_trans_id(
					unsigned char trans_id)
{
	sensor_state_t *p_sensor_state = (sensor_state_t *) get_sensor_state_with_name("EVENT");
	session_state_t *p_session_state;

	if (p_sensor_state == NULL) {
		log_message(DEBUG, "%s: p_sensor_state is NULL \n", __func__);
		return NULL;
	}

	p_session_state = p_sensor_state->list;

	while (p_session_state != NULL) {
		if (p_session_state->trans_id == trans_id)
			return p_session_state;

		p_session_state = p_session_state->next;
	}

	return NULL;
}

#define MAX_UNSIGNED_CHAR 0xff
/* trans_id starts from 1 */
unsigned char allocate_trans_id()
{
	/* rewind transaction ID */
	static unsigned char trans_id = 0;
	static int rewind = 0;

	trans_id++;

	if (rewind) {
		while ((get_session_state_with_trans_id(trans_id) != NULL) || (trans_id == 0))
		trans_id++;
	}

	if (trans_id == MAX_UNSIGNED_CHAR)
		rewind = 1;

	return trans_id;
}


static int recalculate_data_rate(sensor_state_t *p_sensor_state, int data_rate)
{
	short freq_max;

	if (p_sensor_state == NULL) {
		log_message(DEBUG, "%s: p_sensor_state is NULL \n", __func__);
		return -1;
	}

	freq_max = p_sensor_state->freq_max;

	if (freq_max == -1)
		return -1;

	return data_rate;
}

/* 1 arbiter
   2 send cmd to control node under sysfs.
     cmd format is <TRANID><CMDID><SENSORID><RATE><BUFFER DELAY>
     CMDID refer to psh_fw/include/cmd_engine.h;
     SENSORID refer to psh_fw/sensors/include/sensor_def.h */
static ret_t handle_cmd(int fd, cmd_event* p_cmd, int parameter, int parameter1,
						int parameter2,	int *reply_now)
{
	session_state_t *p_session_state;
	sensor_state_t *p_sensor_state;
	int data_rate_calculated, size;
	int ret = get_sensor_state_session_state_with_fd(fd, &p_sensor_state,
							&p_session_state);
	cmd_t cmd = p_cmd->cmd;

	*reply_now = 1;

	if (ret != 0)
		return ERR_SESSION_NOT_EXIST;

	log_message(DEBUG, "[%s] Ready to handle command %d\n", __func__, cmd);

	if (cmd == CMD_START_STREAMING) {
		data_rate_calculated = recalculate_data_rate(p_sensor_state, parameter);
		start_streaming(p_sensor_state, p_session_state,
					data_rate_calculated, parameter1, parameter2);
	} else if (cmd == CMD_STOP_STREAMING) {
		stop_streaming(p_sensor_state, p_session_state);
	} else if (cmd == CMD_GET_SINGLE) {
		get_single(p_sensor_state, p_session_state);
		*reply_now = 0;
	} else if (cmd == CMD_GET_CALIBRATION) {
		get_calibration(p_sensor_state, p_session_state);
		*reply_now = 0;
	} else if (cmd == CMD_SET_CALIBRATION) {
		char *p = (char*)p_cmd;
		struct cmd_calibration_param *cal_param;

		if (!parameter) {
			log_message(DEBUG, "ERR: This CMD need take parameter\n");
			return ERR_CMD_NOT_SUPPORT;
		}

		cal_param = (struct cmd_calibration_param*)(p + sizeof(cmd_event));

		if (cal_param->sub_cmd == SUBCMD_CALIBRATION_SET)
			set_calibration_status(p_sensor_state,
						CALIBRATION_DONE | CALIBRATION_NEED_STORE,
						cal_param);
		else if (cal_param->sub_cmd == SUBCMD_CALIBRATION_START)
			set_calibration_status(p_sensor_state,
						CALIBRATION_RESET | CALIBRATION_NEED_STORE,
						cal_param);
		else if (cal_param->sub_cmd == SUBCMD_CALIBRATION_STOP)
			set_calibration_status(p_sensor_state,
						CALIBRATION_IN_PROGRESS,
						cal_param);
	} else if (cmd == CMD_ADD_EVENT) {
		handle_add_event(p_session_state, p_cmd);
		*reply_now = 0;
	} else if (cmd == CMD_CLEAR_EVENT) {
		if (p_session_state->event_id == p_cmd->parameter)
			handle_clear_event(p_session_state);
		else
			return ERR_SESSION_NOT_EXIST;
	} else if (cmd == CMD_SET_PROPERTY) {
		send_set_property(p_sensor_state,  (property_type) p_cmd->parameter,p_cmd->parameter1, p_cmd->buf);
	}

	return SUCCESS;
}

static void handle_message(int fd, char *message)
{
	int event_type = *((int *) message);

	log_message(DEBUG, "handle_message(): fd is %d, event_type is %d\n",
							fd, event_type);

	if (event_type == EVENT_HELLO_WITH_SENSOR_TYPE) {
		hello_with_sensor_type_ack_event hello_with_sensor_type_ack;
		session_state_t *p_session_state, **next;
		hello_with_sensor_type_event *p_hello_with_sensor_type =
					(hello_with_sensor_type_event *)message;
		sensor_state_t *p_sensor_state = NULL;

		if ((p_sensor_state = get_sensor_state_with_name(p_hello_with_sensor_type->name)) == NULL) {
			hello_with_sensor_type_ack.event_type = EVENT_HELLO_WITH_SENSOR_TYPE;
			hello_with_sensor_type_ack.session_id = 0;
			send(fd, &hello_with_sensor_type_ack, sizeof(hello_with_sensor_type_ack), 0);
			log_message(CRITICAL, "handle_message(): sensor type %s not supported \n", p_hello_with_sensor_type->name);
			return;
		}
		session_id_t session_id = allocate_session_id();

		/* allocate session ID and return it to client;
		   allocate session_state and add it to sensor_list */
		hello_with_sensor_type_ack.event_type =
					EVENT_HELLO_WITH_SENSOR_TYPE_ACK;
		hello_with_sensor_type_ack.session_id = session_id;
		send(fd, &hello_with_sensor_type_ack,
			sizeof(hello_with_sensor_type_ack), 0);

		p_session_state = (session_state_t*) malloc(sizeof(session_state_t));
		if (p_session_state == NULL) {
			log_message(CRITICAL, "handle_message(): malloc failed \n");
			return;
		}
		memset(p_session_state, 0, sizeof(session_state_t));
		p_session_state->datafd = fd;
		p_session_state->session_id = session_id;

		if ((strncmp(p_sensor_state->name, "COMPS", SNR_NAME_MAX_LEN) == 0 || strncmp(p_sensor_state->name, "GYRO", SNR_NAME_MAX_LEN) == 0) &&
			p_sensor_state->list == NULL) {
			sensor_state_t *p_cal_sensor_state;
			/* In this case, this sensor is opened firstly,
			 * So calibration init is needed
			 */
			if (strncmp(p_sensor_state->name, "COMPS", SNR_NAME_MAX_LEN) == 0)
				p_cal_sensor_state = get_sensor_state_with_name("COMPC");
			else
				p_cal_sensor_state = get_sensor_state_with_name("GYROC");

			set_calibration_status(p_cal_sensor_state, CALIBRATION_INIT, NULL);
		}

		p_session_state->next = p_sensor_state->list;
		p_sensor_state->list = p_session_state;

	} else if (event_type == EVENT_HELLO_WITH_SESSION_ID) {
		hello_with_session_id_ack_event hello_with_session_id_ack;
		hello_with_session_id_event *p_hello_with_session_id =
					(hello_with_session_id_event *)message;
		session_id_t session_id = p_hello_with_session_id->session_id;
		session_state_t *p_session_state =
				get_session_state_with_session_id(session_id);

		if (p_session_state == NULL) {
			log_message(CRITICAL, "handle_message(): EVENT_HELLO_WITH_SESSION_ID, not find matching session_id \n");
			return;
		}

		p_session_state->ctlfd = fd;

		hello_with_session_id_ack.event_type =
					EVENT_HELLO_WITH_SESSION_ID_ACK;
		hello_with_session_id_ack.ret = SUCCESS;
		send(fd, &hello_with_session_id_ack,
				sizeof(hello_with_session_id_ack), 0);

	} else if (event_type == EVENT_CMD) {
		ret_t ret;
		int reply_now;
		cmd_event *p_cmd = (cmd_event *)message;
		cmd_ack_event cmd_ack;

		ret = handle_cmd(fd, p_cmd, p_cmd->parameter,
					p_cmd->parameter1, p_cmd->parameter2, &reply_now);

		if (reply_now == 0)
			return;

		cmd_ack.event_type = EVENT_CMD_ACK;
		cmd_ack.ret = ret;
		send(fd, &cmd_ack, sizeof(cmd_ack), 0);
	} else {
		/* TODO: unknown message and drop it */
	}
}

/* 1 (re)open control node and send 'reset' cmd
   2 (re)open data node
   3 (re)open Unix socket */
static void reset_sensorhub()
{
	if (sockfd != -1) {
		close(sockfd);
		sockfd = -1;
	}

	if (wakefd != -1) {
		close(wakefd);
		wakefd = -1;
	}

	if (sleepfd != -1) {
		close(sleepfd);
		sleepfd = -1;
	}


	reset_hw_layer();

	
	/* create sensorhubd Unix socket */
	sockfd = android_get_control_socket(UNIX_SOCKET_PATH);
	log_message(DEBUG,"sockFD is %d\n", sockfd);

	listen(sockfd, MAX_Q_LENGTH);

	/* open wakeup and sleep sysfs node */
	wakefd = open(WAKE_NODE, O_RDONLY, 0);
	if (wakefd == -1) {
		log_message(CRITICAL, "open %s failed, errno is %d\n",
				WAKE_NODE, errno);
		exit(EXIT_FAILURE);
	}

	sleepfd = open(SLEEP_NODE, O_RDONLY, 0);
	if (sleepfd == -1) {
		log_message(CRITICAL, "open %s failed, errno is %d\n",
			SLEEP_NODE, errno);
		exit(EXIT_FAILURE);
	}
}


static void send_data_to_clients(sensor_state_t *p_sensor_state, void *data,
						int size)
{
	session_state_t *p_session_state = p_sensor_state->list;
	char buf[MAX_MESSAGE_LENGTH];

	/* Use slide window mechanism to send data to target client,
	   buffer_delay is gauranteed, data count is not gauranteed.
	   calculate count of incoming data, send to clients by count  */
	for (; p_session_state != NULL;
		p_session_state = p_session_state->next) {
		int step, index = 0, i = 0;

		if ((p_session_state->state != ACTIVE) && (p_session_state->state != ALWAYS_ON))
			continue;

		send(p_session_state->datafd, data, size, MSG_NOSIGNAL|MSG_DONTWAIT);

	}
}

void dispatch_get_single(struct cmd_resp *p_cmd_resp)
{
	int i;
	sensor_state_t *p_sensor_state = NULL;
	session_state_t *p_session_state;
	cmd_ack_event *p_cmd_ack;

	for (i = 0; i < current_sensor_index; i ++) {
		if (sensor_list[i].hw_sensor_id == p_cmd_resp->hw_sensor_id) {
			p_sensor_state = &sensor_list[i];
			break;
		}
	}

	if (p_sensor_state == NULL) {
		log_message(CRITICAL, "dispatch_get_single(): unkown sensor id from psh fw %d \n", p_cmd_resp->hw_sensor_id);
		return;
	}

	p_session_state = p_sensor_state->list;
	for (; p_session_state != NULL;
		p_session_state = p_session_state->next) {
		if (p_session_state->get_single == 0)
			continue;
		// TODO Compass data breaks the uniform handle of get_single ...
		// FIXME hard copy , too bad ...
		if (strncmp(p_sensor_state->name, "COMPS", SNR_NAME_MAX_LEN) == 0 || strncmp(p_sensor_state->name, "GYRO", SNR_NAME_MAX_LEN) == 0) {
			sensor_state_t *p_cal_sensor_state;

			if (strncmp(p_sensor_state->name, "COMPS", SNR_NAME_MAX_LEN) == 0) {
				p_cal_sensor_state = get_sensor_state_with_name("COMPC");
			} else {
				p_cal_sensor_state = get_sensor_state_with_name("GYROC");
			}

			p_cmd_ack = (cmd_ack_event*) malloc(sizeof(cmd_ack_event) + p_cmd_resp->data_len + 2);
			if (p_cmd_ack == NULL) {
				log_message(CRITICAL, "dispatch_get_single(): malloc failed \n");
				goto fail;
			}
			p_cmd_ack->event_type = EVENT_CMD_ACK;
			p_cmd_ack->ret = SUCCESS;
			p_cmd_ack->buf_len = p_cmd_resp->data_len + 2;
			{
				short *p = (short*)(p_cmd_ack->buf + p_cmd_resp->data_len);
				*p = check_calibration_status(p_cal_sensor_state, CALIBRATION_DONE);
			}
			memcpy(p_cmd_ack->buf, p_cmd_resp->buf, p_cmd_resp->data_len + 2);

			send(p_session_state->ctlfd, p_cmd_ack, sizeof(cmd_ack_event)
						+ p_cmd_resp->data_len + 2, 0);
		} else if (strncmp(p_sensor_state->name, "BIST", SNR_NAME_MAX_LEN) == 0) {
			int i;
			p_cmd_ack = (cmd_ack_event*) malloc(sizeof(cmd_ack_event) + sizeof(struct bist_data));
			if (p_cmd_ack == NULL) {
				log_message(CRITICAL, "dispatch_get_single(): malloc failed \n");
				goto fail;
			}
			p_cmd_ack->event_type = EVENT_CMD_ACK;
			p_cmd_ack->ret = SUCCESS;
			p_cmd_ack->buf_len = sizeof(struct bist_data);

//			p_cmd_resp->buf[0] = BIST_RET_NOSUPPORT;

			for (i = 0; i < PHY_SENSOR_MAX_NUM; i++) {
				sensor_state_t *p_sensor_state = get_sensor_state_with_name(sensor_type_to_name_str[i].name);

				if (p_sensor_state == NULL)
					p_cmd_ack->buf[i] = BIST_RET_NOSUPPORT;
				else if (p_sensor_state->hw_sensor_id <= PHY_SENSOR_MAX_NUM)
					p_cmd_ack->buf[i] = p_cmd_resp->buf[p_sensor_state->hw_sensor_id];
				else
					p_cmd_ack->buf[i] = BIST_RET_NOSUPPORT;
			}

			send(p_session_state->ctlfd, p_cmd_ack, sizeof(cmd_ack_event)
						+ sizeof(struct bist_data), 0);
		} else {
			p_cmd_ack = (cmd_ack_event*) malloc(sizeof(cmd_ack_event)
						+ p_cmd_resp->data_len);
			if (p_cmd_ack == NULL) {
				log_message(CRITICAL, "dispatch_get_single(): malloc failed \n");
				goto fail;
			}
			p_cmd_ack->event_type = EVENT_CMD_ACK;
			p_cmd_ack->ret = SUCCESS;
			p_cmd_ack->buf_len = p_cmd_resp->data_len;
			memcpy(p_cmd_ack->buf, p_cmd_resp->buf, p_cmd_resp->data_len);

			send(p_session_state->ctlfd, p_cmd_ack, sizeof(cmd_ack_event)
						+ p_cmd_resp->data_len, 0);

		}
			free(p_cmd_ack);
fail:
			p_session_state->get_single = 0;
	}
}

void dispatch_streaming(struct cmd_resp *p_cmd_resp)
{
	psh_sensor_t sensor_type;
	unsigned char hw_sensor_id = p_cmd_resp->hw_sensor_id;
	sensor_state_t *p_sensor_state = get_sensor_state_with_id(hw_sensor_id);

	if (p_sensor_state == NULL) {
		log_message(CRITICAL, "%s: p_sensor_state is NULL \n", __func__);
		return;
	}

	if ((strncmp(p_sensor_state->name, "GYRO", SNR_NAME_MAX_LEN) == 0)
		|| (strncmp(p_sensor_state->name, "GYRO1", SNR_NAME_MAX_LEN) == 0)) {

		log_message(WARNING, "dispatch_streaming: in Gyro data \n");
		sensor_state_t *p_cal_sensor_state;
		struct gyro_raw_data *p_gyro_raw_data;
		struct gyro_raw_data *p_fake
			= (struct gyro_raw_data *)p_cmd_resp->buf;
		int i, len;

		len = p_cmd_resp->data_len / (sizeof(struct gyro_raw_data) - 2);
		p_gyro_raw_data = (struct gyro_raw_data *)malloc(len * sizeof(struct gyro_raw_data));
		if (p_gyro_raw_data == NULL)
			goto fail;
		log_message(WARNING, "dispatch_streaming: Gyro len is %d \n", len);

		p_cal_sensor_state = get_sensor_state_with_name("GYROC");
		for (i = 0; i < len; ++i) {
			p_gyro_raw_data[i].x = p_fake[i].x;
			p_gyro_raw_data[i].y = p_fake[i].y;
			p_gyro_raw_data[i].z = p_fake[i].z;
			p_gyro_raw_data[i].accuracy =
				check_calibration_status(p_cal_sensor_state,
							CALIBRATION_DONE);
		}
		log_message(WARNING, "dispatch_streaming: Gyro sending to clients \n", len);
		send_data_to_clients(p_sensor_state, p_gyro_raw_data,
					len*sizeof(struct gyro_raw_data));
		free(p_gyro_raw_data);
	} else if ((strncmp(p_sensor_state->name, "COMPS", SNR_NAME_MAX_LEN) == 0)
		|| (strncmp(p_sensor_state->name, "COMP1", SNR_NAME_MAX_LEN) == 0)) {
		sensor_state_t *p_cal_sensor_state;
		struct compass_raw_data *p_compass_raw_data;
		struct compass_raw_data *p_fake = (struct compass_raw_data *)p_cmd_resp->buf;
		int i, len;

		len = p_cmd_resp->data_len / (sizeof(struct compass_raw_data) - 2);
		p_compass_raw_data = (struct compass_raw_data*) malloc(len * sizeof(struct compass_raw_data));
		if(p_compass_raw_data == NULL)
			goto fail;

		p_cal_sensor_state = get_sensor_state_with_name("COMPC");
		for (i = 0; i < len; ++i){
			p_compass_raw_data[i].x = p_fake[i].x;
			p_compass_raw_data[i].y = p_fake[i].y;
			p_compass_raw_data[i].z = p_fake[i].z;
			p_compass_raw_data[i].accuracy =
				check_calibration_status(p_cal_sensor_state,
							CALIBRATION_DONE);
		}
		//TODO What to do?
		send_data_to_clients(p_sensor_state, p_compass_raw_data,
					len*sizeof(struct compass_raw_data)); // Unggg ...
		free(p_compass_raw_data);
        } else {
		send_data_to_clients(p_sensor_state, p_cmd_resp->buf, p_cmd_resp->data_len);
	}

	return;
fail:
	log_message(CRITICAL, "failed to allocate memory \n");
}

void handle_calibration(struct cmd_calibration_param * param)
{
	sensor_state_t *p_sensor_state;
	session_state_t *p_session_state;
	cmd_ack_event *p_cmd_ack;
	psh_sensor_t sensor_type = param->sensor_type;

	p_sensor_state = get_sensor_state_with_name(sensor_type_to_name_str[sensor_type].name);
	if (p_sensor_state == NULL) {
		log_message(CRITICAL, "%s: p_sensor_state is NULL \n", __func__);
		return;
	}
	p_session_state = p_sensor_state->list;

	log_message(DEBUG, "Subcmd:%d.  Calibrated? %u\n", param->sub_cmd, param->calibrated);

	for (; p_session_state != NULL;
			p_session_state = p_session_state->next){
		struct cmd_calibration_param * p_cal_info;

		if (p_session_state->get_calibration == 0)
			continue;

		p_cmd_ack = (cmd_ack_event*) malloc(sizeof(cmd_ack_event) + sizeof(struct cmd_calibration_param));
		if (p_cmd_ack == NULL) {
			log_message(CRITICAL, "failed to allocate memory \n");
			goto fail;
		}
		p_cmd_ack->event_type = EVENT_CMD_ACK;
		p_cmd_ack->ret = SUCCESS;
		p_cmd_ack->buf_len = sizeof(struct cmd_calibration_param);

		p_cal_info = (struct cmd_calibration_param *)p_cmd_ack->buf;
		*p_cal_info = *param;

		send(p_session_state->ctlfd, p_cmd_ack,
				sizeof(cmd_ack_event) + sizeof(struct cmd_calibration_param), 0);

		free(p_cmd_ack);
fail:
		p_session_state->get_calibration = 0;
	}

	if (param->calibrated == SUBCMD_CALIBRATION_TRUE) {
		param->sub_cmd = SUBCMD_CALIBRATION_SET;
		set_calibration_status(p_sensor_state, CALIBRATION_DONE, param);
	} else {
		log_message(DEBUG, "Get calibration message, but calibrated not done.\n");
	}
}

/* use trans_id to match client */
void handle_add_event_resp(struct cmd_resp *p_cmd_resp)
{
	unsigned char trans_id = p_cmd_resp->tran_id;
	sensor_state_t *p_sensor_state = get_sensor_state_with_name("EVENT");
	session_state_t *p_session_state;
	cmd_ack_event *p_cmd_ack;

	if (p_sensor_state == NULL) {
		log_message(CRITICAL, "%s: p_sensor_state is NULL \n", __func__);
		return;
	}

	p_session_state = p_sensor_state->list;

	/* trans_id starts from 1, 0 means no trans_id */
	for (; p_session_state != NULL;
		p_session_state = p_session_state->next) {
		if (trans_id == p_session_state->trans_id) {
			p_cmd_ack = (cmd_ack_event *) malloc(sizeof(cmd_ack_event)
					+ p_cmd_resp->data_len);
			if (p_cmd_ack == NULL) {
				log_message(CRITICAL, "handle_add_event_resp(): malloc failed \n");
				return;
			}

			p_cmd_ack->event_type = EVENT_CMD_ACK;
			p_cmd_ack->ret = SUCCESS;
			p_cmd_ack->buf_len = p_cmd_resp->data_len;
			memcpy(p_cmd_ack->buf, p_cmd_resp->buf, p_cmd_resp->data_len);
			send(p_session_state->ctlfd, p_cmd_ack, sizeof(cmd_ack_event)
				+ p_cmd_resp->data_len, 0);

			log_message(DEBUG, "event id is %d \n", p_cmd_resp->buf[0]);
			free(p_cmd_ack);

			p_session_state->event_id = p_cmd_resp->buf[0];
//			p_session_state->trans_id = 0;
			break;
		}
	}
}

/* use event_id to match client */
void handle_clear_event_resp(struct cmd_resp *p_cmd_resp)
{

}

/* use event_id to match client */
void dispatch_event(struct cmd_resp *p_cmd_resp)
{
	unsigned char event_id;
	sensor_state_t *p_sensor_state = get_sensor_state_with_name("EVENT");
	session_state_t *p_session_state;

	if (p_sensor_state == NULL) {
		log_message(CRITICAL, "%s: p_sensor_state is NULL \n", __func__);
		return;
	}

	p_session_state = p_sensor_state->list;

	if (p_cmd_resp->data_len != 0) {
		event_id = p_cmd_resp->buf[0];
	} else
		return;

	for (; p_session_state != NULL;
		p_session_state = p_session_state->next) {
		if (event_id == p_session_state->event_id) {
//			write(p_session_state->datafd, p_cmd_resp->buf, p_cmd_resp->data_len);
			send(p_session_state->datafd, p_cmd_resp->buf, p_cmd_resp->data_len, MSG_NOSIGNAL);
			break;
		}
	}
}

static void remove_session_by_fd(int fd)
{
	int i = 0;

	for (i = 0; i < current_sensor_index; i ++) {
		session_state_t *p, *p_session_state = sensor_list[i].list;

		p = p_session_state;

		for (; p_session_state != NULL;
			p_session_state = p_session_state->next) {
			if ((p_session_state->ctlfd != fd)
				&& (p_session_state->datafd != fd)) {
				p = p_session_state;
				continue;
			}
//			close(p_session_state->ctlfd);
//			close(p_session_state->datafd);

			if (strncmp(sensor_list[i].name, "EVENT", SNR_NAME_MAX_LEN) == 0) {
			/* special treatment for SENSOR_EVENT */
				handle_clear_event(p_session_state);
			} else if (i == SENSOR_CALIBRATION_COMP ||
					i == SENSOR_CALIBRATION_GYRO) {

			} else {

				stop_streaming(&sensor_list[i], p_session_state);
			}
			if (p_session_state == sensor_list[i].list)
				sensor_list[i].list = p_session_state->next;
			else
				p->next = p_session_state->next;

			if (strncmp(sensor_list[i].name, "COMPS", SNR_NAME_MAX_LEN) == 0 || strncmp(sensor_list[i].name, "GYRO", SNR_NAME_MAX_LEN) == 0) {
				sensor_state_t *p_sensor_state;
				/* In this case, this session is closed,
				 * So calibration parameter is needed to store to file
				 */
				if (strncmp(sensor_list[i].name, "COMPS", SNR_NAME_MAX_LEN) == 0) {
					p_sensor_state = get_sensor_state_with_name("COMPC");
				} else {
					p_sensor_state = get_sensor_state_with_name("GYROC");
				}

				/* Set the CALIBRATION_NEED_STORE bit,
				 * parameter will be stored when get_calibration() response arrives
				 */
				set_calibration_status(p_sensor_state, CALIBRATION_NEED_STORE, NULL);
				get_calibration(p_sensor_state, NULL);
			}

			log_message(DEBUG, "session with datafd %d, ctlfd %d "
				"is removed \n", p_session_state->datafd,
				p_session_state->ctlfd);
			free(p_session_state);
			return;
		}
        }
}



static void *screen_state_thread(void *cookie)
{
	int fd, err;
	char buf;
	pthread_mutex_t update_mutex = PTHREAD_MUTEX_INITIALIZER;

	while(1) {
		fd = open(SLEEP_NODE, O_RDONLY, 0);
		do {
			err = read(fd, &buf, 1);
			log_message(DEBUG,"wait for sleep err: %d, errno: %d\n", err, errno);
		} while (err < 0 && errno == EINTR);

		pthread_mutex_lock(&update_mutex);
		screen_state = 0;
		pthread_mutex_unlock(&update_mutex);

		write(notifyfd, "0", 1);

		close(fd);

		fd = open(WAKE_NODE, O_RDONLY, 0);
		do {
			err = read(fd, &buf, 1);
			log_message(DEBUG,"wait for wake err: %d, errno: %d\n", err, errno);
		} while (err < 0 && errno == EINTR);

		pthread_mutex_lock(&update_mutex);
		screen_state = 1;
		pthread_mutex_unlock(&update_mutex);

		write(notifyfd, "0", 1);

		close(fd);
	}
	return NULL;
}

#if 0
void handle_screen_state_change(void)
{
	int i;
	session_state_t *p;

	if (screen_state == 0) {
		for (i = 0; i < SENSOR_BIST; i++) {
			/* calibration is not stopped */
			if (i == SENSOR_CALIBRATION_COMP ||
				i == SENSOR_CALIBRATION_GYRO)
				continue;

			p = sensor_list[i].list;

			while (p != NULL) {
				if ((p->state == ACTIVE)) {
					stop_streaming(i, p);
					p->state = NEED_RESUME;
				}
				p = p->next;
			}
		}
	} else if (screen_state == 1) {
		for (i = 0; i < SENSOR_BIST; i++) {
			/* calibration is not stopped */
			if (i == SENSOR_CALIBRATION_COMP ||
				i == SENSOR_CALIBRATION_GYRO)
				continue;

			p = sensor_list[i].list;

			while (p != NULL) {
				if (p->state == NEED_RESUME) {
					start_streaming(i, p, p->data_rate, p->buffer_delay, 0);
				}
				p = p->next;
			}
		}
	}
}
#endif

static void handle_no_stop_no_report()
{
	int i;
	int value;
	session_state_t *p;

	for (i = 0; i < current_sensor_index; i ++) {
		if (strncmp(sensor_list[i].name, "COMPC", SNR_NAME_MAX_LEN) == 0 || strncmp(sensor_list[i].name, "GYROC", SNR_NAME_MAX_LEN) == 0)
			continue;

		p = sensor_list[i].list;

		while (p != NULL) {
			if ((p->state == ALWAYS_ON) && (p->flag == 1)) {
				//TODO: send set_property to psh fw
				if (i == SENSOR_PEDOMETER) {
					if (screen_state == 0) {
						value = 1;
						send_set_property(&sensor_list[i], PROP_STOP_REPORTING, 4, (unsigned char *)&value);
					} else {
						value = 0;
						send_set_property(&sensor_list[i], PROP_STOP_REPORTING, 4, (unsigned char *)&value);
					}
				}
			}
			p = p->next;
		}

	}
}


int FillFixedReadDescriptors(fd_set *fds, int *notifyfds)
{
		
	int ret;
	
	FD_SET(sockfd, fds);
	int maxfd = sockfd;

	/* add notifyfds[0] to listen_fds */
	ret = pipe(notifyfds);
	if (ret < 0) {
		log_message(CRITICAL, "sensorhubd failed to create pipe \n");
		exit(EXIT_FAILURE);
	}
	log_message(DEBUG,"sleep-wake fd is :%d\n", notifyfds[0]);

	notifyfd = notifyfds[1];

	FD_SET(notifyfds[0],fds);

	if (notifyfds[0] > maxfd)
		maxfd = notifyfds[0];

	return maxfd;
}

/* 1 create data thread
   2 wait and handle the request from client in main thread */
static void start_sensorhubd()
{
	fd_set listen_fds, read_fds;
	int notifyfds[2];	

	pthread_t t;

	/* two fd_set for I/O multiplexing */
	FD_ZERO(&listen_fds);
	FD_ZERO(&read_fds);

	int fixedMaxfd = FillFixedReadDescriptors(&listen_fds, notifyfds);

	pthread_create(&t, NULL, screen_state_thread, NULL);
	
	log_message(DEBUG,"after sleep thread launch\n");

	acquire_wake_lock(PARTIAL_WAKE_LOCK, WAKE_LOCK_ID);

	bool toException;

	fd_set exception_fds;
	FD_ZERO(&exception_fds);
	int printCnt = 10;
	while (1) {
		int hw_fds[255], hw_fds_num = 0;
		read_fds = listen_fds;
		int maxfd = fixedMaxfd;
		maxfd = FillHWDescriptors(maxfd, &read_fds, &exception_fds, hw_fds, &hw_fds_num, &toException);
		if(printCnt > 0)
		{
			printCnt--;
			log_message(DEBUG,"got %d data sockets first fd:%d\n", hw_fds_num, hw_fds[0]);
		}
		/* error handling of select() */
		//log_message(DEBUG,"before first select\n");

		if (select(maxfd + 1, &read_fds, NULL, toException? &exception_fds : NULL, NULL)
								== -1) {
			if (errno == EINTR)
				continue;
			else {
				log_message(CRITICAL, "sensorhubd socket "
					"select() failed. errno "
					"is %d\n", errno);
				release_wake_lock(WAKE_LOCK_ID);
				exit(EXIT_FAILURE);
			}
		}

		/* handle events from hw */
		int k;
		for (k = 0; k < hw_fds_num; k++)
		{
			if ((toException && FD_ISSET(hw_fds[k], &exception_fds)) ||
				(!toException && FD_ISSET(hw_fds[k], &read_fds)))
			{
				log_message(DEBUG,"data woke select fd:%d\n", hw_fds[k]);
				if (!toException) 
					FD_CLR(hw_fds[k], &read_fds);
				ProcessHW(hw_fds[k]);
			}
		}
		
		/* handle new connection request */
		if (FD_ISSET(sockfd, &read_fds)) {
			log_message(DEBUG,"new conn woke select\n");
			struct sockaddr_un client_addr;
			socklen_t addrlen = sizeof(client_addr);
			int clientfd = accept(sockfd,
					(struct sockaddr *) &client_addr,
					&addrlen);
			if (clientfd == -1) {
				log_message(CRITICAL, "sensorhubd socket "
					"accept() failed.\n");
				release_wake_lock(WAKE_LOCK_ID);
				exit(EXIT_FAILURE);
			} else {
				log_message(DEBUG, "new connection from client adding FD = %d\n", clientfd);
				FD_SET(clientfd, &listen_fds);
				if (clientfd > maxfd)
					fixedMaxfd = clientfd;
			}

			FD_CLR(sockfd, &read_fds);
		}


		/* handle wake/sleep notification, begin */
		if (FD_ISSET(notifyfds[0], &read_fds)) {
			log_message(DEBUG,"wake sleep woke select\n");
			char buf[20];
			int ret;
			ret = read(notifyfds[0], buf, 20);
			if (ret > 0)
				log_message(DEBUG, "Get notification,buf is %s, screen state is %d \n", buf, screen_state);

			// make sure sensor start/stop is not interrupted by S3 suspend
			if (screen_state == 1)
				acquire_wake_lock(PARTIAL_WAKE_LOCK, WAKE_LOCK_ID);

//			handle_screen_state_change();
			handle_no_stop_no_report();

			if (screen_state == 0)
				release_wake_lock(WAKE_LOCK_ID);
			
			FD_CLR(notifyfds[0], &read_fds);

		}

		/* handle wake/sleep notification, end */

		/* handle request from clients */
		int i;
		for (i = maxfd; i >= 0; i--) {
			char message[MAX_MESSAGE_LENGTH];

			if (!FD_ISSET(i, &read_fds))
				continue;
			//log_message(DEBUG,"cleint woke select\n");

			int length = recv(i, message, MAX_MESSAGE_LENGTH, 0);
			if (length <= 0) {
				/* release session resource if necessary */
				remove_session_by_fd(i);
				close(i);
				log_message(WARNING, "fd %d:error reading message \n", i);
				FD_CLR(i, &listen_fds);
			} else {
				/* process message */
				handle_message(i, message);
			}
			FD_CLR(i, &read_fds);
		}

	}
}


static void usage()
{
	printf("Usage: sensorhubd [OPTION...] \n");
	printf("  -d, --enable-data-rate-debug      1: enable; 0: disable (default) \n");
	printf("  -l, --log-level        0-2, 2 is most verbose level \n");
	printf("  -h, --help             show this help message \n");

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int log_level = CRITICAL;

	if (is_first_instance() != 1)
		exit(0);

	signal(SIGUSR1, start_callback);
	signal(SIGUSR2, stop_callback);
	set_log_file(log_file);
	while (1) {
		static struct option opts[] = {
			{"log-level", 2, NULL, 'l'},
			{"help", 0, NULL, 'h'},
			{0, 0, NULL, 0}
		};

		int index, o;

		o = getopt_long(argc, argv, "d:l::h", opts, &index);
		if (o == -1)
			break;

		switch (o) {
		case 'l':
			if (optarg != NULL)
				log_level = strtod(optarg, NULL);
			log_message(DEBUG, "log_level is %d \n", log_level);
			set_log_level((message_level) log_level);
			break;
		case 'h':
			usage();
			break;
		default:
			usage();
		}
	}

	sensor_list = (sensor_state_t*) malloc((MAX_SENSOR_INDEX + 1) * sizeof(sensor_state_t));
	if (sensor_list == NULL)
		exit(EXIT_FAILURE);
	memset(sensor_list, 0, (MAX_SENSOR_INDEX + 1) * sizeof(sensor_state_t));

	while (1) {
		reset_sensorhub();
		start_sensorhubd();
	}
	return 0;
}
