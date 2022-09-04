
#ifndef _isl_h
#define _isl_h

#include "pixelcolor.h"
#include "string.h"
#include <stdarg.h>

typedef enum {
    ISL_PCUT,
    ISL_LCUT,
    ISL_COLOR,
    ISL_SWAP,
    ISL_MERGE,
    ISL_NOF
} Opcode;

#define ISL_PAINT ISL_COLOR
#define ISL_NOOP ISL_NOF

extern const int isl_cost[ISL_NOF];

typedef union {
    u64 qword;
    struct __attribute__((packed)) {
        unsigned nof_digits : 10;
        u64 digits : 54;
    };
} blockid;


typedef struct __attribute__((packed)) {
    int opcode;
    union {
        struct {
            blockid id;
            int x, y;
        } pcut;
        struct {
            blockid id;
            int orientation;
            int l;
        } lcut;
        struct {
            blockid id;
            PixelColor col;        
        } color;
        struct {
            blockid id[2];
        } swap;
        struct {
            blockid id[2];
        } merge;    
    };
} ISL;

blockid blockid_childid(blockid parent, int n);
blockid blockid_parentid(blockid child);
String *blockid_str(blockid id);

String *ISL_ToString(const ISL *isl);
void ISL_Print(const ISL *isl);
/*
void ISL_LCut(ISL *isl, const BlockData *bd, int o, int l);
void ISL_PCut(ISL *isl, const BlockData *bd, int x, int y);
void ISL_Paint(ISL *isl, const BlockData *bd, PixelColor a);
*/
void ISL_LCut(ISL *isl, blockid bid, int o, int l);
void ISL_PCut(ISL *isl, blockid bid, int x, int y);
void ISL_Paint(ISL *isl, blockid bid, PixelColor a);
#define ISL_Color ISL_Paint
#define ISL_Colour ISL_Paint
Bool ISL_IsEqualPaint(ISL *isl1, ISL *isl2);
ISL *ISL_Clone(const ISL *isl_org);
void ISL_Construct(ISL *isl, int opcode, ... );
void ISL_Copy(ISL *dst, const ISL *src);    
void ISL_Destroy(ISL *isl);

#ifdef CMOD_NOMAKEFILE
#include "isl.c"
#endif
    
#endif
