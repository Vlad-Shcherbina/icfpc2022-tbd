#ifndef _bitmap_h
#define _bitmap_h

#include "cmod.h"
#include "intcoord.h"
#include "pixelcolor.h"
#include "string.h"
#ifdef CMOD_SERIALIZED
#include "serialized.h"
#endif

#define CMOD_PNG

typedef struct {
	char        type;
	char        bpp;
	int         sizex;
	int         sizey;
	int         clip_rect_x1;
	int         clip_rect_x2;
	int         clip_rect_y1;
	int         clip_rect_y2;
	int        *multab;
	PixelColor *buf;
} BitMap;

void       BitMap_Init(BitMap *bitmap_new, const BitMap *bitmap_org);
BitMap    *BitMap_Create(int sizex, int sizey);
BitMap    *BitMap_CreateVal(int sizex, int sizey, PixelColor c);
BitMap    *BitMap_CreateStretched(const BitMap *src, int width, int height); /* linear subpixel interpolation, slowish */
BitMap    *BitMap_Clone(const BitMap *BitMap);
BitMap    *BitMap_CloneGrayed(const BitMap *bitmap);
Bool       BitMap_IsEqual(BitMap *bitmap1, BitMap *bitmap2);
#ifdef CMOD_SERIALIZED
void       BitMap_Serialize(BitMap *bitmap, Serialized *ser);
BitMap    *BitMap_DeSerialize(Serialized *ser);
#endif
#ifdef CMOD_PNG
void       BitMap_SavePNGFile(BitMap *bm, const String *filename);
#define    BitMap_StorePNGFile BitMap_SavePNGFile 
#define    BitMap_WritePNGFile BitMap_SavePNGFile 
BitMap    *BitMap_ReadPNGFile(const String *filename);
#define    BitMap_LoadPNGFile BitMap_ReadPNGFile
#define    BitMap_LoadPNG BitMap_LoadPNGFile
void       BitMap_SaveRAWFile(BitMap *bm, const String *filename);
#define    BitMap_StoreRawFile BitMap_SaveRAWFile 
#define    BitMap_SaveRaw BitMap_SaveRAWFile 
#define    BitMap_DumpRaw BitMap_SaveRAWFile 
#define    BitMap_RawDump BitMap_SaveRAWFile 
#endif
void       BitMap_Resize(BitMap *bitmap, int newsizex, int newsizey, Bool copy);
void       BitMap_SetPixel(BitMap *bitmap, int x, int y, PixelColor color);
void       BitMap_PutPixelAlpha(BitMap *bitmap, int x, int y, PixelColor color);
#define    BitMap_SetPixelRGB(bm, x, y, r, g, b) BitMap_SetPixel((bm),(x),(y),(PixelColor_Construct(r,g,b)))
PixelColor BitMap_Pixel(const BitMap *bitmap, int x, int y);
#define    BitMap_PutPixel BitMap_SetPixel
#define    BitMap_GetPixel BitMap_Pixel 
void       BitMap_SetClipRect(BitMap *bitmap, int x1, int y1, int x2, int y2);
void       BitMap_ResetClipRect(BitMap *bitmap);
void       BitMap_ReplacePixels(BitMap *bitmap, int x1, int y1, int x2, int y2, PixelColor src, PixelColor dst);
void       BitMap_DrawLine(BitMap *bitmap, int x1, int x2, int y1, int y2, PixelColor color);
void       BitMap_DrawLineAA(BitMap *bitmap, int x1, int x2, int y1, int y2, PixelColor color);
void       BitMap_DrawLinePattern(BitMap *bitmap, int x1, int y1, int x2, int y2, int n, PixelColor *color, int start, int thickness);
void       BitMap_FillRectangle(BitMap *bitmap, int x1, int y1, int x2, int y2, PixelColor color);
#define    BitMap_FillRect BitMap_FillRectangle
#define    BitMap_Clear(bm) BitMap_FillRectangle((bm), 0, 0, (bm)->sizex-1, (bm)->sizey-1, pixelcolor_black)
void       BitMap_DrawRectangle(BitMap *bitmap, int x1, int y1, int x2, int y2, PixelColor color);
void       BitMap_DrawThickRectangle(BitMap *bitmap, int x1, int y1, int x2, int y2, PixelColor color, int thickness);
#define    BitMap_DrawRect BitMap_DrawRectangle
#define    BitMap_DrawThickRect BitMap_DrawThickRectangle
void       BitMap_FillCircle(BitMap *bitmap, int cx, int cy, int r, PixelColor color);
void       BitMap_Circle(BitMap *bitmap, int cx, int cy, int r, PixelColor color);
void       BitMap_FloodFill(BitMap *bm, IntCoord pos, PixelColor initial, PixelColor p); /* Slow as helly */
void       BitMap_DrawView(BitMap *canvas, BitMap *src, int dx1, int dy1, int dx2, int dy2, int sx, int sy, int scale, int stx, int sty);
void       BitMap_PutBitMap(BitMap *canvas, const BitMap *image, int x, int y);
BitMap    *BitMap_GetBitMap(const BitMap *org, int x1, int x2, int y1, int y2);
void       BitMap_PutBitMapRecolour(BitMap *canvas, const BitMap *image, int x, int y, PixelColor color);
void       BitMap_PutBitMapAlpha(BitMap *canvas, const BitMap *image, int x, int y);
void       BitMap_TextureMapRect(BitMap *canvas, BitMap *texture, int tlx, int tly,  int trx, int _try, int blx, int bly,  int brx, int bry, int alpha);
void       BitMap_PutBitMapScanLineAlpha(BitMap *canvas, const BitMap *image, int canvasx, int canvasy, int imagex, int imagey, int length);
void       BitMap_HLine(BitMap *bm, int x1, int x2, int y, PixelColor c);
void       BitMap_VLine(BitMap *bm, int x, int y1, int y2, PixelColor c);
void       BitMap_PushButtonRect(BitMap *bm, int x1, int y1, int x2, int y2, PixelColor l, PixelColor u, PixelColor r, PixelColor d, int thickness);
void       BitMap_PushButton(BitMap *bm, int x1, int y1, int x2, int y2, PixelColor middle, PixelColor l, PixelColor u, PixelColor r, PixelColor d, int thickness);
void       BitMap_RoundedPushButton(BitMap *bm, int x1, int y1, int x2, int y2, PixelColor middle, PixelColor l, PixelColor u, PixelColor r, PixelColor d, int thickness);
//void       BitMap_DoPushButton(BitMap *canvas, int x1, int y1, int x2, int y2, int thickness, Bool pushed);
void       BitMap_SetAlpha(BitMap *bm, unsigned char alpha);
void       BitMap_FlipY(BitMap *bm);
//BitMap    *BitMap_GetImage(BitMap *org, int x1, int x2, int y1, int y2);

#define BitMap_Type(bitmap) ((bitmap)->type)
#define BitMap_SetType(bitmap, t) (((bitmap)->type) = (t))
#define BitMap_Bpp(bitmap) ((bitmap)->bpp)
#define BitMap_SetBpp(bitmap, b) (((bitmap)->bpp) = (b))
#define BitMap_SizeX(bitmap) ((bitmap)->sizex)
#define BitMap_SizeY(bitmap) ((bitmap)->sizey)
#define BitMap_Width BitMap_SizeX
#define BitMap_Height BitMap_SizeY
#define BitMap_ClipRectX1(bitmap) ((bitmap)->clip_rect_x1)
#define BitMap_SetClipRectX1(bitmap, c) (((bitmap)->clip_rect_x1) = (c))
#define BitMap_ClipRectX2(bitmap) ((bitmap)->clip_rect_x2)
#define BitMap_SetClipRectX2(bitmap, c) (((bitmap)->clip_rect_x2) = (c))
#define BitMap_ClipRectY1(bitmap) ((bitmap)->clip_rect_y1)
#define BitMap_SetClipRectY1(bitmap, c) (((bitmap)->clip_rect_y1) = (c))
#define BitMap_ClipRectY2(bitmap) ((bitmap)->clip_rect_y2)
#define BitMap_SetClipRectY2(bitmap, c) (((bitmap)->clip_rect_y2) = (c))
#define BitMap_Multab(bitmap) ((bitmap)->multab)
#define BitMap_Buf(bitmap) ((bitmap)->buf)
void    BitMap_PrintHTML(BitMap *bm);
void    BitMap_SetBuf(BitMap *bitmap, PixelColor *buf);
void    BitMap_SetSize(BitMap *bitmap, int x, int y);

void    BitMap_Restore(BitMap *bitmap);
void    BitMap_Destroy(BitMap *bitmap);

#ifdef CMOD_NOMAKEFILE
#include "bitmap.c"
#endif
#endif

