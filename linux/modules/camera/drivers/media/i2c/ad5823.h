#ifndef __AD5823_H__
#define __AD5823_H__

#include <linux/atomisp_platform.h>
#include <linux/types.h>
#include <linux/time.h>


#define AD5823_VCM_ADDR	0x0c

#define AD5823_VCM_SLEW_STEP_MAX		0x7
#define AD5823_VCM_SLEW_STEP_MASK		0x7

#define AD5823_VCM_SLEW_STEP			0x30F0
#define AD5823_VCM_CODE					0x30F2
#define AD5823_VCM_SLEW_TIME			0x30F4
#define AD5823_VCM_SLEW_TIME_MAX		0xffff
#define AD5823_VCM_ENABLE				0x8000

#define AD5823_INVALID_CONFIG	0xffffffff
#define AD5823_MAX_FOCUS_POS	1023
#define AD5823_MAX_FOCUS_NEG	(-1023)

#define DELAY_PER_STEP_NS		1000000
#define DELAY_MAX_PER_STEP_NS	(1000000 * 40) // (1000000 * 1023)

#define AD5823_16BIT 			0x0002
#define I2C_MSG_LENGTH			0x2


/* Register Definitions */
//#define AD5823_IC_INFO			0x00
#define AD5823_RESET			0x01
#define AD5823_MODE				0x02
#define AD5823_VCM_MOVE_TIME    0x03
#define AD5823_VCM_CODE_MSB		0x04
#define AD5823_VCM_CODE_LSB		0x05
#define AD5823_THRESHOLD_MSB	0x06
#define AD5823_THRESHOLD_LSB	0x07
//#define AD5823_VCM_THRESHOLD	 0x08


/* ARC MODE ENABLE */
#define AD5823_ARC_EN			0x02
/* ARC RES2 MODE */
#define AD5823_ARC_RES2			0x01
/* ARC VCM FREQ - 78.1Hz */
#define AD5823_DEF_FREQ			0x7a
/* ARC VCM THRESHOLD - 0x08 << 1 */
#define AD5823_DEF_THRESHOLD	0x64
#define AD5823_ID				0x24
#define VCM_CODE_MASK			0x03ff

#define AD5823_MODE_1M_SWITCH_CLOCK	0x30


/* ad5823 device structure */
struct ad5823_device {
	const struct camera_af_platform_data *platform_data;
	struct timespec timestamp_t_focus_abs;
	struct timespec focus_time;	/* Time when focus was last time set */
	s32 focus;			/* Current focus value */
	s16 number_of_steps;
};



#endif

