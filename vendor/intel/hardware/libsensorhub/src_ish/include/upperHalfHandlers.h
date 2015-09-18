#ifndef UPPERHH_H
#define UPPERHH_H

#include "message.h"

extern sensor_state_t *sensor_list;
extern int current_sensor_index;	// means the current empty slot of sensor_list[]

void dispatch_get_single(struct cmd_resp *p_cmd_resp);
void dispatch_streaming(struct cmd_resp *p_cmd_resp);
void handle_calibration(struct cmd_calibration_param * param);
void handle_add_event_resp(struct cmd_resp *p_cmd_resp);
void handle_clear_event_resp(struct cmd_resp *p_cmd_resp);
void dispatch_event(struct cmd_resp *p_cmd_resp);

//todo - move to another header
unsigned char allocate_trans_id();
sensor_state_t* get_sensor_state_with_name(const char *name);

#endif