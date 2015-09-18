/* Android Modem Traces and Logs
 *
 * Copyright (C) Intel 2012
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __OPENGSMTTY_JNI_H__
#define __OPENGSMTTY_JNI_H__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     com_intel_amtl_modem_Gsmtty
 * Method:    OpenSerial
 * Signature: (Ljava/lang/String;I)I;
 */

JNIEXPORT jint JNICALL Java_com_intel_amtl_modem_Gsmtty_OpenSerial(JNIEnv *,
        jobject, jstring, jint);
/*
 * Class:     com_intel_amtl_modem_Gsmtty
 * Method:    CloseSerial
 * Signature: (I)I;
 */
JNIEXPORT jint JNICALL Java_com_intel_amtl_modem_Gsmtty_CloseSerial(JNIEnv *, jobject, jint);

#ifdef __cplusplus
}
#endif

#endif //__OPENGSMTTY_JNI_H_
