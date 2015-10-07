/*   ----------------------------------------------------------------------
     Copyright (C) 2013-2014 Intel Mobile Communications GmbH
     
          Sec Class: Intel Confidential (IC)
     
     Copyright (C) 2010 Infineon Technologies Denmark A/S (IFWD)
     ----------------------------------------------------------------------
     Revision Information:
       $$File name:  /mhw_drv_inc/inc/target_test/at_router.h $
       $$CC-Version: .../oint_drv_target_interface/4 $
       $$Date:       2014-02-21    11:23:15 UTC $
     ----------------------------------------------------------------------
     by CCCT (v0.6)
     ---------------------------------------------------------------------- */
     
#ifndef _AT_ROUTER_H_
#define _AT_ROUTER_H_

#if (defined (WIN32) || defined (_Windows)) 
#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)
#else
#define DLL_EXPORT
#define DLL_IMPORT
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef COMPILER_SIZE_CHECK
#define COMPILER_SIZE_CHECK(cond,errormsg) extern char errormsg[(cond) ? 1 : -1]
#endif

#ifndef BUILD_64_BIT_LIBRARY
/* User has not set the 64 bit define, make sure that the application is indeed a 32 bit application.
   If a 64 bit application, user must add the compiler define BUILD_64_BIT_LIBRARY for 64 bit applications.
*/
COMPILER_SIZE_CHECK(sizeof(U32*) == sizeof(U32),enable_compiler_define_BUILD_64_BIT_LIBRARY_for_64_bit_application); /*lint !e506*/
#endif



/*! AT Router terminal handle
*/
#if defined (BUILD_64_BIT_LIBRARY)
typedef U64 T_AT_ROUTER_HDL;
#else
typedef U32 T_AT_ROUTER_HDL;
#endif

/*!
AT notification events.
*/
typedef enum
{
    AT_ROUTER_EVENT_CONNECTED,    /* An AT data-link was connected towards the indicated URL. */
    AT_ROUTER_EVENT_REROUTED,     /* Routing was modified as indicated in the URL. */
    AT_ROUTER_EVENT_DISCONNECT,   /* The AT data-link was disconnected and the route handle is no longer valid. */
    AT_ROUTER_EVENT_CMD,          /* An AT command was received. */
    AT_ROUTER_EVENT_RSP,          /* An AT response was received. */
    AT_ROUTER_EVENT_RSP_OK,       /* An AT "OK" response was received. */
    AT_ROUTER_EVENT_RSP_ERROR     /* An AT "ERROR" response was received. */
}T_AT_ROUTER_EVENT;


/*!
\brief Router AT notification callback function.
\param[in] ro_hdl    At router handle.
\param[in] event     Router event.
\param[in] p_str     URL, command or response string depending on event type.
\param[in] p_usr_val User value (can be modified per router handle by at_router_set_usrval() API).
\return Returns TRUE if the event is handled (otherwise the AT router will manage based on configuration e.g. forward to other AT handler).
*/
typedef BOOL (*T_RO_AT_NOTIF_CB)(T_AT_ROUTER_HDL ro_hdl, T_AT_ROUTER_EVENT event, const CHAR* p_str, void* p_usr_val);


/*!
\brief Adds a AT Router instance for the datalink.
\param[in] dl_hdl Datalink handle, can be IPICOM_DL_INVALID_HANDLE.
\param[in] at_url Url of at handler, can be NULL.
\param[in] gti_url Url of gti handler, can be NULL.
\return AT Router handle, 0 if failure.
*/
T_AT_ROUTER_HDL DLL_EXPORT at_router_add_datalink(T_IPICOM_DLH dl_hdl,const CHAR* at_url, const CHAR* gti_url);

/*!
\brief Closes the AT Router instance.
\param[in] ro_hdl At router handle.
\return Returns TRUE if succesful
*/
BOOL DLL_EXPORT at_router_remove(T_AT_ROUTER_HDL ro_hdl);

/*!
\brief Closes the AT Router instance.
\param[in] dl_hdl Datalink handle.
\return Returns TRUE if succesful
*/
BOOL DLL_EXPORT at_router_remove_by_dl(T_IPICOM_DLH dl_hdl);

/*!
\brief Set user value for the router instance.
\param[in] ro_hdl At router handle.
\param[in] p_usr_val User value.
\return Returns TRUE if successful
*/
BOOL DLL_EXPORT at_router_set_usrval(T_AT_ROUTER_HDL ro_hdl, void* p_usr_val);

/*!
\brief Registers an AT notification callback function.
\param[in] p_notif_cb At notification callback.
\return Returns TRUE if successful
*/
BOOL DLL_EXPORT at_router_set_at_handler_cb(T_RO_AT_NOTIF_CB p_notif_cb);

/*!
\brief Send an AT command towards AT provider.
\   This may be the command received in the at handler callback, just remember to return TRUE from that callback to tell that it is already handled.
\param[in] ro_hdl At router handle.
\param[in] p_cmd At command.
\return Returns TRUE if successful
*/
BOOL DLL_EXPORT at_router_send_command(T_AT_ROUTER_HDL ro_hdl, const CHAR* p_cmd);

/*!
\brief Send an AT command response towards AT consumer.
\param[in] ro_hdl At router handle.
\param[in] p_rsp At command response.
\return Returns TRUE if successful
*/
BOOL DLL_EXPORT at_router_send_response(T_AT_ROUTER_HDL ro_hdl, const CHAR* p_rsp);


#ifdef __cplusplus
}
#endif 


#endif //_AT_ROUTER_H_

