/*  -------------------------------------------------------------------------
    Copyright (C) 2013-2014 Intel Mobile Communications GmbH
    
         Sec Class: Intel Confidential (IC)
    
     ----------------------------------------------------------------------
     Revision Information:
       $$File name:  /mhw_drv_inc/inc/target_test/ipicom.h $
       $$CC-Version: .../oint_drv_target_interface/10 $
       $$Date:       2014-02-21    11:23:08 UTC $
     ---------------------------------------------------------------------- */


#ifndef _IPICOM_H_
#define _IPICOM_H_
 
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


/*! \file ipicom.h
    \brief IPICOM v3 API for usage from applications.
*/

/*! \defgroup IPICOM_GENERAL General APIs
     Documentation for general APIs related to version number, tracing etc.
*/

/*! \defgroup IPICOM_SOCKET Socket related APIs
     Documentation for sockets related APIs
*/

/*! \defgroup IPICOM_DATALINK Datalink related APIs
    Documentation for datalink related API
*/

/*! \defgroup IPICOM_SAPN SAPN related APIs
    Documentation for SAPN (Simple Adhoc Path based Network) related APIs
*/

/*! \addtogroup  IPICOM_GENERAL
*/

/* \{ */

/*! IPICOM error codes
*/
typedef enum
{
    IPICOM_SUCCESS = 0,
    IPICOM_FAILURE = -1,
    IPICOM_TIMEOUT = -2,
    IPICOM_INVALID_PARAM = -3,
    IPICOM_SEND_Q_FULL = -4,
    IPICOM_OUT_OF_MEMORY = -5,    
    IPICOM_RESPONSE_NOTIF = -6,  
    IPICOM_SOCKET_ERR_INVALID = -7,
    IPICOM_SOCKET_ERR_PORT_IN_USE = -8,
    IPICOM_SOCKET_INVALID_PARAM = -9,
    IPICOM_SOCKET_WOULD_BLOCK = -10,    /* indicates for a non-blocking socket that it would have blocked if the socket was blocking */
    IPICOM_SEND_BLOCK_LIMIT_REACHED = -11,        
    IPICOM_RECV_Q_EMPTY = -12,          /* indicates that receive queue is empty */
} T_IPICOM_ERROR;

/*! IPICOM Trace severity enum types
 */

typedef enum  {
  IPICOM_TRACE_SEVERITY_DEBUG=-1, /** Debug level. Allows debug, info, warning and error traces */
  IPICOM_TRACE_SEVERITY_INFO=0, /** information level. Default severity . Allows info, warning and error traces*/
  IPICOM_TRACE_SEVERITY_WARNING=1, /** warning level. Allows warning and error traces*/
  IPICOM_TRACE_SEVERITY_ERROR=2 /** error level. Allows only error traces*/
} T_IPICOM_TRACE_SEVERITY;

/*! IPICOM version information types*/
typedef enum
{
    IPICOM_VERSION_MAJOR, /** Major number */
    IPICOM_VERSION_MINOR, /** Minor Number */
    IPICOM_VERSION_REVISION /**Revision number */
}T_IPICOM_VERSION_INFO;

/*!
\brief Function for getting IPICOM version info.
\param[in] info_type    Type of version info to retrieve (see T_IPICOM_VERSION_INFO).
\param[out] p_value  Pointer to integer where version value will be written.
\return T_IPICOM_ERROR - Error code. 
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_get_version(T_IPICOM_VERSION_INFO info_type, U32 *p_value);

/*!
\brief Starts trace if not already started with default severity level of info and without any group filter.
\return void
*/
void DLL_EXPORT IPICOM_trace_start( void );

/*!
\brief Stops trace if it was earlier started using IPICOM_trace_start() api.
\return void
*/
void DLL_EXPORT IPICOM_trace_stop( void );

/**
\brief Sets severity level for tracing. Lower severity level allows all higher levels.
\param[in] severity Severity to be set.
\return T_IPICOM_ERROR type return code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_trace_set_severity_level(T_IPICOM_TRACE_SEVERITY severity);

/**
\brief Sets group filter for tracing so traces from only the groups in the filter are visible.
\param [in] groups String of ',' separated  group names. Following group names are available.
"nw_layer"
"tr_layer"
"ss_layer"
"sock_api"
"trace"
Additional group names registered by datalink type plugins can be obtained from IPICOM_trace_get_group_names().
\return T_IPICOM_ERROR type return code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_trace_set_group_filter(char *groups);

/**
\brief Function to get a string consisting of ',' separated available trace group names. If buffer length is not
enough then IPICOM_OUT_OF_MEMORY is returned and buffer is filled with group names which just fit.
\param[out] buffer Buffer into which group names will be written.
\param[in] buflen Length of buffer.
\return T_IPICOM_ERROR - Error code. 
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_trace_get_group_names(char * buffer, U32 buflen);

/* \} */


/*! \addtogroup  IPICOM_SOCKET
*/
/* \{ */

/*! IPICOM SAPN protocol port number
*/
typedef U8 T_IPICOM_SAPN_PORT;

#define IPICOM_SAPN_PORT_INVALID ((T_IPICOM_SAPN_PORT)0xff) /**Invalid sanp port number */

/** IPICOM SAPN Address */
typedef struct 
{
    U32 route_hdl; /**Route handle to remote node returned by IPICOM_sapn_mount or IPICOM_sapn_get_route*/
    T_IPICOM_SAPN_PORT  port; /**Port number on remote node */
} T_IPICOM_SAPN_ADDRESS;

/** IPICOM Hop Address*/
typedef struct
{
    U32 dl_hdl; /**Datalink handle*/
}T_IPICOM_HOP_ADDRESS;

/** IPICOM AT address*/
typedef struct
{
    U32 dl_hdl; /**Datalink handle*/
}T_IPICOM_AT_ADDRESS;

/** Socket address type union*/
typedef union
{
    T_IPICOM_SAPN_ADDRESS  sapn;
    T_IPICOM_HOP_ADDRESS hop;
    T_IPICOM_AT_ADDRESS at;
    //T_IPICOM_IP_ADDRESS; //FFS
} T_IPICOM_SOCK_ADDR_IN;


/** Socket domains (currently only IPICOM) */
typedef enum
{
    PF_IPICOM, /* IPICOM family */
    //PF_IP      /* IP based family. //FFS */
} T_IPICOM_SOCKET_DOMAIN;

/** Types of socket */
typedef enum
{    
    IPICOM_SOCKET_TYPE_SEQPACKET = 0,  /** Connection oriented send and recv of datagrams, with flow control */
    IPICOM_SOCKET_TYPE_DGRAM = 1,      /** Connection less send and recv datagrams, without flow control */
    IPICOM_SOCKET_TYPE_BYTESTREAM = 2, /** Connection oriented send and recv of bytestream, with flow control */

   /* Future (FFS)*/    
} T_IPICOM_SOCKET_TYPE;

/** Transport Protocol to be used.
*/
typedef enum
{
    IPICOM_DEFAULT_PROTO = 0, /** Default protocol for the given socket family and type. */
    IPICOM_SAPN_PROTO,  /** Path based protocol. Can be used with all types of sockets and is the default protocol. */
    IPICOM_HOP_PROTO,   /** Binary Hop protocol which works across a single datalink. Can be used with DGRAM type sockets.*/
    IPICOM_AT_PROTO,    /** Send and receive ascii strings over a single datalink. Can be used with DGRAM type sockets. */
} T_IPICOM_SOCKET_COMMUNICATION_PROTOCOL;


/*! Socket related flags
*/
typedef U32 T_IPICOM_SOCKET_SEND_FLAGS;
/* TODO: Define flag values */


/*! IPICOM socket handle
*/
typedef S32 T_IPICOM_SH;

/*! Invalid socket handle */
#define IPICOM_SOCKET_INVALID_HANDLE ((T_IPICOM_SH)-1)

/*! IPICOM user value (64 bits in 64 bit library).
*/
#ifdef BUILD_64_BIT_LIBRARY
typedef S64 T_IPICOM_USER_VALUE;
#else
typedef S32 T_IPICOM_USER_VALUE;
#endif


/*! Indications which are provided by IPICOM on the non-blocking socket callback function on the specified events.
*/
typedef enum
{   
    IPICOM_SOCKET_CONNECT_IND,      /** Connection established to listening socket. */
    IPICOM_SOCKET_CONNECT_CNF,      /** Connection req confirmed by server. */
    IPICOM_SOCKET_DISCONNECT_IND,   /** Connection to socket disconnected. The socket handle passed is invalid and cannot be used anymore. */
    IPICOM_SOCKET_CLOSE_IND,        /** Socket is closed. */
    IPICOM_SOCKET_UNBIND_IND,       /** Socket got unbound because the route or datalink was closed. */
    IPICOM_SOCKET_RECV_IND,         /** Data is ready in receive queue. */
    IPICOM_SOCKET_CONNECT_ERR       /** Connection req failed. */
} T_IPICOM_SOCKET_CBF_IND_TYPE;

/*! Reserved IPICOM port number enumerations */
typedef enum
{
   IPICOM_PORT_GTI_SERVER = 0,   /** Port number for listerning GTI server*/
} T_IPICOM_RESERVED_PORT_ID;

/*! Indications which are provided by IPICOM on the PLP callback function on the specified events.
*/
typedef enum
{   
    IPICOM_PLP_CONNECT_IND,      /** Connection established to socket. */
    IPICOM_PLP_DISCONNECT_IND,   /** Connection to socket disconnected. The handle passed is invalid and cannot be used anymore. */
    IPICOM_PLP_RECV_IND          /** Data is ready in receive queue. */
} T_IPICOM_PLP_CBF_IND_TYPE;

/*! Presentation Layer Protocol defines*/
typedef enum
{
   IPICOM_PLP_DEFAULT = 0,   /** Default presentation layer protocol on a socket*/
   PLP_DEFAULT = IPICOM_PLP_DEFAULT, /** Deprecated. Use IPICOM_PLP_DEFAULT instead */
   IPICOM_PLP_GTI = 1,       /** GTI protocol "gti" */
   PLP_GTI = IPICOM_PLP_GTI, /** Deprecated. Use IPICOM_PLP_GTI instead */
   /***/
   IPICOM_PLP_UNDEFINED = 0xff
} T_IPICOM_PLP_ID;

/*! Socket options which could be set.
*/
typedef enum
{   
    IPICOM_SOCK_OPT_TIMEOUT      /** Option to set socket timeout interval in ms. */
} T_IPICOM_SOCKET_OPT;

/*! Socket option level.
*/
typedef enum
{   
    IPICOM_SOCK_OPT_LVL_SESSION      /** Session level. */
} T_IPICOM_SOCKET_OPT_LEVEL;


/*!
\brief Socket owner callback prototype.
\param[in] shdl - Socket handle
\param[in] ind_type - socket indication type.
\param[in] user_value -  Arbitrary user value which was registered for this session. Useful for setting "context" if e.g. the same cb is used for multiple sockets/sessions.
\return void
*/
typedef void (*T_IPICOM_SOCKET_IND_CBF)(T_IPICOM_SH shdl, T_IPICOM_SOCKET_CBF_IND_TYPE ind_type, T_IPICOM_USER_VALUE user_value);

/*!
\brief Socket presentation layer callback prototype which is used to indicate events from IPICOM towards the PLP.
\param[in] shdl - Socket handle
\param[in] ind_type - Presentation layer socket indication type.
\param[in] user_value - Arbitrary user value which was registered for this PLP on the session. Useful for setting "context" if e.g. the same cb is used for multiple sockets/sessions.
\return None
*/
typedef void (*T_IPICOM_SOCKET_PLP_IND_CBF)(T_IPICOM_SH shdl, T_IPICOM_PLP_CBF_IND_TYPE ind_type, T_IPICOM_USER_VALUE user_value);

/*!
\brief Creates a socket. 
\param[in] domain - The protocol family domain (typically from T_IPICOM_SOCKET_DOMAIN). 
\param[in] type - Socket type (e.g. SOCK_STREAM or SOCK_DGRAM).
\param[in] protocol - Protocol to use (e.g. IPICOM_SAPN_PROTO from T_IPICOM_SOCKET_COMMUNICATION_PROTOCOL). Set to 0 to use default sapn protocol.
\return S32 - Returns socket handle or IPICOM_SOCKET_INVALID_HANDLE on error.
*/
S32 DLL_EXPORT IPICOM_socket(S32 domain, S32 type, S32 protocol);

/*!
\brief Set socket to options.
\param[in] shdl - Socket handle
\param[in] level - Socket option level, Default IPICOM_SOCK_OPT_LVL_SESSION.
\param[in] opt - Socket option to set.
\param[in] p_opt_val - Pointer to the socket option value.
\param[in] opt_len - Length of socket option value.
\return T_IPICOM_ERROR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_socket_set_opt(T_IPICOM_SH shdl,T_IPICOM_SOCKET_OPT_LEVEL level, T_IPICOM_SOCKET_OPT opt, const void* p_opt_val, U32 opt_len);


/*!
\brief Set socket to non-blocking and assign a callback.
\param[in] shdl - Socket handle
\param[in] cb_socket_ind - Callback for socket indications
\return T_IPICOM_ERROR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_socket_set_nonblocking(T_IPICOM_SH shdl, T_IPICOM_SOCKET_IND_CBF cb_socket_ind);

/*!
\brief Binds a socket to an address or path and port.
\param[in] shdl - Socket handle from an #IPICOM_socket() call.
\param[in] bind_addr - Structure defining the address and port to bind to. 
\param[in] addrlen - Size of the addr argument.
\return T_IPICOM_ERROR Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_socket_bind(T_IPICOM_SH shdl, T_IPICOM_SOCK_ADDR_IN *bind_addr, U32 addrlen);

/*!
\brief Closes and releases resources associated to a socket. 
\param[in] shdl - Socket handle
\return T_IPICOM_ERROR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_socket_close(T_IPICOM_SH shdl);

/*!
\brief Connects a socket to a server
\param[in] shdl - Handle for local socket
\param[in] addr - Socket address to connect to. 
\param[in] addrlen - Size of the address parameter.
\return T_IPICOM_ERRORR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_socket_connect(T_IPICOM_SH shdl, T_IPICOM_SOCK_ADDR_IN *addr, U32 addrlen);

/*!
\brief Prepares a socket for incoming connections.
\param[in] shdl - Handle for socket.
\param[in] max_pending - The maximum number of connections pending for accept.
\return T_IPICOM_ERRORR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_socket_listen(T_IPICOM_SH shdl, U8 max_pending);

/*!
\brief Accepts an incoming connection
\param[in] shdl - Handle for socket.
\param[out] cli_addr - Pointer to socket address which is filled with connected clients address.
\param[out] addrlen - Size of the client address structure. When returning addrlen indicates how many bytes of
structure was actually used.
\return S32 - Returns connection-handle or IPICOM_SOCKET_INVALID_HANDLE on error.
*/
S32 DLL_EXPORT IPICOM_socket_accept(T_IPICOM_SH shdl, T_IPICOM_SOCK_ADDR_IN *cli_addr, U32 *addrlen);

/*!
\brief Sends data to a socket or datalink opened in transparent mode. With socket handle the API can be blocking 
or non-blocking depending on socket has blocking option set or not. With datalink handle the API is always blocking.
\param[in] handle - Socket or Datalink handle.
\param[in] p_data - Pointer to data to send. 
\param[in] len - Length of data to send.
\return S32 - Returns sent length or errorcode (T_IPICOM_ERROR).
*/
S32 DLL_EXPORT IPICOM_send(S32 handle, void *p_data, U32 len);

/*!
\brief Receive data from socket or datalink opened in transparent mode. With socket handle the API can be blocking 
or non-blocking depending on socket has blocking option set or not. With datalink handle the API is always blocking.
\param[in] handle - Socket or datalink handle. CURRENTLY ONLY SOCKET-HANDLE IS SUPPORTED.
\param[in] p_data - Pointer where received data is copied.
\param[in] len - Set to length of data buffer.
\return S32 - Returns actual received length or errorcode (T_IPICOM_ERROR).
*/
S32 DLL_EXPORT IPICOM_recv(S32 handle, void *p_data, U32 len);

/*!
\brief Sends data to a socket given by its address.
\param[in] shdl - Socket handle
\param[in] p_data - Pointer to data to send
\param[in] len - Data length
\param[in] flags - (reserved for future) 
\param[in] p_addr - Pointer to address structure defining the destination peer.
\param[in] addrlen - Length of address 
\return S32 - Returns sent length or errorcode (T_IPICOM_ERROR).
*/
S32 DLL_EXPORT IPICOM_socket_sendto(T_IPICOM_SH shdl, void *p_data, U32 len, T_IPICOM_SOCKET_SEND_FLAGS flags, T_IPICOM_SOCK_ADDR_IN *p_addr, U32 *addrlen);

/*!
\brief Receive reference to message from socket. No copying is done. Instead p_data is set to point to the received data.
\param[in] shdl - Socket handle.
\param[in] p_data - Pointer to pointer for the received read-only data.
\return S32 - Returns actual receive length or errorcode (T_IPICOM_ERROR).
*/
S32 DLL_EXPORT IPICOM_socket_recv_ref(T_IPICOM_SH shdl, void ** const p_data);

/*!
\brief Get address:port information for a socket given by socket-handle. This is useful when a client-socket is connected without 
         binding first as connect() will assign a random free port if port is not set.
\param[in]  shdl - Socket handle.
\param[out] p_adr - Address of structure to write address and port info to.
\param[out] p_addr_len - Number of bytes written to address structure.
\return T_IPICOM_ERROR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_socket_getsockname(T_IPICOM_SH shdl, T_IPICOM_SOCK_ADDR_IN *p_adr, U32 *p_addr_len);


/*!
\brief Set user value associated to session (on socket and/or associated PLP sockets). This user value is given
on the indication callback function for the session.
\param[in] shdl - Handle for socket.
\param[in] user_value - Arbitrary user value. 
\return T_IPICOM_ERROR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_socket_set_usrval(T_IPICOM_SH shdl, T_IPICOM_USER_VALUE user_value);

/*!
\brief Add support for a presentation-layer protocol (PLP) to a socket. The PLP can then join a socket connection when it gets established.
\param[in] shdl - Socket handle
\param[in] plp_id - Presentation layer protocol ID.
\param[in] socket_ind_cbf - Optional indication callback. If NULL the default protocol-callback is used.
\return S32 - New socket-handle for PLP or errorcode (T_IPICOM_ERROR) on errors.
*/
S32 DLL_EXPORT IPICOM_socket_add_plp(T_IPICOM_SH shdl, T_IPICOM_PLP_ID plp_id, T_IPICOM_SOCKET_PLP_IND_CBF socket_ind_cbf);


/*********************************************/
/**** Presentation Layer related API ********/
/*********************************************/

/*!
\brief Register a Presentation Layer Protocol (PLP) ID to IPICOM.
\param[in] plp_id  Presentation layer protocol ID.
\param[in] protocol_ind_cbf Default callback function for indications.
\return T_IPICOM_ERROR - Error code
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_register_plp(T_IPICOM_PLP_ID plp_id, T_IPICOM_SOCKET_PLP_IND_CBF protocol_ind_cbf);
/* \} */


/*! \addtogroup  IPICOM_DATALINK
*/
/* \{ */

/*! IPICOM datalink handle
*/
typedef S32 T_IPICOM_DLH;

#define IPICOM_DL_INVALID_HANDLE ((T_IPICOM_DLH)-1) /** Invalid datalink handle */

//#define IPICOM_DL_INVALID_ID IPICOM_DL_INVALID_HANDLE /** Deprecated. Use IPICOM_DL_INVALID_HANDLE instead */

typedef enum
{   
    IPICOM_DATALINK_OPEN_IND,    /** A datalink is opened by one of the plugins which auto detects. */
    IPICOM_DATALINK_CLOSE_IND,   /** An open datalink got closed. */
    IPICOM_DATALINK_SAPN_MOUNT_IND,  /** Peer node on a datalink mounted it. */
    IPICOM_DATALINK_SAPN_UNMOUNT_IND /** Peer node on a datalink can no longer support SAPN protocol. This can happen for eg. due to peer node rebooting or crashing.*/
} T_IPICOM_DATALINK_CBF_IND_TYPE;

/** Enumeration of different datalink information types*/
typedef enum
{
    IPICOM_DATALINK_INFO_TYPENAME, /**gets the datalink type name. User should pass a pointer to character array with minimum size of bytes.*/
    IPICOM_DATALINK_INFO_PEER_ROUTE /**get route handle to datalink peer. User should pass a pointer to T_IPICOM_SAPN_RH type variable.*/

}T_IPICOM_DATALINK_INFO;

/*!
\brief Datalink callbacks prototype which is used to indicate events from IPICOM concerning datalinks.
\param[in] dl_hdl Datalink handle
\param[in] ind_type
*/
typedef void (*T_IPICOM_DATALINK_IND_CBF)(T_IPICOM_DLH dl_hdl ,T_IPICOM_DATALINK_CBF_IND_TYPE ind_type);

/*!
\brief Function to open a datalink.
\param[in] connection_name - String telling the datalink to open.
\param[in] connection_option - Options for the datalink.
\return S32 - Positive T_IPICOM_DLH type handle or in case of failure IPICOM_DL_INVALID_HANDLE.
*/
S32 DLL_EXPORT IPICOM_datalink_open(const char *connection_name, const char *connection_option);

/*!
\brief Function to open a datalink in transparent mode. In transparent mode IPICOM_recv() and IPICOM_send() apis
can be used to send and receive directly using the datalink handle.
\param[in] *connection_name - String telling the datalink to open.
\param[in] *connection_option - Options for the datalink.
\return S32 - Positive T_IPICOM_DLH type handle or in case of failure IPICOM_DL_INVALID_HANDLE.
*/
S32 DLL_EXPORT IPICOM_datalink_open_transparent(const char *connection_name, const char *connection_option);

/*!
\brief Function to set options for an open datalink.
\param[in] dl_hdl - Datalink handle.
\param[in] *connection_option - Options for the datalink.
\return T_IPICOM_ERROR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_datalink_options(T_IPICOM_DLH dl_hdl, const char * connection_option);

/*!
\brief Function to close an open datalink.
\param[in] dl_hdl- Datalink handle.
\return T_IPICOM_ERROR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_datalink_close(T_IPICOM_DLH dl_hdl);

/*!
\brief Function to register a callback which will be called by IPICOM to give indications concerning datalinks.
\param[in] cbf T_IPICOM_DATALINK_IND_CBF type function pointer.
\return T_IPICOM_ERROR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_datalink_set_notification(T_IPICOM_DATALINK_IND_CBF cbf);

/**
\brief Function to read information about a datalink.
\param[in] dl_hdl Datalink handle.
\param[in] info_type - Type of information requested.
\param[out] p_answer - Pointer to an appropriate data structure where the information will be written.
\param[in] ans_size - Size of answer buffer.
\return T_IPICOM_ERROR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_datalink_get_info(T_IPICOM_DLH dl_hdl, T_IPICOM_DATALINK_INFO info_type, void * p_answer, U16 ans_size);

/* \} */


/*! \addtogroup  IPICOM_SAPN
*/
/* \{ */

/*! IPICOM SAPN protocol Route handle
*/
typedef S32 T_IPICOM_SAPN_RH;

#define IPICOM_SAPN_ROUTE_INVALID_HANDLE ((T_IPICOM_SAPN_RH)-1) /**Invalid route handle */
#define IPICOM_SAPN_ROUTE_ANY       0xFFFF    /** used for binding server sockets to any route  */
#define IPICOM_SAPN_ROUTE_LOCALROUTE 0xc000   /** used for binding server sockets to local host */

/** Enumeration of different information types that can be read from a remote node */
typedef enum
{
    IPICOM_SAPN_ROUTE_INFO_REMOTE_NODENAME=0x1, /** Name of Remote node*/
    IPICOM_SAPN_ROUTE_INFO_REMOTE_SIGNAGES=0x2, /** Signages at remote node*/
    IPICOM_SAPN_ROUTE_INFO_REMOTE_PORTNAMES=0x4, /** Portnames at remote node*/
    IPICOM_SAPN_ROUTE_INFO_ALL = 0x7 /**Read all info types*/
}T_IPICOM_SAPN_ROUTE_INFO;

/*
\brief Function to set a node name in the current process. It can be called succesfully only once.
The added name is used to identify current node to remote nodes while establishing a route.
\param[in] *node_name - Name to be set for local node.
\return T_IPICOM_ERROR - Error code. IPICOM_SUCCESS if succesful.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_sapn_set_node_name(const char * node_name);

/** DEPRECIATED- kept for compatiblity, use IPICOM_sapn_set_node_name instead */
#if defined(__ANDROID__)
/** DEPRECIATED- kept for compatiblity, use IPICOM_sapn_set_node_name instead */
T_IPICOM_ERROR IPICOM_sapn_add_node(const char * node_name);
#else
/** DEPRECIATED- kept for compatiblity, use IPICOM_sapn_set_node_name instead */
#define IPICOM_sapn_add_node IPICOM_sapn_set_node_name;
#endif


/**
\brief Function to add a datalink to SAPN network.
\param[in] dl_hdl - Datalink handle.
\return S32 - Positive T_IPICOM_SAPN_RH type route handle or if datalink callabck function is registered then IPICOM_RESPONSE_NOTIF or in case of failure an T_IPICOM_ERROR type code is returned.
*/
S32 DLL_EXPORT IPICOM_sapn_mount(T_IPICOM_DLH dl_hdl);

/**
\brief Function to unmount a mounted datalink which is part of SAPN network. All routes using the datalink are teared down when the datalink is unmounted.
\param[in] dl_hdl - Datalink handle.
\return T_IPICOM_ERROR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_sapn_unmount(T_IPICOM_DLH dl_hdl);

/**
\brief Function to resolve path to a route handle leading to an IPICOM network node.
\param[in] path - String specifying path to remote node.
\return S32 - Positive T_IPICOM_SAPN_RH type handle. T_IPICOM_ERROR type error code in case of failure.
*/
S32 DLL_EXPORT IPICOM_sapn_get_route(const char *path);

/**
\brief Function to resolve a path and optinaly a port name to a route handle and port number. The port number will be 0xff in case no port name is present in the path.
\param[in] path - Path to be resolved. Each signage in the path should be '/' separated. And port name if any should be end of the path separated by a ':'
\param[out] p_sapn_addr - Pointer to sapn address structure where the resolved route handle and port number will be specified.
\param[in] timeout_ms - Timeout in milliseconds. Cannot be 0.
\return T_IPICOM_ERROR - error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_sapn_get_addr_info(const char * path, T_IPICOM_SAPN_ADDRESS *p_sapn_addr, U32 timeout_ms);

/**
\brief Function to add a signage at the local end of a route.
\param[in] route_hdl - Route handle.
\param[in] signage - Signange to be added.
\return T_IPICOM_ERROR - error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_sapn_add_signage(T_IPICOM_SAPN_RH route_hdl, const char* signage);

/**
\brief Function to read different kind of information from the remote node at the end of a route. Each information type will have a prefix followed by ',' separated elements.
And if more than one type of information is requested then each information type is separated by ';'. Information which fits in the buffer is only
read out.
\param[in] route_hdl Route handle.
\param[out] buffer - Buffer where information is written.
\param[in] buf_len - Length of buffer.
\param[in] flags - Bitmask of T_IPICOM_SAPN_ROUTE_INFO type flags.
\return T_IPICOM_ERROR - Error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_sapn_route_read_info(T_IPICOM_SAPN_RH route_hdl, char * buffer, U32 buf_len, U32 flags);


/**
\brief Function to reserve a port at local node.
\param[in] port_name - Name to be given to reserved port.
\return T_IPICOM_SAPN_PORT - Reserved port number. IPICOM_SAPN_PORT_INVALID in case of failure
*/
T_IPICOM_SAPN_PORT DLL_EXPORT IPICOM_sapn_port_reserve(const char * port_name);

/**
\brief Function to do port forwarding at a remote node towards a reserved port at local node specified by port_name.
At remote node a port is reserved with the same portname.
\param[in] local_port_name - Local port name to be used for reserving forwarded port.
\param[in] route_hdl - Route handle to remote node to which local port should be forwarded.
\param[in] remote_port_name - Remote port name to which local port should be forwarded. If it is null then local_port_name will be used as remote_port_name.
\return T_IPICOM_ERROR - error code.
*/
T_IPICOM_ERROR DLL_EXPORT IPICOM_sapn_port_forward(const char * local_port_name, T_IPICOM_SAPN_RH route_hdl, const char* remote_port_name);

#if 0
/* FFS */
S32 IPICOM_getaddrinfo(const char *node,     // e.g. "ap@wifi" or "ap/cp@main" 
                const char *service,  // e.g. "unittest" or port number
                const struct IPICOM_addrinfo *hints,
                struct addrinfo **res);
#endif

/* \} */

#ifdef __cplusplus
}
#endif 

#endif /*_IPICOM_H_*/







