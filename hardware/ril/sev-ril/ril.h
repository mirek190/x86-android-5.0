#define LOG_TAG "sev-ril"

#include <utils/Log.h>
#include <android/log.h>

#ifdef LOG_NDEBUG
#undef LOG_NDEBUG
#define LOG_NDEBUG 0
#endif

#define MODE_GSM        1       //GSM/TDSCDMA
#define MODE_CDMA       2      //CDMA


/* Begin added by x84000798 for android4.1 Jelly Bean DTS2012101601022 2012-10-16 */	
#ifndef ALOGI
#define ALOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#endif
#ifndef ALOGD
#define ALOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#endif
#ifndef ALOGE
#define ALOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args) 
#endif
#ifndef ALOGW
#define ALOGW(fmt, args...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, fmt, ##args)
#endif