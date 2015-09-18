#ifndef HW_H
#define HW_H

#include "message.h"


void reset_hw_layer();
int send_control_cmd(int tran_id, int cmd_id, int hw_sensor_id, unsigned short data_rate, unsigned short buffer_delay, unsigned short bit_cfg);
int send_set_property(sensor_state_t *p_sensor_state, property_type prop_type, int len, unsigned char *value);
void set_calibration(sensor_state_t *p_sensor_state, struct cmd_calibration_param *param);
void get_calibration(sensor_state_t *p_sensor_state, session_state_t *p_session_state);
void handle_clear_event(session_state_t *p_session_state);
void handle_add_event(session_state_t *p_session_state, cmd_event* p_cmd);
int FillHWDescriptors(int maxfd, void *read_fds, void *exception_fds, int *hw_fds, int *hw_fds_num, bool *toException);
void ProcessHW(int fd);

#endif
