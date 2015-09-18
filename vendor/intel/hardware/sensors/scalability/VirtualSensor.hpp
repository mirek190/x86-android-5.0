#ifndef VIRTUAL_SENSOR_HPP
#define VIRTUAL_SENSOR_HPP

/*****************************************************************************/
// Definition for virtual sensors, it's one-one map to the user space library
// Sensor types
#define SENSOR_TYPE_GESTURE_FLICK           SENSOR_TYPE_DEVICE_PRIVATE_BASE + 1
#define SENSOR_TYPE_GESTURE                 SENSOR_TYPE_DEVICE_PRIVATE_BASE + 2
#define SENSOR_TYPE_PHYSICAL_ACTIVITY       SENSOR_TYPE_DEVICE_PRIVATE_BASE + 3
#define SENSOR_TYPE_TERMINAL                SENSOR_TYPE_DEVICE_PRIVATE_BASE + 4
#define SENSOR_TYPE_AUDIO_CLASSIFICATION    SENSOR_TYPE_DEVICE_PRIVATE_BASE + 5
#define SENSOR_TYPE_PEDOMETER               SENSOR_TYPE_DEVICE_PRIVATE_BASE + 6
#define SENSOR_TYPE_SHAKE                   SENSOR_TYPE_DEVICE_PRIVATE_BASE + 7
#define SENSOR_TYPE_SIMPLE_TAPPING          SENSOR_TYPE_DEVICE_PRIVATE_BASE + 8
#define SENSOR_TYPE_MOVE_DETECT             SENSOR_TYPE_DEVICE_PRIVATE_BASE + 9
#define SENSOR_TYPE_GESTURE_EARTOUCH        SENSOR_TYPE_DEVICE_PRIVATE_BASE + 10
#define SENSOR_TYPE_GESTURE_HMM             SENSOR_TYPE_DEVICE_PRIVATE_BASE + 11
#define SENSOR_TYPE_DEVICE_POSITION         SENSOR_TYPE_DEVICE_PRIVATE_BASE + 12
#define SENSOR_TYPE_LIFT                    SENSOR_TYPE_DEVICE_PRIVATE_BASE + 13
#define SENSOR_TYPE_PAN_ZOOM                SENSOR_TYPE_DEVICE_PRIVATE_BASE + 14
#define SENSOR_TYPE_CALIBRATION             110

// SENSOR_STRING_TYPE_XXX
#define SENSOR_STRING_TYPE_GESTURE_FLICK        "intel.sensor.gesture_flick"
#define SENSOR_STRING_TYPE_GESTURE              "intel.sensor.gesture"
#define SENSOR_STRING_TYPE_PHYSICAL_ACTIVITY    "intel.sensor.physical_activity"
#define SENSOR_STRING_TYPE_TERMINAL             "intel.sensor.terminal"
#define SENSOR_STRING_TYPE_AUDIO_CLASSIFICATION "intel.sensor.audio_classificataion"
#define SENSOR_STRING_TYPE_PEDOMETER            "intel.sensor.pedometer"
#define SENSOR_STRING_TYPE_SHAKE                "intel.sensor.shake"
#define SENSOR_STRING_TYPE_SIMPLE_TAPPING       "intel.sensor.simple_tapping"
#define SENSOR_STRING_TYPE_MOVE_DETECT          "intel.sensor.move_detect"
#define SENSOR_STRING_TYPE_GESTURE_EARTOUCH     "intel.sensor.gseture_eartouch"
#define SENSOR_STRING_TYPE_GESTURE_HMM          "intel.sensor.gesture_hmm"
#define SENSOR_STRING_TYPE_DEVICE_POSITION      "intel.sensor.device_position"
#define SENSOR_STRING_TYPE_LIFT                 "intel.sensor.lift"
#define SENSOR_STRING_TYPE_PAN_ZOOM             "intel.sensor.pan_zoom"
#define SENSOR_STRING_TYPE_CALIBRATION          "intel.sensor.calibration"


// Sensor event types

#define SENSOR_EVENT_TYPE_GESTURE_LEFT_FLICK        (1)
#define SENSOR_EVENT_TYPE_GESTURE_RIGHT_FLICK       (2)
#define SENSOR_EVENT_TYPE_GESTURE_UP_FLICK          (3)
#define SENSOR_EVENT_TYPE_GESTURE_DOWN_FLICK        (4)
#define SENSOR_EVENT_TYPE_GESTURE_LEFT_FLICK_TWICE  (5)
#define SENSOR_EVENT_TYPE_GESTURE_RIGHT_FLICK_TWICE (6)
#define SENSOR_EVENT_TYPE_GESTURE_NO_FLICK          (7)

#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_ONE            (1)
#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_TWO            (2)
#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_THREE          (3)
#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_FOUR           (4)
#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_FIVE           (5)
#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_SIX            (6)
#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_SEVEN          (7)
#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_EIGHT          (8)
#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_NINE           (9)
#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_ZERO           (10)
#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_EAR_TOUCH      (17)
#define SENSOR_EVENT_TYPE_GESTURE_NUMBER_EAR_TOUCH_BACK (18)

#define SENSOR_EVENT_TYPE_PHYSICAL_ACTIVITY_BIKING      (1)
#define SENSOR_EVENT_TYPE_PHYSICAL_ACTIVITY_WALKING     (2)
#define SENSOR_EVENT_TYPE_PHYSICAL_ACTIVITY_RUNNING     (3)
#define SENSOR_EVENT_TYPE_PHYSICAL_ACTIVITY_INCAR       (4)
#define SENSOR_EVENT_TYPE_PHYSICAL_ACTIVITY_JUMPING     (5)
#define SENSOR_EVENT_TYPE_PHYSICAL_ACTIVITY_RANDOM      (6)
#define SENSOR_EVENT_TYPE_PHYSICAL_ACTIVITY_SEDENTARY   (7)

#define SENSOR_EVENT_TYPE_TERMINAL_FACE_UP              (1)
#define SENSOR_EVENT_TYPE_TERMINAL_FACE_DOWN            (2)
#define SENSOR_EVENT_TYPE_TERMINAL_PORTRAIT_UP          (3)
#define SENSOR_EVENT_TYPE_TERMINAL_PORTRAIT_DOWN        (4)
#define SENSOR_EVENT_TYPE_TERMINAL_HORIZONTAL_UP        (5)
#define SENSOR_EVENT_TYPE_TERMINAL_HORIZONTAL_DOWN      (6)
#define SENSOR_EVENT_TYPE_TERMINAL_UNKNOWN              (7)

#define SENSOR_EVENT_TYPE_AUDIO_CLASSIFICATION_CROWD            (1)
#define SENSOR_EVENT_TYPE_AUDIO_CLASSIFICATION_SOFT_MUSIC       (2)
#define SENSOR_EVENT_TYPE_AUDIO_CLASSIFICATION_MECHANICAL       (3)
#define SENSOR_EVENT_TYPE_AUDIO_CLASSIFICATION_MOTION           (4)
#define SENSOR_EVENT_TYPE_AUDIO_CLASSIFICATION_MALE_SPEECH      (5)
#define SENSOR_EVENT_TYPE_AUDIO_CLASSIFICATION_FEMALE_SPEECH    (6)
#define SENSOR_EVENT_TYPE_AUDIO_CLASSIFICATION_SILENT           (7)
#define SENSOR_EVENT_TYPE_AUDIO_CLASSIFICATION_UNKNOWN          (8)

#define SENSOR_EVENT_TYPE_SHAKE         (1)
#define SENSOR_EVENT_TYPE_SIMPLE_TAPPING_SINGLE_TAPPING         (1)
#define SENSOR_EVENT_TYPE_SIMPLE_TAPPING_DOUBLE_TAPPING         (2)

#define SENSOR_EVENT_TYPE_MOVE_DETECT_STILL             (1)
#define SENSOR_EVENT_TYPE_MOVE_DETECT_SLIGHT            (2)
#define SENSOR_EVENT_TYPE_MOVE_DETECT_MOVE              (3)

// Sensor delay types
#define SENSOR_DELAY_TYPE_PHYSICAL_ACTIVITY_INSTANT     (((1 << 3) + 1) * 1000)
#define SENSOR_DELAY_TYPE_PHYSICAL_ACTIVITY_QUICK       (((1 << 3) + 2) * 1000)
#define SENSOR_DELAY_TYPE_PHYSICAL_ACTIVITY_NORMAL      (((1 << 3) + 3) * 1000)
#define SENSOR_DELAY_TYPE_PHYSICAL_ACTIVITY_STATISTIC   (((1 << 3) + 4) * 1000)

#define SENSOR_DELAY_TYPE_AUDIO_CLASSIFICATION_INSTANT  (((1 << 3) + 5) * 1000)
#define SENSOR_DELAY_TYPE_AUDIO_CLASSIFICATION_SHORT    (((1 << 3) + 6) * 1000)
#define SENSOR_DELAY_TYPE_AUDIO_CLASSIFICATION_MEDIUM   (((1 << 3) + 7) * 1000)
#define SENSOR_DELAY_TYPE_AUDIO_CLASSIFICATION_LONG     (((1 << 3) + 8) * 1000)

#define SENSOR_DELAY_TYPE_PEDOMETER_INSTANT         (((1 << 3) + 9) * 1000)
#define SENSOR_DELAY_TYPE_PEDOMETER_QUICK           (((1 << 3) + 10) * 1000)
#define SENSOR_DELAY_TYPE_PEDOMETER_NORMAL          (((1 << 3) + 11) * 1000)
#define SENSOR_DELAY_TYPE_PEDOMETER_STATISTIC       (((1 << 3) + 12) * 1000)

/*****************************************************************************/

// PSH session related
// gesture sensor
#define GS_BUF_SIZE     512     /* accel & gyro buffer size */
#define PX_BUF_SIZE     512     /* proximity buffer size */
#define GS_SAMPLE_RATE  50      /* accel & gyro sampling rate, Hz */
#define GS_BUF_DELAY    0       /* accel & gyro buffer delay, ms */
#define PX_SAMPLE_RATE  5       /* proximity sampling rate, Hz */
#define PX_BUF_DELAY    0       /* proximity buffer delay, ms */
#define GS_DATA_LENGTH  6       /* length of input single data */
#define INVALID_GESTURE_RESULT -1

// gesture flick sensor
#define GF_SAMPLE_RATE  20      /* gesture flick sampling rate, Hz */
#define GF_BUF_DELAY    0       /* gesture flick buffer delay, ms */
#define PSH_SESSION_NOT_OPENED NULL

typedef enum { /* definition flick gestures value */
        NO_FLICK = 0,
        LEFT_FLICK,
        RIGHT_FLICK,
        UP_FLICK,
        DOWN_FLICK,
        LEFT_TWICE,
        RIGHT_TWICE,
        FLICK_GESTURES_MAX = 0x7FFFFFFF
} FlickGestures;

// physical activity sensor
#define SLEEP_ON_FAIL_USEC 500000

#define PA_INTERVAL (static_cast<float>(5.12))
#define PA_QUICK (static_cast<float>(5.12))
#define PA_NORMAL (static_cast<float>(40.96))
#define PA_STATISTIC (static_cast<float>(163.84))


// pedometer sensor
#define SLEEP_ON_FAIL_USEC 500000
#define PEDO_RATE 1
#define PEDO_BUFD 0
/*This flag makes pedometer context
  continue computing but not report
  when the screen is off
 */
#define PEDO_FLAG NO_STOP_NO_REPORT_WHEN_SCREEN_OFF

#define PEDO_QUICK 4
#define PEDO_NORMAL 32
#define PEDO_STATISTIC 256
// eartouch sensor
#define EARTOUCH              1
#define EARTOUCH_BACK         2

// shake sensor
#define SHAKING_BUF_SIZE            512
#define SHAKING_SAMPLE_RATE         20
#define SHAKING_BUF_DELAY           0
#define SHAKE_SEN_LOW               "{\"sensitivity\":0}"
#define SHAKE_SEN_MEDIUM            "{\"sensitivity\":1}"
#define SHAKE_SEN_HIGH              "{\"sensitivity\":2}"
#define SHAKING 1

// simple tapping sensor
#define STAP_BUF_SIZE            512
#define STAP_SAMPLE_RATE         20
#define STAP_BUF_DELAY           0
#define STAP_CLSMASK_SINGLE      "{\"classMask\":1,\"level\":0}"
#define STAP_CLSMASK_DOUBLE      "{\"classMask\":2,\"level\":0}"
#define STAP_CLSMASK_BOTH        "{\"classMask\":3,\"level\":0}"
#define STAP_CLSMASK_FOR_WAKE    "{\"classMask\":2,\"level\":-5}"
#define STAP_DEFAULT_LEVEL       0
#define DOUBLE_TAPPING 2
#define SINGLE_TAPPING 1
#define INVALID_EVENT -1

// move detect sensor
#define MD_UNKNOW 0
#define MD_MOVE 1
#define MD_SLIGHT 2
#define MD_STILL 3

// terminal sensor
#define TERM_RATE 20
#define TERM_DELAY (1000/TERM_RATE)
#define FACE_UNKNOWN 0
#define FACE_UP 1
#define FACE_DOWN 2
#define ORIENTATION_UNKNOWN 0
#define HORIZONTAL_UP 1
#define HORIZONTAL_DOWN 2
#define PORTRAIT_UP 3
#define PORTRAIT_DOWN 4

// some common definitions
#define THREAD_NOT_STARTED 0
#define PIPE_NOT_OPENED -1
#define PSH_SESSION_NOT_OPENED NULL

// some common functions defined in class
#include <queue>
#include <hardware/sensors.h>
class VirtualLogical {
public:
        VirtualLogical(int type, int handle) {
                this->handle = handle;
                memset(&event, 0, sizeof(sensors_event_t));
                event.version = sizeof(sensors_event_t);
                event.sensor = handle;
                event.type = type;

                memset(&metaEvent, 0, sizeof(metaEvent));
                metaEvent.version = META_DATA_VERSION;
                metaEvent.type = SENSOR_TYPE_META_DATA;
                metaEvent.meta_data.sensor = handle;
                metaEvent.meta_data.what = META_DATA_FLUSH_COMPLETE;
        }
        virtual ~VirtualLogical() {
        }
        virtual int getPollfd() = 0;
        virtual int activate(int handle, int enabled) {
                return 0;
        }
        virtual int setDelay(int handle, int64_t ns) {
                return 0;
        }
        virtual int getData(std::queue<sensors_event_t> &eventQue) = 0;
        virtual bool selftest() = 0;
        virtual int flush(int handle) = 0;
        virtual void resetEventHandle(int handle) = 0;
        class SensorState {
                bool activated;
                bool flushSuccess;
                int delay;
        public:
                SensorState() {
                        activated = false;
                        flushSuccess = false;
                        delay = 200;
                }
                bool getActivated() {
                        return activated;
                }
                void setActivated(bool new_activated) {
                        activated = new_activated;
                }
                bool getFlushSuccess() {
                        return flushSuccess;
                }
                void setFlushSuccess(bool new_flushSuccess) {
                        flushSuccess = new_flushSuccess;
                }
                int getDelay() {
                        return delay;
                }
                void setDelay(int new_delay) {
                        delay = new_delay;
                }

        };
        SensorState state;
        sensors_event_t event;
        sensors_meta_data_event_t metaEvent;
        int handle;
};

#endif
