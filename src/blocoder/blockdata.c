
#include "blockdata.h"
#include "cmod.h"
#include "memory.h"


void BlockData_Print(const BlockData *b) {
    printf("id = %s [%i:%lx], bbox = (%i,%i) - (%i,%i)\n", blockid_str(b->id), b->id.nof_digits, b->id.digits, IntCoord_X(b->ul), IntCoord_Y(b->ul), IntCoord_X(b->br), IntCoord_Y(b->br));
}
/*
void BlockData_Print(const BlockData *b) {
    String *s = blockid_str(b->id);
    printf("ID=%s, valid=%i, nof_coords=%i {", s, b->valid, b->nof_coords);
    for (int i=0; i<BlockData_NofCoords(b); i++) {
        printf("(%i,%i)", BlockData_X(b,i), BlockData_Y(b,i));
        if (i<BlockData_NofCoords(b)) printf(", ");
    }
    printf("}\n");
}
*/
void BlockData_UpdateBBox(BlockData *parent) {
    int xmin = BlockData_X(parent, 0);
    int ymin = BlockData_Y(parent, 0);
    int xmax = xmin;
    int ymax = xmax;
    for (int i=1; i<BlockData_NofCoords(parent); i++) {
        if (BlockData_X(parent, i) < xmin) xmin = BlockData_X(parent, i);
        else
        if (BlockData_X(parent, i) > xmax) xmax = BlockData_X(parent, i);
        if (BlockData_Y(parent, i) < ymin) ymin = BlockData_Y(parent, i);
        else
        if (BlockData_Y(parent, i) > ymax) ymax = BlockData_Y(parent, i);
    }
    parent->ul = IntCoord_Construct(xmin, ymin);
    parent->br = IntCoord_Construct(xmax, ymax);
}

/* (x1,y1) and (x2,y2) are diagonally opposite */
BlockData *BlockData_CreateRect(blockid id, int x1, int y1, int x2, int y2) {
    BlockData *bl = Memory_Reserve(1, BlockData);
    bl->id = id;
    bl->valid = TRUE;
    bl->nof_coords = 4;
    bl->alloced_coords = 4;
    bl->coord = Memory_Reserve(4, IntCoord);
    bl->coord[0] = IntCoord_Construct(x1, y1);
    bl->coord[1] = IntCoord_Construct(x2, y1);
    bl->coord[2] = IntCoord_Construct(x2, y2);
    bl->coord[3] = IntCoord_Construct(x1, y2);
    bl->area = (x2-x1) * (y2-y1);
    BlockData_UpdateBBox(bl);

    return bl;
}

/* Requires manual setting of ID after calling ! */
BlockData *BlockData_CreateMerge(const BlockData *b1, const BlockData *b2) {
    BlockData *bl = Memory_Reserve(1, BlockData);
    const BlockData *parent[2] = { b1, b2 };
    int xmin, ymin, xmax, ymax;
    xmin = xmax = BlockData_X(parent[0], 0);
    ymin = ymax = BlockData_X(parent[0], 0);
    for (int j=0; j<2; j++) {
        for (int i=0; i<BlockData_NofCoords(parent[j]); i++) {
            if (BlockData_X(parent[j], i) < xmin) xmin = BlockData_X(parent[j], i);
            else
            if (BlockData_X(parent[j], i) > xmax) xmax = BlockData_X(parent[j], i);
            if (BlockData_Y(parent[j], i) < ymin) xmin = BlockData_Y(parent[j], i);
            else
            if (BlockData_Y(parent[j], i) > ymax) ymax = BlockData_X(parent[j], i);
        }
    }
    bl->id.qword = 0ULL;
    bl->valid = TRUE;
    bl->nof_coords = 4;
    bl->alloced_coords = 4;
    bl->coord = Memory_Reserve(4, IntCoord);
    bl->coord[0] = IntCoord_Construct(xmin, ymin);
    bl->coord[1] = IntCoord_Construct(xmax, ymin);
    bl->coord[2] = IntCoord_Construct(xmax, ymax);
    bl->coord[3] = IntCoord_Construct(xmin, ymax);
    bl->area = (xmax-xmin)*(ymax-ymin);
    BlockData_UpdateBBox(bl);
    return bl;
}


void BlockData_Destroy(BlockData *bd) {
    Memory_Free(bd->coord, IntCoord);
    Memory_Free(bd, BlockData);
}


