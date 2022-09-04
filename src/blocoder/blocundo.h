
#ifndef _blocundo_h
#define _blocundo_h

#include "bitmap.h"
#include "blockdata.h"
#include "cmod.h"

typedef struct {
    BlockData *result;
    BitMap *bg;
    int score;
} BlocUndo;

BlocUndo *BlocUndo_Create(void);
void BlocUndo_Restore(BlocUndo *undo);
void BlocUndo_Destroy(BlocUndo *undo);

#ifdef CMOD_NOMAKEFILE
#include "blocundo.c"
#endif

#endif
