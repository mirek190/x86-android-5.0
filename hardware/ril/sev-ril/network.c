
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
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include <termios.h>
#include <arpa/inet.h>


#include "ril_common.h"
#include "network.h"

#define LOG_NDEBUG 0
#define LOG_TAG "RIL"
#include <utils/Log.h>

#include "ril.h"

#define PPP_TTY_PATH "ppp0"

#define PPP_OPERSTATE_PATH "/sys/class/net/ppp0/operstate"

#define SERVICE_PPPD_GPRS "pppd_gprs"

#define PROPERTY_PPPD_EXIT_CODE "net.ppp0.ppp-exit"
#define POLL_PPP_SYSFS_SECONDS	3
#define POLL_PPP_SYSFS_RETRY	20

#define PROP_NAME_MAX 1000

#define PROP_VALUE_MAX 1000


extern int dongle_vendor;

static int pppd = 0;

static char local_ip[PROPERTY_VALUE_MAX]={0};
static char remote_ip[PROPERTY_VALUE_MAX]={0};
static char local_pdns[PROPERTY_VALUE_MAX]={0};
static char local_sdns[PROPERTY_VALUE_MAX]={0};

extern int phone_has;
extern int phone_is;

static const char* networkStatusToRilString(int state)
{
    switch(state)
    {

    case 0:
        return("unknown");
        break;
    case 1:
        return("available");
        break;
    case 2:
        return("current");
        break;
    case 3:
        return("forbidden");
        break;
    default:
        return NULL;
    }
}

static void requestOrSendCdmaDataCallList(RIL_Token *t)
{
    ATResponse *p_response = NULL;
    int srv_status = 0;
	int srv_domain = 0;
	int roam_status = 0;
	int sys_mode = 0;
	int sim_status = 0;
	char *line = NULL;	
	RIL_Data_Call_Response_v6 responses;

	int err = -1;

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
	
	if(srv_domain == 3 || srv_domain == 2) { //ps
	    
		responses.status = 0;
        responses.suggestedRetryTime = -1;
        responses.cid = 1;
        responses.active = 2;
        		
        asprintf(&responses.addresses, "%s", local_ip);
        asprintf(&responses.dnses, "%s %s", local_pdns, local_sdns);
		asprintf(&responses.gateways, "%s", remote_ip);
    } else {
        responses.status = 0;
        responses.suggestedRetryTime = -1;
        responses.cid = 0;
        responses.active = -1;
        
        asprintf(&responses.type, "%s", "IP");
        asprintf(&responses.ifname, "%s", PPP_TTY_PATH);
        responses.addresses = "";
        responses.dnses = "";
        responses.gateways = "";
	}
	



	
	at_response_free(p_response);
	if (t != NULL)
        RIL_onRequestComplete(*t, RIL_E_SUCCESS, &responses,
                              sizeof(RIL_Data_Call_Response_v6));
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                                  &responses,
                                   sizeof(RIL_Data_Call_Response_v6));

error:
    if (t != NULL)
        RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                                  NULL, 0);

    at_response_free(p_response);

}

static void requestOrSendDataCallList(RIL_Token *t)
{
    ATResponse *p_response;
    ATLine *p_cur;
    int err;
    int n = 0;
    char *out;
    char ipaddress[16];
	


    err = at_send_command_multiline ("AT+CGACT?", "+CGACT:", &p_response);
    if (err != 0 || p_response->success == 0) {
        if (t != NULL)
            RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
        else
            RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                                      NULL, 0);
        return;
    }

    for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next)
        n++;

    RIL_Data_Call_Response_v6 *responses =
        alloca(n * sizeof(RIL_Data_Call_Response_v6));

    int i;
    for (i = 0; i < n; i++) {
        responses[i].status = -1;
        responses[i].suggestedRetryTime = -1;
        responses[i].cid = -1;
        responses[i].active = -1;
        responses[i].type = "";
        responses[i].ifname = "";
        responses[i].addresses = "";
        responses[i].dnses = "";
        responses[i].gateways = "";
    }

    RIL_Data_Call_Response_v6 *response = responses;
    for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next) {
        char *line = p_cur->line;

        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &response->cid);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &response->active);
        if (err < 0)
            goto error;

        response++;
    }

    at_response_free(p_response);

    err = at_send_command_multiline ("AT+CGDCONT?", "+CGDCONT:", &p_response);
    if (err != 0 || p_response->success == 0) {
        if (t != NULL)
            RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
        else
            RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                                      NULL, 0);
        return;
    }

    for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next) {
        char *line = p_cur->line;
        int cid;

        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &cid);
        if (err < 0)
            goto error;

        for (i = 0; i < n; i++) {
            if (responses[i].cid == cid)
                break;
        }


        if (i >= n) {
            /* details for a context we didn't hear about in the last request */
            continue;
        }

        // Assume no error
        responses[i].status = 0;

        // type
        err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;
        responses[i].type = alloca(strlen(out) + 1);
        strcpy(responses[i].type, out);

        err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;

        responses[i].ifname = alloca(strlen(PPP_TTY_PATH) + 1);
        strcpy(responses[i].ifname, PPP_TTY_PATH);

        err = at_tok_nextstr(&line, &out);
        if (err < 0)
           goto error;
        
        responses[i].addresses = alloca(strlen(local_ip) + 1);
        strcpy(responses[i].addresses, local_ip);

        
            
         /* I don't know where we are, so use the public Google DNS
          * servers by default and no gateway.
         */
         responses[i].dnses = "8.8.8.8 8.8.4.4";
         responses[i].gateways = "";
           
        
    }

    at_response_free(p_response);

    if (t != NULL)
        RIL_onRequestComplete(*t, RIL_E_SUCCESS, responses,
                              n * sizeof(RIL_Data_Call_Response_v6));
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                                  responses,
                                  n * sizeof(RIL_Data_Call_Response_v6));

    return;

error:
    if (t != NULL)
        RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                                  NULL, 0);

    at_response_free(p_response);
}



static void requestOrSendDataCallList(RIL_Token *t);

/*called by onUnsolicited() */
void onDataCallListChanged(void *param)
{
    requestOrSendDataCallList(NULL);
}

void requestCdmaDataCallList(void *data, size_t datalen, RIL_Token t)
{
    requestOrSendCdmaDataCallList(&t);
}

void requestDataCallList(void *data, size_t datalen, RIL_Token t)
{
    requestOrSendDataCallList(&t);
}


void requestQueryNetworkSelectionMode(void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;
    int response = 0;
    char *line;

    err = at_send_command_singleline("AT+COPS?", "+COPS:", &p_response);

    if (err < 0 || p_response->success == 0)
    {
        goto error;
    }

    line = p_response->p_intermediates->line;
    ALOGD("requestQueryNetworkSelectionMode initial-line=%s",line);
    err = at_tok_start(&line);

    if (err < 0)
    {
        goto error;
    }

    err = at_tok_nextint(&line, &response);


    if (err < 0)
    {
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));
    at_response_free(p_response);
    return;
error:
    at_response_free(p_response);
    ALOGD("requestQueryNetworkSelectionMode must never return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

void requestQueryCdmaNetworkSelectionMode(void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;
    int response = 0;
    char *line;

    err = at_send_command_singleline("AT^PREFMODE?", "^PREFMODE:", &p_response);

    if (err < 0 || p_response->success == 0)
    {
        goto error;
    }

    line = p_response->p_intermediates->line;
    ALOGD("requestQueryNetworkSelectionMode initial-line=%s",line);
    err = at_tok_start(&line);

    if (err < 0)
    {
        goto error;
    }

    err = at_tok_nextint(&line, &response);

    if (err < 0)
    {
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));
    at_response_free(p_response);
    return;
error:
    at_response_free(p_response);
    ALOGD("requestQueryNetworkSelectionMode must never return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

void requestQueryAvailableNetworks(void *data, size_t datalen, RIL_Token t)
{
    /* We expect an answer on the following form:
       +COPS: (2,"AT&T","AT&T","310410",0),(1,"T-Mobile ","TMO","310260",0)
     */

    int err, operators, i, skip, status;
    ATResponse *p_response = NULL;
    char * c_skip, *line, *p = NULL;
    char ** response = NULL;

    err = at_send_command_singleline("AT+COPS=?", "+COPS:", &p_response);

    if (err < 0 || p_response->success == 0) goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    /* Count number of '(' in the +COPS response to get number of operators*/
    operators = 0;
    for (p = line ; *p != '\0' ; p++)
    {
        if (*p == '(') operators++;
    }

    response = (char **)alloca(operators * 4 * sizeof(char *));

    for (i = 0 ; i < operators ; i++ )
    {
        err = at_tok_nextstr(&line, &c_skip);
        if (err < 0) goto error;
        if (strcmp(c_skip,"") == 0)
        {
            operators = i;
            continue;
        }
        status = atoi(&c_skip[1]);
        response[i*4+3] = (char*)networkStatusToRilString(status);

        err = at_tok_nextstr(&line, &(response[i*4+0]));
        if (err < 0) goto error;

        err = at_tok_nextstr(&line, &(response[i*4+1]));
        if (err < 0) goto error;

        err = at_tok_nextstr(&line, &(response[i*4+2]));
        if (err < 0) goto error;

        err = at_tok_nextstr(&line, &c_skip);

        if (err < 0) goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, (operators * 4 * sizeof(char *)));
    at_response_free(p_response);
    return;

error:
    at_response_free(p_response);
    ALOGD("ERROR - requestQueryAvailableNetworks() failed");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

void requestSignalStrength(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    int response[13];
    char *line;


    err = at_send_command_singleline("AT+CSQ", "+CSQ:", &p_response);

    if (err < 0 || p_response->success == 0)
    {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response[0]));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response[1]));
    if (err < 0) goto error;

    response[2] = 75;
    response[3] = 99;
    response[4] = 65;
    response[5] = 99;
    response[6] = 8;

    response[7] = 99;
    response[8] = 0x7FFFFFFE;
    response[9] = 0x7FFFFFFE;
    response[10]= 0x7FFFFFFE;
    response[11]= 0x7FFFFFFE;
    response[12]= 0x7FFFFFFE;
 

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));

    at_response_free(p_response);
    return;


error:
    RLOGE("requestSignalStrength must never return an error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

void requestCdmaRegistrationState(int request, void *data,size_t datalen, RIL_Token t)
{
   
 
    char * responseStr[14];
    ATResponse *p_response = NULL;

    int count = 3;

	int srv_status = 0;
	int srv_domain = 0;
	int roam_status = 0;
	int sys_mode = 0;
	int sim_status = 0;
	int lock_status = 0;
	int sys_submode = 0;
	char *line = NULL;
	

	int err = -1;

    err = at_send_command_singleline("AT^SYSINFO", "^SYSINFO:", &p_response);
    if (err < 0 || p_response->success == 0) goto error;
    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;
	err = at_tok_nextint(&line, &srv_status);
	ALOGD("requestCdmaRegistrationState srv_status: %d",srv_status);
	
    if (err < 0) goto error;
	err = at_tok_nextint(&line, &srv_domain);
	ALOGD("requestCdmaRegistrationState srv_domain: %d",srv_domain);
	
    if (err < 0) goto error;
	err = at_tok_nextint(&line, &roam_status);
	ALOGD("requestCdmaRegistrationState roam_status: %d",roam_status);
	
    if (err < 0) goto error;
	err = at_tok_nextint(&line, &sys_mode);
	ALOGD("requestCdmaRegistrationState sys_mode: %d",sys_mode);
	
    if (err < 0) goto error;
	err = at_tok_nextint(&line, &sim_status);
	ALOGD("requestCdmaRegistrationState sim_status: %d",sim_status);
    if (err < 0) goto error;
	
    if(at_tok_hasmore(&line)) {
        ALOGD("requestCdmaRegistrationState 1: %c",*line);
        ALOGD("requestCdmaRegistrationState 1: %s",line);
        if(*line++ == ',') ;
        else line++;    //skip <lock_state> no null
        ALOGD("requestCdmaRegistrationState 2: %c",*line) ;
	    ALOGD("requestCdmaRegistrationState 2: %s",line) ;   
    	if (at_tok_hasmore(&line)) {
    	  //lock_status = sys_submode;
            
    	  err = at_tok_nextint(&line, &sys_submode);
          if (err < 0) goto error;
    	}
	}
	RLOGD("srv_status %d, srv_domain %d, roam_status %d, sys_mode %d, sim_status %d, lock_status %d, sys_submode %d", srv_status, srv_domain, roam_status, sys_mode, sim_status, lock_status, sys_submode);

	
	if(srv_status == 2) {
	  asprintf(&responseStr[0], "%d", 1);
	  asprintf(&responseStr[8], "%d", 13844);
	  asprintf(&responseStr[9], "%d", 3);
	}
	else {
	  asprintf(&responseStr[0], "%d", 0);
	  asprintf(&responseStr[8], "%d", 0);
	  asprintf(&responseStr[9], "%d", 0);
	}  
	  //asprintf(&responseStr[1], "%d", 0);
	  
	  responseStr[1] = NULL;
	  responseStr[2] = NULL;
	  
      if((dongle_vendor == HUAWEI) ) {
          int radio_tech = 6;
 
	       switch(sys_submode) {
/*	  
in ril.h      
typedef enum {
    RADIO_TECH_UNKNOWN = 0,
    RADIO_TECH_GPRS = 1,
    RADIO_TECH_EDGE = 2,
    RADIO_TECH_UMTS = 3,
    RADIO_TECH_IS95A = 4,
    RADIO_TECH_IS95B = 5,
    RADIO_TECH_1xRTT =  6,
    RADIO_TECH_EVDO_0 = 7,
    RADIO_TECH_EVDO_A = 8,
    RADIO_TECH_HSDPA = 9,
    RADIO_TECH_HSUPA = 10,
    RADIO_TECH_HSPA = 11,
    RADIO_TECH_EVDO_B = 12,
    RADIO_TECH_EHRPD = 13,
    RADIO_TECH_LTE = 14,
    RADIO_TECH_HSPAP = 15, // HSPA+
    RADIO_TECH_GSM = 16 // Only supports voice
} RIL_RadioTechnology;
*/    	        
            case 4:
                radio_tech = RADIO_TECH_IS95A;
                break;
            case 5:
                radio_tech = RADIO_TECH_IS95B;
                break;
            case 6:
                radio_tech = RADIO_TECH_1xRTT;
                break;                
            case 7:
                radio_tech = RADIO_TECH_EVDO_0;
                break;
            case 8:
                radio_tech = RADIO_TECH_EVDO_A;
                break;
            case 12:
                radio_tech = RADIO_TECH_EVDO_B;
                break;                       
            case 13:
                radio_tech = RADIO_TECH_EHRPD;
                break;                             
		    default:
			      radio_tech = RADIO_TECH_1xRTT;
                  break;
		   }

		    asprintf(&responseStr[3], "%d", radio_tech);
	       
	
	   } else {  //ZTE,StrongRising
#if 0
	     if(sys_mode == 8)
	        asprintf(&responseStr[3], "%d", sys_mode);
	     else if(sys_mode == 2)
	        asprintf(&responseStr[3], "%d", RADIO_TECH_1xRTT);
	      else
	        asprintf(&responseStr[3], "%d", 0);
#else
	     if((sys_mode == 8)||(sys_mode == 2)||(sys_mode == 13))
	        asprintf(&responseStr[3], "%d", RADIO_TECH_EHRPD);
	      else
	        asprintf(&responseStr[3], "%d", 0);
#endif	        
	  }	  
	   
	   asprintf(&responseStr[4], "%d", -1);
	   
	   asprintf(&responseStr[5], "%d", 0x7FFFFFFF);
	   asprintf(&responseStr[6], "%d", 0x7FFFFFFF);
	   
	   asprintf(&responseStr[7], "%d", 0);
	   
	   asprintf(&responseStr[10], "%d", 1);
	   asprintf(&responseStr[11], "%d", 1);
	   asprintf(&responseStr[12], "%d", 1);
	   asprintf(&responseStr[13], "%d", 0);

	   
	  count = 14;
	  

    ALOGD("requestRegistrationState=%s",*responseStr);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, count*sizeof(char*));
    at_response_free(p_response);

    return;
error:
    ALOGD("requestRegistrationState must never return an error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

void requestRegistrationState(int request, void *data,size_t datalen, RIL_Token t)
{
    int err;
    int response[4];
    char * responseStr[4];
    ATResponse *p_response = NULL;
    const char *cmd;
    const char *prefix;
    char *line, *p;
    int commas;
    int skip;
    int count = 3;
	int networkType = 0;
	
	int srv_status = 0;
	int srv_domain = 0;
	int roam_status = 0;
	int sys_mode = 0;
	int sim_status = 0;
	int lock_status = 0;
	int sys_submode = 0;

	
	 responseStr[1] = NULL;
	 responseStr[2] = NULL;
	 
	 RLOGD("****requestRegistrationState****");

    if ((request == RIL_REQUEST_VOICE_REGISTRATION_STATE))
    {
        cmd = "AT+CREG?";
        prefix = "+CREG:";
    }
    else if ((request == RIL_REQUEST_DATA_REGISTRATION_STATE))
    {
        if(dongle_vendor == ZTE){
            cmd = "AT+CREG?";
            prefix = "+CREG:";            
        }else {
            cmd = "AT+CGREG?";
            prefix = "+CGREG:";
        }
    }
    else
    {
        assert(0);
        goto error;
    }

    err = at_send_command_singleline(cmd, prefix, &p_response);

    if (err != 0 || p_response->success == 0) goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    /* Ok you have to be careful here
     * The solicited version of the CREG response is
     * +CREG: n, stat, [lac, cid]
     * and the unsolicited version is
     * +CREG: stat, [lac, cid]
     * The <n> parameter is basically "is unsolicited creg on?"
     * which it should always be
     *
     * Now we should normally get the solicited version here,
     * but the unsolicited version could have snuck in
     * so we have to handle both
     *
     * Also since the LAC and CID are only reported when registered,
     * we can have 1, 2, 3, or 4 arguments here
     *
     * finally, a +CGREG: answer may have a fifth value that corresponds
     * to the network type, as in;
     *
     *   +CGREG: n, stat [,lac, cid [,networkType]]
     */

    /* count number of commas */
    commas = 0;
    for (p = line ; *p != '\0' ; p++)
    {
        if (*p == ',') commas++;
    }

    switch (commas)
    {
    case 0: /* +CREG: <stat> */
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
        response[1] = -1;
        response[2] = -1;
        break;

    case 1: /* +CREG: <n>, <stat> */
        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
        response[1] = -1;
        response[2] = -1;
        response[3] = -1;
        if (err < 0) goto error;
        break;

    case 2: /* +CREG: <stat>, <lac>, <cid> */
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[1]);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[2]);
        if (err < 0) goto error;
        break;
    case 3: /* +CREG: <n>, <stat>, <lac>, <cid> */
        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[1]);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[2]);
        if (err < 0) goto error;
        break;
    case 4:/* +CGREG: <n>, <stat>, <lac>, <cid>, <networkType> */
        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[1]);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[2]);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[3]);
        if (err < 0) goto error;
		count = 4;  
        break;
    default:
        goto error;
    }
	
	asprintf(&responseStr[0], "%d", response[0]);
    asprintf(&responseStr[1], "%x", response[1]);
    asprintf(&responseStr[2], "%x", response[2]);
	
	if(dongle_vendor == ZTE){
        char * nettypestr;
        ATResponse *p_nettyperesponse=NULL;
        char *nettypeline,*p_nettype;
        int i=0;//added by zte-yuyang for rild-closed at weak signal
	    
        if (response[1] > 0)
            asprintf(&responseStr[1], "%04x", response[1]);
        else
            responseStr[1] = NULL;
    
        if (response[2] > 0)
            asprintf(&responseStr[2], "%08x", response[2]);
        else
            responseStr[2] = NULL;
      //add by gaoijing for CREG return 5 parameters begin 
        if (response[3] > 0)
        	  asprintf(&responseStr[3], "%d", response[3]);
        else
        	  responseStr[3] = NULL;    	
      //add by gaojing for CREG return 5 paramters end
    
#if 1
        err = at_send_command_singleline("AT+ZPAS?", "+ZPAS:", &p_nettyperesponse);
    
        if (err != 0 || p_nettyperesponse->success == 0) goto error;
    
        nettypeline = p_nettyperesponse->p_intermediates->line;
    
        err = at_tok_start(&nettypeline);
        if (err < 0) goto error;
        err = at_tok_nextstr(&nettypeline, &nettypestr);
        if (err < 0) goto error;
        if((!strcmp(nettypestr,"UMTS"))||(!strcmp(nettypestr,"HSPA"))||(!strcmp(nettypestr,"HSDPA"))
                ||(!strcmp(nettypestr,"HSUPA"))||(!strcmp(nettypestr,"3G")))
        {
            count++;
            networkType=3;
            ALOGD("ZTE-3G-networkType=%d",networkType);
        }
        if((!strcmp(nettypestr,"GSM"))||(!strcmp(nettypestr,"GPRS")))
        {
            count++;
            networkType=1;
            ALOGD("ZTE-GPRS-networkType=%d",networkType);
        }
        if(!strcmp(nettypestr,"EDGE"))
        {
            count++;
            networkType=2;
            ALOGD("ZTE-EDGE-networkType=%d",networkType);
        }
        ALOGD("ZTE-FINAL-networkType=%d",networkType);
        asprintf(&responseStr[3], "%d", networkType);
        /* modified by zte-yuyang for rild-closed at weak signal begin */
        for(i=0; i<4; i++)
        {
            if(responseStr[i] != NULL)
            {
                ALOGD("ZTE-FINAL-responseStr[%d]=%d",i,*responseStr[i]);
            }
        }
        /* modified by zte-yuyang for rild-closed at weak signal end */
#endif
    //    asprintf(&responseStr[3], "%d", 3);
    
        ALOGD("ZTE-requestRegistrationState=%s",*responseStr);//modified by zte-yuyang
        RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, count*sizeof(char*));

	}else {
    	err = at_send_command_singleline("AT^SYSINFO", "^SYSINFO:", &p_response);
        if (err < 0 || p_response->success == 0) goto error;
        line = p_response->p_intermediates->line;
        err = at_tok_start(&line);
        if (err < 0) goto error;
    	
    	 /* count number of commas */
        commas = 0;
        for (p = line ; *p != '\0' ; p++)
        {
            if (*p == ',') commas++;
        }
    	
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
    	
    	ALOGD("the last line = %s", line);
    	
    	if (at_tok_hasmore(&line)) {
    	  line++;
    	  
    	 // err = at_tok_nextint(&line, &lock_status);
        //  if (err < 0) goto error;
    	
    	  ALOGD("%s", line);
    
    	  err = at_tok_nextint(&line, &sys_submode);
          if (err < 0) goto error;
    	}
    	
    	ALOGD("srv_status %d, srv_domain %d, roam_status %d, sys_mode %d, sim_status %d, lock_status %d, sys_submode %d", 
    	       srv_status, srv_domain, roam_status, sys_mode, sim_status, lock_status, sys_submode);
    
          if((dongle_vendor == HUAWEI) && (sys_mode == 5)) {
    	  
    	      //if((dongle_vendor == HUAWEI)) {
               int radio_tech = 0;
    	   
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
    		     response[3]  = radio_tech;	
    			 asprintf(&responseStr[3], "%d", response[3]);       
    	
    	   } else {  //StrongRising
    	     if(sys_mode == 8)
    	        //asprintf(&responseStr[3], "%d", sys_mode);
    			response[3] = RADIO_TECH_EVDO_A;
    	      else
    	        //asprintf(&responseStr[3], "%d", 0);
    			response[3] = sys_mode;
    			
    		  asprintf(&responseStr[3], "%d", response[3]);
    	  }	  

        RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, 4*sizeof(char*));
    }
    at_response_free(p_response);

    return;
error:
    ALOGD("requestRegistrationState must never return an error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

int requestIMSI(char * IMSI_buffer){
    ATResponse *p_response = NULL;
    int err = 0;
    char * responseStr[5],*line;
    char *temp;

    /*case 1:
        sev-ril : AT> AT+CIMI
        sev-ril : AT< 460011962607040
        sev-ril : AT< OK    
    */	     
    err = at_send_command_numeric("AT+CIMI", &p_response);
    if (err < 0 || p_response->success == 0) {
        goto IMSI_try_again1;
    } else {
        strncpy(IMSI_buffer, p_response->p_intermediates->line, 30);
        at_response_free(p_response);
        return 0;
    }
       
IMSI_try_again1:
    /*case 2:
        sev-ril : AT> AT+CIMI
        sev-ril : AT< ^CIMI:460036181032426
        sev-ril : AT< OK    
    */	    
    err = at_send_command_singleline("AT+CIMI", "^CIMI:", &p_response);     //AT< ^CIMI:460036181032426  ZTE CDMA AC2787 return
    if (err < 0 || p_response->success == 0)
        goto IMSI_try_again2;
    
    line = p_response->p_intermediates->line;
    err = at_tok_start (&line);
    if (err < 0)    
        goto error;         
    
    err = at_tok_nextstr(&line, &temp);			
    if (err < 0)
        goto error;   	
        
    goto success;

IMSI_try_again2:   	        	
    /*case 3:
        sev-ril : AT> AT+CIMI
        sev-ril : AT< +CIMI:460036181032426
        sev-ril : AT< OK    
    */	    
    err = at_send_command_singleline("AT+CIMI", "+CIMI:", &p_response);     //AT< ^CIMI:460036181032426  ZTE CDMA AC2787 return
    if (err < 0 || p_response->success == 0)
        goto error;
    
    line = p_response->p_intermediates->line;
    err = at_tok_start (&line);
    if (err < 0)    
        goto error;         
    
    err = at_tok_nextstr(&line, &temp);			
    if (err < 0)
        goto error;   	

success:        
    memcpy(IMSI_buffer, temp, strlen(temp));
    at_response_free(p_response);
    return 0;

error:
    return -1;     
}

void requestCDMAOperator(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err = 0;
	char *response[3] = {"CDMA", "CDMA", ""};
	static char operid[12];
	char temp[30]={0};
	
	memset(response, 0, sizeof(response));
	memset(operid, 0, sizeof(operid));
	
	ALOGD("get CDMA Operater information");
	
#if 0	
    err = at_send_command_numeric("AT+CIMI", &p_response);

    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
	    strncpy(operid, p_response->p_intermediates->line, 5);
		
		asprintf(&response[0], "CDMA");
		asprintf(&response[1], "CDMA");
		response[2] = operid;
		
        RIL_onRequestComplete(t, RIL_E_SUCCESS,
                              response, sizeof(response));
    }
    at_response_free(p_response);
#else
    err = requestIMSI(temp);
    if(err == 0){
        strncpy(operid, temp, 5);
		asprintf(&response[0], "CDMA");
		asprintf(&response[1], "CDMA");
		response[2] = operid;
        RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));		        
    }else{
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
#endif      
	return;
}

void requestOperator(void *data, size_t datalen, RIL_Token t)
{
    int err;
    int i;
    int skip;
    ATLine *p_cur;
    char *response[3];

    memset(response, 0, sizeof(response));

    ATResponse *p_response = NULL;
	
	

    err = at_send_command_multiline(
              "AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?",
              "+COPS:", &p_response);

    /* we expect 3 lines here:
     * +COPS: 0,0,"T - Mobile"
     * +COPS: 0,1,"TMO"
     * +COPS: 0,2,"310170"
     */

    if ( err < 0 || p_response->success == 0) goto error;

    for (i = 0, p_cur = p_response->p_intermediates
                        ; p_cur != NULL
            ; p_cur = p_cur->p_next, i++
        )
    {
        char *line = p_cur->line;

        err = at_tok_start(&line);
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;

        // If we're unregistered, we may just get
        // a "+COPS: 0" response
        if (!at_tok_hasmore(&line))
        {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;

        // a "+COPS: 0, n" response is also possible
        if (!at_tok_hasmore(&line))
        {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextstr(&line, &(response[i]));
        if (err < 0) goto error;
    }

    if (i != 3)
    {
        /* expect 3 lines exactly */
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
    at_response_free(p_response);

    return;
error:
    ALOGD("requestOperator must not return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}



static void dataCallsetup(RIL_Token *t) 
{
    int fd;
	char buffer[20];
    char exit_code[PROPERTY_VALUE_MAX]={0};
    int retry = POLL_PPP_SYSFS_RETRY;
    int ready_for_connect = 0;
    int try_connect_numbers = 0;
    char apntype[128];
	int i = 0;
	
    sleep(1);
    do {
        // Check whether PPPD exit abnormally
        property_get(PROPERTY_PPPD_EXIT_CODE, exit_code, "");
        if(strcmp(exit_code, "") != 0) {
            RLOGI("PPPd exit with code %s", exit_code);
            retry = 0;
            break;
        }

        fd  = open(PPP_OPERSTATE_PATH, O_RDONLY);
        if (fd >= 0) {
            buffer[0] = 0;
            read(fd, buffer, sizeof(buffer));
            close(fd);
            if(!strncmp(buffer, "up", strlen("up")) || !strncmp(buffer, "unknown", strlen("unknown"))) {
                // Should already get local IP address from PPP link after IPCP negotation
                // system property net.ppp0.local-ip is created by PPPD in "ip-up" script
                //local_ip[0] = 0;
                property_get("net.ppp0.local-ip", local_ip, "");
                property_get("net.ppp0.dns1", local_pdns, "");
                property_get("net.ppp0.dns2", local_sdns, "");
        	    property_get("ril.currentapntype", apntype, "");
				property_get("net.ppp0.gw", remote_ip, "");

                ALOGD("local_pdns:%s",local_pdns);
                ALOGD("local_sdns:%s",local_sdns);
                ALOGD("local_ip:%s",local_ip);
				ALOGD("remote_ip:%s",remote_ip);
                ALOGD("ril.currentapntype:%s",apntype);
               
                if ((!strcmp(apntype, "mms")))  {
                    if((!strcmp(local_ip, "")))  {
                        ALOGD("PPP link is up but no local IP is assigned. Will retry %d times after %d seconds", \
                             retry, POLL_PPP_SYSFS_SECONDS);
                    } else {
                        RLOGI("PPP link is up with local IP address %s", local_ip);
                        // other info like dns will be passed to framework via property (set in ip-up script)
			       
                        // now we think PPPd is ready
                        break;
                    }                	
                } else {
                    // if(!strcmp(local_ip, ""))
                    if((!strcmp(local_ip, "")) || (!strcmp(local_pdns,"")) || 
                    	 (!strcmp(local_sdns,"10.11.12.13"))  ||(!strcmp(local_sdns,"10.11.12.14")) )  {
                        ALOGD("PPP link is up but no local IP is assigned. Will retry %d times after %d seconds", \
                             retry, POLL_PPP_SYSFS_SECONDS);
                    } else {
                        ALOGD("PPP link is up with local IP address %s", local_ip);
                        // other info like dns will be passed to framework via property (set in ip-up script)
				       
                        // now we think PPPd is ready
                        break;
                    }
                }

            }  else  {
                ALOGI("PPP link status in %s is %s. Will retry %d times after %d seconds", \
                     PPP_OPERSTATE_PATH, buffer, retry, POLL_PPP_SYSFS_SECONDS);
            }
        } else {
            RLOGI("Can not detect PPP state in %s. Will retry %d times after %d seconds", \
                 PPP_OPERSTATE_PATH, retry-1, POLL_PPP_SYSFS_SECONDS);
        }
        sleep(POLL_PPP_SYSFS_SECONDS);
    }  while (--retry);
	
    RLOGI("******** local ip %s ********", local_ip);

	//requestOrSendDataCallList(&t);
	if(local_ip[0] == 0)        //not a valid ip addr
	    goto error;
   
	{
	 RIL_Data_Call_Response_v6 responses;
	 		
		responses.type = "";
        responses.ifname = "";
        responses.addresses = "";
        responses.dnses = "";
        responses.gateways = "";
		
		//memset(&responses, 0x0, sizeof(RIL_Data_Call_Response_v6));
		responses.status = 0;
        responses.suggestedRetryTime = -1;
        responses.cid = 1;
        responses.active = 2;
        asprintf(&responses.type, "%s", "IP");
        //responses.ifname = "";
		asprintf(&responses.ifname, "%s", PPP_TTY_PATH);
        asprintf(&responses.addresses, "%s", local_ip);
        asprintf(&responses.dnses, "%s %s", local_pdns, local_sdns);
		asprintf(&responses.gateways, "%s", remote_ip);
    
		
	if (t != NULL)
        RIL_onRequestComplete(*t, RIL_E_SUCCESS, &responses,
                              sizeof(RIL_Data_Call_Response_v6));
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                                  &responses,
                                   sizeof(RIL_Data_Call_Response_v6));

   }

    

    return;


 error:

    RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
   
}

void requestCdmaSetupDataCall(void *data, size_t datalen, RIL_Token t)
{
	int err = 0;
	const char *apn;
	const char *pdp_type =NULL;  
    const char *username =NULL;  
    const char *password =NULL;  
    const char *auth_type =NULL;

	char *cmd;


	
    property_set("ctl.stop", SERVICE_PPPD_GPRS);

    ALOGD("******** Enter CdmasetupDataCall ********");
	
	 apn = ((const char **)data)[2];

     username = ((const char **)data)[3]; 
     password = ((const char **)data)[4]; 
     auth_type = ((const char **)data)[5]; 

      if (datalen > 6 * sizeof(char *)) {
            pdp_type = ((const char **)data)[6];
      } else  {
            pdp_type = "IP";
      }
		
	  property_set("net.ppp0.user", username); 
      property_set("net.ppp0.password", password); 
	  
	  property_set(PROPERTY_PPPD_EXIT_CODE, "");
      property_set("net.ppp0.local-ip", "");
	  
	  property_set("ril.dialer", "/etc/ppp/cdma_ppp_dialer");
	  
      
	  
     err = at_send_command("ATH", NULL);
	 ALOGD("******** start pppd now ********");
     err = property_set("ctl.start", SERVICE_PPPD_GPRS);
	 
	 
     dataCallsetup(&t);

}

void requestGsmSetupDataCall(void *data, size_t datalen, RIL_Token t)
{

	int err = 0;
	const char *apn;
	const char *pdp_type =NULL;  
    const char *username =NULL;  
    const char *password =NULL;  
    const char *auth_type =NULL;

	char *cmd;
	
   // pthread_t       ppid;
	
    property_set("ctl.stop", SERVICE_PPPD_GPRS);

    ALOGD("******** Enter setupDataCall ********");
	
	 apn = ((const char **)data)[2];

     username = ((const char **)data)[3]; 
     password = ((const char **)data)[4]; 
     auth_type = ((const char **)data)[5]; 

      if (datalen > 6 * sizeof(char *)) {
            pdp_type = ((const char **)data)[6];
      } else  {
            pdp_type = "IP";
      }
	

	
	  property_set("net.ppp0.user", username); 
      property_set("net.ppp0.password", password); 
	  
	  property_set(PROPERTY_PPPD_EXIT_CODE, "");
      property_set("net.ppp0.local-ip", "");
	  
	  property_set("ril.dialer", "/etc/ppp/gsm_ppp_dialer");
   
	  
    err = at_send_command("ATH", NULL);
	
    //err = property_set("ctl.start", SERVICE_PPPD_GPRS);
	 
    RLOGI("requesting data connection to APN '%s'", apn);
    asprintf(&cmd, "AT+CGDCONT=1,\"IP\",\"%s\",,0,0", apn);
    err = at_send_command(cmd, NULL);
    free(cmd);
    sleep(2);

    // Set required QoS params to default
    err = at_send_command("AT+CGQREQ=1", NULL);
    usleep(100000);
    // Set minimum QoS params to default
    err = at_send_command("AT+CGQMIN=1", NULL);
    usleep(100000);
    // packet-domain event reporting
    err = at_send_command("AT+CGEREP=1,0", NULL);
    usleep(100000);

    // Hangup anything that's happening there now
    err = at_send_command("AT+CGACT?", NULL);
    err = at_send_command("AT+CGACT=0,1", NULL);
    sleep(3);
    err = at_send_command("AT+CGACT?", NULL);

    usleep(100000);
	
	
	RLOGI("******** start pppd now ********");
    property_set(PROPERTY_PPPD_EXIT_CODE, "");
    property_set("net.ppp0.local-ip", "");
	
    err = property_set("ctl.start", SERVICE_PPPD_GPRS);
	dataCallsetup(&t);
}

static int killConn(char *cid)
{
	int err = 0;
	char * cmd;
	int i = 0;
	ATResponse *p_response = NULL;

	if (access(PPP_OPERSTATE_PATH, F_OK) == 0) {
		/* Did we already send a kill? */
		
		ALOGD("killConn, set %s stop.", SERVICE_PPPD_GPRS);
		property_set("ctl.stop", SERVICE_PPPD_GPRS);
		for (i=0; i<25; i++) {
			sleep(1);
		    ALOGD("killConn, time = %d, scan %s node", i,PPP_OPERSTATE_PATH);	
			if (access(PPP_OPERSTATE_PATH, F_OK)){
			    ALOGD("killConn, time = %d, scan %s node -- dismiss , OK, break", i,PPP_OPERSTATE_PATH);	    
				break;
			}
		}
		if (i == 25){
		    ALOGD("killConn, timeout");	
			goto error;
		}
	}
	
	if (phone_is == MODE_GSM) {
		asprintf(&cmd, "AT+CGACT=0,%s", cid);
		err = at_send_command(cmd, &p_response);
		free(cmd);
	} else {
		//CDMA
		;
	}
	
	at_response_free(p_response);
	if (err)
		goto error;

	return 0;

error:
	return -1;
}

void requestDeactivateDataCall(void *data, size_t datalen, RIL_Token t)
{
	char *cid;
	int fd,written;
	
	ALOGD("Enter: DeactivateDataCall.");
	
	local_ip[0] = 0;
	
    cid = ((char **)data)[0];
    if (killConn(cid) < 0){
        goto error;
	}

	RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
	return;

error:
	RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


void requestGetPreferredNetworkType(void *data, size_t datalen, RIL_Token t)
{
    int err;

    int response = 3;

    ALOGD("requestGetPreferredNetworkType response=%d",response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));

    return;
error:
 
    ALOGD("ERROR: requestGetPreferredNetworkType() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

void requestSetPreferredNetworkType(void *data, size_t networktype, RIL_Token t)
{
    int err, rat;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, networktype, sizeof(int));

    return;
error:

    ALOGD("ERROR: requestSetPreferredNetworkType() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

void requestSetNetworkSelectionManual(void *data, size_t datalen, RIL_Token t)
{
    char *operator = NULL;
    char *cmd = NULL;
    int err = 0;
    ATResponse *p_response = NULL;

    char *Act=NULL;	
    ALOGD("requestSetNetworkSelectionManual raw data=%s",(char *)data);
    operator = ((char **)data)[0];
    Act= ((char **)data)[1];	
       ALOGD("requestSetNetworkSelectionManual operator data=%s",operator);
   if(Act!=NULL && strcmp(Act,""))       
             asprintf(&cmd, "AT+COPS=1,2,\"%s\",%s", operator,Act);
    else {
        if(operator == NULL) goto error;
        asprintf(&cmd, "AT+COPS=1,2,\"%s\" ", operator);	
	}

    err = at_send_command(cmd, &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0)
    {
			goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);

    return;

error:
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

void requestNeighboringCellIds(void * data, size_t datalen, RIL_Token t)
{
    int err;
    int response[4];
    char * responseStr[4];
    ATResponse *p_response = NULL;
    char *line, *p;
    int commas;
    int skip;
    int i,j;
    int count = 3;

    RIL_NeighboringCell **pp_cellIds;
    RIL_NeighboringCell *p_cellIds;

    pp_cellIds = (RIL_NeighboringCell **)alloca(sizeof(RIL_NeighboringCell *));
    p_cellIds = (RIL_NeighboringCell *)alloca(sizeof(RIL_NeighboringCell));
    pp_cellIds[0]=p_cellIds;

    ALOGD("Starting Loop of CREG");
    err = 1;
    for (i=0; i<4 && err != 0; i++)
        err = at_send_command_singleline("AT+CREG?", "+CREG:", &p_response);

    if (err < 0 || p_response->success == 0 ) goto error;
    ALOGD("Out of Loop of CREG");

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;
    ALOGD("Counting Commas");
    for(j=0; j<1; j++)
    {
        ALOGD("J=%d",j);
        commas=0;
        for(p=line; *p!='\0'; p++)
        {
            if(*p==',') commas++;
        }
        switch(commas)
        {
        case 3: // +CREG: <n>, <stat>, <lac>, <cid> //
            ALOGD("Getting lst response");
            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;
            ALOGD("Getting 2nd response");
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            ALOGD("Getting 3rd response");
            err = at_tok_nexthexint(&line, &response[1]);
            if (err < 0) goto error;
            ALOGD("Getting 4th response");
            err = at_tok_nextstr(&line, &p_cellIds[0].cid);
            if (err < 0) goto error;
            break;
        default:
            goto error;
        }
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, pp_cellIds, sizeof(pp_cellIds));
    return;
error:
    //free(p_cellIds);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    return;
}


void requestSetNetworkSelectionAutomatic(void * data, size_t datalen, RIL_Token t)
{
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

void requestSetLocationUpdates(void * data, size_t datalen, RIL_Token t)
{
/**
 * "data" is int *
 * ((int *)data)[0] is == 1 for updates enabled (+CREG=2)
 * ((int *)data)[0] is == 0 for updates disabled (+CREG=1)
 */
    int err;
   if( ((int *)data)[0] == 1 )
   	{
   	   err = at_send_command("AT+CREG=2",NULL);
   	}
   else if(((int *)data)[0] == 0)
   	{
   	   err = at_send_command("AT+CREG=1",NULL);
   	}
   	    else
   	    	{
   	    	  goto error;
   	    	}
   if (err < 0)
     goto error;
   
	 RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
	 return;
	 
	 error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    return; 	
}
