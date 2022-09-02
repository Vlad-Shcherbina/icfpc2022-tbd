#ifndef _xodget_h
#define _xodget_h

#include "cmod.h"
#include "intcoord.h"

/* TODO: siblings! */

typedef struct {
	int   id;
   IntCoord x;
   IntCoord y;
	int   data;
   void *ptr;
} Xodget;

Xodget *Xodget_Create(int id, IntCoord x, IntCoord  y);
Xodget *Xodget_Create2(int id, int x1, int y1, int x2, int y2);
Xodget *Xodget_Clone(Xodget *xodget);
int     Xodget_CompareId(const Xodget *xt1, const Xodget *xt2);
#define Xodget_Id(xodget) ((xodget)->id)
#define Xodget_SetId(xodget, i) (((xodget)->id) = (i))
#define Xodget_X(xodget) ((xodget)->x)
#define Xodget_SetX(xodget, xx) (((xodget)->x) = (xx))
#define Xodget_Y(xodget) ((xodget)->y)
#define Xodget_SetY(xodget, yy) (((xodget)->y) = (yy))
#define Xodget_Data(xodget) ((xodget)->data)
#define Xodget_SetData(xodget, d) (((xodget)->data) = (d))
#define Xodget_Ptr(xodget) ((xodget)->ptr)
#define Xodget_SetPtr(xodget,p) ((xodget)->ptr = (p))
#define Xodget_LeftX(xo) (IntCoord_X((xo)->x))
#define Xodget_RightX(xo) (IntCoord_X((xo)->y))
#define Xodget_UpperY(xo) (IntCoord_Y((xo)->x))
#define Xodget_LowerY(xo) (IntCoord_Y((xo)->y))
Bool    Xodget_IsInside(Xodget *xodget, int x, int y);
void    Xodget_Destroy(Xodget *xodget);
void    Xodget_Display(Xodget *xodget, int n);

#ifdef CMOD_NOMAKEFILE
#include "xodget.c"
#endif
#endif

