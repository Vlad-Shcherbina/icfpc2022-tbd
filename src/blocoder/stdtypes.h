#ifndef _stdtypes_h
#define _stdtypes_h

/*****************************************************************************
 * File        : stdtypes.h
 * Author      : Johan K”pman
 * Description : Standard types. Preferably this file should be included by
 *               all other sourcefiles.
 *****************************************************************************/
#include <stdint.h>
#include <stdlib.h>

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef CMOD_OS_VXWORKS
#include "base_types.h"
#endif

#ifndef CMOD_COMPILER_VISUALC
#include <sys/types.h>
#endif


#define MIN(a,b) ((a)<=(b)?(a):(b))
#define MAX(a,b) ((a)>=(b)?(a):(b))

#ifndef MAXINT
#define MAXINT 2147483647
#endif

#ifndef MININT
#define MININT -2147483648
#endif

typedef int Int;
typedef double Double;

typedef int            Bool;

typedef unsigned char  UChar;
typedef signed char    SChar;
typedef char    Char;

typedef unsigned short UShort;
typedef signed short   SShort;
typedef short   Short;

typedef unsigned int   UInt;
typedef signed int     SInt;
typedef short int      Int16;
typedef long int       Int32;
#ifdef CMOD_COMPILER_GCC
typedef long long int  Int64;
typedef unsigned long long UInt64;
#endif
typedef unsigned long long U64;

typedef unsigned short UInt16;
typedef unsigned long  UInt32;

typedef unsigned long  ULong;
typedef signed long    SLong;
typedef signed long    Long;

typedef void*          Ref;

typedef void           Void;

typedef void  (*Proc)(void *);
typedef void* (*Func)(void *);

typedef uint8_t	u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef void *(CloneFunc)(const void *);
typedef Bool (EqualFunc)(const void *, const void *);
typedef int (CompFunc)(const void *, const void *);
typedef int (SortFunc)(const void *);
typedef void (DestFunc)(void *);
typedef void (DispFunc)(const void *, int);
#define DisplayFunc DispFunc
typedef unsigned int (HashFunc)(const void *);

#endif
