
#include "blockdata.h"
#include "isl.h"
#include "memory.h"
#include "stdtypes.h"
#include "string.h"
#include <stdio.h>

const int isl_cost[ISL_NOF] = { 
    [ISL_PCUT] = 10, 
    [ISL_LCUT] = 7, 
    [ISL_COLOR] = 5, 
    [ISL_SWAP] = 3, 
    [ISL_MERGE] = 1 
};

/*--------------------------------------------------------------------------*/

blockid blockid_childid(blockid parent, int n) {
    blockid ret = parent;
    ret.digits <<= 2;
    ret.digits += n;
    ret.nof_digits++;
    return ret;
}

/*--------------------------------------------------------------------------*/

blockid blockid_parentid(blockid child) {
    blockid ret = child;
    ret.digits >>= 2;
    ret.nof_digits--;
    return ret;
}

/*--------------------------------------------------------------------------*/

String *blockid_str(blockid id) {
    static String ss[16][80];
    volatile static int scount = 0;
    String *sb = ss[scount++ & 0xf];
    if (id.nof_digits > 0) {
        if (id.nof_digits > 29) { fprintf(stderr, "Digit overflow\n"); return NULL; }
        String *s = sb + sprintf(sb, "%lu", (u64)(id.digits >> id.nof_digits*2));
        for (int i=id.nof_digits-1; i>=0; i--) {
            u8 a = (id.digits >> i*2) & 3;
            s += sprintf(s, ".%i", a);
        }
    } else
        sprintf(sb, "%lu", (u64)id.digits);
    return sb;
}

/*--------------------------------------------------------------------------*/

String *ISL_ToString(const ISL *isl) {
    switch (isl->opcode) {
    case ISL_PCUT:
        return String_CreatePrintf("cut [%s] [%i,%i]", blockid_str(isl->pcut.id), isl->pcut.x, isl->pcut.y);
    case ISL_LCUT:
        return String_CreatePrintf("cut [%s] [%c] [%i]", blockid_str(isl->lcut.id), 'x'+isl->lcut.orientation, isl->lcut.l);    
    case ISL_SWAP:
        return String_CreatePrintf("swap [%s] [%s]", blockid_str(isl->swap.id[0]), blockid_str(isl->swap.id[1]));
    case ISL_MERGE:
        return String_CreatePrintf("merge [%s] [%s]", blockid_str(isl->merge.id[0]), blockid_str(isl->merge.id[1]));
    case ISL_COLOR:
        return String_CreatePrintf("color [%s] [%i,%i,%i,%i]", blockid_str(isl->color.id), isl->color.col.r, isl->color.col.g, isl->color.col.b, isl->color.col.a);
    default:
        return String_CreatePrintf("# unknown opcode 0x%02x\n", isl->opcode);
    }
}

/*--------------------------------------------------------------------------*/

void ISL_Print(const ISL *isl) {
    String *s = ISL_ToString(isl);
    printf("%s", s);
    String_Destroy(s);
}

/*--------------------------------------------------------------------------*/
#if 0
void ISL_LCut(ISL *isl, const BlockData *bd, int o, int l) {
    isl->opcode = ISL_LCUT;
    isl->lcut.id = bd->id;
    isl->lcut.orientation = o;
    isl->lcut.l = l;
}

/*--------------------------------------------------------------------------*/

void ISL_PCut(ISL *isl, const BlockData *bd, int x, int y) {
    isl->opcode = ISL_PCUT;
    isl->pcut.id = bd->id;
    isl->pcut.x = x;
    isl->pcut.y = y;
}

/*--------------------------------------------------------------------------*/

void ISL_Paint(ISL *isl, const BlockData *bd, PixelColor a) {
    isl->opcode = ISL_COLOR;
    isl->color.id = bd->id;
    isl->color.col = a;
}
#endif

/*--------------------------------------------------------------------------*/

ISL *ISL_Create(void) {
    ISL *isl = Memory_Reserve(1, ISL);
    return isl;
}

/*--------------------------------------------------------------------------*/

void ISL_LCut(ISL *isl, blockid bid, int o, int l) {
    isl->opcode = ISL_LCUT;
    isl->lcut.id = bid;
    isl->lcut.orientation = o;
    isl->lcut.l = l;
}

/*--------------------------------------------------------------------------*/

void ISL_PCut(ISL *isl, blockid bid, int x, int y) {
    isl->opcode = ISL_PCUT;
    isl->pcut.id = bid;
    isl->pcut.x = x;
    isl->pcut.y = y;
}

/*--------------------------------------------------------------------------*/

void ISL_Paint(ISL *isl, blockid bid, PixelColor a) {
    isl->opcode = ISL_COLOR;
    isl->color.id = bid;
    isl->color.col = a;
}

/*--------------------------------------------------------------------------*/

void ISL_Merge(ISL *isl, blockid bid1, blockid bid2) {
    isl->opcode = ISL_MERGE;
    isl->merge.id[0] = bid1;
    isl->merge.id[1] = bid2;
}

/*--------------------------------------------------------------------------*/

void ISL_Swap(ISL *isl, blockid bid1, blockid bid2) {
    isl->opcode = ISL_SWAP;
    isl->swap.id[0] = bid1;
    isl->swap.id[1] = bid2;
}

/*--------------------------------------------------------------------------*/

ISL *ISL_CreateLCut(blockid bid, int o, int l) {
    ISL *isl = ISL_Create();
    ISL_LCut(isl, bid, o, l);
    return isl;
}

/*--------------------------------------------------------------------------*/

ISL *ISL_CreatePCut(blockid bid, int x, int y) {
    ISL *isl = ISL_Create();
    ISL_PCut(isl, bid, x, y);
    return isl;
}

/*--------------------------------------------------------------------------*/

ISL *ISL_CreatePaint(blockid bid, PixelColor a) {
    ISL *isl = ISL_Create();
    ISL_Paint(isl, bid, a);
    return isl;
}

/*--------------------------------------------------------------------------*/

ISL *ISL_CreateMerge(blockid bid1, blockid bid2) {
    ISL *isl = ISL_Create();
    ISL_Merge(isl, bid1, bid2);
    return isl;
}

/*--------------------------------------------------------------------------*/

ISL *ISL_CreateSwap(blockid bid1, blockid bid2) {
    ISL *isl = ISL_Create();
    ISL_Swap(isl, bid1, bid2);
    return isl;
}

/*--------------------------------------------------------------------------*/

Bool ISL_IsEqualPaint(ISL *isl1, ISL *isl2) {
    return isl1->opcode == isl2->opcode && isl1->color.id.qword == isl2->color.id.qword && isl1->color.col.dword == isl2->color.col.dword;
}

/*--------------------------------------------------------------------------*/

ISL *ISL_Clone(const ISL *isl_org) {
    ISL *isl = Memory_Reserve(1, ISL);
    *isl = *isl_org;
    return isl;
}

/*--------------------------------------------------------------------------*/

void ISL_Destroy(ISL *isl) { Memory_Free(isl, ISL); }

/*--------------------------------------------------------------------------*/

void ISL_Construct(ISL *isl, int opcode, ... );
void ISL_Copy(ISL *dst, const ISL *src);    

