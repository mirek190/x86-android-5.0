#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <assert.h>
#include <cutils/sockets.h>
#include <pthread.h>

#include "../include/message.h"
#include "../include/socket.h"
#include "../include/utils.h"
#include "../include/bist.h"

#undef LOG_TAG
#define LOG_TAG "LibsensorhubClient"

typedef struct {
	int datafd;
	int ctlfd;
	session_id_t session_id;
	char name[SNR_NAME_MAX_LEN + 1];
	unsigned char evt_id;
	struct cmd_event_param *evt_param;
        pthread_mutex_t ctlfd_lock;
} session_context_t;

static ssize_t psh_send_recv_cmd_locked(int sockfd, void *buf_send, void *buf_recv,
                                        size_t len_send, size_t len_recv, pthread_mutex_t *lock)
{
	int err, ret, size = 0;
	int event_type;
	struct timeval timeout = {5, 0};
	cmd_ack_event *p_cmd_ack;

	err = pthread_mutex_lock(lock);
	if (err)
		ALOGE("%s: Cannot lock ctlfd! error: %d", __FUNCTION__, err);

	ret = send(sockfd, buf_send, len_send, 0);
	if (ret <= 0) {
		ret = ERROR_MESSAGE_NOT_SENT;
		goto err_send;
	}

	/* set receive timeout to 5 sec. */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

	ret = recv(sockfd, buf_recv, len_recv, 0);
	if (ret <= 0) {
		ret = ERROR_CAN_NOT_GET_REPLY;
		goto err_recv;
	}

        size = ret;

	event_type = *((int *)buf_recv);
	if (event_type != EVENT_CMD_ACK) {
		ret = ERROR_CAN_NOT_GET_REPLY;
		goto err_recv;
	}

	p_cmd_ack = (cmd_ack_event *)buf_recv;

        size -= sizeof(cmd_ack_event) + p_cmd_ack->buf_len;

	if (p_cmd_ack->ret == E_ANOTHER_REPLY) {
                if (size > 0) {
                        memcpy(buf_recv, buf_recv + sizeof(cmd_ack_event) + p_cmd_ack->buf_len, size);
                } else {
                        ret = recv(sockfd, buf_recv, len_recv, 0);
                        if (ret <= 0) {
                                ret = ERROR_CAN_NOT_GET_REPLY;
                                goto err_recv;
                        }
                }
	}

	ret = 0;

err_send:
err_recv:
        err = pthread_mutex_unlock(lock);
        if (err)
                ALOGE("%s: Cannot unlock ctlfd! error: %d", __FUNCTION__, err);

        return ret;

}

handle_t psh_open_session_with_name(const char *name)
{
	int datafd, len, ret, event_type, ctlfd;
	char message[MAX_MESSAGE_LENGTH];
	hello_with_sensor_type_event hello_with_sensor_type;
	hello_with_sensor_type_ack_event *p_hello_with_sensor_type_ack;
	hello_with_session_id_event hello_with_session_id;
	hello_with_session_id_ack_event *p_hello_with_session_id_ack;
	session_id_t session_id;
	struct cmd_event_param *evt_param = NULL;
	static int retry = 0;

	if (name == NULL)
		return NULL;

	len = strlen(name);
	if (len > SNR_NAME_MAX_LEN)
		return NULL;

	/* set up data connection */
	datafd = socket_local_client(UNIX_SOCKET_PATH, ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);
	if (datafd < 0)	{
		while (retry < 10) {
			usleep(500000);
			datafd = socket_local_client(UNIX_SOCKET_PATH, ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);
			if (datafd >= 0)
				break;
			retry ++;
		}
	}

	if (datafd < 0) {
		ALOGE("socket_local_client() failed\n");
		return NULL;
	}

	/* send  EVENT_HELLO_WITH_SENSOR_TYPE and get session_id in ACK */
	hello_with_sensor_type.event_type = EVENT_HELLO_WITH_SENSOR_TYPE;
	memcpy(hello_with_sensor_type.name, name, len);
	hello_with_sensor_type.name[len] = '\0';
	ret = send(datafd, &hello_with_sensor_type, sizeof(hello_with_sensor_type), 0);
	if (ret < 0) {
		close(datafd);
		ALOGE("write EVENT_HELLO_WITH_SENSOR_TYPE "
						"failed \n");
		return NULL;
	}

	ret = recv(datafd, message, MAX_MESSAGE_LENGTH, 0);
	if (ret < 0) {
		close(datafd);
		ALOGE("read EVENT_HELLO_WITH_SENSOR_TYPE_ACK failed \n");
		return NULL;
	}

	event_type = *((int *) message);
	if (event_type != EVENT_HELLO_WITH_SENSOR_TYPE_ACK) {
		close(datafd);
		ALOGE("not get expected EVENT_HELLO_WITH_SENSOR_TYPE_ACK \n");
		return NULL;
	}

	p_hello_with_sensor_type_ack = (hello_with_sensor_type_ack_event *) message;
	session_id = p_hello_with_sensor_type_ack->session_id;

	/* set up control connection */
	ctlfd = socket_local_client("sensorhubd", ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);
	if (ctlfd < 0) {
		close(datafd);
		ALOGE("socket_local_client() failed\n");
		return NULL;
	}

	/* send EVENT_HELLO_WITH_SESSION_ID and get ACK */
	hello_with_session_id.event_type = EVENT_HELLO_WITH_SESSION_ID;
	hello_with_session_id.session_id = session_id;
	ret = send(ctlfd, &hello_with_session_id, sizeof(hello_with_session_id), 0);
	if (ret < 0) {
		close(datafd);
		close(ctlfd);
		ALOGE("write EVENT_HELLO_WITH_SESSION_ID failed \n");
		return NULL;
	}

	ret = recv(ctlfd, message, MAX_MESSAGE_LENGTH, 0);
	if (ret < 0) {
		close(datafd);
		close(ctlfd);
		ALOGE("read EVENT_HELLO_WITH_SESSION_ID_ACK failed \n");
		return NULL;
	}

	event_type = *((int *) message);
	if (event_type != EVENT_HELLO_WITH_SESSION_ID_ACK) {
		close(datafd);
		close(ctlfd);
		ALOGE("not get expected EVENT_HELLO_WITH_SESSION_ID_ACK \n");
		return NULL;
	}

	p_hello_with_session_id_ack = (hello_with_session_id_ack_event *) message;
	ret = p_hello_with_session_id_ack->ret;
	if (ret != SUCCESS) {
		close(datafd);
		close(ctlfd);
		ALOGE("failed: EVENT_HELLO_WITH_SESSION_ID_ACK returned %d \n", ret);
		return NULL;
	}

	if (strncmp(name, "EVENT", SNR_NAME_MAX_LEN) == 0) {
		evt_param = malloc(sizeof(struct cmd_event_param));
		if (evt_param == NULL) {
			close(datafd);
			close(ctlfd);
			ALOGE("failed to allocate memory for cmd_event_param");
			return NULL;
		}

		evt_param->num = 0;
		evt_param->relation = 0;
	}

	session_context_t *session_context = malloc(sizeof(session_context_t));
	if (session_context == NULL) {
		close(datafd);
		close(ctlfd);
		free(evt_param);
		ALOGE("failed to allocate memory for session_context \n");
		return NULL;
	}

	session_context->datafd = datafd;
	session_context->ctlfd = ctlfd;
	session_context->session_id = session_id;
	memcpy(session_context->name, name, len);
	hello_with_sensor_type.name[len] = '\0';
	session_context->evt_id = 0;
	session_context->evt_param = evt_param;

	pthread_mutex_init(&session_context->ctlfd_lock, NULL);

	return session_context;
}

/* 1 set up data connection
   2 set up control connection */
handle_t psh_open_session(psh_sensor_t sensor_type)
{
	if (sensor_type >= SENSOR_MAX)
		return NULL;

	return psh_open_session_with_name(sensor_type_to_name_str[sensor_type].name);
}

void psh_close_session(handle_t handle)
{
	session_context_t *session_context = (session_context_t *)handle;

	if (session_context == NULL)
		return;

	close(session_context->datafd);
	close(session_context->ctlfd);

        pthread_mutex_destroy(&session_context->ctlfd_lock);

	if (session_context->evt_param != NULL)
		free(session_context->evt_param);
	free(session_context);
}

error_t psh_start_streaming(handle_t handle, int data_rate, int buffer_delay)
{

	return psh_start_streaming_with_flag(handle, data_rate, buffer_delay, 0);
}

/* flag: 2 means no_stop_no_report when IA sleep; 1 means no_stop when IA sleep; 0 means stop when IA sleep */
error_t psh_start_streaming_with_flag(handle_t handle, int data_rate, int buffer_delay, streaming_flag flag)
{
	session_context_t *session_context = (session_context_t *)handle;
	cmd_event cmd;
	int ret, event_type;
	char message[MAX_MESSAGE_LENGTH];
	cmd_ack_event *p_cmd_ack;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	if (data_rate == 0)
		return ERROR_NOT_AVAILABLE;

	if ((flag != 0) && (flag != 1) && (flag != 2))
		return ERROR_WRONG_PARAMETER;

	cmd.event_type = EVENT_CMD;
	cmd.cmd = CMD_START_STREAMING;
	cmd.parameter = data_rate;
	cmd.parameter1 = buffer_delay;
	cmd.parameter2 = flag;

	ret = psh_send_recv_cmd_locked(session_context->ctlfd, &cmd, message,
                                       sizeof(cmd), MAX_MESSAGE_LENGTH, &session_context->ctlfd_lock);
	if (ret)
		return ret;

	event_type = *((int *) message);
	if (event_type != EVENT_CMD_ACK)
		return ERROR_CAN_NOT_GET_REPLY;

	p_cmd_ack = (cmd_ack_event *)message;

	if (p_cmd_ack->ret != E_SUCCESS)
		return ERROR_DATA_RATE_NOT_SUPPORTED;

	return ERROR_NONE;
}

error_t psh_flush_streaming(handle_t handle, unsigned int data_unit_size)
{
	session_context_t *session_context = (session_context_t *)handle;
	cmd_event cmd;
	int ret, event_type;
	char message[MAX_MESSAGE_LENGTH];
	cmd_ack_event *p_cmd_ack;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	if (data_unit_size > MAX_UNIT_SIZE)
		return ERROR_NOT_AVAILABLE;

	cmd.event_type = EVENT_CMD;
	cmd.cmd = CMD_FLUSH_STREAMING;
	cmd.parameter = data_unit_size;

	ret = psh_send_recv_cmd_locked(session_context->ctlfd, &cmd, message,
                                       sizeof(cmd), MAX_MESSAGE_LENGTH, &session_context->ctlfd_lock);
	if (ret)
		return ret;

	event_type = *((int *) message);
	if (event_type != EVENT_CMD_ACK)
		return ERROR_CAN_NOT_GET_REPLY;

	p_cmd_ack = (cmd_ack_event *)message;
	if (p_cmd_ack->ret != E_SUCCESS)
		return ERROR_DATA_RATE_NOT_SUPPORTED;

	return ERROR_NONE;
}

error_t psh_stop_streaming(handle_t handle)
{
	session_context_t *session_context = (session_context_t *)handle;
	cmd_event cmd;
	int ret, event_type;
	char message[MAX_MESSAGE_LENGTH];
	cmd_ack_event *p_cmd_ack;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	cmd.event_type = EVENT_CMD;
	cmd.cmd = CMD_STOP_STREAMING;

	ret = psh_send_recv_cmd_locked(session_context->ctlfd, &cmd, message,
                                       sizeof(cmd), MAX_MESSAGE_LENGTH, &session_context->ctlfd_lock);
	if (ret)
		return ret;

	event_type = *((int *) message);
	if (event_type != EVENT_CMD_ACK)
		return ERROR_CAN_NOT_GET_REPLY;

	p_cmd_ack = (cmd_ack_event *)message;

	if (p_cmd_ack->ret != E_SUCCESS)
		return ERROR_NOT_AVAILABLE;

	return ERROR_NONE;
}

error_t psh_get_single(handle_t handle, void *buf, int buf_size)
{
	session_context_t *session_context = (session_context_t *)handle;
	cmd_event cmd;
	int ret, event_type, len;
	char message[MAX_MESSAGE_LENGTH];
	cmd_ack_event *p_cmd_ack;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	cmd.event_type = EVENT_CMD;
	cmd.cmd = CMD_GET_SINGLE;

	ret = psh_send_recv_cmd_locked(session_context->ctlfd, &cmd, message,
                                       sizeof(cmd), MAX_MESSAGE_LENGTH, &session_context->ctlfd_lock);
	if (ret)
		return ret;

	event_type = *((int *) message);
	if (event_type != EVENT_CMD_ACK)
		return ERROR_CAN_NOT_GET_REPLY;

	p_cmd_ack = (cmd_ack_event *)message;

	if (p_cmd_ack->ret != E_SUCCESS)
		return ERROR_NOT_AVAILABLE;

	if (p_cmd_ack->buf_len < buf_size)
		len = p_cmd_ack->buf_len;
	else
		len = buf_size;
	memcpy(buf, p_cmd_ack->buf, len);

	return len;
}

int psh_get_fd(handle_t handle)
{
	session_context_t *session_context = (session_context_t *)handle;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	return session_context->datafd;
}

error_t psh_set_calibration(handle_t handle,
			        struct cmd_calibration_param * param)
{
	session_context_t *session_context = (session_context_t *)handle;
	int ret, event_type,  cmd_event_len;
	char message[MAX_MESSAGE_LENGTH];
	cmd_ack_event *p_cmd_ack;
	struct cmd_cal_info {
		cmd_event cmd;
		struct cmd_calibration_param param;
	} __attribute__ ((packed)) cmd_cal;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	if (param == NULL)
		return ERROR_WRONG_PARAMETER;

	cmd_event_len = sizeof(cmd_event) + sizeof(*param);

	cmd_cal.cmd.event_type = EVENT_CMD;
	cmd_cal.cmd.cmd = CMD_SET_CALIBRATION;
	cmd_cal.param = *param;
	/* Why need this? */
	/* Because handle_cmd only sees cmd->parameter{0,1} ...  */
	cmd_cal.cmd.parameter = 1;

	ret = psh_send_recv_cmd_locked(session_context->ctlfd, &cmd_cal, message,
                                       cmd_event_len, MAX_MESSAGE_LENGTH, &session_context->ctlfd_lock);
	if (ret)
		return ret;

	event_type = *((int *) message);
	if (event_type != EVENT_CMD_ACK)
		return ERROR_CAN_NOT_GET_REPLY;

	p_cmd_ack = (cmd_ack_event *)message;

	if (p_cmd_ack->ret != E_SUCCESS)
		return ERROR_NOT_AVAILABLE;

	/*	No non-status return message ...
	 *
	 *	if (p_cmd_ack->buf_len < buf_size)
	 *		len = p_cmd_ack->buf_len;
	 *	else
	 *		len = buf_size;
	 *	memcpy(buf, p_cmd_ack->buf, len);
	 */

	return ERROR_NONE;
}

error_t psh_get_calibration(handle_t handle,
				struct cmd_calibration_param * param){

	session_context_t *session_context = (session_context_t *)handle;
	int ret, event_type,  cmd_event_len;
	char message[MAX_MESSAGE_LENGTH];
	cmd_ack_event *p_cmd_ack;
	cmd_event req;

	log_message(DEBUG, "***********DEL-%s:%d", __func__, __LINE__);
	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	req.event_type = EVENT_CMD;
	req.cmd = CMD_GET_CALIBRATION;

	ret = psh_send_recv_cmd_locked(session_context->ctlfd, &req, message,
                                       sizeof req, MAX_MESSAGE_LENGTH, &session_context->ctlfd_lock);
	if (ret)
		return ret;

	event_type = *((int *) message);
	if (event_type != EVENT_CMD_ACK)
		return ERROR_CAN_NOT_GET_REPLY;

	p_cmd_ack = (cmd_ack_event *)message;

	if (p_cmd_ack->ret != E_SUCCESS)
		return ERROR_NOT_AVAILABLE;

	assert(p_cmd_ack->buf_len == sizeof(*param));

	*param = *(struct cmd_calibration_param*)p_cmd_ack->buf;
	return ERROR_NONE;
}

/* relation: 0, AND; 1, OR */
error_t psh_event_set_relation(handle_t handle, relation relation)
{
	session_context_t *session_context = (session_context_t *)handle;

	if ((relation != 0) && (relation != 1))
		return ERROR_WRONG_PARAMETER;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	if (strncmp(session_context->name, "EVENT", SNR_NAME_MAX_LEN) != 0)
		return ERROR_WRONG_ACTION_ON_SENSOR_TYPE;

	session_context->evt_param->relation = relation;

	return ERROR_NONE;
}

error_t psh_event_append(handle_t handle, struct sub_event *sub_evt)
{
	session_context_t *session_context = (session_context_t *)handle;
	struct cmd_event_param *evt_param;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	if (strncmp(session_context->name, "EVENT", SNR_NAME_MAX_LEN) != 0)
		return ERROR_WRONG_ACTION_ON_SENSOR_TYPE;

	/* currently only support 5 sub events */
	evt_param = session_context->evt_param;
	if (evt_param->num >= 5) {
		ALOGE("psh_event_append() "
			"currently only support 5 sub events \n");
		return ERROR_NOT_AVAILABLE;
	}

	if (sub_evt->sensor_id >= SENSOR_EVENT)
		return ERROR_WRONG_PARAMETER;

	if ((sub_evt->chan_id == 0) || (sub_evt->chan_id > 7))
		return ERROR_WRONG_PARAMETER;

	if (sub_evt->opt_id >= OP_MAX)
		return ERROR_WRONG_PARAMETER;

	evt_param->evts[evt_param->num] = *sub_evt;
	evt_param->num++;

	return ERROR_NONE;
}

error_t psh_add_event(handle_t handle)
{
	session_context_t *session_context = (session_context_t *)handle;
	struct cmd_event_param *evt_param;
	cmd_event *cmd;
	int ret, event_type;
	char message[MAX_MESSAGE_LENGTH];
	cmd_ack_event *p_cmd_ack;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	if (strncmp(session_context->name, "EVENT", SNR_NAME_MAX_LEN) != 0)
		return ERROR_WRONG_ACTION_ON_SENSOR_TYPE;

	/* psh_add_event() has been called before */
	if (session_context->evt_id != 0)
		return ERROR_NOT_AVAILABLE;

	evt_param = session_context->evt_param;
	if (evt_param->num == 0)
		return ERROR_NOT_AVAILABLE;

	cmd = malloc(sizeof(cmd_event) + sizeof(struct cmd_event_param));
	if (cmd == NULL)
		return ERROR_NOT_AVAILABLE;

	cmd->event_type = EVENT_CMD;
	cmd->cmd = CMD_ADD_EVENT;
	*(struct cmd_event_param *)(cmd->buf) = *evt_param;

	ret = psh_send_recv_cmd_locked(session_context->ctlfd, cmd, message,
                                       sizeof(cmd_event) + sizeof(struct cmd_event_param), MAX_MESSAGE_LENGTH, &session_context->ctlfd_lock);
	free(cmd);
	if (ret)
		return ret;

	event_type = *((int *) message);
	if (event_type != EVENT_CMD_ACK)
		return ERROR_CAN_NOT_GET_REPLY;

	p_cmd_ack = (cmd_ack_event *)message;
	if (p_cmd_ack->ret != E_SUCCESS)
		return ERROR_NOT_AVAILABLE;

	if (p_cmd_ack->buf_len > 0)
		session_context->evt_id = p_cmd_ack->buf[0];

	return ERROR_NONE;
}

error_t psh_clear_event(handle_t handle)
{
	session_context_t *session_context = (session_context_t *)handle;
	cmd_event cmd;
	int ret, event_type;
	char message[MAX_MESSAGE_LENGTH];
	cmd_ack_event *p_cmd_ack;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	if (strncmp(session_context->name, "EVENT", SNR_NAME_MAX_LEN) != 0)
		return ERROR_WRONG_ACTION_ON_SENSOR_TYPE;

	if (session_context->evt_id == 0)
		return ERROR_NOT_AVAILABLE;

	cmd.event_type = EVENT_CMD;
	cmd.cmd = CMD_CLEAR_EVENT;
	cmd.parameter = session_context->evt_id;

	ret = psh_send_recv_cmd_locked(session_context->ctlfd, &cmd, message,
                                       sizeof(cmd_event), MAX_MESSAGE_LENGTH, &session_context->ctlfd_lock);
	if (ret)
		return ret;

	event_type = *((int *) message);
	if (event_type != EVENT_CMD_ACK)
		return ERROR_CAN_NOT_GET_REPLY;

	p_cmd_ack = (cmd_ack_event *)message;
	if (p_cmd_ack->ret != E_SUCCESS)
		return ERROR_NOT_AVAILABLE;

	return ERROR_NONE;
}

/* 0 means psh_add_event() has not been called */
short psh_get_event_id(handle_t handle)
{
	session_context_t *session_context = (session_context_t *)handle;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	if (strncmp(session_context->name, "EVENT", SNR_NAME_MAX_LEN) != 0)
		return ERROR_WRONG_ACTION_ON_SENSOR_TYPE;

	return session_context->evt_id;
}

error_t psh_set_property(handle_t handle, property_type prop_type, void *value)
{
	int ret;
	ret = psh_set_property_with_size(handle, prop_type, 4, value);
	return ret;
}

error_t psh_set_property_with_size(handle_t handle, property_type prop_type, int size, void *value)
{
	session_context_t *session_context = (session_context_t *)handle;
	cmd_event *cmd;
	int ret, event_type;
	char message[MAX_MESSAGE_LENGTH];
	cmd_ack_event *p_cmd_ack;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	cmd = malloc(sizeof(cmd_event) + size);
	if (cmd == NULL)
		return ERROR_NOT_AVAILABLE;

	cmd->event_type = EVENT_CMD;
	cmd->cmd = CMD_SET_PROPERTY;
	cmd->parameter = prop_type;
	cmd->parameter1 = size;
	memcpy(cmd->buf, value, size);

	ret = psh_send_recv_cmd_locked(session_context->ctlfd, cmd, message,
                                       sizeof(cmd_event) + size, MAX_MESSAGE_LENGTH, &session_context->ctlfd_lock);
	free(cmd);
	if (ret)
		return ret;

	event_type = *((int *) message);
	if (event_type != EVENT_CMD_ACK)
		return ERROR_CAN_NOT_GET_REPLY;

	p_cmd_ack = (cmd_ack_event *)message;
	if (p_cmd_ack->ret != E_SUCCESS)
		return ERROR_DATA_RATE_NOT_SUPPORTED;

	return ERROR_NONE;
}

error_t psh_get_property_with_size(handle_t handle, int size, void *value, int *outlen, void *outbuf)
{
	session_context_t *session_context = (session_context_t *)handle;
	cmd_event *cmd;
	int ret, event_type;
	char message[MAX_MESSAGE_LENGTH];
	cmd_ack_event *p_cmd_ack;

	if (session_context == NULL)
		return ERROR_NOT_AVAILABLE;

	cmd = malloc(sizeof(cmd_event) + size);
	if (cmd == NULL)
		return ERROR_NOT_AVAILABLE;

	cmd->event_type = EVENT_CMD;
	cmd->cmd = CMD_GET_PROPERTY;
	cmd->parameter = size;
	memcpy(cmd->buf, value, size);

	ret = psh_send_recv_cmd_locked(session_context->ctlfd, cmd, message,
                                       sizeof(cmd_event) + size, MAX_MESSAGE_LENGTH, &session_context->ctlfd_lock);
	free(cmd);
	if (ret)
		return ret;

	event_type = *((int *) message);
	if (event_type != EVENT_CMD_ACK)
		return ERROR_CAN_NOT_GET_REPLY;

	p_cmd_ack = (cmd_ack_event *)message;
	if (p_cmd_ack->ret != E_SUCCESS)
		return ERROR_NOT_AVAILABLE;

	*outlen = p_cmd_ack->buf_len;
	memcpy(outbuf, p_cmd_ack->buf, p_cmd_ack->buf_len);

	return ERROR_NONE;
}

error_t run_bist(struct bist_data *bist)
{
	handle_t handle;
	int size;

	handle = psh_open_session(SENSOR_BIST);
	if (handle == NULL)
		return ERROR_NOT_AVAILABLE;

	size = psh_get_single(handle, bist, sizeof(struct bist_data));
	if (size <= 0)
	{
		psh_close_session(handle);
		return ERROR_NOT_AVAILABLE;
	}


	psh_close_session(handle);
	return ERROR_NONE;
}
