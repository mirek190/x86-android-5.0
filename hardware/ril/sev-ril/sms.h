
/*
 *  sms.h
 *  
 *
 *  Created by fupeilong on 1/22/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */
 
/*when who why modified*/

#ifndef SMS_H
#define SMS_H 1
#endif


/*added by zte-yuang  for adding sms_send_report begin */
typedef enum
{
    SMS_GENERAL = 0, 
    SMS_SEND_REPORT = 1,
    SMS_BROADCAST = 2
} SMS_Type;

/*added by zte-yuang  for adding sms_send_report end */
/*  request functions
 *
 */

void receiveSMS(void *param);

void requestWriteSmsToSim(void *data, size_t datalen, RIL_Token t);

void requestSendSMS(void *data, size_t datalen, RIL_Token t);

void requestSMSAcknowledge(void *data, size_t datalen, RIL_Token t);

void requestDeleteSmsOnSim(void *data, size_t datalen, RIL_Token t);

#if 1
/* added by zte-yuyang begin */

void requestGsmGetBroadcastSMSConfig(void *data, size_t datalen, RIL_Token t);

void requestGsmSetBroadcastSMSConfig(void *data, size_t datalen, RIL_Token t);

void requestGsmSMSBroadcastActivation(void *data, size_t datalen, RIL_Token t);

void requestGetSMSCAddress(void *data, size_t datalen, RIL_Token t);
 
void requestSetSMSCAddress(void *data, size_t datalen, RIL_Token t);

void requestReportSMSMemoryStatus(void *data, size_t datalen, RIL_Token t);

/* added by zte-yuyang end */
#endif
/* * 
 * functions called when AT command comes up  
 *
 */
void onNewSmsNotification(char *line);
