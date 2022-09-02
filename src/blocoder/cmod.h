
#ifndef _cmod_h
#define _cmod_h

/* Optimization defines */
#define CMOD_OPTIMIZE_SPEED
//#define CMOD_OPTIMIZE_SIZE
//#define CMOD_OPTIMIZE_NOMAKEFILE

/* Debug defines */
//#define CMOD_DEBUG_MEMORY
//#define CMOD_MEMORY_DEBUG_DUMPUNALLOCATED
//#define CMOD_DEBUG_MEMORY_ALL
#define CMOD_DEBUG_MISC

/* Architecture defines*/
#define CMOD_ARCHITECTURE_X86
//#define CMOD_ARCHITECTURE_SPARC
//#define CMOD_ARCHITECTURE_MIPS
//#define CMOD_ARCHITECTURE_PPC

#if defined(CMOD_ARCHITECTURE_X86) || defined(CMOD_ARCHITECTURE_ALPHA)
#define CMOD_ARCHITECTURE_BIGENDIAN 
#else
#if defined(CMOD_ARCHITECTURE_SPARC) || defined(CMOD_ARCHITECTURE_PPC)
#define CMOD_ARCHITECTURE_LITTLEENDIAN 
#else
#error Unknown architecture.
#endif
#endif

/* Operating system / platform defines*/
#define CMOD_OS_LINUX
//#define CMOD_OS_SOLARIS
//#define CMOD_OS_WINDOWS
//#define CMOD_OS_VXWORKS

#if (defined(CMOD_OS_LINUX) || defined(CMOD_OS_SOLARIS)) 
#define CMOD_OS_UNIX
#endif

/* Compiler defines */
#define CMOD_COMPILER_GCC
//#define CMOD_COMPILER_WATCOM
//#define CMOD_COMPILER_WORKSHOP

//#define CMOD_ALLOW_EXIT

#ifdef CMOD_COMPILER_GCC
 #define CMOD_EMPTY_ARRAY 0
#else
 #define CMOD_EMPTY_ARRAY
#endif

#include "stdtypes.h"

#endif

