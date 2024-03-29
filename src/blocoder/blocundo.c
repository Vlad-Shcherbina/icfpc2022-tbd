
#include "bitmap.h"
#include "blocundo.h"
#include "cmod.h"
#include "memory.h"

BlocUndo *BlocUndo_Create(void) {
    BlocUndo *undo = Memory_Reserve(1, BlocUndo);
    undo->bg = NULL;
    undo->score = 0;
    return undo;
}

void BlocUndo_Restore(BlocUndo *undo) {
    if (undo->bg != NULL) BitMap_Destroy(undo->bg);
}

void BlocUndo_Destroy(BlocUndo *undo) {
    BlocUndo_Restore(undo);
    Memory_Free(undo, BlocUndo);
}

