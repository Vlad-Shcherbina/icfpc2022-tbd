
#include "cmod.h"
#include "intcoord.h"
#include "stdfunc.h"
#include "stdtypes.h"

int std_dintcoord[] = {0, 
   IntCoord_ConstructOld( 1, 0),
   IntCoord_ConstructOld( 1,-1),
   IntCoord_ConstructOld( 0,-1),
   IntCoord_ConstructOld(-1,-1),
   IntCoord_ConstructOld(-1, 0),
   IntCoord_ConstructOld(-1, 1),
   IntCoord_ConstructOld( 0, 1),
   IntCoord_ConstructOld( 1, 1)};

int std_nintcoord[] = {
   IntCoord_ConstructOld(-1,-1),
   IntCoord_ConstructOld( 1, 0),
   IntCoord_ConstructOld( 1, 0),
   IntCoord_ConstructOld( 0, 1),
   IntCoord_ConstructOld( 0, 1),
   IntCoord_ConstructOld(-1, 0),
   IntCoord_ConstructOld(-1, 0),
   IntCoord_ConstructOld( 0,-1),
   IntCoord_ConstructOld( 0,-1)};

/*-------------------------------------------------------------------------*/
/*
inline IntCoord IntCoord_Construct(int x, int y) {
   IntCoord ret;
   asm("\t"
      "shl eax, 0x10\n\t"
      "mov ax, dx\n" 
      : "=a" (ret)
      : "a" (x), "d" (y));
   return ret;
}
*/
IntCoord IntCoord_Add(IntCoord a, IntCoord b) {
   return (a+b)&INTCOORD_MASK;
}

/*-------------------------------------------------------------------------*/

IntCoord IntCoord_Sub(IntCoord a, IntCoord b) {
   return (a-b)&INTCOORD_MASK;
}

/*-------------------------------------------------------------------------*/

#ifdef INTCOORD_ASM
static inline Bool IntCoord_IsInBox(IntCoord c, IntCoord box) {
   asm(
      : "=0"
      : "0"
      );
}
#else
Bool IntCoord_IsInBox(IntCoord c, IntCoord box) {
//   return ((box - c) & 0x80008000) == 0;
   return c>=0 && IntCoord_X(c)<=IntCoord_X(box) && IntCoord_Y(c)<=IntCoord_Y(box);
}
#endif

/*-------------------------------------------------------------------------*/

IntCoord IntCoord_Abs(IntCoord c) {
   if ((c & 0x40000000) != 0) c ^= 0x7fff0000;
   if ((c & 0x00004000) != 0) c ^= 0x00007fff;
   return c;
}

/*-------------------------------------------------------------------------*/

int IntCoord_ManhattanDist(IntCoord c1, IntCoord c2) {
    return abs(IntCoord_X(c1) - IntCoord_X(c2)) + abs(IntCoord_Y(c1) - IntCoord_Y(c2));
}

/*-------------------------------------------------------------------------*/

inline int IntCoord_Norm2(IntCoord c) {
   int res;

   asm("\t"
      "movl %%eax, %%edx\n\t"
      "xorl $0x00007fff, %%eax\n\t"
      "mull %%edx\n\t"
      "subl %%eax, %%edx\n"
      :"=d" (res)
      :"a" (c)
      );
   
/*
   asm("\t"
      "xorl %%ebx, %%ebx\n\t"
      "movl %%eax, %%edx\n\t"
      "mov %%ax, %%bx\n\t"
      "subl %%ebx, %%eax\n\t"
      "subl %%ebx, %%eax\n\t"
      "imull %%edx\n\t"
      "subl %%eax, %%edx\n"
      :"=d" (res)
      :"a" (c)
      :"%ebx"
      );
*/
   return res;
}

int IntCoord_Distance2(IntCoord c1, IntCoord c2) {
   int x,y;
   y = IntCoord_Y(c1)-IntCoord_Y(c2);
   x = IntCoord_X(c1)-IntCoord_X(c2);
   return SQR(x) + SQR(y);
}

/*-------------------------------------------------------------------------*/

IntCoord IntCoord_Iterate(IntCoord cur, IntCoord dst) {
	cur += 65536;
   if (cur > dst) {
      if (IntCoord_Y(cur) < IntCoord_Y(dst)) return IntCoord_Y(cur)+1;
      else return 0;
	} else
		return cur;
}

/*-------------------------------------------------------------------------*/

void IntCoord_Display(IntCoord c) { printf("(%i,%i) ", IntCoord_X(c), IntCoord_Y(c)); }

/*-------------------------------------------------------------------------*/

void IntCoord_Test() {
   IntCoord box = IntCoord_Construct(3, 3);
   IntCoord c = 0;
   do {
      IntCoord_Display(c); printf("\n");
   } while ((c = IntCoord_Iterate(c, box)));
   
   c = IntCoord_Construct(0,0); IntCoord_Display(c);
   if (IntCoord_IsInBox(c, box)) printf(" is in box (3,3)\n"); 
   else printf(" is NOT in box (3,3)\n"); 
   
   c = IntCoord_Construct(3,0); IntCoord_Display(c);
   if (IntCoord_IsInBox(c, box)) printf(" is in box (3,3)\n"); 
   else printf(" is NOT in box (3,3)\n"); 

   c = IntCoord_Construct(0,3); IntCoord_Display(c);
   if (IntCoord_IsInBox(c, box)) printf(" is in box (3,3)\n"); 
   else printf(" is NOT in box (3,3)\n"); 

   c = IntCoord_Construct(2,2); IntCoord_Display(c);
   if (IntCoord_IsInBox(c, box)) printf(" is in box (3,3)\n"); 
   else printf(" is NOT in box (3,3)\n"); 

   c = IntCoord_Construct(2,0); IntCoord_Display(c);
   if (IntCoord_IsInBox(c, box)) printf(" is in box (3,3)\n"); 
   else printf(" is NOT in box (3,3)\n"); 

   c = IntCoord_Construct(0,2); IntCoord_Display(c);
   if (IntCoord_IsInBox(c, box)) printf(" is in box (3,3)\n"); 
   else printf(" is NOT in box (3,3)\n"); 

   c = IntCoord_Construct(3,3); IntCoord_Display(c);
   if (IntCoord_IsInBox(c, box)) printf(" is in box (3,3)\n"); 
   else printf(" is NOT in box (3,3)\n"); 
   
   c = IntCoord_Construct(4,0); IntCoord_Display(c);
   if (IntCoord_IsInBox(c, box)) printf(" is in box (3,3)\n"); 
   else printf(" is NOT in box (3,3)\n"); 

   c = IntCoord_Construct(0,4); IntCoord_Display(c);
   if (IntCoord_IsInBox(c, box)) printf(" is in box (3,3)\n"); 
   else printf(" is NOT in box (3,3)\n"); 
}


