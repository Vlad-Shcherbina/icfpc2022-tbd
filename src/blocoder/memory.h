#ifndef _memory_h
#define _memory_h

/************************************************************************
 * File         : memory.h
 * Author(s)    : Johan Köpman, Tapani
 *               
 * Original     : 1997-10-17
 * Revidated    : 2000-09-19
 * Description  : A very simple memory handler.
 *               
 ************************************************************************/

/* -----------------------------------------------------------------------
 * Includes:                                                            */

#include "cmod.h"
#include "stdtypes.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef CMOD_DEBUG_MEMORY
#define Memory_Reserve(a,b) (b *)Memory_DebugAlloc2((a)*sizeof(b), #b, __FILE__, __LINE__)
#define Memory_Free(a,b) Memory_DebugDealloc2((a), #b, __FILE__, __LINE__)
#define Memory_Realloc(a,b,c) (c *)Memory_DebugRealloc(a,b,#c,__FILE__, __LINE__)
#define Memory_ReserveSize(a,b) (b *)Memory_DebugAlloc2(a, #b, __FILE__, __LINE__)
#define Memory_Calloc(a,b) (b *)Memory_DebugCalloc((a)*sizeof(b), #b, __FILE__, __LINE__)

/* Initializes / restores memory debug functions. Must be called first / last! */ 
void  Memory_DebugInit(FILE *f);
void  Memory_DebugRestore(void);

/* The 'name' is a String (15 characters long) that describes what type of "object" 
   that was allocated. If the same String is given to DebugDealloc() the memory handler will keep
   track of allocated and deallocated objects. This will help the programmer finding memory leakage. */

void *Memory_DebugRealloc(void *ptr, int size, char *name, char *fname, int row);
void *Memory_DebugAlloc2(int size, char* name, char *fname, int row);
void *Memory_DebugCalloc(int size, char* name, char *fname, int row);

/* Deallocates memory allocated by Memory_DebugAlloc(). Make sure that the
   'name' are the same TString that you used for allocating the memory. */
void  Memory_DebugDealloc2(void *mem, char* name, char *fname, int row);
int   Memory_DebugNofAllocs(void);
int   Memory_DebugNofDeallocs(void);
int   Memory_DebugCurrentUsage(void); // returns nof bytes used now
void *Memory_Alloc(int size); 
void *Memory_NormalAlloc(int size); // as malloc
void *Memory_NormalRealloc(void *ptr, int size); // as realloc

void  Memory_Dealloc(void *mem);
void  Memory_NormalDealloc(void *mem); // as free

/* Copies 'n' bytes of memory from 'src' to 'dest'. It returns 'dest'. */
void *Memory_Copy(void *dest, const void *src, int n);

/* Copies n bytes from src to dest, but with correct overlap */
void *Memory_Move(void *dest, const void *src, int n);

void  Memory_Set(void *dest, int n, char data);
void  Memory_Clear(void *dest, int n);

void *Memory_AllocAligned(int aligned, int size);
#else
 #include <malloc.h>
 #include <string.h>
 #define Memory_Reserve(a,b) (b *)malloc((a)*sizeof(b))
 #define Memory_Free(a,b) free(a)
 #define Memory_Realloc(a,b,c) (c *)realloc(a,b)
 #define Memory_ReserveSize(a,b) (b *)malloc(a)
 #define Memory_Copy(d, s, n) memcpy(d,s,n)
 #define Memory_Move(d, s, n) memmove(d,s,n)
 #define Memory_Set(d, n, data) memset(d,data,n)
 #define Memory_Clear(d, n) memset(d,0,n)
 #define Memory_NormalAlloc malloc
 #define Memory_NormalDealloc free
 #define Memory_NormalRealloc realloc
 #define Memory_Calloc(a,b) (b *)calloc((a), sizeof(b))
 #define Memory_AllocAligned(a,s) memalign((a), (s));
/*
 #define Memory_Reserve(a,b) (b *)Memory_Alloc((a)*sizeof(b))
 #define Memory_Free(a,b) Memory_Dealloc(a)
// #define Memory_Free(a,b) ;
 #define Memory_Realloc(a,b,c) (c *)Memory_NormalRealloc(a,b)
 #define Memory_ReserveSize(a,b) (b *)Memory_Alloc(a)
*/
 #define Memory_DebugInit(f) ;
 #define Memory_DebugRestore() ;
#endif

#define Memory_AlignedAlloc Memory_AllocAligned
#define Memory_Zero(p) Memory_Clear(p, sizeof(*p));

#ifdef CMOD_NOMAKEFILE
#include "memory.c"
#endif

#endif

