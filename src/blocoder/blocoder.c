
#include "bitmap.h"
#include "blockdata.h"
#include "blocundo.h"
#include "cmod.h"
#include "darray.h"
#include "isl.h"
#include "memory.h"
#include "string.h"
#include "xogui.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

XoGUI *gui;

typedef struct {
   BitMap *target;
   BitMap *canvas;
   int target_size;
   unsigned int master_blockid;
   DArray *blockdata;
} BloCoder;

/* ------------------------------------------------------------------------ */

BloCoder *BloCoder_Create(const char *target_png) {
   BloCoder *blocoder;
   BlockData *initial;
   
   blocoder = Memory_Reserve(1, BloCoder);
   blocoder->target = BitMap_LoadPNGFile(target_png);
   gui = XoGUI_Create(BitMap_SizeX(blocoder->target), BitMap_SizeY(blocoder->target));
   blocoder->blockdata = DArray_Create(32);
   blockid root;
   root.qword = 0ULL;
   root.digits = 0ULL;
   root.nof_digits = 0;
   initial = BlockData_CreateRect(root, 0, 0, BitMap_SizeX(blocoder->target), BitMap_SizeY(blocoder->target));
   DArray_Add(blocoder->blockdata, initial);
   blocoder->master_blockid = 0;
   blocoder->target_size = BitMap_SizeX(blocoder->target) * BitMap_SizeY(blocoder->target);
   blocoder->canvas = BitMap_CreateVal(BitMap_SizeX(blocoder->target), BitMap_SizeY(blocoder->target), PixelColor_FromRGBA(0,0,0,0));
   BitMap_FlipY(blocoder->target);
   
   return blocoder;
}

/* ------------------------------------------------------------------------ */

void BloCoder_Update(BloCoder *blocoder) {
   XoGUI_Update(gui, 0.1);
}

/* ------------------------------------------------------------------------ */

void BloCoder_PrintValidBlocks(const BloCoder *blo) {
    for (int i=0; i<DArray_NofElems(blo->blockdata); i++) {
        BlockData *b = (BlockData *)DArray_Elem(blo->blockdata, i);
        if (b->valid) BlockData_Print(b);
    }
}

/* ------------------------------------------------------------------------ */

void BloCoder_Destroy(BloCoder *blocoder) {
    DArray_Destroy(blocoder->blockdata, (DestFunc *)BlockData_Destroy);
    XoGUI_Destroy(gui);
    BitMap_Destroy(blocoder->target);
    BitMap_Destroy(blocoder->canvas);
    Memory_Free(blocoder, BloCoder);
}

/* ------------------------------------------------------------------------ */

BlockData *BloCoder_LookUp(const BloCoder *bl, blockid id) {
    for (int i=0; i<DArray_NofElems(bl->blockdata); i++) {
        BlockData *b = DArray_Elem(bl->blockdata, i);
        if (b->id.qword == id.qword) return b;
    }
    return NULL;
}

/* ------------------------------------------------------------------------ */

BlockData *BloCoder_Delete(BloCoder *bl, blockid id) {
    for (int i=0; i<DArray_NofElems(bl->blockdata); i++) {
        BlockData *b = DArray_Elem(bl->blockdata, i);
        fflush(stdout);
        if (b->id.qword == id.qword) {
            DArray_Del(bl->blockdata, i);
            return b;
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------------ */

int BloCoder_CanvasScore(const BloCoder *bl) {
    int w = BitMap_SizeY(bl->target);
    int h = BitMap_SizeX(bl->target);
    double bsum = 0.0;
    for (int y=0; y<h; y++) {
        for (int x=0; x<w; x++) {
            PixelColor s = BitMap_GetPixel(bl->canvas, x, y);
            PixelColor t = BitMap_GetPixel(bl->target, x, y);
            int d = s.r - t.r;
            int sum = d*d;
            d = s.g - t.g;
            sum = d*d;
            d = s.b - t.b;
            sum = d*d;
            d = s.a - t.a;
            sum = d*d;
            bsum += sqrt(sum);
        }
    }
    return (int)(0.5 + bsum*0.05);
}

/* ------------------------------------------------------------------------ */

/* Assumes Blockdata coordinates in clockwise order */
BlocUndo *BloCoder_ApplyISL(BloCoder *bl, ISL *isl) {
    BlockData *g1, *g2, *g3, *g4;
    BlocUndo *undo = BlocUndo_Create();
    undo->score = isl_cost[isl->opcode];
    switch(isl->opcode) {
    case ISL_LCUT: {
        BlockData *parent = BloCoder_LookUp(bl, isl->lcut.id);
        if (parent == NULL) { fprintf(stderr, "Incorrect blockid %s in LCUT\n", blockid_str(isl->lcut.id)); return undo; }
        int xmin = IntCoord_X(parent->ul);
        int ymin = IntCoord_Y(parent->ul);
        int xmax = IntCoord_X(parent->br);
        int ymax = IntCoord_Y(parent->br);
        int l = isl->lcut.l;

        printf("l=%i ", l);
        BlockData_Print(parent);
        if (isl->lcut.orientation) { /* cut along y=l */
            if (l >= ymax || l < ymin) { fprintf(stderr, "Incorrect line cut Y\n"); return undo; }
            g1 = BlockData_CreateRect(blockid_childid(isl->lcut.id, 0), xmin, ymin, xmax, l);
            g2 = BlockData_CreateRect(blockid_childid(isl->lcut.id, 1), xmin, l, xmax, ymax);
        } else {
            if (l >= xmax || l < xmin) { fprintf(stderr, "Incorrect line cut X\n"); return undo; }
            g1 = BlockData_CreateRect(blockid_childid(isl->lcut.id, 0), xmin, ymin, l, ymax);
            g2 = BlockData_CreateRect(blockid_childid(isl->lcut.id, 1), l, ymin, xmax, ymax);
        }
        DArray_Add(bl->blockdata, g1);
        DArray_Add(bl->blockdata, g2);
        parent->valid = FALSE;
        break; }
    case ISL_PCUT: {
        BlockData *parent = BloCoder_LookUp(bl, isl->pcut.id);
        if (parent == NULL) { fprintf(stderr, "Incorrect blockid %s in PCUT\n", blockid_str(isl->lcut.id)); return undo; }
        int xmin = IntCoord_X(parent->ul);
        int ymin = IntCoord_Y(parent->ul);
        int xmax = IntCoord_X(parent->br);
        int ymax = IntCoord_Y(parent->br);
        int x = isl->pcut.x;
        int y = isl->pcut.y;

        g1 = BlockData_CreateRect(blockid_childid(isl->pcut.id, 0), xmin, ymin, x, y);
        g2 = BlockData_CreateRect(blockid_childid(isl->pcut.id, 1), x, ymin, xmax, y);
        g3 = BlockData_CreateRect(blockid_childid(isl->pcut.id, 2), x, y, xmax, ymax);
        g4 = BlockData_CreateRect(blockid_childid(isl->pcut.id, 3), xmin, y, x, ymax);
        DArray_Add(bl->blockdata, g1);
        DArray_Add(bl->blockdata, g2);
        DArray_Add(bl->blockdata, g3);
        DArray_Add(bl->blockdata, g4);
        parent->valid = FALSE;
        
        break; }
    case ISL_SWAP: {
        BlockData *parent[2];
        int xmin[2], ymin[2], xmax[2], ymax[2];
        
        for (int j=0; j<2; j++) {
            parent[j] = BloCoder_LookUp(bl, isl->swap.id[j]);
            xmin[j] = IntCoord_X(parent[j]->ul);
            ymin[j] = IntCoord_Y(parent[j]->ul);
            xmax[j] = IntCoord_X(parent[j]->br);
            ymax[j] = IntCoord_Y(parent[j]->br);        }
        /* Better: loop through list and compare */
        if (xmax[1] - xmin[1] == xmax[0] - xmin[0] && ymax[1] - ymin[1] == ymax[0] - ymin[0]) {
            BitMap *a1 = BitMap_GetBitMap(bl->canvas, xmin[0], xmax[0], ymin[0], ymax[0]);
            BitMap *a2 = BitMap_GetBitMap(bl->canvas, xmin[1], xmax[1], ymin[1], ymax[1]);
            BitMap_PutBitMap(bl->canvas, a2, xmin[0], ymin[0]);
            BitMap_PutBitMap(bl->canvas, a1, xmin[1], ymin[1]);
            BitMap_Destroy(a1);
            BitMap_Destroy(a2);
        } else {
            fprintf(stderr, "SWAP between incompatible regions\n");
        }
        break; }
    case ISL_MERGE: {
        BlockData *parent[2];
        for (int j=0; j<2; j++) parent[j] = BloCoder_LookUp(bl, isl->merge.id[j]);            
        BlockData *b = BlockData_CreateMerge(parent[0], parent[1]);
        undo->result = b;
        parent[0]->valid = FALSE;
        parent[1]->valid = FALSE;
        
        b->id.nof_digits = 0;
        b->id.digits = bl->master_blockid++;
        DArray_Add(bl->blockdata, b);
        /* TODO */
        break; }
    case ISL_COLOR: {
        BlockData *parent = BloCoder_LookUp(bl, isl->color.id);
        int xmin = IntCoord_X(parent->ul);
        int ymin = IntCoord_Y(parent->ul);
        int xmax = IntCoord_X(parent->br);
        int ymax = IntCoord_Y(parent->br);
        undo->bg = BitMap_GetBitMap(bl->canvas, xmin, xmax, ymin, ymax);
        BitMap_FillRect(bl->canvas, xmin, ymin, xmax-1, ymax-1, isl->color.col);
      printf(" debug : filling %i,%i to %i,%i\n", xmin, ymin, xmax-1, ymax-1);
        break; }
    default: ;
    }
    return undo;
}

/* ------------------------------------------------------------------------ */

void BloCoder_UndoISL(BloCoder *bl, ISL *isl, const BlocUndo *undo) {
    switch(isl->opcode) {
    case ISL_LCUT: {
        BlockData *parent = BloCoder_LookUp(bl, isl->lcut.id);
        parent->valid = TRUE;
        
        for (int j=0; j<2; j++) {
            BlockData *child = BloCoder_Delete(bl, blockid_childid(isl->lcut.id, j));
            child->valid = FALSE;
            BlockData_Destroy(child);
        }
        break; }
    case ISL_PCUT: {
        BlockData *parent = BloCoder_LookUp(bl, isl->pcut.id);
        parent->valid = TRUE;
        
        for (int j=0; j<4; j++) {
            BlockData *child = BloCoder_Delete(bl, blockid_childid(isl->lcut.id, j));
            child->valid = FALSE;
            BlockData_Destroy(child);
        }
        break; }
    case ISL_SWAP: {
        BlockData *parent[2];
        int xmin[2], ymin[2], xmax[2], ymax[2];
        
        for (int j=0; j<2; j++) {
            parent[j] = BloCoder_LookUp(bl, isl->swap.id[j]);
            xmin[j] = xmax[j] = BlockData_X(parent[j], 0);
            ymin[j] = ymax[j] = BlockData_X(parent[j], 0);
            for (int i=1; i<BlockData_NofCoords(parent[j]); i++) {
                if (BlockData_X(parent[j], i) < xmin[j]) xmin[j] = BlockData_X(parent[j], i);
                else
                if (BlockData_X(parent[j], i) > xmax[j]) xmax[j] = BlockData_X(parent[j], i);
                if (BlockData_Y(parent[j], i) < ymin[j]) ymin[j] = BlockData_Y(parent[j], i);
                else
                if (BlockData_Y(parent[j], i) > ymax[j]) ymax[j] = BlockData_Y(parent[j], i);
            }
        }
        /* Better: loop through list and compare */
        if (xmax[1] - xmin[1] == xmax[0] - xmin[0] && ymax[1] - ymin[1] == ymax[0] - ymin[0]) {
            BitMap *a1 = BitMap_GetBitMap(bl->canvas, xmin[0], xmax[0], ymin[0], ymax[0]);
            BitMap *a2 = BitMap_GetBitMap(bl->canvas, xmin[1], xmax[1], ymin[1], ymax[1]);
            BitMap_PutBitMap(bl->canvas, a2, xmin[0], ymin[0]);
            BitMap_PutBitMap(bl->canvas, a1, xmin[1], ymin[1]);
            BitMap_Destroy(a1);
            BitMap_Destroy(a2);
        } else {
            fprintf(stderr, "SWAP between incompatible regions\n");
        }
        break; }
    case ISL_MERGE: {
        BlockData *parent[2];
        for (int j=0; j<2; j++) parent[j] = BloCoder_LookUp(bl, isl->merge.id[j]);
        undo->result->valid = FALSE;
        parent[0]->valid = TRUE;
        parent[1]->valid = TRUE;
        BloCoder_Delete(bl, undo->result->id);
        BlockData_Destroy(undo->result);
        break; }
    case ISL_COLOR: {
        BlockData *parent = BloCoder_LookUp(bl, isl->color.id);
        int xmin = BlockData_X(parent, 0);
        int ymin = BlockData_Y(parent, 0);
        for (int i=1; i<BlockData_NofCoords(parent); i++) {
            if (BlockData_X(parent, i) < xmin) xmin = BlockData_X(parent, i);
            if (BlockData_Y(parent, i) < ymin) ymin = BlockData_Y(parent, i);
        }
        BitMap_PutBitMap(bl->canvas, undo->bg, xmin, ymin);
        break; }
    default:
    }
}


/* ------------------------------------------------------------------------ */

int main(int argc, String **argv) {
   BloCoder *blocoder;
   BlocUndo *undo[8];
   ISL isl[8];
   
   if (argc < 2) {
      printf("Usage: %s png-file\n", argv[0]);
      return -1;
   }
   
   Memory_DebugInit(stdout);
   
   blocoder = BloCoder_Create(argv[1]);
   isl[0].opcode = ISL_PCUT;
   isl[0].pcut.id.qword = 0ULL;
   isl[0].pcut.x = 200;
   isl[0].pcut.y = 200;

   printf("%s\n", ISL_ToString(isl));
   undo[0] = BloCoder_ApplyISL(blocoder, isl);  
   
   isl[1].pcut.id = blockid_childid(isl[0].pcut.id, 2);
   isl[1].opcode = ISL_COLOR;
   isl[1].color.col = PixelColor_Construct(200, 80, 10);
   printf("%s\n", ISL_ToString(isl+1));
   undo[1] = BloCoder_ApplyISL(blocoder, isl+1);

   BloCoder_PrintValidBlocks(blocoder);

   isl[2].opcode = ISL_LCUT;
   isl[2].lcut.id = blockid_childid(isl[0].pcut.id, 2);
   isl[2].lcut.l = 250;
   isl[2].lcut.orientation = 0;
   printf("%s\n", ISL_ToString(isl+2));
   undo[2] = BloCoder_ApplyISL(blocoder, isl+2);

   BloCoder_PrintValidBlocks(blocoder);   
   
   isl[3].opcode = ISL_COLOR;
   isl[3].color.id = blockid_childid(isl[2].lcut.id, 0);
   isl[3].color.col = PixelColor_Construct(20, 80, 190);
   printf("%s\n", ISL_ToString(isl+3));
   undo[3] = BloCoder_ApplyISL(blocoder, isl+3);

   /*
   BloCoder_UndoISL(blocoder, isl+2, undo[2]);
   BloCoder_UndoISL(blocoder, isl+1, undo[1]);
   BloCoder_UndoISL(blocoder, isl, undo[0]);
   */
   while (XoGUI_Key(gui) != 27) {
       XoGUI_PutImage(gui, -1, blocoder->canvas, 0, 0);
       BloCoder_Update(blocoder);       
   }
   BloCoder_Destroy(blocoder);
   
   Memory_DebugRestore();
   return 0;
}


