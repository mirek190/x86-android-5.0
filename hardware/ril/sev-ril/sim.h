
typedef enum
{
    SIM_ABSENT = 0,
    SIM_NOT_READY = 1,
    SIM_READY = 2, /* SIM_READY means the radio state is RADIO_STATE_SIM_READY */
    SIM_PIN = 3,
    SIM_PUK = 4,
    SIM_NETWORK_PERSONALIZATION = 5
} SIM_Status;

void  requestSIM_IO(void *data, size_t datalen, RIL_Token t);

void  requestQueryFacilityLock(void*  data, size_t  datalen, RIL_Token  t);

void  requestSetFacilityLock(void*  data, size_t  datalen, RIL_Token  t);

void  requestEnterSimPin(void*  data, size_t  datalen, RIL_Token  t);

void  requestEnterSimPuk(void*  data, size_t  datalen, RIL_Token  t);

void  requestChangeSimPin(void*  data, size_t  datalen, RIL_Token  t);

void requestChangePassword(void *data, size_t datalen, RIL_Token t,
                           const char *facility, int request);