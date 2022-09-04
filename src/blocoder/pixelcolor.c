
#include "cmod.h"
#include "pixelcolor.h"
#include "memory.h"
#include "stdtypes.h"

#include <stdio.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
const PixelColor pixelcolor_black = {0,0,0,255};
const PixelColor pixelcolor_blue = {200,0,0,255};
const PixelColor pixelcolor_green = {0,200,0,255};
const PixelColor pixelcolor_cyan = {200,200,0,255};
const PixelColor pixelcolor_red = {0,0,200,255};
const PixelColor pixelcolor_magenta = {200,0,200,255};
const PixelColor pixelcolor_yellow = {0,255,255,255};
const PixelColor pixelcolor_white = {255,255,255,255};
const PixelColor pixelcolor_gray = {160, 160, 160, 255};
const PixelColor pixelcolor_darkgray = {80,80,80, 255};
const PixelColor pixelcolor_lightgray = {210, 210, 210, 255};
const PixelColor pixelcolor_brown = {0, 120, 130, 255};

const PixelColor pixelcolor_brightblue = {255,50,50,255};
const PixelColor pixelcolor_brightgreen = {50,255,50,255};
const PixelColor pixelcolor_brightcyan = {255,255,50,255};
const PixelColor pixelcolor_brightred = {50,50,255,255};
const PixelColor pixelcolor_brightmagenta = {255,50,255,255};
#pragma GCC diagnostic pop

/* --------------------------------------------------------------------- */

void PixelColor_Init(PixelColor *dst, PixelColor *org) {
   dst->r = org->r;
   dst->g = org->g;
   dst->b = org->b;
   dst->a = org->a;
}

/* --------------------------------------------------------------------- */

PixelColor PixelColor_FromRGBA(int r, int g, int b, int a) {
   PixelColor p;
   p.r = r;
   p.g = g;
   p.b = b;
   p.a = a;
   return p;
}

/* --------------------------------------------------------------------- */
/*
PixelColor PixelColor_FromRGB(int r, int g, int b) {
   PixelColor c;
   c.a = 0;
   c.r = r;
   c.g = g;
   c.b = b;
   return c;
}
*/
/* --------------------------------------------------------------------- */

#pragma GCC push_options
#pragma GCC optimize("3,omit-frame-pointer")
PixelColor PixelColor_Mix(PixelColor a, PixelColor b, float w1) {
   PixelColor c;
   float w2 = 1.0 - w1;
   c.a = a.a*w1 + b.a*w2;
   c.r = a.r*w1 + b.r*w2;
   c.g = a.g*w1 + b.g*w2;
   c.b = a.b*w1 + b.b*w2;
   return c;
}

/* --------------------------------------------------------------------- */

PixelColor PixelColor_Blend256(PixelColor a, PixelColor b, unsigned int w) {
   PixelColor res;
   int z = 255 - w;
   res.r = (w*a.r + z*b.r)/255;
   res.g = (w*a.g + z*b.g)/255;
   res.b = (w*a.b + z*b.b)/255;
   res.a = (w*a.a + z*b.a)/255;
   return res;
/*
   unsigned int arb, ag, brb, bg, resba, resg, z, res;
   
   arb = *(unsigned int *)&a & 0xff00ff;
   brb = *(unsigned int *)&b & 0xff00ff;
   ag = *(unsigned int *)&a & 0x00ff00;
   bg = *(unsigned int *)&b & 0x00ff00;   
   z = 255 - w;
   resba = w*arb + z*brb;
   resg = w*ag + z*bg;
   resba /= 255;
   resg /= 255;
   res = (resba & 0xff00ff);
   res += (resg & 0xff00);
   return *(PixelColor *)&res;
*/
}

/* --------------------------------------------------------------------- */

inline PixelColor PixelColor_Blend(PixelColor a, PixelColor b, unsigned int w, unsigned int z) {
   unsigned int ar, ab, ag, br, bb, bg, resr, resg, resb, res;
   
   if (w == 0 && z == 0) return a;
   ar = *(unsigned int *)&a & 0x0000ff;
   br = *(unsigned int *)&b & 0x0000ff;
   ag = (*(unsigned int *)&a & 0x00ff00) >> 8;
   bg = (*(unsigned int *)&b & 0x00ff00) >> 8;   
   ab = (*(unsigned int *)&a & 0xff0000) >> 16;
   bb = (*(unsigned int *)&b & 0xff0000) >> 16;
   resr = w*ar + z*br;
   resg = w*ag + z*bg;
   resb = w*ab + z*bb;
   resr /= (w+z);
   resg /= (w+z);
   resb /= (w+z);
   res = (resr & 0xff) + ((resg & 0xff) << 8) + ((resb & 0xff) << 16);
   return *(PixelColor *)&res;
}
#pragma GCC pop_options
/* --------------------------------------------------------------------- */

Bool PixelColor_IsEqual(PixelColor p1, PixelColor p2) { return (p1.r == p2.r && p1.g == p2.g && p1.b == p2.b); }

/* --------------------------------------------------------------------- */

PixelColor *PixelColor_CreateScale(PixelColor start, PixelColor stop, int steps) {
   PixelColor *scale;
   int i;
   scale = Memory_Reserve(steps, PixelColor);
   for (i=0; i<steps; i++) {
      scale[i].r = start.r + (i*((int)stop.r - (int)start.r))/(steps-1);
      scale[i].g = start.g + (i*((int)stop.g - (int)start.g))/(steps-1);
      scale[i].b = start.b + (i*((int)stop.b - (int)start.b))/(steps-1);
//      PixelColor_Display(scale[i], 1);
   }
   return scale;
}

/* --------------------------------------------------------------------- */

void PixelColor_DestroyScale(PixelColor *scale) { Memory_Free(scale, PixelColor); }

/* --------------------------------------------------------------------- */

void PixeColor_Restore(PixelColor p) {} 

/* --------------------------------------------------------------------- */

void PixelColor_Display(PixelColor p, int n) {
   printf("%i %i %i  %i\n", p.r, p.g, p.b, p.a);
}

