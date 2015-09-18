/*
 * Copyright (C) Intel 2010
 *
 *	ma, xindong <xindong.ma@intel.com> Su Xuemin <xuemin.su@intel.com> xiaobing tu <xiaobing.tu@intel.com>
 *
 */

#include "JNIHelp.h"
#include "jni.h"
#include "cutils/log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cutils/log.h"
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/rtc.h>
#include <errno.h>



#include <cutils/properties.h>

namespace android {

extern "C" int read_all_process_info(FILE *f, int pid ,int tid);
static void android_server_am_DebugAnr_logToFile(JNIEnv* env, jobject obj, jstring idObj) {

//pass name from java
    if (idObj == NULL) {
        jniThrowNullPointerException(env, "id is null");
        return ;
    }
    const char *name = env->GetStringUTFChars(idObj, NULL);

    FILE *fp = fopen(name, "w");
    if (fp == NULL) {
	    ALOGE("Create file failed %s:%s\n", name, strerror(errno));
	    return;
    }
    env->ReleaseStringUTFChars(idObj, name);
    read_all_process_info(fp, 0, 0);
    fclose(fp);
}
static void android_server_am_DebugAnr_logNameToTrace(JNIEnv* env, jobject obj,jstring idObj, jstring idobj1) {

    if (idObj == NULL  || idobj1 == NULL) {
	    jniThrowNullPointerException(env, "id is null");
	    return ;
    }
    const char *name = env->GetStringUTFChars(idObj, NULL);
    const char *filename = env->GetStringUTFChars(idobj1, NULL);
    char file[100];
    memset(file, 0, 100);
    FILE *fp = NULL;
    char davik_anr_path[200] = {0,};

    property_get("dalvik.vm.stack-trace-file", davik_anr_path, "");

    if(filename)
	    fp = fopen(filename, "a");
    else
	    fp = fopen(davik_anr_path,"a");

    if (fp == NULL) {
	    return;
    }

    snprintf(file, sizeof(file), "Trace file:/data/anr/%s.txt", name);
    fputs(file,fp);
    env->ReleaseStringUTFChars(idObj, name);
    env->ReleaseStringUTFChars(idobj1, filename);

    fclose(fp);
}


static JNINativeMethod sMethods[] = {
	/* name, signature, funcPtr */
	{ "logToFile", "(Ljava/lang/String;)V", (void*) android_server_am_DebugAnr_logToFile },
	{ "logname",   "(Ljava/lang/String;Ljava/lang/String;)V",(void*)android_server_am_DebugAnr_logNameToTrace},};

int register_android_server_am_DebugAnr(JNIEnv* env) {
	return jniRegisterNativeMethods(env, "com/android/server/am/DebugAnr",
			sMethods, NELEM(sMethods));
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
	    LOGE("GetEnv failed!");
	    return result;
    }
    ALOGE_IF(!env, "Could not retrieve the env!");

    register_android_server_am_DebugAnr(env);

    return JNI_VERSION_1_4;
}

}
