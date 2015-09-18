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

#include "ril_common.h"
#include "sim.h"
#include "ril.h"

static int sim_pin_retry = 3;     
static int sim_puk_retry = 10;    
static struct timeval TIMEVAL_DELAYINIT = {0,0}; 
extern SIM_Status getSimStatus();

void  requestSIM_IO(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    RIL_SIM_IO_Response sr;
    int err;
    char *cmd = NULL;
    RIL_SIM_IO_v5 *p_args;
    char *line;

    memset(&sr, 0, sizeof(sr));

    p_args = (RIL_SIM_IO_v5 *)data;

    /* FIXME handle pin2 */

    if (p_args->data == NULL)
    {
        asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d",
                 p_args->command, p_args->fileid,
                 p_args->p1, p_args->p2, p_args->p3);
    }
    else
    {
        asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d,%s",
                 p_args->command, p_args->fileid,
                 p_args->p1, p_args->p2, p_args->p3, p_args->data);
    }

    err = at_send_command_singleline(cmd, "+CRSM:", &p_response);

    if (err < 0 || p_response->success == 0)
    {
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(sr.sw1));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(sr.sw2));
    if (err < 0) goto error;

    if (at_tok_hasmore(&line))
    {
        err = at_tok_nextstr(&line, &(sr.simResponse));
        if (err < 0) goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &sr, sizeof(sr));
    at_response_free(p_response);
    free(cmd);

    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    free(cmd);
    return;
}

void  requestQueryFacilityLock(void*  data, size_t  datalen, RIL_Token  t)
{
    ATResponse   *p_response = NULL;
    int           err;
    char*         cmd = NULL;
    const char**  strings = (const char**)data;
    SIM_Status    sim_sta;
    int  response;
    char *line;

    if (strcmp("SC", strings[0]) == 0)                  //SIM card (if this command is configured, the password must be input when powering on the MS)
    {
        sim_sta = getSIMStatus();
        if (sim_sta != SIM_READY)
        {

		RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
		
        }
    } else if (strcmp("FD", strings[0]) == 0)           //SIM card or active application in the UICC(GSM or USIM) fixed dialing memory feature (Reserved, not supported currently)
    {
        RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
        return;
    }
    
    asprintf(&cmd, "AT+CLCK=\"%s\",2", strings[0]);

    err = at_send_command_singleline(cmd, "+CLCK:", &p_response);

    if (err < 0 || p_response->success == 0)
    {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
	  goto error;
   }
    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) 
    	{
           RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
           goto error;
	}
		
    err = at_tok_nextint(&line, &response);
    if (err < 0) 
    	{
           RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
           goto error;

	}
    ALOGD("test query sim status response is %d\n",response);
  
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int *)); 

error:
	
	at_response_free(p_response);
    if(cmd != NULL)
        free(cmd);
    return;
}

void  requestSetFacilityLock(void*  data, size_t  datalen, RIL_Token  t)
{
    ATResponse   *p_response = NULL;
    ATResponse   *c_response = NULL;
    int           err;
    int count = 0;
    int rc=0;
    int response[4];
    char*   cmd = NULL;
    char* line=NULL;
    char discardstr[15];
    const char**  strings = (const char**)data;
    char pin = 0;
    char *nettypeline;

    if (strcmp("SC", strings[0]) == 0)
    {
        pin = 1;
    }

    asprintf(&cmd, "AT+CLCK=\"%s\",%s,\"%s\"", strings[0], strings[1], strings[2]);
    err = at_send_command(cmd, &p_response);

    if(err < 0 || p_response->success == 0)
    {
        ALOGD("wrong Sim Pin, err=%d",err);
        err = at_send_command_singleline("AT^CPIN?","^CPIN:",&c_response);
        if (err < 0 || c_response->success == 0){
            goto error;
        }
        line = c_response->p_intermediates->line;
        ALOGD("c_response p_intermediates line %s",line);
        err = at_tok_start(&line);

        ALOGD("after at_tok_start, err=%d",err);
        ALOGD("c_response p_intermediates line after at_tok_start is %s",line);
        if (err < 0) {
            goto error;
        }
/*       
        eg:  
        <CR><LF>%CPIN:<code>,[<times>],<puk_times>,<pin_times>,<puk2_times>,<pin2_times><CR><LF><CR><LF>OK<CR><LF>
*/
        at_tok_nextstr(&line,&discardstr);    //skip 1st str
        at_tok_nextstr(&line,&discardstr);    //skip 2nd str
        ALOGD("after at_tok_nextstr in for loop line %s",line);
        for (count = 0; count < 4; count++) {
            err = at_tok_nextint(&line, &(response[count]));
            ALOGD("after at_tok_nextint in for loop line %s",line);
            ALOGD("response[%d] = %d\n",count,response[count]);
            ALOGD("requestEnterSimPin's err is %d",err);
            if (err < 0) {
                goto error;
            }
        }
       
 error:
         RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, &response[1], sizeof(int *));
    }
    else
    {
        if (pin == 1)
        {
            response[0]=3;
            RIL_onRequestComplete(t, RIL_E_SUCCESS, &response[0], sizeof(int *));
        }
    }

    if(cmd != NULL)
        free(cmd);    
    at_response_free(c_response);
    at_response_free(p_response);
    return;
}

void  requestEnterSimPin(void*  data, size_t  datalen, RIL_Token  t)
{
	
    ATResponse   *p_response = NULL;
	ATResponse   *c_response = NULL;
    int           err;
	char discardstr[15];    
	int response[4];
	char *line = NULL;
	int count = 0;
    char*         cmd = NULL;
    const char**  strings = (const char**)data;;

    if ( datalen == 2*sizeof(char*) ) {
        asprintf(&cmd, "AT+CPIN=\"%s\"", strings[0]);
    } else if ( datalen == 3*sizeof(char*) ) {
        asprintf(&cmd, "AT+CPIN=\"%s\",\"%s\"", strings[0], strings[1]);
    } else
        goto error;

    err = at_send_command(cmd, &p_response);
    free(cmd);
    
    ALOGD("requestEnterSimPin, err=%d,p_response->success=%d",err,p_response->success);
    if (err!=0 || p_response->success == 0) {   
        ALOGD("wrong Sim Pin, err=%d",err);
        err = at_send_command_singleline("AT^CPIN?","^CPIN:",&c_response);
        if (err < 0 || c_response->success == 0){
            goto error;
        }
        line = c_response->p_intermediates->line;
        ALOGD("c_response p_intermediates line %s",line);
        err = at_tok_start(&line);

        ALOGD("after at_tok_start, err=%d",err);
        ALOGD("c_response p_intermediates line after at_tok_start is %s",line);
        if (err < 0) {
            goto error;
        }
/*       
        eg:  
        <CR><LF>%CPIN:<code>,[<times>],<puk_times>,<pin_times>,<puk2_times>,<pin2_times><CR><LF><CR><LF>OK<CR><LF>
*/
        at_tok_nextstr(&line,&discardstr);    //skip 1st str
        at_tok_nextstr(&line,&discardstr);    //skip 2nd str 
        ALOGD("after at_tok_nextstr in for loop line %s",line);
        for (count = 0; count < 4; count++) {
            err = at_tok_nextint(&line, &(response[count]));
            ALOGD("after at_tok_nextint in for loop line %s",line);
            ALOGD("response[%d] = %d\n",count,response[count]);
            ALOGD("requestEnterSimPin's err is %d",err);
            if (err < 0) {
                goto error;
            }
        }

error:
         RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, &response[1], sizeof(int *));
    } else {
    	    response[0]=3;    
    	    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response[0], sizeof(int *));
    	    
        	TIMEVAL_DELAYINIT.tv_sec = 5;
            RIL_requestTimedCallback(initializeCallback, NULL, &TIMEVAL_DELAYINIT);    	    
    }
    if(cmd != NULL)
        free(cmd);
    at_response_free(c_response);    
    at_response_free(p_response);
    
    return;
}

void  requestEnterSimPuk(void*  data, size_t  datalen, RIL_Token  t)
{
    ATResponse  *p_response = NULL;
    ATResponse  *c_response = NULL;    
    int     err;    
    int     rc;
    int     count;
    int response[4];    
    char*   cmd = NULL;
    char *line = NULL;
    const char**  strings = (const char**)data;
    char discardstr[15];
    char *nettypeline,*nettypestr;

    asprintf(&cmd, "AT+CPIN=\"%s\",\"%s\"", strings[0], strings[1]);
    err = at_send_command(cmd, &p_response);
    if (err < 0 || p_response->success == 0)
        goto error;

    ALOGD("test Enter puk else to set the retry count is 10\n");
    response[1]=10;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response[1], sizeof(int *));

    TIMEVAL_DELAYINIT.tv_sec = 5;
    RIL_requestTimedCallback(initializeCallback, NULL, &TIMEVAL_DELAYINIT);
    
    at_response_free(p_response);
    if(cmd != NULL)
        free(cmd);    
    return;

error:
    err = at_send_command_singleline("AT^CPIN?","^CPIN:",&c_response);
    if (err < 0 || c_response->success == 0){
         goto error_out;
    }
    line = c_response->p_intermediates->line;
    ALOGD("c_response p_intermediates line %s",line);
    err = at_tok_start(&line);

    ALOGD("after at_tok_start, err=%d",err);
    ALOGD("c_response p_intermediates line after at_tok_start is %s",line);
    if (err < 0) {
        goto error_out;
    }
/*       
        eg:  
        <CR><LF>%CPIN:<code>,[<times>],<puk_times>,<pin_times>,<puk2_times>,<pin2_times><CR><LF><CR><LF>OK<CR><LF>
*/
    at_tok_nextstr(&line,&discardstr);    //skip 1st str
    at_tok_nextstr(&line,&discardstr);    //skip 2nd str    

    ALOGD("after at_tok_nextstr in for loop line %s",line);
    for (count = 0; count < 4; count++) {
        err = at_tok_nextint(&line, &(response[count]));
        ALOGD("after at_tok_nextint in for loop line %s",line);
        ALOGD("response[%d] = %d\n",count,response[count]);
        ALOGD("requestEnterSimPin's err is %d",err);
        if (err < 0) {
            goto error_out;
        }
    }

error_out:     
    at_response_free(p_response);
    at_response_free(c_response);     
    RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, &response[0], sizeof(int *));
    return;
}

void requestChangePassword(void *data, size_t datalen, RIL_Token t,
                           const char *facility, int request)
{
    int err = 0;
    int num_retries = -1;
    int count;
    int response[4];
    char *line = NULL;
    char *oldPassword = NULL;
    char *newPassword = NULL;
    char *cmd  = NULL;
    char discardstr[15];
    ATResponse *p_response = NULL;
    ATResponse *c_response = NULL;
    
    if (datalen != 3 * sizeof(char *) || strlen(facility) != 2)
        goto error;

    oldPassword = ((char **) data)[0];
    newPassword = ((char **) data)[1];
    asprintf(&cmd, "AT+CPWD=\"%s\",\"%s\",\"%s\"", facility,
                oldPassword, newPassword);
    err = at_send_command(cmd, &p_response);
    if(cmd != NULL)
        free(cmd);
        
    ALOGD("%s,err:%d, success:%d",__FUNCTION__,err,p_response->success);
    if (err != 0 || p_response->success == 0)
        goto error;

    num_retries = 3;
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &num_retries, sizeof(int *));

    return;

error:
    err = at_send_command_singleline("AT^CPIN?","^CPIN:",&c_response);
    if (err < 0 || c_response->success == 0){
         goto error_out;
    }
    line = c_response->p_intermediates->line;
    ALOGD("c_response p_intermediates line %s",line);
    err = at_tok_start(&line);

    ALOGD("after at_tok_start, err=%d",err);
    ALOGD("c_response p_intermediates line after at_tok_start is %s",line);
    if (err < 0) {
        goto error_out;
    }
/*       
        eg:  
        <CR><LF>%CPIN:<code>,[<times>],<puk_times>,<pin_times>,<puk2_times>,<pin2_times><CR><LF><CR><LF>OK<CR><LF>
*/
    at_tok_nextstr(&line,&discardstr);    //skip 1st str
    at_tok_nextstr(&line,&discardstr);    //skip 2nd str    

    ALOGD("after at_tok_nextstr in for loop line %s",line);
    for (count = 0; count < 4; count++) {
        err = at_tok_nextint(&line, &(response[count]));
        ALOGD("after at_tok_nextint in for loop line %s",line);
        ALOGD("response[%d] = %d\n",count,response[count]);
        ALOGD("requestEnterSimPin's err is %d",err);
        if (err < 0) {
            goto error_out;
        }
    }

error_out:     
    at_response_free(c_response); 
    at_response_free(p_response);    
    RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, &response[1], sizeof(int *));
    return;
}
