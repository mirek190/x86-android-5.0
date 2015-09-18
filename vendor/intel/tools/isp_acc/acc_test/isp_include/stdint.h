#ifndef __STDINT_H_INCLUDED__
#define __STDINT_H_INCLUDED__

/* stdint.h for ISP/SP code */
typedef unsigned char      uint8_t;
typedef signed char        int8_t;

typedef unsigned short     uint16_t;
typedef signed short       int16_t;

typedef unsigned int       uint32_t;
typedef signed int         int32_t;

typedef unsigned long long uint64_t;
typedef signed long long   int64_t;

#define UINT8_MAX          0xff
#define UINT16_MAX         0xffff
#define UINT32_MAX         0xffffffffUL
#define UINT64_MAX         0xffffffffffffffffULL

#define INT8_MAX           0x7f
#define INT16_MAX          0x7fff
#define INT32_MAX          0x7fffffffL
#define INT64_MAX          0x7fffffffffffffffLL

#define INT8_MIN           0x80
#define INT16_MIN          0x8000
#define INT32_MIN          0x80000000L
#define INT64_MIN          0x8000000000000000LL

#endif /* __STDINT_H_INCLUDED__ */
