/* =========================================================================
** Copyright (C) 2011-2013 Intel Mobile Communications GmbH
** 
**      Sec Class: Intel Confidential (IC)
** 
** =========================================================================
**
** =========================================================================
**
** This document contains proprietary information belonging to Intel Comneon.
** Passing on and copying of this document, use and communication of its
** contents is not permitted without prior written authorisation.
**
** =========================================================================
**
** Revision Information :
**   $File name:  /stack-interface/globals/bastypes.h $
**   $CC-Version: .../oint_if_cpstack_03/7 $
**   $Date:       2013-06-25    8:18:12 UTC $
**
** =========================================================================
*/

/* ===========================================================================
**
** History:
**
** Date		Author          Comment
** ---------------------------------------------------------------------------
** 2005-09-30  nourbaks	    merge for N7, xnear, int32 long int
** 2004-12-13  nourbaks Added SDTENV to avoid compiler error due to long long
** 2004-02-13  schmulbo CN1100002836 avoids compiler warning "unused parameter"
** 2004-01-21  MZI          CN1100002648 LONGLONG_EMUL
** 2003-12-16  GHI          CN1100002477 avoids ANSI C compiler erors
**                          gcc compiler switch -ansi must be used!
** 2003-11-27  GHI          CN1100002320 bastypes.h: near, far, bit
** 2003-11-18  GHI          CN1100002187 Add STD_OK and STD_ERR to bastypes.h
** 2003-10-21  MZI          Added ullong.
** 03-07-21    WMA          created: merged bastypes from different vobs
** ===========================================================================
*/
/*
** ===========================================================================
**
**				 DEFINITIONS
**
** ===========================================================================
*/
#ifndef BASTYPES_H

#define BASTYPES_H 1

#if defined(__GNUC__) && !defined(__ARMCC_VERSION)
#include <sys/types.h>
#endif

#if !defined(__ANDROID__)
#define near
#define xnear
#define far
#define huge
#define bit
#endif /* __ANDROID__ */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#define STD_OK    0

#ifndef STD_ERR
#define STD_ERR (-1)
#endif

/*
** ===========================================================================
**
**				   TYPES
**
** ===========================================================================
*/


/* 
** Data types as defined by SDHB
** =============================
*/
#ifndef __S8_defined
typedef signed char         S8;
#define __S8_defined
#endif
#ifndef __S16_defined
typedef signed short        S16;
#define __S16_defined
#endif
#ifndef __U8_defined
typedef unsigned char       U8;
#define __U8_defined
#endif
#ifndef __U16_defined
typedef unsigned short      U16;
#define __U16_defined
#endif
#ifndef __S32_defined
typedef signed int          S32;
#define __S32_defined
#endif
#ifndef __U32_defined
typedef unsigned int        U32;
#define __U32_defined
#endif
#ifndef __BOOL_defined
typedef int                 BOOL;               /* TRUE or FALSE */
#define __BOOL_defined
#endif
#ifndef __CHAR_defined
typedef char                CHAR;               /* for char types */	
#define __CHAR_defined
#endif

#ifndef __boolean_defined
typedef unsigned char       boolean;     /* Boolean value type */
#define __boolean_defined
#endif
#ifndef __qword_defined
typedef unsigned long       qword;
#define __qword_defined
#endif
#ifndef __uint64_defined
typedef unsigned long long  uint64;
#define __uint64_defined
#endif
#ifndef __uint32_defined
typedef unsigned long int   uint32;      /* Unsigned 32 bit value */
#define __uint32_defined
#endif
#ifndef __uint16_defined
typedef unsigned short      uint16;      /* Unsigned 16 bit value */
#define __uint16_defined
#endif
#ifndef __uint8_defined
typedef unsigned char       uint8;       /* Unsigned 8  bit value */
#define __uint8_defined
#endif
#ifndef __byte_defined
typedef unsigned char       byte;        /* Unsigned 8  bit value type. */
#define __byte_defined
#endif
#ifndef __word_defined
typedef unsigned short      word;        /* Unsinged 16 bit value type. */
#define __word_defined
#endif
#ifndef __dword_defined
typedef unsigned long       dword;       /* Unsigned 32 bit value type. */
#define __dword_defined
#endif
#ifndef __UINT16_defined
typedef uint16              UINT16;
#define __UINT16_defined
#endif

/* 
** Data types as used in generated SDL code
** ========================================
*/
#ifndef __ubyte_defined
typedef U8      ubyte;              /*           0 to +255        */
#define __ubyte_defined
#endif
#ifndef __sbyte_defined
typedef S8      sbyte;              /*        -128 to +127        */
#define __sbyte_defined
#endif
#ifndef __sshort_defined
typedef S16     sshort;             /*      -32768 to +32767      */
#define __sshort_defined
#endif
#ifndef __slong_defined
typedef S32     slong;              /* -2147483648 to +2147483647 */
#define __slong_defined
#endif
/* already defined by SunOS headers as used by SunOS cc and GNU gcc compilers */
#if !defined(__sun) && !defined(__linux) && !defined(__unix)
#ifndef __ushort_defined
typedef U16     ushort;
#define __ushort_defined
#endif
#ifndef __ulong_defined
typedef U32     ulong;    
#define __ulong_defined
#endif
#ifndef __uint_defined
typedef U32     uint;
#define __uint_defined
#endif
#ifndef __sint_defined
typedef S32     sint;
#define __sint_defined
#endif
#endif


/*
** Data defines as used by DWD driver
** ==================================
*/
#define int16   short
#define int32   long int

/*
** Microsoft Visual C++
** ====================
*/
#if defined(_WIN32) || defined(_lint)
#undef shuge
#define shuge
#endif

#ifndef __UINT8_defined
typedef U8       UINT8;
#define __UINT8_defined
#endif
#ifndef __UINT16_defined
typedef U16      UINT16;
#define __UINT16_defined
#endif


/*
** For non-ARM compiler builds, the align macro is not recognised so define here
** to allow files that are included for the hardware to be used also on
** non-hardware builds
**
*/
#if !defined (__arm)
#define __align(x)
#endif


/* 
** Definition of 64 bit data types (non-ANSI/ISO C constructs):
** ============================================================
*/
#if defined (_MSC)

/* Microsoft Visual C++ */
#ifndef __S64_defined
typedef signed __int64    S64;
#define __S64_defined
#endif
#ifndef __U64_defined
typedef unsigned __int64  U64;
#define __U64_defined
#endif
#ifndef __ullong_defined
typedef __int64 ullong;
#define __ullong_defined
#endif
#ifndef __sllong_defined
typedef __int64 sllong;
#define __sllong_defined
#endif
#define  LL(a) (a)
#define ULL(a) (a)

/* applicable for:
 * - ARM compiler
 * - GCC compiler with C99 standard
 * - GCC compiler with C90 standard and no STRICT_ANSI
 * not applicable for:
 * - GCC compiler with C90 standard + STRICT_ANSI
 * - SUN cc in strict ANSI mode 
 */
#elif defined (__arm) || \
      (defined (__GNUC__) && !defined (__STRICT_ANSI__)) || \
      ((defined (__GNUC__) && ( __STDC_VERSION__ )) && __STDC_VERSION__ >= 199901L) || \
      (defined (SUN5_ANSICC) && (__STDC__ == 0) && !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE))

#ifndef __S64_defined
typedef signed   long long  S64;
#define __S64_defined
#endif
#ifndef __U64_defined
typedef unsigned long long  U64;
#define __U64_defined
#endif
#ifndef __ullong_defined
typedef unsigned long long ullong;
#define __ullong_defined
#endif
#ifndef __sllong_defined
typedef   signed long long sllong;
#define __sllong_defined
#endif
#define  LL(a) (a##LL)  /* eg. LL(0) := 0LL */
#define ULL(a) (a##ULL)

/* default: no 64 bit data types supported in strict ANSI C mode */
#else

#define LONGLONG_EMUL 1
#ifndef __ullong_defined
typedef struct
{
    U32  hi;
    U32  lo;
} ullong;
#define __ullong_defined
#endif
#ifndef __sllong_defined
typedef struct
{
    S32  hi;
    S32  lo;
} sllong;
#define __sllong_defined
#endif

#endif


#ifndef MS_UNUSED_VAR
/*
** This macro is used to avoid compiler warnings and has no functional impact.
**
** However this macro causes 2 lint warnings, which are explained below (extract 
** from lint manual). These lint warnings are explicitly suppressed via additional
** lint options as embedded comments inside this macro.
**
** - LINT warning 506 - Constant value Boolean
**   A Boolean, i.e., a quantity found in a context that requires a Boolean such
**   as an argument to && or || or an if() or while() clause or ! was found to be
**   a constant and hence will evaluate the same way each time.
**
** - LINT warning 550 - Symbol 'Symbol' (Location) not accessed
**   A variable (local to some function) was not accessed. This means that the value
**   of a variable was never used. Perhaps the variable was assigned a value but was
**   never used. Note that a variable's value is not considered accessed by autoincrementing
**   or autodecrementing unless the autoincrement/decrement appears within a larger
**   expression which uses the resulting value. The same applies to a construct of the
**   form: var += expression. If an address of a variable is taken, its value is assumed
**   to be accessed. An array, struct or union is considered accessed if any portion
**   thereof is accessed.
**
*/
#define MS_UNUSED_VAR(a) /*lint -e(506) --e{550} */ { if(sizeof(a)){} }
#endif

#endif /* BASTYPES_H */
