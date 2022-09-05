
#include "bitmap.h"
#include "cmod.h"
#include "intcoord.h"
#include "darray.h"
#include "memory.h"
#include "pixelcolor.h"
#include "stdfunc.h"
#ifdef CMOD_SERIALIZED
#include "serialized.h"
#endif
#ifdef CMOD_PNG
#include <png.h>
#endif

/*--------------------------------------------------------------------------*/

void BitMap_Init(BitMap *bitmap, const BitMap *bitmap_org) {

	if (bitmap_org == NULL) {
		bitmap->type = 0;
		bitmap->bpp = 0;
		bitmap->sizex = 0;
		bitmap->sizey = 0;
		bitmap->clip_rect_x1 = 0;
		bitmap->clip_rect_x2 = 0;
		bitmap->clip_rect_y1 = 0;
		bitmap->clip_rect_y2 = 0;
		bitmap->multab = NULL;
		bitmap->buf = NULL;
	} else {
		bitmap->type = bitmap_org->type;
		bitmap->bpp = bitmap_org->bpp;
		bitmap->sizex = bitmap_org->sizex;
		bitmap->sizey = bitmap_org->sizey;
		bitmap->clip_rect_x1 = bitmap_org->clip_rect_x1;
		bitmap->clip_rect_x2 = bitmap_org->clip_rect_x2;
		bitmap->clip_rect_y1 = bitmap_org->clip_rect_y1;
		bitmap->clip_rect_y2 = bitmap_org->clip_rect_y2;
		bitmap->multab = Memory_Reserve(bitmap->sizey, int);
      Memory_Copy(bitmap->multab, bitmap_org->multab, bitmap->sizey*sizeof(int));
		bitmap->buf = Memory_Reserve(bitmap->sizex*bitmap->sizey, PixelColor);
      Memory_Copy(bitmap->buf, bitmap_org->buf, bitmap->sizex*bitmap->sizey*sizeof(PixelColor));
	}
}

/*--------------------------------------------------------------------------*/

BitMap *BitMap_Create(int w, int h) {
	BitMap *bitmap;
   register int y, s;

	bitmap = Memory_Reserve(1, BitMap);
	bitmap->type = 0;
	bitmap->bpp = sizeof(PixelColor)*8;
	bitmap->sizex = w;
	bitmap->sizey = h;
	bitmap->clip_rect_x1 = 0;
	bitmap->clip_rect_x2 = w-1;
   bitmap->clip_rect_y1 = 0;
	bitmap->clip_rect_y2 = h-1;
	bitmap->multab = Memory_Reserve(bitmap->sizey, int);
	bitmap->buf = Memory_Reserve(bitmap->sizey * bitmap->sizex, PixelColor);
   s = 0;
   for (y=0; y<bitmap->sizey; y++) {
      bitmap->multab[y] = s;
      s+=bitmap->sizex;
   }

	return bitmap;
}

/*--------------------------------------------------------------------------*/

BitMap *BitMap_CreateVal(int sizex, int sizey, PixelColor c) {
   BitMap *bm = BitMap_Create(sizex, sizey);
   BitMap_FillRect(bm, 0, 0, sizex-1, sizey-1, c);
   return bm;
}

/*--------------------------------------------------------------------------*/

BitMap *BitMap_Clone(const BitMap *bitmap) {
	BitMap *bitmap2;

	bitmap2 = Memory_Reserve(1, BitMap);
	BitMap_Init(bitmap2, bitmap);

	return bitmap2;
}

/*--------------------------------------------------------------------------*/

BitMap *BitMap_CloneGrayed(const BitMap *bitmap) {
        BitMap *bm2 = BitMap_Clone(bitmap);
        int x, y;
        for (y=0; y<BitMap_SizeY(bm2); y++) {
                for (x=0; x<BitMap_SizeX(bm2); x++) {
                        PixelColor c = BitMap_GetPixel(bm2, x, y);
                        unsigned char m = 2*MAX(c.r, MAX(c.g, c.b))/3;
                        c.r = c.g = c.b = m;
                        BitMap_PutPixel(bm2, x, y, c);
                }
        }
        return bm2;
}

/*--------------------------------------------------------------------------*/

#define BITMAP_SCALEGRAN 256

/* Creates a stretched bitmap with subpixel accuracy */
BitMap *BitMap_CreateStretched(const BitMap *src, int width, int height) {
   int x, y, srcx, srcy, rx, ry, rxy, sx, sy, dx, dy;
   int m11, m12, m21, m22;
   BitMap *res;   
   union {
      PixelColor p;
      unsigned int v;
   } r11, r12, r21, r22, p;
   
   if (width < 0 || height < 0) return NULL;
   res = BitMap_Create(width, height);
   if (width == 0 || height == 0) return res;
   
   dy = ((BitMap_SizeY(src)) * BITMAP_SCALEGRAN - 1) / height;
   dx = ((BitMap_SizeX(src)) * BITMAP_SCALEGRAN - 1) / width;
   srcy = 0;
   for (y=0; y<height; y++) {      
      sy = srcy / BITMAP_SCALEGRAN;
      ry = srcy % BITMAP_SCALEGRAN;
      srcx = 0;
      for (x=0; x<width; x++) {      
         sx = srcx / BITMAP_SCALEGRAN;
         rx = srcx % BITMAP_SCALEGRAN;
         rxy = rx*ry / BITMAP_SCALEGRAN;

         m11 = (BITMAP_SCALEGRAN - rx - ry + rxy);
         m21 = ry - rxy;
         m12 = rx - rxy;
         m22 = rxy;
         r11.p = BitMap_GetPixel(src, sx, sy);
         r12.p = BitMap_GetPixel(src, sx+1, sy);
         r21.p = BitMap_GetPixel(src, sx, sy+1);
         r22.p = BitMap_GetPixel(src, sx+1, sy+1);
                  
         p.v = ((r11.v & 0x00ff00ff) * m11 
            + (r12.v & 0x00ff00ff) * m12
            + (r21.v & 0x00ff00ff) * m21
            + (r22.v & 0x00ff00ff) * m22) / BITMAP_SCALEGRAN;            
         p.p.g = (r11.p.g * m11 + r12.p.g * m12 + r21.p.g * m21 + r22.p.g * m22) / BITMAP_SCALEGRAN;
         p.p.a = (r11.p.a * m11 + r12.p.a * m12 + r21.p.a * m21 + r22.p.a * m22) / BITMAP_SCALEGRAN;
         
         BitMap_SetPixel(res, x, y, p.p);
                  
         srcx += dx;         
      }
      srcy += dy;
   }
   return res;
}
#undef BITMAP_SCALEGRAN

/*--------------------------------------------------------------------------*/
/*
Bool BitMap_IsEqual(BitMap *bitmap1, BitMap *bitmap2) {
	return (bitmap1->type == bitmap2->type)
	&& (bitmap1->bpp == bitmap2->bpp)
	&& (bitmap1->sizex == bitmap2->sizex)
	&& (bitmap1->sizey == bitmap2->sizey)
	&& (bitmap1->clip_rect_x1 == bitmap2->clip_rect_x1)
	&& (bitmap1->clip_rect_x2 == bitmap2->clip_rect_x2)
	&& (bitmap1->clip_rect_y1 == bitmap2->clip_rect_y1)
	&& (bitmap1->clip_rect_y2 == bitmap2->clip_rect_y2)
	&& PixelColor_IsEqual(bitmap1->buf, bitmap2->buf);
}
*/
/*--------------------------------------------------------------------------*/
#ifdef CMOD_SERIALIZED
void BitMap_Serialize(BitMap *bitmap, Serialized *ser) {
   int s;
	Serialized_AppendData(ser, &bitmap->type, sizeof(bitmap->type));
	Serialized_AppendData(ser, &bitmap->bpp, sizeof(bitmap->bpp));
	Serialized_AppendWrapped(ser, &bitmap->sizex, sizeof(bitmap->sizex));
	Serialized_AppendWrapped(ser, &bitmap->sizey, sizeof(bitmap->sizey));
	Serialized_AppendWrapped(ser, &bitmap->clip_rect_x1, sizeof(bitmap->clip_rect_x1));
	Serialized_AppendWrapped(ser, &bitmap->clip_rect_x2, sizeof(bitmap->clip_rect_x2));
	Serialized_AppendWrapped(ser, &bitmap->clip_rect_y1, sizeof(bitmap->clip_rect_y1));
	Serialized_AppendWrapped(ser, &bitmap->clip_rect_y2, sizeof(bitmap->clip_rect_y2));
   
   for (s=0; s<bitmap->sizey; s++) {
      Serialized_AppendWrapped(ser, &bitmap->multab[s], sizeof(int));
   }

   for (s=0; s<bitmap->sizey * bitmap->sizex; s++)
      Serialized_AppendWrapped(ser, &bitmap->buf[s], sizeof(PixelColor));   
}

/*--------------------------------------------------------------------------*/

BitMap *BitMap_DeSerialize(Serialized *ser) {
	BitMap *bitmap;
   int s;
   
	bitmap = Memory_Reserve(1, BitMap);
	Serialized_ReadData(ser, &bitmap->type, sizeof(bitmap->type));
	Serialized_ReadData(ser, &bitmap->bpp, sizeof(bitmap->bpp));
	Serialized_ReadWrapped(ser, &bitmap->sizex, sizeof(bitmap->sizex));
	Serialized_ReadWrapped(ser, &bitmap->sizey, sizeof(bitmap->sizey));
	Serialized_ReadWrapped(ser, &bitmap->clip_rect_x1, sizeof(bitmap->clip_rect_x1));
	Serialized_ReadWrapped(ser, &bitmap->clip_rect_x2, sizeof(bitmap->clip_rect_x2));
	Serialized_ReadWrapped(ser, &bitmap->clip_rect_y1, sizeof(bitmap->clip_rect_y1));
	Serialized_ReadWrapped(ser, &bitmap->clip_rect_y2, sizeof(bitmap->clip_rect_y2));

	bitmap->multab = Memory_Reserve(bitmap->sizey, int);
	bitmap->buf = Memory_Reserve(bitmap->sizey * bitmap->sizex, PixelColor);

   for (s=0; s<bitmap->sizey; s++) {
      Serialized_ReadWrapped(ser, &bitmap->multab[s], sizeof(int));
   }

   for (s=0; s<bitmap->sizey * bitmap->sizex; s++)
      Serialized_ReadWrapped(ser, &bitmap->buf[s], sizeof(PixelColor));   
	
	return bitmap;
}
#endif
/*--------------------------------------------------------------------------*/

#ifdef CMOD_PNG

#include "file.h"

BitMap *BitMap_ReadPNGFile(const String *filename) {  /* We need to open the file */
   png_structp png_ptr;
   png_infop info_ptr;
   unsigned int sig_read = 0;
   png_uint_32 width, height;
   int bit_depth, color_type, interlace_type;
   int dc1, dc2;
   FILE *fp;
   BitMap *bm;
   png_bytep *row_pointers;
   unsigned int x,y;
   PixelColor p;

   if (filename == NULL) return NULL;
   fp = fopen(filename, "rb");
   
   if (fp == NULL) return (NULL);

   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if (png_ptr == NULL) {
      fclose(fp);
      return (NULL);
   }

   /* Allocate/initialize the memory for image information.  REQUIRED. */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL) {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      return (NULL);
   }
#if 0
   if (setjmp(png_ptr->jmpbuf)) {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      fclose(fp);
      /* If we get here, we had a problem reading the file */
      return (NULL);
   }
#endif
   /* Set up the input control if you are using standard C streams */
   png_init_io(png_ptr, fp);

   /* If we have already read some of the signature */
   png_set_sig_bytes(png_ptr, sig_read);

   png_read_png(png_ptr, info_ptr, 0, NULL);

   /* At this point you have read the entire image */

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, &dc1, &dc2);
   bm = BitMap_Create(width, height);
   printf("%s : bit_depth = %i, color_type = %i\n", filename, bit_depth, color_type);
   row_pointers = png_get_rows(png_ptr, info_ptr);
   if ((color_type & 4) && (color_type & 2) && bit_depth == 8) {
      for (y=0; y<height; y++) {
         for (x=0; x<width; x++) {
            p = PixelColor_FromRGBA(row_pointers[y][4*x], row_pointers[y][4*x+1], row_pointers[y][4*x+2], 255);
            p.a = row_pointers[y][4*x+3];
            BitMap_SetPixel(bm,x,y,p);
         }
      }
   } else {   
      if (color_type & 2 && bit_depth == 8) {
         for (y=0; y<height; y++) {
            for (x=0; x<width; x++) {
               p = PixelColor_FromRGBA(row_pointers[y][3*x], row_pointers[y][3*x+1], row_pointers[y][3*x+2], 255);
               BitMap_SetPixel(bm,x,y,p);
            }
         }
      } else 
      if (color_type == 0 && bit_depth == 8) {
         for (y=0; y<height; y++) {
            for (x=0; x<width; x++) {
               p = PixelColor_FromRGBA(row_pointers[y][x], row_pointers[y][x], row_pointers[y][x], 255);
               BitMap_SetPixel(bm,x,y,p);
            }
         }
      } else 
      if (color_type == 4 && bit_depth == 8) {
         for (y=0; y<height; y++) {
            for (x=0; x<width; x++) {
               p = PixelColor_FromRGBA(row_pointers[y][x*2], row_pointers[y][x*2], row_pointers[y][x*2], 255);
               p.a = (255-row_pointers[y][x*2+1]);
               p.a = (p.a & 0xf0) | (p.a >> 4);
               if (p.a == 0xff) p.r = p.g = p.b = 0;
               BitMap_SetPixel(bm,x,y,p);
            }
         }
      } else 
      if (bit_depth == 16) {
         for (y=0; y<height; y++) {
            for (x=0; x<width; x++) {
               p = PixelColor_FromRGBA(row_pointers[y][6*x], row_pointers[y][6*x+2], row_pointers[y][6*x+4], 255);
//               p.a = 
               BitMap_SetPixel(bm,x,y,p);
            }
         }
      }
   }

   /* clean up after the read, and free any memory allocated - REQUIRED */
   png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

   /* close the file */
   fclose(fp);

   return (bm);
}

/*--------------------------------------------------------------------------*/

#define errx(ex, m) do { fprintf(stderr, m); exit(ex); } while (0)

void BitMap_SavePNGFile(BitMap *bm, const String *filename) {
	png_structp png_ptr;
	png_infop info_ptr;
	struct { 
      unsigned char r, g, b;
   } __attribute__((packed)) *line;
	int x, y;
   PixelColor p;
   FILE *f;

   line = malloc(3*BitMap_SizeX(bm));

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		(png_voidp)NULL, NULL, NULL);
	if (!png_ptr)
		errx(1, "png_create_write_struct");

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) 
		errx(1, "png_create_info_struct");

	if (setjmp(png_jmpbuf(png_ptr)))
		/* All future errors jump here */
		errx(1, "png error");

   f = fopen(filename, "wb");
	png_init_io(png_ptr, f);
   
	png_set_compression_level(png_ptr, PNG_Z_DEFAULT_COMPRESSION);
	png_set_IHDR(png_ptr, info_ptr,
		BitMap_SizeX(bm),	/* width */
		BitMap_SizeY(bm),	/* height */
		8,	/* bit depth */
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	for (y = 0; y < BitMap_SizeY(bm); y++) {
		for (x = 0; x < BitMap_SizeX(bm); x++) {
         p = BitMap_GetPixel(bm, x, y);
			line[x].r = p.r;
			line[x].g = p.g;
			line[x].b = p.b;
		}
		png_write_row(png_ptr, (unsigned char *)line);
	}
	png_write_end(png_ptr, info_ptr);

   free(line);

	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(f);
}

#undef png_jmpbuf
#undef errx
/*--------------------------------------------------------------------------*/

void BitMap_SaveRAWFile(BitMap *bm, const String *filename) {
   File *f;
   int x, y;
   PixelColor s;
   
   f = File_Open(filename, "wb");
   for (y=0; y<BitMap_SizeY(bm); y++) {
      for (x=0; x<BitMap_SizeX(bm); x++) {
         s = BitMap_GetPixel(bm, x, y);
         File_WriteByte(f, s.r);
         File_WriteByte(f, s.g);
         File_WriteByte(f, s.b);
      }
   }
   File_Close(f);
}

#endif

/* ----------------------------------------------------------------------- */

void BitMap_SetClipRect(BitMap *bitmap, int x1, int y1, int x2, int y2) {
   bitmap->clip_rect_x1 = MAX(0, MIN(x1, x2));
   bitmap->clip_rect_x2 = MIN(bitmap->sizex, MAX(x1, x2));
   bitmap->clip_rect_y1 = MAX(0, MIN(y1, y2));
   bitmap->clip_rect_y2 = MIN(bitmap->sizey, MAX(y1, y2));
}

/* ----------------------------------------------------------------------- */

void BitMap_ResetClipRect(BitMap *bitmap) {
	bitmap->clip_rect_x1 = 0;
	bitmap->clip_rect_x2 = BitMap_SizeX(bitmap)-1;
   bitmap->clip_rect_y1 = 0;
	bitmap->clip_rect_y2 = BitMap_SizeY(bitmap)-1;
}

/* ----------------------------------------------------------------------- */

/* Does not stretch/shrink, just changes memory alloc */
void BitMap_Resize(BitMap *bitmap, int newsizex, int newsizey, Bool copy) {
   int y, s;
   PixelColor *newbuf;
   int *newmultab;
   
   if (newsizex <=0 || newsizey <=0 || (newsizex == bitmap->sizex && newsizey == bitmap->sizey)) return;
   
   newbuf = Memory_Reserve(newsizey * newsizex, PixelColor);
   if(copy) {
      for (y=0; y<MIN(bitmap->sizey, newsizey); y++) {
      	Memory_Copy(&newbuf[y*newsizex], &bitmap->buf[bitmap->multab[y]], MIN(bitmap->sizex,newsizex)*sizeof(PixelColor));
      }
   }
      
   newmultab = Memory_Reserve(newsizey, int);
   s = 0;
   for (y=0; y<newsizey; y++) {
      newmultab[y] = s;
      s+=newsizex;
   }
   /* TODO: mutex here */
   Memory_Free(bitmap->multab, int);
   bitmap->multab = newmultab;
   Memory_Free(bitmap->buf, PixelColor);
   bitmap->buf = newbuf;
   
   if (bitmap->clip_rect_x2 == bitmap->sizex-1) bitmap->clip_rect_x2 = newsizex-1;
   if (bitmap->clip_rect_y2 == bitmap->sizey-1) bitmap->clip_rect_y2 = newsizey-1;

   bitmap->sizex = newsizex;
   bitmap->sizey = newsizey;
   /* To here*/
}

/*--------------------------------------------------------------------------*/

#define BITMAP_CLIPCODE_BOTTOM 1
#define BITMAP_CLIPCODE_TOP 2
#define BITMAP_CLIPCODE_LEFT 4
#define BITMAP_CLIPCODE_RIGHT 8

int BitMap_ClipCode(const BitMap *bitmap, int x, int y) {
   int code;
      
   if (y > bitmap->clip_rect_y2) code = BITMAP_CLIPCODE_BOTTOM;
   else {
      if (y < bitmap->clip_rect_y1) 
         code = BITMAP_CLIPCODE_TOP;
      else
         code = 0;
   }
   
   if (x < bitmap->clip_rect_x1) code |= BITMAP_CLIPCODE_LEFT;
   else
      if (x > bitmap->clip_rect_x2) code |= BITMAP_CLIPCODE_RIGHT;
   
   return code;
}

/*--------------------------------------------------------------------------*/

void BitMap_PutPixelAlpha(BitMap *bitmap, int x, int y, PixelColor color) {
//   if (color.a > 0)
      color = PixelColor_Blend256(BitMap_GetPixel(bitmap, x, y), color, color.a);
   BitMap_PutPixel(bitmap, x, y, color);
}

/*--------------------------------------------------------------------------*/

void    BitMap_SetPixel(BitMap *bitmap, int x, int y, PixelColor color) {
   if (BitMap_ClipCode(bitmap, x, y) != 0) return;
   bitmap->buf[bitmap->multab[y] + x] = color;
}

/*--------------------------------------------------------------------------*/

PixelColor BitMap_Pixel(const BitMap *bitmap, int x, int y) {
   if (BitMap_ClipCode(bitmap, x, y) != 0) return pixelcolor_black;
   return bitmap->buf[bitmap->multab[y] + x];
}

/*--------------------------------------------------------------------------*/

void BitMap_ReplacePixels(BitMap *bitmap, int x1, int y1, int x2, int y2, PixelColor src, PixelColor dst) {
   int x, y;
   PixelColor p;
   for (y=y1; y<y2; y++) {
      for (x=x1; x<x2; x++) {
         p = BitMap_Pixel(bitmap, x, y);
         if (p.r == src.r && p.g == src.g && p.b == src.b) BitMap_SetPixel(bitmap, x, y, dst);
      }
   }
}

/*--------------------------------------------------------------------------*/

/* Uses Cohen-Sutherland clipping algorithm */

void BitMap_DrawLine(BitMap *bitmap, int x1, int y1, int x2, int y2, PixelColor color) {
   int c, x, y, c0, c1, dy, dx, dest;

   c0 = BitMap_ClipCode(bitmap, x1, y1);
   c1 = BitMap_ClipCode(bitmap, x2, y2);
   
   while((c0|c1) != 0) {
      if ((c0&c1) != 0) return;
      
      if (c0 != 0) c = c0; else c = c1;
      
      if (c & BITMAP_CLIPCODE_BOTTOM) {
         y = bitmap->clip_rect_y2;
         x = x1 + (x2 - x1)*(y - y1) / (y2 - y1);
      } else
      if (c & BITMAP_CLIPCODE_TOP) {
         y = bitmap->clip_rect_y1;
         x = x1 + (x2 - x1)*(y - y1) / (y2 - y1);
      } else
      if (c & BITMAP_CLIPCODE_LEFT) {
         x = bitmap->clip_rect_x1;
         y = y1 + (y2 - y1)*(x -x1) / (x2 - x1);
      } else
      if (c & BITMAP_CLIPCODE_RIGHT) {
         x = bitmap->clip_rect_x2;
         y = y1 + (y2 - y1)*(x -x1) / (x2 - x1);
      } else {
         x = 0;
         y = 0;
      }
      
      if (c0 == c) {
         x1 = x;
         y1 = y;
         c0 = BitMap_ClipCode(bitmap, x1, y1);
      } else {
         x2 = x;
         y2 = y;
         c1 = BitMap_ClipCode(bitmap, x2, y2);
      }
   }
   
//   fprintf(stderr, "(%i, %i), (%i, %i)\n", x1, y1, x2, y2);
   
   dy = y2 - y1;
   dx = x2 - x1;
   if (dx < 0) {
      x = x2; 
      y = y2;
      dx = -dx;
      dy = -dy;
   } else {
      x = x1; 
      y = y1;   
   }
   dest = bitmap->multab[y] + x;
   c = 0;
   bitmap->buf[dest] = color;
   
   if (dy > dx || -dy > dx) {
      if (dy < 0) 
         c1 = -dy;
      else
         c1 = dy;

      if (dy > 0) {
         while (c1 > 0) {
            c += dx;
            if (c>dy) { dest++; c-=dy; }
            dest += bitmap->sizex;
            bitmap->buf[dest] = color;
            c1--;
         } 
      } else {
         while (c1 > 0) {
            c += dx;
            if (c>dy) { dest++; c+=dy; }
            dest -= bitmap->sizex;
            bitmap->buf[dest] = color;
            c1--;
         }
      }
   } else {
      c1 = dx;

      if (dy > 0) {
         while (c1 > 0) {
            c += dy;
            if (c>dx) { dest += bitmap->sizex; c-=dx; }
            dest ++;
            bitmap->buf[dest] = color;
            c1--;
         } 
      } else {
         while (c1 > 0) {
            c -= dy;
            if (c>dx) { dest -= bitmap->sizex; c-=dx; }
            dest ++;
            bitmap->buf[dest] = color;
            c1--;
         }
      }
   }
}

/* Uses Cohen-Sutherland clipping algorithm */
/* Uses Wu Xiaolins fast-AA algorithm*/
void BitMap_DrawLineAA(BitMap *bitmap, int x1, int y1, int x2, int y2, PixelColor color) {
   int c, x, y, c0, c1, dy, dx, yd;
   unsigned int dest, dest2;

   /* Clipping */
   do {
      if (x1 == x2) { BitMap_VLine(bitmap, x1, y1, y2, color); return; }
      if (y1 == y2) { BitMap_HLine(bitmap, x1, x2, y1, color); return; }

      c0 = BitMap_ClipCode(bitmap, x1, y1);
      c1 = BitMap_ClipCode(bitmap, x2, y2);
      
      if ((c0&c1) != 0) return;
      
      if (c0 != 0) c = c0; else c = c1;
      
      if (c & BITMAP_CLIPCODE_BOTTOM) {
         y = bitmap->clip_rect_y2;
         x = x1 + (x2 - x1)*(y - y1) / (y2 - y1);
      } else
      if (c & BITMAP_CLIPCODE_TOP) {
         y = bitmap->clip_rect_y1;
         x = x1 + (x2 - x1)*(y - y1) / (y2 - y1);
      } else
      if (c & BITMAP_CLIPCODE_LEFT) {
         x = bitmap->clip_rect_x1;
         y = y1 + (y2 - y1)*(x -x1) / (x2 - x1);
      } else
      if (c & BITMAP_CLIPCODE_RIGHT) {
         x = bitmap->clip_rect_x2;
         y = y1 + (y2 - y1)*(x -x1) / (x2 - x1);
      } else {
         x = 0;
         y = 0;
      }
      
      if (c != 0) {
         if (c0 == c) {
            x1 = x;
            y1 = y;
         } else {
            x2 = x;
            y2 = y;
         }
      }
   } while((c0|c1) != 0);
   
   dy = y2 - y1;
   dx = x2 - x1;
   yd = bitmap->sizex;
   
   if (dx < 0) {
      dx = -dx;
      dy = -dy;
      dest = bitmap->multab[y2] + x2;
      dest2 = bitmap->multab[y1] + x1;
   } else {
      dest = bitmap->multab[y1] + x1;
      dest2 = bitmap->multab[y2] + x2;
   }
   
   if (dy < 0) {
      dy = -dy;
      yd = -yd;
   }  
   
   bitmap->buf[dest] = color;
   bitmap->buf[dest2] = color;
   c = 0;
   
   if (dy > dx || -dy > dx) {
      c1 = (1+dy)>>1;
      
      while (c1 > 0) {
         c += dx;
         if (c>dy) { dest++; dest2--; c-=dy; }
         dest += yd; dest2 -= yd;
         bitmap->buf[dest] = PixelColor_Blend(color, bitmap->buf[dest], dy-c, c);
         bitmap->buf[dest+1] = PixelColor_Blend(color, bitmap->buf[dest+1], c, dy-c);
         bitmap->buf[dest2] = PixelColor_Blend(color, bitmap->buf[dest2], dy-c, c);
         bitmap->buf[dest2-1] = PixelColor_Blend(color, bitmap->buf[dest2-1], dy-c, c);
         c1--;
      } 
   } else {
      c1 = (1+dx)>>1;

      if (dy > 0) {
         while (c1 > 0) {
            c += dy;
            if (c>dx) { dest += yd; dest2 -= yd; c-=dx; }
            dest ++; 
            dest2--;
            bitmap->buf[dest] = PixelColor_Blend(color, bitmap->buf[dest], dx-c, c);
            bitmap->buf[dest+yd] = PixelColor_Blend(color, bitmap->buf[dest+yd], c, dx-c);
            bitmap->buf[dest2] = PixelColor_Blend(color, bitmap->buf[dest2], dx-c, c);
            bitmap->buf[dest2-yd] = PixelColor_Blend(color, bitmap->buf[dest2-yd], dx-c, c);
            c1--;
         } 
      }
   }
}

/*--------------------------------------------------------------------------*/

void BitMap_DrawLinePattern(BitMap *bitmap, int x1, int y1, int x2, int y2, int n, PixelColor *color, int ccount, int thickness) {
   int c, x, y, c0, c1, dy, dx, dest, i, cdir;
   PixelColor cur;
   /* TODO: Clipping does not work always - up to thickness-1 pixels might end up outside */
/* 
   fprintf(stderr, "Bitmap size: %i, %i    clip: (%i,%i), (%i,%i)   ", bitmap->sizex, bitmap->sizey,
      bitmap->clip_rect_x1, bitmap->clip_rect_y1,
      bitmap->clip_rect_x2, bitmap->clip_rect_y2);
      
   fprintf(stderr, "Line: (%i, %i), (%i, %i)     Clipped: ", x1, y1, x2, y2);
*/
   c0 = BitMap_ClipCode(bitmap, x1, y1);
   c1 = BitMap_ClipCode(bitmap, x2, y2);
   
   while((c0|c1) != 0) {
      if ((c0&c1) != 0) return;
      
      if (c0 != 0) c = c0; else c = c1;
      
      if (c & BITMAP_CLIPCODE_BOTTOM) {
         y = bitmap->clip_rect_y2;
         x = x1 + (x2 - x1)*(y - y1) / (y2 - y1);
      } else
      if (c & BITMAP_CLIPCODE_TOP) {
         y = bitmap->clip_rect_y1;
         x = x1 + (x2 - x1)*(y - y1) / (y2 - y1);
      } else
      if (c & BITMAP_CLIPCODE_LEFT) {
         x = bitmap->clip_rect_x1;
         y = y1 + (y2 - y1)*(x -x1) / (x2 - x1);
      } else
      if (c & BITMAP_CLIPCODE_RIGHT) {
         x = bitmap->clip_rect_x2;
         y = y1 + (y2 - y1)*(x -x1) / (x2 - x1);
      } else {
         x = 0;
         y = 0;
      }
      
      if (c0 == c) {
         x1 = x;
         y1 = y;
         c0 = BitMap_ClipCode(bitmap, x1, y1);
      } else {
         x2 = x;
         y2 = y;
         c1 = BitMap_ClipCode(bitmap, x2, y2);
      }
   }
   
//   fprintf(stderr, "(%i, %i), (%i, %i)\n", x1, y1, x2, y2);
   dy = y2 - y1;
   dx = x2 - x1;
   if (dx < 0) {
      x = x2; 
      y = y2;
      dx = -dx;
      dy = -dy;
      cdir = 1;
   } else {
      x = x1; 
      y = y1;
      cdir = n-1;
   }
   dest = bitmap->multab[y] + x;
   c = 0;
   bitmap->buf[dest] = color[ccount];
   ccount++; if (ccount >= n) ccount=0;
   
   if (dy > dx || -dy > dx) {
      if (dy < 0) 
         c1 = -dy;
      else
         c1 = dy;

      if (dy > 0) {
         while (c1 > 0) {
            c += dx;
            if (c>dy) { dest++; c-=dy; }
            dest += bitmap->sizex;
            cur = color[ccount];
            for (i=0; i<thickness; i++) bitmap->buf[dest+i] = cur;
            ccount+=cdir; ccount %= n;
            c1--;
         } 
      } else {
         while (c1 > 0) {
            c += dx;
            if (c>dy) { dest++; c+=dy; }
            dest -= bitmap->sizex;
            cur = color[ccount];
            for (i=0; i<thickness; i++) bitmap->buf[dest+i] = cur;
//            bitmap->buf[dest] = color[ccount];
            ccount+=cdir; ccount %= n;
//            ccount++; if (ccount >= n) ccount=0;
            c1--;
         }
      }
   } else {
      c1 = dx;

      if (dy > 0) {
         while (c1 > 0) {
            c += dy;
            if (c>dx) { dest += bitmap->sizex; c-=dx; }
            dest ++;
//            bitmap->buf[dest] = color[ccount];
            cur = color[ccount];
            for (i=0; i<thickness; i++) bitmap->buf[dest+i*bitmap->sizex] = cur;

            ccount+=cdir; ccount %= n;
//            ccount--; if (ccount<0) ccount=n-1;
            c1--;
         } 
      } else {
         while (c1 > 0) {
            c -= dy;
            if (c>dx) { dest -= bitmap->sizex; c-=dx; }
            dest ++;
//            bitmap->buf[dest] = color[ccount];
            cur = color[ccount];
            for (i=0; i<thickness; i++) bitmap->buf[dest-i*bitmap->sizex] = cur;
            ccount+=cdir; ccount %= n;
//            ccount--; if (ccount < 0) ccount=n-1;
            c1--;
         }
      }
   }
}

/*
void BitMap_DrawChar(BitMap *bitmap, Font *font, int x, int y, PixelColor color, float angle, char c) {
   
}
*/
/*--------------------------------------------------------------------------*/
/*
void BitMap_DrawString(BitMap *bitmap, Font *font, int x, int y, PixelColor color, ..., String *str) {
   
}
*/
void BitMap_FillRectangle(BitMap *bitmap, int x1, int y1, int x2, int y2, PixelColor color) {
   int xstart,ystart,xmax,ymax;
   int x, y, off;
   
   x = x1;
   if (x > bitmap->clip_rect_x2) return;
   if (x < bitmap->clip_rect_x1) x = bitmap->clip_rect_x1;
   
   y = y1;
   if (y > bitmap->clip_rect_y2) return;
   if (y < bitmap->clip_rect_y1) y = bitmap->clip_rect_y1;
   
   xmax = x2;
   if (xmax < bitmap->clip_rect_x1) return;
   if (xmax > bitmap->clip_rect_x2) xmax = bitmap->clip_rect_x2;

   ymax = y2;
   if (ymax < bitmap->clip_rect_y1) return;
   if (ymax > bitmap->clip_rect_y2) ymax = bitmap->clip_rect_y2;
   xstart = x;
   ystart = y;

   for (y=ystart; y<=ymax; y++) {
      for (off = bitmap->multab[y] + xstart; off<=bitmap->multab[y] + xmax; off++) {
         bitmap->buf[off] = color;
      }
   }
}

/*--------------------------------------------------------------------------*/

void BitMap_DrawRectangle(BitMap *bitmap, int x1, int y1, int x2, int y2, PixelColor color) {
   int xstart,ystart,xmax,ymax;
   int x, y, off;
   
   x = x1;
   if (x > bitmap->clip_rect_x2) return;
   if (x < bitmap->clip_rect_x1) x = bitmap->clip_rect_x1;
   
   y = y1;
   if (y > bitmap->clip_rect_y2) return;
   if (y < bitmap->clip_rect_y1) y = bitmap->clip_rect_y1;
   
   xmax = x2;
   if (xmax < bitmap->clip_rect_x1) return;
   if (xmax > bitmap->clip_rect_x2) xmax = bitmap->clip_rect_x2;

   ymax = y2;
   if (ymax < bitmap->clip_rect_y1) return;
   if (ymax > bitmap->clip_rect_y2) ymax = bitmap->clip_rect_y2;
   xstart = x;
   ystart = y;

   for (off = bitmap->multab[ystart] + xstart; off<=bitmap->multab[ystart] + xmax; off++) {
      bitmap->buf[off] = color;
   }
   for (off = bitmap->multab[ymax] + xstart; off<=bitmap->multab[ymax] + xmax; off++) {
      bitmap->buf[off] = color;
   }
   for (y=ystart; y<=ymax; y++) {   
      bitmap->buf[bitmap->multab[y] + xstart] = color;
      bitmap->buf[bitmap->multab[y] + xmax] = color;
   }
}

/*--------------------------------------------------------------------------*/

void BitMap_DrawThickRectangle(BitMap *bitmap, int x1, int y1, int x2, int y2, PixelColor color, int thickness) {
   int i;
   for (i=0; i<thickness; i++) {
      BitMap_DrawRect(bitmap, x1+i, y1+i, x2-i, y2-i, color);
   }
}

/*--------------------------------------------------------------------------*/

void BitMap_FillCircle(BitMap *bitmap, int cx, int cy, int r, PixelColor color) {
   int y, x, r2, dx2, dy2;
   r2 = SQR(r);

   if (bitmap->clip_rect_y2>cy+r && bitmap->clip_rect_y1<cy-r
   && bitmap->clip_rect_x2>cx+r && bitmap->clip_rect_x1<cx-r) {
      for (x=0; x<=r; x++) {
      	bitmap->buf[bitmap->multab[cy]+cx+x] = color;
         bitmap->buf[bitmap->multab[cy]+cx-x] = color;
      }
      dy2 = 1;
      for (y=1; y<=r; y++) {
      	dx2 = 0;
         x = 0;
         while (dx2+dy2<=r2) {
         	if (bitmap->clip_rect_y2>cy+y) {
            	bitmap->buf[bitmap->multab[cy+y]+cx+x] = color;
               bitmap->buf[bitmap->multab[cy+y]+cx-x] = color;
            }
            if (bitmap->clip_rect_y1<cy-y) {
            	bitmap->buf[bitmap->multab[cy-y]+cx+x] = color;
               bitmap->buf[bitmap->multab[cy-y]+cx-x] = color;
            }
            dx2 += 1 + (x<<1);
            x++;
         }      
         dy2 += 1 + (y<<1);
      }
   }
}

/*--------------------------------------------------------------------------*/

void       BitMap_Circle(BitMap *bitmap, int cx, int cy, int r, PixelColor color) {
   int y, x, r2, dx2, dy2;
   r2 = SQR(r);

   if (bitmap->clip_rect_y2>cy+r && bitmap->clip_rect_y1<cy-r
   && bitmap->clip_rect_x2>cx+r && bitmap->clip_rect_x1<cx-r) {
/*      for (x=0; x<=r; x++) {
      	bitmap->buf[bitmap->multab[cy]+cx+x] = color;
         bitmap->buf[bitmap->multab[cy]+cx-x] = color;
      }*/
      x = r;
      dy2 = 1;
      for (y=1; y<=r; y++) {
      	dx2 = x*x;
//         x = 0;
         while (dx2+dy2>r2) {
            dx2 -= 1 + (x<<1);
            x--;
         }      
         dy2 += 1 + (y<<1);
         if (bitmap->clip_rect_y2>cy+y) {
            bitmap->buf[bitmap->multab[cy+y]+cx+x] = color;
            bitmap->buf[bitmap->multab[cy+y]+cx-x] = color;
         }
         if (bitmap->clip_rect_y1<cy-y) {
            bitmap->buf[bitmap->multab[cy-y]+cx+x] = color;
            bitmap->buf[bitmap->multab[cy-y]+cx-x] = color;
         }
      }
   }
}

/*--------------------------------------------------------------------------*/

void BitMap_FloodFill(BitMap *bm, IntCoord pos, PixelColor initial, PixelColor p) {
   int x, y;
   IDArray *q;
   PixelColor o;
   IntCoord box = IntCoord_Construct(BitMap_SizeX(bm)-1, BitMap_SizeY(bm)-1);
   
   q = IDArray_Create(4096);
   IDArray_Push(q, pos);
   while (IDArray_NofElems(q) > 0) {
      
      pos = IDArray_Pop(q);
      
      if (IntCoord_IsInBox(pos, box)) {
         x = IntCoord_X(pos);
         y = IntCoord_Y(pos);
         o = BitMap_GetPixel(bm, x, y);
      
         if (*(int *)&o == *(int *)&initial) {   
            BitMap_PutPixel(bm, x, y, p);
//            if (x == 482 && y == 315) PixelColor_Display(p, 0);
            
            IDArray_Push(q, pos + std_dintcoord[7]);            
            IDArray_Push(q, pos + std_dintcoord[3]);
            IDArray_Push(q, pos + std_dintcoord[1]);
            IDArray_Push(q, pos + std_dintcoord[5]);
            
/*            for (d=1; d<9; d+=2) {
               IDArray_Add(q, pos + std_dintcoord[d]);
            }*/
         }
      }
   }
   IDArray_Destroy(q);
}

/*--------------------------------------------------------------------------*/

void BitMap_PutBitMap(BitMap *canvas, const BitMap *image, int x, int y) {
   int xs,ys,xl,yl,yc, iy, ix;
   
   ys = MAX(y, canvas->clip_rect_y1);
   iy = ys - y;
   yl = MIN(image->sizey-iy, canvas->clip_rect_y2 - ys);
   xs = MAX(x, canvas->clip_rect_x1);
   ix = xs - x;
   xl = MIN(image->sizex-ix, canvas->clip_rect_x2 - xs);
   if (xl>0)
   for (yc=ys; yc<ys+yl; yc++) {
      Memory_Copy(&canvas->buf[canvas->multab[yc]+xs], &image->buf[image->multab[iy+yc-ys]+ix], xl*sizeof(PixelColor));
   }   
}

/*--------------------------------------------------------------------------*/

void BitMap_PutBitMapRecolour(BitMap *canvas, const BitMap *image, int x, int y, PixelColor color) {
   int xs,ys,xl,yl,yc,xc,iy,ix;
   PixelColor old;
   PixelColor plot;
   PixelColor ow;
   int r, g, b, a;
/* 
   ys = MAX(y, canvas->clip_rect_y1);
   yl = MIN(image->sizey, canvas->clip_rect_y2 - ys);
   iy = ys - y;
   xs = MAX(x, canvas->clip_rect_x1);
   xl = MIN(image->sizex, canvas->clip_rect_x2 - xs);
   ix = xs - x;
   
   if (xl>0)
*/
   yl = image->sizey;
   ys = y;
   iy = 0;
   if (ys < canvas->clip_rect_y1) {
      iy = canvas->clip_rect_y1 - ys;
      yl -= iy;
      ys = canvas->clip_rect_y1;
   }
   if (ys + yl > canvas->clip_rect_y2) {
      yl -= ys + yl - canvas->clip_rect_y2;      
   }
   if (ys > canvas->clip_rect_y2) return;
   if (ys+yl < canvas->clip_rect_y1) return;
   if (yl <= 0) return;

   xl = image->sizex;
   xs = x;
   ix = 0;
   if (xs < canvas->clip_rect_x1) {
      ix = canvas->clip_rect_x1 - xs;
      xl -= ix;
      xs = canvas->clip_rect_x1;
   }
   if (xs + xl > canvas->clip_rect_x2) {
      xl -= xs + xl - canvas->clip_rect_x2;      
   }
   if (xs > canvas->clip_rect_x2) return;
   if (xs+xl < canvas->clip_rect_x1) return;
   if (xl <= 0) return;

   for (yc=ys; yc<ys+yl; yc++) {
      for (xc=xs; xc<xs+xl; xc++) {
         ow = image->buf[image->multab[iy+yc-ys]+ix+xc-xs];
         if (ow.a == 1) plot = color;
         else
         if (ow.a > 1) {
            a = ow.a + 1;
            plot = 
            old = canvas->buf[canvas->multab[yc]+xc];
            r = ((int)old.r * a + (int)ow.r*(255-a)) / 256;
            g = ((int)old.g * a + (int)ow.g*(255-a)) / 256;
            b = ((int)old.b * a + (int)ow.b*(255-a)) / 256;
            plot = PixelColor_FromRGB(r,g,b);
//   			plot = old;
         } else
            plot = ow;
         
         canvas->buf[canvas->multab[yc]+xc] = plot;
      }
   }   
}

/*--------------------------------------------------------------------------*/

void BitMap_PutBitMapAlpha(BitMap *canvas, const BitMap *image, int x, int y) {
   int xs,ys,yc,xc,iy,ix;
   int xt,yt;
   PixelColor old;
   PixelColor plot;
   PixelColor ow;
   int r, g, b, a;
   
   /* TODO: redo with y1-y2 and x1 to x2 instead of start and length. buggy clipping atm. */
   
   ys = y;
   yt = ys + image->sizey;
   iy = 0;
   if (ys < canvas->clip_rect_y1) {
      iy = canvas->clip_rect_y1 - ys;
      ys = canvas->clip_rect_y1;
   }
   if (yt > canvas->clip_rect_y2) {
      yt = canvas->clip_rect_y2;      
   }
   if (ys > canvas->clip_rect_y2) return;
   if (yt < canvas->clip_rect_y1) return;

   xs = x;
   xt = xs + image->sizex;
   ix = 0;
   if (xs < canvas->clip_rect_x1) {
      ix = canvas->clip_rect_x1 - xs;
      xs = canvas->clip_rect_x1;
   }
   if (xt > canvas->clip_rect_x2) {
      xt = canvas->clip_rect_x2;      
   }
   if (xs > canvas->clip_rect_x2) return;
   if (xt < canvas->clip_rect_x1) return;

   for (yc=ys; yc<yt; yc++) {
      for (xc=xs; xc<xt; xc++) {
         ow = image->buf[image->multab[yc-ys+iy]+xc-xs+ix];
         if (ow.a == 255) continue;
         else
         if (ow.a >= 1) {
            a = ow.a + 1;
            old = canvas->buf[canvas->multab[yc]+xc];
            r = ((int)old.r * a + (int)ow.r*(255-a)) / 255;
            g = ((int)old.g * a + (int)ow.g*(255-a)) / 255;
            b = ((int)old.b * a + (int)ow.b*(255-a)) / 255;
            plot = PixelColor_FromRGB(r,g,b);
         } else
            plot = ow;
         
         canvas->buf[canvas->multab[yc]+xc] = plot;
      }
   }   
}

/*--------------------------------------------------------------------------*/

void BitMap_PutBitMapScanLineAlpha(BitMap *canvas, const BitMap *image, int canvasx, int canvasy, int imagex, int imagey, int length) {
   PixelColor *srcpos, *dstpos;

   if (imagey < 0 || imagey > BitMap_SizeY(image)) return;
   if (canvasy < canvas->clip_rect_y1 || canvasy > canvas->clip_rect_y2) return;
   if (canvasx > canvas->clip_rect_x2) return;
   
   srcpos = image->buf + image->multab[imagey] + imagex;
   
   if (canvasx < canvas->clip_rect_x1) {
      int d = canvas->clip_rect_x1 - canvasx;
      length -= d;
      canvasx = canvas->clip_rect_x1;
      srcpos += d;
   }
   if (canvasx + length > canvas->clip_rect_x2) length = canvas->clip_rect_x2 - canvasx;
   dstpos = canvas->buf + canvas->multab[canvasy] + canvasx;
   
   while (length-- > 0) {
      PixelColor ow = *srcpos++;
      if (ow.a == 255) { dstpos++; continue; }
      if (ow.a == 0) 
         *dstpos++ = ow;
      else {
         PixelColor old = *dstpos;
         int a = ow.a;
         int na = 255 - a;
         int r = ((int)old.r * a + (int)ow.r*na) / 255;
         int g = ((int)old.g * a + (int)ow.g*na) / 255;
         int b = ((int)old.b * a + (int)ow.b*na) / 255;
         *dstpos++ = PixelColor_FromRGB(r, g, b);
      }
   }
}

/*--------------------------------------------------------------------------*/

void BitMap_HLine(BitMap *bm, int x1, int x2, int y, PixelColor c) {
   int x, xdest;

   if (x2 < x1) { x = x1; x1 = x2; x2 = x; }
   if (y >= bm->clip_rect_y1 && y < bm->clip_rect_y2 && x1 < bm->clip_rect_x2 && x2 >= bm->clip_rect_x1)
      x = bm->multab[y];
   else return;
   
   xdest = x + MIN(bm->clip_rect_x2, x2);
   for (x+=MAX(bm->clip_rect_x1, x1); x<=xdest; x++) bm->buf[x] = c;
}

/*--------------------------------------------------------------------------*/

void BitMap_VLine(BitMap *bm, int x, int y1, int y2, PixelColor c) {
   int y, ydest;

   if (y2 < y1) { y = y1; y1 = y2; y2 = y; }
   if (x < bm->clip_rect_x1 || x >= bm->clip_rect_x2 || y1 > bm->clip_rect_y2 || y2 <= bm->clip_rect_y1) return;
   
   ydest = bm->multab[MIN(bm->clip_rect_y2, y2)];
   for (y=x+bm->multab[MAX(bm->clip_rect_y1, y1)]; y<ydest; y+=bm->sizex) bm->buf[y] = c;
}

/*--------------------------------------------------------------------------*/

void BitMap_PushButtonRect(BitMap *bm, int x1, int y1, int x2, int y2, PixelColor l, PixelColor u, PixelColor r, PixelColor d, int thickness) {
   int i;
   
   for (i=0; i<thickness; i++) {
      BitMap_HLine(bm, x1+i, x2-i, y1+i, u);
      BitMap_VLine(bm, x1+i, y1+i, y2-i, l);
      BitMap_VLine(bm, x2-i, y1+i, y2-i, r);
      BitMap_HLine(bm, x1+i, x2-i, y2-i, d);      
   }
}

/*--------------------------------------------------------------------------*/

void BitMap_PushButton(BitMap *bm, int x1, int y1, int x2, int y2, PixelColor middle, PixelColor l, PixelColor u, PixelColor r, PixelColor d, int thickness) {
   BitMap_FillRect(bm, x1+thickness, y1+thickness, x2-thickness, y2-thickness, middle);
   BitMap_PushButtonRect(bm, x1, y1, x2, y2, l, u, r, d, thickness);
}

/*--------------------------------------------------------------------------*/

#include <math.h>

void BitMap_RoundedPushButton(BitMap *bm, int x1, int y1, int x2, int y2, PixelColor middle, PixelColor l, PixelColor u, PixelColor r, PixelColor d, int thickness) {
   float f, t, v;
   PixelColor col;
   int i;
   
   BitMap_FillRect(bm, x1+thickness, y1+thickness, x2-thickness, y2-thickness, middle);
   t = 1.0 / thickness;
   
   for (i=0; i<thickness; i++) {
      f = (float)(thickness - i) - 1.0;
      f *= t;
      v = sqrt(1.0 - f);
//      printf("%f ", v);
      
      col.r = u.r - (u.r - middle.r)*v;
      col.g = u.g - (u.g - middle.g)*v;
      col.b = u.b - (u.b - middle.b)*v;      
      BitMap_HLine(bm, x1+i, x2-i, y1+i, col);

      col.r = l.r - (l.r - middle.r)*v;
      col.g = l.g - (l.g - middle.g)*v;
      col.b = l.b - (l.b - middle.b)*v;
      BitMap_VLine(bm, x1+i, y1+i, y2-i, col);

      col.r = r.r - (r.r - middle.r)*v;
      col.g = r.g - (r.g - middle.g)*v;
      col.b = r.b - (r.b - middle.b)*v;
      BitMap_VLine(bm, x2-i, y1+i, y2-i, col);

      col.r = d.r - (d.r - middle.r)*v;
      col.g = d.g - (d.g - middle.g)*v;
      col.b = d.b - (d.b - middle.b)*v;
      BitMap_HLine(bm, x1+i, x2-i, y2-i, col);
   }
//   printf("\n");
}

/*--------------------------------------------------------------------------*/

/* Draws from src to canvas, painting over (dx1,dy1)-(dx2,dy2) from (sx,sy). src is scaled by factor scale */
void BitMap_DrawView(BitMap *canvas, BitMap *src, int dx1, int dy1, int dx2, int dy2, int sx, int sy, int scale, int stx, int sty) {
   int x, y, t, s, i;

   if (dx1 < canvas->clip_rect_x1) dx1 = canvas->clip_rect_x1;
   if (dx2 > canvas->clip_rect_x2) dx2 = canvas->clip_rect_x2;
   if (dy1 < canvas->clip_rect_y1) dy1 = canvas->clip_rect_y1;
   if (dy2 > canvas->clip_rect_y2) dy2 = canvas->clip_rect_y2;
   
   for (y=dy1; y<dy2; y++) {
      s = src->multab[sy + (y-dy1+sty)/scale] + sx;
      t = canvas->multab[y]+dx1; 
      i = stx;         
      for (x=dx1; x<dx2; x++) {
         canvas->buf[t++] = src->buf[s];
         i++;
         if (i == scale) { i = 0; s++; }
      }
   }
}

/*--------------------------------------------------------------------------*/

BitMap *BitMap_GetBitMap(const BitMap *org, int x1, int x2, int y1, int y2) {
   BitMap *bg;
   int x, y;
   
   bg = BitMap_Create(x2-x1, y2-y1);
   for (y=0; y<y2-y1; y++) {
      for (x=0; x<x2-x1; x++) {
         bg->buf[bg->multab[y]+x] = org->buf[org->multab[y+y1] + x + x1];
      }
   }
   return bg;
}

/*--------------------------------------------------------------------------*/

void BitMap_PutBitMapNoClip(BitMap *dst, const BitMap *sprite, int x1, int y1) {
   BitMap *bg;
   int x, y;
   int w = BitMap_SizeX(sprite);
   int h = BitMap_SizeY(sprite);
   
   for (y=0; y<h; y++) {
      for (x=0; x<w; x++) {
          dst->buf[dst->multab[y+y1] + x + x1] = sprite->buf[sprite->multab[y]+x];
      }
   }
}

/*--------------------------------------------------------------------------*/

void BitMap_SetAlpha(BitMap *bm, unsigned char alpha) {
   int x, y;
   for (y=0; y<BitMap_SizeY(bm); y++) {
      for (x=0; x<BitMap_SizeX(bm); x++) {
         bm->buf[bm->multab[y]+x].a = alpha;
      }
   }
}

/*--------------------------------------------------------------------------*/

#define TG 7
PixelColor BitMap_SubSample(BitMap *bm, int x, int y) {
   int xf[2], yf[2];
   int w[2][2];
   int r=0, g=0, b=0, a=0;
   PixelColor pix[2][2];
   int i, j;
   PixelColor res;
   
   xf[1] = x & ((1<<TG)-1);
   xf[0] = (1<<TG) - xf[1];
   yf[1] = y & ((1<<TG)-1);
   yf[0] = (1<<TG) - yf[1];
   
   for (j=1; j>=0; j--) {
      for (i=1; i>=0; i--) {
         w[i][j] = xf[i]*yf[j];
         pix[i][j] = BitMap_GetPixel(bm, (x>>TG) + i, (y>>TG) + j);
      }
   }

   for (j=1; j>=0; j--) {
      for (i=1; i>=0; i--) {
         r += w[i][j] * (int)pix[i][j].r;
         g += w[i][j] * (int)pix[i][j].g;
         b += w[i][j] * (int)pix[i][j].b;
         a += w[i][j] * (int)pix[i][j].a;
      }
   }

   res.r = r / SQR(1<<TG);
   res.g = g / SQR(1<<TG);
   res.b = b / SQR(1<<TG);
   res.a = a / SQR(1<<TG);
   
   return res;   
}

void bitmap_texturemap_interp(int *left_edge, int *right_edge, IntCoord *left_text, IntCoord *right_text, int startx, int starty, int stopx, int stopy, int tstartx, int tstarty, int tstopx, int tstopy) {
   int x, dx, y, dy, ys, tx, ty, dtx, dty; /* t,dt are for texture space */

   ys = abs(starty-stopy);
   if (ys == 0) ys = 1;
   
   if (starty>stopy) dy=-1; else dy=1;
   x = startx << TG;
   tx = tstartx << TG; 
   ty = tstarty << TG; 
   dx = ((stopx - startx)<<TG) / ys;
   dtx = ((tstopx - tstartx) << TG) / ys;
   dty = ((tstopy - tstarty) << TG) / ys;
   
   for (y=starty; y!=stopy; y+=dy) {
      if (y>=0) {
         if (x < left_edge[y]) {
            left_edge[y] = x;
            left_text[y] = IntCoord_Construct(tx, ty);
         }
         if (x > right_edge[y]) {
            right_edge[y] = x;
            right_text[y] = IntCoord_Construct(tx, ty);
         }
      }
      x += dx;
      tx += dtx;
      ty += dty;
/*      tx &= 65535;
      ty &= 32767;*/
      if (y>=0) {
         if (x > right_edge[y]) {
            right_edge[y] = x;
            right_text[y] = IntCoord_Construct(tx, ty);
         }
         if (x < left_edge[y]) {
            left_edge[y] = x;
            left_text[y] = IntCoord_Construct(tx, ty);
         }
      }
   }
}

/* (tlx,tly) |-> (0,0) in texture space */
void BitMap_TextureMapRect(BitMap *canvas, BitMap *texture, int tlx, int tly,  int trx, int _try,  int blx, int bly,  int brx, int bry,  int alpha) {
   int *left_edge, *right_edge;
   IntCoord *left_texturepos, *right_texturepos;
   int bottom, top, tw, th, y, tx, ty, dtx, dty, x, xs;

   top = MIN(tly, MIN(_try, MIN(bly, bry)));
   bottom = 1 + MAX(tly, MAX(_try, MAX(bly, bry)));
   tw = BitMap_SizeX(texture)-2;
   th = BitMap_SizeY(texture)-2;

   /* Edge tables are shifted */
   left_edge = Int_CreateVal(bottom, MAXINT);
   right_edge = Int_CreateVal(bottom, -MAXINT);
   left_texturepos = (IntCoord *)Int_Create(bottom);
   right_texturepos = (IntCoord *)Int_Create(bottom);
   
   /* First edge: between (tlx,tly) -- (trx,_try). Texture space goes from (0,0)-(Tw,0) */
   bitmap_texturemap_interp(left_edge, right_edge, left_texturepos, right_texturepos,
      tlx, tly,  trx, _try,  1, 1, tw, 1);

   /* Second edge: between (trx,try) -- (brx, bry). Texture space goes from (Tw,0)-(Tw,Th) */
   bitmap_texturemap_interp(left_edge, right_edge, left_texturepos, right_texturepos,
      trx, _try,  brx, bry,  tw, 1,  tw, th);

   /* Third edge: between (brx,bry) -- (blx, bly). Texture space goes from (Tw,Th)-(0,Th) */
   bitmap_texturemap_interp(left_edge, right_edge, left_texturepos, right_texturepos,
      brx, bry,  blx, bly,  tw, th,  1, th);

   /* Fourth edge: between (blx,bly) -- (tlx, tly). Texture space goes from (0,Th)-(0,0) */
   bitmap_texturemap_interp(left_edge, right_edge, left_texturepos, right_texturepos,
      blx, bly,  tlx, tly,  1, th,  1, 1);
      
   /* Now draw the texture... */
   if (top < canvas->clip_rect_y1) top = canvas->clip_rect_y1;
   if (bottom > canvas->clip_rect_y2) bottom = canvas->clip_rect_y2;
   for (y=top; y<bottom; y++) {
      xs = (right_edge[y] - left_edge[y]);
      if (xs > 0) {
         tx = IntCoord_X(left_texturepos[y]);
         ty = IntCoord_Y(left_texturepos[y]);
         dtx = (((int)IntCoord_X(right_texturepos[y]) - tx) << TG) / xs;
         dty = (((int)IntCoord_Y(right_texturepos[y]) - ty) << TG) / xs;

         tx -= dtx*(left_edge[y]&((1<<TG)-1)) >> TG;
         ty -= dty*(left_edge[y]&((1<<TG)-1)) >> TG;

/*
         if (alpha == 255) {
            for (x=left_edge[y]>>TG; x<right_edge[y]>>TG; x++) {
               if (x >= canvas->clip_rect_x1 && x <= canvas->clip_rect_x2)
                  BitMap_PutPixelAlpha(canvas, x, y, BitMap_GetPixel(texture, tx>>TG, ty>>TG));
               tx += dtx;
               ty += dty;
            }
         } else {
*/
            for (x=left_edge[y]>>TG; x<right_edge[y]>>TG; x++) {
               if (x >= canvas->clip_rect_x1 && x <= canvas->clip_rect_x2) {
//                  PixelColor p = PixelColor_Blend256(BitMap_GetPixel(texture, tx>>TG, ty>>TG), BitMap_GetPixel(canvas, x, y), alpha);
                  PixelColor p = PixelColor_Blend256(BitMap_SubSample(texture, tx, ty), BitMap_GetPixel(canvas, x, y), alpha);
//   					PixelColor p = BitMap_GetPixel(texture, tx>>TG, ty>>TG);
                  BitMap_PutPixelAlpha(canvas, x, y, p);
               }
               tx += dtx;
               ty += dty;
            }
//         }
      }
   }
   Int_Destroy(left_edge);
   Int_Destroy(right_edge);
   Int_Destroy((int *)left_texturepos);
   Int_Destroy((int *)right_texturepos);
}
#undef TG

/*--------------------------------------------------------------------------*/
   
#ifndef CMOD_OPTIMIZE_SPEED
char BitMap_Type(BitMap *bitmap) { return bitmap->type; }

/*--------------------------------------------------------------------------*/

void BitMap_SetType(BitMap *bitmap, char type) { bitmap->type = type; }

/*--------------------------------------------------------------------------*/

char BitMap_Bpp(BitMap *bitmap) { return bitmap->bpp; }

/*--------------------------------------------------------------------------*/

void BitMap_SetBpp(BitMap *bitmap, char bpp) { bitmap->bpp = bpp; }

/*--------------------------------------------------------------------------*/

int BitMap_Sizex(BitMap *bitmap) { return bitmap->sizex; }

/*--------------------------------------------------------------------------*/

void BitMap_SetSizex(BitMap *bitmap, int sizex) { bitmap->sizex = sizex; }

/*--------------------------------------------------------------------------*/

int BitMap_Sizey(BitMap *bitmap) { return bitmap->sizey; }

/*--------------------------------------------------------------------------*/

void BitMap_SetSizey(BitMap *bitmap, int sizey) { bitmap->sizey = sizey; }

/*--------------------------------------------------------------------------*/

int BitMap_ClipRectX1(BitMap *bitmap) { return bitmap->clip_rect_x1; }

/*--------------------------------------------------------------------------*/

void BitMap_SetClipRectX1(BitMap *bitmap, int clip_rect_x1) { bitmap->clip_rect_x1 = clip_rect_x1; }

/*--------------------------------------------------------------------------*/

int BitMap_ClipRectX2(BitMap *bitmap) { return bitmap->clip_rect_x2; }

/*--------------------------------------------------------------------------*/

void BitMap_SetClipRectX2(BitMap *bitmap, int clip_rect_x2) { bitmap->clip_rect_x2 = clip_rect_x2; }

/*--------------------------------------------------------------------------*/

int BitMap_ClipRectY1(BitMap *bitmap) { return bitmap->clip_rect_y1; }

/*--------------------------------------------------------------------------*/

void BitMap_SetClipRectY1(BitMap *bitmap, int clip_rect_y1) { bitmap->clip_rect_y1 = clip_rect_y1; }

/*--------------------------------------------------------------------------*/

int BitMap_ClipRectY2(BitMap *bitmap) { return bitmap->clip_rect_y2; }

/*--------------------------------------------------------------------------*/

void BitMap_SetClipRectY2(BitMap *bitmap, int clip_rect_y2) { bitmap->clip_rect_y2 = clip_rect_y2; }

/*--------------------------------------------------------------------------*/

int *BitMap_Multab(BitMap *bitmap) { return bitmap->multab; }

/*--------------------------------------------------------------------------*/

void BitMap_SetMultab(BitMap *bitmap, int *multab) { bitmap->multab = multab; }

/*--------------------------------------------------------------------------*/

PixelColor *BitMap_Buf(BitMap *bitmap) { return bitmap->buf; }

/*--------------------------------------------------------------------------*/
#endif

void BitMap_SetBuf(BitMap *bitmap, PixelColor *buf) { 
   Memory_Free(bitmap->buf, PixelColor);
   bitmap->buf = buf; 
}

/*--------------------------------------------------------------------------*/

void BitMap_SetSize(BitMap *bitmap, int x, int y) {
   int i,s;

   if (x <=0 || y <=0 || (x == bitmap->sizex && y == bitmap->sizey)) return;

   Memory_Free(bitmap->multab, int);
   bitmap->sizex = x;
   bitmap->sizey = y;
   bitmap->multab = Memory_Reserve(y, int);
   s = 0;
   for (i=0; i<y; i++) {
      bitmap->multab[i] = s;
      s+=x; 
   }
}

/*--------------------------------------------------------------------------*/

void BitMap_FlipY(BitMap *bm) {
    int y;
    PixelColor temp[bm->sizex];
    for (y=0; y<bm->sizey/2; y++) {
        Memory_Copy(temp, bm->buf + bm->multab[y], sizeof(temp));
        Memory_Copy(bm->buf + bm->multab[y], bm->buf + bm->multab[bm->sizey - 1 - y], sizeof(temp));
        Memory_Copy(bm->buf + bm->multab[bm->sizey - 1 - y], temp, sizeof(temp));
    }
}

/*--------------------------------------------------------------------------*/

void BitMap_Restore(BitMap *bitmap) {
   Memory_Free(bitmap->multab, int);
   Memory_Free(bitmap->buf, PixelColor);
}

/*--------------------------------------------------------------------------*/

void BitMap_Destroy(BitMap *bitmap) {
   if (bitmap != NULL) {
      BitMap_Restore(bitmap);
     	Memory_Free(bitmap, BitMap);
   }
}


/*--------------------------------------------------------------------------*/

void BitMap_PrintHTML(BitMap *bm) {
   int x, y, l;
   PixelColor p;
   
   printf("<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" width=\"%i\" height=\"%i\">\n",
      BitMap_SizeX(bm), BitMap_SizeY(bm));
   for (y=0; y<BitMap_SizeY(bm); y++) {
      printf("<tr height=\"1\">");
      for (x=0; x<BitMap_SizeX(bm); x+=l) {
         l = 1;
         p = BitMap_Pixel(bm,x,y);
         while (x+l<BitMap_SizeX(bm) && PixelColor_IsEqual(BitMap_Pixel(bm,x+l,y), p)) l++;
         printf("<td bgcolor=\"#%02X%02X%02X\" width=\"%i\"", p.r, p.g, p.b, l);
         if (l>1) printf(" colspan=\"%i\"", l);
         printf("></td>");         
      }
      printf("</tr>\n");
   }
   printf("</table>\n");
}

/*--------------------------------------------------------------------------*/

#ifdef CMOD_DEBUG_OUTPUT
void BitMap_Display(BitMap *bitmap, int n) {
	int j;

	for (j=0; j<n; j++) printf(" ");
	printf("type = ");
	char_Display(bitmap->type, n+3);

	for (j=0; j<n; j++) printf(" ");
	printf("bpp = ");
	char_Display(bitmap->bpp, n+3);

	for (j=0; j<n; j++) printf(" ");
	printf("sizex = ");
	printf("%i", bitmap->sizex);

	for (j=0; j<n; j++) printf(" ");
	printf("sizey = ");
	printf("%i", bitmap->sizey);

	for (j=0; j<n; j++) printf(" ");
	printf("clip_rect_x1 = ");
	printf("%i", bitmap->clip_rect_x1);

	for (j=0; j<n; j++) printf(" ");
	printf("clip_rect_x2 = ");
	printf("%i", bitmap->clip_rect_x2);

	for (j=0; j<n; j++) printf(" ");
	printf("clip_rect_y1 = ");
	printf("%i", bitmap->clip_rect_y1);

	for (j=0; j<n; j++) printf(" ");
	printf("clip_rect_y2 = ");
	printf("%i", bitmap->clip_rect_y2);

	for (j=0; j<n; j++) printf(" ");
	printf("multab = ");
	printf("%i", bitmap->multab);

	for (j=0; j<n; j++) printf(" ");
	printf("buf = ");
	PixelColor_Display(bitmap->buf, n+3);

	printf("\n");
}
	#endif

