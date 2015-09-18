/*voice.h*/

/*when who why modified*/

void requestGetCurrentCalls(void *data, size_t datalen, RIL_Token t);

void requestDial(void *data, size_t datalen, RIL_Token t);

void requestGetMute(void *data, size_t datalen, RIL_Token t);

void requestDTMFStart(void *data, size_t datalen, RIL_Token t);

void requestDTMFStop(void *data, size_t datalen, RIL_Token t);

void requestDTMF(void *data, size_t datalen, RIL_Token t);

void requestSeparateConnection(void *data, size_t datalen, RIL_Token t);

void requestUDUB(void *data, size_t datalen, RIL_Token t);

void requestConference(void *data, size_t datalen, RIL_Token t);

void requestAnswer(void *data, size_t datalen, RIL_Token t);

void requestSwitchWaitingOrHoldingAndActive(void *data, size_t datalen, RIL_Token t);

void requestHangupForegroundResumeBackground(void *data, size_t datalen, RIL_Token t);

void requestHangupWaitingOrBackground(void *data, size_t datalen, RIL_Token t);

void requestHangup(void *data, size_t datalen, RIL_Token t);

/* added by zte-yuyang begin */
void  requestGetClir(void *data, size_t datalen, RIL_Token t);  

void  requestSetClir(void *data, size_t datalen, RIL_Token t);  

void requestQueryCallWaiting(void *data, size_t datalen, RIL_Token t);

void requestSetCallWaiting(void *data, size_t datalen, RIL_Token t);
/* added by zte-yuyang end */

