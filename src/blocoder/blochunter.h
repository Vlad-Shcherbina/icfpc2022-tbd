#ifndef _blochunter_h
#define _blochunter_h

#include "bitmap.h"
#include "cmod.h"
#include "isl.h"

typedef struct {
	int     origin_x;
	int     origin_y;
	BitMap *bm;
	int     isl_len;
    int 	isl_alloc;
	ISL    *isl;
	double  error;
} BlocHunter;

BlocHunter *BlocHunter_Create(const BitMap *master, int x1, int y1, int x2, int y2);
BlocHunter *BlocHunter_Clone(BlocHunter *BlocHunter);
#define     BlocHunter_OriginX(blochunter) ((blochunter)->origin_x)
#define     BlocHunter_SetOriginX(blochunter, o) (((blochunter)->origin_x) = (o))
#define     BlocHunter_OriginY(blochunter) ((blochunter)->origin_y)
#define     BlocHunter_SetOriginY(blochunter, o) (((blochunter)->origin_y) = (o))
#define     BlocHunter_Bm(blochunter) ((blochunter)->bm)
#define     BlocHunter_SetBm(blochunter, b) (((blochunter)->bm) = (b))
#define     BlocHunter_IslLen(blochunter) ((blochunter)->isl_len)
#define     BlocHunter_SetIslLen(blochunter, i) (((blochunter)->isl_len) = (i))
#define     BlocHunter_ISL(blochunter) ((blochunter)->isl)
#define     BlocHunter_Error(blochunter) ((blochunter)->error)
#define     BlocHunter_SetError(blochunter, e) (((blochunter)->error) = (e))
void        BlocHunter_Destroy(BlocHunter *blochunter);

#ifdef CMOD_NOMAKEFILE
#include "blochunter.c"
#endif
#endif

