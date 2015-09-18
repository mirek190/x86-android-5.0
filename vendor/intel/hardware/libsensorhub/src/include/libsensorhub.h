#ifndef _LIBSENSORHUB_H
#define _LIBSENSORHUB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
        SENSOR_INVALID = -1,
	SENSOR_ACCELEROMETER = 0,
	SENSOR_GYRO,
	SENSOR_COMP,
	SENSOR_BARO,
	SENSOR_ALS,
	SENSOR_PROXIMITY,
	SENSOR_TC,
	SENSOR_LPE,
	SENSOR_ACCELEROMETER_SEC,
	SENSOR_GYRO_SEC,
	SENSOR_COMP_SEC,
	SENSOR_ALS_SEC,
	SENSOR_PROXIMITY_SEC,
	SENSOR_BARO_SEC,

	SENSOR_ACTIVITY,
	SENSOR_GS,
	SENSOR_GESTURE_FLICK,

	SENSOR_ROTATION_VECTOR,
	SENSOR_GRAVITY,
	SENSOR_LINEAR_ACCEL,
	SENSOR_ORIENTATION,
	SENSOR_CALIBRATION_COMP,
	SENSOR_CALIBRATION_GYRO,
	SENSOR_9DOF,
	SENSOR_PEDOMETER,
	SENSOR_MAG_HEADING,
	SENSOR_SHAKING,
	SENSOR_MOVE_DETECT,
	SENSOR_STAP,
	SENSOR_PAN_TILT_ZOOM,
	SENSOR_LIFT_VERTICAL, /*invalid sensor, leave for furture using*/
	SENSOR_DEVICE_POSITION,
	SENSOR_STEPCOUNTER,
	SENSOR_STEPDETECTOR,
	SENSOR_SIGNIFICANT_MOTION,
	SENSOR_GAME_ROTATION_VECTOR,
	SENSOR_GEOMAGNETIC_ROTATION_VECTOR,
	SENSOR_6DOFAG,
	SENSOR_6DOFAM,
	SENSOR_LIFT,
	SENSOR_DTWGS,
	SENSOR_GESTURE_HMM,
	SENSOR_GESTURE_EARTOUCH,
	SENSOR_PEDESTRIAN_DEAD_RECKONING,
	SENSOR_INSTANT_ACTIVITY,
	SENSOR_DIRECTIONAL_SHAKING,
	SENSOR_GESTURE_TILT,
	SENSOR_GESTURE_SNAP,
	SENSOR_PICKUP,
	SENSOR_TILT_DETECTOR,

	SENSOR_BIST,

	SENSOR_EVENT,
	SENSOR_MAX
} psh_sensor_t;

typedef enum {
	ERROR_NONE = 0,
	ERROR_DATA_RATE_NOT_SUPPORTED = -1,
	ERROR_NOT_AVAILABLE = -2,
	ERROR_MESSAGE_NOT_SENT = -3,
	ERROR_CAN_NOT_GET_REPLY = -4,
	ERROR_WRONG_ACTION_ON_SENSOR_TYPE = -5,
	ERROR_WRONG_PARAMETER = -6,
	ERROR_PROPERTY_NOT_SUPPORTED = -7
} error_t;

enum {
	OP_EQUAL = 0,
	OP_GREAT,
	OP_LESS,
	OP_BETWEEN,
	OP_MAX
};

typedef enum {
	PROP_GENERIC_START = 0,
	PROP_STOP_REPORTING = 1,
	PROP_GENERIC_END = 20,

	PROP_PEDOMETER_START = 20,
	PROP_PEDOMETER_MODE = 21,
	PROP_PEDOMETER_N = 22,
	PROP_PEDOMETER_END = 40,

	PROP_ACT_START = 40,
	PROP_ACT_MODE,
	PROP_ACT_CLSMASK,
	PROP_ACT_N,
	PROP_ACT_DUTY_M,
	PROP_ACT_END = 60,

	PROP_GFLICK_START = 60,
	PROP_GFLICK_CLSMASK,
	PROP_GFLICK_LEVEL,
	PROP_GFLICK_END = 80,

	PROP_SHAKING_START = 80,
	PROP_SHAKING_SENSITIVITY,
	PROP_SHAKING_END = 100,

	PROP_STAP_START = 100,
	PROP_STAP_CLSMASK,
	PROP_STAP_LEVEL,
	PROP_STAP_END = 120,

	PROP_DTWGSM_START = 120,
	PROP_DTWGSM_LEVEL,
	PROP_DTWGSM_DST,
	PROP_DTWGSM_TEMPLATE,
	PROP_DTWGSM_END = 140,

	PROP_WPD_START = 140,
	PROP_WPD_SCOUNTERMODE,
	PROP_WPD_NSC,
	PROP_WPD_REPORT_PAUSE,
	PROP_WPD_END = 150,

	PROP_INSTANTACTIVITY_START = 150,
	PROP_INSTANTACTIVITY_CLSMASK,
	PROP_INSTANTACTIVITY_END = 160,

	PROP_EARTOUCH_START = 160,
	PROP_EARTOUCH_CLSMASK,
	PROP_EARTOUCH_END = 180,

	PROP_PDR_START = 180,
	PROP_PDR_USER_H,	/* user's height */
	PROP_PDR_FLOOR_H,	/* building's floor height */
	PROP_PDR_INIT_X,	/* init position x (East) */
	PROP_PDR_INIT_Y,	/* init position y (North) */
	PROP_PDR_INIT_FL,	/* init floor level */
	PROP_PDR_N,		/* report interval */
	PROP_PDR_6DOF,	/* use 6DOF as rmat input, or not */
	PROP_PDR_END = 200,

	PROP_LIFT_START = 200,
	PROP_LIFT_MASK,
	PROP_LIFT_END = 220,

	PROP_DIRECTIONAL_SHAKING_START = 220,
	PROP_DIRECTIONAL_SHAKING_LEVEL,
	PROP_DIRECTIONAL_SHAKING_END = 240,

	GTILT_PROP_START = 240,
	GTILT_PROP_CLSMASK,
	GTILT_PROP_LRTHRESHOLD,
	GTILT_PROP_UDTHRESHOLD,
	GTILT_PROP_END = 260,

	GSNAP_PROP_START = 260,
	GSNAP_PROP_CLSMASK,
	GSNAP_PROP_LRTHRESHOLD,
	GSNAP_PROP_UDTHRESHOLD,
	GSNAP_PROP_CWTHRESHOLD,
	GSNAP_PROP_END = 280,
} property_type;

typedef enum {
	PROP_PEDO_MODE_NCYCLE,
	PROP_PEDO_MODE_ONCHANGE,
} property_pedo_mode;

typedef enum {
	PROP_ACT_MODE_NCYCLE,
	PROP_ACT_MODE_ONCHANGE
} property_act_mode;

typedef enum {
	PROP_WPD_MODE_RESOLUTION,
	PROP_WPD_MODE_ONCHANGE,
} property_wpd_mode;

typedef enum {
	STOP_WHEN_SCREEN_OFF = 0,
	NO_STOP_WHEN_SCREEN_OFF = 1,
	NO_STOP_NO_REPORT_WHEN_SCREEN_OFF = 2,
} streaming_flag;

typedef enum {
	AND = 0,
	OR = 1,
} relation;

typedef void * handle_t;

/* return NULL if failed */
handle_t psh_open_session(psh_sensor_t sensor_type);

handle_t psh_open_session_with_name(const char *name);

void psh_close_session(handle_t handle);

/* return 0 when success
   data_rate: the unit is HZ;
   buffer_delay: the unit is ms. It's used to tell psh that application
   can't wait more than 'buffer_delay' ms to get data. In another word
   data can arrive before 'buffer_delay' ms elaps.
   So every time the data is returned, the data size may vary and the
   application need to buffer the data by itself */
error_t psh_start_streaming(handle_t handle, int data_rate, int buffer_delay);

/* flag: 2 means no_stop_no_report when IA sleep; 1 means no_stop when IA sleep; 0 means stop when IA sleep */
error_t psh_start_streaming_with_flag(handle_t handle, int data_rate, int buffer_delay, streaming_flag flag);

#define MAX_UNIT_SIZE 128
error_t psh_flush_streaming(handle_t handle, unsigned int data_unit_size);

error_t psh_stop_streaming(handle_t handle);

/* return data size if success; error_t if failure */
error_t psh_get_single(handle_t handle, void *buf, int buf_size);

struct cmd_calibration_param;
/* set the calibration data of compass sensor
   If param not NULL, it will set calibration parameters.  */
error_t psh_set_calibration(handle_t handle, struct cmd_calibration_param * param);

error_t psh_get_calibration(handle_t handle, struct cmd_calibration_param * param);
/* return -1 if failed */
int psh_get_fd(handle_t handle);

/* relation: 0, AND; 1, OR; default is AND */
error_t psh_event_set_relation(handle_t handle, relation relation);

struct sub_event;
error_t psh_event_append(handle_t handle, struct sub_event *sub_evt);

error_t psh_add_event(handle_t handle);

error_t psh_clear_event(handle_t handle);

/* 0 means psh_add_event() has not been called */
short psh_get_event_id(handle_t handle);

/* set properties */
error_t psh_set_property(handle_t handle, property_type prop_type, void *value);
error_t psh_set_property_with_size(handle_t handle, property_type prop_type, int size, void *value);

/* get properties */
error_t psh_get_property_with_size(handle_t handle, int size, void *value, int *outlen, void *outbuf);

/* data format of each sensor type */
struct accel_data {
	int64_t ts;
	short x;
	short y;
	short z;
} __attribute__ ((packed));

struct gyro_raw_data {
	int64_t ts;
	short x;
	short y;
	short z;
} __attribute__ ((packed));

struct compass_raw_data {
	int64_t ts;
	short x;
	short y;
	short z;
} __attribute__ ((packed));

struct tc_data {
	int64_t ts;
	short orien_xy;
	short orien_z;
} __attribute__ ((packed));

struct baro_raw_data {
	int64_t ts;
	int p;
} __attribute__ ((packed));

struct als_raw_data {
	int64_t ts;
	unsigned short lux;
} __attribute__ ((packed));

#define MAX_PHY_ACT_DATA_LEN 64
struct phy_activity_data {
	int64_t ts;
	short len;
	short values[0];
} __attribute__ ((packed));

struct gs_data {
	int64_t ts;
	unsigned short size; //unit is byte
	short sample[0];
} __attribute__ ((packed));

struct gesture_hmm_data {
	int64_t ts;
	short prox_gesture; //proximity if not use context arbiter; gesture if use context arbiter
	unsigned short size; //unit is byte
	short sample[0];
} __attribute__ ((packed));

struct pdr_sample {
	int x;		/* position x, unit is cm */
	int y;		/* position y, unit is cm */
	int fl;	/* floor level, unit is 1 */
	int heading;	/* heading angle, unit is 0.01 deg */
	int step;	/* step counts in PDR, unit is 1 step */
	int distance;	/* total PDR distance, unit is cm */
	int confidence;/* heading confidence, the smaller the better */
} __attribute__ ((packed));

struct pdr_data {
	int64_t ts;
	short size;
	struct pdr_sample sample[0];
} __attribute__ ((packed));

struct gesture_eartouch_data {
	int64_t ts;
	short eartouch;
} __attribute__ ((packed));

struct ps_phy_data {
	int64_t ts;
	unsigned short near;
} __attribute__ ((packed));

struct gesture_flick_data {
	int64_t ts;
	short flick;
} __attribute__ ((packed));

struct shaking_data {
	int64_t ts;
	short shaking;
} __attribute__ ((packed));

struct directional_shaking_data {
	int64_t ts;
	short shaking;
} __attribute__ ((packed));

struct gesture_tilt_data {
	int64_t ts;
	short tilt;
} __attribute__ ((packed));

struct gesture_snap_data {
	int64_t ts;
	short snap;
} __attribute__ ((packed));

struct stap_data {
	int64_t ts;
	short stap;
} __attribute__ ((packed));

struct pickup_data {
	int64_t ts;
	short pickup;
} __attribute__ ((packed));

struct tilt_detector_data {
	int64_t ts;
	short tiltd;
} __attribute__ ((packed));

struct pz_data {
	int64_t ts;
	short deltX;
	short deltY;		/* deltX and deltY: 0.01deg/s */
}__attribute__ ((packed));

struct rotation_vector_data {
	int64_t ts;
	int x;
	int y;
	int z;
	int w;
} __attribute__ ((packed));

struct game_rotation_vector_data {
	int64_t ts;
	int x;
	int y;
	int z;
	int w;
} __attribute__ ((packed));

struct geomagnetic_rotation_vector_data {
	int64_t ts;
	int x;
	int y;
	int z;
	int w;
} __attribute__ ((packed));

struct gravity_data {
	int64_t ts;
	int x;
	int y;
	int z;
} __attribute__ ((packed));

struct linear_accel_data {
	int64_t ts;
	int x;
	int y;
	int z;
} __attribute__ ((packed));

struct orientation_data {
	int64_t ts;
	int azimuth;
	int pitch;
	int roll;
} __attribute__ ((packed));

#define MAX_CALDATA_SIZE		128
struct cal_param {
	unsigned char size;
	char data[MAX_CALDATA_SIZE];
} __attribute__ ((packed));

#define SUBCMD_CALIBRATION_SET		(0x1)
#define SUBCMD_CALIBRATION_GET		(0x2)
#define SUBCMD_CALIBRATION_START	(0x3)
#define SUBCMD_CALIBRATION_STOP		(0x4)

#define SUBCMD_CALIBRATION_TRUE		(100)
#define SUBCMD_CALIBRATION_FALSE 	(0)
struct cmd_calibration_param {
	unsigned char sub_cmd;
	unsigned char calibrated;
	struct cal_param cal_param;
} __attribute__ ((packed));

struct sub_event {
	unsigned char sensor_id;
	unsigned char chan_id;		/* 1:x, 2:y, 4:z, 7:all */
	unsigned char opt_id;		/* 0:OP_EQUAL, 1:OP_GREAT, 2:OP_LESS, 3:OP_BETWEEN */
	int param1;
	int param2;
} __attribute__ ((packed));

struct event_notification_data {
	unsigned char event_id;
} __attribute__ ((packed));

struct ndof_data {
	int64_t ts;
	int	m[9];
} __attribute__ ((packed));

struct ndofag_data {
	int64_t ts;
	int     m[9];
} __attribute__ ((packed));

struct ndofam_data {
	int64_t ts;
	int     m[9];
} __attribute__ ((packed));

struct pedometer_data {
	int64_t ts;
	int num;
	short mode;
	int vec[0];
} __attribute__ ((packed));

struct mag_heading_data {
	int64_t ts;
	int heading;
} __attribute__ ((packed));

struct lpe_phy_data {
	int64_t ts;
	unsigned int lpe_msg;
} __attribute__ ((packed));

#define MD_STATE_UNCHANGE 0
#define MD_STATE_MOVE 1
#define MD_STATE_STILL 2

struct md_data {
	int64_t ts;
	short state;
} __attribute__ ((packed));

struct device_position_data {
	int64_t ts;
	short pos;
} __attribute__ ((packed));

struct sm_data {
	int64_t ts;
	short state;
} __attribute__ ((packed));

struct stepcounter_data {
	int64_t ts;
	int num;
} __attribute__ ((packed));

struct stepdetector_data {
	int64_t ts;
	int state;
} __attribute__ ((packed));

struct instant_activity_data {
	int64_t ts;
	int typeclass;
} __attribute__ ((packed));

struct lift_data {
	int64_t ts;
	short look;
	short vertical;
} __attribute__ ((packed));

struct dtwgs_data {
	int64_t ts;
	short gsnum;
	int score;
} __attribute__ ((packed));

#ifdef __cplusplus
}
#endif

#endif /* libsensorhub.h */
