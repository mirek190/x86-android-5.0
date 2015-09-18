/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "videoclient.h"
#include <jni.h>
#include <string.h>

#include <stdio.h>
#include <sstream>
#include <android/log.h>
#include <cstdio>
#include <cstring>
#include <time.h>
#include <iomanip>
#include <iostream>
#include <vector>

#include "org_webrtc_videoP2P_VideoClient.h"

#include "third_party/webrtc/system_wrappers/interface/thread_wrapper.h"

#include "third_party/webrtc/voice_engine/include/voe_base.h"

//start of includes for login
#include "talk/base/logging.h"
#include "talk/base/thread.h"
#include "talk/xmpp/xmppclientsettings.h"
#include "kxmppthread.h"
#include "talk/xmpp/xmppengine.h"
#include "talk/base/ssladapter.h"
// webrtc media engine
#include "talk/media/base/mediaengine.h"
#include "callclient.h"
#include "talk/base/physicalsocketserver.h"
//end of includes for login

// Video/Video Capture
#include "third_party/webrtc/modules/video_capture/include/video_capture_factory.h"
#include "third_party/webrtc/modules/video_render/include/video_render.h"
#include "third_party/webrtc/modules/video_capture/android/video_capture_android.h"

#include "third_party/webrtc/video_engine/include/vie_base.h"

#if LOGGING
#define LOG_TAG "c-libjingle-VideoClient" // As in WEBRTC Native...
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#else
#define LOGI(...) (void)0
#define LOGE(...) (void)0
#endif

#define WEBRTC_LOG_TAG  "*WEBRTCN-VideoClient*"

#include "trace.h"
#include <android/log.h>

using namespace webrtc;

JavaVM *gJavaVM;
static jobject gInterfaceObject;
const char *kInterfacePath = "org/webrtc/videoP2P/VideoClient";


//////////////////////////////////////////////////////////////////
// General functions
//////////////////////////////////////////////////////////////////

/////////////////////////////////////////////
// JNI_OnLoad
//
jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{

  gJavaVM = vm;
  if (!vm)
  {
    LOGE("JNI_OnLoad did not receive a valid VM pointer");
    return -1;
  }

  // Get JNI
  JNIEnv* env;
  if (JNI_OK != vm->GetEnv(reinterpret_cast<void**> (&env),
                           JNI_VERSION_1_4))
  {
    LOGE("JNI_OnLoad could not get JNI env");
    return -1;
  }

  initClassHelper(env, kInterfacePath, &gInterfaceObject);

  return JNI_VERSION_1_4;
}

/////////////////////////////////////////////
// Native initialization
//
JNIEXPORT jboolean JNICALL
Java_org_webrtc_videoP2P_VideoClient_NativeInit(
        JNIEnv * env,
        jclass)
{
  return true;
}



JNIEXPORT jint JNICALL Java_org_webrtc_videoP2P_VideoClient_Init(
                                                                         JNIEnv *,
                                                                         jobject,
                                                                         jboolean enableTrace,
                                                                         jboolean useExtTrans)
{
    return true;
}

/////////////////////////////////////////////
// Testing XMPP log in.
//
KXmppThread* xmpp_thread_ = NULL;

JNIEXPORT jint JNICALL
Java_org_webrtc_videoP2P_VideoClient_Login(
        JNIEnv * env,
        jobject context, jstring username, jstring password, jstring loginDomain, jstring loginServer, jboolean useSSL)
{
  const char* usernameNative = env->GetStringUTFChars(username, NULL);
  const char* passwordNative = env->GetStringUTFChars(password, NULL);
  const char* loginDomainNative = env->GetStringUTFChars(loginDomain, NULL);
  const char* loginServerNative = env->GetStringUTFChars(loginServer, NULL);

  if(!usernameNative || !passwordNative || !loginDomainNative)
  {
    LOGE("Username/Password or Domain not set");
    return -1;
  }

  LOGI("Login username=%s, password=%s", usernameNative, passwordNative);
  if(xmpp_thread_){
    Terminate();
  }

  VideoEngine::SetAndroidObjects(gJavaVM, context);
  VoiceEngine::SetAndroidObjects(gJavaVM, env, context);

  talk_base::LogMessage::LogThreads();
  talk_base::LogMessage::LogTimestamps();

  if(useSSL){
    talk_base::InitializeSSL();
  }

  LOGI("Creating new KXmppThread");
  xmpp_thread_ = new KXmppThread();
  xmpp_thread_->Start();

  //Call Client
  std::string caps_node = "http://www.google.com/xmpp/client/caps";
  std::string caps_ver = "1.0";

  int32 portallocator_flags = 0;
  cricket::SignalingProtocol initial_protocol = cricket::PROTOCOL_JINGLE;

  buzz::XmppClientSettings xcs;
  xcs.set_user(usernameNative);//WEBRTC_LOGIN_USER);//your username
  xcs.set_resource("rtcdemo.");
  xcs.set_host(loginDomainNative);
  //__android_log_print(ANDROID_LOG_DEBUG, WEBRTC_LOG_TAG,
    //                  "username %s pwd %s domain %s", usernameNative,passwordNative, 
      //                loginDomainNative);
  if(useSSL){
    xcs.set_use_tls(buzz::TLS_REQUIRED);
  } else {
    xcs.set_use_tls(buzz::TLS_DISABLED);
  }
  //Plain config
  xcs.set_allow_plain(!useSSL);

  talk_base::InsecureCryptStringImpl pass;
  pass.password() = passwordNative;//WEBRTC_LOGIN_PASS;//Your password

  xcs.set_pass(talk_base::CryptString(pass));
  xcs.set_server(talk_base::SocketAddress(loginServerNative, 5222));

  //This at the bottom. Call client listens for errors on xmpp's clients.
  xmpp_thread_->Login(xcs);

  env->ReleaseStringUTFChars(username, usernameNative);
  env->ReleaseStringUTFChars(password, passwordNative);
  env->ReleaseStringUTFChars(loginDomain, loginDomainNative);
  env->ReleaseStringUTFChars(loginServer, loginServerNative);
  /*if(isAttached) {
    gJavaVM->DetachCurrentThread();
  }*/

  return 0;
}

JNIEXPORT jint JNICALL
Java_org_webrtc_videoP2P_VideoClient_PlaceCall(
        JNIEnv * env,
        jobject, jstring user, jboolean video, jboolean audio)
{
    LOGI("PlaceCall video=%d, audio=%d", video, audio);
    int status;
    bool isAttached = false;

    status = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
    if(status < 0) {
      LOGI("callback_handler_ext: failed to get JNI environment, assuming native thread");
      status = gJavaVM->AttachCurrentThread(&env, NULL);
      if(status < 0) {
        LOGE("ashoka: place call: failed to attach "
        "current thread");
        return -1;
      }
      isAttached = true;
    }

  const char* userNative = env->GetStringUTFChars(user, NULL);
  if(userNative && xmpp_thread_){
    xmpp_thread_->MakeCall(userNative, video, audio);
    env->ReleaseStringUTFChars(user, userNative);
  } else {
    LOGE("Username not set, or not yet initialized");
  }

  if(isAttached) {
     gJavaVM->DetachCurrentThread();
   }
  return 0;
}

JNIEXPORT jint JNICALL
Java_org_webrtc_videoP2P_VideoClient_AcceptCall(
        JNIEnv *,
        jobject, jboolean video, jboolean audio)
{
  LOGI("AcceptCall video=%d, audio=%d", video, audio);
  if(xmpp_thread_ ){
    xmpp_thread_->AnswerCall(video, audio);
  }
  return 0;
}

JNIEXPORT jint JNICALL
Java_org_webrtc_videoP2P_VideoClient_HangUp(
        JNIEnv *,
        jobject)
{
  LOGI("HangUp");
  if(xmpp_thread_){
    xmpp_thread_->HangUp();
  }
  return 0;
}

JNIEXPORT jint JNICALL
Java_org_webrtc_videoP2P_VideoClient_RejectCall(
        JNIEnv *,
        jobject)
{
  LOGI("RejectCall");
  if(xmpp_thread_){
    xmpp_thread_->RejectCall();
  }
  return 0;
}

JNIEXPORT jstring JNICALL
Java_org_webrtc_videoP2P_VideoClient_GetCaller(
        JNIEnv * env,
        jobject)
{
  jstring js = NULL;
  LOGI("GetCaller");
  if(xmpp_thread_){
    std::string caller;
    caller = xmpp_thread_->GetCaller();
    js = env->NewStringUTF(caller.c_str());
  }

  return js;
}

JNIEXPORT jboolean JNICALL
Java_org_webrtc_videoP2P_VideoClient_SetVideo(
        JNIEnv *,
        jobject, jboolean enable)
{
  LOGI("SetVideo = %d", enable);
  if(xmpp_thread_){
    return(xmpp_thread_->SetVideo(enable));
  }

  return false;
}

JNIEXPORT jboolean JNICALL
Java_org_webrtc_videoP2P_VideoClient_SetVoice(
        JNIEnv *,
        jobject, jboolean enable)
{
  LOGI("SetVoice = %d", enable);
  if(xmpp_thread_){
    return(xmpp_thread_->SetVoice(enable));
  }

  return false;
}

JNIEXPORT jboolean JNICALL
    Java_org_webrtc_videoP2P_VideoClient_IsTestModeActive
  (JNIEnv *, jobject ) {
  LOGI("IsTestModeActive");
#ifdef VIDEOP2P_TESTMODE
  return true;
#else
  return false;
#endif
}

JNIEXPORT jboolean JNICALL
    Java_org_webrtc_videoP2P_VideoClient_IsIncomingCallSupportVideo
  (JNIEnv *, jobject ) {
  LOGI("IsIncomingCallSupportVideo");
  if(xmpp_thread_) {
    return xmpp_thread_->client_->IsIncomingCall() &&
           xmpp_thread_->client_->IsIncomingCallSupportVideo();
  }
  return false;
}

JNIEXPORT jboolean JNICALL
    Java_org_webrtc_videoP2P_VideoClient_IsIncomingCallSupportAudio
  (JNIEnv *, jobject ) {
  LOGI("IsIncomingCallSupportAudio");
  if(xmpp_thread_) {
    return xmpp_thread_->client_->IsIncomingCall() &&
           xmpp_thread_->client_->IsIncomingCallSupportAudio();
  }
  return false;
}

JNIEXPORT jint JNICALL
Java_org_webrtc_videoP2P_VideoClient_Destroy(
        JNIEnv *env,
        jobject)
{
  Terminate();
/* TODO: khanh fix this
  if (remoteSurface_ != NULL) {
    env->DeleteGlobalRef(reinterpret_cast<jobject>(remoteSurface_));
  }
*/
  return 0;
}

void Terminate(){
  if(xmpp_thread_){
    KXmppThread* tmp = xmpp_thread_;
    xmpp_thread_ = NULL;
    tmp->Disconnect();
    tmp->Terminate();
    delete tmp;
  }
}

JNIEXPORT jint JNICALL Java_org_webrtc_videoP2P_VideoClient_SetCamera
  (JNIEnv * env, jobject, jint deviceId, jstring deviceUniqueName)
{
  std::string s = env->GetStringUTFChars(deviceUniqueName, NULL);
  if ( xmpp_thread_ ) {
    LOGI("SetCamera(%d, %s)", deviceId, s.c_str());
    xmpp_thread_->SetCamera(deviceId, s);
  }
  return 0;
}

JNIEXPORT jint JNICALL Java_org_webrtc_videoP2P_VideoClient_SetImageOrientation(JNIEnv *env, jobject context, jint degrees)
{
  if ( xmpp_thread_ ) {
    LOGI("SetImageOrientation(%d)",degrees);
    xmpp_thread_->SetImageOrientation(degrees);
  }
  return 0;
}

JNIEXPORT void JNICALL Java_org_webrtc_videoP2P_VideoClient_SetBufferCount(JNIEnv *env, jobject context, jint count)
{
  if ( xmpp_thread_ ) {
    xmpp_thread_->client_->count_ = count;
    xmpp_thread_->client_->y_plane_ = (jbyte**)malloc(count*sizeof(jbyte*));
    xmpp_thread_->client_->u_plane_ = (jbyte**)malloc(count*sizeof(jbyte*));
    xmpp_thread_->client_->v_plane_ = (jbyte**)malloc(count*sizeof(jbyte*));
  }
}

JNIEXPORT void JNICALL Java_org_webrtc_videoP2P_VideoClient_SetBuffer(JNIEnv *env, jobject context, jobjectArray yuvPlanes, jint index)
{
  if ( xmpp_thread_ ) {
    jobject y = env->GetObjectArrayElement(yuvPlanes, 0);
    jobject u = env->GetObjectArrayElement(yuvPlanes, 1);
    jobject v = env->GetObjectArrayElement(yuvPlanes, 2);

    xmpp_thread_->client_->y_plane_[index] = (jbyte*)env->GetDirectBufferAddress(y);
    xmpp_thread_->client_->u_plane_[index] = (jbyte*)env->GetDirectBufferAddress(u);
    xmpp_thread_->client_->v_plane_[index] = (jbyte*)env->GetDirectBufferAddress(v);
  }
}

JNIEXPORT void JNICALL Java_org_webrtc_videoP2P_VideoClient_SetBufferDone(JNIEnv *env, jobject context, jboolean done)
{
  if ( xmpp_thread_ ) {
    xmpp_thread_->client_->buffer_initialized_ = done;
  }
}


/**
Copyright (C) 2009 Jurij Smakov <jurij@wooyd.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
void callback_handler(const char *methodname, short code) {
  int status;
  JNIEnv *env;
  bool isAttached = false;
  status = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
  if(status < 0) {
    LOGI("callback_handler: failed to get JNI environment, assuming native thread");
    status = gJavaVM->AttachCurrentThread(&env, NULL);
    if(status < 0) {
      LOGE("callback_handler: failed to attach "
      "current thread");
      return;
    }
    isAttached = true;
  }

  /* Construct a Java string */
  //TODO: Bad hack.
  //This conversion sucks. I can't just use strings otherwise
  //I get a duplicate definition error on my constants, which seems
  //is an ndk bug.
  //As well due to the problem below I can't use a method with
  //short natively and string seems to be the only one that works.
  char buf[2];
  snprintf(buf,2,"%d",code);
  jstring js = env->NewStringUTF(buf);
  // End Bad hack.

  jclass interfaceClass = env->GetObjectClass(gInterfaceObject);
  if(!interfaceClass) {
    LOGE("callback_handler: failed to get class reference");
    if(isAttached) gJavaVM->DetachCurrentThread();
    return;
  }
  /* Find the callBack method ID */
  //TODO:: This doesn't work. It can't find the method
  //if it has a definition of short
  //jmethodID method = env->GetStaticMethodID(
  //    interfaceClass, methodname, "(S)V");
  jmethodID method = env->GetStaticMethodID(
        interfaceClass, methodname, "(Ljava/lang/String;)V");
  if(!method) {
    LOGE("callback_handler: failed to get method ID");
    if(isAttached) gJavaVM->DetachCurrentThread();
    return;
  }
  env->CallStaticVoidMethod(interfaceClass, method, js);
  if(isAttached) gJavaVM->DetachCurrentThread();
}

void callback_handler_ext(const char *methodname, short code, const char *data) {
  int status;
  JNIEnv *env;
  bool isAttached = false;

  status = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
  if(status < 0) {
    LOGI("callback_handler_ext: failed to get JNI environment, assuming native thread");
    status = gJavaVM->AttachCurrentThread(&env, NULL);
    if(status < 0) {
      LOGE("callback_handler_ext: failed to attach "
      "current thread");
      return;
    }
    isAttached = true;
  }

  /* Construct a Java string */
  //TODO: Bad hack.
  //This conversion sucks. I can't just use strings otherwise
  //I get a duplicate definition error on my constants, which seems
  //is an ndk bug.
  //As well due to the problem below I can't use a method with
  //short natively and string seems to be the only one that works.
  char buf[2];
  snprintf(buf,2,"%d",code);
  jstring js = env->NewStringUTF(buf);
  // End Bad hack.

  /* Construct a Java string for the data*/
  jstring jsData = env->NewStringUTF(data);

  jclass interfaceClass = env->GetObjectClass(gInterfaceObject);
  if(!interfaceClass) {
    LOGE("callback_handler_ext: failed to get class reference");
    if(isAttached) gJavaVM->DetachCurrentThread();
    return;
  }
  /* Find the callBack method ID */
  //TODO:: This doesn't work. It can't find the method
  //if it has a definition of short
  //jmethodID method = env->GetStaticMethodID(
  //    interfaceClass, methodname, "(S)V");
  jmethodID method = env->GetStaticMethodID(
        interfaceClass, methodname, "(Ljava/lang/String;Ljava/lang/String;)V");
  if(!method) {
    LOGE("callback_handler: failed to get method ID");
    if(isAttached) gJavaVM->DetachCurrentThread();
    return;
  }
  env->CallStaticVoidMethod(interfaceClass, method, js, jsData);
  if(isAttached) gJavaVM->DetachCurrentThread();
}

void initClassHelper(JNIEnv *env, const char *path, jobject *objptr) {
  jclass cls = env->FindClass(path);
  if(!cls) {
    LOGE("initClassHelper: failed to get %s class reference", path);
    return;
  }
  jmethodID constr = env->GetMethodID(cls, "<init>", "()V");
  if(!constr) {
    LOGE("initClassHelper: failed to get %s constructor", path);
    return;
  }
  jobject obj = env->NewObject(cls, constr);
  if(!obj) {
    LOGE("initClassHelper: failed to create a %s object", path);
    return;
  }
  (*objptr) = env->NewGlobalRef(obj);
}

/** End Copied Code **/
