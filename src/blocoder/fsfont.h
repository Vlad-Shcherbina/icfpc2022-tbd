#ifndef _fsfont_h
#define _fsfont_h

#include "bitmap.h"
#include "cmod.h"
#include "darray.h"
#include "pixelcolor.h"
#ifdef CMOD_SERIALIZED
#include "serialized.h"
#endif
#include "string.h"

typedef struct {
	int     startchar;
	DArray *glyph;
	int     width;
	int     height;
	int     baseline;
} FSFont;

void    FSFont_Init(FSFont *fsfont_new, FSFont *fsfont_org);
FSFont *FSFont_Create();
FSFont *FSFont_ReadBDFFile(String *filename);
FSFont *FSFont_Default(); /* A built-in 8x16 font */
#define FSFont_CreateFromBDFFile FSFont_ReadBDFFile
FSFont *FSFont_Clone(FSFont *FSFont);
#ifdef CMOD_SERIALIZED
void    FSFont_Serialize(FSFont *fsfont, Serialized *ser);
FSFont *FSFont_DeSerialize(Serialized *ser);
#endif
void    FSFont_PutText(FSFont *fsfont, BitMap *bm, int x, int y, const String *text, PixelColor color);
void    FSFont_PutThickText(FSFont *fsfont, BitMap *bm, int x, int y, const String *text, PixelColor color, int thickness);
void    FSFont_PutTextBG(FSFont *fsfont, BitMap *bm, int x, int y, const String *text, PixelColor color, PixelColor bg);
void    FSFont_PutTextCenter(FSFont *fsfont, BitMap *bm, int x, int y, const String *text, PixelColor color);
void    FSFont_PutThickTextCenter(FSFont *fsfont, BitMap *bm, int x, int y, const String *text, PixelColor color, int thickness);
void    FSFont_PutTextRightAligned(FSFont *fsfont, BitMap *bm, int x, int y, const String *text, PixelColor color);
void    FSFont_PutThickTextRightAligned(FSFont *fsfont, BitMap *bm, int x, int y, const String *text, PixelColor color, int thickness);
BitMap *FSFont_Glyph(FSFont *fsfont, int encoding);
#define FSFont_StartChar(fsfont) ((fsfont)->startchar)
#define FSFont_Character(fsfont,car) ((BitMap *)DArray_Elem((fsfont)->glyph, (car-(fsfont)->startchar)))
//#define FSFont_Glyph(fsfont,ix) ((BitMap *)DArray_Elem((fsfont)->glyph, (ix)))
#define FSFont_Width(fsfont) ((fsfont)->width)
#define FSFont_Height(fsfont) ((fsfont)->height)
#define FSFont_Baseline(fsfont) ((fsfont)->baseline)
int     FSFont_TextWidth(FSFont *f, const String *text);
void    FSFont_SetGlyph(FSFont *f, BitMap *glyph, int id);

void    FSFont_Restore(FSFont *fsfont);
void    FSFont_Destroy(FSFont *fsfont);

#ifdef CMOD_DEBUG_OUTPUT
void    FSFont_Display(FSFont *fsfont, int n);
#endif

#ifdef CMOD_NOMAKEFILE
#include "fsfont.c"
#endif
#endif

