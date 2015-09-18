#ifndef ANDROID_TEST_H_
#define ANDROID_TEST_H_

#include <jni.h>
#include <string>

namespace videoP2P{

const char CALL_STATE_CALLBACK[] = "callBackCallState";
const char XMPPENGINE_CALLBACK[] = "callBackXMPPEngineState";
const char XMPPERROR_CALLBACK[] = "callBackXMPPError";
const char PRESENCE_STATE_CALLBACK[] = "callBackPresenceState";

const short CALL_CALLING = 0;
const short CALL_ANSWERED = 1;
const short CALL_REJECTED = 2;
const short CALL_INPROGRESS = 3;
const short CALL_RECEIVED_TERMINATE = 4;
const short CALL_INCOMING = 5;
const short CALL_PRESENCE_DETECTED = 6;
const short CALL_PRESENCE_DROPPED = 7;
const short CALL_RECEIVED_REJECT = 8;

const short XMPPENGINE_CLOSED = 0;
const short XMPPENGINE_OPEN = 1;
const short XMPPENGINE_OPENING = 2;
const short XMPPENGINE_START = 3;

const short XMPPERROR_NONE = 0;
const short XMPPERROR_XML = 1;
const short XMPPERROR_STREAM = 2;
const short XMPPERROR_VERSION = 3;
const short XMPPERROR_UNAUTH = 4;
const short XMPPERROR_TLS = 5;
const short XMPPERROR_AUTH = 6;
const short XMPPERROR_BIND = 7;
const short XMPPERROR_CONN_CLOSED = 8;
const short XMPPERROR_DOC_CLOSED = 9;
const short XMPPERROR_SOCK_ERR = 10;
const short XMPPERROR_UNKNOWN = 11;

}

void initClassHelper(JNIEnv *env, const char *path, jobject *obj);
void Terminate();
void callback_handler(const char *method, short code);
void callback_handler_ext(const char *method, short code, const char *data);

#endif
