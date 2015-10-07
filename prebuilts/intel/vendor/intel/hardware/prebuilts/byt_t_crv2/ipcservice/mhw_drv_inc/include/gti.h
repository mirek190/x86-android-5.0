/*********************************************************************************
*       Copyright (C) 2011-2014 Intel Mobile Communications GmbH
*       
*            Sec Class: Intel Confidential (IC)
*       
*
* This document contains proprietary information belonging to IMC.
* Design A/S. Passing on and copying of this document, use and communication
* of its contents is not permitted without prior written authorisation.
*
* Description:
*   Declaration of GTI interfaces which shall be enabled from boot up.
*   Note: Only declare interfaces as shown, -no additional stuff can be put in this file.
*
* Revision Information:
*  $File name:  /mhw_drv_inc/inc/target_test/gti.h $
*  $CC-Version: .../oint_drv_target_interface/93 $
*  $Date:       2014-02-21    11:23:13 UTC $
*   Comment:
*     Changed mapping, new NVM callback commands
*********************************************************************************/
#ifndef GTI_H
#define GTI_H

#if defined(__cplusplus)
    extern "C" {
#endif

#ifdef __cplusplus
#define IMP_CPP "C"
#else
#define IMP_CPP
#endif

#if defined (WIN32) || defined (_Windows)
#define  EXPORT_WIN __declspec(dllexport)
#else
#define EXPORT_WIN
#endif


#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <bastypes.h>

#if !defined (TARGET) 

 /* #define PROCEDURE_VARS byte dummy; */

#if defined (REGI)
#include <scttypes.h>
#else
  struct process_vars
 {
    S32 trial;
 };
#endif

#else
#if defined(UMTS) || defined(GPRS)
#include <scttypes.h>
#else
struct process_vars
{
    S32 trial;
};
#endif

#endif

/*! definition of 8 bit integer type compilations */
#ifndef int8
#if defined (BOARD_QUASAR) && defined (LINT)
typedef signed   char int8; /* !e652 */ 
#else
#define int8 char
#endif
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

/*! GTI unsigned value, architecture specific 32 bits or 64 bits.
*/
#ifdef BUILD_64_BIT_LIBRARY
typedef U64 T_GTI_UINT;
#else
typedef U32 T_GTI_UINT;
#endif



#define GTI_CONST const /* OBSOLETE: Please dont use */

/** 
@defgroup ISPEC_DRV_GTI Interface Specification for Generic Test Interface (GTI) module	
Description of the GTI interface to program test interfaces for the GTI environment, 
utilizing GTI test/debug facilities and interfacing to GTI Sequencers.

For more information about the GTI see:

- TARGET TEST ASPEC:  <c>/mhw_drv_src/target_test/doc/ASPEC-TargetTest.pdf </c>\n
- GTI FSPEC: <c>/mhw_drv_src/target_test/gti/doc/FSpec_DRV-GTI.doc </c>\n
- GTI DSPEC: <c>/mhw_drv_src/target_test/gti/doc/DSpec_Drv-GTI.doc </c>\n
*/


/**
@addtogroup ISPEC_DRV_GTI_TUNING GTI Tuning values
Defines which scales different aspects of GTI. They are listed here for information. They should only be used by the GTI module owner.
@ingroup ISPEC_DRV_GTI 
*/
/**@{  */ /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/


#ifndef GTI_LIGHT

#ifndef GTI_NOF_TST_INTFACES_POOL
/*! Max number of possible test-interfaces to be active at a time. NOTE default interface occupies one place. */
#define GTI_NOF_TST_INTFACES_POOL 128  
#endif

#ifndef NOF_ATIF_CH_PUBLIC
/*! Number of GTI channels i.e. AT@ channels on a terminal*/
#define NOF_ATIF_CH_PUBLIC 50
#endif

#else
/* Max number of possible test-interfaces to be active at a time (reduced in GTI light version). NOTE default interface occupies one place. */
#define GTI_NOF_TST_INTFACES_POOL 4
#define NOF_ATIF_CH_PUBLIC 4  
#endif

#ifndef GTI_WATER_STREAM_MARK
/*! Configuration: Number of bytes before end of response buffer GTI will start streaming response i.e. write what is in the response buffer and clear it for further writing by the parser */
#define GTI_WATER_STREAM_MARK    150    /* In GTI_FL_STREAM mode, when this number of bytes is left in outbuf, it is flushed */
#endif

#ifndef GTI_MAX_FUNCS_ARGSBYTES
/*! Configuration: Number of bytes allocated for GTI function parameters */
  #if defined(DRV_TARGET_TEST_MCL_BRINGUP) && !defined(DRV_TARGET_TEST_MCL_QUARK)
#define GTI_MAX_FUNCS_ARGSBYTES   (6*508)
  #else
#define GTI_MAX_FUNCS_ARGSBYTES   (6*512+32) 
  #endif /*if-else DRV_TARGET_TEST_MCL_BRINGUP*/
#endif

#ifndef GTI_MAX_INSTRING_LEN_BYTESIZE   /*If a size has been given as a sysdef on compiletime - use this value directly - else use 1500 as default value*/
/*! Configuration: Number of bytes reserved for GTI command parameters lists */
#define GTI_MAX_INSTRING_LEN_BYTESIZE 1500
#endif

/*! Max length of test interface name (keep short for practical reasons) */
#define GTI_MAX_IFNAME_LEN         11  

#ifndef MAX_SPRINTF_ARRAY_SIZE
/*! Max length of sprintf(), sprintf_va() array argument */
#define MAX_SPRINTF_ARRAY_SIZE 0x7FFF
#endif

#ifndef GTI_MIN_RESP_BUF_LEN
/*! Minimum response buffer when GTI allocates e.g. for threaded commands. */
#define GTI_MIN_RESP_BUF_LEN 400
#endif

#ifndef GTI_DEF_RESP_BUF_LEN
/*! Default response buffer when GTI allocates e.g. for threaded commands.  */
#define GTI_DEF_RESP_BUF_LEN 1500
#endif

#ifndef GTI_DEFAULT_THREAD_STACK_SIZE
/*! Defaults stack size allocated for dynamically launched GTI Threads*/
#define GTI_DEFAULT_THREAD_STACK_SIZE (10*1024)
#endif

#ifndef GTI_MAX_NUM_ARRAY_DIMENSIONS
/*! Maximum number of array dimensions on exposed variables */
#define GTI_MAX_NUM_ARRAY_DIMENSIONS 3
#endif

#ifndef GTI_NOF_OS_EVENT_GROUPS_POOL
/*! Number of OS events allocated for GTI's pool offered to users with the GTI abstraction. */
#define GTI_NOF_OS_EVENT_GROUPS_POOL 8
#endif

#ifndef GTI_SHARED_HISR_STACK_SIZE
/*! Stack size for GTI Shared HISR */
#define GTI_SHARED_HISR_STACK_SIZE (3*1024)
#endif

#ifndef GTI_NVM_ADD_SUB_ARRAY_IDS
/*! When GTI generates ID's for NVM parameters individual ID's are made to access multidimensional arrays as sub-arrays. Alternatively the arrays are represented as flat arrays (one dimension). Define value (1) to enable or value (0) to disable. */
#define GTI_NVM_ADD_SUB_ARRAY_IDS 1
#endif

#ifndef GTI_NVM_ADD_EXTRA_FLAT_ARRAY_IDS
/*! (Only relevant when GTI_NVM_ADD_SUB_ARRAY_IDS=TRUE): When GTI generates ID's for NVM parameters extra ID's are made to access multidimensional arrays as 
    flat array (one dimensional). The extra ID name will be postfixed with the array dimensions. Note: Does not affect one dimensional arrays. Define value (1) to enable or value (0) to disable. */
#define GTI_NVM_ADD_EXTRA_FLAT_ARRAY_IDS 1
#endif


/**@}*/ /* End of ISPEC_DRV_GTI_TUNING group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */


/**
@addtogroup ISPEC_DRV_GTI_BASIC_DEFINITIONS GTI Basic definitions
@ingroup ISPEC_DRV_GTI 
*/
/**@{  */ /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/*! Definition of handle used within GTI to route responses and keep track of settings for this GTI command flow... */
typedef U32 T_GTI_TRANSACTION_H;
typedef T_GTI_TRANSACTION_H gti_atif_handle_type; /* Depreciated: use T_GTI_TRANSACTION_H*/


/*! GTI error code ID's  */
typedef enum
{
    GTI_SUCCESS,                        /* Everything OK */
    
    GTI_RSP_DELAYED = -1,               /* Handler will provide the response later*/
        GTI_ERR_SEQ_ALLREADY_INITIALISED = -2, /* Sequencer allrady initialized */
        GTI_ERR_ILLEGAL_SEQ_ID = -3,        /* An invalid sequencer ID given */
        GTI_ERR_NO_MEMORY = -4,             /* no memory available to complete operation */
        GTI_ERR_HISR_CREATE_FAIL = -5,      /* A HISR could not be created */
        GTI_ERR_UNSPECIFIED = -6,           /* not specified error */
        GTI_ERR_OUT_OF_BOUNDS = -7,         /* out of range of array */
        GTI_ERR_OUT_OF_BITS = -8,           /* to few bits available in ID */
        GTI_ERR_OUT_OF_BUFFER = -9,         /* buffer given is too small */
        GTI_ERR_INVALID_ID = -10,           /* ID is invalid */
        GTI_ERR_INVALID_VIRTUALVAR_POINTER = -11, /* call back functio not defined when called */
        GTI_ERR_DYN_VAR_INVALID = -12,      /* Dynamic variable not valid */
        GTI_ERR_DYN_VAR_EXISTS = -13        /* Dynamic variable allready exists */    
}T_GTI_ERR_CODE;

typedef T_GTI_ERR_CODE gti_err_code_type; /* Depreciated: Use T_GTI_ERR_CODE */


/*! GTI return code ID's */
typedef enum
{
    GTI_RETVAL_CONTINUE_AHEAD = -5, /*Continue searching for duplicate definition of the command*/
    GTI_RETVAL_DELAYED_NOWAIT = -4,   /* Delayed return. GTI will not wait for response */
    GTI_RETVAL_DELAYED = -3,   /* Depreciated: Use GTI_EXEC_DELAYED */
    GTI_UNSOLICITED_RSP = -2,         /* Unsolicited response after command is finished */
    GTI_INTERMEDIATE_RSP = -1,         /* Intermediate response before command is finished*/
    GTI_NORETVAL = GTI_INTERMEDIATE_RSP,         /* No return value. Default when user does not set result. Ex- Intermediate responses */
    GTI_RETVAL_OK = 0,         /* Depreciated: Use GTI_EXEC_SUCCESS*/
    GTI_RETVAL_OK_NOPRINT,     /* OK without any "OK" printing */
    GTI_RETVAL_UNKNOWNCOMMAND, /* Parsing error. To be only used internaly by GTI */
    GTI_RETVAL_PARAMFAIL,      /* Input parameter error */
    GTI_RETVAL_EXECUTIONFAIL,   /* Depreciated: Use GTI_EXEC_FAIL */

    // GTI V2. Return values
    GTI_EXEC_DELAYED = GTI_RETVAL_DELAYED, /* Delayed return. GTI will wait for response. Same as returning NULL from command handler*/
    GTI_EXEC_SUCCESS = GTI_RETVAL_OK,         /* Everything OK */
    GTI_EXEC_FAIL   = GTI_RETVAL_EXECUTIONFAIL/* Execution error */
} T_GTI_EXEC_STATUS;

typedef T_GTI_EXEC_STATUS gti_retval_type; /* Depreciated: Use T_GTI_EXEC_STATUS*/


/*! Standard "OK" string which may e.g. be used for response to GTI test functions. Obsolete use GTI_SET_RETVAL*/
extern CHAR gti_ok_str[];
/*! Standard "ERROR" string which may e.g. be used for response to GTI test functions. Obsolete use GTI_SET_RETVAL*/
extern CHAR gti_err_str[];

/*! Callback function for output from GTI to calling interface (normally 'terminal') */
typedef void (*gti_write_at_func_ptr_type)(gti_atif_handle_type atif_handle, const CHAR *response_buf, S32 len);


/**@}*/ /* End of ISPEC_DRV_GTI_TUNING group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */


/**
@addtogroup ISPEC_DRV_GTI_CHANNEL_RESULTBOX GTI channels & Result Box Interface
Interface to work with channels and results box's. The gti_atif_handle_type is given to interface functions and 
contains the information needed to send back responses on the right channel and with right properties. In some special cases
a response needs to be send even if there is no ATIF handle available. For this there are macros to construct one depending on purpose.
@ingroup ISPEC_DRV_GTI 
*/
/**@{  */ /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/* Flag on ATIF handle indicating the result shall go to the resultbox of the GTI channel (not to be used directly by modules, -mainly for internal GTI use) */
#define GTI_ATIF_H_RESULT_BOX_FLAG 0x0100   /*do not change! and do not use directly */
/* Flag on ATIF handle indicating the that the current context is 'interrupt' (not to be used directly by modules, -mainly for internal GTI use) */
#define GTI_ATIF_H_ISR_CTXT_FLAG   0x0200   /*do not change! and do not use directly */

/*! Get a constructed atif handle for use in GTI result write function like gti_write_at() when writing from interrupt context. The given input atif_handle should be a valid handle obtained from a GTI function. */
#define GTI_ATIF_ISR_HANDLE(atif_handle) ((atif_handle) | GTI_ATIF_H_ISR_CTXT_FLAG)  

/*! Get a constructed atif handle for use in GTI result write function like gti_write_at() to write to resultbox*/
#define GTI_ATIF_RB_HANDLE(atif_handle) ((atif_handle) | GTI_ATIF_H_RESULT_BOX_FLAG)

/*! atif handle to start a sequencer without having an actual atif_handle. Output written to this atif_handle, directly
or from seqeucner will be ignored.*/ 
#define GTI_DEFAULT_ATIF_HANDLE (NOF_ATIF_CH_PUBLIC+GTI_NUM_SEQ)/* default ATIF handle, to use writing responses without a given "real" atif handle */

/* Obsolete definition, -only here for backwards compatibility - (DONT USE) */
#define GTI_DB_ATIF_HANDLE  GTI_DEFAULT_ATIF_HANDLE   /* DON'T USE!! OBSOLETE!!!  */


/** @} */ /* End of ISPEC_DRV_GTI_CHANNEL_RESULTBOX group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */




/************************************************************/
// 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0  
//                                         x  x  x    datatype (fixed position)
//                                      x             virtual variable indicator
//                                   x                pointer flag
//                                x                   bitfield indicator
//                             x                      sign indicator (fixed position)
//  x  x  x  x  x  x                                  virtual variable ID (ATT optional not used yet)
//  x  x  x  x  x  x  x  x (x)(x)(x)                  struct width (overlapping orthogonal types)
/************************************************************/
/*
These flags are used to define the data type of variables to GTI
*/
/*! Bit flag indicating 8 bit datatype */
#define GTDTF_W8      (1<<0)
/*! Bit flag indicating 16 bit datatype */
#define GTDTF_W16     (1<<1)
/*! Bit flag indicating 32 bit datatype */
#define GTDTF_W32     (1<<2)
/*! Bit mask indicating structure datatype */
#define GTDTF_STRUCT  (0x03)  /* special value */
/*! Bit flag indicating there is a function behind the variable */
#define GTDTF_VIRTUAL_VAR    (1<<3)
/*! Bit flag indicating that structure is represented as a compound. Mutually exclusive with virtual var */
#define GTDTF_COMPOUND    (1<<3)

/*! Bit flag indicating pointer datatype */
#define GTDTF_POINTER (1<<4)
/*! Bit flag indicating bitfield datatype*/
#define GTDTF_BITFIELD   (1<<5) /* only valid with w8, w16 and w32 */
/*! Bit mask indicating string datatype */
#define GTDTF_STRING  (0x05)/*Special Value*/
/*! Bit flag indicating signed datatype */
#define GTDTF_SIGNED  (1<<6)


/**
@addtogroup ISPEC_DRV_GTI_VARIABLES GTI Variable Definitions
Interface for mapping variable/structure datatypes to GTI Interface. Basically create an array of structures of type gti_variable_table_type
and make reference to it from the binding structure of type gti_definition_type.
@ingroup ISPEC_DRV_GTI 
*/
/**@{  */ /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/*! Enumeration of supported GTI data types. More combinations can be constructed, -but there is no guarantee they will work. */
typedef enum
{
    /*! no data type */
    GTDT_CLEAR         = 0,
    /*!  unsigned 8 bit integer data type */
    GTDT_UINT8         = GTDTF_W8,
    /*!  unsigned 16 bit integer data type */
    GTDT_UINT16        = GTDTF_W16,
    /*!  unsigned 32 bit integer data type */
    GTDT_UINT32        = GTDTF_W32,
    /*!  structure data type */
    GTDT_STRUCT        = GTDTF_STRUCT,      
    /*!  signed int 8 bit integer data type */
    GTDT_SINT8         = GTDTF_W8  | GTDTF_SIGNED,
    /*!  signed int 16 bit integer data type */
    GTDT_SINT16        = GTDTF_W16 | GTDTF_SIGNED,
    /*! signed int 32 bit integer  data type */
    GTDT_SINT32        = GTDTF_W32 | GTDTF_SIGNED,
    /*! string data type (array_size > 0)*/
    GTDT_STRING        = GTDTF_STRING,
    /*---------*/
    /*! Union type same same as struct in GTI, but required to distinguish for tools */
    GTDT_UNION         = GTDTF_STRUCT,

    /*! Width 8 Bitfield type */
    GTDT_W8_BITFIELD = GTDTF_W8 | GTDTF_BITFIELD,
    /*! Width 16 Bitfield type */
    GTDT_W16_BITFIELD = GTDTF_W16 | GTDTF_BITFIELD,
    /*! Width 32 Bitfield type */
    GTDT_W32_BITFIELD = GTDTF_W32 | GTDTF_BITFIELD,

    _GTDT_ALIGN16BIT = 0xffff
}gti_datatypes;

/*! Type definition of GTI data type specifier. This is constructed by bit-fields, -but fixed types are defined by gti_datatypes. */ 
typedef U16 gti_datatype_type;


/*! Macro to declare a signed integer providing the variable as argument (setting 'datatype' in gti_variable_table_type).*/
#define GTDT_SVAR(var)  ((gti_datatypes)((sizeof(var)&0x07) | GTDTF_SIGNED))
/*! Macro to declare an unsigned integer providing the variable as argument (setting 'datatype' in gti_variable_table_type).*/
#define GTDT_UVAR(var)  ((gti_datatypes)((sizeof(var)&0x07)))

/*! Macro to declare a signed integer providing the sizeof the variable (setting 'datatype' in gti_variable_table_type).*/
#define GTDT_SINT(size) ((gti_datatypes)(((size)&0x07) | GTDTF_SIGNED))
/*! Macro to declare an unsigned integer providing the sizeof the variable (setting 'datatype' in gti_variable_table_type).*/
#define GTDT_UINT(size) ((gti_datatypes)((size)&0x07))

/*! Macro defining the start position of struct size in the datatype bitmask in gti_variable_table_type*/
#define GTDT_BITMASK_STRUCT_SIZE_START_POS 5

/*! Macro defining the start position of, start bit of a bitfield member, in the datatype bitmask in gti_variable_table_type*/
#define GTDT_BITMASK_STARTBIT_START_POS (5+1)

/*! Macro defining the start position of, number of bits of a bitfield member, in the datatype bitmask in gti_variable_table_type*/
#define GTDT_BITMASK_NUMBITS_START_POS (5+5+1)



/*! Macro to declare a GTI array of structures (setting 'datatype' in gti_variable_table_type).*/
#define GTDT_STRUCT_ARRAY(ssize)  (GTDTF_STRUCT + ((ssize)<<GTDT_BITMASK_STRUCT_SIZE_START_POS)) /* using 11 upper bits i.e. effective range is (0..2047) */

/*! Macro to declare a structure as a compound (setting 'datatype' in gti_variable_table_type).*/
#define GTDT_COMPOUND(ssize)  ((GTDTF_STRUCT|GTDTF_COMPOUND) + ((ssize)<<GTDT_BITMASK_STRUCT_SIZE_START_POS)) /* using 11 upper bits i.e. effective range is (0..2047) */

/*! Macro to declare a bitfield member of a 8bit bitfield(setting 'datatype' in gti_variable_table_type).*/
#define GTDT_W8_BITFIELD_MEMBER(start_bit,num_bits)  (GTDT_W8_BITFIELD| (num_bits<<GTDT_BITMASK_NUMBITS_START_POS) | (start_bit<<GTDT_BITMASK_STARTBIT_START_POS))

/*! Macro to declare a bitfield member of a 16bit bitfield(setting 'datatype' in gti_variable_table_type).*/
#define GTDT_W16_BITFIELD_MEMBER(start_bit,num_bits) (GTDT_W16_BITFIELD| (num_bits<<GTDT_BITMASK_NUMBITS_START_POS)| (start_bit<<GTDT_BITMASK_STARTBIT_START_POS))

/*! Macro to declare a bitfield member of a 32bit bitfield(setting 'datatype' in gti_variable_table_type).*/
#define GTDT_W32_BITFIELD_MEMBER(start_bit,num_bits) (GTDT_W32_BITFIELD| (num_bits<<GTDT_BITMASK_NUMBITS_START_POS)| (start_bit<<GTDT_BITMASK_STARTBIT_START_POS))


#ifdef GTI_PC
/*! Variable data pointer used in gti_variable_table_type */
typedef void* gti_var_data_ptr_type;


/*! GTI Variable table element. Structure describes properties of variable to GTI parser */
struct gti_variable_table_s
{
    /*! name of variable/array/structure on GTI interface */
    const CHAR                  *variable_name;
    /*! pointer to actual variable/array/structure-definition */
    gti_var_data_ptr_type  data_p;
    /*! data type specification */
    gti_datatype_type      datatype;
    /*! Size of array. For single variables use value GTAS_SINGLE (obsoleted is GTDT_SINGLE)*/
    U16         array_size;
};

/*! GTI Variable table element, placed in ROM area. Structure describes properties of variable to GTI parser */
typedef struct gti_variable_table_s gti_variable_table_type;

#else
/*! Variable data pointer used in gti_variable_table_type */
typedef const void* gti_var_data_ptr_type;


/*! GTI Variable table element. Structure describes properties of variable to GTI parser */
struct gti_variable_table_s
{
    /*! name of variable/array/structure on GTI interface */
    const CHAR                  *variable_name;
    /*! pointer to actual variable/array/structure-definition */
    gti_var_data_ptr_type  data_p;
    /*! data type specification */
    gti_datatype_type      datatype;
    /*! Size of array. For single variables use value GTAS_SINGLE (obsoleted is GTDT_SINGLE)*/
    U16         array_size;
};

/*! GTI Variable table element, placed in ROM area. Structure describes properties of variable to GTI parser */
typedef const struct gti_variable_table_s gti_variable_table_type;
#endif


/*! Constant value to use for variables (non-array) as 'array_size' in gti_variable_table_type. */
#define GTAS_SINGLE 0

/*! OBSOLETE, please use equivalent GTAS_SINGLE. Constant value to use for variables (non-array) as 'array_size' in gti_variable_table_type. */
#define GTDT_SINGLE 0

/* ###############################
    GTI v2 OPTIMISED VAR START
   ############################### */

/*! Defines number of group-mask entries in group-mask table */
#define GTI_VAR_GROUP_MASK_CNT 8


/*! GTI v2 var entry type enumeration */
typedef enum
{
    GTI_ENTRY_NONE = 0,     /*< End marker of the table */
    GTI_ENTRY_GROUP,        /*< Group / structure type */
    GTI_ENTRY_COMPOUND,     /*< Compound type */
    GTI_ENTRY_INTEGER,      /*< Integer type */
    GTI_ENTRY_STRING,       /*< String type */
    GTI_ENTRY_FLOAT,        /*< Floating point type */
    GTI_ENTRY_FIXINTEGER,   /*< Fixed point integer type */
    GTI_ENTRY_FIXED_BITFIELD,/*< Fixed point bitfield type */
    GTI_ENTRY_BITFIELD,     /*< Bitfield type */
    GTI_ENTRY_TABLE_HEADER, /*< Table Header type */
} T_GTI_ENTRY_TYPE;

/*! GTI v2 access type enumeration */
typedef enum
{
    GTI_ACCESS_FULL = 0,    /*< Full, unrestricted access */
    GTI_ACCESS_RO,          /*< ReadOnly access */
    GTI_ACCESS_WO,          /*< WriteOnly access */
    GTI_ACCESS_HIDDEN       /*< Hidden variable */
} T_GTI_ACCESS_TYPE;

/*! GTI v2 addressing type enumeration */
typedef enum
{
    GTI_ADDRESS_ACTUAL = 0, /*< Actual addressing */
    GTI_ADDRESS_POINTER,    /*< Pointer adressing */
    GTI_ADDRESS_VFP,        /*< Virtual Function Pointer addressing */
    GTI_ADDRESS_OFFSET      /*< Offset from basepointer addressing */
} T_GTI_ADDRESS_TYPE;


/*! GTI v2 Variable info - Common for all types and is used to determine interpretation */
typedef struct GTI_VARIABLE_INFO_COMMON
{
    U16 is_end_marker:1;/*lint !e46*/ /*< End marker indicating the last entry for the table. */
    U16 access_type:2;/*lint !e46*/ /*< T_GTI_ACCESS_TYPE , ro,wo etc */
    U16 is_array_index:1;/*lint !e46*/ /*< Is an array - use array-spec table for specs */
    U16 address_type:2;/*lint !e46*/ /*< T_GTI_ADDRESS_TYPE , paged, pointer, vfp etc */
    U16 group_index:3;/*lint !e46*/ /*< Index into group mask table for actual group-access mask */
    U16 security_level:3;/*lint !e46*/ /*< Security level of type gti_sec_level_type */
    U16 entry_type:4;/*lint !e46*/ /*< T_GTI_ENTRY_TYPE, entry type of variable */
    U16 name_offset;        /*< Offset into name pool where variable name is stored - Support about 2500 names assuming avg name length of 28 */
} T_GTI_VARIABLE_INFO_COMMON;

/*! GTI var table info structure */
typedef struct
{
    CHAR *p_names_pool;                         /*< Pool of space allocated for variable names */
    U32  *p_array_spec;                         /*< Array-specification table */
    U8   group_masks[GTI_VAR_GROUP_MASK_CNT];   /*< Group mask table */

    /*! The members below only exist for dynamically allocated tables */
    U16 names_pool_size;        /* The allocated size of the names pool */
    U16 names_pool_high_mark;   /*< High-marker offset for the name pool. */
    U16 array_spec_size;        /* The allocated size of the array spec */
    U16 array_spec_high_mark;   /*< High-marker offset for array-spec pool */
}T_GTI_VAR_TABLE_INFO;


/*! GTI v2 integer Variable */
typedef struct
{
    T_GTI_VARIABLE_INFO_COMMON info; /*< Variable information */

    U16 reserved:10;/*lint !e46*/ /*< RESERVED */
    U16 is_signed:1;/*lint !e46*/ /*< Set if signed integer */
    U16 base_size:5;/*lint !e46*/ /*< Base size of variable. Number of bytes in base type. E.g. 1=8bit, 2=16bit, 4=32bit etc. */

    U16 array_index_size;   /*< Index into array spec table if set. 0 for non-arrays. */

    void *p_address;        /*< Variable address pointer */
} T_GTI_VAR_INT_TYPE;

/*! GTI v2 string Variable */
typedef struct
{
    T_GTI_VARIABLE_INFO_COMMON info; /*< Variable information */

    U16 reserved:10;/*lint !e46*/ /*< RESERVED */
    U16 is_signed:1;/*lint !e46*/ /*< Allways 1 */
    U16 base_size:5;/*lint !e46*/ /*< Always 1 (U8) */

    U16 array_index_size;   /*< Index into array spec table if set. 0 for non-arrays. */

    CHAR *p_address;        /*< Variable address pointer */
} T_GTI_VAR_STRING_TYPE;

/*! GTI v2 floating point Variable */
typedef struct
{
    T_GTI_VARIABLE_INFO_COMMON info; /*< Variable information */

    U16 reserved:10;/*lint !e46*/ /*< RESERVED */
    U16 is_signed:1;/*lint !e46*/ /*< Will be always true as float is, by nature, signed */
    U16 base_size:5;/*lint !e46*/ /*< Base size can be 4 or 8 for "long" & "long long" respectively */

    U16 array_index_size;   /*< Index into array spec table if set. 0 for non-arrays. */

    void *p_address;        /*< Variable address pointer */
} T_GTI_VAR_FLOAT_TYPE;

/*! GTI v2 fixed point integer Variable */
typedef struct
{
    T_GTI_VARIABLE_INFO_COMMON info; /*< Variable information */

    U16 comma_position:8;/*lint !e46*/ /*< Position of the fraction comma from MSB */
    U16 reserved:2;/*lint !e46*/ /*< RESERVED */
    U16 is_signed:1;/*lint !e46*/ /*< Set if signed */
    U16 base_size:5;/*lint !e46*/ /*< Base size of variable. Number of bytes in base type. E.g. 1=8bit, 2=16bit, 4=32bit etc. */

    U16 array_index_size;   /*< Index into array spec table if set. 0 for non-arrays. */

    void *p_address;        /*< Variable address pointer */
} T_GTI_VAR_FIXINTEGER_TYPE;

/*! GTI v2 fixed point bitfield Variable */
typedef struct
{
    T_GTI_VARIABLE_INFO_COMMON info; /*< Variable information */

    U16 comma_position:8;/*lint !e46*/   /*< Position of the fraction comma from MSB */
    U16 reserved:2;/*lint !e46*/         /*< RESERVED */
    U16 is_signed:1;/*lint !e46*/        /*< Set if signed */
    U16 base_size:5;/*lint !e46*/        /*< Base size of variable. Number of bytes in base type. E.g. 1=8bit, 2=16bit, 4=32bit etc. */

    U16 start_bit:8;/*lint !e46*/        /*< Starting bit in base-var */
    U16 num_bits:8;/*lint !e46*/         /*< Number of bits used */

    void *p_address;        /*< Variable address pointer */
} T_GTI_VAR_FIXBITFIELD_TYPE;

/*! GTI v2 bitfield Variable */
typedef struct
{
    T_GTI_VARIABLE_INFO_COMMON info; /*< Variable information */

    U16 reserved:10;/*lint !e46*/ /*< RESERVED */
    U16 is_signed:1;/*lint !e46*/ /*< Set if signed */
    U16 base_size:5;/*lint !e46*/ /*< Base size of variable. Number of bytes in base type. E.g. 1=8bit, 2=16bit, 4=32bit etc. */

    U16 start_bit:8;/*lint !e46*/ /*< Starting bit in base-var */
    U16 num_bits:8;/*lint !e46*/ /*< Number of bits used */

    void *p_address;        /*< Variable address pointer */
} T_GTI_VAR_BITFIELD_TYPE;

/*! GTI v2 group/structure Variable */
typedef struct
{
    T_GTI_VARIABLE_INFO_COMMON info; /*< Variable information */

    U16 group_size;         /*< Size of the group/structure */
    U16 array_index_size;   /*< Index into array spec table if set. 0 for non-arrays. */

    void *p_address;        /*< Table address pointer for the sub-structure */
} T_GTI_VAR_GROUP_TYPE;

/*! GTI v2 compound Variable */
typedef struct
{
    T_GTI_VARIABLE_INFO_COMMON info; /*< Variable information */

    U16 compound_size;      /*< Compound size */
    U16 array_index_size;   /*< Index into array spec table if set. 0 for non-arrays. */

    void *p_address;        /*< Table address pointer for the sub-structure */
} T_GTI_VAR_COMPOUND_TYPE;

/*! GTI v2 Table Header Element */
typedef struct
{
    T_GTI_VARIABLE_INFO_COMMON info; /*< Variable information */

    S32 dyn_ctrl_idx;        /*< Index for the dynamic control (Mounted variables) */

    T_GTI_VAR_TABLE_INFO *p_table_info;  /*< Pointer to GTI var table info structure */

} T_GTI_TABLE_HEADER_TYPE;



/*! GTI generic datatype used to init tables as the current compiler dont support designated initializers */
typedef struct
{
    T_GTI_VARIABLE_INFO_COMMON info;

    U16 w1;
    U16 w2;

    void *p_address;
} T_GTI_VAR_GENERIC_TYPE;

/*! Variable table union - common for all types */
typedef union
{
    T_GTI_VAR_GENERIC_TYPE gti_generic; /*< Should always be the first entry as long as the compiler dont support designated initializers */
    T_GTI_VAR_INT_TYPE gti_int;
    T_GTI_VAR_STRING_TYPE gti_string;
    T_GTI_VAR_FLOAT_TYPE gti_float;
    T_GTI_VAR_FIXINTEGER_TYPE gti_fixint;
    T_GTI_VAR_FIXBITFIELD_TYPE gti_fixbitfield;
    T_GTI_VAR_BITFIELD_TYPE gti_bitfield;
    T_GTI_VAR_GROUP_TYPE gti_group;
    T_GTI_VAR_COMPOUND_TYPE gti_compound;
    T_GTI_TABLE_HEADER_TYPE gti_table_header;
} T_GTI_VAR_TABLE_ELEMENT;


/*! GTI Optimized var table main structure */
typedef struct
{
    T_GTI_VAR_TABLE_ELEMENT *p_table_elements;   /*< Table containing T_GTI_VAR_TABLE_ELEMENT elements */
    void *p_mirror_address;                      /*< Mirror Address for mapped variable space, mandatory when Offset addressing scheme is used */
} T_GTI_OPTIMIZED_VAR_TABLE;


/*! GTI Optimized var - datatype and adressing structure - used when parsing variable description */
typedef struct
{
    U8 access:2;/*lint !e46*/
    U8 num_dimensions:6;/*lint !e46*/

    U16 entry_type:4;/*lint !e46*/
    U16 is_signed:1;/*lint !e46*/
    U16 security_level:3;/*lint !e46*/
    U16 base_size:5;/*lint !e46*/
    U16 addressing:2;/*lint !e46*/

    U8 start_bit;
    U8 num_bits;
    U8 comma_position;
    U8 group_mask;

    U32 struct_size;
    U32 array_flat_size;
    U32 array_dimension_multipliers[GTI_MAX_NUM_ARRAY_DIMENSIONS];
    
} T_GTI_V2_VAR_INFO;

/* ###############################
    GTI v2 OPTIMISED VAR END
   ############################### */



/*!
\brief Generates a formatting string for a specified GTI datatype, which may be an array.
\param datatype         GTI datatype e.g. GTDT_UINT16.
\param fmt_str_buf_p    Pointer to destination buffer for formatting string.
\param array_size       Size of array or GTAS_SINGLE (=0) for single variables.
\param num_base         10 or 16 (decimal and hex respectively)
\return pointer to null terminator at end of printed string in destination buffer .
*/
CHAR * gti_datatype_to_format_string(gti_datatypes datatype, CHAR *fmt_str_buf_p, S32 array_size, S32 num_base);

/*!
\brief Generates a formatting string for a specified GTI datatype, which may be an array.
\param p_variable       Pointer to GTI variable.
\param p_fmt_str_buf    Pointer to destination buffer for formatting string.
\param array_size       Size of array or GTAS_SINGLE (=0) for single variables.
\param num_base         10 or 16 (decimal and hex respectively)
\return pointer to null terminator at end of printed string in destination buffer .
*/
CHAR * gti_v2_datatype_to_format_string(T_GTI_VAR_TABLE_ELEMENT *p_variable, CHAR *p_fmt_str_buf, S32 array_size, S32 num_base);

#if 0
/*!
\brief Generates the C base type string string for a specified GTI datatype e.g. U8 or S32.
\param datatype         GTI datatype e.g. GTDT_UINT16.
\param base_type_str_p  Pointer to destination buffer for base type string.
\return Pointer to output string, at the terminating zero appended.
*/
CHAR *gti_datatype_to_c_basetype_string(gti_datatypes datatype, CHAR *base_type_str_p);
#endif

/*!
\brief Generates the C base type string string for a specified GTI variable e.g. U8 or S32.
\param p_variable       Pointer to GTI variable.
\param p_base_type_str  Pointer to destination buffer for base type string.
\return Pointer to output string, at the terminating zero appended.
*/
CHAR *gti_v2_datatype_to_c_basetype_string(T_GTI_VAR_TABLE_ELEMENT *p_variable, CHAR *p_base_type_str);


/*!
\brief Generates an array size in brackets or an empty string if it is not an array.
\param array_size       Size of array or GTAS_SINGLE (=0) for a single variable.
\param array_str_p      Pointer to destination buffer.
\return Pointer to output string, at the terminating zero appended.
*/
CHAR *gti_make_array_size_string(S32 array_size, CHAR *array_str_p);

/*! GTI variable reference structure. Includes pointer to GTI variable and number of bytes to translate the pointer to data due to array indexing */
typedef struct
{
    T_GTI_VAR_TABLE_ELEMENT *p_variable; /* !Pointer to GTI variable */   
    S32 translation;                   /* !Number of bytes to translate data pointer due to array indexing */
    U16 array_offset;               /*Offset into total array-size one-dimensiona mapping*/
    U16 sub_array_size;         /*Sizeof final/last dimension in multidemensional arrays*/
    U16 rel_id;                 /*the referenced id-no relative to the grp where placed*/
}gti_variable_ref_type;


/*!
\brief Copies data between GTI variable/array and some 'outside' data array or variable. Handles conversions between different variable typs (8, 16,32 bit)
\param write            TRUE means write from outside to variable. FALSE means read from variable to outside.
\param variable_ref_p   Pointer to variable refence
\param outside_data_p   Pointer to 'outside' data
\param outside_datatype GTI datatype of 'outside' data
\param count            number of elements to copy. If null (0) the maximum possible will be read/written i.e. maximum til end of array.
\param offset           first element to copy from/to in GTI variable array (0 in case of single variable)
\return                 GTI error code. May fail if exceeding array boundary.
*/
typedef gti_err_code_type (*gti_virtual_var_rd_wr_func_ptr_type)(gti_atif_handle_type atif_handle, BOOL write, gti_variable_ref_type *variable_ref_p, void *outside_data_p, gti_datatypes outside_datatype, U32 count, U32 offset);

/*!
\brief Function to be called when data for delayed virtual variable read is available.The data pointer holding the read data must be the same as supplied to the virtual variable handler.
\param read_data_p      pointer to data read. Must be same as supplied earlier to virtual variable handler. This pointer is used to find out which delayed read is being referred to.
\param err_code         final err_code for the read request
\return                 success/failure
*/
BOOL gti_virt_var_rd_delayed (void * read_data_p, gti_err_code_type err_code);



/** @} */ /* End of ISPEC_DRV_GTI_VARIABLES group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */



/** 
@addtogroup ISPEC_DRV_GTI_FUNCS GTI Function Definitions
Interface to map C-functions to GTI Interface. Basically create an array of structures of type gti_func_definition_type
and make reference to it from the binding structure of type gti_definition_type.
@ingroup ISPEC_DRV_GTI 
*/
/**@{  */ /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/*! Enumeration of function info strings in gti_func_definition_type */
typedef enum
{
    GTI_FI_PARM_FORMAT, /** Parameter formatting string */
    GTI_FI_PARM_TYPES,  /** Parameter type specification */
    GTI_FI_HELP,        /** Help text for function */

    /** Below marker to specify max number of info strings for functions */
    GTI_FI_NOF_INFO_MARKER
}gti_func_info_type;

/*! Used in gti_func_definition_type. It signifies number of strings for descriping function (to both GTI parser and user). 
First string is the formatting string of the function argument list. 
Next string describes the names of input arguments and possibly associates with an enumeration. 
Third string is an informative information which the user can query, -shall briefly describe the function. */    
#define GTI_FI_NOF_INFO GTI_FI_NOF_INFO_MARKER

#ifdef GTI_PC
typedef CHAR *(*gti_if_func_type)(gti_atif_handle_type atif_handle, CHAR *resp_buf, void *args);
/*! Structure for defining a function element of a GTI interface. Used in connection to gti_func_table_type. */
typedef struct
{
    /*! Pointer to C-function GTI shall call */
    CHAR *(*func)(gti_atif_handle_type atif_handle, CHAR *resp_buf, void *args);

   /*! Array of strings for descriping function to both GTI parser and user. First string is the formatting string of the function argument list. Next string describes the names of input arguments and possibly associates with an enumeration. Third string is an informative information which the user can query, -shall briefly describe the function. */    
   CHAR *func_info[GTI_FI_NOF_INFO];
}gti_func_definition_type; 




/*! GTI Function table element. Structure maps function name to GTI function definition */
struct gti_func_table_type_s
{
    /*! Name of function on GTI interface */
    const CHAR *p_name; 

    /*! Pointer to definition of function */
    gti_func_definition_type *p_func_definition;
};

/*! GTI Function table element placed in ROM area. Structure maps function name to GTI function definition */
typedef struct gti_func_table_type_s gti_func_table_type;

#else
typedef CHAR *(*gti_if_func_type)(gti_atif_handle_type atif_handle, CHAR *resp_buf, void *args);


/*! Structure for defining a function element of a GTI interface. Used in connection to gti_func_table_type. */
typedef const struct
{
    /*! Pointer to C-function GTI shall call */
    CHAR *(*func)(gti_atif_handle_type atif_handle, CHAR *resp_buf, void *args);

   /*! Array of strings for descriping function to both GTI parser and user. First string is the formatting string of the function argument list. Next string describes the names of input arguments and possibly associates with an enumeration. Third string is an informative information which the user can query, -shall briefly describe the function. */    
   const CHAR *func_info[GTI_FI_NOF_INFO];
}gti_func_definition_type; 




/*! GTI Function table element. Structure maps function name to GTI function definition */
struct gti_func_table_type_s
{
    /*! Name of function on GTI interface */
    const CHAR *p_name; 

    /*! Pointer to definition of function */
    gti_func_definition_type *p_func_definition;
};

/*! GTI Function table element placed in ROM area. Structure maps function name to GTI function definition */
typedef const struct gti_func_table_type_s gti_func_table_type;
#endif

/*! Typedef for v2.0 function command context info structure */
typedef struct
{
    T_GTI_TRANSACTION_H transaction_handle;
} T_GTI_CTXT_INFO;

/*!
    Typedefs for v2.0 function-type
*/    
typedef T_GTI_EXEC_STATUS (*T_GTI_V2_FUNC_CB)(T_GTI_CTXT_INFO *p_command_info, void *p_output_parms, void *p_input_parms);

/*! GTI Function table element. Structure maps function name and GTI function definition */
typedef struct
{
    /*! Name of function on GTI interface */
    const CHAR *p_name; 

    /*! Pointer to C-function GTI should call when function is called. Return value should reflect success or failure to process. */
    T_GTI_V2_FUNC_CB p_func;

    /*! Array of strings for descriping function to both GTI parser and user. 
        First string is the formatting string of the function argument list and return values. E.g. "%u %hd > %d %d[10]". 
        Next string describes the names of input arguments and possibly associates with an enumeration. E.g. "$gain_level gain, channel > num_results, result".
        Third string is an informative information which the user can query - shall briefly describe the function. 
    */
    const CHAR *p_func_info[GTI_FI_NOF_INFO];

    /*! Size of function input variable-structure */
    U16 input_var_size;
    /*! Size of function output varible-structure */
    U16 output_var_size;
}T_GTI_V2_FUNC_DEFINITION;


/*! macro which may optionally be applied as GTI function argument format and argument-description when the function takes no arguements. */
#define GTI_NO_FUNC_ARGS ""

/*! macro which may optionally be applied to GTI function argument-format for readability */
#define GTI_AFMT(str) str
/*! macro which may optionally be applied to GTI function argument-description for readability */
#define GTI_ARGS(str) str
/*! macro which may optionally be applied to GTI function help-string for readability*/
#define GTI_HELP(str) str

/*! macro to set the return value of type gti_retval_type*/
#define GTI_SET_RETVAL(retval, args) /*lint -e571*/ ((U32*)(((CHAR*)args)-sizeof(U32*)-sizeof(U32)))[0] = (U32)(retval)

/*! macro to get the return value of type gti_retval_type from func_args*/
#define GTI_GET_RETVAL(args) /*lint -e571*/ ((gti_retval_type)(((U32*)(((CHAR*)args)-sizeof(U32*)-sizeof(U32)))[0]))

#ifdef GTI_PC
/*!
\brief  Function to write back onsolicited responses to a GTI function call (from which the atif_handle is normally aquired).
\param atif_handle  Combined value containing information about the current command flow e.g. channel and flags. This is normally aquired from a previous GTI test function call.
\param string       String to be written.
\param len          Length of string excluding terminating zero (0) or alternatively -1 if function shall measure the length
\return none
*/
extern IMP_CPP EXPORT_WIN void  gti_write_at(gti_atif_handle_type atif_handle, const CHAR *string, S32 len);

#else
/*!
\brief  Function to write back onsolicited responses to a GTI function call (from which the atif_handle is normally aquired).
\param atif_handle  Combined value containing information about the current command flow e.g. channel and flags. This is normally aquired from a previous GTI test function call.
\param string       String to be written.
\param len          Length of string excluding terminating zero (0) or alternatively -1 if function shall measure the length
\return none
*/
void  gti_write_at(gti_atif_handle_type atif_handle, const CHAR *string, S32 len);
#endif

/*! Depreciated:- only for V1 functions
\brief  Function to write back a delayed GTI function responses (from which function the atif_handle is normally aquired). See module specifiction about delayed function responses.
\param atif_handle  Combined value containing information about the current command flow e.g. channel and flags. This is normally aquired from a previous GTI test function call.
\param response     Response string to be written. Note "string" must always be from RAM area, -not flash!
\param len          Length of response excluding terminating zero termination or alternatively -1 if function shall measure the length (zero terminated string)
\param retval       GTI return value. See more in GTI_retval_type.
\return none
*/
void  gti_write_delayed_func_result(gti_atif_handle_type atif_handle, const CHAR *response, S32 len, gti_retval_type retval);

/*! To be used by V2 functions
\brief  Function to write back a delayed GTI function responses. See module specifiction about delayed function responses.
\param p_ctxt       Command context information, which is passed to the function.
\param exec_status  Execution status of the delayed response, TRUE if success.
\return none
*/
void gti_return_delayed_response(T_GTI_CTXT_INFO *p_ctxt, BOOL exec_status);


/*!
\brief Function to copy data from variable associated with variable varName
\param var_name  Name of variable from which data needs to be read.
\param dest_buf  Pointer to which data needs to be copied.
\param dest_size Size of dest_buf in bytes.
\return Number of bytes copied. Returns -1 if variable was not found.
*/
S16 gti_var_read(const CHAR *var_name, void *dest_buf, U16 dest_size);

/*!
\brief Log error to GTI errorlist. The error can be queried from the AT-terminal interface using "at@get_error()".
\param p_ctxt The context structure for the function.
\param err_category Error code for GTI. Used to determine which error occured.
\param format_string Formatting string for error description. Formatting is like normal sprintf/scanf c-functions.
\param ... Variable parameter list (depends on format_str) - E.g. gti_log_error(p_ctxt,GTI_RETVAL_EXECUTIONFAIL,"Error at pos %u in %s", line_pos, error_desc_str) where line_pos & error_desc_str are defined by format_string.
\return Returns GTI_EXEC_SUCCESS on succesful logging.
*/
T_GTI_EXEC_STATUS gti_log_error(T_GTI_CTXT_INFO *p_ctxt, T_GTI_EXEC_STATUS err_category, const CHAR *format_string, ...);


/** @} */ /* End of ISPEC_DRV_GTI_FUNCS group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */



/**
@addtogroup ISPEC_DRV_GTI_ENUMS GTI Constants/Enums Interface
Interface to create enumerations/constants for use on the GTI interface. Basically create an array of structures of type gti_sint_const_table_type
and make reference to it from the binding structure of type gti_definition_type.
@ingroup ISPEC_DRV_GTI 
*/
/**@{  */ /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/*! Structure for defining an enumeration element of a GTI interface. This is an array of constants of this type. First array element is special as it defines the name of the enumeration of following constants and the value indicates the number of constants of the enumerations. */
typedef struct
{
    /*! name of enumeration. For first constant in array it is the name of the enumeration. */
    CHAR        *name;
    /*! value of constant. For first constant in array it is the number of following constants of the enumeration. */
    S32 value;
}T_GTI_ENUMERATION_ELEMENT;

/*! Depreciated: compatability definition*/
typedef const T_GTI_ENUMERATION_ELEMENT gti_sint_const_table_type;
/*! Obsolete: compatability definition*/
typedef gti_sint_const_table_type gti_const_table_type;

/*! Definition of GTI Enumeration type. */
typedef struct
{
    /*! Pointer to GTI constant definition. See T_GTI_ENUMERATION_ELEMENT */
    const T_GTI_ENUMERATION_ELEMENT *consts;
}T_GTI_ENUMERATION_TABLE;

/*! Depreciated: Definition of GTI Enumeration type. */
typedef struct
{
    /*! Pointer to GTI constant definition. See T_GTI_ENUMERATION_ELEMENT */
    gti_sint_const_table_type *consts;
}gti_enum_table_type;

/** @} */ /* End of ISPEC_DRV_GTI_ENUMS group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */






/**    
@addtogroup ISPEC_DRV_TABLE_MACROS GTI Table Construction Macros
Optional macros intended to simplity creation of GTI tables. It is not urged that they are used. It may simplify table creation and improve readability.
@ingroup ISPEC_DRV_GTI 
*/
/**@{  */ /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/*! 
\brief Macro to define an array of signed integers.
\param GTI_VNAME_STR    Name of array
\param VAR              Array name (including full path e.g. family.children.boys )
\param NOF_ELEMENTS     Number of array elements
*/
#define GTI_VAR_ENTRY_SINT_ARRAY(GTI_VNAME_STR,VAR,NOF_ELEMENTS)                        { GTI_VNAME_STR, VAR, GTDT_SINT(sizeof(VAR[0])), NOF_ELEMENTS }
/*! 
\brief Macro to define an array of unsigned signed integers.
\param GTI_VNAME_STR    Name of array
\param VAR              Array name (including full path e.g. family.children.girls)
\param NOF_ELEMENTS     Number of array elements
*/
#define GTI_VAR_ENTRY_UINT_ARRAY(GTI_VNAME_STR,VAR,NOF_ELEMENTS)                        { GTI_VNAME_STR, VAR, GTDT_UINT(sizeof(VAR[0])), NOF_ELEMENTS }

/*! 
\brief Macro to define a signed integers.
\param GTI_VNAME_STR    Name of integer
\param VAR              integer name
*/
#define GTI_VAR_ENTRY_SINT(GTI_VNAME_STR,VAR)                                           { GTI_VNAME_STR, &VAR, GTDT_SINT(sizeof(VAR)), GTDT_SINGLE }
/*! 
\brief Macro to define an unsigned integers.
\param GTI_VNAME_STR    Name of integer
\param VAR              integer name
*/
#define GTI_VAR_ENTRY_UINT(GTI_VNAME_STR,VAR)                                           { GTI_VNAME_STR, &VAR, GTDT_UINT(sizeof(VAR)), GTDT_SINGLE}

/*! 
\brief Macro to define an array of structures.
\param GTI_VNAME_STR    Name of structure
\param GTI_TABLE_NAME   Name of GTI table describing structure
\param SVAR_NAME        structure name (including full path e.g. family.parents.mother)
\param NOF_ELEMENTS     Number of array elements
*/
#define GTI_VAR_ENTRY_STRUCT_ARRAY(GTI_VNAME_STR,GTI_TABLE_NAME,SVAR_NAME,NOF_ELEMENTS) { GTI_VNAME_STR, GTI_TABLE_NAME, GTDT_STRUCT_ARRAY(sizeof(SVAR_NAME [1])), NOF_ELEMENTS }

/*! 
\brief Macro to define a structure.
\param GTI_VNAME_STR    Name of structure
\param GTI_TABLE_NAME   Name of GTI table describing structure
*/
#define GTI_VAR_ENTRY_STRUCT(GTI_VNAME_STR,GTI_TABLE_NAME)                              { GTI_VNAME_STR, &GTI_TABLE_NAME, GTDT_STRUCT, GTDT_SINGLE }

/*! 
\brief Macro terminate a variable table.
*/
#define GTI_VAR_LIST_TERMINATOR()                                                       {NULL, NULL, 0,0 } 

/*! 
\brief Macro start declaring a variable table.
*/
#define GTI_VAR_TABLE_BEGIN(TABLE_NAME) gti_variable_table_type TABLE_NAME[] = {
/*! 
\brief Macro to terminate declaration of a variable table.
*/
#define GTI_VAR_TABLE_END()  GTI_VAR_LIST_TERMINATOR };



/*
#define GTI_VAR_ENTRY_SINT_ARRAY(gti_vname_str,var,nof_elements)                        { gti_vname_str, &var, GTDT_SINT(sizeof(var)), nof_elements },
#define GTI_VAR_ENTRY_UINT_ARRAY(gti_vname_str,var,nof_elements)                        { gti_vname_str, &var, GTDT_UINT(sizeof(var)), nof_elements },
#define GTI_VAR_ENTRY_SINT(gti_vname_str,var)                                           GTI_VAR_ENTRY_SINT_ARRAY(gti_vname_str,var,GTDT_SINGLE),
#define GTI_VAR_ENTRY_UINT(gti_vname_str,var)                                           GTI_VAR_ENTRY_UINT_ARRAY(gti_vname_str,var,GTDT_SINGLE),
#define GTI_VAR_ENTRY_STRUCT_ARRAY(gti_vname_str,svar_name,gti_table_name,nof_elements) { gti_vname_str, &gti_table_name, GTDT_STRUCT_ARRAY(sizeof(svar_name)), nof_elements },
#define GTI_VAR_ENTRY_STRUCT(gti_vname_str,svar_name,gti_table_name)                    GTI_VAR_ENTRY_STRUCT_ARRAY(gti_vname_str,svar_name,gti_table_name,GTDT_SINGLE),

#define GTI_VAR_LIST_TERMINATOR() {NULL, NULL, 0,0 }
*/

/** @} */ /* End of ISPEC_DRV_TABLE_MACROS group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */



/**    
@addtogroup ISPEC_DRV_GTI_SEC GTI Security definitions
Interface for setting security levels
@ingroup ISPEC_DRV_GTI 
*/
/**@{  */ /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/


/*! Definition of security levels in GTI. The level value in the level name (e.g. two (2) for gti_sec_lev_2_lab) can be used in security level modifiers for individual functions e.g. "@2:mysymbol" */
typedef enum
{
    /*! DO NOT USE! In case this value is specified (or value uninitialized) GTI will work as for gti_sec_lev_0_public but provide a warning when setting the interface. */ 
    GTI_SEC_LEV_UNINITIALIZED = 0,
        
    /*! GTI mapped security level: security level for public access i.e. no user restriction. */ 
    GTI_SEC_LEV_0_PUBLIC = 1,

    /*! GTI mapped security level: typical security level for access in production (open at security levels SEC, BOOT and TEST). */ 
    GTI_SEC_LEV_1_PRODUCTION = 2,
   
    /*! GTI mapped security level: typical security level for access in LAB or for debugging in field where security level can be set at this level (open at security levels BOOT and SEC). */ 
    GTI_SEC_LEV_2_LAB = 3,

    /*! GTI mapped security level: typical security level for only very strict access (open at security level BOOT). */ 
    GTI_SEC_LEV_3_STRICT = 4,

    /*! GTI mapped security level: highest possible access restriction . */ 
    GTI_SEC_LEV_MAX = GTI_SEC_LEV_3_STRICT,

    /***************************************************************/
    /* OBSOLETE values, please convert to new values defined above */
    /***************************************************************/

    /* OBSOLETE: DO NOT USE! In case this value is specified (or value uninitialized) GTI will work as for gti_sec_lev_0_public but provide a warning when setting the interface. */ 
    gti_sec_lev_uninitialized = 0,   /*please use UPPER case version*/
        
    /* OBSOLETE: GTI mapped security level: security level for public access i.e. no user restriction. */ 
    gti_sec_lev_0_public = 1,   /*please use UPPER case version*/

    /* OBSOLETE: GTI mapped security level: typical security level for access in production (open at security levels SEC, BOOT and TEST). */ 
    gti_sec_lev_1_production = 2,   /*please use UPPER case version*/
   
    /* OBSOLETE: GTI mapped security level: typical security level for access in LAB or for debugging in field where security level can be set at this level (open at security levels BOOT and SEC). */ 
    gti_sec_lev_2_lab = 3,  /*please use UPPER case version*/

    /* OBSOLETE: GTI mapped security level: typical security level for only very strict access (open at security level BOOT). */ 
    gti_sec_lev_3_strict = 4,   /*please use UPPER case version*/

    /* OBSOLETE: GTI mapped security level: highest possible access restriction . */ 
    gti_sec_lev_max = gti_sec_lev_3_strict,

    
    /* OBSOLETE, DO NOT USE: Access for all security levels */
    gti_sec_lev_none = 1,  

    /* OBSOLETE, DO NOT USE:Access UE access is opened at 'test' and more strict security levels. Recommended if the interface shall be available in production. */
    gti_sec_lev_test = 2,

    /* OBSOLETE, DO NOT USE:Access UE access is opened at 'sec' and more strict security levels. Recommended if the interface shall be used only in highly secured mode (not typical). Notice level properties is actually exchanged with SEC. */
    gti_sec_lev_boot = 4,

    /* OBSOLETE, DO NOT USE:Access UE access is opened at 'boot' and more strict security levels. Recommended if the interface shall be used for LAB purpose only or e.g. debugging where the phone can be unlocked to this level. Notice level is actually exchanged with BOOT.*/
    gti_sec_lev_sec  = 3   
}gti_sec_level_type;

/*! Group mask: Each bit represent one group (0..7). A set bit means disable group. */
typedef U8 gti_group_mask_type;

/* Group ID's for interfaces and GTI_NVM (each group corresponds to one bit in the gti_group_mask_type) */
typedef enum
{
    GTI_GROUP_ID0,
    GTI_GROUP_ID1,
    GTI_GROUP_ID2,
    GTI_GROUP_ID3,
    GTI_GROUP_ID4,
    GTI_GROUP_ID5,
    GTI_GROUP_ID6,
    GTI_GROUP_ID7,
    /**/
    GTI_NUM_GROUP_ID /* not a valid group */
}gti_group_id_type;


#define GTI_MAX_NUM_GROUPS (8*sizeof(gti_group_mask_type)) /* one group per bit */

/** @} */ /* End of ISPEC_DRV_GTI_SEC group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */


/**    
@addtogroup ISPEC_DRV_GTI_IF_DEF GTI Interface Description
Interface for defining the exposed GTI interface on AT@
@ingroup ISPEC_DRV_GTI 
*/
/**@{  */ /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/


/*! DEPRECIATED : GTI interface definition structure. This binds together all the parts of a test interface and is used as hook into the GTI system. */
struct gti_definition_s
{    
    /*! name-tag for interface. Preferably short. This tag is used as prefix to symbols in GTI commands, to distinguish implementation. */
#if defined(__cplusplus)
    const CHAR                               name[GTI_MAX_IFNAME_LEN+1];
#else
    CHAR                               name[GTI_MAX_IFNAME_LEN+1];
#endif
    /*! Short textual description of test interface. User of interface can query this as help. */
    const CHAR                              *if_description;
    /*! Pointer to main GTI v1.0 variable table - Depreciated*/
    gti_variable_table_type           *variable_table;
    /*! Pointer to GTI v1.0 function table */
    gti_func_table_type               *func_table;
    /*! Pointer to GTI v1.0 enumeration table */
    gti_enum_table_type               *enum_table;
    /*! Pointer to initialization function for test interface. GTI will call this at first reference to the interface */
    S32  (*init_func)(const struct gti_definition_s *if_def);
    /*! Pointer to kill function for test interface. GTI will call this when/if the user chooses to terminate the interface by "AT@<if-name>:!" command. */
    void (*kill_func)(void);
    /*! Arbitrary version string in ASCII - notice limited length */
    CHAR version_string[8];

    /* security access settings */
    /*! Security level required to access whole interface */
    gti_sec_level_type sec_lev_interface;
    /*! Default security level required to access variables - may be overwritten for individual variables by "@<level>:" prefix on name. */
    gti_sec_level_type default_sec_lev_variables;
    /*! Default security level required to access functions - may be overwritten for individual functions by "@<level>:" prefix on name.*/
    gti_sec_level_type default_sec_lev_functions;
    /*! Default security level required to access enumeration - may be overwritten for individual enumerations by "@<level>:" prefix on name.*/
    gti_sec_level_type default_sec_lev_enums;
    /*! Help Visual Security level. For lower levels queries of help info will fail */
    gti_sec_level_type help_visual_level;

    /*! Unused */
    U8 unused;

};
#ifdef GTI_PC
/*! DEPRECIATED :Definition gti interface definition type - see gti_definition_s structure */
typedef struct gti_definition_s gti_definition_type;
#else
/*! DEPRECIATED :Definition gti interface definition type - see gti_definition_s structure */
typedef const struct gti_definition_s gti_definition_type;
#endif

/*! GTI interface definition structure. This binds together all the parts of a test interface and is used as hook into the GTI system. */
typedef struct GTI_IF_DEFINITION_S
{
    /*! name-tag for interface. Preferably short. This tag is used as prefix to symbols in GTI commands, to distinguish implementation. */
    CHAR                               name[GTI_MAX_IFNAME_LEN+1];
    /*! Short textual description of test interface. User of interface can query this as help. */
    const CHAR                              *p_if_description;
    /*! Pointer to main GTI v2.0 variable table*/
    T_GTI_OPTIMIZED_VAR_TABLE           *p_variable_table;
    /*! Pointer to GTI v1.0 function table */
    gti_func_table_type               *p_func_table;
    /*! Pointer to GTI V2.0 enumeration table */
    T_GTI_ENUMERATION_TABLE               *p_enum_table;
    /*! Pointer to initialization function for test interface. GTI will call this at first reference to the interface */
    S32  (*p_init_func)(const struct GTI_IF_DEFINITION_S *p_if_def);
    /*! Pointer to kill function for test interface. GTI will call this when/if the user chooses to terminate the interface by "AT@<if-name>:!" command. */
    void (*p_kill_func)(void);
    /*! Arbitrary version string in ASCII */
    CHAR *p_version_string;

    /* security access settings */
    /*! Security level required to access whole interface */
    gti_sec_level_type sec_lev_interface;
    /*! Default security level required to access variables - may be overwritten for individual variables by "@<level>:" prefix on name. */
    gti_sec_level_type default_sec_lev_variables;
    /*! Default security level required to access functions - may be overwritten for individual functions by "@<level>:" prefix on name.*/
    gti_sec_level_type default_sec_lev_functions;
    /*! Default security level required to access enumeration - may be overwritten for individual enumerations by "@<level>:" prefix on name.*/
    gti_sec_level_type default_sec_lev_enums;
    /*! Help Visual Security level. For lower levels queries of help info will fail */
    gti_sec_level_type help_visual_level;

}T_GTI_IF_DEFINITION;


/*! Enum for interface operation management */
typedef enum 
{
    GTI_IF_OPM_INIT = 0,    /*!< Interface init notification */
    GTI_IF_OPM_KILL = 1     /*!< Interface kill notification */
} T_GTI_IF_OPM_TYPE;

/*! Operation management structure definition */
typedef struct
{
    U32 if_handle;
}T_GTI_V2_IF_OPM_ARGS;

/*! Operation management function definition */
typedef BOOL (*T_GTI_V2_IF_OPM_CB)(T_GTI_IF_OPM_TYPE opm, T_GTI_V2_IF_OPM_ARGS *p_opm_args);


/*! GTI interface definition v2 structure. This binds together all the parts of a test interface and is used as hook into the GTI system. 
      When the interface_version variable indicates GTIv2 interface-definition should be typecast to this.
      Note that 
      - OPM-callback is used instead of specific init- & kill-functions.
      - func_table is different. Function definition is of type 'T_GTI_V2_FUNC_DEFINITION'
      - enum_table is different. Enum definition is of type 'T_GTI_ENUMERATION_TABLE'
      - An U32 version_string is added  containing major, minor, sub, build information
      - 
*/
typedef struct GTI_V2_IF_DEFINITION
{
    /*! name-tag for interface. Preferably short. This tag is used as prefix to symbols in GTI commands, to distinguish implementation. */
    CHAR                           name[GTI_MAX_IFNAME_LEN+1];
    /*! Short textual description of test interface. User of interface can query this as help. */
    CHAR                           *p_if_description;
    /*! Pointer to main GTI v2.0 variable table*/
    T_GTI_OPTIMIZED_VAR_TABLE      *p_variable_table;
    /*! Pointer to GTI v2.0 function table */
    T_GTI_V2_FUNC_DEFINITION       *p_func_table;
    /*! Pointer to GTI V2.0 enumeration table */
    T_GTI_ENUMERATION_TABLE               *p_enum_table;
    /*! Pointer to operation management function for test interface. GTI will call this at interface init, kill and when setting properties */
    T_GTI_V2_IF_OPM_CB if_opm_func;
    /*! Version - U32 where version major, minor, sub is in lowest 3 octets. */
    U32 version;
    /*! Arbitrary version string in ASCII */
    CHAR *p_version_string;

    /* security access settings */
    /*! Security level required to access whole interface */
    gti_sec_level_type sec_lev_interface;
    /*! Default security level required to access variables - may be overwritten for individual variables by "@<level>:" prefix on name. */
    gti_sec_level_type default_sec_lev_variables;
    /*! Default security level required to access functions - may be overwritten for individual functions by "@<level>:" prefix on name.*/
    gti_sec_level_type default_sec_lev_functions;
    /*! Default security level required to access enumeration - may be overwritten for individual enumerations by "@<level>:" prefix on name.*/
    gti_sec_level_type default_sec_lev_enums;
    /*! Help Visual Security level. For lower levels queries of help info will fail */
    gti_sec_level_type help_visual_level;

}T_GTI_V2_IF_DEFINITION;


#ifdef GTI_PC
extern  IMP_CPP EXPORT_WIN S32   gti_add_interface(gti_definition_type *ifdef);
#else
/*!
\brief DEPRECIATED Function to dynamically add a GTI test interface to the active interface list i.e. it gets visible and accessible.
Alternatively the interface can be added statically in gti_init_if_cfg.h. 
\param ifdef    Structure defining the test interface content and attributes
\return         Whether the addition was succesfull (TRUE/FALSE). May fail if too many interface are allready active. 
*/ 
S32   gti_add_interface(gti_definition_type *ifdef);
#endif

#ifndef DUMMY_TEST_IF_ENABLED
/*!
\brief Function to dynamically add a GTI interface to the active interface list i.e. it gets visible and accessible.
Alternatively the interface can be added statically in gti_init_if_cfg.h. 
\param p_ifdef  Structure defining the test interface content and attributes
\return         Whether the addition was succesfull (TRUE/FALSE). May fail if too many interface are allready active. 
*/ 
S32   gti_add_v1_interface(T_GTI_IF_DEFINITION *p_ifdef);
#endif

/*!
\brief Function to dynamically add a GTI v2.0 interface to the active interface list i.e. it gets visible and accessible.
\param p_ifdef  Structure defining the test interface content and attributes
\return         Whether the addition was succesfull (TRUE/FALSE). May fail if too many interface are allready active. 
*/ 
S32   gti_add_v2_interface(T_GTI_V2_IF_DEFINITION *p_ifdef);

#ifdef GTI_PC
/*!
\brief DEPRECIATED: Function to kill and remove a GTI test interface from the active interface list. 
\param ifdef    Structure defining the test interface content and attributes
\return         None 
*/ 
extern  IMP_CPP EXPORT_WIN void gti_remove_interface(gti_definition_type *ifdef);
#elif (!defined DUMMY_TEST_IF_ENABLED)
/*!
\brief DEPRECIATED: Function to kill and remove a GTI test interface from the active interface list. 
\param ifdef    Structure defining the test interface content and attributes
\return         None 
*/ 
void gti_remove_interface(gti_definition_type *ifdef);
#endif

/*!
\brief Function to kill and remove a GTI test interface from the active interface list. 
\param p_if_name  Name of the interface to remove
\return           None 
*/ 
void gti_remove_interface_by_name(CHAR *p_if_name);


/*!
\brief Initialize a GTI interface including calling the init_func specified in the interface table.  This corresponds to an "AT@<ifnametag>:" command.
\param interface_tag     Name-tag for interface to initialize
\param check_security    Specifies if the initialization shall be only done if the current security level allows access to the interface.
\return                  TRUE in case of success, FALSE in case of error.
*/ 
BOOL gti_init_interface(CHAR *interface_tag, BOOL check_security);


/*!
\brief Kills a GTI interface including calling the kill_func specified in the interface table. 
This may imply deallocating memory used. This corresponds to an "AT@<ifnametag>:!" command.
\param interface_name_tag  Name-tag for interface to kill
\return             TRUE in case of success, FALSE in case of error.
*/ 
void gti_kill_interface(CHAR *interface_name_tag);

#ifndef DUMMY_TEST_IF_ENABLED
/*!
\brief DEPRECIATED- Function to set enabled groups mask on interface. GTI variable and function commands can 
       be assigned one or more groups
       by an "@G<gr1>...<grN>:" prefix e.g. "@G136:myfunc" means this "myfunc" command is only enabled when 
       group 1,3 and 6 are enabled i.e. the 'group enabled' bits are set.
\param ifdef    Structure defining the test interface content and attributes
\param enabled_group_mask One bit per group. group 1 and 3 enable means a mask of (1<<1)+(1<<3)
\param old_mask_p pointer for return of the old mask (NULL is allowed)
\return         TRUE/FALSE (may fail if the interface is not registered) 
*/ 
BOOL gti_set_interface_enabled_groups_mask(gti_definition_type *ifdef, gti_group_mask_type enabled_group_mask, gti_group_mask_type *old_mask_p);
#endif

/*!
\brief Function to set enabled groups mask on interface. GTI variable and function commands can 
       be assigned one or more groups
       by an "@G<gr1>...<grN>:" prefix e.g. "@G136:myfunc" means this "myfunc" command is only enabled when 
       group 1,3 and 6 are enabled i.e. the 'group enabled' bits are set.
\param ifdef    Structure defining the test interface content and attributes
\param enabled_group_mask One bit per group. group 1 and 3 enable means a mask of (1<<1)+(1<<3)
\param old_mask_p pointer for return of the old mask (NULL is allowed)
\return         TRUE/FALSE (may fail if the interface is not registered) 
*/ 
BOOL gti_v2_set_interface_enabled_groups_mask(T_GTI_IF_DEFINITION *ifdef, gti_group_mask_type enabled_group_mask, gti_group_mask_type *old_mask_p);


#ifndef DUMMY_TEST_IF_ENABLED
/*!
\brief Function to set a specific group on interface. See gti_set_interface_enabled_groups_mask() function for more information on groups.
\param p_ifdef    Structure defining the test interface content and attributes
\param group_id   Group to enable
\return           TRUE/FALSE (may fail if the interface is not registered or the group is invalid)
*/
BOOL gti_enable_interface_group(T_GTI_IF_DEFINITION *p_ifdef, gti_group_id_type group_id);
#endif

/*!
\brief Function to check if a specific group is enabled on an interface. See gti_set_interface_enabled_groups_mask() function for more information on groups.
\param p_ifdef    Structure defining the test interface content and attributes
\param group      Group to check
\return           Group status (TRUE/FALSE) (Notice! FALSE may also be due to that the interface is not registered or the group is invalid)
*/
BOOL gti_check_interface_group(T_GTI_IF_DEFINITION *p_ifdef, gti_group_id_type group);


/** @} */ /* End of ISPEC_DRV_GTI_IF_DEF group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */



/**    
@addtogroup ISPEC_DRV_GTI_DIVERT_FUNC GTI Interrupt and SDL-Signal Divert Mechanism's
Interfaces for diverting interrupts and SDL messages to testmodule
@ingroup ISPEC_DRV_GTI 
*/
/**@{  */ /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/


/*! Definition of valid responses to GTICOM SDL divert functions (see function gti_com_add_sdl_divert_func()) */
typedef enum    /* following flags may be combined to give a proper return value from your divert function */
{
/*! Signal is not used by the divert function */
    GTI_SDL_DIVERT_SIGNAL_NOT_USED      = 0x0,
    UTIF_DIVERT_SIGNAL_NOT_USED         = 0x0,  /* OBSOLETE definition name*/
/*! Signal is used and consumed by the divert function (may still be forwarded to other divert funcs) */
    GTI_SDL_DIVERT_SIGNAL_CONSUMED      = 0x1,
    UTIF_DIVERT_SIGNAL_CONSUMED         = 0x1,  /* OBSOLETE definition name*/
/*! Signal shall not be send further (not forwardet to other divert funcs) */
    GTI_SDL_DIVERT_SIGNAL_NO_FURTHER    = 0x2,
    UTIF_DIVERT_SIGNAL_NO_FURTHER       = 0x2   /* OBSOLETE definition name*/
} divert_funct_return;
/*Note: OBSOLETE definition name "divert_funct_return", do instead use name "gti_sdl_divert_funct_return" defined further down*/

typedef divert_funct_return gti_sdl_divert_funct_return;
/*Note: OBSOLETE definition "divert_funct_return", do instead use name "gti_sdl_divert_funct_return"*/

/*! Prototype for SDL Signal callback function 
*/
typedef gti_sdl_divert_funct_return (*gti_com_divert_func_type)(struct process_vars *yVarP);


/*!
\brief Install an SDL Divert function.
The divert function will be called for every SDL signal send in the system.
\param new_divert_func  Function which shall be called.
\result one of the values of gti_sdl_divert_funct_return
*/
S32 gti_com_add_sdl_divert_func(gti_com_divert_func_type new_divert_func);

/*!
\brief De-install an SDL Divert function.
The divert function will no longer be called for SDL signal send in the system.
\param divert_func  Function which was previously installed.
*/
void gti_com_remove_sdl_divert_func(gti_com_divert_func_type divert_func);

/*! Prototype for lisr (interrupt service routine) function 
*/
typedef void(*gti_com_lisr_func_ptr)(U32);

/*!
\brief Searches for old lisr function in interrupt vector list and substitues with new one.
\param old_lisr  the function pointer to substitute.
\param new_lisr  the new function
*/
S32 gti_com_hijack_lisr(gti_com_lisr_func_ptr old_lisr, gti_com_lisr_func_ptr new_lisr);


/** @} */ /* End of ISPEC_DRV_GTI_DIVERT_FUNC group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#undef GTI_SEQ_EVENT_DECLARATION
#define GTI_SEQ_EVENT_DECLARATION(EVENT,_SUBSYSTEM,_DESC) GTIEV_##EVENT,

//#define GTI_SEQ_NOF_INT_EVENTS 9


#define GTI_SEQ_INT_EVENTS()  \
/* do not remove*/           /*GTI_SEQ_EVENT_DECLARATION(noev,            gti, "no event")*/\
/* should always be first  GTI_SEQ_EVENT_DECLARATION(delayed_rsp,     gti, "delayed rsp event")*/\
                             GTI_SEQ_EVENT_DECLARATION(finish,          gti, "finish event")\
/* internal req events */    GTI_SEQ_EVENT_DECLARATION(kill,            gti, "kill event")\
                             GTI_SEQ_EVENT_DECLARATION(run,             gti, "run event")\
                             GTI_SEQ_EVENT_DECLARATION(break,           gti, "break event")\
                             GTI_SEQ_EVENT_DECLARATION(step,            gti, "step event")\
                             GTI_SEQ_EVENT_DECLARATION(reset,           gti, "reset event")\
                             GTI_SEQ_EVENT_DECLARATION(wakeup,          gti, "wakeup event") /* NB: used to define GTI_LAST_INTERNAL_EVENT */

#define GTI_FIRST_INTERNAL_EVENT         (0)
#define GTI_LAST_INTERNAL_EVENT          (GTIEV_wakeup)

#define GTI_SEQ_NUM_INTERNAL_EVENTS (GTI_LAST_INTERNAL_EVENT+1-GTI_FIRST_INTERNAL_EVENT)

#define GTI_FIRST_TRIGERABLE_INTERNAL_EVENT         (GTIEV_kill)

#define GTI_SEQ_FIRST_PUBLIC_EVENT_ID (GTI_SEQ_NUM_INTERNAL_EVENTS)


/* enum of events */
typedef enum
{
   _GTI_SEQ_EVENT_NOT_VALID = -1, /* error indication */
    GTI_SEQ_INT_EVENTS()
   _GTI_SEQ_PUBLIC_EVENTS_START_MARKER = GTI_SEQ_FIRST_PUBLIC_EVENT_ID - 1,

#include "gti_seq_events_cfg.h"     /* including enumeration of public events */

     GTI_SEQ_NOF_EVENTS
}gti_events_type;

#define GTI_SEQ_LAST_PUBLIC_EVENT_ID (GTI_SEQ_NOF_EVENTS-1)



/*! Array defining sequencer interest per event. Not to be changes, -only provided to allow optimized event handling. */
extern  U8 gti_event_seq_mask[];

/**    
@addtogroup ISPEC_DRV_GTI_SEQUENCER GTI Sequencer Interface
Interface for working with GTI Sequencers
@ingroup ISPEC_DRV_GTI 
*/
/**@{ */ /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/



/*! GTI Sequencer ID's  */
typedef enum
{
    GTI_SEQ_INVALID_ID = -1,
    GTI_SEQ_ID0 = 0,
    GTI_SEQ_ID1 = 1,
    GTI_SEQ_ID2 = 2,
    /*********/
    GTI_NUM_SEQ
}gti_seq_id_type;


/*! Enumeration of sequencer states */
typedef enum
{
    GTI_SEQST_DEAD,              /*! No thread */
    GTI_SEQST_RESERVED,          /*! Reserved a free seq by an user */
    GTI_SEQST_STOPPED,           /*! Stopped */
    GTI_SEQST_WAITING_EVENT_RT,  /*! Waiting event for realtime execution */
    GTI_SEQST_WAITING_EVENT,     /*! waiting on event semaphore */
    GTI_SEQST_RUNNING,           /*! Sequencer is running */
    GTI_SEQST_EXCEPTION,         /*! Sequencer ran into exception */
    GTI_SEQST_RELAY_EVENTS,      /*TBD*/
}gti_seq_state_type;

/*! Enumeration of action types which can be given to  gti_run_seq_script*/
typedef enum
{
    gti_seq_action_initload_only = 0x0,  /*! Only load and init*/
    gti_seq_action_run = 0x1,            /*! Run sequencer */
    gti_seq_action_wait_finish = 0x2,    /*! Wait for finish and kill sequencer*/
    gti_seq_action_run_wait_finish = 0x3,/*! Run and then wait for finish and kill sequencer*/
    gti_seq_action_invalid               /*! Invalid */
}gti_seq_action_type;


/*!
\brief callback function definition for sequencers running in plain C-code. I.e.
instead of running a GTISEQ script sequence the sequence is run by a C-function.
The 'active_step' keeps track of the sequence progress. It is incremented by one for every invocation of the callback. 
It is possible to single-step with the "at@seq_step()" command similar to when working with GTISEQ scripts. Please refer also gti_init_seq() and 
functional specification for more detailed information.
*/
typedef BOOL (*gti_seq_execute_step_callback_type)(gti_atif_handle_type atif_handle, gti_seq_id_type seq_id, S16 active_step);



/*!
\brief Extracts sequencer ID from an atif_handle.

\param atif_handle  Combined value containing information about the current command flow e.g. channel and flags. This is normally aquired from a previous GTI test function call.
\return             GTI sequencer ID (negative value means it is not a sequencer call)
*/ 
gti_seq_id_type gti_get_seq_id(gti_atif_handle_type atif_handle);

/*!
\brief Add a command line into the sequencer script.

\param seq_id   ID of sequencer in which to insert the line
\param line     at which line to insert (-1 means append)
\param line_str the command line to insert
\return         TRUE/FALSE depending if operation succeeded
*/ 
BOOL gti_add_seq_line(gti_seq_id_type seq_id, S32 line, CHAR *line_str);


/*!
\brief Remove lines from a sequencer script.

\param seq_id     ID of sequencer in which to insert the line
\param first_line First line to remove (-1 or 0 means first line)
\param last_line  Last line to remove (-1 or 0 means last line)
\return         TRUE/FALSE depending if operation succeeded
*/ 
BOOL gti_remove_seq_lines(gti_seq_id_type seq_id, S32 first_line, S32 last_line);


/*!
\brief Finds an unused sequencer if available and return ID 
\return sequencer ID (or GTI_SEQ_INVALID if no is unused)
*/
gti_seq_id_type gti_get_free_seq(void);


/*!
\brief GTI interface to initialize sequencer. In case it is allready initialized, it is re-initialized (cleared). 
\param atif_handle                     Combined value containing information about the current command flow e.g. channel and flags. This is normally aquired from a previous GTI test function call.
\param seq_id                          Sequencer ID
\param verbose_bitmask                 Bitmask to control output of sequencer. (0 is no output)
\param gti_seq_execute_step_callback   Pointer to optional callback function which is then executed instead of a GTISEQ script. See functional specification for details.
\return gti_err_code_type              Error code
*/
gti_err_code_type gti_init_seq(gti_atif_handle_type atif_handle, gti_seq_id_type seq_id, U16 verbose_bitmask, gti_seq_execute_step_callback_type gti_seq_execute_step_callback);


/*!
\brief Set verbose level for a sequencer.
\param seq_id        Sequencer ID
\param verbose_bitmask Bitmask to control output of sequencer. (0 is no output)
\return successfull or not (TRUE/FALSE)
*/
BOOL gti_seq_set_verbose(gti_seq_id_type seq_id, U16 verbose_bitmask);


/*!
\brief Resets sequencer state. Script is kept.
\param seq_id        Sequencer ID
\return none
*/
void gti_seq_reset(gti_seq_id_type seq_id);


/*!
\brief Kill sequencer i.e. reset and deallocate. Notice this is done asynchronously and 
may therefore not be done immediately after return of this function.
\param seq_id        Sequencer ID
\param from_isr      Obsolete parameter.
\return none
*/
void gti_seq_kill(gti_seq_id_type seq_id, BOOL from_isr);


/*!
\brief Set event callback functions for a sequencer. There is one callback for internal events and one for public events.
\param seq_id        Sequencer ID
\param internal_event_callback Function to get called when an internal event occur (before processing).
\param public_event_callback Function to get called when a public event occur (before processing).
\param state_change_callback Function to get called when a state change happens in sequencer
\return none
*/

void gti_seq_set_event_callbacks(gti_seq_id_type seq_id,
                             void (*internal_event_callback)(gti_seq_id_type seq_id, gti_events_type event),
                             void (*public_event_callback)(gti_seq_id_type seq_id, gti_events_type event),
                             void (*state_change_callback)(gti_seq_id_type seq_id, gti_seq_state_type old_state, gti_seq_state_type new_state));
                             


/*!
\brief Set log output callback functions for a sequencer. Any log information is given through this. Default is writing to terminal.
\param seq_id        Sequencer ID
\param log_write_back_ptr Function to get called to output debug information.
\return none
*/
void gti_seq_set_log_callback(gti_seq_id_type seq_id, gti_write_at_func_ptr_type log_write_back_ptr);


/*! Macro to generate event sequencer event ID from event name */
#define GTI_SEQ_EVENT_ID(EVENT_NAME) GTIEV_##EVENT_NAME

/*!
\brief Trigger sequencer with event i.e. schedules sequencer(s) waiting for this event, possibly only stores event depending on mode.
\param event_mask   bit 0..<GTI_NUM_SEQ-1> each represent a sequencer. High bit 'n' means trigger sequencer 'n' 
\param gti_event_id name of event, -as defined in gti_seq_events_cfg.h. Use macro GTI_SEQ_EVENT_ID(<eventname>) e.g. GTI_SEQ_EVENT_ID(run)
*/
void gti_seq_trig_event(U16 event_mask, gti_events_type gti_event_id);

/*!
\brief Add a script (multiple lines) into a sequencer.

\param seq_id   ID of sequencer in which to insert the line
\param start_line at which line to insert (-1 means append)
\param script_lines array of script lines to insert (NULL terminated)
\return         TRUE/FALSE depending if operation succeeded
*/
BOOL gti_load_seq_script(gti_seq_id_type seq_id, S32 start_line, CHAR **script_lines);

/*!
\brief Waits untill a given sequencer is finished execution. Waits only if sequencer is not dead.
\param seq_id   ID of sequencer on which to wait.
\return         void
*/
void gti_wait_seq_finish (gti_seq_id_type seq_id);

/*!
\brief  Initializes and loads a free sequencer with user scripts or callback. It can also further
        trigger it to run and wait untill sequencer is finished depending on actions required. If 
        user selects wait action then it also kills the sequencer after waiting.

\param atif_handle   Atif handle which can be used to write back to terminal from sequencer.
\param verbose_bitmask Bitmask to control output of sequencer. (0 is no output)
\param callback      Step execution callback. Can be NULL if user wants to run script lines.
\param script        Array of script lines to insert (NULL terminated). Can be NULL if user
                     wants to run callback.
\param actions       Actions specified  by gti_seq_action_type.
\return              Id of sequencer running user script or callback
*/
gti_seq_id_type gti_run_seq_script(gti_atif_handle_type atif_handle, U16 verbose_bitmask,
                                gti_seq_execute_step_callback_type callback, CHAR ** script,
                                gti_seq_action_type actions);


/*! Macro function for optimized triggering of an event. In case no sequencer is waiting for the event no trigger is generated. */
#define GTI_TRIG_EVENT(event_name) { if (gti_event_seq_mask[GTI_SEQ_EVENT_ID(event_name)]) gti_seq_trig_event(0xffff, GTI_SEQ_EVENT_ID(event_name)); }

/*! Macro function for optimized triggering of an event for a specific sequencer. In case the sequencer is not waiting for the event no trigger is generated. Note: Only used in special cases where the sequencer id is known. */
#define GTI_TRIG_EVENT_SEQ(seq_id, event_name) { if (gti_event_seq_mask[GTI_SEQ_EVENT_ID(event_name)]) gti_seq_trig_event(1<<(seq_id), GTI_SEQ_EVENT_ID(event_name)); }

/*! Macro function for optimized triggering of an event from ISR context. In case no sequencer is waiting for the event no trigger is generated. */
#define GTI_TRIG_EVENT_ISR(event_name) GTI_TRIG_EVENT(event_name)

/*! Macro function for optimized triggering of an event from ISR context for a specific sequencer. In case the sequencer is not waiting for the event no trigger is generated. Note: Only used in special cases where the sequencer id is known. */
#define GTI_TRIG_EVENT_SEQ_ISR(seq_id, event_name) GTI_TRIG_EVENT_SEQ(seq_id, event_name)




/*!
\brief Enables reception of specified events for this sequencer. I.e. it will be stored in event queue. Events must be separated by ' ' or '+' or ';'
\param seq_id     ID of sequencer.
\param event_list      String containing list of events seperated by '+' or ';'
\return success (TRUE) Failure (FALSE)
*/
BOOL gti_seq_enable_events(gti_seq_id_type seq_id, CHAR *event_list);

/*!
\brief Disables reception of specified events for this sequencer. Events must be separated by ' ' or '+' or ';'
\param seq_id     ID of sequencer.
\param event_list      String containing list of events seperated by '+' or ';'
\return success (TRUE) Failure (FALSE)
*/
BOOL gti_seq_disable_events(gti_seq_id_type seq_id, CHAR *event_list);

/*!
\brief Waits until one of the specified events occur.  Events must be separated by '+' or ';'. 
If 'blocking_mode' chosen, the function will block the thread and only return when the event(s) has occurred.
Otherwise the function will configure the sequencer and return immediately. 
Then please exit the calling gti_seq_execute_step_callback() function. GTI will call the function again when the event occur.
\param seq_id     ID of sequencer.
\param event_list
\param blocking_mode See description above. 
\return success (TRUE) Failure (FALSE)
*/
BOOL gti_seq_wait_events(gti_seq_id_type seq_id, CHAR *event_list, BOOL blocking_mode);

/*!
\brief Waits until one of the specified events occur. The function will configure for following real-time execution and return. 
Then please exit the calling gti_seq_execute_step_callback() function.
GTI will call the function again when the event occur.
\param seq_id          ID of sequencer.
\param event_list      String containing list of events seperated by '+' or ';'
\return success (TRUE) Failure (FALSE)
*/
BOOL gti_seq_wait_events_rt(gti_seq_id_type seq_id, CHAR *event_list);

/*!
\brief Programs wait events for a GTI command only when executing in a sequencer or pipe.  Events must be separated by '+' or ';'. 
If 'blocking_mode' chosen, the function will block the thread and only return when the event(s) has occurred.
Otherwise the function will configure the wait events and return immediately.
\param atif_handle     Atif handle given from GTI
\param event_list
\param blocking_mode See description above. 
\return success (TRUE) Failure (FALSE)
*/
BOOL gti_wait_events (gti_atif_handle_type atif_handle, CHAR *event_list, BOOL blocking_mode);

/*!
\brief Check if the specified events is in the event list. Keeps event in list afterwards.  
\param seq_id          ID of sequencer.
\param event_name      String containing name of GTI event
\return success (TRUE) Failure (FALSE)
*/
BOOL gti_seq_check_event(gti_seq_id_type seq_id, CHAR *event_name);


/*!
\brief Check if the specified events is in the event list, and if true also removes it. 
\param seq_id          ID of sequencer.
\param event_name      String containing name of GTI event
\return success (TRUE) Failure (FALSE)
*/
BOOL gti_seq_checkout_event(gti_seq_id_type seq_id, CHAR *event_name);

/*!
\brief Returns latest event from event queue and also removes it from queue 
\param seq_id     ID of sequencer.
\param event_p  Pointer where to store event ID.
\return success (TRUE) Failure (FALSE) and *event_p
*/
BOOL gti_seq_get_event(gti_seq_id_type seq_id, gti_events_type *event_p);

/*!
\brief Clears all events in the sequencer queue 
\param seq_id          ID of sequencer.
\return success (TRUE) Failure (FALSE)
*/
BOOL gti_seq_clear_events(gti_seq_id_type seq_id);


/*!
\brief Changes sequencer into exception state. Outputs an error message to the terminal indicating exception and 'info' as cause.
\param seq_id          ID of sequencer.
\param info information about exception.
\return none
*/
void gti_seq_exception(gti_seq_id_type seq_id, CHAR *info);

/*! Macro  to wait on sequencer events from a sequencer step callback.*/
#define GTI_SEQ_WAIT_ON_EVENTS(seq_id,event_list) gti_seq_wait_events(seq_id, event_list,TRUE)

/*! Macro  to configure wait on sequencer events from a sequencer step callback. Seqeuncer will wait for the events after
    callback returns.*/
#define GTI_SEQ_CONFIG_WAIT_EVENT(seq_id,event_list) gti_seq_wait_events(seq_id,event_list,FALSE)

/*! Macro  to wait on sequencer events from a GTI command. Will work only when command is executed from
    a pipe or sequencer.*/
#define GTI_WAIT_ON_EVENTS(atif_handle,event_list) gti_wait_events(atif_handle,event_list,TRUE)

/*! Macro  to configure wait on sequencer events from a GTI command. Will work only when command is executed from
    a pipe or sequencer. Wait is done after the command handler exits.*/
#define GTI_CONFIG_WAIT_ON_EVENT(atif_handle,event_list) gti_wait_events(atif_handle,event_list,FALSE)

/** @} */ /* End of ISPEC_DRV_GTI_SEQUENCER group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */


/**    
@addtogroup ISPEC_DRV_NVM_IF GTI NVM support interface
The GTI NVM interface provides access to NVM structures of target modules using "at@" commands. An "at@nvm:" interface is exposed with 
an NVM variable tables for each module. Also functions to read and write the NVM parameters by NVM_ID's (most suitable for tooling). 
The NVM_ID's are obtained by an at@ command, on the "at@nvm:" interface, capable of dumping the NVM structure with unique ID's for each NVM parameter. 
Different formats are supported e.g XML.
@ingroup ISPEC_DRV_GTI 
*/
/**@{ */


/*! typedefinition for NVM parameter ID (may be queried from target on "at@nvm:" interface) */
typedef U16 gti_nvm_id_type;

/*!
\ typedefinition for mapping info parameter ID (e.g. for NU: nvm unique id's) 
\ reflects as for gti_var_id_type
*/
typedef U32 gti_nvm_mapinfo_type;

/*!
\ typedefinition for handle to GTI's information regarding an NVM group.
*/
typedef void * T_GTI_NVM_GRP_HANDLE;

/*!
\ typedefinition for structure in which GTI fills in typedefinition for a NVM ID.
*/
typedef struct
{
    void * data_ptr; /** Pointer to requested element of this id in the mirror*/
    gti_datatype_type datatype; /**datatype info stored in gti var tables for this id*/
    U32 size_info; /** base size of one element of this id */
    U32 num_elements; /** Number of elements in this id */
    U32 atomic_size; /** read write size. Atomic size in which one element of this id can we read or written */
    U32 step_size; /** Step size. Size to jump inorder to reach next atomic part of an element of this id */
    U32 num_steps; /** number of steps of step_size which are in this id */
    U32 bitmask; /** Bitmask of atomic part of an element of an id. for non-bitfields it is 0xffffffff (all bits)*/
    U8  start_bit; /** Start bit of atomic part of an element of an id. For non-bitfields it is 0*/ 
    gti_nvm_id_type parent_id; /** Parent id for an volatile id. For non-volatile this is GTI_NVM_ID_INVALID*/
}gti_nvm_id_info_type;

/*!
\ typedefinition for structure which is used for translating an mirror address to an external address.
*/
typedef struct TRANSLATION_ENTRY
{
    void* mirror_address; /** Address in the RAM mirror */
    U32 external_address; /** External address for the mirror offset */
}T_TRANSLATION_ENTRY;


/*!
\brief  Initializes the GTI NVM system. This means that NVM parameters can be accessed using NVM ID's
\return TRUE/FALSE verdict if initialization was ok. Should only fail in case of lacking memory. This function may allready be called from the GTI OPM function at boot-up.
*/
BOOL gti_init_nvm_system(void);

/*!
\brief  Kills the GTI NVM system (deallocates data). 
\return none
*/
void gti_kill_nvm_system(void);

/*!
\brief Obtains handle to GTI NVM group for use in related API's
\param grp_name     Name of group like "cal_aud"
\return GTI NVM group handle
*/
T_GTI_NVM_GRP_HANDLE gti_nvm_get_grp_handle(const CHAR *grp_name);

/*!
\brief Copies data between GTI variable/array and some 'outside' data array. Handles conversions between different variable typs (8, 16,32 bit)
\param atif_handle      e.g. GTI_DEFAULT_ATIF_HANDLE or specific if known
\param write            TRUE means write from outside to variable. FALSE means read from variable to outside.
\param nvm_id           nvm_id identifying the NVM parameter (queried from "at@nvm:" interface)
\param outside_data_p   Pointer to 'outside' data
\param outside_datatype GTI datatype of 'outside' data
\param count            number of elements to copy. If null (0) the maximum possible will be read/written i.e. maximum til end of array.
\param offset           first element to copy from/to in GTI variable array (0 in case of single variable)
\return                 TRUE/FALSE verdict if operation went OK. May fail if exceeding array boundary.
*/
gti_err_code_type gti_nvm_rd_wr_atif(gti_atif_handle_type atif_handle,
                                BOOL write, 
                                gti_nvm_id_type  nvm_id, 
                                void            *outside_data_p, 
                                gti_datatypes    outside_datatype, 
                                U32     count, 
                                U32     offset);

/*!
\brief Copies data between GTI variable/array and some 'outside' data array. Handles conversions between different variable typs (8, 16,32 bit)
\param write            TRUE means write from outside to variable. FALSE means read from variable to outside.
\param nvm_id           nvm_id identifying the NVM parameter (queried from "at@nvm:" interface)
\param outside_data_p   Pointer to 'outside' data
\param outside_datatype GTI datatype of 'outside' data
\param count            number of elements to copy. If null (0) the maximum possible will be read/written i.e. maximum til end of array.
\param offset           first element to copy from/to in GTI variable array (0 in case of single variable)
\return                 TRUE/FALSE verdict if operation went OK. May fail if exceeding array boundary.
*/
gti_err_code_type gti_nvm_rd_wr(BOOL write, 
                                gti_nvm_id_type  nvm_id, 
                                void            *outside_data_p, 
                                gti_datatypes    outside_datatype, 
                                U32     count, 
                                U32     offset);

/*!
\brief Copies data between GTI variable/array and some 'outside' data array. Handles conversions between different variable typs (8, 16,32 bit)
\param write            TRUE means write from outside to variable. FALSE means read from variable to outside.
\param grp_name         Name of group like "cal_aud"
\param relative_id      ID in range defined by module owning group (FFS: make enum of ID's by Perl script or similar)
\param outside_data_p   Pointer to 'outside' data
\param outside_datatype GTI datatype of 'outside' data
\param count            number of elements to copy. If null (0) the maximum possible will be read/written i.e. maximum til end of array.
\param offset           first element to copy from/to in GTI variable array (0 in case of single variable)
\return                 TRUE/FALSE verdict if operation went OK. May fail if exceeding array boundary.
*/
gti_err_code_type gti_nvm_group_rd_wr(BOOL write,
                                const CHAR *grp_name,
                                U32         relative_id,
                                void            *outside_data_p,
                                gti_datatypes    outside_datatype,
                                U32     count,
                                U32     offset);


/*!
\brief Obsolete. Use gti_virt_var_rd_delayed instead
\return          void
*/
void gti_nvm_rd_func_delayed (gti_atif_handle_type atif_handle, gti_err_code_type err_code);


/*!
\brief Get pointer to data in mirror of group onspecified NVM ID. Notice this will fail if there is no mirror pointer to data.
\param grp_handle   Handle to GTI NVM group obtained by gti_nvm_get_grp_handle()
\param relative_id           ID in range defined by module owning group (FFS: make enum of ID's by Perl script or similar)
\param array_index           Array Index in case of arrays OR arrays of Structures
\param id_info      Pointer to info structure into which gti will fill the info about the NVM parameter
\return GTI error code
*/
gti_err_code_type  gti_nvm_get_id_info(T_GTI_NVM_GRP_HANDLE grp_handle, U32 relative_id, U32 array_index, gti_nvm_id_info_type *id_info);


/*!
\brief Get pointer to the default data onspecified NVM ID. Notice this will return NULL if there is no default data.
\param grp_handle   Handle to GTI NVM group obtained by gti_nvm_get_grp_handle()
\param relative_id           ID in range defined by module owning group (FFS: make enum of ID's by Perl script or similar)
\param array_index           Array Index in case of arrays OR arrays of Structures
\return Pointer to the default data for the ID. (NULL if there is no default data or default data for a compound is split or a Killed or invalid ID is passed)
*/
void*  gti_nvm_get_default_data_ptr(T_GTI_NVM_GRP_HANDLE grp_handle, U32 relative_id, U32 array_index);

/*!
\brief Get pointer to the data stored in flash for the tff ID.
\param translate_info_p   Pointer to copy of translatation data from NVM flash.
\param nvm_data_p         Pointer to copy of NVM flash data i.e. to the NVM header just before the NVM data.
\param tff_id             TFF ID for which the offset is required.
\return Pointer to the data for the tff_id
*/
void*  gti_nvm_get_flash_data_ptr(void *translate_info_p, void *nvm_data_p, U16 tff_id);

/*!
\brief Get external address mapping for a RAM mirror offset. Use case is when the RAM mirror contain copies of data with origin from fragmented addresses. By this method it is possible to extract this mapping e.g. when it is needed to copy back data from the mirror.
\param table_p              Pointer to address translation table.
\param num_entries          Number of entries in the table.
\param mirror_address       Address in the RAM mirror.
\param external_address_p   External address for the mirror offset (Output parameter).
\return GTI error code
*/
gti_err_code_type  gti_nvm_get_external_address(T_TRANSLATION_ENTRY *table_p, U16 num_entries, void* mirror_address, U32* external_address_p);




/*! Pointer to NVM parameter init data (used for default data initialization). 
    The data pointers must be in a specified sequence. 
    The "at@nvm:dump_structure()" command can be used for dumping required tables and enumerations.
*/
typedef const void* gti_nvm_init_data_el_ptr_type;

/*! Structure providing default data for part of or complete NVM group in binary
    Table should be terminated by a NULL entry*/
typedef struct
{
    void * data_p; /** Pointer to default data in binary*/
    U32 offset;  /** Offset in the mirror where this default data needs to be copied*/
    U32 size;   /** Size of this default data */
}gti_nvm_default_data_binary_type;



/*!
\brief Fills in NVM mirror with default data.
\param group_name              Name of group e.g. "cal_rf3g_ue2"
\param filter_p     Pointer to string specifying which variables and sub-structures was included generating the 'init_table'. May be NULL.
\param init_table_p Array of pointers to initialization data for each NVM ID, if NULL default data in Binary format is used if available.
\param from_id      First ID which shall be copied
\param to_id        Last ID which shall be copied (included). Special value 0 means all ID's.
\return TRUE/FALSE verdict if operation went OK. May fail if interface name tag is not registered.
*/
BOOL gti_nvm_fill_default_data(CHAR *group_name,
                          const CHAR *filter_p,
                          const gti_nvm_init_data_el_ptr_type *init_table_p, 
                          gti_nvm_id_type from_id, 
                          gti_nvm_id_type to_id);


/* NVM Tagged File Format ID type. A unique ID stored together with the NVM data to converto to other NVM versions */
typedef gti_nvm_id_type gti_nvm_tff_id_type;

/*! Limited to 1023 TFF ID's  due to format for storing nvm translation info*/
#define GTI_NVM_MAX_NOF_TFF_IDS 0x03FF
/*! Limited to 1023 TFF ID's  due to format for storing nvm translation info*/
#define GTI_NVM_ID_INVALID 0xFFFF

/*! End marker for gti_nvm_tff_id_type tables */
#define GTI_NVM_TFF_END_MARKER  GTI_NVM_MAX_NOF_TFF_IDS
/*! Special back up ID marking that the ID is obsolete */
#define GTI_NVM_TFF_KILLED_ID_MARKER    0xefff
/*! Special back up ID marking that the ID shall be translated by callback function before use */
#define GTI_NVM_TFF_TRANSLATE_ID_MARKER 0xdfff

/*! Obsolete definition: Back up ID type definition - Use defined value instead */
#define gti_nvm_bu_id_type gti_nvm_tff_id_type  /* OBSOLETE */

/*! Limited to 1023  ID's  due to format for storing nvm translation info - NU -"nvm unique"*/
#define GTI_NVM_MAX_NOF_NU_IDS 0x03FF
/*! End marker for gti_nvm_unigue_id_type tables */
#define GTI_NVM_NU_END_MARKER  GTI_NVM_MAX_NOF_NU_IDS
/*! Special back up VAR-ID marking that the VAR-ID is obsolete - meaning not to be used anymore */
#define GTI_NVM_NU_KILLED_ID_MARKER    GTI_NVM_ID_INVALID

/*! Limited to 65535 ID's as we have 16 bits for ID in the LUT table */
#define GTI_NVM_MAX_NOF_LUT_IDS 0xFFFF
#define GTI_NVM_TFF_LUT_END_MARKER  GTI_NVM_MAX_NOF_LUT_IDS



/*! Callback function prototype for translation functions taking an GTI NVM Backup ID and data as input, for translation and copy into local NVM mirror. */
typedef void (*gti_nvm_data_convert_cbf_type)(gti_nvm_tff_id_type nvm_tff_id, void *data, gti_datatypes data_type, U32 array_len);

/*! Callback function prototype for translation functions taking an relative GTI NVM LUT ID and data as input, for translation and copy into local NVM mirror. */
typedef void (*gti_nvm_lutdata_convert_cbf_type)(gti_nvm_id_type relative_id, void *data, U32 version_in_flash);

/*!
\brief Writes the NVM data specified by interface tag and filter into a tff translation info block, -which can be read using gti_nvm_tff_rd(). The
NVM data must be registered in the GTI NVM system in the include file "gti_nvm_cfg.h".
\param group_name           name tag for nvm group e.g. "cal_urf"
\param filter_p             Pointer to string specifying which variables and sub-structures was included generating the 'init_table'. May be NULL.
\param dest_buf_p           Pointer to the destination buffer.
\param dest_buf_size        Size of the destination buffer. This function will do an MS_exit() in case this is exceeded.
\param tff_id_mapping_p     Table mapping tff ID's to NVM_ID's directly or by translation function (or simply killing it).
\param offset_mode_base_ptr This is NULL in the normal case. Optionally provide pointer to the local mirror. This is used as reference for offsets.
                            The backup block will now refer to data in the local mirror by offsets. Use the same or some other buffer (mirror) as
                            reference when reading with the gti_nvm_tff_rd() function. It can e.g. be used to read old EEP data as a backup block,
                            which like normal backup blocks can be translated to any NVM version.
\return Pointer to next free byte in dest_buf_p. Can be used to evaluate length of tff translation info block.
*/
void *gti_nvm_tff_wr( const CHAR *group_name,
                   const CHAR *filter_p,
                   void *dest_buf_p,
                   S32   dest_buf_size,
                   const gti_nvm_id_type *tff_id_mapping_p,
                   const void *offset_mode_base_ptr);


#define gti_nvm_bu_wr gti_nvm_tff_wr /* OBSOLETE: Use defined value */

/*!
\brief Reads NVM data from a 'backup' block into the modules local mirror. It is capable of doing this no matter the current NVM version. 
TFF Translation ID's which are not know are ignored. Others which need translation will be given to the callback function registered in the gti_init_nvm_cfg.h
Some ID's may not be translateable. These are marked as "killed" in the tff_id_mapping_p table, and are therefore just ignored. The 
remaining parameters are directly copied into the miror by this function.
\param nvm_tff_data_p           Pointer to tff translation info data stored by gti_nvm_tff_wr(). Either data is included (interleaved) or it is pointed to seperately in offset_mode_base_ptr.
\param nvm_tff_id_mapping_p         Table mapping tff ID's to NVM_ID's directly or by translation function (or simply killing it).
\param translate_param_cbfunc_p Callback function which gets called for all tff ID's marked by macro GTI_NVM_TFF_TRANSL_ID(<id>).
\param offset_mode_base_ptr    Pointer to local NVM 'backup' data block. See gti_nvm_tff_wr()
\return TRUE/FALSE verdict if operation went OK. May e.g. fail if interface name tag is not registered.
*/
BOOL gti_nvm_tff_rd(void *nvm_tff_data_p, 
                 const gti_nvm_tff_id_type *nvm_tff_id_mapping_p, 
                 gti_nvm_data_convert_cbf_type translate_param_cbfunc_p,                    
                 const void *offset_mode_base_ptr);

/***********LUT*********************************/

/*! End-marker to be put as last entry in tables of T_GTI_NVM_LUT_ENTRY */
#define GTI_NVM_LUT_TABLE_END_MARKER 0xFFFFFFFF

/*! Possible states of NVM ID's. The state is stored in the two least significant bits of the T_GTI_NVM_LUT_ENTRY (Look Up Table) elements */
typedef enum
{
    GTI_NVM_ID_STATE_ACTIVE     = 0, /** Active ID which can be read and stored */
    GTI_NVM_ID_STATE_KILLED     = 1,  /** Obsolete ID, which stored data can in no way be used */
    GTI_NVM_ID_STATE_VOLATILE   = 2, /** ID, but which is info only and not stored */
    GTI_NVM_ID_STATE_VOLATILE_COMPOUND_PARAM     = 3,  /** ID, for a compound param. IT is not used while storing */
    GTI_NVM_ID_STATE_VOLATILE_SUBARRAY     = 4  /** ID, for a sub-array. IT is not used while storing */
}T_GTI_NVM_LUT_ID_STATE;


typedef struct 
{
    U32 state:3;
    U32 reserved:1;
    U32 var_table_offset:12; //is also used as sub-arrayidx when state is VOLATILE_SUBARRAY
    U32 translation:16; //is also used as parent_id when state is VOLATILE_COMPOUND_PARAM or VOLATILE_SUBARRAY
}T_GTI_NVM_LUT_ENTRY;

typedef enum
{
   GTI_DEFAULT_DATA_TYPE_ID_TABLE = 0, /** Default data initialization arrays. One 'array' per NVM ID */
   GTI_DEFAULT_DATA_TYPE_BINARY, /** Binary default*/
   GTI_DEFAULT_DATA_TYPE_BINARY_TABLE /** Binary default data table with variant data */
}GTI_DEFAULT_DATA_TYPE;


/*! Macro to get the state of a LUT ID*/
#define M_GET_GTI_NVM_LUT_ID_STATE(GTI_NVM_LUT_ENTRY) ((T_GTI_NVM_LUT_ID_STATE)((GTI_NVM_LUT_ENTRY).state))


/*!
\brief Get pointer to the default data for specified LUT ID (Can be used before the mirror is initialized).
\param grp_handle   Handle to GTI NVM group obtained by gti_nvm_get_grp_handle()
\param lut_id_table_p   Pointer to the LUT Table
\param default_data_p   Pointer to the Default Data
\param default_data_type   Type of default data
\param relative_id  ID in range defined by module owning group
\return Pointer to the default data for the ID. (NULL if there is no default data or incomplete default data or Killed or invalid ID)
*/
void*  gti_nvm_lut_get_default_data_ptr(T_GTI_NVM_GRP_HANDLE grp_handle, const T_GTI_NVM_LUT_ENTRY *lut_id_table_p, void* default_data_p, GTI_DEFAULT_DATA_TYPE default_data_type, U32 relative_id);


/********************LUT end***************************/

/*! Callback actions which shall be implemented by the GTI NVM clients registering using the GTI_NVM_ACTIONS_CB() macro in gti_nvm_cfg.h */
typedef enum
{
    GTI_NVM_CBA_DUMP_OFFSET_BACKUP = 0,  /* notice value must not be changed */

    GTI_NVM_CBA_MIRROR_TO_NVM      = 10,  /* notice value must not be changed */     
    GTI_NVM_CBA_MIRROR_TO_BACKUP   = 11,  /* OBSOLETE - notice value must not be changed */      

    GTI_NVM_CBA_BACKUP_TO_MIRROR   = 20,  /* OBSOLETE -notice value must not be changed */
    GTI_NVM_CBA_EEP_TO_MIRROR      = 21,  /* notice value must not be changed */
    GTI_NVM_CBA_NVM_TO_MIRROR      = 22,  /* notice value must not be changed */
    GTI_NVM_CBA_DEFAULT_TO_MIRROR  = 23,  /* notice value must not be changed */
    GTI_NVM_CBA_STORE_NVM          = 24,  /* notice value must not be changed */
    GTI_NVM_CBA_WAIT_NVM_IDLE      = 25,  /* notice value must not be changed */
    GTI_NVM_RELOAD_NVM_MIRRORS     = 26,  /* notice value must not be changed */
    GTI_NVM_CBA_STORE_NVM_SYNC     = 27,  /* notice value must not be changed */

    GTI_NVM_CBA_INVALIDATE_BACKUP  = 30,  /* notice value must not be changed */
    GTI_NVM_CBA_INVALIDATE_CALIB   = 31,  /* notice value must not be changed */
    GTI_NVM_CBA_INVALIDATE_FIXED   = 32,  /* notice value must not be changed */
    GTI_NVM_CBA_INVALIDATE_GROUP   = 33,  /* notice value must not be changed */
    GTI_NVM_CBA_READ_NVM_INFO      = 40,  /* notice value must not be changed */
    GTI_NVM_CBA_GET_VER_REV_LENGTH = 41,  /* notice value must not be changed */
    GTI_NVM_CBA_SET_VER_REV        = 42,  /* notice value must not be changed */
    GTI_NVM_CBA_CRC32_CHKSUM_VALUE = 43,  /* notice value must not be changed */
    __DUMMY_GTI_NVM_CBA__
}gti_nvm_callback_action_type;

/*! GTI notification cbf operation ID's  */
typedef enum
{
    GTI_NVM_NOTIFY_NO_ACTION,   /* actually nothing done */
    GTI_NVM_NOTIFY_WRITE,   /* a nvm write has been carried out */
    GTI_NVM_NOTIFY_READ,    /* a nvm read is to be carried out */
}gti_nvm_notify_operation_info_type;


/*! Callback function prototype to inform interface-"owners" of gti-updates of cal-area,.. for action to be made on a nvm write (change to calibration area) */
typedef void (*gti_nvm_notification_cbf_type)(const gti_variable_ref_type *gvar_ref_p, gti_nvm_notify_operation_info_type operation_info);

/*! special info structure for the GTI_NVM_CBA_DUMP_OFFSET_BACKUP callback action. */    
typedef struct
{
    S32   resp_buf_size;  
}gti_nvm_cb_dump_offset_backup_action_info_type;
   

/*! Union of special info structures specific to the actions by the enum gti_nvm_callback_action_type */
typedef union
{
    gti_nvm_cb_dump_offset_backup_action_info_type dump_offset_backup_info;

}gti_nvm_callback_action_info_type;


/*! Structure defining properties of nvm group to GTI_NVM */
typedef struct
{
    void *idx_subst_info_p;
    U32 nvm_id;
}gti_nvm_group_def_type;


/*! Purpose types for GTI asking callback function for NVM group info */
typedef enum
{
    GTI_NVM_ICBP_WRITE_MIRROR,
    GTI_NVM_ICBP_INFO_ONLY  
}gti_nvm_info_cb_purpose_type;


/*! Structure encapsulating data for NVM translation between versions. GTI will ask the driver for this during initialization of the external NVM mirror. */
typedef struct
{
    const CHAR *textual_info_p;  /* Optionally fill in short textual description of the group (please max 200 chars) */
    const CHAR *group_assignment[GTI_MAX_NUM_GROUPS]; /* If using GTI grouping (not same as NVM group): Textual name explaining what each GTI group is assigned for */

    const gti_nvm_init_data_el_ptr_type *default_data_table_p;  /* Pointer to table of default data initialization arrays. One 'array' per NVM ID */
    const void *default_data_binary_p;  /* Pointer to binary default data as alternative to default_data_table_p. If default_data_table_p is assigned this pointer will not be used */
    const gti_nvm_default_data_binary_type * default_data_binary_table_p;

    const gti_nvm_id_type               *tff_id_mapping_p; /* Mapping table where the index is the NVM TFF ID and the value is the NVM ID where to map */
    const T_GTI_NVM_LUT_ENTRY           *lut_id_mapping_p; /* Mapping table where the index is the NVM UNIQUE ID  */
    const gti_nvm_mapinfo_type          *nu_id_mapping_p; /**Temporary***/
    const T_GTI_OPTIMIZED_VAR_TABLE     *p_opt_var_table; /* Pointer to GTI Optimized var table which could be set dynamically */

    gti_nvm_data_convert_cbf_type       translate_param_cbfunc_p; /* callback function to handle NVM TFF ID's marked for translation in the tcff_id table */
    gti_nvm_lutdata_convert_cbf_type    translate_lutparam_cbfunc_p; /* callback function to handle NVM TFF ID's marked for translation in the tcff_id table */
    gti_nvm_notification_cbf_type           nvm_notification_cbfunc_p; /* callback function to handle gti informatiing of parameter-change to cal area  */

    U32 current_version;    /* Write the current version of the NVM group. GTI will use this for decision making and also write this to the mirror header */
    U32 current_revision;/* Write the current revision of the NVM group. GTI write this to the mirror header */
    U32 current_data_size; /* OBSOLETE (no need to use): Write the current size of the NVM group EXCLUDING the NVM header. GTI will use to avoid writing to fare and also write this to the mirror header */
}gti_nvm_group_info_type;

/*! Function prototype for NVM user callback to get NVM group info. Shall be registered in "gti_nvm_cfg.h" */
typedef BOOL (*gti_nvm_get_group_info_cbf_type)(gti_nvm_info_cb_purpose_type purpose, void *stored_nvm_data, void *translate_info_p, gti_nvm_group_info_type *return_group_info_p);


/*!
\brief Function to set enabled groups mask on a GTI NVM group. An NVM group variable can be assigned a
       "@G<gr1>...<grN>:" prefix e.g. "@G136:pow_offset" means this "pow_offset" parameter is only enabled when 
       group 1,3 and 6 are enabled i.e. the 'group enabled' bits are set. Setting the enable_group_mask decides
       groups are exposed on the "AT@nvm" interface.
\param group_name    Name of the GTI NVM group
\param enabled_group_mask One bit per group. group 1 and 3 enable means a mask of (1<<1)+(1<<3)
\param old_group_mask_p pointer for return of the old mask (NULL is allowed)
\return         TRUE/FALSE. Will fail if specified NVM group does not exist.
*/ 

BOOL gti_nvm_set_enabled_groups_mask(const CHAR *group_name, gti_group_mask_type enabled_group_mask, gti_group_mask_type *old_group_mask_p);

/** @} */ /* End of ISPEC_DRV_NVM_IF group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */


/**    
@addtogroup ISPEC_DRV_OS_EVENT_IF Interface to generate and wait for masks of events between threads.
GTI provides an API to work with OS events. Different underlying OS's and abstraction layers provides different API's. GTI supports
event masks and an abstration to the OS API, which can be very convenient for test code.

@ingroup ISPEC_DRV_GTI 
*/
/**@{ */

/*! Handle used to identify a GTI OS event group */
typedef U16 gti_os_event_group_handle_type;

/*! If allocation of an event group fails this value is retured as handle */
#define GTI_INVALID_OS_EVENT_GROUP_HANDLE 0

/*!
\brief Initializes an event group to be used with GTI API. This offers an abstraction to the underlying OS and supports 
event masks (not only binary event as defined for events in UTA API). 
Notice only a limited pool of event groups are available. Please release it after use by gti_release_os_event_group().
\param event_name   Some name to give the eventgroup
\return Handle identifying event group. Returns GTI_INVALID_OS_EVENT_GROUP_HANDLE in case of error.
*/
gti_os_event_group_handle_type gti_init_os_event_group(CHAR *event_name);

/*!
\brief Initializes an shared event group to be used with GTI API. This offers an abstraction to the underlying OS.
\param num_events Number of events in the eventgroup.
\return Handle identifying event group. Returns GTI_INVALID_OS_EVENT_GROUP_HANDLE in case of error.
*/
gti_os_event_group_handle_type gti_init_shared_os_event_group(U8 num_events);


/*!
\brief Releases a GTI OS event group to the pool for reuse. NOTICE: Deleting a event if some thread is currently 
using it (for example suspended) results in undefined behavior.
\param event_group_handle Handle identifying the OS event group, -initialized with gti_init_os_event_group() or gti_init_shared_os_event_groupi(). 
\return none
*/
void gti_release_os_event_group(gti_os_event_group_handle_type event_group_handle);


/*!
\brief Waits for an event on GTI event group
\param event_group_handle Handle identifying the OS event group, -initialized with gti_init_os_event_group() or gti_init_shared_os_event_group(). 
\param event_mask mask of events to wait for
\return mask of events received
*/
U32 gti_wait_os_events(gti_os_event_group_handle_type event_group_handle, U32 event_mask);

/*!
\brief Set one or more events on GTI event group
\param event_group_handle Handle identifying the OS event group, -initialized with gti_init_os_event_group() or gti_init_shared_os_event_group(). 
\param event_mask mask of events to set
\return none
*/
void gti_set_os_events(gti_os_event_group_handle_type event_group_handle, U32 event_mask);

/*!
\brief Retrieve one or more events on GTI event group
\param event_group_handle Handle identifying the OS event group, -initialized with gti_init_os_event_group() or gti_init_shared_os_event_group(). 
\param event_mask Mask of events to retrieve if set.
\param clear If retrieved events should be cleared.
\return none
*/
U32 gti_retrieve_pending_os_events(gti_os_event_group_handle_type event_group_handle, U32 event_mask, BOOL clear);

/** @} */ /* End of ISPEC_DRV_OS_EVENT_IF group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */


/**    
@addtogroup ISPEC_DRV_GTI_MISC Miscellaneous GTI functions
Different functions to help test interface developer.
@ingroup ISPEC_DRV_GTI 
*/
/**@{*/ /* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/*!
\brief   Puts thread/task to sleep for specified duration in milliseconds.
\return  none
*/ 
void gti_sleep(U32 duration_ms);


#ifdef TARGET

/*! 
\deprecated This function is deprecated. Please use gti_deserialize_struct() instead.

\brief Function to look up a result of a GTI function result string with following syntax: "<result1_name>=<value> <result2_name>=<value> ... <resultN_name>=<value>"
\param sname    Symbol name af the desired result
\param sformat  Formatting string for the result e.g. "%d", "%hu" or "%d[30]
\param inbuf    The result string to do the lookup in.
\param outbuf   Pointer to variable for storing value. Shall match the given formatting string.
\return Indicating success by TRUE/FALSE
*/
S32 gti_slookup(CHAR *sname, CHAR *sformat, CHAR *inbuf, void *outbuf);


/*! 
\brief Function to de-serialize a result of a GTI function result string with following syntax: "<result1_name>=<value> <result2_name>=<value> ... <resultN_name>=<value>"
\param sname    Symbol name af the desired result
\param sformat  Formatting string for the result e.g. "%d", "%hu" or "%d[30]
\param inbuf    The result string to do the lookup in.
\param outbuf   Pointer to variable for storing value. Shall match the given formatting string.
\return Pointer at end of inbuf after deserializing. NULL in case of failure.
*/
CHAR * gti_deserialize_struct(CHAR *sname, CHAR *sformat, CHAR *inbuf, void *outbuf);


/*! 
\deprecated This function is deprecated. Please use gti_vsprintf() instead.

\brief GTI variation (extensions) of well-know sprintf function.
\param outbuf       Buffer for resulting string.
\param format_str   Formatting string e.g. "%d %hu %u[200] %s[30]".
\param args_p       Pointer to argument list obtained with va_start()
\return Pointer to terminating zero "\0" of outbuf. Practical in case of concatenation.
*/ 
CHAR *gti_sprintf_va(CHAR *outbuf, const CHAR *format_str, va_list args_p );


/*! 
\brief GTI variation (extensions) of well-know vsprintf function.
\param outbuf       Buffer for resulting string.
\param format_str   Formatting string e.g. "%d %hu %u[200] %s[30]".
\param args_p       Pointer to argument list obtained with va_start()
\return Pointer to terminating zero "\0" of outbuf. Practical in case of concatenation.
*/ 
CHAR *gti_vsprintf(CHAR *outbuf, const CHAR *format_str, va_list args_p );

/*!
\brief Function to serialize a binary structure to a GTI string.       
\param p_outbuf     Buffer for resulting string
\param p_format     Formatting string e.g. "%d %hu %u[200] %s[30]".
\param p_struct     Pointer to arguments structure 
\return Pointer to terminating zero of outbuf. Practical in case of concatenation.
*/
CHAR *gti_serialize_struct(CHAR *p_outbuf, const CHAR *p_format, const void *p_struct);

/*!
\brief GTI variation (extensions) of well-know sprintf function.

\param outbuf       Buffer for resulting string.
\param format_str   Formatting string e.g. "%d %hu %u[200] %s[30]".
\param ...          var list
\return Pointer to terminating zero "\0" of outbuf. Practical in case of concatenation.
*/ 
CHAR *gti_sprintf(CHAR *outbuf, const CHAR *format_str, ... );


#else


/*      NOT IN DOXYGEN, AS ONLY FOR PC
\brief Function to look up a result of a GTI function result string with following syntax: "<result1_name>=<value> <result2_name>=<value> ... <resultN_name>=<value>"  
\param sname    Symbol name af the desired result
\param sformat  Formatting string for the result e.g. "%d", "%hu" or "%d[30]
\param inbuf    The result string to do the lookup in.
\param outbuf   Pointer to variable for storing value. Shall match the given formatting string.
\return Indicating success by TRUE/FALSE
*/
extern  IMP_CPP EXPORT_WIN S32   gti_slookup(CHAR *sname, CHAR *sformat, CHAR *inbuf, void *outbuf);


/* 
\brief Function to de-serialize a result of a GTI function result string with following syntax: "<result1_name>=<value> <result2_name>=<value> ... <resultN_name>=<value>"
\param sname    Symbol name af the desired result
\param sformat  Formatting string for the result e.g. "%d", "%hu" or "%d[30]
\param inbuf    The result string to do the lookup in.
\param outbuf   Pointer to variable for storing value. Shall match the given formatting string.
\return Pointer at end of inbuf after deserializing. NULL in case of failure.
*/
extern  IMP_CPP EXPORT_WIN CHAR * gti_deserialize_struct(CHAR *sname, CHAR *sformat, CHAR *inbuf, void *outbuf);


/*      NOT IN DOXYGEN, AS ONLY FOR PC
\brief GTI variation (extensions) of well-know sprintf function.

\param outbuf       Buffer for resulting string.
\param format_str   Formatting string e.g. "%d %hu %u[200] %s[30]".
\param ...          var list
\return Pointer to terminating zero (\0) of outbuf. Practical in case of concatenation.
*/
extern  IMP_CPP EXPORT_WIN CHAR *gti_sprintf(CHAR *outbuf, const CHAR *format_str, ... );

/*      NOT IN DOXYGEN, AS ONLY FOR PC
\brief GTI variation (extensions) of well-know sprintf() function.

\param outbuf       Buffer for resulting string.
\param format_str   Formatting string e.g. "%d %hu %u[200] %s[30]".
\param args_p       Pointer to argument list obtained with va_start()
\return Pointer to terminating zero (\0) of outbuf. Practical in case of concatenation.
*/ 
extern  IMP_CPP EXPORT_WIN CHAR *gti_sprintf_va(CHAR *outbuf, const CHAR *format_str, va_list args_p );

/* 
\brief GTI variation (extensions) of well-know vsprintf function.
\param outbuf       Buffer for resulting string.
\param format_str   Formatting string e.g. "%d %hu %u[200] %s[30]".
\param args_p       Pointer to argument list obtained with va_start()
\return Pointer to terminating zero "\0" of outbuf. Practical in case of concatenation.
*/ 
extern  IMP_CPP EXPORT_WIN CHAR *gti_vsprintf(CHAR *outbuf, const CHAR *format_str, va_list args_p );

/*
\brief Function to serialize a binary structure to a GTI string.
\param p_outbuf     Buffer for resulting string
\param p_format     Formatting string e.g. "%d %hu %u[200] %s[30]".
\param p_struct     Pointer to arguments structure 
\return Pointer to terminating zero of outbuf. Practical in case of concatenation.
*/
extern  IMP_CPP EXPORT_WIN CHAR *gti_serialize_struct(CHAR *p_outbuf, const CHAR *p_format, const void *p_struct);


#undef IMP_CPP
#undef EXPORT_WIN

#endif

/* Used for registry group handling */
typedef U32 T_GTI_DYNAMIC_GROUP_ID;     /* ID for groups */
typedef U32 T_GTI_DYNAMIC_ELEMENT_ID;   /* ID for elements */

#define GTI_INVALID_GROUP_ID 0xFFFFFFFF /* 32 bit */
#define GTI_REGISTRY_ROOT 0 /* Root element of registry */

#define REGISTRY_ROOT ROOT
#define BUILD_ENUM_ITEM(A,B) UTA_GTI_REG_ ##A  ##_ ##B 
#define GTI_REGISTRY_GROUP(parent, item) BUILD_ENUM_ITEM(parent,item), 
typedef enum {
    #include "gti_registry_cfg.h"
    GTI_MAX_DEFAULT_GROUP
} T_GTI_REGISTRY_GROUPS;
#undef GTI_REGISTRY_GROUP
#undef BUILD_ENUM_ITEM
#undef REGISTRY_ROOT


/*!
\brief Creates a dynamically expandable group
\param parent_grp_id ID for parent group
\param *p_group_name Name of the group to create
\param *p_return_gid Return var for new group ID
\return gti_retval_type
*/
gti_err_code_type gti_registry_create_dyn_group(T_GTI_DYNAMIC_GROUP_ID parent_grp_id, const CHAR *p_group_name, T_GTI_DYNAMIC_GROUP_ID *p_return_gid);

/*!
\brief Function to get dynamic group id for registry entry
\param parent_grp_id    ID for parent group. If set to GTI_INVALID_GROUP_ID the extended path is interpreted as the absolute path to group
\param *extended_path    Extended path for group (absolute path if parent_grp_id is GTI_INVALID_GROUP_ID). 
\param *return_gid Return var for dynamic group. If group is not found *gid is GTI_INVALID_GROUP_ID.
\return gti_retval_type 
*/
gti_err_code_type gti_registry_get_dyn_element_id(T_GTI_DYNAMIC_GROUP_ID parent_grp_id, const CHAR *extended_path, T_GTI_DYNAMIC_GROUP_ID *return_gid);

/*!
\brief Function to map a variable to the dynamic group table
\param parent_grp_id    ID for parent group. 
\param *var_def GTI definition for variable to map.
\param *p_data_ptr    Pointer for data to map.
\param struct_size Size of struct.
\param *return_gid Return var for the new variable group ID
\return gti_retval_type 
*/
gti_err_code_type gti_registry_map_dyn_element(T_GTI_DYNAMIC_GROUP_ID parent_grp_id, const CHAR *var_def, void *p_data_ptr, U16 struct_size, T_GTI_DYNAMIC_GROUP_ID *return_gid);


/*!
\brief Reads a dynamic variable from registry
\param parent_grp_id Id for parent group
\param *p_var_def Variable definition to fetch (I.e. "%d[10]:defjam"). Can contain an extended path which indexes from parent group (I.e. "%d[10]:sw.modules.defjam")
\param *p_return_data The pointer to return result to
\return gti_retval_type
*/
gti_err_code_type gti_registry_read_dyn_element(T_GTI_DYNAMIC_GROUP_ID parent_grp_id, const CHAR *p_var_def, void *p_return_data);


/*!
\brief Write to a dynamic variable in registry
\param parent_grp_id Id for parent group
\param *p_var_def Variable definition to write (I.e. "%d[10]:defjam"). GTI converts type when applicable
\param *p_variable The pointer to variable to write
\return gti_retval_type
*/
gti_err_code_type gti_registry_write_dyn_element(T_GTI_DYNAMIC_GROUP_ID parent_grp_id, const CHAR *p_var_def, void *p_variable);


/* Registry end */

/* Interface variable lock functions */

typedef U32 T_GTI_LOCK_HDL;

#define GTI_HDL_INVALID ((T_GTI_LOCK_HDL)(-1))

/*!
\brief Lock variable(s) by its pointer, size & lock-type
\param p_start_addr Start address of variable(s)
\param size Size of locked area
\return Returns handle for lock
*/
T_GTI_LOCK_HDL gti_lock(void *p_start_addr, U32 size);

/*!
\brief Unlocks variable(s) by earlier aquired handle
\param lock_hdl Lock handle
\return void
*/
void gti_unlock(T_GTI_LOCK_HDL lock_hdl);


/* Interface variable lock functions END */



/** @} */ /* End of ISPEC_DRV_GTI_API group <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */



#if defined(__cplusplus)
    }   /* extern "C" */
#endif

#endif /* gti.h */
