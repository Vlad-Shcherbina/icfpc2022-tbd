
#ifndef _pixelcolor_h
#define _pixelcolor_h

#include "cmod.h"

typedef union {
    struct __attribute__((packed)) {
        unsigned char b;
        unsigned char g;
        unsigned char r;
        unsigned char a;
   };
   u32 dword;
   u8 v[4];
} __attribute__((packed)) PixelColor;

extern const PixelColor pixelcolor_black;
extern const PixelColor pixelcolor_blue;
extern const PixelColor pixelcolor_green;
extern const PixelColor pixelcolor_cyan;
extern const PixelColor pixelcolor_red;
extern const PixelColor pixelcolor_magenta;
extern const PixelColor pixelcolor_yellow;
extern const PixelColor pixelcolor_white;
extern const PixelColor pixelcolor_gray;
extern const PixelColor pixelcolor_darkgray;
extern const PixelColor pixelcolor_lightgray;
extern const PixelColor pixelcolor_brown;

extern const PixelColor pixelcolor_brightblue;
extern const PixelColor pixelcolor_brightgreen;
extern const PixelColor pixelcolor_brightcyan;
extern const PixelColor pixelcolor_brightred;
extern const PixelColor pixelcolor_brightmagenta;

void        PixelColor_Init(PixelColor *dst, PixelColor *org);
//PixelColor  PixelColor_FromRGB(int r, int g, int b);
PixelColor  PixelColor_FromRGBA(int r, int g, int b, int a);
#define     PixelColor_FromRGB(r,g,b) PixelColor_FromRGBA(r,g,b,255)
#define     PixelColor_Construct PixelColor_FromRGB
PixelColor  PixelColor_Mix(PixelColor a, PixelColor b, float weight); // 1.0 == 100% from a
PixelColor  PixelColor_Blend256(PixelColor a, PixelColor b, unsigned int w); // w=256 == 100% a
PixelColor  PixelColor_Blend16(PixelColor a, PixelColor b, unsigned int w); // w=16 == 100% a
PixelColor  PixelColor_Blend(PixelColor a, PixelColor b, unsigned int w, unsigned int z);
Bool        PixelColor_IsEqual(PixelColor p1, PixelColor p2);
void        PixeColor_Restore(PixelColor p);
void        PixelColor_Display(PixelColor p, int n);
PixelColor *PixelColor_CreateScale(PixelColor start, PixelColor stop, int steps);
void        PixelColor_DestroyScale(PixelColor *scale);
#define     PixelColor_Blue(p) ((p).b)
#define     PixelColor_Red(p) ((p).r)
#define     PixelColor_Green(p) ((p).g)

#ifdef CMOD_NOMAKEFILE
#include "pixelcolor.c"
#endif

#endif

