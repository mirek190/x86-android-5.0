/* 
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#define LOG_NDEBUG 0
#include <telephony/ril.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <alloca.h>
#include "atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include <getopt.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include <termios.h>
#include "ril_common.h"
#include "sim.h"
#include "voice.h"
#include "network.h"

#include "ril.h"

#define MAX_AT_RESPONSE 0x1000
#define SKIP_BOOT_TIME  40 //60
#define REFERENCE_RIL_VERSION  "IntelDongleRil1103-001" 

#define LIB_PATH_MODEM_PORT_PROPERTY   "ril.datachannel"
#define LIB_PATH_AT_PORT_PROPERTY   "ril.atport"
#define LIB_PATH_START_PORT_PROPERTY   "ril.startport"

static void onRequest (int request, void *data, size_t datalen, RIL_Token t);
static RIL_RadioState currentState();
static int onSupports (int requestCode);
static void onCancel (RIL_Token t);
static const char *getVersion();
static int isRadioOn();
SIM_Status getSIMStatus();
static int getCardStatus(RIL_CardStatus_v5 **pp_card_status);
static void freeCardStatus(RIL_CardStatus_v5 *p_card_status);
static void pollSIMState (void *param);
static void setRadioState(RIL_RadioState newState);



extern const char * requestToString(int request);

#define SERVICE_PPPD_GPRS "pppd_gprs"

enum RADIO_STATUS
{
	//UNKNOW = 0,
	BOOTUP = 1,
	PREDATACALL,
	DATACALLFAIL,
	DATACALLSUCCESS,
	DATACALLTERMINIT,	
};

static int radio_status = UNKNOWN;

int dongle_vendor = UNKNOWN;
int phone_has = 0;              /* Modes that the phone hardware supports */
int phone_is  = MODE_GSM;       /* Mode the phone is in ,default is gsm*/

/*** Static Variables ***/
static const RIL_RadioFunctions s_callbacks =
{
   RIL_VERSION,
   onRequest,
   currentState,
   onSupports,
   onCancel,
   getVersion
};

static RIL_RadioState sState = RADIO_STATE_UNAVAILABLE;

static pthread_mutex_t s_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_state_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t s_ppp_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_ppp_state_cond = PTHREAD_COND_INITIALIZER;
static int s_polling_status = 0;

/* trigger change to this with s_state_cond */
static int s_closed = 0;

static int sFD;     /* file desc of AT channel */
static char sATBuffer[MAX_AT_RESPONSE+1];
static char *sATBufferCur = NULL;

static const struct timeval TIMEVAL_SIMPOLL = {1,0};

static const struct timeval TIMEVAL_0 = {0,0};
static struct timeval TIMEVAL_DELAYINIT = {0,0}; // will be set according to property value
static struct timeval TIMEVAL_DELAYPPP = {2,0};

#define PPP_OPERSTATE_PATH     "/sys/class/net/ppp0/operstate"
#define POLL_PPP_SYSFS_RETRY 5
#define POLL_PPP_SYSFS_SECONDS	3

static unsigned int getUsbVidPid(const char *path, char *buffer, int len, char *usb_vid);

void requestScreenState(void *data, size_t datalen, RIL_Token t)
{
       
       int status =((int*)data)[0];//1 for "Screen On",0 for "Screen Off"
	   //pthread_t       ppid;
	   	   
	   ALOGD("status = %d", status);
       if(status == 0) {
	   
          ALOGD("******** ---Screen State OFF--- ********");
		  if((dongle_vendor == HUAWEI)) {
		    if((phone_is == MODE_GSM)) {
		 	  //at_send_command("AT+CGACT?", NULL);
			  at_send_command("AT^CURC=0", NULL);
			  at_send_command("AT+CREG=0", NULL);
			  at_send_command("AT+CGREG=0", NULL);
			} else {
			  at_send_command("AT^RSSIREP=0", NULL);
			}
		 }
		  property_set("ctl.stop", SERVICE_PPPD_GPRS);
		  
       } else if(status == 1) {
	   
         ALOGD("********---Screen State On---- ********");
		 
		 if((dongle_vendor == HUAWEI)) {
		    if((phone_is == MODE_GSM)) {
		 	  at_send_command("AT+CGACT?", NULL);
			  at_send_command("AT^CURC=1", NULL);
			  at_send_command("AT+CREG=2", NULL);
			  at_send_command("AT+CGREG=2", NULL);
			} else {
			   at_send_command("AT^RSSIREP=1", NULL);
			}
			
		 }
		 else if(dongle_vendor == ZTE){
           ALOGD("********Screen State On ********");
           at_send_command("AT+CGACT?",NULL);
           at_send_command("AT+ZPAS?",NULL);
           at_send_command("AT+CSQ",NULL);		
           goto OUT;    
		}
//	     if((RADIO_STATE_SIM_READY == currentState())  || (RADIO_STATE_RUIM_READY == currentState())) {
//		 
//		     
//		     int ret = -1;
//		     ret = property_set("ctl.start", SERVICE_PPPD_GPRS);	
//			 ALOGD("enter the screen on state..ret = %d", ret);	
//
//         }
       }

OUT:	
       	 RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
	 return;
}

/** do post-AT+CFUN=1 initialization */
static void onRadioPowerOn()
{
   ALOGD("******** Enter onRadioPowerOn() ********");
   pollSIMState(NULL);
}

/** do post- SIM ready initialization */
static void onSIMReady()
{
    ATResponse *p_response = NULL;
    
    ALOGD("******** Enter onSIMReady() ********");
    
    at_send_command_singleline("AT+CSMS=0", "+CSMS:", NULL);
   
    at_send_command("AT+CMGF=0",NULL);   //support PDU mode only now
    
    at_send_command_singleline("AT+CNMI?", "+CNMI:", &p_response);
    
    at_send_command("AT+CNMI=1,1,2,2,0", NULL);
//     at_send_command("AT+CNMI=1,2,1,2,0", NULL);
//     at_send_command("AT+CNMI=2,2,2,1", NULL);
   
    at_send_command("AT+CPMS=\"SM\",\"SM\",\"SM\"", NULL);

}

static void requestRadioPower(void *data, size_t datalen, RIL_Token t)
{
   int onOff;

   int err;
   ATResponse *p_response = NULL;
   
   ALOGD("******** Enter requestRadioPower() ********");

   assert (datalen >= sizeof(int *));
   onOff = ((int *)data)[0];
		
   if (onOff == 0 && sState != RADIO_STATE_OFF)
   {
       err = at_send_command("AT+CFUN=0", &p_response);
#if 0       
       if (err < 0 || p_response->success == 0) goto error;
       setRadioState(RADIO_STATE_OFF);
#else
        setRadioState(RADIO_STATE_OFF);
//        if (err < 0 || p_response->success == 0)      goto error; //don't care this
#endif       
   }
   else if (onOff > 0 && sState == RADIO_STATE_OFF)
   {
       err = at_send_command("AT+CFUN=1", &p_response);
       if (err < 0|| p_response->success == 0)
       {
           if (isRadioOn() != 1)
           {
               goto error;
           }
       }
	   
	   if(phone_is == MODE_CDMA)
	     setRadioState(RADIO_STATE_RUIM_NOT_READY);
	   else 
         setRadioState(RADIO_STATE_SIM_NOT_READY);
   }

    if(onOff > 0){
        err = at_send_command_singleline("AT^CURC?","^CURC", p_response);
        at_send_command("AT^CURC=1", NULL);     //the unsolicited reports open
    }
   at_response_free(p_response);
   RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
   return;
error:
   at_response_free(p_response);
   RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}



void requestGetSimStatus(void* data, size_t datalen, RIL_Token t)
{
   RIL_CardStatus_v5 *p_card_status;
   char *p_buffer;
   int buffer_size;

   ALOGD("******** Enter requestGetSimStatus() ********");

   int result = getCardStatus(&p_card_status);

   if (result == RIL_E_SUCCESS)
   {
       p_buffer = (char *)p_card_status;
       buffer_size = sizeof(*p_card_status);
   }
   else
   {
       p_buffer = NULL;
       buffer_size = 0;
   }
   RIL_onRequestComplete(t, result, p_buffer, buffer_size);
   freeCardStatus(p_card_status);
}

/**
* Synchronous call from the RIL to us to return current radio state.
* RADIO_STATE_UNAVAILABLE should be the initial state.
*/
static RIL_RadioState
currentState()
{    
   ALOGD("******** Enter currentState() %d ********", sState);
   return sState;
}
/**
* Call from RIL to us to find out whether a specific request code
* is supported by this implementation.
*
* Return 1 for "supported" and 0 for "unsupported"
*/

static int
onSupports (int requestCode)
{
   //@@@ todo
   ALOGD("******** Enter onSupports() ********");
   return 1;
}

static void onCancel (RIL_Token t)
{
   //@@@todo
   ALOGD("******** Enter onCancel() ********");
}
static const char * getVersion(void)
{
   ALOGD("******** Enter getVersion() ********");
   return REFERENCE_RIL_VERSION;
}

static int getSysSimInfo(void)
{
    ATResponse *p_response = NULL;
	int srv_status = 0;
	int srv_domain = 0;
	int roam_status = 0;
	int sys_mode = 0;
	int sim_status = 0;
	char *line = NULL;
	int sim_type = 0;
	int err = -1;
	
	ALOGD("***getSysSimInfo()***");
	
    err = at_send_command_singleline("AT^SYSINFO", "^SYSINFO:", &p_response);
    if (err < 0 || p_response->success == 0) goto error;
    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;
	err = at_tok_nextint(&line, &srv_status);
    if (err < 0) goto error;
	err = at_tok_nextint(&line, &srv_domain);
    if (err < 0) goto error;
	err = at_tok_nextint(&line, &roam_status);
    if (err < 0) goto error;
	err = at_tok_nextint(&line, &sys_mode);
    if (err < 0) goto error;
	err = at_tok_nextint(&line, &sim_status);
    if (err < 0) goto error;
	
	ALOGD("srv_status %d, srv_domain %d, roam_status %d, sys_mode %d, sim_status %d", srv_status, srv_domain, roam_status, sys_mode, sim_status);
	
	
	if((sim_status == 1)||(sim_status == 240)) {
		sim_type = RADIO_STATE_RUIM_NOT_READY;
	    phone_is = MODE_CDMA;
	} else {  //jw 1103
	    if(phone_is == -1)
		   phone_is = MODE_CDMA;
	    sim_type = RADIO_STATE_RUIM_NOT_READY;  // if something wrong happen
	}
	
	return sim_type;
error:
    at_response_free(p_response);
    return -1;
}

static int getSysInfo(void)
{
    
    ATResponse *p_response = NULL;
	int id = 0;
	int protocol = 0;
	int offline = 0;
	int p_class = 0;
	int p_id = 0;
	char *line = NULL;
	
	int sim_type = -1;
	int err = -1;
	
	if(dongle_vendor == HUAWEI || dongle_vendor == STRONGRISING) {
	    err = at_send_command_singleline("AT^HS=0,0", "^HS:",  &p_response);
		if (err < 0 || p_response->success == 0) goto error;
    	line = p_response->p_intermediates->line;
    	err = at_tok_start(&line);
    	if (err < 0) goto error;
		err = at_tok_nexthexint(&line, &id);
		if (err < 0) goto error;
		err = at_tok_nextint(&line, &protocol);
    	if (err < 0) goto error;
		err = at_tok_nextint(&line, &offline);
    	if (err < 0) goto error;
		err = at_tok_nextint(&line, &p_class);
    	if (err < 0) goto error;
		err = at_tok_nextint(&line, &p_id);
    	if (err < 0) goto error;
		
		switch(p_class) {
		   case 0:		
		      sim_type = RADIO_STATE_SIM_NOT_READY;
	          phone_is = MODE_GSM;   
	          ALOGD("*********module type is WCDMA**********,sim_type=%d",sim_type);
		      break;
	      case 2:
		  	  sim_type = getSysSimInfo();
		  	  phone_is = MODE_CDMA;
	          ALOGD("*********module type is CDMA**********,sim_type=%d",sim_type);
			  break;
		}
	}

   
	return sim_type;
error:
    at_response_free(p_response);
    return -1;
}

static void
setRadioState(RIL_RadioState newState)
{
   RIL_RadioState oldState;
	
   ALOGD("******** Enter setRadioState() ********");
   ALOGD("oldState %d, sState %d, newState %d", oldState, sState, newState);
   pthread_mutex_lock(&s_state_mutex);
   
   oldState = sState;

   if (s_closed > 0) {
       newState = RADIO_STATE_UNAVAILABLE;
   }

   if (sState != newState || s_closed > 0) {
       sState = newState;

       pthread_cond_broadcast (&s_state_cond);
   }
   
   pthread_mutex_unlock(&s_state_mutex);

   ALOGD("setRadioState sState = %d, oldState = %d", sState, oldState);
   /* do these outside of the mutex */
   if ((sState != oldState)) {
       RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED,
                                  NULL, 0);
								  
	   
	       if ((sState == RADIO_STATE_RUIM_READY) || (sState == RADIO_STATE_SIM_READY)) {
	           onSIMReady();
	       } else if((sState == RADIO_STATE_RUIM_NOT_READY) || (sState == RADIO_STATE_SIM_NOT_READY)) {
   		       onRadioPowerOn();
           }

   } 
   
   
}

/** Returns SIM_NOT_READY on error */
SIM_Status
getSIMStatus()
{
   ATResponse *p_response = NULL;
   int err;
   int ret;
   char *cpinLine;
   char *cpinResult;
	
   ALOGD("******** Enter getSIMStatus() ********");
	
   if (sState == RADIO_STATE_OFF || sState == RADIO_STATE_UNAVAILABLE) {
       ret = SIM_NOT_READY;
       goto done;
   }

   err = at_send_command_singleline("AT+CPIN?", "+CPIN:", &p_response);

    if (err != 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

   switch (at_get_cme_error(p_response))
   {
   case CME_SUCCESS:
       break;

   case CME_SIM_NOT_INSERTED:
       ret = SIM_ABSENT;
       goto done;

   case QCDATACARD_SIM_NOT_INSERTED:
   	   ret = SIM_ABSENT;
	   goto done;
	   
   default:
       ret = SIM_NOT_READY;
       goto done;
   }

   /* CPIN? has succeeded, now look at the result */

   cpinLine = p_response->p_intermediates->line;
   err = at_tok_start (&cpinLine);

   if (err < 0) {
       ret = SIM_NOT_READY;
       goto done;
   }

   err = at_tok_nextstr(&cpinLine, &cpinResult);

   if (err < 0) {
       ret = SIM_NOT_READY;
       goto done;
   }

   if (0 == strcmp (cpinResult, "SIM PIN")) {
       ret = SIM_PIN;
       goto done;
   } else if (0 == strcmp (cpinResult, "SIM PUK")) {
       ret = SIM_PUK;
       goto done;
   } else if (0 == strcmp (cpinResult, "PH-NET PIN")) {
       return SIM_NETWORK_PERSONALIZATION;
   } else if (0 != strcmp (cpinResult, "READY")) {
       /* we're treating unsupported lock types as "sim absent" */
       ret = SIM_ABSENT;
       goto done;
   }

   at_response_free(p_response);
   p_response = NULL;
   cpinResult = NULL;

   ret = SIM_READY;

done:
   at_response_free(p_response);
   return ret;
}


/**
* Get the current card status.
*
* This must be freed using freeCardStatus.
* @return: On success returns RIL_E_SUCCESS
*/
static int getCardStatus(RIL_CardStatus_v5 **pp_card_status)
{
   static RIL_AppStatus app_status_array[] =
   {
       // SIM_ABSENT = 0
       {
           RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_UNKNOWN, RIL_PERSOSUBSTATE_UNKNOWN,
           NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN
       },
       // SIM_NOT_READY = 1
       {
           RIL_APPTYPE_SIM, RIL_APPSTATE_DETECTED, RIL_PERSOSUBSTATE_UNKNOWN,
           NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN
       },
       // SIM_READY = 2
       {
           RIL_APPTYPE_SIM, RIL_APPSTATE_READY, RIL_PERSOSUBSTATE_READY,
           NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN
       },
       // SIM_PIN = 3
       {
           RIL_APPTYPE_SIM, RIL_APPSTATE_PIN, RIL_PERSOSUBSTATE_UNKNOWN,
           NULL, NULL, 0, RIL_PINSTATE_ENABLED_NOT_VERIFIED, RIL_PINSTATE_UNKNOWN
       },
       // SIM_PUK = 4
       {
           RIL_APPTYPE_SIM, RIL_APPSTATE_PUK, RIL_PERSOSUBSTATE_UNKNOWN,
           NULL, NULL, 0, RIL_PINSTATE_ENABLED_BLOCKED, RIL_PINSTATE_UNKNOWN
       },
       // SIM_NETWORK_PERSONALIZATION = 5
       {
           RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO, RIL_PERSOSUBSTATE_SIM_NETWORK,
           NULL, NULL, 0, RIL_PINSTATE_ENABLED_NOT_VERIFIED, RIL_PINSTATE_UNKNOWN
       }
   };
   RIL_CardState card_state;
   int num_apps;

   int sim_status = getSIMStatus();
	
   ALOGD("******** Enter getCardStatus() sim_status = %d ********", sim_status);
	
   if (sim_status == SIM_ABSENT) {
       card_state = RIL_CARDSTATE_ABSENT;
       num_apps = 0;
   } else {
       card_state = RIL_CARDSTATE_PRESENT;
       num_apps = 1;
   }

   // Allocate and initialize base card status.
   RIL_CardStatus_v5 *p_card_status = malloc(sizeof(RIL_CardStatus_v5));
   p_card_status->card_state = card_state;
   p_card_status->universal_pin_state = RIL_PINSTATE_UNKNOWN;
   p_card_status->gsm_umts_subscription_app_index = RIL_CARD_MAX_APPS;
   p_card_status->cdma_subscription_app_index = RIL_CARD_MAX_APPS;
   p_card_status->num_applications = num_apps;

   // Initialize application status
   int i;
   for (i = 0; i < RIL_CARD_MAX_APPS; i++)
   {
       p_card_status->applications[i] = app_status_array[SIM_ABSENT];
   }
   
   if(phone_is == MODE_CDMA) {
        if(app_status_array[sim_status].app_type == RIL_APPTYPE_SIM)
            app_status_array[sim_status].app_type = RIL_APPTYPE_RUIM;
   }
   

   // Pickup the appropriate application status
   // that reflects sim_status for gsm.
   if (num_apps != 0)
   {
   
       if(phone_is == MODE_CDMA) {
	     p_card_status->num_applications = 1;
         p_card_status->cdma_subscription_app_index = 0;
	   } else {
       // Only support one app, gsm
         p_card_status->num_applications = 1;
         p_card_status->gsm_umts_subscription_app_index = 0;
       }
       // Get the correct app status
       p_card_status->applications[0] = app_status_array[sim_status];
   }

   *pp_card_status = p_card_status;
   return RIL_E_SUCCESS;
}

/**
* Free the card status returned by getCardStatus
*/
static void freeCardStatus(RIL_CardStatus_v5 *p_card_status)
{
   ALOGD("******** Enter freeCardStatus() ********");
   free(p_card_status);
}

/**
* SIM ready means any commands that access the SIM will work, including:
*  AT+CPIN, AT+CSMS, AT+CNMI, AT+CRSM
*  (all SMS-related commands)
*/

static void pollSIMState (void *param)
{
   ATResponse *p_response;
   int ret;
	
   ALOGD("******** Enter pollSIMState() ********");
   if(phone_is == MODE_CDMA){
     if (sState != RADIO_STATE_RUIM_NOT_READY) {
       return;
     }
   } else {  //GSM WCDMA
       if (sState != RADIO_STATE_SIM_NOT_READY) {
           return;
       }
   }
   
   switch(getSIMStatus())
   {
   case SIM_ABSENT:
   case SIM_PIN:
   case SIM_PUK:
   case SIM_NETWORK_PERSONALIZATION:
   default:
       if(phone_is == MODE_CDMA)
	       setRadioState(RADIO_STATE_RUIM_LOCKED_OR_ABSENT);
	   else
           setRadioState(RADIO_STATE_SIM_LOCKED_OR_ABSENT);
       return;

   case SIM_NOT_READY:
       RIL_requestTimedCallback (pollSIMState, NULL, &TIMEVAL_SIMPOLL);
       return;

   case SIM_READY:
       if(phone_is == MODE_CDMA)
           setRadioState(RADIO_STATE_RUIM_READY);
       else
           setRadioState(RADIO_STATE_SIM_READY);
       return;
   }
}

/** returns 1 if on, 0 if off, and -1 on error */
static int isRadioOn()
{
   ATResponse *p_response = NULL;
   int err;
   char *line;
   char ret;
	
   ALOGD("******** Enter isRadioOn() ********");
	
   err = at_send_command_singleline("AT+CFUN?", "+CFUN:", &p_response);

   if (err < 0 || p_response->success == 0)
   {
       // assume radio is off
       goto error;
   }

   line = p_response->p_intermediates->line;

   err = at_tok_start(&line);
   if (err < 0) goto error;

   err = at_tok_nextbool(&line, &ret);
   if (err < 0) goto error;

   at_response_free(p_response);

   return (int)ret;

error:

   at_response_free(p_response);
   return -1;
}


static void waitForClose()
{
   ALOGD("******** Enter waitForClose() ********");

   pthread_mutex_lock(&s_state_mutex);

   while (s_closed == 0)
   {
       pthread_cond_wait(&s_state_cond, &s_state_mutex);
   }

   pthread_mutex_unlock(&s_state_mutex);
}


/* Called on command or reader thread */
static void onATReaderClosed()
{
   ALOGD("******** Enter onATReaderClosed() ********");
   at_close();
   s_closed = 1;

   setRadioState (RADIO_STATE_UNAVAILABLE);
   ALOGD("AT channel closed\n");
}

/* Called on command thread */
static void onATTimeout()
{
   ALOGD("******** Enter onATTimeout() ********");
   at_close();

   s_closed = 1;

   /* FIXME cause a radio reset here */

   setRadioState (RADIO_STATE_UNAVAILABLE);
   ALOGD("AT channel timeout; closing\n");
}

/**
* Initialize everything that can be configured while we're still in
* AT+CFUN=0
*/
void initializeCallback(void *param)
{
   ATResponse *p_response = NULL;
   int err;
   int timeout = 0;
   int sim_type = 0;

   ALOGD("******** Enter initializeCallback() ********");
	
   setRadioState (RADIO_STATE_OFF);
   RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED,
                                  NULL, 0);

   at_handshake();

   /*  have verbose result codes */
   at_send_command("ATE0", NULL);
   usleep(100000);
   at_send_command("ATQ0V1", NULL);
   usleep(100000);
    
   /*  No auto-answer */
   err = at_send_command("ATS0=0", &p_response);

   while (err < 0 || p_response->success == 0) {
       at_response_free(p_response);
       p_response = NULL;
       err = at_send_command("ATS0=0", &p_response);       
       sleep(1);
       timeout++;
       if (timeout > 10)
          break;
   }

   at_response_free(p_response);
   p_response = NULL;

   /*  Extended errors */
   at_send_command("AT+CMEE=1", NULL);
   usleep(100000);
   if(dongle_vendor == STRONGRISING) {
      at_send_command("AT+CFUN=1", NULL);  //jw 1103
	  if(phone_is == MODE_CDMA)
	     sim_type = RADIO_STATE_RUIM_NOT_READY;  
        goto OUT; 
   } else if(dongle_vendor == ZTE){      //fixme ,need support ZTE CDMA Dongle.
//        phone_is = MODE_GSM;
        if(phone_is == MODE_GSM)
            sim_type = RADIO_STATE_SIM_NOT_READY;
        else 
            sim_type = RADIO_STATE_RUIM_NOT_READY;  
        goto OUT;      
            
    }   

   sim_type = getSysInfo();
   ALOGD("----***********get_phone_sim_relation********---,sim_type=%d",sim_type);
   if(sim_type == -1) {
    
        // send cmd cops to identify cdma2000 or wcdma
        // cdma without response
        // wcdma with response     	  
        err = at_send_command_singleline("AT+COPS?", "+COPS:", &p_response);	
        if (err < 0 || p_response->success == 0) {
        	//cdma mode
            phone_is = MODE_CDMA;// default is CDMA mode 
        } else {
        	// gsm mode
            phone_is = MODE_GSM;// default is gsm mode 
            sim_type = RADIO_STATE_SIM_NOT_READY;
        }	
        at_response_free(p_response);	    	    
   }
   
   if(dongle_vendor == HUAWEI) {
       if(0/*phone_is == MODE_GSM*/) {
	     #if 1
         /*  Network registration events */
         err = at_send_command( "AT+CREG=2", &p_response);

         /* some handsets -- in tethered mode -- don't support CREG=2 */
         if (err < 0 || p_response->success == 0) {
            at_send_command("AT+CREG=1", NULL);
         }

         at_response_free(p_response);

         /*  GPRS registration events */
         at_send_command( "AT+CGREG=2", NULL);
         #endif
	   }
        if(sim_type == RADIO_STATE_SIM_NOT_READY)
            at_send_command("AT^CURC=0", NULL);
    	   
        at_send_command("AT^BOOT=0,0", &p_response);
    	  
        if(sim_type == RADIO_STATE_RUIM_NOT_READY)  
            at_send_command("AT^RSSIREP=1", NULL);

   }

OUT:   
   at_send_command("AT+CFUN?", NULL);
   /* assume radio is off on error */
   if (isRadioOn() > 0) {   
        setRadioState (sim_type);
       /*if((phone_is == MODE_CDMA))
         setRadioState (RADIO_STATE_RUIM_NOT_READY);
	   else
         setRadioState (RADIO_STATE_SIM_NOT_READY);*/
   }
}

/**
* Call from RIL to us to make a RIL_REQUEST
*
* Must be completed with a call to RIL_onRequestComplete()
*
* RIL_onRequestComplete() may be called from any thread, before or after
* this function returns.
*
*/

static void
onRequest (int request, void *data, size_t datalen, RIL_Token t)
{
   ATResponse *p_response;
   int err;
   static int sn = 0;

   ALOGD("********%3d, onRequest(%d): %s********", sn, request,requestToString(request));
   sn++;
   if (sState == RADIO_STATE_UNAVAILABLE
           && request != RIL_REQUEST_GET_SIM_STATUS
      )
   {
       RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
       return;
   }

   if (sState == RADIO_STATE_OFF
       && !(request == RIL_REQUEST_RADIO_POWER || request == RIL_REQUEST_GET_SIM_STATUS)
      )
   {
       RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
       return;
   }
	
   switch (request)
   {
       /**
       * the first part : initialize
       */

   case RIL_REQUEST_GET_SIM_STATUS:
   {
       requestGetSimStatus(data,datalen,t);
       break;
   }
   case RIL_REQUEST_CDMA_SEND_SMS: 
    {
//       requestSendSMS_CDMA(data, datalen, t);
        request_send_cdma_sms(data, datalen, t);
       break;            
    }
   case RIL_REQUEST_SEND_SMS:
   {
       requestSendSMS(data, datalen, t);
       break;
   }
   
   case RIL_REQUEST_RADIO_POWER:
   {
       requestRadioPower(data, datalen, t);
       break;
   }

   case RIL_REQUEST_SIM_IO:
   {
       requestSIM_IO(data,datalen,t);
       break;
   }
   case RIL_REQUEST_QUERY_FACILITY_LOCK:
   {
       requestQueryFacilityLock(data, datalen, t);
//        RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
       break;
   }

   case RIL_REQUEST_SET_FACILITY_LOCK:
   {
       requestSetFacilityLock(data, datalen, t);
       break;
   }
#if 1   
   case RIL_REQUEST_ENTER_SIM_PIN:
   {
       requestEnterSimPin(data, datalen, t);
       break;
   }

   case RIL_REQUEST_ENTER_SIM_PUK:
   {
       requestEnterSimPuk(data, datalen, t);
       break;
   }

   case RIL_REQUEST_ENTER_SIM_PIN2:
   {
       RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
       break;
   }

   case RIL_REQUEST_ENTER_SIM_PUK2:
   {
       RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
       break;
   }

   case RIL_REQUEST_CHANGE_SIM_PIN:
   {
//       requestChangeSimPin(data, datalen, t);
        requestChangePassword(data, datalen, t, "SC", request);
       break;
   }

   case RIL_REQUEST_CHANGE_SIM_PIN2:
   {
       RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
       break;
   }
#else
    case RIL_REQUEST_ENTER_SIM_PIN:
    case RIL_REQUEST_ENTER_SIM_PUK:
    case RIL_REQUEST_ENTER_SIM_PIN2:
    case RIL_REQUEST_ENTER_SIM_PUK2:
    case RIL_REQUEST_CHANGE_SIM_PIN:
    case RIL_REQUEST_CHANGE_SIM_PIN2:
        requestEnterSimPin(data, datalen, t);
        break;
#endif
   case RIL_REQUEST_SIGNAL_STRENGTH:
   {
       requestSignalStrength(data, datalen, t);
       break;
   }

   case RIL_REQUEST_VOICE_REGISTRATION_STATE:
   case RIL_REQUEST_DATA_REGISTRATION_STATE:
   {
   
       if(phone_is == MODE_CDMA)
	      requestCdmaRegistrationState(request, data, datalen, t);
	   else
          requestRegistrationState(request, data, datalen, t);
       break;
   }

   case RIL_REQUEST_OPERATOR:
   {
       if(phone_is == MODE_CDMA)
	   	  requestCDMAOperator(data, datalen, t);
	   else
          requestOperator(data, datalen, t);
       break;
   }

   case RIL_REQUEST_SETUP_DATA_CALL:
   {
      if(phone_is == MODE_CDMA)
        requestCdmaSetupDataCall(data, datalen, t);
	  else
       requestGsmSetupDataCall(data, datalen, t);
       break;
   }

   case RIL_REQUEST_DEACTIVATE_DATA_CALL:
   {
        requestDeactivateDataCall(data, datalen, t);
        break;
   }
   case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC:
   {
       requestSetNetworkSelectionAutomatic(data,datalen,t);
       break;
   }

   case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL:
   {
       requestSetNetworkSelectionManual(data, datalen, t);
       break;      
   }

   case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE:
   {
       int networktype;
	   if(phone_is == MODE_CDMA)
	   	   networktype = PREF_NET_TYPE_CDMA_EVDO_AUTO;
       requestSetPreferredNetworkType(data, networktype, t);
       break;
   }

   case RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE:
   {
       requestGetPreferredNetworkType(data, datalen, t);
       break;
   }

   case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS:
   {
       requestQueryAvailableNetworks(data, datalen, t);
       break;
   }
   case RIL_REQUEST_GET_NEIGHBORING_CELL_IDS:
   {
       requestNeighboringCellIds(data,datalen,t);
       break;
   }
   case RIL_REQUEST_DATA_CALL_LIST:
   {
       if(phone_is == MODE_CDMA)
	     requestCdmaDataCallList(data, datalen, t);
	   else
         requestDataCallList(data, datalen, t);
       break;
   }

   case RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE:
   {    
        if(phone_is == MODE_CDMA)
           requestQueryCdmaNetworkSelectionMode(data, datalen, t);
		else
           requestQueryNetworkSelectionMode(data, datalen, t);
       break;
   }

   case RIL_REQUEST_SCREEN_STATE:
   {
//       requestScreenState(data, datalen, t);
        RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
       break;
   }
   
   case RIL_REQUEST_DEVICE_IDENTITY:
   {
    int err;

    char * responseStr[4];
    ATResponse *p_response = NULL;

    int count = 4;

    // Fixed values. TODO: Query modem
    responseStr[0] = "----";
    responseStr[1] = "----";
    responseStr[2] = "77777777";

    err = at_send_command_numeric("AT+GSN", &p_response);
    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    } else {
        responseStr[3] = p_response->p_intermediates->line;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, count*sizeof(char*));
    at_response_free(p_response);

    return;
error:
    ALOGE("requestCdmaDeviceIdentity must never return an error when radio is on");
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
   }
   
   case RIL_REQUEST_GET_IMSI:
   {
       ATResponse *p_response = NULL;
       int err = 0;
       char temp[30]={0};
#if 0
       err = at_send_command_numeric("AT+CIMI", &p_response);

       if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
       } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS,
                              p_response->p_intermediates->line, sizeof(char *));
       }
       at_response_free(p_response);
#else 
    err = requestIMSI(temp);
    if(err == 0){
        RIL_onRequestComplete(t, RIL_E_SUCCESS, temp, sizeof(char *));		        
    }else{
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
#endif       
       break;
   }
   case RIL_REQUEST_CDMA_SUBSCRIPTION:
   {
        ATResponse *p_response = NULL;
        int err = 0;
        char * responseStr[5];
        char temp[30]={0},imsi_buffer[30]={0};
	   
        memset(temp, 0x0, sizeof(temp));
	   
        asprintf(&responseStr[0], "%c", '\0');
        asprintf(&responseStr[1], "%d", 0);
        asprintf(&responseStr[2], "%d", 0);

        err = requestIMSI(temp);
        if(err == 0){
            ALOGE("RIL_REQUEST_CDMA_SUBSCRIPTION return tmp is %s",temp);
            strncpy(imsi_buffer, temp+5, 10);
            responseStr[3] = imsi_buffer;
            asprintf(&responseStr[4], "%d", 0);
            RIL_onRequestComplete(t, RIL_E_SUCCESS, &responseStr, sizeof(responseStr));
        }
        else{
            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        }

        break;                        
   }
   case RIL_REQUEST_GET_IMEISV:
   {
       
       ATResponse *p_response = NULL;
       int err = 0;
       char imeisv_str[3];
       int len;
        err = at_send_command_numeric("AT+CGSN", &p_response);

       if (err < 0 || p_response->success == 0)
       {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
       }
       else
       {
        len = strlen(p_response->p_intermediates->line);
        imeisv_str[0] = p_response->p_intermediates->line[len-2];
        imeisv_str[1] = p_response->p_intermediates->line[len-1];
        imeisv_str[2] = 0;
        RIL_onRequestComplete(t, RIL_E_SUCCESS,imeisv_str, sizeof(char *));
       }
       at_response_free(p_response);
       break;
   }

   case RIL_REQUEST_GET_IMEI:
   {
   
       ATResponse *p_response = NULL;
       int err = 0;
       err = at_send_command_numeric("AT+CGSN", &p_response);

       if (err < 0 || p_response->success == 0)
       {
          RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
       } else {
          RIL_onRequestComplete(t, RIL_E_SUCCESS,
                              p_response->p_intermediates->line, sizeof(char *));
       }
       at_response_free(p_response);
       break;
   }

   case RIL_REQUEST_BASEBAND_VERSION:
   {
   
       ATResponse *p_response = NULL;
       int err = 0;
    
       err = at_send_command_singleline("AT+CGMR","+CGMR:",&p_response);

       if(err<0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
       } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, p_response->p_intermediates->line, sizeof(char *));
       }
       at_response_free(p_response);

       break;
   }

   case RIL_REQUEST_OEM_HOOK_RAW:
   {
       // echo back data
       RIL_onRequestComplete(t, RIL_E_SUCCESS, data, datalen);
       break;
   }

   case RIL_REQUEST_OEM_HOOK_STRINGS:
   {
       //requestOemHookStrings(data,datalen,t);
       break;
   }

   /**
   * the sixth part : voice
   */
   case RIL_REQUEST_GET_CURRENT_CALLS:
   {
//       requestGetCurrentCalls(data, datalen, t);
        RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
       break;
   }
   
   case RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE:
   {
   		break;
   }
   
   
   default:
       RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
       break;
   }
}

/**
* Called by atchannel when an unsolicited line appears
* This is called on atchannel's reader thread. AT commands may
* not be issued here
*/
static void onUnsolicited (const char *s, const char *sms_pdu)
{
   char *line = NULL;
   int err;
   RLOGI("******** Enter onUnsolicited():%s ********",s);
   RLOGI("******** Enter onUnsolicited(),sState= %d ********",sState);
   /* Ignore unsolicited responses until we're initialized.
    * This is OK because the RIL library will poll for initial state
    */
   if (sState == RADIO_STATE_UNAVAILABLE)
   {
       ALOGD("it is exit onunsolicited because RADIO_STATE_UNAVAILABLE");
       return;
   }

    if (strStartsWith(s, "%CTZV:")) {
        /* TI specific -- NITZ time */
        char *response;

       line = strdup(s);
       at_tok_start(&line);

       err = at_tok_nextstr(&line, &response);

       if (err != 0)
       {
           ALOGD("invalid NITZ line %s\n", s);
       }
       else
       {
           RIL_onUnsolicitedResponse (
               RIL_UNSOL_NITZ_TIME_RECEIVED,
               response, strlen(response));
       }
   }
   else if (strStartsWith(s,"+CRING:")
            || strStartsWith(s,"RING")
            || strStartsWith(s,"NO CARRIER")
            || strStartsWith(s,"+CCWA")
            || strStartsWith(s,"HANGUP:")
           )
   {
       RIL_onUnsolicitedResponse (
           RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED,
           NULL, 0);
#ifdef WORKAROUND_FAKE_CGEV
       RIL_requestTimedCallback (onDataCallListChanged, NULL, NULL); //TODO use new function
#endif /* WORKAROUND_FAKE_CGEV */
   }
   else if (strStartsWith(s,"+CREG:")
            || strStartsWith(s,"+CGREG:")
           )
   {
       RIL_onUnsolicitedResponse (
           RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
           NULL, 0);
#ifdef WORKAROUND_FAKE_CGEV
       RIL_requestTimedCallback (onDataCallListChanged, NULL, NULL);
#endif /* WORKAROUND_FAKE_CGEV */
   }
   else if (strStartsWith(s, "+CMT:"))
   {
       RIL_onUnsolicitedResponse (
           RIL_UNSOL_RESPONSE_NEW_SMS,
           sms_pdu, strlen(sms_pdu));
   }
//add by gaojing to test at+ecm



   else if (strStartsWith(s, "+ZPDPACT:"))
   {

         RIL_requestTimedCallback (onDataCallListChanged, NULL, NULL);

   }

   //add by gaojing to test at+ecm
   else if (strStartsWith(s, "+CDS:"))
   {
       RIL_onUnsolicitedResponse (
           RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT,
           sms_pdu, strlen(sms_pdu));
   }
   else if(strStartsWith(s,"+CMGR:"))
   {

            RIL_onUnsolicitedResponse (
               RIL_UNSOL_RESPONSE_NEW_SMS,
               sms_pdu, strlen(sms_pdu));
        

   } else if(strStartsWith(s,"^SIMST:"))
   {   
       int sim_status;
       line = strdup(s);
       at_tok_start(&line);

       err = at_tok_nextint(&line, &sim_status);
       //ALOGD("sim_status : %d", sim_status);
            //RIL_onUnsolicitedResponse (
            //   RIL_UNSOL_RESPONSE_NEW_SMS,
            //   sms_pdu, strlen(sms_pdu));
   } else if(strStartsWith(s,"^MODE:"))
   {   
       int sys_mode = 0;
	   int sys_submode = 0;
	   int radio_tech = 0;
	   //int tech;
       line = strdup(s);
       at_tok_start(&line);

       err = at_tok_nextint(&line, &sys_mode);
	   ALOGD("^MODE: phone_is=%d,dongle_vendor=%d,sys_mod = %d",phone_is,dongle_vendor,sys_mode);
	   
	   if(((sys_mode == 8) || (sys_mode == 2)) && (phone_is == MODE_GSM) && (dongle_vendor == STRONGRISING)) //jw  1103
	         phone_is = MODE_CDMA;
			 
	   if((phone_is == MODE_CDMA)/* && (dongle_vendor == STRONGRISING)*/) {
	      switch(sys_mode) {
		     case 2:
	           radio_tech = RADIO_TECH_1xRTT;
			   break;
			 case 8:
			   radio_tech = RADIO_TECH_EVDO_A;
			   break;
			 default:
			    radio_tech = 0;
			   break;
		 }
	   }
	   
	   err = at_tok_nextint(&line, &sys_submode);
	   ALOGD("^MODE: phone_is=%d,dongle_vendor=%d,sys_submode = %d",phone_is,dongle_vendor,sys_submode);
	   if((sys_mode == 5) && (dongle_vendor == HUAWEI)) {

	       switch(sys_submode) {
		      case 4:
	              radio_tech = RADIO_TECH_UMTS;
                  break;
			  case 5:
			      radio_tech = RADIO_TECH_HSDPA;
				  break;
			  case 6:
		          radio_tech = RADIO_TECH_HSUPA;
				  break;
			  case 7:
		          radio_tech = RADIO_TECH_HSPA;
				  break;
			  default:
			      radio_tech = RADIO_TECH_UMTS;
                  break;
		   }
	   } 
	   ALOGD("^MODE: phone_is=%d,dongle_vendor=%d,radio_tech = %d",phone_is,dongle_vendor,radio_tech);
       RIL_onUnsolicitedResponse (
                RIL_UNSOL_VOICE_RADIO_TECH_CHANGED,
                &radio_tech, sizeof(int));
       RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED, NULL, 0);

   } else if(strStartsWith(s, "^HRSSILVL:") && (dongle_vendor == STRONGRISING)) {
        int ss;
		int signal[] = {99, 99, 75, 99, 75, 99, 8, 99, 0x7FFFFFFE, 0x7FFFFFFE, 0x7FFFFFFE, 0x7FFFFFFE, 0x7FFFFFFE};
        line = strdup(s);
        at_tok_start(&line);
	    err = at_tok_nextint(&line, &ss);
		
		if(ss == 99) {
		    signal[2] = 75;
			signal[4] = 65;			
		} else if(ss == 80) {
		    signal[2] = 85;
			signal[4] = 75;
		} else if(ss == 60) {
			signal[2] = 95;
			signal[4] = 90;
		} else if(ss == 40 || ss == 20) {
		    signal[2] = 100;
			signal[4] = 105;
		} else if(ss == 0) {
		    signal[2] = 110;
			signal[4] = 110;
		}	
		
	   
        RIL_onUnsolicitedResponse (RIL_UNSOL_SIGNAL_STRENGTH, &signal, sizeof(signal));
   } else if(strStartsWith(s, "^RSSI:") && (phone_is == MODE_GSM)) {
        int ss;
        int signal[] = {99, 99, 75, 99, 75, 99, 8, 99, 0x7FFFFFFE, 0x7FFFFFFE, 0x7FFFFFFE, 0x7FFFFFFE, 0x7FFFFFFE};
        line = strdup(s);
        at_tok_start(&line);
	    err = at_tok_nextint(&line, &ss);
		
		signal[0] = ss;
		
        RIL_onUnsolicitedResponse (RIL_UNSOL_SIGNAL_STRENGTH, &signal, sizeof(signal));
		
   } else if(strStartsWith(s,"^SRVST:")) {
//        ALOGD("don't know...");
        RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED, NULL, 0);
   }else if (strStartsWith(s, "+CGEV:")) {
       RIL_requestTimedCallback (onDataCallListChanged, NULL, NULL);
#ifdef WORKAROUND_FAKE_CGEV
   }
   else if (strStartsWith(s, "+CME ERROR: 150"))
   {
       RIL_requestTimedCallback (onDataCallListChanged, NULL, NULL);
#endif /* WORKAROUND_FAKE_CGEV */
   }
   /*modified by zte-yuyang begin for  handling CBMI (new Broadcast SMS ) and CDSI (new SMS-STATUS-REPORT)*/
   else if ( strStartsWith(s, "+CMTI:")  || strStartsWith(s, "+CBMI:") || strStartsWith(s, "+CDSI:"))
   {
        onNewSmsNotification(s); // modify by FPL
   } 
   /* added by zte-yuyang begin */
    else if(strStartsWith(s,"+CUSD:"))
    {
       char *response;
       char *uusd_line;
       uusd_line = strdup(s);
       at_tok_start(&uusd_line);
       err = at_tok_nextstr(&uusd_line, &response);
        RIL_onUnsolicitedResponse (
           RIL_UNSOL_ON_USSD,
           response, strlen(response));
    }
   /* added by zte-yuyang end */
   
out:
   if(line != NULL) free(line);
	
   RLOGI("******** Leave onUnsolicited() ********");
}

#define USB_DONGLE_PATH_DEPTH 256

void get_at_modem_port(int vendor, char *at_port)
{
    char start_port_value[128];
    char datachannel[128] = {0};
    char pid[6];
	
    
    char uid_pid_path[USB_DONGLE_PATH_DEPTH] ={'\0'}; 
    char usb_vid[USB_DONGLE_PATH_DEPTH] ={'\0'}; 
    unsigned int vid_pid_cur = 0;
	
    property_get(LIB_PATH_START_PORT_PROPERTY, start_port_value, "");
	ALOGD("start port : %s", start_port_value);
	
	if(at_port == NULL)
	  return;
	  
    switch(vendor) {
	   case HUAWEI:	   //E160E only has ttyUSB0 and ttyUSB1
	       vid_pid_cur = getUsbVidPid("/sys/class/tty/ttyUSB0", uid_pid_path, USB_DONGLE_PATH_DEPTH,usb_vid);
		   
	       if (vid_pid_cur == 0x12d11003) {
	   
               int prop_at_port = 1;
			   sprintf(at_port, "/dev/ttyUSB%d", prop_at_port);
			   property_get(LIB_PATH_MODEM_PORT_PROPERTY, datachannel, "");
			   //property_set(LIB_PATH_MODEM_PORT_PROPERTY, datachannel);
			} else {   
    	       sprintf(at_port, "/dev/ttyUSB%d", (int)((start_port_value[6] -'0') + 2));
    	       ALOGD("%s,%d,start port : %s, modem port:%s",__FUNCTION__,__LINE__, at_port,"/dev/ttyUSB0");
    	       property_set(LIB_PATH_MODEM_PORT_PROPERTY, "/dev/ttyUSB0");
			}
	   break;
	   
	   case ZTE:
            getdevinfors("19d2", pid, at_port, datachannel, &phone_is);
            property_set(LIB_PATH_MODEM_PORT_PROPERTY, datachannel);
	   break;
	   
	   case STRONGRISING:
            property_get(LIB_PATH_AT_PORT_PROPERTY, at_port, "");
	   break;
	}
}

static unsigned int getUsbVidPid(const char *path, char *buffer, int len, char *usb_vid)
{
    //link path
    //../../devices/platform/sw-ehci.2/usb3/3-1/3-1.6/    3-1.6:1.4/ttyUSB4/tty/ttyUSB4
    //../../devices/platform/sw-ehci.2/usb3/3-1/    3-1:1.0/ttyUSB0/tty/ttyUSB0
    // /sys
    FILE *fp  = NULL;
    char *pos = NULL;
    int link_len = 0; 
    int i = 0;
    char tmp[256]= {'/','s','y','s','/','\0'};
    char tmp1[5] = {'\0'};
    unsigned int value  = 0;

    if(len < 4)
        return 0;
    
    memset(buffer, '\0', len*sizeof(char));
    
    link_len =  readlink(path, buffer, len - 1);
    if(link_len < 0 || link_len >= len - 1) {
        ALOGE("readfail code = %d", link_len);
        return 0;
    }

    for( i = 0; i < len - 4; ) {
        if( '.' == buffer[i] && '.' == buffer[i+1] && '/'== buffer[i+2]){
            i += 3;
        } else {
            break;
        }
    }  
    
    memcpy(tmp + 5, buffer + i, link_len);
    for( i = 4; i > 0 ; i-- ){    
       pos = (char *)strrchr(tmp, '/'); 
       *pos = '\0';
    }    
    
    strcat(tmp, "/idVendor");    
    fp = fopen(tmp, "r");
    if(!fp) {
        ALOGD("open file error 1");
        return 0;
    }    
    fread((void *)tmp1, 1, 4, fp);
    value  = strtol(tmp1,'\0', 16);
    value <<=16;    
    fclose(fp);
    memcpy(usb_vid,tmp1,4);

    *pos = '\0';

    strcat(tmp, "/idProduct");    
    fp = fopen(tmp, "r");
    if(!fp) {
        ALOGD("open file error 2");
        return 0;
    }    
    fread((void *)tmp1, 1, 4, fp);      
    value  |= strtol(tmp1,'\0', 16);  
    fclose(fp);   

    return value;    
}

void wait_sleep_time()
{
    //#define USB_DONGLE_PATH_DEPTH 256
    
    char uid_pid_path[USB_DONGLE_PATH_DEPTH] ={'\0'}; 
    char usb_vid[USB_DONGLE_PATH_DEPTH] ={'\0'}; 
    unsigned int vid_pid_cur = 0;
    int time = 8;
    
    if(access("/sys/class/tty/ttyUSB0", 0) == 0){
        vid_pid_cur = getUsbVidPid("/sys/class/tty/ttyUSB0", uid_pid_path, USB_DONGLE_PATH_DEPTH,usb_vid); 
        ALOGD("Get device vid-pid:0x%08x,usb_vid:%s", vid_pid_cur,usb_vid);
        if ((vid_pid_cur    == 0x12d11001)       //need more time,Huawei CDMA EC306
            ||(vid_pid_cur  == 0x19d2fff1)       //ZTE CDMA AC585
            ||(vid_pid_cur  == 0x19d2fff1))      //ZTE WCDMA MF668A
        {                         
            time = 12;
        }
    }
    ALOGD("sleep_time: %d s", time);    
    sleep(time);    
}

int skip_boot_time()
{
    struct sysinfo info;

    if (sysinfo(&info)) {
        ALOGE("Failed to get sysinfo, errno:%u, reason:%s\n", errno, strerror(errno));
        return -1;
    }
    
    ALOGD("System boot time is :%d s",info.uptime);
    
    if(info.uptime < SKIP_BOOT_TIME)
    {
        ALOGD("Need to sleep %d(s) first.",SKIP_BOOT_TIME - info.uptime);
        sleep(SKIP_BOOT_TIME - info.uptime);
    } else ALOGD("No need to sleep first.");

    return 0;
}

//disable autosuspend feature for many Dongles
void pre_process(){
    char *p,*cmd = NULL;
    ALOGD("disable autosuspend feature, echo 0 > /sys/devices/modem_dev/modem_autosuspend");
    asprintf(&cmd, "echo 0 > /sys/devices/modem_dev/modem_autosuspend &"); 
    system(cmd); 
}

//void post_process(){
//    char *p,*cmd = NULL;
//    asprintf(&cmd, "echo 1 > /sys/devices/modem_dev/modem_autosuspend &"); 
//    system(cmd);     
//    
//}
    
static void *
mainLoop(void *param)
{
    int fd;
    int ret;
    char at_port[128];
    char dongleInfo[128];
    struct termios new_termios, old_termios;
    char delay_init[PROPERTY_VALUE_MAX];
    int delay;
   
    ALOGD("******** Enter mainLoop() ********");
    AT_DUMP("== ", "entering mainLoop()", -1 );
   
    skip_boot_time();    //skip system boot time. Many Dongles (Huawei E3131s need it). fix bug #1740
   
    at_set_on_reader_closed(onATReaderClosed);
    at_set_on_timeout(onATTimeout);
   
    property_get("dongle.vendorinfo", dongleInfo, "");
   
    if(0 == strcmp(dongleInfo, "HUAWEI")) {
        dongle_vendor = HUAWEI;
    } else if(0 == strcmp(dongleInfo, "ZTE")) {
        dongle_vendor = ZTE;
    } else if(0 == strcmp(dongleInfo, "StrongRising")) {
        dongle_vendor = STRONGRISING;
    }
    
    ALOGD("the vendor is %d", dongle_vendor);
   
    get_at_modem_port(dongle_vendor, &at_port);
   
    if(dongle_vendor == STRONGRISING)
        sprintf(at_port, "/dev/ttyUSB1");
     
    pre_process();      //disable autosuspend feature for many Dongles
    

   for (;;) {
       fd = -1;
	   
	   ALOGD("at_port : %s", at_port);
              
	   fd = open (at_port, O_RDWR);
       if (fd < 0) {
              ALOGD ("opening AT interface. retrying...");           
               sleep(3);
               continue;
               /* never returns */
       }

       wait_sleep_time(); 
      
       tcgetattr(fd, &old_termios);
       new_termios = old_termios;
       new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
       new_termios.c_cflag |= (CREAD | CLOCAL);
       new_termios.c_cflag &= ~(CSTOPB | PARENB | CRTSCTS);
       new_termios.c_cflag &= ~(CBAUD | CSIZE) ;
       new_termios.c_cflag |= (B115200 | CS8);
       ret = tcsetattr(fd, TCSANOW, &new_termios);
	   
       if(ret < 0) {
           ALOGD ("Fail to set UART parameters. tcsetattr return %d \n", ret);
           return 0;
       }


       s_closed = 0;
       ret = at_open(fd, onUnsolicited);

       if (ret < 0) {
           ALOGD ("AT error %d on at_open\n", ret);
           return 0;
       }

       property_set("ril.currentapntype", "default"); 

       RIL_requestTimedCallback(initializeCallback, NULL, NULL);
       
       sleep(1);

       waitForClose();
       ALOGD("Re-opening after close");
   }
}


pthread_t s_tid_mainloop;


const RIL_RadioFunctions *RIL_Init(const struct RIL_Env *env, int argc, char **argv)
{
   int ret = 0;
   int fd = -1;
   int opt;

   int i = 0;
   pthread_attr_t attr;

   
   s_rilenv = env;
   ALOGD("Intel test: enter ril_init");

   pthread_attr_init (&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   ret = pthread_create(&s_tid_mainloop, &attr, mainLoop, NULL);

   return &s_callbacks;
}

