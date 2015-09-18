#include <stdlib.h>
#include <stdio.h>
#include <jni.h>
#include <assert.h>
#include <android/log.h>

#include <libsensorhub.h>

#undef LOG_TAG
#define LOG_TAG    "JNI_sensorcal"

#define LOGV(...) __android_log_print(ANDROID_LOG_SILENT, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define JNIREG_CLASS "com/intel/sensor/SensorCalibration"

/* sensor_type :
 * 0. compass calibration
 * 1. gyro calibration
 * ......
 */
enum {
	COMPASS_CAL,
	GYRO_CAL,
	SENSOR_CAL_MAX,
};

struct calibration_param {
	handle_t handle;
	struct cmd_calibration_param param;
};

static struct calibration_param cal_param[SENSOR_CAL_MAX];

/* This JNI function will open a sensor calibration.
 * return a sensor calibration handle for open success,
 * otherwise, retrun 0
 */
JNIEXPORT jint JNICALL psh_calibration_open(JNIEnv *env,
						jobject jobj,
						jint sensor_type)
{
	int ret;
	handle_t cal;

	if (sensor_type == COMPASS_CAL){
		LOGI("PSH_S_C: Start compass calibration");

		cal = psh_open_session_with_name("COMPS");
		if (cal == NULL) {
			LOGE("PSH_S_C: Can not connect compass_cal");
			return 0;
		}
	} else if (sensor_type == GYRO_CAL) {
		LOGI("PSH_S_C: Start gyro calibration");

		cal = psh_open_session_with_name("GYRO");
		if (cal == NULL) {
			LOGE("PSH_S_C: Can not connect gyro_cal");
			return 0;
		}
	} else {
		LOGE("PSH_S_C: No this sensor type supported!");
		return 0;
	}

	cal_param[sensor_type].handle = cal;
	memset((void*)&cal_param[sensor_type].param, 0,
				sizeof(struct cmd_calibration_param));

	return (int)cal;
}

/* This JNI function will trigger a new sensor calibration operation
 * Return 1 for success, 0 for fail
 */
JNIEXPORT jint JNICALL psh_calibration_start(JNIEnv *env,
						jobject jobj,
						jint handle)
{
	int ret;
	struct cmd_calibration_param param;
	handle_t cal = (handle_t)handle;

	LOGI("PSH_S_C: Start calibration");

	if (cal == NULL) {
		LOGE("PSH_S_C: illegal sensor handle");
		return 0;
	}

	param.sub_cmd = SUBCMD_CALIBRATION_START;
	ret = psh_set_calibration(cal, &param);
	if (ret != 0) {
		LOGE("PSH_S_C: Reset calibration failed, ret is %d", ret);
		return 0;
	}

	return 1;
}

/* This JNI function get the calibration result
 * Return 0xff for finished, 0 for not yet,
 * other for calibration result.
 */
JNIEXPORT jint JNICALL psh_calibration_get(JNIEnv *env,
						jobject jobj,
						jint handle)
{
	int ret;
	unsigned int result;
	struct cmd_calibration_param param;
	handle_t cal = (handle_t)handle;

	LOGI("PSH_S_C: Get calibration");

	if (cal == NULL) {
		LOGE("PSH_S_C: illegal sensor handle");
		return 0;
	}

	memset(&param, 0, sizeof(struct cmd_calibration_param));

	ret = psh_get_calibration(cal, &param);
	if (ret != 0) {
		LOGE("PSH_S_C: Get calibration failed, ret is %d", ret);
		return 0;
	}

	LOGI("PSH_S_C: Get calibration result");

	result = param.calibrated;

	if (result != SUBCMD_CALIBRATION_FALSE) {
		int i;

		for (i = 0; i < SENSOR_CAL_MAX; i++) {
			if (cal_param[i].handle == cal) {
				if ((cal_param[i].param.calibrated == SUBCMD_CALIBRATION_FALSE) ||
					(cal_param[i].param.calibrated > result) ||
					(result == SUBCMD_CALIBRATION_TRUE)) {
					cal_param[i].param = param;
				}

				break;
			}
		}
	}

	return result;
}

/* This JNI function set calibration param
 * Return 1 for set successfully, 0 for failed
 */
JNIEXPORT jint JNICALL psh_calibration_set(JNIEnv *env,
						jobject jobj,
						jint handle)
{
	struct cmd_calibration_param param;
	handle_t cal = (handle_t)handle;
	int ret = 0, i;

	LOGI("PSH_S_C: Set calibration");

	if (cal == NULL) {
		LOGE("PSH_S_C: illegal sensor handle");
		return ret;
	}

	for (i = 0; i < SENSOR_CAL_MAX; i++) {
		if (cal_param[i].handle == cal) {
                        if (cal_param[i].param.calibrated == 0) {
				LOGE("PSH_S_C: No valid param to set");
				ret = 0;
				break;
                        }

			param = cal_param[i].param;
			param.sub_cmd = SUBCMD_CALIBRATION_SET;
			param.calibrated = SUBCMD_CALIBRATION_TRUE;

			ret = psh_set_calibration(cal, &param);
			if (ret != 0) {
				LOGE("PSH_S_C: Set calibration failed, ret is %d", ret);
				ret = 0;
			} else {
				LOGI("PSH_S_C: Set calibration successful");
				ret = 1;
			}

			break;
		}
	}

	return ret;
}

/* This JNI function stop calibration
 * Return 1 for set successfully, 0 for failed
 */
JNIEXPORT jint JNICALL psh_calibration_stop(JNIEnv *env,
						jobject jobj,
						jint handle)
{
	struct cmd_calibration_param param;
	handle_t cal = (handle_t)handle;
	int ret;

	LOGI("PSH_S_C: Stop calibration");

	if (cal == NULL) {
		LOGE("PSH_S_C: illegal sensor handle");
		return 0;
	}

	param.sub_cmd = SUBCMD_CALIBRATION_STOP;
	ret = psh_set_calibration(cal, &param);
	if (ret != 0) {
		LOGE("PSH_S_C: Stop calibration failed, ret is %d", ret);
		return 0;
	}

	return 1;
}


JNIEXPORT void JNICALL psh_calibration_close(JNIEnv *env,
						jobject jobj,
						jint handle)
{
	handle_t cal = (handle_t)handle;

	LOGI("PSH_S_C: Close calibration");

	psh_close_session(cal);
}

static JNINativeMethod gMethods[] = {
	{"CalibrationOpen", "(I)I", (void*)psh_calibration_open},
	{"CalibrationStart", "(I)I", (void*)psh_calibration_start},
	{"CalibrationGet", "(I)I", (void*)psh_calibration_get},
	{"CalibrationSet", "(I)I", (void*)psh_calibration_set},
	{"CalibrationStop", "(I)I", (void*)psh_calibration_stop},
	{"CalibrationClose", "(I)V", (void*)psh_calibration_close},
};

static int registerNativeMethods(JNIEnv* env, const char* className,
        JNINativeMethod* gMethods, int numMethods)
{
	jclass clazz;
	clazz = (env)->FindClass(className);
	if (clazz == NULL) {
		return JNI_FALSE;
	}

	if ((env)->RegisterNatives( clazz, gMethods, numMethods) < 0) {
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

static int registerNatives(JNIEnv* env)
{
	if (!registerNativeMethods(env, JNIREG_CLASS, gMethods,
				sizeof(gMethods) / sizeof(gMethods[0])))
		return JNI_FALSE;

	return JNI_TRUE;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	jint result = -1;

	if ((vm)->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		return -1;
	}

	assert(env != NULL);

	if (!registerNatives(env)) {
		return -1;
	}

	result = JNI_VERSION_1_4;

	return result;
}

