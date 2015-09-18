#include "PSHSensor.hpp"

#define ACCELERATION_CALIBATION_FILE   "/data/ACCEL.conf"
#define MAGNETIC_CALIBATION_FILE       "/data/COMPS.conf"
#define GYROSCOPE_CALIBATION_FILE      "/data/GYRO.conf"

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

bool SensorHubHelper::getCalibrationFileName(int sensorType, char* file)
{
        switch (sensorType) {
        case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
                memcpy(file, MAGNETIC_CALIBATION_FILE, 17);
                return true;
        case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
                memcpy(file, GYROSCOPE_CALIBATION_FILE, 16);
                return true;
        default:
                return false;
        }
        return false;
}

bool SensorHubHelper::getCalibrationOffset(int sensorType, struct cmd_calibration_param* param, float* offset)
{
        struct magnetic_info* minfo;
        struct gyroscope_info* ginfo;

        if (param->calibrated != SUBCMD_CALIBRATION_TRUE)
                return false;

        switch (sensorType) {
        case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
                minfo = (struct magnetic_info*)param->cal_param.data;
                offset[0] = minfo->offset[0];
                offset[1] = minfo->offset[1];
                offset[2] = minfo->offset[2];
                return true;
        case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
                ginfo = (struct gyroscope_info*)param->cal_param.data;
                offset[0] = ginfo->x;
                offset[1] = ginfo->y;
                offset[2] = ginfo->z;
                return true;
        default:
                return false;
        }
        return false;
}


psh_sensor_t SensorHubHelper::getType(int sensorType, sensors_subname subname)
{
        switch (sensorType) {
        case SENSOR_TYPE_ACCELEROMETER:
                if (subname == SECONDARY)
                        return SENSOR_ACCELEROMETER_SEC;
                return SENSOR_ACCELEROMETER;
        case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
        case SENSOR_TYPE_MAGNETIC_FIELD:
                if (subname == SECONDARY)
                        return SENSOR_COMP_SEC;
                return SENSOR_COMP;
        case SENSOR_TYPE_ORIENTATION:
                return SENSOR_ORIENTATION;
        case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
        case SENSOR_TYPE_GYROSCOPE:
                if (subname == SECONDARY)
                        return SENSOR_GYRO_SEC;
                return SENSOR_GYRO;
        case SENSOR_TYPE_LIGHT:
                if (subname == SECONDARY)
                        return SENSOR_ALS_SEC;
                return SENSOR_ALS;
        case SENSOR_TYPE_PRESSURE:
                if (subname == SECONDARY)
                        return SENSOR_BARO_SEC;
                return SENSOR_BARO;
        case SENSOR_TYPE_PROXIMITY:
                if (subname == SECONDARY)
                        return SENSOR_PROXIMITY_SEC;
                return SENSOR_PROXIMITY;
        case SENSOR_TYPE_GRAVITY:
                return SENSOR_GRAVITY;
        case SENSOR_TYPE_LINEAR_ACCELERATION:
                return SENSOR_LINEAR_ACCEL;
        case SENSOR_TYPE_ROTATION_VECTOR:
                return SENSOR_ROTATION_VECTOR;
        case SENSOR_TYPE_WAKE_GESTURE:
                return SENSOR_STAP;
        case SENSOR_TYPE_GLANCE_GESTURE:
                return SENSOR_SHAKING;
        case SENSOR_TYPE_PICK_UP_GESTURE:
                return SENSOR_PICKUP;
        case SENSOR_TYPE_TILT_DETECTOR:
                return SENSOR_TILT_DETECTOR;
        case SENSOR_TYPE_GESTURE_FLICK:
                return SENSOR_GESTURE_FLICK;
        case SENSOR_TYPE_TERMINAL:
                return SENSOR_TC;
        case SENSOR_TYPE_SHAKE:
                return SENSOR_SHAKING;
        case SENSOR_TYPE_SIMPLE_TAPPING:
                return SENSOR_STAP;
        case SENSOR_TYPE_MOVE_DETECT:
                return SENSOR_MOVE_DETECT;
        case SENSOR_TYPE_STEP_DETECTOR:
                return SENSOR_STEPDETECTOR;
        case SENSOR_TYPE_STEP_COUNTER:
                return SENSOR_STEPCOUNTER;
        case SENSOR_TYPE_SIGNIFICANT_MOTION:
                return SENSOR_SIGNIFICANT_MOTION;
        case SENSOR_TYPE_GAME_ROTATION_VECTOR:
                return SENSOR_GAME_ROTATION_VECTOR;
        case SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
                return SENSOR_GEOMAGNETIC_ROTATION_VECTOR;
        case SENSOR_TYPE_DEVICE_POSITION:
                return SENSOR_DEVICE_POSITION;
        case SENSOR_TYPE_LIFT:
                return SENSOR_LIFT;
        case SENSOR_TYPE_PAN_ZOOM:
                return SENSOR_PAN_TILT_ZOOM;
        case SENSOR_TYPE_GESTURE:
        case SENSOR_TYPE_PHYSICAL_ACTIVITY:
        case SENSOR_TYPE_PEDOMETER:
        case SENSOR_TYPE_TEMPERATURE:
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
        case SENSOR_TYPE_AUDIO_CLASSIFICATION:
        case SENSOR_TYPE_CALIBRATION:
        default:
                ALOGE("%s: Unsupported Sensor Type: %d", __FUNCTION__, sensorType);
                break;
        }
        return SENSOR_INVALID;
}

void SensorHubHelper::getStartStreamingParameters(int sensorType, int &dataRate, int &bufferDelay, streaming_flag &flag)
{
        switch (sensorType) {
        case SENSOR_TYPE_WAKE_GESTURE:
                dataRate = STAP_SAMPLE_RATE;
                bufferDelay = STAP_BUF_DELAY;
                flag = NO_STOP_WHEN_SCREEN_OFF;
                break;
        case SENSOR_TYPE_GLANCE_GESTURE:
                dataRate = SHAKING_SAMPLE_RATE;
                bufferDelay = SHAKING_BUF_DELAY;
                flag = NO_STOP_WHEN_SCREEN_OFF;
                break;
        case SENSOR_TYPE_PICK_UP_GESTURE:
                flag = NO_STOP_WHEN_SCREEN_OFF;
                break;
        case SENSOR_TYPE_TILT_DETECTOR:
                flag = NO_STOP_WHEN_SCREEN_OFF;
                break;
        case SENSOR_TYPE_GESTURE_FLICK:
                dataRate = GF_SAMPLE_RATE;
                bufferDelay = GF_BUF_DELAY;
                break;
        case SENSOR_TYPE_TERMINAL:
                dataRate = TERM_RATE;
                bufferDelay = TERM_DELAY;
                break;
        case SENSOR_TYPE_SHAKE:
                dataRate = SHAKING_SAMPLE_RATE;
                bufferDelay = SHAKING_BUF_DELAY;
                break;
        case SENSOR_TYPE_SIMPLE_TAPPING:
                dataRate = STAP_SAMPLE_RATE;
                bufferDelay = STAP_BUF_DELAY;
                break;
        case SENSOR_TYPE_STEP_COUNTER:
                flag = NO_STOP_NO_REPORT_WHEN_SCREEN_OFF;
                break;
        default:
                break;
        }
}

error_t SensorHubHelper::setPSHPropertyIfNeeded(int sensorType, struct sensor_hub_methods methods, handle_t handler) {
        switch (sensorType) {
        case SENSOR_TYPE_SHAKE:
                return methods.psh_set_property_with_size(handler, PROP_GENERIC_START, strlen(SHAKE_SEN_MEDIUM) + 1, const_cast<char*>(SHAKE_SEN_MEDIUM));
        case SENSOR_TYPE_SIMPLE_TAPPING:
                return methods.psh_set_property_with_size(handler, PROP_GENERIC_START, strlen(STAP_CLSMASK_BOTH) + 1, const_cast<char*>(STAP_CLSMASK_BOTH));
        case SENSOR_TYPE_WAKE_GESTURE:
                return methods.psh_set_property_with_size(handler, PROP_GENERIC_START, strlen(STAP_CLSMASK_FOR_WAKE) + 1, const_cast<char*>(STAP_CLSMASK_FOR_WAKE));
        case SENSOR_TYPE_GLANCE_GESTURE:
                return methods.psh_set_property_with_size(handler, PROP_GENERIC_START, strlen(SHAKE_SEN_LOW) + 1, const_cast<char*>(SHAKE_SEN_LOW));
        default:
                break;
        }
        return ERROR_NONE;
}

int SensorHubHelper::getGestureFlickEvent(struct gesture_flick_data data)
{
        switch ((FlickGestures)data.flick) {
        case LEFT_FLICK:
                return SENSOR_EVENT_TYPE_GESTURE_LEFT_FLICK;
        case RIGHT_FLICK:
                return SENSOR_EVENT_TYPE_GESTURE_RIGHT_FLICK;
        case LEFT_TWICE:
                return SENSOR_EVENT_TYPE_GESTURE_LEFT_FLICK_TWICE;
        case RIGHT_TWICE:
                return SENSOR_EVENT_TYPE_GESTURE_RIGHT_FLICK_TWICE;
        case UP_FLICK:
                return SENSOR_EVENT_TYPE_GESTURE_UP_FLICK;
        case DOWN_FLICK:
                return SENSOR_EVENT_TYPE_GESTURE_DOWN_FLICK;
        case NO_FLICK:
        default:
                return SENSOR_EVENT_TYPE_GESTURE_NO_FLICK;
        }
        return SENSOR_EVENT_TYPE_GESTURE_NO_FLICK;
}

int SensorHubHelper::getTerminalEvent(struct tc_data data)
{
        if (data.orien_z != FACE_UNKNOWN
            && data.orien_xy != ORIENTATION_UNKNOWN) {
                // This shouldn't happen
                return SENSOR_EVENT_TYPE_TERMINAL_UNKNOWN;
        }

        if (data.orien_xy == ORIENTATION_UNKNOWN) {
                // Face up/down
                switch (data.orien_z) {
                case FACE_UP:
                        return SENSOR_EVENT_TYPE_TERMINAL_FACE_UP;
                case FACE_DOWN:
                        return SENSOR_EVENT_TYPE_TERMINAL_FACE_DOWN;
                default:
                        return SENSOR_EVENT_TYPE_TERMINAL_UNKNOWN;
                }
        }

        if (data.orien_z == FACE_UNKNOWN) {
                // Orientation up/down
                switch (data.orien_xy) {
                case PORTRAIT_UP:
                        return SENSOR_EVENT_TYPE_TERMINAL_PORTRAIT_UP;
                case PORTRAIT_DOWN:
                        return SENSOR_EVENT_TYPE_TERMINAL_PORTRAIT_DOWN;
                case HORIZONTAL_UP:
                        return SENSOR_EVENT_TYPE_TERMINAL_HORIZONTAL_UP;
                case HORIZONTAL_DOWN:
                        return SENSOR_EVENT_TYPE_TERMINAL_HORIZONTAL_DOWN;
                default:
                        return SENSOR_EVENT_TYPE_TERMINAL_UNKNOWN;
                }
        }

        // Shouldn't reach here
        return SENSOR_EVENT_TYPE_TERMINAL_UNKNOWN;
}

int SensorHubHelper::getShakeEvent(struct shaking_data data)
{
        if (data.shaking == SHAKING) {
                return SENSOR_EVENT_TYPE_SHAKE;
        }
        return INVALID_EVENT;
}

int SensorHubHelper::getSimpleTappingEvent(struct stap_data data)
{
        if (data.stap == DOUBLE_TAPPING) {
                return SENSOR_EVENT_TYPE_SIMPLE_TAPPING_DOUBLE_TAPPING;
        } else if(data.stap == SINGLE_TAPPING) {
                return SENSOR_EVENT_TYPE_SIMPLE_TAPPING_SINGLE_TAPPING;
        }
        return INVALID_EVENT;
}

int SensorHubHelper::getMoveDetectEvent(struct md_data data)
{
        switch (data.state) {
        case MD_MOVE:
                return SENSOR_EVENT_TYPE_MOVE_DETECT_MOVE;
        case MD_SLIGHT:
                return SENSOR_EVENT_TYPE_MOVE_DETECT_SLIGHT;
        case MD_STILL:
                return SENSOR_EVENT_TYPE_MOVE_DETECT_STILL;
        default:
                return INVALID_EVENT;
        }
}

size_t SensorHubHelper::getUnitSize(int sensorType)
{
        switch (sensorType) {
        case SENSOR_TYPE_ACCELEROMETER:
                return sizeof(struct accel_data);
        case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
        case SENSOR_TYPE_MAGNETIC_FIELD:
                return sizeof(struct compass_raw_data);
        case SENSOR_TYPE_ORIENTATION:
                return sizeof(struct orientation_data);
        case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
        case SENSOR_TYPE_GYROSCOPE:
                return sizeof(struct gyro_raw_data);
        case SENSOR_TYPE_LIGHT:
                return sizeof(struct als_raw_data);
        case SENSOR_TYPE_PRESSURE:
                return sizeof(struct baro_raw_data);
        case SENSOR_TYPE_PROXIMITY:
                return sizeof(struct ps_phy_data);
        case SENSOR_TYPE_GRAVITY:
                return sizeof(struct gravity_data);
        case SENSOR_TYPE_LINEAR_ACCELERATION:
                return sizeof(struct linear_accel_data);
        case SENSOR_TYPE_ROTATION_VECTOR:
                return sizeof(struct rotation_vector_data);
        case SENSOR_TYPE_WAKE_GESTURE:
                return sizeof(struct stap_data);
        case SENSOR_TYPE_GLANCE_GESTURE:
                return sizeof(struct shaking_data);
        case SENSOR_TYPE_PICK_UP_GESTURE:
                return sizeof(struct pickup_data);
        case SENSOR_TYPE_TILT_DETECTOR:
                return sizeof(struct tilt_detector_data);
        case SENSOR_TYPE_GESTURE_FLICK:
                return sizeof(struct gesture_flick_data);
        case SENSOR_TYPE_GESTURE:
                return sizeof(struct gs_data);
        case SENSOR_TYPE_PHYSICAL_ACTIVITY:
                return sizeof(struct phy_activity_data);
        case SENSOR_TYPE_TERMINAL:
                return sizeof(struct tc_data);
        case SENSOR_TYPE_PEDOMETER:
                return sizeof(struct pedometer_data);
        case SENSOR_TYPE_SHAKE:
                return sizeof(struct shaking_data);
        case SENSOR_TYPE_SIMPLE_TAPPING:
                return sizeof(struct stap_data);
        case SENSOR_TYPE_MOVE_DETECT:
                return sizeof(struct md_data);
        case SENSOR_TYPE_STEP_DETECTOR:
                return sizeof(struct stepdetector_data);
        case SENSOR_TYPE_STEP_COUNTER:
                return sizeof(struct stepcounter_data);
        case SENSOR_TYPE_SIGNIFICANT_MOTION:
                return sizeof(struct sm_data);
        case SENSOR_TYPE_GAME_ROTATION_VECTOR:
                return sizeof(struct game_rotation_vector_data);
        case SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
                return sizeof(struct geomagnetic_rotation_vector_data);
        case SENSOR_TYPE_DEVICE_POSITION:
                return sizeof(struct device_position_data);
        case SENSOR_TYPE_LIFT:
                return sizeof(struct lift_data);
        case SENSOR_TYPE_PAN_ZOOM:
                return sizeof(struct pz_data);
        case SENSOR_TYPE_TEMPERATURE:
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
        case SENSOR_TYPE_AUDIO_CLASSIFICATION:
        case SENSOR_TYPE_CALIBRATION:
        default:
                ALOGE("%s: Unsupported Sensor Type: %d", __FUNCTION__, sensorType);
                break;
        }
        return -1;
}

ssize_t SensorHubHelper::readSensorhubEvents(int fd, struct sensorhub_event_t* events, size_t count, int sensorType)
{
        if (fd < 0)
                return fd;

        if (count <= 0)
                return 0;

        size_t unitSize = getUnitSize(sensorType);
        size_t streamSize = unitSize * count;
        byte* stream = new byte[streamSize];

        streamSize = read(fd, reinterpret_cast<void *>(stream), streamSize);

        if (streamSize % unitSize != 0) {
                ALOGE("%s line: %d: invalid stream size: type: %d size: %d",
                     __FUNCTION__, __LINE__, sensorType, streamSize);
                delete[] stream;
                return -1;
        }

        count = streamSize / unitSize;
        switch (sensorType) {
        case SENSOR_TYPE_ACCELEROMETER:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct accel_data*>(stream))[i].x;
                        events[i].data[1] = (reinterpret_cast<struct accel_data*>(stream))[i].y;
                        events[i].data[2] = (reinterpret_cast<struct accel_data*>(stream))[i].z;
                        events[i].accuracy = getVectorStatus(sensorType);
                        events[i].timestamp = (reinterpret_cast<struct accel_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
        case SENSOR_TYPE_MAGNETIC_FIELD:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct compass_raw_data *>(stream))[i].x;
                        events[i].data[1] = (reinterpret_cast<struct compass_raw_data *>(stream))[i].y;
                        events[i].data[2] = (reinterpret_cast<struct compass_raw_data *>(stream))[i].z;
                        events[i].accuracy = getVectorStatus(sensorType);
                        events[i].timestamp = (reinterpret_cast<struct compass_raw_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_ORIENTATION:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct orientation_data*>(stream))[i].azimuth;
                        events[i].data[1] = (reinterpret_cast<struct orientation_data*>(stream))[i].pitch;
                        events[i].data[2] = (reinterpret_cast<struct orientation_data*>(stream))[i].roll;
                        events[i].accuracy = getVectorStatus(sensorType);
                        events[i].timestamp = (reinterpret_cast<struct orientation_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
        case SENSOR_TYPE_GYROSCOPE:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct gyro_raw_data*>(stream))[i].x;
                        events[i].data[1] = (reinterpret_cast<struct gyro_raw_data*>(stream))[i].y;
                        events[i].data[2] = (reinterpret_cast<struct gyro_raw_data*>(stream))[i].z;
                        events[i].accuracy = getVectorStatus(sensorType);
                        events[i].timestamp = (reinterpret_cast<struct gyro_raw_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_LIGHT:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct als_raw_data *>(stream))[i].lux;
                        events[i].timestamp = (reinterpret_cast<struct als_raw_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_PRESSURE:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct baro_raw_data*>(stream))[i].p;
                        events[i].timestamp = (reinterpret_cast<struct baro_raw_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_PROXIMITY:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct ps_phy_data*>(stream))[i].near == 0 ? 1 : 0;
                        events[i].timestamp = (reinterpret_cast<struct ps_phy_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_GRAVITY:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct gravity_data*>(stream))[i].x;
                        events[i].data[1] = (reinterpret_cast<struct gravity_data*>(stream))[i].y;
                        events[i].data[2] = (reinterpret_cast<struct gravity_data*>(stream))[i].z;
                        events[i].timestamp = (reinterpret_cast<struct gravity_data*>(stream))[i].ts;
                        events[i].accuracy = SENSOR_STATUS_ACCURACY_MEDIUM;
                }
                break;
        case SENSOR_TYPE_LINEAR_ACCELERATION:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct linear_accel_data*>(stream))[i].x;
                        events[i].data[1] = (reinterpret_cast<struct linear_accel_data*>(stream))[i].y;
                        events[i].data[2] = (reinterpret_cast<struct linear_accel_data*>(stream))[i].z;
                        events[i].timestamp = (reinterpret_cast<struct linear_accel_data*>(stream))[i].ts;
                        events[i].accuracy = SENSOR_STATUS_ACCURACY_MEDIUM;
                }
                break;
        case SENSOR_TYPE_ROTATION_VECTOR:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct rotation_vector_data*>(stream))[i].x;
                        events[i].data[1] = (reinterpret_cast<struct rotation_vector_data*>(stream))[i].y;
                        events[i].data[2] = (reinterpret_cast<struct rotation_vector_data*>(stream))[i].z;
                        events[i].data[3] = (reinterpret_cast<struct rotation_vector_data*>(stream))[i].w;
                        events[i].timestamp = (reinterpret_cast<struct rotation_vector_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_WAKE_GESTURE:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = 1.0;
                        events[i].timestamp = (reinterpret_cast<struct stap_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_GLANCE_GESTURE:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = 1.0;
                        events[i].timestamp = (reinterpret_cast<struct shaking_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_PICK_UP_GESTURE:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = 1.0;
                        events[i].timestamp = (reinterpret_cast<struct pickup_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_TILT_DETECTOR:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = 1.0;
                        events[i].timestamp = (reinterpret_cast<struct tilt_detector_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_GESTURE_FLICK:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = getGestureFlickEvent((reinterpret_cast<struct gesture_flick_data*>(stream))[i]);
                        events[i].timestamp = (reinterpret_cast<struct gesture_flick_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_TERMINAL:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = getTerminalEvent((reinterpret_cast<struct tc_data*>(stream))[i]);
                        events[i].timestamp = (reinterpret_cast<struct tc_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_SHAKE:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = getShakeEvent((reinterpret_cast<struct shaking_data*>(stream))[i]);
                        events[i].timestamp = (reinterpret_cast<struct shaking_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_SIMPLE_TAPPING:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = getSimpleTappingEvent((reinterpret_cast<struct stap_data*>(stream))[i]);
                        events[i].timestamp = (reinterpret_cast<struct stap_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_MOVE_DETECT:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = getMoveDetectEvent((reinterpret_cast<struct md_data*>(stream))[i]);
                        events[i].timestamp = (reinterpret_cast<struct md_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_STEP_DETECTOR:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = ((reinterpret_cast<struct stepdetector_data*>(stream))[i]).state;
                        events[i].timestamp = (reinterpret_cast<struct stepdetector_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_STEP_COUNTER:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].step_counter = ((reinterpret_cast<struct stepcounter_data*>(stream))[i]).num;
                        events[i].timestamp = (reinterpret_cast<struct stepcounter_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_SIGNIFICANT_MOTION:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = ((reinterpret_cast<struct sm_data*>(stream))[i]).state;
                        events[i].timestamp = (reinterpret_cast<struct sm_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_GAME_ROTATION_VECTOR:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct game_rotation_vector_data*>(stream))[i].x;
                        events[i].data[1] = (reinterpret_cast<struct game_rotation_vector_data*>(stream))[i].y;
                        events[i].data[2] = (reinterpret_cast<struct game_rotation_vector_data*>(stream))[i].z;
                        events[i].data[3] = (reinterpret_cast<struct game_rotation_vector_data*>(stream))[i].w;
                        events[i].timestamp = (reinterpret_cast<struct game_rotation_vector_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
                for (unsigned int i = 0; i < count; i++) {
                        ALOGI("geomagnetic_rotation_vector event");
                        events[i].data[0] = (reinterpret_cast<struct geomagnetic_rotation_vector_data*>(stream))[i].x;
                        events[i].data[1] = (reinterpret_cast<struct geomagnetic_rotation_vector_data*>(stream))[i].y;
                        events[i].data[2] = (reinterpret_cast<struct geomagnetic_rotation_vector_data*>(stream))[i].z;
                        events[i].data[3] = (reinterpret_cast<struct geomagnetic_rotation_vector_data*>(stream))[i].w;
                        events[i].timestamp = (reinterpret_cast<struct geomagnetic_rotation_vector_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_DEVICE_POSITION:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct device_position_data*>(stream))[i].pos;
                        events[i].timestamp = (reinterpret_cast<struct device_position_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_LIFT:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct lift_data*>(stream))[i].look;
                        events[i].data[1] = (reinterpret_cast<struct lift_data*>(stream))[i].vertical;
                        events[i].timestamp = (reinterpret_cast<struct lift_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_PAN_ZOOM:
                for (unsigned int i = 0; i < count; i++) {
                        events[i].data[0] = (reinterpret_cast<struct pz_data*>(stream))[i].deltX;
                        events[i].data[1] = (reinterpret_cast<struct pz_data*>(stream))[i].deltY;
                        events[i].timestamp = (reinterpret_cast<struct pz_data*>(stream))[i].ts;
                }
                break;
        case SENSOR_TYPE_TEMPERATURE:
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
        case SENSOR_TYPE_AUDIO_CLASSIFICATION:
        case SENSOR_TYPE_GESTURE:
        case SENSOR_TYPE_PHYSICAL_ACTIVITY:
        case SENSOR_TYPE_PEDOMETER:
        case SENSOR_TYPE_CALIBRATION:
        default:
                ALOGE("%s: Unsupported Sensor Type: %d", __FUNCTION__, sensorType);
                break;
        }

        delete[] stream;

        return count;
}

#include <sys/inotify.h>
#include <pthread.h>
#include <fcntl.h>

#define ACCELERATION_CALIBATION_STATUS (1 << 0)
#define MAGNETIC_CALIBATION_STATUS     (1 << 1)
#define GYROSCOPE_CALIBATION_STATUS    (1 << 2)

static void update_vector_status(int fd, int* vector_status, int flag)
{
        struct cmd_calibration_param param;
        pread(fd, &param, sizeof(param), 0);

        if (param.calibrated == SUBCMD_CALIBRATION_TRUE)
                *(int*)vector_status |= flag;
        else
                *(int*)vector_status &= ~flag;
}

static int monitor_calibration_flies(const char* file, int fd_inotify, int* vector_status, int flag, int* wd)
{
        int fd = -1;
        fd = open(file, O_RDONLY);
        if (fd < 0) {
                ALOGW("Cannot open file: %s %d", file, fd);
                *(int*)vector_status |= flag;
        } else {
                update_vector_status(fd, (int*)vector_status, flag);
                if (fd_inotify >= 0) {
                        *wd = inotify_add_watch(fd_inotify, file, IN_MODIFY);
                        if (*wd < 0) {
                                ALOGE("Cannot watch file: %s %d", file, *wd);
                        }
                }
        }

        return fd;
}

static void* vector_status_monitor(void* vector_status)
{
        int fd_accl, fd_comp, fd_gyro, fd_inotify;
        int wd_accl = -1, wd_comp = -1, wd_gyro = -1;
        fd_set rfds;
        int ret = -1;
        struct inotify_event ievent[16];

        fd_inotify = inotify_init();
        if (fd_inotify < 0) {
                ALOGE("Cannot initialize inotify!, %s", strerror(errno));
        }

        fd_accl = monitor_calibration_flies(ACCELERATION_CALIBATION_FILE, fd_inotify, (int*)vector_status, ACCELERATION_CALIBATION_STATUS, &wd_accl);
        fd_comp = monitor_calibration_flies(MAGNETIC_CALIBATION_FILE, fd_inotify, (int*)vector_status, MAGNETIC_CALIBATION_STATUS, &wd_comp);
        fd_gyro = monitor_calibration_flies(GYROSCOPE_CALIBATION_FILE, fd_inotify, (int*)vector_status, GYROSCOPE_CALIBATION_STATUS, &wd_gyro);

        if ((fd_accl < 0 && fd_comp < 0 && fd_gyro < 0) || fd_inotify < 0)
                goto exit_monitor;

        while (true) {
                bool accl_change = false, comp_change = false, gyro_change = false;
                FD_ZERO (&rfds);
                FD_SET (fd_inotify, &rfds);
                ret = select (fd_inotify + 1, &rfds, NULL, NULL, NULL);
                if (ret <= 0) {
                        ALOGE("select fd_inotify error: %d", ret);
                        continue;
                }
                ret = read(fd_inotify, ievent, 16 * sizeof(struct inotify_event));
                if (ret < 0) {
                        ALOGE("read fd_inotify error: %d", ret);
                        continue;
                }
                for (int i = 0; i < ret / sizeof(struct inotify_event); i++) {
                        if (ievent[i].wd == wd_accl) {
                                accl_change = true;
                        } else if (ievent[i].wd == wd_comp) {
                                comp_change = true;
                        } else if (ievent[i].wd == wd_gyro) {
                                gyro_change = true;
                        }
                }

                if (accl_change && fd_accl >= 0) {
                        update_vector_status(fd_accl, (int*)vector_status, ACCELERATION_CALIBATION_STATUS);
                }
                if (comp_change && fd_comp >= 0) {
                        update_vector_status(fd_comp, (int*)vector_status, MAGNETIC_CALIBATION_STATUS);
                }
                if (gyro_change && fd_gyro >= 0) {
                        update_vector_status(fd_gyro, (int*)vector_status, GYROSCOPE_CALIBATION_STATUS);
                }
        }

exit_monitor:
        if (fd_inotify >= 0)
                close(fd_inotify);
        if (fd_accl >= 0)
                close(fd_accl);
        if (fd_comp >= 0)
                close(fd_comp);
        if (fd_gyro >= 0)
                close(fd_gyro);

        return NULL;
}

int8_t SensorHubHelper::getVectorStatus(int sensorType)
{
        static int vector_status = 0;
        static pthread_t tid;
        static bool thread_created_success = false;
        int err;

        if (!thread_created_success) {
                err = pthread_create(&tid, NULL, vector_status_monitor, &vector_status);
                if (err) {
                        ALOGE("vector_status_monitor thread create error: %d", err);
                        thread_created_success = false;
                        return SENSOR_STATUS_UNRELIABLE;
                }
                thread_created_success = true;
        }

        switch (sensorType) {
        case SENSOR_TYPE_ACCELEROMETER:
                return (vector_status & ACCELERATION_CALIBATION_STATUS) ? SENSOR_STATUS_ACCURACY_HIGH : SENSOR_STATUS_ACCURACY_LOW;
        case SENSOR_TYPE_MAGNETIC_FIELD:
                return (vector_status & MAGNETIC_CALIBATION_STATUS) ? SENSOR_STATUS_ACCURACY_HIGH : SENSOR_STATUS_ACCURACY_LOW;
        case SENSOR_TYPE_GYROSCOPE:
                return (vector_status & GYROSCOPE_CALIBATION_STATUS) ? SENSOR_STATUS_ACCURACY_HIGH : SENSOR_STATUS_ACCURACY_LOW;
        case SENSOR_TYPE_ORIENTATION:
                if (vector_status == (ACCELERATION_CALIBATION_STATUS | MAGNETIC_CALIBATION_STATUS | GYROSCOPE_CALIBATION_STATUS))
                        return SENSOR_STATUS_ACCURACY_HIGH;
                else
                        return SENSOR_STATUS_ACCURACY_LOW;
        default:
                return SENSOR_STATUS_UNRELIABLE;
        }

        return SENSOR_STATUS_UNRELIABLE;
}
