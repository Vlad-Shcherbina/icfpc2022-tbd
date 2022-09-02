
#include "isl.h"
#include "string.h"
#include <stdio.h>

const int isl_cost[ISL_NOF] = { 10, 7, 5, 3, 1 };

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
        if (id.nof_digits > 30) { fprintf(stderr, "Digit overflow\n"); return NULL; }
        String *s = sb + sprintf(sb, "%lu", (u64)(id.digits >> id.nof_digits*2));
        for (unsigned int i=id.nof_digits-1; i<id.nof_digits; i++) {
            u8 a = (id.digits >> i*2) & 3;
            s += sprintf(s, ".%i", a);
        }
    } else
        sprintf(sb, "%lu", (u64)id.digits);
    return sb;
}

/*--------------------------------------------------------------------------*/

String *ISL_ToString(ISL *isl) {
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

void ISL_Construct(ISL *isl, int opcode, ... );
void ISL_Copy(ISL *dst, const ISL *src);    

