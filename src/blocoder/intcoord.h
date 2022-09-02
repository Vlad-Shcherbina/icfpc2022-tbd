
#ifndef _intcoord_h
#define _intcoord_h

#include "cmod.h"

/*
IntCoords capture values 0..32767, 0..32767
bits 31 and 15 are supposed to be 0
*/

typedef unsigned int IntCoord;

typedef Bool (ValidCoordFunc)(const void *, IntCoord, IntCoord, int);

//#define INTCOORD_ASM

#define INTCOORD_MASK 0x7fff7fff

//#define IntCoord_Construct(x,y)
//   __asm__ ("shll $0x10, %0\n movw %1, %0\n" : "0" : "0" (x), "1" (y) )
//asm ("shll $0x10, %0\n movw %1, %0\n" : "0" : "0" (x), "1" (y) )
//#define IntCoord_ConstructOld(x,y) ((((x)<<16) | (y&65535)) & INTCOORD_MASK)
#define IntCoord_ConstructOld(x,y) (((x)<<16) + (y))
#define IntCoord_Construct IntCoord_ConstructOld
#define IntCoord_X(c) ((c)>>16)
#define IntCoord_Y(c) ((c)&65535)
#define IntCoord_SetX(i,x) (i = IntCoord_Construct( (x), IntCoord_Y(i) ))
#define IntCoord_SetY(i,y) (i = IntCoord_Construct( IntCoord_X(i), (y) ))

/* 0 is here, 1..8 are directions */
extern int std_dintcoord[];
extern int std_nintcoord[];
//extern IntCoord intcoord_d

/* Assumes UP decreseas Y coordinates (like CharMatrix and ShortMatrix do) */
#define INTCOORD_RIGHT IntCoord_Construct(1,0)
#define INTCOORD_UPRIGHT IntCoord_ConstructOld(1,-1)
#define INTCOORD_UP IntCoord_ConstructOld(0,-1)
#define INTCOORD_UPLEFT IntCoord_ConstructOld(-1,-1)
#define INTCOORD_LEFT IntCoord_ConstructOld(-1,0)
#define INTCOORD_DOWNLEFT IntCoord_ConstructOld(-1,1)
#define INTCOORD_DOWN IntCoord_ConstructOld(0,1)
#define INTCOORD_DOWNRIGHT IntCoord_ConstructOld(1,1)

#define INTCOORD_EAST INTCOORD_RIGHT
#define INTCOORD_NORTH INTCOORD_UP
#define INTCOORD_WEST INTCOORD_LEFT
#define INTCOORD_SOUTH INTCOORD_DOWN

Bool    IntCoord_IsInBox(IntCoord c, IntCoord box); /* Is c in box (0,0)--(x,y) */
//inline IntCoord IntCoord_Construct(int x, int y);
IntCoord IntCoord_Add(IntCoord a, IntCoord b);
IntCoord IntCoord_Sub(IntCoord a, IntCoord b);
int IntCoord_Norm2(IntCoord c);
int     IntCoord_Distance2(IntCoord c1, IntCoord c2);
IntCoord IntCoord_Abs(IntCoord c);
int      IntCoord_ManhattanDist(IntCoord c1, IntCoord c2);

/* Return value is new coordinate! Loops including the end coordinates of dst */
IntCoord IntCoord_Iterate(IntCoord cur, IntCoord dst);
void     IntCoord_Display(IntCoord c);

#ifdef CMOD_NOMAKEFILE
#include "intcoord.c"
#endif

#endif
