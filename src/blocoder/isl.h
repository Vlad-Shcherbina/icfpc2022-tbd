
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

extern const int isl_cost[ISL_NOF];

typedef union {
    u64 qword;
    struct __attribute__((packed)) {
        unsigned nof_digits : 6;
        u64 digits : 58;
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

String *ISL_ToString(ISL *isl);
void ISL_Construct(ISL *isl, int opcode, ... );
void ISL_Copy(ISL *dst, const ISL *src);    

#ifdef CMOD_NOMAKEFILE
#include "isl.c"
#endif
    
#endif
