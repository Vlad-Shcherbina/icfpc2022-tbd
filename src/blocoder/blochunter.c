
#include "bitmap.h"
#include "blochunter.h"
#include "cmod.h"
#include "isl.h"
#include "memory.h"

/*--------------------------------------------------------------------------*/
/*
void BlocHunter_Init(BlocHunter *blochunter, BlocHunter *blochunter_org) {

	if (blochunter_org == NULL) {
		blochunter->origin_x = 0;
		blochunter->origin_y = 0;
		blochunter->bm = BitMap_Create();
		blochunter->isl_len = 0;
		blochunter->best_so_far = ISL_Create();
		blochunter->error = 0;
	} else {
		blochunter->origin_x = blochunter_org->origin_x;
		blochunter->origin_y = blochunter_org->origin_y;
		blochunter->bm = BitMap_Clone(blochunter_org->bm);
		blochunter->isl_len = blochunter_org->isl_len;
		blochunter->best_so_far = ISL_Clone(blochunter_org->best_so_far);
		blochunter->error = blochunter_org->error;
	}
}
*/
/*--------------------------------------------------------------------------*/

BlocHunter *BlocHunter_Create(const BitMap *master, int x1, int y1, int x2, int y2) {
	BlocHunter *bh;

	bh = Memory_Reserve(1, BlocHunter);
    bh->origin_x = x1;
    bh->origin_y = y1;
    bh->bm = BitMap_GetBitMap(master, x1, x2, y1, y2);
    bh->isl_alloc = 256;
    bh->isl = Memory_Reserve(bh->isl_alloc, ISL);
    bh->isl_len = 0;
    bh->error = -0.0;
    
	return bh;
}

/*--------------------------------------------------------------------------*/
/*
BlocHunter *BlocHunter_Clone(BlocHunter *blochunter) {
	BlocHunter *blochunter2;

	blochunter2 = Memory_Reserve(1, BlocHunter);
	BlocHunter_Init(blochunter2, blochunter);

	return blochunter2;
}
*/
/*--------------------------------------------------------------------------*/

void BlocHunter_Restore(BlocHunter *blochunter) {
	BitMap_Destroy(blochunter->bm);
    Memory_Free(blochunter->isl, ISL);
}

/*--------------------------------------------------------------------------*/

void BlocHunter_Destroy(BlocHunter *blochunter) {
	BlocHunter_Restore(blochunter);
	Memory_Free(blochunter, BlocHunter);
}

/*--------------------------------------------------------------------------*/

