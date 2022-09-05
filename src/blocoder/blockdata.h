
#ifndef _blockdata_h
#define _blockdata_h

#include "intcoord.h"
#include "isl.h"
#include "stdtypes.h"

typedef struct {
    blockid id;
    u8 valid;
    u16 nof_coords;
    u16 alloced_coords;
    IntCoord *coord;
    int area;
    IntCoord ul; /* BBox upper left */
    IntCoord br; /* BBox bottom right*/
} BlockData;

BlockData *BlockData_CreateRect(blockid id, int x1, int y1, int x2, int y2);
/* Requires manual setting of ID after calling ! */
BlockData *BlockData_CreateMerge(const BlockData *b1, const BlockData *b2);
BlockData *BlockData_Clone(const BlockData *b);
Bool BlockData_IsMergeable(const BlockData *b1, const BlockData *b2);
Bool BlockData_IsSwappable(const BlockData *b1, const BlockData *b2);
Bool BlockData_IsMergeableHoriz(const BlockData *b1, const BlockData *b2);
Bool BlockData_IsMergeableVert(const BlockData *b1, const BlockData *b2);
Bool BlockData_IsMergeableOrient(const BlockData *b1, const BlockData *b2, Bool dir);
void BlockData_Destroy(BlockData *bd);
int BlockData_X(const BlockData *bd, int n);
int BlockData_Y(const BlockData *bd, int n);
#define BlockData_X(bd, i) (IntCoord_X((bd)->coord[i]))
#define BlockData_Y(bd, i) (IntCoord_Y((bd)->coord[i]))
#define BlockData_NofCoords(bd) ((bd)->nof_coords)

void BlockData_Print(const BlockData *b);

#ifdef CMOD_NOMAKEFILE
#include "blockdata.c"
#endif

#endif
