
#include "cmod.h"
#include "intcoord.h"
#include "memory.h"
#include "xodget.h"

/*--------------------------------------------------------------------------*/

void Xodget_Init(Xodget *xodget, Xodget *xodget_org) {

	if (xodget_org == NULL) {
		xodget->id = 0;
		xodget->x = 0;
		xodget->y = 0;
		xodget->data = 0;
      xodget->ptr = NULL;
	} else {
		xodget->id = xodget_org->id;
		xodget->x = xodget_org->x;
		xodget->y = xodget_org->y;
		xodget->data = xodget_org->data;
      xodget->ptr = xodget_org->ptr;
	}
}

/*--------------------------------------------------------------------------*/

Xodget *Xodget_Create(int id, IntCoord x, IntCoord y) {
	Xodget *xodget;

	xodget = Memory_Reserve(1, Xodget);
	Xodget_Init(xodget, NULL);
   xodget->id = id;
   xodget->x = x;
   xodget->y = y;

	return xodget;
}

/*--------------------------------------------------------------------------*/

Xodget *Xodget_Create2(int id, int x1, int y1, int x2, int y2) {
	Xodget *xodget;

	xodget = Memory_Reserve(1, Xodget);
	Xodget_Init(xodget, NULL);
   xodget->id = id;
   xodget->x = IntCoord_Construct(x1, y1);
   xodget->y = IntCoord_Construct(x2, y2);

	return xodget;
}

/*--------------------------------------------------------------------------*/

Xodget *Xodget_Clone(Xodget *xodget) {
	Xodget *xodget2;

	xodget2 = Memory_Reserve(1, Xodget);
	Xodget_Init(xodget2, xodget);

	return xodget2;
}

/*--------------------------------------------------------------------------*/

int Xodget_CompareId(const Xodget *xt1, const Xodget *xt2) { return xt1->id - xt2->id; }

/*--------------------------------------------------------------------------*/

Bool Xodget_IsInside(Xodget *xodget, int x, int y) {
   return (x>=(signed)IntCoord_X(xodget->x) && x<(signed)IntCoord_X(xodget->y) && y>=(signed)IntCoord_Y(xodget->x) && y<(signed)IntCoord_Y(xodget->y));
}

/*--------------------------------------------------------------------------*/

void Xodget_Restore(Xodget *xodget) { 
   if (xodget->ptr != NULL) Memory_Free(xodget->ptr, void);
}

/*--------------------------------------------------------------------------*/

void Xodget_Destroy(Xodget *xodget) {
	Xodget_Restore(xodget);
	Memory_Free(xodget, Xodget);
}

/*--------------------------------------------------------------------------*/

void Xodget_Display(Xodget *xo, int n) {
   int i;
   for (i=0; i<n; i++) printf(" ");
   printf("id=%i (%i, %i) - (%i, %i)\n", xo->id, 
      IntCoord_X(xo->x), IntCoord_Y(xo->x), IntCoord_X(xo->y), IntCoord_Y(xo->y));
}

/*--------------------------------------------------------------------------*/

