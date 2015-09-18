

enum MODEM_VENDOR
{
    UNKNOWN = 0,
	HUAWEI,
	ZTE,
	STRONGRISING
};


void onDataCallListChanged(void *param); 
void requestCdmaDataCallList(void *data, size_t datalen, RIL_Token t);
void requestDataCallList(void *data, size_t datalen, RIL_Token t);
void requestQueryCdmaNetworkSelectionMode(void *data, size_t datalen, RIL_Token t);
void requestQueryNetworkSelectionMode(void *data, size_t datalen, RIL_Token t);

void requestQueryAvailableNetworks(void *data, size_t datalen, RIL_Token t);

void requestSignalStrength(void *data, size_t datalen, RIL_Token t);

void requestCdmaRegistrationState(int request, void *data,size_t datalen, RIL_Token t);
void requestRegistrationState(int request, void *data,size_t datalen, RIL_Token t);

void requestOperator(void *data, size_t datalen, RIL_Token t);
void requestCDMAOperator(void *data, size_t datalen, RIL_Token t);
void requestGsmSetupDataCall(void *data, size_t datalen, RIL_Token t);
void requestCdmaSetupDataCall(void *data, size_t datalen, RIL_Token t);
void requestDeactivateDataCall(void *data, size_t datalen, RIL_Token t);

void requestGetPreferredNetworkType(void *data, size_t datalen, RIL_Token t);

void requestSetPreferredNetworkType(void *data, size_t datalen, RIL_Token t);

void requestSetNetworkSelectionManual(void *data, size_t datalen, RIL_Token t);

void requestNeighboringCellIds(void * data, size_t datalen, RIL_Token t);

void requestSetNetworkSelectionAutomatic(void * data, size_t datalen, RIL_Token t);



