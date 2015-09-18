#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>
#include <assert.h>
#include "../include/libsensorhub.h"


#define GET	(1 << 0)
#define SET	(1 << 1)
#define READ	(1 << 2)
#define COMP	(1 << 3)
#define GYRO	(1 << 4)
#define ACCL	(1 << 5)
#define ERR	(1 << 6)

struct magnetic_info {
        float offset[3];
        float w[3][3];
        float bfield;
} __attribute__ ((packed));

struct gyroscope_info {
        short x;
        short y;
        short z;
} __attribute__ ((packed));

struct accelerometer_info {
        float factors[3][2];
} __attribute__ ((packed));

static void dump_calibration_info(int req, struct cmd_calibration_param * param)
{
	printf ("calibration sub cmd: %u\n", param->sub_cmd);
	printf ("calibration result: %u\n", param->calibrated);
	printf ("cal_info size: %d \n", param->cal_param.size);

        if (req & COMP) {
                struct magnetic_info* info = (struct magnetic_info*)param->cal_param.data;
                printf("magnetic filed:\n");
                printf("offset: %f %f %f\n", info->offset[0], info->offset[1], info->offset[2]);
                printf("w matrix:\n");
                printf("%f %f %f\n", info->w[0][0], info->w[0][1], info->w[0][2]);
                printf("%f %f %f\n", info->w[1][0], info->w[1][1], info->w[1][2]);
                printf("%f %f %f\n", info->w[2][0], info->w[2][1], info->w[2][2]);
                printf("bfiled: %f\n", info->bfield);
        } else if(req & GYRO) {
                struct gyroscope_info* info = (struct gyroscope_info*)param->cal_param.data;
                printf("gyroscope:\n");
                printf("x: %d\n", info->x);
                printf("y: %d\n", info->y);
                printf("z: %d\n", info->z);
        } else if(req & ACCL) {
                struct accelerometer_info* info = (struct accelerometer_info*)param->cal_param.data;
                printf("accelerometer:\n");
                printf("x: %f %f\n", info->factors[0][0], info->factors[0][1]);
                printf("y: %f %f\n", info->factors[1][0], info->factors[1][1]);
                printf("z: %f %f\n", info->factors[2][0], info->factors[2][1]);
        }
}

#define ACCL_FILE "/data/ACCEL.conf"
#define COMP_FILE "/data/COMPS.conf"
#define GYRO_FILE "/data/GYRO.conf"

static int read_calibration_file(int req, struct cmd_calibration_param * param)
{
        int fd = -1, ret;
        char* file = NULL;

        if (req & COMP)
                file = COMP_FILE;
        else if (req & ACCL)
                file = ACCL_FILE;
        else if (req & GYRO)
                file = GYRO_FILE;

        if (file == NULL) {
                printf("No match file!\n");
                return -1;
        }

        fd = open(file, O_RDONLY);
        if (fd < 0) {
                printf("open %s error: %d\n", file, fd);
                return -1;
        }

        ret = read(fd, param, sizeof(struct cmd_calibration_param));
        close(fd);

        return 0;
}

static int get_set(int argc, char* argv[])
{
	static struct option opts[] = {
		{"COMP", 0, NULL, 'C'},
		{"GYRO", 0, NULL, 'G'},
		{"ACCL", 0, NULL, 'A'},
		{"get", 0, NULL, 'g'},
		{"set", 0, NULL, 's'},
		{"read", 0, NULL, 'r'},
		{0, 0, NULL, 0},
	};

	static int req = 0;

	int i, index;

	while (1) {
		i = getopt_long(argc, argv, "CGAgsr", opts, &index);
		if (i == -1)
			break ;

		switch (i){
		case 'C':
			req |= COMP;
			break;
		case 'G':
			req |= GYRO;
			break;
		case 'A':
			req |= ACCL;
			break;
		case 'g':
			req |= GET;
			break;
		case 's':
			req |= SET;
			break;
		case 'r':
			req |= READ;
			break;
		default:
			req = ERR;
		}

	if (req == ERR)
		break;
	}

	return req;

}

static char* enum2str[] = {
	[COMP | GET] = "comp get",
	[COMP | SET] = "comp set",
	[COMP | READ] = "comp read",
	[GYRO | GET] = "gyro get",
	[GYRO | SET] = "gyro set",
	[GYRO | READ] = "gyro read",
	[ACCL | GET] = "accl get",
	[ACCL | SET] = "accl set",
	[ACCL | READ] = "accl read",
	[ERR] = "err",
};

static void usage(void)
{
	printf ("\nUsage: compasscal [--COMP | --GYRO | --ACCL][--set | --get | --read]\n");
	printf ("\t --COMP target is compass calibration.\n");
	printf ("\t --GYRO target is compass calibration.\n");
	printf ("\t --set reset calibration parameters.\n");
	printf ("\t --get dump current calibration parameters data.\n");
}

int main (int argc, char* argv[])
{
	int req;
	struct cmd_calibration_param param;
	handle_t cal = NULL;;

	req =  get_set(argc, argv);
	printf("Your request is %s\n", enum2str[req]);
	if (req == ERR){
		usage();
		return 0;
	}

	memset(&param, 0, sizeof(struct cmd_calibration_param));

	if (req & GYRO)
		cal = psh_open_session_with_name("GYRO");
	else if (req & COMP)
		cal = psh_open_session_with_name("COMPS");
	else if (req & ACCL)
		cal = psh_open_session_with_name("ACCEL");
	assert(cal != NULL);

	if (req & SET){
		param.sub_cmd = SUBCMD_CALIBRATION_START;
		psh_set_calibration(cal, &param);
	} else if (req & GET) {
		psh_get_calibration(cal, &param);
                printf("Get result from PSH:\n");
		dump_calibration_info(req, &param);
	} else if (req & READ) {
                if (read_calibration_file(req, &param) == 0) {
                        printf("Read result from file:\n");
                        dump_calibration_info(req, &param);
                }
        }

	psh_close_session(cal);

	return 0;
}
