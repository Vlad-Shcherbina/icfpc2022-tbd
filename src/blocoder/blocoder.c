
#include "bitmap.h"
#include "blockdata.h"
#include "blocundo.h"
#include "cmod.h"
#include "darray.h"
#include "file.h"
#include "isl.h"
#include "islseq.h"
#include "memory.h"
#include "stdfunc.h"
#include "string.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <x86intrin.h>

typedef struct {
   BitMap *target; /* stays untouched */
   BitMap *start_canvas; /* untouched */
   BitMap *canvas;
   DArray *initial_blockdata; /* untouched */
   int target_size;
   double target_size_div;
   unsigned int start_master_blockid;
   unsigned int master_blockid;
   DArray *blockdata;
} BloCoder;

double sqroot_table[4*65536];

/* ------------------------------------------------------------------------ */

BloCoder *BloCoder_Create(const char *target_png) {
   BloCoder *blocoder;
   BlockData *initial;
   File *f;
   String *initial_conf = String_Clone(target_png);
   int n = String_Length(initial_conf);
   blockid root;
   
   root.qword = 0ULL;
   root.digits = 0ULL;
   root.nof_digits = 0;

   blocoder = Memory_Reserve(1, BloCoder);
   blocoder->target = BitMap_LoadPNGFile(target_png);
//   gui = XoGUI_Create(BitMap_SizeX(blocoder->target), BitMap_SizeY(blocoder->target));
   blocoder->initial_blockdata = DArray_Create(32);
   blocoder->master_blockid = 0;

   blocoder->canvas = BitMap_CreateVal(BitMap_SizeX(blocoder->target), BitMap_SizeY(blocoder->target), PixelColor_FromRGBA(255,255,255,255));
   if (n>3) String_Printf(initial_conf+n-3, "txt");
   f = File_Open(initial_conf, "r");
   if (f != NULL) {
       int id, x1,y1, x2,y2, r,g,b,a;
       while (!File_Eof(f) && File_Scanf(f, "%i %i,%i %i,%i %i,%i,%i,%i\n", &id, &x1,&y1, &x2,&y2, &r,&g,&b,&a) == 9) {
           blockid nid = { .nof_digits = 0, .digits = id };
           BlockData *bd = BlockData_CreateRect(nid, x1, y1, x2, y2);
           BitMap_FillRect(blocoder->canvas, x1, y1, x2, y2, PixelColor_FromRGBA(r, g, b, a));
           DArray_Add(blocoder->initial_blockdata, bd);
           if (id > blocoder->master_blockid) blocoder->master_blockid = id+1;
       }
       File_Close(f);
   }
   printf("Read %i initial blocks\n", DArray_NofElems(blocoder->initial_blockdata));
   
   if (DArray_NofElems(blocoder->initial_blockdata) == 0) {
       initial = BlockData_CreateRect(root, 0, 0, BitMap_SizeX(blocoder->target), BitMap_SizeY(blocoder->target));
       DArray_Add(blocoder->initial_blockdata, initial);
       blocoder->master_blockid = root.digits + 1;
   }
   blocoder->blockdata = DArray_Clone(blocoder->initial_blockdata, (CloneFunc *)BlockData_Clone);
   blocoder->start_master_blockid = blocoder->master_blockid;

   BitMap_FlipY(blocoder->target);
   BitMap_FlipY(blocoder->canvas);
   blocoder->start_canvas = BitMap_Clone(blocoder->canvas);
   
   blocoder->target_size = BitMap_SizeX(blocoder->target) * BitMap_SizeY(blocoder->target);
   blocoder->target_size_div = 1.0 / (double)blocoder->target_size;   
   
   String_Destroy(initial_conf);
   
   return blocoder;
}

/* ------------------------------------------------------------------------ */

void BloCoder_Reset(BloCoder *blo) {
    DArray_Destroy(blo->blockdata, (DestFunc *)BlockData_Destroy);
    blo->blockdata = DArray_Clone(blo->initial_blockdata, (CloneFunc *)BlockData_Clone);
    BitMap_Destroy(blo->canvas);
    blo->canvas = BitMap_Clone(blo->start_canvas);
    blo->master_blockid = blo->start_master_blockid;
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

BlockData *BloCoder_LookUpChild(const BloCoder *bl, blockid id, int n) {
    return BloCoder_LookUp(bl, blockid_childid(id, n));
}

/* ------------------------------------------------------------------------ */

BlockData *BloCoder_LookUpChildBd(const BloCoder *bl, const BlockData *parent, int n) {
    return BloCoder_LookUp(bl, blockid_childid(parent->id, n));
}

/* ------------------------------------------------------------------------ */

BlockData *BloCoder_LookUpParent(const BloCoder *bl, blockid id) {
    return BloCoder_LookUp(bl, blockid_parentid(id));
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

int BloCoder_ScoreArea(const BloCoder *bl, int xmin, int ymin, int xmax, int ymax) {
    double bsum = 0.0;
    for (int y=ymin; y<ymax; y++) {
        for (int x=xmin; x<xmax; x++) {
            PixelColor s = BitMap_GetPixel(bl->canvas, x, y);
            PixelColor t = BitMap_GetPixel(bl->target, x, y);
            int d = s.r - t.r;
            int sum = d*d;
            d = s.g - t.g;
            sum += d*d;
            d = s.b - t.b;
            sum += d*d;
            d = s.a - t.a;
            sum += d*d;
            bsum = sqroot_table[sum];
        }
    }
    return (int)(0.5 + bsum*0.005);
}

/* ------------------------------------------------------------------------ */

typedef __m256i u256;

int BloCoder_CanvasScore(const BloCoder *bl) {
    int w = BitMap_SizeY(bl->target);
    int h = BitMap_SizeX(bl->target);
    double bsum = 0.0;
    int i=0;

    union __attribute__((aligned(32))) {
        u256 qqword;
        u8   d[64];
        u16  sqr[32];
        u32  to_lookup[16];
    } ixes;

    for (i=0; i<w*h-7; i+=8) {
        u256 s = _mm256_loadu_si256((u256 *)(bl->canvas->buf + i));
        u256 t = _mm256_loadu_si256((u256 *)(bl->target->buf + i));
        u256 d = _mm256_or_si256(_mm256_subs_epu8(s,t), _mm256_subs_epu8(t,s));
        
        u256 z = _mm256_setzero_si256();
        u256 l1 = _mm256_unpacklo_epi8(d, z);
        u256 l2 = _mm256_unpackhi_epi8(d, z);
        u256 sqr1 = _mm256_mullo_epi16(l1, l1);
        u256 sqr2 = _mm256_mullo_epi16(l2, l2);

        _mm256_store_si256((u256 *)ixes.sqr+0, sqr1);
        _mm256_store_si256((u256 *)ixes.sqr+1, sqr2);
        for (int ix=0; ix<8; ix++) {
            ixes.to_lookup[ix] = ixes.sqr[ix*4] + ixes.sqr[ix*4+1] + ixes.sqr[ix*4+2] + ixes.sqr[ix*4+3];
        }
        for (int ix=0; ix<8; ix++)
            bsum += sqroot_table[ixes.to_lookup[ix]];
    }

    for (;i<w*h; i++) {
        PixelColor s = bl->canvas->buf[i];
        PixelColor t = bl->target->buf[i];
        int d = s.r - t.r;
        int sum = d*d;
        d = s.g - t.g;
        sum += d*d;
        d = s.b - t.b;
        sum += d*d;
        d = s.a - t.a;
        sum += d*d;
        bsum += sqroot_table[sum];
    }

//    printf("Canvas bsum = %f  vs AVX %f\n", 0.5 + bsum2*0.005, 0.5 + bsum*0.005);
//#endif
    return (int)(0.5 + bsum*0.005);
}

/* ------------------------------------------------------------------------ */

//static int intcmp(int **a, int **b) { return **a - *b; }

PixelColor BloCoder_MostFrequentPixel(const BloCoder *blo, int xmin, int ymin, int xmax, int ymax) {
    int area = (xmax - xmin)*(ymax - ymin);
    PixelColor pres[area];
    int n = 0;
    for (int y=ymin; y<ymax; y++) {
    	for (int x=xmin; x<xmax; x++) {
            pres[n++] = BitMap_GetPixel(blo->target, x, y);
        }
    }
    qsort(pres, n, sizeof(PixelColor), Int_Compare);
    PixelColor best = pres[0];
    int bestnof = 0;
    int i, nof = 1;
    for (i=1; i<n; i++) {
        if (pres[i-1].dword == pres[i].dword) nof++;
        else {
            if (nof > bestnof) {
                bestnof = nof;
                best = pres[i-1];
            }
            nof = 1;
        }
    }
    if (nof > bestnof) best = pres[i-1];
//    printf("%08x %02x\n", best.dword, best.a);
    return best;
}

/* ------------------------------------------------------------------------ */

PixelColor BloCoder_MostFrequentPixelBD(const BloCoder *blo, const BlockData *parent) {
    int xmin = IntCoord_X(parent->ul);
    int ymin = IntCoord_Y(parent->ul);
    int xmax = IntCoord_X(parent->br);
    int ymax = IntCoord_Y(parent->br);
    
    return BloCoder_MostFrequentPixel(blo, xmin, ymin, xmax, ymax);
}

/* ------------------------------------------------------------------------ */

BlockData *BloCoder_GetChild(const BloCoder *blo, const BlockData *bd, int n) {
    blockid cid = blockid_childid(bd->id, n);
    return BloCoder_LookUp(blo, cid);
}

/* ------------------------------------------------------------------------ */

PixelColor BloCoder_AvgTargetColor(const BloCoder *blo, int xmin, int ymin, int xmax, int ymax) {
    int area = (ymax-ymin)*(xmax - xmin);
    int sum[4] = { area/2, area/2, area/2, area/2 };
    
    for (int y=ymin; y<ymax; y++) {
        for (int x=xmin; x<xmax; x++) {
            PixelColor t = BitMap_GetPixel(blo->target, x, y);
            sum[0] += t.r;
            sum[1] += t.g;
            sum[2] += t.b;
            sum[3] += t.a;
        }
    }
    sum[0] /= area;
    sum[1] /= area;
    sum[2] /= area;
    sum[3] /= area;
    return PixelColor_FromRGBA(sum[0], sum[1], sum[2], sum[3]);
}

/* ------------------------------------------------------------------------ */

PixelColor BloCoder_AvgTargetColorBD(const BloCoder *blo, const BlockData *bd) {
    int xmin = IntCoord_X(bd->ul);
    int ymin = IntCoord_Y(bd->ul);
    int xmax = IntCoord_X(bd->br);
    int ymax = IntCoord_Y(bd->br);
    return BloCoder_AvgTargetColor(blo, xmin, ymin, xmax, ymax);
}

/* ------------------------------------------------------------------------ */

PixelColor BloCoder_GeometricMeanColor(const BloCoder *blo,  int xmin, int ymin, int xmax, int ymax) {
    int area = (ymax-ymin)*(xmax - xmin);
    int nof[4][256] = { 0 };
    PixelColor ret = { 0 };
    
    for (int y=ymin; y<ymax; y++) {
        for (int x=xmin; x<xmax; x++) {
            PixelColor t = BitMap_GetPixel(blo->target, x, y);
            for (int c=0; c<4; c++) nof[c][t.v[c]]++;
        }
    }
    for (int c=0; c<4; c++) {
        int m = area/2;
        int b = 255;
        while (m>0) m-= nof[c][b--];
        ret.v[c] = b+1;
    }
    return ret;
}

/* ------------------------------------------------------------------------ */

PixelColor BloCoder_GeometricMeanColorBD(const BloCoder *blo, const BlockData *bd) {
    int xmin = IntCoord_X(bd->ul);
    int ymin = IntCoord_Y(bd->ul);
    int xmax = IntCoord_X(bd->br);
    int ymax = IntCoord_Y(bd->br);
    return BloCoder_GeometricMeanColor(blo, xmin, ymin, xmax, ymax);
}

int debug = FALSE;

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
        undo->score *= bl->target_size;
        undo->score = 0.5 + (double)undo->score / (double)parent->area;
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
        if (y < ymin || y >= ymax) fprintf(stderr, "Bad y-coordinate for pcut\n");
        if (x < xmin || x >= xmax) fprintf(stderr, "Bad x-coordinate for pcut\n");

        g1 = BlockData_CreateRect(blockid_childid(isl->pcut.id, 0), xmin, ymin, x, y);
        g2 = BlockData_CreateRect(blockid_childid(isl->pcut.id, 1), x, ymin, xmax, y);
        g3 = BlockData_CreateRect(blockid_childid(isl->pcut.id, 2), x, y, xmax, ymax);
        g4 = BlockData_CreateRect(blockid_childid(isl->pcut.id, 3), xmin, y, x, ymax);
        DArray_Add(bl->blockdata, g1);
        DArray_Add(bl->blockdata, g2);
        DArray_Add(bl->blockdata, g3);
        DArray_Add(bl->blockdata, g4);
        parent->valid = FALSE;
        undo->score *= bl->target_size;
        undo->score = 0.5 + (double)undo->score / (double)parent->area;
        
        break; }
    case ISL_SWAP: {
        BlockData *parent[2];
        int xmin[2], ymin[2], xmax[2], ymax[2];
        
        for (int j=0; j<2; j++) {
            parent[j] = BloCoder_LookUp(bl, isl->swap.id[j]);
            xmin[j] = IntCoord_X(parent[j]->ul);
            ymin[j] = IntCoord_Y(parent[j]->ul);
            xmax[j] = IntCoord_X(parent[j]->br);
            ymax[j] = IntCoord_Y(parent[j]->br);
        }

        BitMap *a1 = BitMap_GetBitMap(bl->canvas, xmin[0], xmax[0], ymin[0], ymax[0]);
        BitMap *a2 = BitMap_GetBitMap(bl->canvas, xmin[1], xmax[1], ymin[1], ymax[1]);
        BitMap_PutBitMapNoClip(bl->canvas, a2, xmin[0], ymin[0]);
        BitMap_PutBitMapNoClip(bl->canvas, a1, xmin[1], ymin[1]);
        BitMap_Destroy(a1);
        BitMap_Destroy(a2);

        undo->score *= bl->target_size;
        undo->score = 0.5 + (double)undo->score / (double)parent[0]->area;
        SWAP(parent[0]->ul, parent[1]->ul);
        SWAP(parent[0]->br, parent[1]->br);
        SWAP(parent[0]->coord, parent[1]->coord);
#if 0
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
        undo->score *= bl->target_size;
        undo->score = 0.5 + (double)undo->score / (double)parent[0]->area;
        SWAP(parent[0]->ul, parent[1]->ul);
        SWAP(parent[0]->br, parent[1]->br);
        SWAP(parent[0]->coord, parent[1]->coord);
        if (debug) {
            printf("Swap %li and %li\n", parent[0]->id.digits, parent[1]->id.digits);
            BlockData_Print(parent[0]);
            BlockData_Print(parent[1]);
        }
#endif
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
        undo->score *= bl->target_size;
        undo->score = 0.5 + (double)undo->score / MAX(parent[0]->area, parent[1]->area);
        if (debug) {
        printf("Merge %i and %li to new id %li\n", parent[0]->id.digits, parent[1]->id.digits, b->id.digits);
        BlockData_Print(parent[0]);
        BlockData_Print(parent[1]);
        }
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
        undo->score *= bl->target_size;
        undo->score = 0.5 + (double)undo->score / (double)parent->area;
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
            BlockData *child = BloCoder_Delete(bl, blockid_childid(isl->pcut.id, j));
            child->valid = FALSE;
            BlockData_Destroy(child);
        }
        break; }
    case ISL_SWAP: {
        BlockData *parent[2];
        int xmin[2], ymin[2], xmax[2], ymax[2];
        
        for (int j=0; j<2; j++) {
            parent[j] = BloCoder_LookUp(bl, isl->swap.id[j]);
            xmin[j] = IntCoord_X(parent[j]->ul);
            ymin[j] = IntCoord_Y(parent[j]->ul);
            xmax[j] = IntCoord_X(parent[j]->br);
            ymax[j] = IntCoord_Y(parent[j]->br);
        }

        BitMap *a1 = BitMap_GetBitMap(bl->canvas, xmin[0], xmax[0], ymin[0], ymax[0]);
        BitMap *a2 = BitMap_GetBitMap(bl->canvas, xmin[1], xmax[1], ymin[1], ymax[1]);
        BitMap_PutBitMapNoClip(bl->canvas, a2, xmin[0], ymin[0]);
        BitMap_PutBitMapNoClip(bl->canvas, a1, xmin[1], ymin[1]);
        BitMap_Destroy(a1);
        BitMap_Destroy(a2);

        SWAP(parent[0]->ul, parent[1]->ul);
        SWAP(parent[0]->br, parent[1]->br);
        SWAP(parent[0]->coord, parent[1]->coord);

#if 0
        BlockData *parent[2];

        int xmin[2], ymin[2], xmax[2], ymax[2];

        for (int j=0; j<2; j++) {
            parent[j] = BloCoder_LookUp(bl, isl->swap.id[j]);
        }
        
        SWAP(parent[0]->ul, parent[1]->ul);
        SWAP(parent[0]->br, parent[1]->br);
        SWAP(parent[0]->coord, parent[1]->coord);
        
        for (int j=0; j<2; j++) {
            xmin[j] = IntCoord_X(parent[j]->ul);
            ymin[j] = IntCoord_X(parent[j]->ul);
            xmax[j] = IntCoord_X(parent[j]->br);
            ymax[j] = IntCoord_X(parent[j]->br);
        }

        if (debug) printf("Canvas score before undo swap %i\n", BloCoder_CanvasScore(bl));
        BitMap *a2 = BitMap_GetBitMap(bl->canvas, xmin[0], xmax[0], ymin[0], ymax[0]);
        BitMap *a1 = BitMap_GetBitMap(bl->canvas, xmin[1], xmax[1], ymin[1], ymax[1]);
        BitMap_PutBitMap(bl->canvas, a1, xmin[0], ymin[0]);
        BitMap_PutBitMap(bl->canvas, a2, xmin[1], ymin[1]);
        BitMap_Destroy(a1);
        BitMap_Destroy(a2);
        if (debug) printf("Canvas score after undo swap %i\n", BloCoder_CanvasScore(bl));
#endif
#if 0
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
#endif
        break; }
    case ISL_MERGE: {
        BlockData *parent[2];
        for (int j=0; j<2; j++) parent[j] = BloCoder_LookUp(bl, isl->merge.id[j]);
        undo->result->valid = FALSE;
        parent[0]->valid = TRUE;
        parent[1]->valid = TRUE;
        BloCoder_Delete(bl, undo->result->id);
        BlockData_Destroy(undo->result);
        bl->master_blockid--;
        break; }
    case ISL_COLOR: {
        BlockData *parent = BloCoder_LookUp(bl, isl->color.id);
        int xmin = IntCoord_X(parent->ul);
        int ymin = IntCoord_Y(parent->ul);
/*
        for (int i=1; i<BlockData_NofCoords(parent); i++) {
            if (BlockData_X(parent, i) < xmin) xmin = BlockData_X(parent, i);
            if (BlockData_Y(parent, i) < ymin) ymin = BlockData_Y(parent, i);
        }
*/
        BitMap_PutBitMap(bl->canvas, undo->bg, xmin, ymin);
        break; }
    default: ;
    }
}

/* ------------------------------------------------------------------------ */

int global_prob_id = -1;
double start_time = 0.0;
int toplevel;

void update_bestsofar(int score, int *bestsofar, ISL *bestisl, int *bestislc, ISL *src, int nsrc, int a) {
    if (score < *bestsofar) {
        printf("Problem %2i : [ t =%5.1fs ] new approximation %i (improves old %i) [depth=%i]\n", global_prob_id, Std_Time() - start_time, score, *bestsofar, a);
        *bestsofar = score;
        Memory_Copy(bestisl, src, nsrc*sizeof(ISL));
        *bestislc = nsrc;
    }
}

/*
will not modify canvas
returns score
adds ops to pv, and nof added in *islc
*/
int BloCoder_Solver(BloCoder *blo, BlockData *parent, ISL *isl, int *islc, int depth, int opscore, int bestsofar, Bool needs_paint) {
    ISL newisl[65536];
    int newislc = 0;
    ISL pv[32];
    int pvc = 0;
    /* generate isls */
    int xmin = IntCoord_X(parent->ul);
    int ymin = IntCoord_Y(parent->ul);
    int xmax = IntCoord_X(parent->br);
    int ymax = IntCoord_Y(parent->br);
    PixelColor medcol = BloCoder_GeometricMeanColor(blo, xmin, ymin, xmax, ymax);    
    
    /* paint ops */
    if (depth == 0) {
        ISL endpaint;
        int cs = BloCoder_CanvasScore(blo);
        
        ISL_Paint(&endpaint, parent->id, medcol);
        BlocUndo *u = BloCoder_ApplyISL(blo, &endpaint);
        int ps = BloCoder_CanvasScore(blo);
        int ret = cs + opscore;
                
        if (ret > ps + opscore + u->score) {
            *isl = endpaint;
            *islc = 1;
            ret = ps + opscore + u->score;
        } else
            *islc = 0;
        BloCoder_UndoISL(blo, &endpaint, u);
        BlocUndo_Destroy(u);
        return ret;
    }
    PixelColor comcol = BloCoder_MostFrequentPixelBD(blo, parent);
    PixelColor avgcol = BloCoder_AvgTargetColorBD(blo, parent);

    ISL_Paint(newisl + newislc++, parent->id, medcol);
    ISL_Paint(newisl + newislc++, parent->id, avgcol);
    ISL_Paint(newisl + newislc++, parent->id, comcol);
//    if (depth > 1) {
        int steplenl = 4;
        if (depth > 1) steplenl = 7;
        int steplenp = 11;
        if (depth > 1) steplenp = MIN(24, MAX(4, (xmax-xmin-2)/2));
        /* generate lcuts. pcuts */
        for (int l=xmin+2; l<xmax-2; l+=steplenl)
             ISL_LCut(newisl + newislc++, parent->id, 0, l);
        for (int l=ymin+2; l<ymax-2; l+=steplenl)
             ISL_LCut(newisl + newislc++, parent->id, 1, l);
  
        for (int y=ymin+2; y<ymax-2; y+=steplenp) {
            for (int x=xmin+2; x<xmax-2; x+=steplenp) {
                ISL_PCut(newisl + newislc++, parent->id, x, y);
            }
        }
//    }
    
    for (int i=0; i<newislc; i++) {
        int score;
        BlockData *child;
        pv[0] = newisl[i];
        String *s = ISL_ToString(pv);
//      for (int k=0; k<4-depth; k++) printf("  ");
//      printf("d=%i ISL=%s bestscore=%i\n", depth, s, bestsofar);
        String_Destroy(s);
        BlocUndo *undo = BloCoder_ApplyISL(blo, pv);

        if (depth == 2) printf("Problem %2i : [ t =%5.1fs ] on option %i/%i \n", global_prob_id, Std_Time() - start_time, i, newislc);
        switch (pv->opcode) {
        case ISL_PCUT:
            child = BloCoder_LookUpChild(blo, parent->id, 2);
            score = BloCoder_Solver(blo, child, pv+1, &pvc, depth-1, opscore + undo->score, bestsofar, needs_paint);
            update_bestsofar(score, &bestsofar, isl, islc, pv, pvc+1, depth);

            child = BloCoder_LookUpChild(blo, parent->id, 3);
            score = BloCoder_Solver(blo, child, pv+1, &pvc, depth-1, opscore + undo->score, bestsofar, needs_paint);
            update_bestsofar(score, &bestsofar, isl, islc, pv, pvc+1, depth);

        case ISL_LCUT:
            child = BloCoder_LookUpChild(blo, parent->id, 0);
            score = BloCoder_Solver(blo, child, pv+1, &pvc, depth-1, opscore + undo->score, bestsofar, needs_paint);
            update_bestsofar(score, &bestsofar, isl, islc, pv, pvc+1, depth);

            child = BloCoder_LookUpChild(blo, parent->id, 1);
            score = BloCoder_Solver(blo, child, pv+1, &pvc, depth-1, opscore + undo->score, bestsofar, needs_paint);
            update_bestsofar(score, &bestsofar, isl, islc, pv, pvc+1, depth);
            break;
        
        case ISL_COLOR:
            score = BloCoder_Solver(blo, parent, pv+1, &pvc, depth-1, opscore + undo->score, bestsofar, FALSE);
            update_bestsofar(score, &bestsofar, isl, islc, pv, pvc+1, depth);
            break;
        default: ;
        }
        BloCoder_UndoISL(blo, pv, undo);
        BlocUndo_Destroy(undo);
    }
    return bestsofar;
}

/* ------------------------------------------------------------------------ */

int BloCoder_ScoreSeq(BloCoder *blo, ISLSeq *seq) {
    BlocUndo *undo[ISLSeq_NofISL(seq)];
    seq->opscore = 0;

    for (int i=0; i<ISLSeq_NofISL(seq); i++) {
        ISL *isl = ISLSeq_ISL(seq, i);
        undo[i] = BloCoder_ApplyISL(blo, isl);
        seq->opscore += undo[i]->score;
    }

    seq->canvas_score = BloCoder_CanvasScore(blo);
    seq->score = seq->canvas_score + seq->opscore;
    
    for (int i=ISLSeq_NofISL(seq)-1; i>=0; i--) {
        ISL *isl = ISLSeq_ISL(seq, i);
        BloCoder_UndoISL(blo, isl, undo[i]);
        BlocUndo_Destroy(undo[i]);
    }
    return seq->score;
}

/* ------------------------------------------------------------------------ */

void BloCoder_ApplySeq(BloCoder *blo, ISLSeq *seq) {
    seq->opscore = 0;

    for (int i=0; i<ISLSeq_NofISL(seq); i++) {
        BlocUndo *undo = BloCoder_ApplyISL(blo, ISLSeq_ISL(seq, i));
        seq->opscore += undo->score;
        BlocUndo_Destroy(undo);
    }

    seq->canvas_score = BloCoder_CanvasScore(blo);
    seq->score = seq->canvas_score + seq->opscore;
}

/* ------------------------------------------------------------------------ */

void ISLSeq_CleanRedundantPaints(ISLSeq *seq) {
    blockid lastp = { .nof_digits = 0xff, .digits = 0xfffffff };
    for (int i=ISLSeq_Nof(seq)-1; i>=0; i--) {
        ISL *isl = ISLSeq_ISL(seq, i);
        
        if (isl->opcode == ISL_PAINT) {
            if (isl->color.id.qword == lastp.qword) {
                ISLSeq_Del(seq, i);
                ISL_Destroy(isl);
                continue;
            } else
                lastp.qword = isl->color.id.qword;
        }
    }
}

/* ------------------------------------------------------------------------ */

void BloCoder_ISLPostProcess(const char *filename, ISLSeq *sseq) {
    BloCoder *blo = BloCoder_Create(filename);
    PixelColor orgcol;
    int orgx, orgy, limitx, limity;
    int score;
    BlockData *parent;
    Bool improved = FALSE;

    ISLSeq_CleanRedundantPaints(sseq);
        
    ISLSeq *bestseq = ISLSeq_Clone(sseq);
    
    int bestscore = BloCoder_ScoreSeq(blo, sseq);
    
    do {
        improved = FALSE;
        printf("Problem %2i : Local optimizer writes intermediate step (score=%i)\n", global_prob_id, sseq->score);
        printf("----------------------------------\n");
        ISLSeq_ToFile(sseq, global_prob_id);
        printf("----------------------------------\n");
        for (int i=0; i<ISLSeq_Nof(sseq); i++) {
            ISL *isl = ISLSeq_ISL(sseq, i);
            switch (isl->opcode) {
            case ISL_PAINT:
                orgcol.dword = isl->color.col.dword;
                for (int c=0; c<4; c++) {
nextdec:
                    if (isl->color.col.v[c] > 1) {
                        isl->color.col.v[c]--;
                        score = BloCoder_ScoreSeq(blo, sseq);
                        if (score < bestscore) {
                            ISLSeq_Destroy(bestseq);
                            bestseq = ISLSeq_Clone(sseq);
                            bestscore = score;
                            orgcol.dword = isl->color.col.dword;
                            improved = TRUE;
                            goto nextdec;
                        } else
                        if (score == bestscore) goto nextdec;
                        else
                            isl->color.col.dword = orgcol.dword;
                    } else
                        isl->color.col.dword = orgcol.dword;
nextinc:
                    if (isl->color.col.v[c] < 254) {
                        isl->color.col.v[c]++;
                        score = BloCoder_ScoreSeq(blo, sseq);
                        if (score < bestscore) {
                            ISLSeq_Destroy(bestseq);
                            bestseq = ISLSeq_Clone(sseq);
                            bestscore = score;
                            orgcol.dword = isl->color.col.dword;
                            improved = TRUE;
                            goto nextinc;
                        } else
                        if (score == bestscore) goto nextinc;
                        else
                            isl->color.col.dword = orgcol.dword;
                    } else
                        isl->color.col.dword = orgcol.dword;
                }
                break;
            case ISL_LCUT:
                orgx = isl->lcut.l;
                parent = BloCoder_LookUp(blo, isl->lcut.id);
                if (parent == NULL) limitx = 0;
                else  {
                    if (isl->lcut.orientation == 0)
                        limitx = IntCoord_X(parent->ul);
                    else
                        limitx = IntCoord_Y(parent->ul);
                }
nextdec2:
                if (isl->lcut.l > limitx+1) {
                    isl->lcut.l--;
                    score = BloCoder_ScoreSeq(blo, sseq);
                    if (score < bestscore) {
                        ISLSeq_Destroy(bestseq);
                        bestseq = ISLSeq_Clone(sseq);
                        bestscore = score;
                        orgx = isl->lcut.l;
                        improved = TRUE;
                        goto nextdec2;
                    } else
                    if (score == bestscore) goto nextdec2;
                    else
                        isl->lcut.l = orgx;
                } else 
                    isl->lcut.l = orgx;
                    
                if (isl->lcut.orientation == 0) {
                    if (parent == NULL) limitx = BitMap_SizeX(blo->target);
                    else limitx = IntCoord_X(parent->br);
                } else {
                    if (parent == NULL) limitx = BitMap_SizeY(blo->target);
                    else limitx = IntCoord_Y(parent->br);
                }
nextinc2:
                if (isl->lcut.l < limitx-1) {
                    isl->lcut.l++;
                    score = BloCoder_ScoreSeq(blo, sseq);
                    if (score < bestscore) {
                        ISLSeq_Destroy(bestseq);
                        bestseq = ISLSeq_Clone(sseq);
                        bestscore = score;
                        orgx = isl->lcut.l;
                        improved = TRUE;
                        goto nextinc2;
                    } else
                    if (score == bestscore) goto nextinc2;
                    else
                        isl->lcut.l = orgx;
                } else
                    isl->lcut.l = orgx;
                break;

            case ISL_PCUT:
                orgx = isl->pcut.x;
                orgy = isl->pcut.y;
                parent = BloCoder_LookUp(blo, isl->pcut.id);
                if (parent == NULL) limitx = 0;
                else limitx = IntCoord_X(parent->ul);
                if (parent == NULL) limity = 0;
                else limity = IntCoord_Y(parent->ul);
nextdec3:
                if (isl->pcut.x > limitx+1) {
                    isl->pcut.x--;
                    score = BloCoder_ScoreSeq(blo, sseq);
                    if (score < bestscore) {
                        ISLSeq_Destroy(bestseq);
                        bestseq = ISLSeq_Clone(sseq);
                        bestscore = score;
                        orgx = isl->pcut.x;
                        improved = TRUE;
                        goto nextdec3;
                    } else
                    if (score == bestscore) goto nextdec3;
                    else
                        isl->pcut.x = orgx;
                } else
                    isl->pcut.x = orgx;
nextdec4:
                if (isl->pcut.y > limity+1) {
                    isl->pcut.y--;
                    score = BloCoder_ScoreSeq(blo, sseq);
                    if (score < bestscore) {
                        ISLSeq_Destroy(bestseq);
                        bestseq = ISLSeq_Clone(sseq);
                        bestscore = score;
                        orgy = isl->pcut.y;
                        improved = TRUE;
                        goto nextdec4;
                    } else
                    if (score == bestscore) goto nextdec4;
                    else
                        isl->pcut.y = orgy;
                } else
                    isl->pcut.y = orgy;
                
                if (parent == NULL) limitx = BitMap_SizeX(blo->target);
                else limitx = IntCoord_X(parent->br);
                if (parent == NULL) limity = BitMap_SizeY(blo->target);
                else limity = IntCoord_Y(parent->br);                
nextinc3:
                if (isl->pcut.x < limitx-1) {
                    isl->pcut.x++;
                    score = BloCoder_ScoreSeq(blo, sseq);
                    if (score < bestscore) {
                        ISLSeq_Destroy(bestseq);
                        bestseq = ISLSeq_Clone(sseq);
                        bestscore = score;
                        orgx = isl->pcut.x;
                        improved = TRUE;
                        goto nextinc3;
                    } else
                    if (score == bestscore) goto nextinc3;
                    else
                        isl->pcut.x = orgx;
                } else
                    isl->pcut.x = orgx;
nextinc4:
                if (isl->pcut.y < limity-1) {
                    isl->pcut.y++;
                    score = BloCoder_ScoreSeq(blo, sseq);
                    if (score < bestscore) {
                        ISLSeq_Destroy(bestseq);
                        bestseq = ISLSeq_Clone(sseq);
                        bestscore = score;
                        orgy = isl->pcut.y;
                        goto nextinc4;
                    } else
                    if (score == bestscore) goto nextinc4;
                    else
                        isl->pcut.y = orgy;
                } else
                    isl->pcut.y = orgy;
                break;
            default: ;

            }        
        }
        if (improved) {
            printf("Problem %2i : Found micro improvement, to %i\n", global_prob_id, bestseq->score);
            for (int i=0; i<ISLSeq_Nof(sseq); i++) {
                ISL *isl = ISLSeq_ISL(sseq, i);
                ISL_Destroy(isl);
            }
            sseq->isl->nof_elems = 0;
            ISLSeq_Append(sseq, bestseq);            
        }
    } while (improved);
    BloCoder_Destroy(blo);    
}

/* ------------------------------------------------------------------------ */

void misc_init(void) {
    for (int i=0; i<ARRAY_SIZE(sqroot_table); i++)
        sqroot_table[i] = sqrt(i);
}

/* ------------------------------------------------------------------------ */

void printsol(int id, int bestscore, ISL *sol, int solc) {
   String solfile[128];
   String_Printf(solfile, "%i/%07i.isl", id, bestscore);
   FILE *f = fopen(solfile, "w");
   fprintf(f, "# Solver : blocoder\n");
   fprintf(f, "# Problem : %i\n", id);
   fprintf(f, "# Score : %i\n\n", bestscore);
   if (f != NULL) {
       for (int i=0; i<solc; i++) {
           String *instr = ISL_ToString(sol+i);
           printf("%s\n", instr);
           fprintf(f, "%s\n", instr);
       }
       fclose(f);
   }
}

/* ------------------------------------------------------------------------ */

void BloCoder_Swapper(BloCoder *blo, ISLSeq *bestseq) {
    Bool swapped = FALSE;

    int n = DArray_NofElems(blo->blockdata);
    int **excluded = Int2_CreateVal(n, n, 0);

    do {
        ISLSeq *bestsol = ISLSeq_Create();
        swapped = FALSE;
        bestsol->score = bestseq->score;
        printf("Problem %2i : Attempting swaps, score = %i\n", global_prob_id, bestsol->score);
        for (int i=0; i<DArray_NofElems(blo->blockdata)-1; i++) {
            BlockData *bdi = DArray_Elem(blo->blockdata, i);
            if (bdi->valid) {
                for (int j=i+1; j<DArray_NofElems(blo->blockdata); j++) {
                    BlockData *bdj = DArray_Elem(blo->blockdata, j);
                    if (bdj->valid && !excluded[i][j] && BlockData_IsSwappable(bdi, bdj)) {
                        ISLSeq *sol = ISLSeq_Create();
                        ISL *swp = ISL_CreateSwap(bdi->id, bdj->id);
                        ISLSeq_Add(sol, swp);
                        int sc1 = BloCoder_CanvasScore(blo);
                        if (BloCoder_ScoreSeq(blo, sol) < bestsol->score) {
                            ISLSeq_Destroy(bestsol);                            
                            bestsol = ISLSeq_Clone(sol);
                            printf("Problem %2i : Found swaps, score = %i\n", global_prob_id, bestsol->score);
                            swapped = TRUE;
                            for (int k=0; k<n; k++) {
                                excluded[i][k] = excluded[k][i] = FALSE;
                                excluded[j][k] = excluded[k][j] = FALSE;
                            }
                        } else {
                            excluded[i][j] = excluded[j][i] = TRUE;
                        }
                        ISLSeq_Destroy(sol);
                    }
                }
            }
        }
        if (swapped) {
            BloCoder_ApplySeq(blo, bestsol);
            ISLSeq_Append(bestseq, bestsol);
            bestseq->score = bestsol->score;
            bestseq->canvas_score = bestsol->canvas_score;
            bestseq->opscore += bestsol->opscore;
        }
        ISLSeq_Destroy(bestsol);        
    } while(swapped);
    Int2_Destroy(excluded, n);
}

/* ------------------------------------------------------------------------ */

void BloCoder_MergeAll(BloCoder *blo, ISLSeq *bestseq, Bool orientation) {
    Bool mergeables = FALSE;
    /*
        for each mergeable pair:
        1. evaluate separate paints
        2. evaluate merge + paint
        
        merge/paint pair with best ratio. go on until no more mergeable pairs.
    */
    
    do {
        ISLSeq *bestsol = ISLSeq_Create();
        bestsol->score = MAXINT;
        mergeables = FALSE;
        for (int i=0; i<DArray_NofElems(blo->blockdata)-1; i++) {
            BlockData *bdi = DArray_Elem(blo->blockdata, i);
            if (bdi->valid) {
//                PixelColor medcoli = BloCoder_GeometricMeanColorBD(blo, bdi);
                for (int j=i+1; j<DArray_NofElems(blo->blockdata); j++) {
                    BlockData *bdj = DArray_Elem(blo->blockdata, j);
                    if (bdj->valid && BlockData_IsMergeableOrient(bdi, bdj, orientation)) {
                        ISLSeq *sol = ISLSeq_Create();
/*
                        PixelColor medcolj = BloCoder_GeometricMeanColorBD(blo, bdj);
                        ISLSeq_Add(sol, ISL_CreatePaint(bdi->id, medcoli));
                        ISLSeq_Add(sol, ISL_CreateMerge(bdi->id, bdj->id));
                        if (BloCoder_ScoreSeq(blo, sol) < bestsol->score) {
                            ISLSeq_Destroy(bestsol);
                            bestsol = ISLSeq_Clone(sol);
                        }
                        ISLSeq_Destroy(sol);

                        sol = ISLSeq_Create();
                        ISLSeq_Add(sol, ISL_CreatePaint(bdi->id, medcolj));
                        ISLSeq_Add(sol, ISL_CreateMerge(bdi->id, bdj->id));
                        if (BloCoder_ScoreSeq(blo, sol) < bestsol->score) {
                            ISLSeq_Destroy(bestsol);
                            bestsol = ISLSeq_Clone(sol);
                        }
                        ISLSeq_Destroy(sol);

                        sol = ISLSeq_Create();
*/
                        ISLSeq_Add(sol, ISL_CreateMerge(bdi->id, bdj->id));
                        if (BloCoder_ScoreSeq(blo, sol) < bestsol->score) {
                            ISLSeq_Destroy(bestsol);
                            bestsol = ISLSeq_Clone(sol);
                        }
                        ISLSeq_Destroy(sol);

                        mergeables = TRUE;  
                    }
                }
            }
        }
        if (mergeables) {
            int nof = 0;
            BloCoder_ApplySeq(blo, bestsol);
            ISLSeq_Append(bestseq, bestsol);
            bestseq->score = bestsol->score;
            bestseq->opscore += bestsol->opscore;
            bestseq->canvas_score = bestsol->canvas_score;
            for (int i=0; i<DArray_NofElems(blo->blockdata); i++) {
                BlockData *bdi = DArray_Elem(blo->blockdata, i);
                if (bdi->valid) {
                    printf("%lu ", bdi->id.digits);
                    nof++;
                }                
            }
            
            printf("\nProblem %2i : Best merge has score %i [ %i blocks remains ]\n", global_prob_id, bestsol->score, nof);
        }
        ISLSeq_Destroy(bestsol);
    } while (mergeables);
}

/* ------------------------------------------------------------------------ */

int main(int argc, String **argv) {
    BloCoder *blocoder;
    ISL isl[64];
    int islc = 0;
    int bestscore = MAXINT, opscore = 0;
    int id = -1;
    String *picfile = argv[1];
    Bool improved = TRUE;
    ISLSeq *bestseq = ISLSeq_Create();
    srand(time(0));

    misc_init();

    if (argc < 2) {
        printf("Usage: %s png-file\n", argv[0]);
        return -1;
    }

    Memory_DebugInit(stdout);
 
    blocoder = BloCoder_Create(picfile);
    int n = strlen(picfile)-1;
    while (n>=0 && picfile[n] != '/') n--;
    global_prob_id = id = atoi(picfile+n+1);   
    start_time = Std_Time();
    bestseq->score = BloCoder_CanvasScore(blocoder);
    
    if (DArray_NofElems(blocoder->blockdata) > 1) {
        Bool orientation = rand() & 1;
        BloCoder_Swapper(blocoder, bestseq);
        BloCoder_MergeAll(blocoder, bestseq, orientation);
        BloCoder_MergeAll(blocoder, bestseq, !orientation);
        printf("Problem %2i : preprocessing done, score=%i (%i ops, %i canvas)\n", bestseq->score, bestseq->opscore, bestseq->canvas_score);
    }
    
    while (improved) {
        ISLSeq *bestsol = ISLSeq_Create();
        bestsol->score = bestseq->score;

        improved = FALSE;
 
        BloCoder_Reset(blocoder);
        BloCoder_ApplySeq(blocoder, bestseq);
        opscore = bestseq->opscore;
        
        for (int i=0; i<DArray_NofElems(blocoder->blockdata); i++) {
            BlockData *bd = (BlockData *)DArray_Elem(blocoder->blockdata, i);
            if (bd->valid) {
                ISLSeq *sol = ISLSeq_Create();
                printf("Problem %2i : processing block %i/%i\n", id, i, DArray_NofElems(blocoder->blockdata));
                BloCoder_Solver(blocoder, bd, isl, &islc, 2, opscore, bestscore, TRUE);
                for (int j=0; j<islc; j++) ISLSeq_Add(sol, ISL_Clone(isl+j));
                if (BloCoder_ScoreSeq(blocoder, sol) < bestsol->score) {
                    ISLSeq *temp = ISLSeq_Clone(bestseq);
                    ISLSeq_Append(temp, sol);
                    ISLSeq_CleanRedundantPaints(temp);
                    printf("Problem %2i : writing intermediate step approximate score %i\n---------------------------------\n", id, temp->score);
                    ISLSeq_ToFile(temp, id);
                    printf("----------------------------------\n");
                    ISLSeq_Destroy(temp);

                    ISLSeq_Destroy(bestsol);
                    bestsol = ISLSeq_Clone(sol);
                    ISLSeq_CleanRedundantPaints(bestsol);
                    
                    improved = TRUE;
                }
                ISLSeq_Destroy(sol);
            }
        }
        if (improved) {
            ISLSeq_Append(bestseq, bestsol);
            BloCoder_ISLPostProcess(picfile, bestseq);
            printf("Problem %2i : Writing post-processed solution SCORE %i\n", id, bestseq->score);
            ISLSeq_ToFile(bestseq, id);
        }
    }
#if 0
    ISL sol[1024];
    int solc = 0;

    printf("Problem %i, Canvas score = %i\n", id, BloCoder_CanvasScore(blocoder));   
    while (improved) {
        improved = FALSE;
        for (int i=0; i<DArray_NofElems(blocoder->blockdata); i++) {
            BlockData *bd = (BlockData *)DArray_Elem(blocoder->blockdata, i);
            if (bd->valid) {
                int score = BloCoder_Solver(blocoder, bd, isl, &islc, 1, opscore, bestscore, TRUE);
                if (score < bestscore) {
                    improved = TRUE;
                    for (int j=0; j<islc; j++) {
                        BlocUndo *undo = BloCoder_ApplyISL(blocoder, isl+j);
//                      ISLSeq_Add(bestsofar, isl[j]);                        
                        sol[solc++] = isl[j];
                        opscore += undo->score;
                        BlocUndo_Destroy(undo);
                    }                    
                    printf("Score = %i, opscore = %i, canvas score = %i, old best = %i\n", score, opscore, BloCoder_CanvasScore(blocoder), bestscore);
                    bestscore = score;
                    printsol(id, bestscore, sol, solc);
                    printf("Postprocessing\n");
                }
            }
        }
    }
    printsol(id, bestscore, sol, solc);
#endif
  /*
   int len = BloCoder_CutFill(blocoder, (BlockData *)DArray_Elem(blocoder->blockdata, 0), isl, 2, MAXINT);
   int score = BloCoder_ScoreSeq(blocoder, isl, len);
   printf("Score = %i\n", score);
   for (int i=0; i<len; i++) {
       printf("%s\n", ISL_ToString(isl+i));
   }
   */
/*
   while (XoGUI_Key(gui) != 27) {
       XoGUI_PutImage(gui, -1, blocoder->canvas, 0, 0);
       BloCoder_Update(blocoder);       
   }
*/
   BloCoder_Destroy(blocoder);
   
   Memory_DebugRestore();
   return 0;
}
