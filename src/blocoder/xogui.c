
#include "bitmap.h"
#include "cmod.h"
#include "darray.h"
#include "fsfont.h"
#include "memory.h"
#include "pixelcolor.h"
#include "xodget.h"
#include "xogui.h"
#include "xowin.h"

#include <stdarg.h>

struct XoGUI {
	XoWin      *win;
	BitMap     *bm;
	FSFont     *font;
	DArray     *xodget;
    DArray     *last_xodget;
	short       style;
	int         xodget_id;
	int         activated_id;
   int         focus_id;
	PixelColor  light;
	PixelColor  surface;
	PixelColor  shadow;
	PixelColor  text;
	PixelColor  selection;
   
   int         mousex;
   int         mousey;
   short       key;
   unsigned char text_thickness;
   unsigned char panel_thickness;
   /* Current view, negative coordinates apply to these */
   int         view_x1;
   int         view_y1;
   int         view_x2;
   int         view_y2;
   IDArray    *viewstack;

   int         minsizex;
   int         minsizey;
   IntCoord    grabpos;
};

#define XOGUI_PRIVATE (1 << 27)
#define XOGUI_TEXTFIELD_MAX 512
#define XOGUI_BUTTONHEIGHT 3
#define XOGUI_CURSORWIDTH 6

enum {
   XOGUI_STYLE_PLAIN,
   XOGUI_STYLE_ROUNDED
};

/*--------------------------------------------------------------------------*/

void XoGUI_Clear(XoGUI *xo) { 
//   BitMap_Clear(xo->bm); 
   if (xo->style == 0) {
      BitMap_PushButton(xo->bm, 0, 0, BitMap_SizeX(xo->bm)-1, BitMap_SizeY(xo->bm)-1,
         xo->surface, xo->light, xo->light, xo->shadow, xo->shadow, 3*xo->panel_thickness/2);
   } else {   
      BitMap_RoundedPushButton(xo->bm, 0, 0, BitMap_SizeX(xo->bm)-1, BitMap_SizeY(xo->bm)-1,
         xo->surface, xo->light, xo->light, xo->shadow, xo->shadow, 10);
      XoGUI_Panel(xo, 0, 8, 8, -8, -8);      
   }
}

/*--------------------------------------------------------------------------*/

void XoGUI_Init(XoGUI *xogui) {
   int pos;
   xogui->bm = xogui->win->bitmap;
// xogui->font = FSFont_CreateFromBDFFile("9x15cards2.bdf");
//   xogui->font = FSFont_CreateFromBDFFile("8x16.bdf");
   xogui->font = FSFont_Default();
   xogui->text_thickness = 1;
   xogui->xodget = DArray_Create(16);
   xogui->last_xodget = DArray_Create(16);
   xogui->style = XOGUI_STYLE_PLAIN;
   xogui->panel_thickness = 2;
   
   /* xodget_id : control being clicked or controlled */
   xogui->xodget_id = -1;
   /* activated : a button was clicked, or a control was manipulated */
   xogui->activated_id = -1;
   /* focus: which control is in focus */
   xogui->focus_id = -1;
   
   xogui->light = PixelColor_FromRGB(208, 208, 208);
   xogui->surface = PixelColor_FromRGB(130, 130, 130);
   xogui->shadow = PixelColor_FromRGB(60, 60, 60);
   xogui->text = PixelColor_FromRGB(0, 0, 0);
   xogui->selection = PixelColor_FromRGB(0, 0, 80);

   pos = XoWin_PointerPos(xogui->win);
   xogui->grabpos = -1;
   xogui->mousex = pos & 65535;
   xogui->mousey = pos >> 16;
   xogui->key = 0;
   XoGUI_Clear(xogui);
   XoGUI_ResetView(xogui);
//   XoWin_HideCursor(xogui->win);

   xogui->viewstack = IDArray_Create(16);
   xogui->minsizex = XoWin_SizeX(xogui->win);
   xogui->minsizey = XoWin_SizeY(xogui->win);
   
}

/*--------------------------------------------------------------------------*/

XoGUI *XoGUI_CreatePos(int x, int y, int w, int h) {
   XoGUI *xogui;

   xogui = Memory_Reserve(1, XoGUI);
   xogui->win = XoWin_CreatePos(x, y, w, h);
   if (xogui->win == NULL) {
           Memory_Free(xogui, XoGUI);
           return NULL;
   }
   XoGUI_Init(xogui);
   return xogui;
}

/*--------------------------------------------------------------------------*/

XoGUI *XoGUI_Create(int w, int h) { return XoGUI_CreatePos(-1,-1,w,h); }

/*=========================================================================-*/

XoGUI   *XoGUI_CreateFullScreen(void) {
   XoGUI *xogui;

   xogui = Memory_Reserve(1, XoGUI);
   xogui->win = XoWin_CreateFullScreen();
   if (xogui->win == NULL) {
           Memory_Free(xogui, XoGUI);
           return NULL;
   }
   XoGUI_Init(xogui);
   return xogui;        
}

/*=========================================================================-*/
/* Information queries */

BitMap *XoGUI_Canvas(XoGUI *xo) { return xo->bm; }

/*--------------------------------------------------------------------------*/

int XoGUI_SizeX(XoGUI *xo) { return BitMap_SizeX(xo->bm); }

/*--------------------------------------------------------------------------*/

int XoGUI_SizeY(XoGUI *xo) { return BitMap_SizeY(xo->bm); }

/*--------------------------------------------------------------------------*/

void XoGUI_MousePos(XoGUI *xo, int *x, int *y) {
   int pos = XoWin_PointerPos(xo->win);
   *y = pos&65535; *x = pos>>16;
}

/*--------------------------------------------------------------------------*/

void XoGUI_WindowPos(XoGUI *xo, int *x, int *y) {
   int pos = XoWin_Pos(xo->win);
   *y = pos&65535; *x = pos>>16;
}

/*--------------------------------------------------------------------------*/

char XoGUI_Key(XoGUI *xo) { return xo->key; } 

/*--------------------------------------------------------------------------*/
/* ids over XOGUI_PRIVATE are internal use, to get their parent */
/* XOGUI_MULT */
int XoGUI_ClickedId(XoGUI *xo) { 
   if (xo->activated_id >= XOGUI_PRIVATE)
      return (xo->activated_id - XOGUI_PRIVATE); 
   else
      return xo->activated_id; 
}

/*--------------------------------------------------------------------------*/
/* A xodget from last update with specified id */
Xodget *XoGUI_LastUpdateXodget(XoGUI *xo, int id) {
   Xodget xtemp;
   int ix;
   xtemp.id = id;
   ix = DArray_BinSearchIx(xo->last_xodget, &xtemp, (CompFunc *)Xodget_CompareId);   
   return ix >= 0 ? (Xodget *)DArray_Elem(xo->last_xodget, ix) : NULL;
}

/*--------------------------------------------------------------------------*/

Xodget *XoGUI_XodgetWithId(XoGUI *xo, int id) {
   unsigned int i;
   Xodget *xt;
   for (i=0; i<DArray_NofElems(xo->xodget); i++) {
      xt = (Xodget *)DArray_Elem(xo->xodget, i);
      if (xt != NULL && Xodget_Id(xt) == id) return xt;
   } 
   /* Failed, last resort is that id exists among the last update.. */
   return XoGUI_LastUpdateXodget(xo, id);
}

/*--------------------------------------------------------------------------*/

void *XoGUI_GetData(XoGUI *xo, int id) { 
   Xodget *xt = XoGUI_XodgetWithId(xo, id);
   if (xt != NULL) return Xodget_Ptr(xt);
   else return NULL;
}

/*--------------------------------------------------------------------------*/

int XoGUI_GetValue(XoGUI *xo, int id) { 
   Xodget *xt = XoGUI_XodgetWithId(xo, id);
   if (xt != NULL) return Xodget_Data(xt);
   else return -1;
}

/*=========================================================================-*/
/* Parameter modifiers */

void XoGUI_SetWindowTitle(XoGUI *xo, const String *t) { XoWin_SetTitle(xo->win, t); }

/*--------------------------------------------------------------------------*/

void XoGUI_SetStyle(XoGUI *xo, int style) { xo->style = style; }
int  XoGUI_Style(XoGUI *xo) { return xo->style; }

/*--------------------------------------------------------------------------*/
/* Enable use of negative coordinates */
int XoGUI_XC(XoGUI *xo, int x) { return x + (x>=0?xo->view_x1:xo->view_x2); }
int XoGUI_YC(XoGUI *xo, int y) { return y + (y>=0?xo->view_y1:xo->view_y2); }

/*--------------------------------------------------------------------------*/
/* Negative xodget ids means don't modify id */
//int XoGUI_ID(XoGUI *xo, int id) { return (id<0) ? -id : id * XOGUI_MULT; }

/*--------------------------------------------------------------------------*/

void XoGUI_SetClip(XoGUI *xo, int x1, int y1, int x2, int y2) {
   BitMap_SetClipRect(xo->bm, x1, y1, x2, y2);
}

/*--------------------------------------------------------------------------*/

void XoGUI_SetView(XoGUI *xo, int x1, int y1, int x2, int y2) {
   int xtemp, ytemp;
   /* TODO! Finish implementing the view stack */
   IDArray_Push(xo->viewstack, xo->view_x1);
   IDArray_Push(xo->viewstack, xo->view_y1);
   IDArray_Push(xo->viewstack, xo->view_x2);
   IDArray_Push(xo->viewstack, xo->view_y2);
   
   /* Need to use temp so the value of x2 is not affected by the new value of x1 */
   xtemp = XoGUI_XC(xo, x2);
   xo->view_x1 = XoGUI_XC(xo, x1); 
   xo->view_x2 = xtemp;

   ytemp = XoGUI_YC(xo, y2);
   xo->view_y1 = XoGUI_YC(xo, y1);    
   xo->view_y2 = ytemp;

   XoGUI_SetClip(xo, xo->view_x1, xo->view_y1, xo->view_x2, xo->view_y2);
}

/*--------------------------------------------------------------------------*/

void XoGUI_ResetView(XoGUI *xo) {
   xo->view_x1 = 0; 
   xo->view_y1 = 0;
   xo->view_x2 = BitMap_SizeX(xo->bm);
   xo->view_y2 = BitMap_SizeY(xo->bm);
   BitMap_ResetClipRect(xo->bm);
}

/*--------------------------------------------------------------------------*/

void XoGUI_RestoreView(XoGUI *xo) {
   if (IDArray_NofElems(xo->viewstack)>=4) {
      xo->view_y2 = IDArray_Pop(xo->viewstack);
      xo->view_x2 = IDArray_Pop(xo->viewstack);
      xo->view_y1 = IDArray_Pop(xo->viewstack);
      xo->view_x1 = IDArray_Pop(xo->viewstack);
      BitMap_SetClipRect(xo->bm, xo->view_x1, xo->view_y1, xo->view_x2, xo->view_y2);
   } else
      XoGUI_ResetView(xo);
}

/*--------------------------------------------------------------------------*/

void XoGUI_AddXodget(XoGUI *xo, Xodget *xt) { 
   if (Xodget_Id(xt) > 0) DArray_Add(xo->xodget, xt); 
   else Xodget_Destroy(xt);
}
/*--------------------------------------------------------------------------*/

int XoGUI_NofXodgets(XoGUI *xo) { return DArray_NofElems(xo->xodget); }
Xodget *XoGUI_Xodget(XoGUI *xo, int n) { return (Xodget *)DArray_Elem(xo->xodget, n); }

/*--------------------------------------------------------------------------*/

void xogui_copy_string_into_xodget(const String *a, Xodget *xt) {
   if (xt->ptr == a) return;
   if (xt->ptr != NULL) Memory_Free(xt->ptr, void);
   xt->ptr = (String *)Memory_ReserveSize(1+XOGUI_TEXTFIELD_MAX, void);
   Memory_Copy(xt->ptr, a, 1+String_Length(a));
}

/*--------------------------------------------------------------------------*/

void XoGUI_SetValue(XoGUI *xo, int id, int val) {
   Xodget *xt = XoGUI_LastUpdateXodget(xo, id);
   if (xt != NULL) Xodget_SetData(xt, val);
}

/*--------------------------------------------------------------------------*/

void XoGUI_SetData(XoGUI *xo, int id, const void *data) {
   Xodget *xt = XoGUI_LastUpdateXodget(xo, id);
   if (xt != NULL) xogui_copy_string_into_xodget((const char *)data, xt);
}

/*--------------------------------------------------------------------------*/

void XoGUI_Button2Pushed(XoGUI *xo, int id, int x1, int y1, int x2, int y2, const String *text) {
   Xodget *xt;
   x1 = XoGUI_XC(xo, x1);
   y1 = XoGUI_YC(xo, y1);
   x2 = XoGUI_XC(xo, x2);
   y2 = XoGUI_YC(xo, y2);

   if (xo->style == 0) {
      BitMap_PushButton(xo->bm, x1, y1, x2, y2,
         xo->surface, xo->shadow, xo->shadow, xo->light, xo->light,
         XOGUI_BUTTONHEIGHT);
      y2+=2;
      x2+=2;
   } else {   
      BitMap_DrawThickRect(xo->bm, x1-xo->panel_thickness, y1-xo->panel_thickness, 
         x2+xo->panel_thickness, y2+xo->panel_thickness, pixelcolor_black, xo->panel_thickness);
      BitMap_RoundedPushButton(xo->bm, x1, y1, x2, y2,
         xo->surface, xo->shadow, xo->shadow, xo->light, xo->light,
         3*XOGUI_BUTTONHEIGHT);
      y2+=2;
      x2+=2;
   }
   if (text != NULL && text[0] != 0)
      FSFont_PutThickTextCenter(xo->font, xo->bm, (x1+x2)>>1,  (3+y1+y2)>>1, text, xo->text, xo->text_thickness);
   xt = Xodget_Create2(id, x1, y1, x2, y2);
   xogui_copy_string_into_xodget(text, xt);
   DArray_Add(xo->xodget, xt);
}

/*--------------------------------------------------------------------------*/

void XoGUI_Button2(XoGUI *xo, int id, int x1, int y1, int x2, int y2, const String *text) {
   Xodget *xt;
   
   if (xo->xodget_id == id) return XoGUI_Button2Pushed(xo, id, x1, y1, x2, y2, text);
   
   x1 = XoGUI_XC(xo, x1); 
   y1 = XoGUI_YC(xo, y1); 
   x2 = XoGUI_XC(xo, x2); 
   y2 = XoGUI_YC(xo, y2);

   if (xo->style == 0) {
      BitMap_PushButton(xo->bm, x1, y1, x2, y2,
         xo->surface, xo->light, xo->light, xo->shadow, xo->shadow,
         XOGUI_BUTTONHEIGHT);
   } else {   
      BitMap_DrawThickRect(xo->bm, x1-xo->panel_thickness, y1-xo->panel_thickness, 
         x2+xo->panel_thickness, y2+xo->panel_thickness, pixelcolor_black, xo->panel_thickness);
      BitMap_RoundedPushButton(xo->bm, x1, y1, x2, y2,
         xo->surface, xo->light, xo->light, xo->shadow, xo->shadow,
         3*XOGUI_BUTTONHEIGHT);
   }
   if (text != NULL && text[0] != 0)
      FSFont_PutThickTextCenter(xo->font, xo->bm, (x1+x2)>>1,  (3+y1+y2)>>1, text, xo->text, xo->text_thickness);
   xt = Xodget_Create2(id, x1, y1, x2, y2);
   xogui_copy_string_into_xodget(text, xt);
   DArray_Add(xo->xodget, xt);
}

/*--------------------------------------------------------------------------*/

void XoGUI_Button(XoGUI *xo, int id, int x, int y, const String *text) {
   int h = FSFont_Height(xo->font)+3*XOGUI_BUTTONHEIGHT;
   int w = FSFont_TextWidth(xo->font, text)+4*XOGUI_BUTTONHEIGHT;
   if (w < h*3) w = h*3;
   XoGUI_Button2(xo, id, x, y, x+w, y+h, text);
}

/*--------------------------------------------------------------------------*/

void XoGUI_Panel(XoGUI *xo, int id, int x1, int y1, int x2, int y2) {
   x1 = XoGUI_XC(xo, x1); 
   y1 = XoGUI_YC(xo, y1); 
   x2 = XoGUI_XC(xo, x2); 
   y2 = XoGUI_YC(xo, y2);

   if (xo->style == 0) {
      BitMap_PushButton(xo->bm, x1, y1, x2, y2,
         xo->surface, xo->light, xo->light, xo->shadow, xo->shadow, xo->panel_thickness);
   } else {   
      BitMap_DrawThickRect(xo->bm, x1+4, y1+4, x2,   y2, xo->surface, xo->panel_thickness);
      BitMap_DrawThickRect(xo->bm, x1+2, y1+2, x2,   y2, xo->shadow, xo->panel_thickness);
      BitMap_DrawThickRect(xo->bm, x1,   y1,   x2-2, y2-2, xo->light, xo->panel_thickness);
   }
   DArray_Add(xo->xodget, Xodget_Create2(id, x1, y1, x2, y2));
}

/*--------------------------------------------------------------------------*/

void XoGUI_SunkenPanel(XoGUI *xo, int id, int x1, int y1, int x2, int y2) {
   x1 = XoGUI_XC(xo, x1); 
   y1 = XoGUI_YC(xo, y1); 
   x2 = XoGUI_XC(xo, x2); 
   y2 = XoGUI_YC(xo, y2);

   if (xo->style == 0) {
      BitMap_PushButton(xo->bm, x1-xo->panel_thickness, y1-xo->panel_thickness, x2+xo->panel_thickness, y2+xo->panel_thickness,
         xo->light, xo->shadow, xo->shadow, xo->light, xo->light, xo->panel_thickness);
   } else {
      int d=xo->panel_thickness/2;
      BitMap_DrawThickRect(xo->bm, x1-xo->panel_thickness, y1-xo->panel_thickness, x2+xo->panel_thickness, y2+xo->panel_thickness,
         xo->shadow, xo->panel_thickness);
      BitMap_RoundedPushButton(xo->bm, x1-xo->panel_thickness+d, y1-xo->panel_thickness+d, x2+xo->panel_thickness, y2+xo->panel_thickness,
         xo->light, xo->shadow, xo->shadow, xo->light, xo->light, 3*xo->panel_thickness);
   }
   DArray_Add(xo->xodget, Xodget_Create2(id, x1, y1, x2, y2));
}

/*--------------------------------------------------------------------------*/

void XoGUI_PlotArea(XoGUI *xo, int id, int x1, int y1, int x2, int y2) {
   x1 = XoGUI_XC(xo, x1); 
   y1 = XoGUI_YC(xo, y1); 
   x2 = XoGUI_XC(xo, x2); 
   y2 = XoGUI_YC(xo, y2);

   XoGUI_SunkenPanel(xo, id, x1, y1, x2, y2);
   XoGUI_SetView(xo, (x1+x2)/2, (y1+y2)/2, (x1+x2)/2, (y1+y2)/2);
   BitMap_SetClipRect(xo->bm, x1, y1, x2, y2);
}

/*--------------------------------------------------------------------------*/

void XoGUI_Printf(XoGUI *xo, int id, int x, int y, const String *format, ...) {
   String *output;
   Xodget *xt;
   va_list a;
   
   x = XoGUI_XC(xo, x); 
   y = XoGUI_YC(xo, y);
//   id*=XOGUI_MULT;
   
   va_start(a, format);   
   output = String_CreateVPrintf(format, a);
   va_end(a);
   FSFont_PutThickText(xo->font, xo->bm, x, y, output, xo->text, xo->text_thickness);
   xt = Xodget_Create2(id, x, y, x+FSFont_TextWidth(xo->font, output), y+FSFont_Height(xo->font));
   xogui_copy_string_into_xodget(output, xt);
   DArray_Add(xo->xodget, xt);
   String_Destroy(output);
}

/*--------------------------------------------------------------------------*/

void XoGUI_PrintfCentered(XoGUI *xo, int id, int x, int y, const String *format, ...) {
   String *output;
   Xodget *xt;
   va_list a;
   int w, h;
   
   x = XoGUI_XC(xo, x); 
   y = XoGUI_YC(xo, y);
   
   va_start(a, format);   
   output = String_CreateVPrintf(format, a);
   va_end(a);
   h = FSFont_Height(xo->font);
   w = FSFont_TextWidth(xo->font, output);
   x -= (w+1)/2; 
   y -= (h+1)/2;
   FSFont_PutThickText(xo->font, xo->bm, x, y, output, xo->text, xo->text_thickness);
   xt = Xodget_Create2(id, x, y, x+w, y+h);
   xogui_copy_string_into_xodget(output, xt);
   DArray_Add(xo->xodget, xt);
   String_Destroy(output);
}

/*--------------------------------------------------------------------------*/

void XoGUI_TextInput2(XoGUI *xo, int id, int x1, int y1, int x2, int y2, String *initial) {
   Xodget *xt, *xtprev;
   int cx, l, xp, yp;
   String *text;

   /* Adjust coordinates */
   x1 = XoGUI_XC(xo, x1); 
   y1 = XoGUI_YC(xo, y1); 
   x2 = XoGUI_XC(xo, x2); 
   y2 = XoGUI_YC(xo, y2);

   xtprev = XoGUI_LastUpdateXodget(xo, id);
   if (xtprev == NULL) {
      cx = MIN(String_Length(initial), XOGUI_TEXTFIELD_MAX);
      text = String_Create(XOGUI_TEXTFIELD_MAX+1);
      Memory_Copy(text, initial, cx);
   } else {
      text = (String *)Xodget_Ptr(xtprev);
//     cx = MIN(Xodget_Data(xtprev), MIN(XOGUI_TEXTFIELD_MAX, String_Length(text)));
   	cx = String_Length(text);
   }
      
   xt = Xodget_Create2(id, x1, y1, x2, y2);   
   if (xo->focus_id == Xodget_Id(xt)) {
      if (xo->key >= ' ' && xo->key <= 255) {
         text[cx] = xo->key;         
         if (cx<XOGUI_TEXTFIELD_MAX-1) cx++;
      } else {
         if (xo->key == 8 || xo->key== 0x7f) { if (cx>0) cx--; }
      }
   }
   text[cx] = 0;
   Xodget_SetData(xt, cx);
   xogui_copy_string_into_xodget(text, xt);
   
   l = FSFont_TextWidth(xo->font, text);
   if (l>x2-x1-XOGUI_CURSORWIDTH) xp = x2-l-XOGUI_CURSORWIDTH; else xp=x1+xo->panel_thickness;
   yp = xo->panel_thickness + (1+y1+y2 -FSFont_Height(xo->font))/2;
   XoGUI_SunkenPanel(xo, 0, x1, y1, x2, y2);

   if (xo->focus_id == id) {
      BitMap_FillRect(xo->bm, xp+l, yp, xp+l+XOGUI_CURSORWIDTH, yp+FSFont_Height(xo->font), xo->selection);
   } else {            
      BitMap_DrawThickRect(xo->bm, xp+l, yp, xp+l+XOGUI_CURSORWIDTH, yp+FSFont_Height(xo->font), xo->selection,2);
   }
   BitMap_SetClipRect(xo->bm, x1, y1, x2, y2);
   FSFont_PutThickText(xo->font, xo->bm, xp, yp, text, xo->text, xo->text_thickness);
   BitMap_ResetClipRect(xo->bm);

   xp = XoWin_PointerPos(xo->win);
   yp = xp&65535;
   xp>>=16;
   if (Xodget_IsInside(xt, xp, yp)) 
      XoWin_SetCursor(xo->win, 152); 
   
   DArray_Add(xo->xodget, xt);   
   if (xtprev == NULL) String_Destroy(text);   
}

/*--------------------------------------------------------------------------*/

void XoGUI_TextInput(XoGUI *xo, int id, int x1, int y1, int w, String *initial) {
   XoGUI_TextInput2(xo, id, x1, y1, x1+w, y1+FSFont_Height(xo->font)+2, initial);
}

/*--------------------------------------------------------------------------*/

void XoGUI_CheckBox(XoGUI *xo, int id, int x1, int y1, const String *message, int initial) {
   Xodget *xt, *xtprev;
   int w, h, yp;
   x1 = XoGUI_XC(xo, x1); 
   y1 = XoGUI_YC(xo, y1); 
   
   h = FSFont_Height(xo->font);
   w = FSFont_Width(xo->font);
   yp = (y1+y1+h)/2 - w/2;
   XoGUI_SunkenPanel(xo, 0, x1, yp, x1+w, yp+w);
   xt = Xodget_Create2(id, x1, y1, x1+w+FSFont_TextWidth(xo->font, message), y1+h);
   xtprev = XoGUI_LastUpdateXodget(xo, Xodget_Id(xt));
   if (xtprev != NULL) {
      Xodget_SetData(xt, Xodget_Data(xtprev));
   } else Xodget_SetData(xt, initial);
   
   if (xo->activated_id == Xodget_Id(xt)) Xodget_SetData(xt, !Xodget_Data(xt));
   if (Xodget_Data(xt)) XoGUI_Printf(xo, 0, x1, yp-w/2, "x");
   XoGUI_Printf(xo, 0, x1+2*w, y1, message);
   XoGUI_AddXodget(xo, xt);
}

/*--------------------------------------------------------------------------*/

int XoGUI_DialogN(XoGUI *xo, const String *message, int nof_buttons, ... ) {
   int n, x, w, h, i, m;
   XoGUI *d;
   DArray *buttons;
   va_list buts;
   
   if (nof_buttons <= 0) return -1;
   x = XoWin_Pos(xo->win);
//   y = x & 0xffff;
   x >>= 16;
   buttons = DArray_Create(nof_buttons+1);
   va_start(buts, nof_buttons);
   for (i=0; i<nof_buttons; i++) DArray_Add(buttons, va_arg(buts, String*));
   va_end(buts);

   w = 0;
   m = FSFont_TextWidth(xo->font, (const String *)DArray_Elem(buttons, 0));
   for (i=1; i<nof_buttons; i++) 
      m = MAX(m, FSFont_TextWidth(xo->font, (const String *)DArray_Elem(buttons, i)));
   /* m is the widest button width 
      w is the automatic width of the dialog
   */
   m += 6*XOGUI_BUTTONHEIGHT;
   w = MAX(nof_buttons*(m+8), FSFont_TextWidth(xo->font, message)) + 32;
   h = FSFont_Height(xo->font)*2 + 12*XOGUI_BUTTONHEIGHT + 20;
   d = XoGUI_CreatePos(XoGUI_SizeX(xo)/2 - w/2, XoGUI_SizeY(xo)/2 - h/2, w, h);
   do {
      XoGUI_SetStyle(d, XoGUI_Style(xo));
      XoGUI_SetColorScheme(d, xo->surface.r, xo->surface.g, xo->surface.b);

      XoGUI_Clear(d);
      XoGUI_PrintfCentered(d, 0, w/2 + XOGUI_BUTTONHEIGHT, FSFont_Height(xo->font) / 2 + 10 + 3*XOGUI_BUTTONHEIGHT, "%s", message);
      for (i=0; i<nof_buttons; i++) {
         XoGUI_Button(d, i+1, i*(m+8) + 24, -3*FSFont_Height(xo->font)/2 - 6*XOGUI_BUTTONHEIGHT - 2, (const String *)DArray_Elem(buttons, i));
      }
      XoGUI_Update(d, 0.05);
      n = XoGUI_ClickedId(d);
   } while(n <= 0);
   DArray_Destroy(buttons, NULL);
   XoGUI_Destroy(d);
   return n-1;
}

/*==========================================================================*/
/* Attributes */
/*--------------------------------------------------------------------------*/

void XoGUI_SetTextColor(XoGUI *xo, int r, int g, int b) { xo->text = PixelColor_FromRGB(r,g,b); }
void XoGUI_SetSelectionColor(XoGUI *xo, int r, int g, int b) { xo->selection = PixelColor_FromRGB(r,g,b); }
/* Set the surface color, and then adjust light and dark suitably */
void XoGUI_SetShadowColor(XoGUI *xo, int r, int g, int b) { xo->shadow = PixelColor_FromRGB(r,g,b); }
void XoGUI_SetLightColor(XoGUI *xo, int r, int g, int b)  { xo->light = PixelColor_FromRGB(r,g,b); }
void XoGUI_SetSurfaceColor(XoGUI *xo, int r, int g, int b)  { xo->surface = PixelColor_FromRGB(r,g,b); }

void XoGUI_SetColorScheme(XoGUI *xo, int r, int g, int b) {
   xo->surface = PixelColor_FromRGB(r,g,b);
   xo->shadow = PixelColor_FromRGB(r/2,g/2,b/2);
   r = MIN(5*r / 3, 255);
   g = MIN(5*g / 3, 255);
   b = MIN(5*b / 3, 255);
   xo->light = PixelColor_FromRGB(r,g,b);
}

/*--------------------------------------------------------------------------*/

Xodget *XoGUI_FindXodget(XoGUI *xog, int x, int y) {
   Xodget *xo;
   int i;
   
   /* Loop backwards so the most recent xodgets have higher priority */
   for (i=DArray_NofElems(xog->xodget)-1; i>=0; i--) {
      xo = (Xodget *)DArray_Elem(xog->xodget, i);
/*	      printf("Event (x,y), Xodget %i (%i,%i)-(%i,%i)\n", x, y, xo->id, 
              Xodget_LeftX(xo), Xodget_UpperY(xo), Xodget_RightX(xo), Xodget_LowerY(xo)); */
      if (Xodget_IsInside(xo,x,y) && Xodget_Id(xo)>0) return xo;
   }
   return NULL;
}

/*==========================================================================*/
/*                    GUI UPDATE AND EVENT PROCESSING                       */
/*--------------------------------------------------------------------------*/

int XoGUI_Update(XoGUI *xogui, double timeout) {
   int ret;
   unsigned int i;
   char k=0;
   Xodget *xt;
   
   XoWin_PutBitMap(xogui->win, xogui->bm, 0, 0);
   xogui->activated_id = -1;
   XoGUI_ResetView(xogui);
//   XoWin_ResetCursor(xogui->win);

    if (XoWin_NofPending(xogui->win)) {
        ret = 1;
        XoWin_HandleEvent(xogui->win, &xogui->mousex, &xogui->mousey, &k);
    } else
        ret = XoWin_WaitUserInput(xogui->win, &xogui->mousex, &xogui->mousey, &k, timeout);

   if (ret & 1 && k>0) xogui->key = XoWin_KeyCodeToASCII(xogui->win, k); else xogui->key = 0;

   if (XoWin_SizeX(xogui->win) != BitMap_SizeX(xogui->bm) || 
      XoWin_SizeY(xogui->win) != BitMap_SizeY(xogui->bm)) {
         xogui->bm = XoWin_BitMap(xogui->win);
   }
   
//   if (XoWin_IsPressed(xogui->win, XOWIN_LMB)) { printf("."); fflush(stdout); }
   if (XoWin_IsPressed(xogui->win, XOWIN_LMB)) {
//printf("x y,k = %i %i, %i\n", xogui->mousex, xogui->mousey, k);
      xt = XoGUI_FindXodget(xogui, xogui->mousex, xogui->mousey);
      if (xt != NULL && Xodget_Id(xt)>0) xogui->xodget_id = Xodget_Id(xt);
      else xogui->xodget_id = -1;      
      if ((signed)xogui->grabpos < 0) xogui->grabpos = XoWin_PointerPos(xogui->win);
   } else
   if ((k == -XOWIN_LMB || (XoWin_IsPressed(xogui->win, XOWIN_LMB) && k==0)) && xogui->xodget_id > 0) {
      xt = XoGUI_FindXodget(xogui, xogui->mousex, xogui->mousey);
      if (xt != NULL) xogui->activated_id = Xodget_Id(xt);
      xogui->xodget_id = -1;
      xogui->grabpos = -1;
      xogui->focus_id = xogui->activated_id;      
   }
//   printf("focus = %i, activated = %i, xodget = %i\n", xogui->focus_id, xogui->activated_id, xogui->xodget_id);
   
   /* Emptyness check is a safe-guard for several update calls w/o painting */
   if (!DArray_IsEmpty(xogui->xodget)) {
      DArray_Destroy(xogui->last_xodget, (DestFunc *)Xodget_Destroy);      
      xogui->last_xodget = DArray_Create(DArray_NofElems(xogui->xodget));
      for (i=0; i<DArray_NofElems(xogui->xodget); i++) { 
         DArray_InsertSorted(xogui->last_xodget, DArray_Elem(xogui->xodget, i), (CompFunc *)Xodget_CompareId);
      }  /* TODO: Write a sorter, better than insertion sort... */
   	xogui->xodget->nof_elems = 0;
   }

   return ret;
}

/*--------------------------------------------------------------------------*/

int XoGUI_IsKeyPressed(XoGUI *xo, int scancode) {
   return XoWin_IsPressed(xo->win, scancode);
}

/*--------------------------------------------------------------------------*/

void XoGUI_Line(XoGUI *xo, int x1, int y1, int x2, int y2, PixelColor p) {
   x1 = XoGUI_XC(xo, x1);
   y1 = XoGUI_YC(xo, y1);
   x2 = XoGUI_XC(xo, x2);
   y2 = XoGUI_YC(xo, y2);
   BitMap_DrawLineAA(xo->bm, x1, y1, x2, y2, p);
}

/*--------------------------------------------------------------------------*/

void XoGUI_Circle(XoGUI *xo, int x, int y, int r, PixelColor p) {
   BitMap_Circle(xo->bm, XoGUI_XC(xo, x), XoGUI_YC(xo, y), r, p);
}

/*--------------------------------------------------------------------------*/

void XoGUI_FillCircle(XoGUI *xo, int x, int y, int r, PixelColor p) {
   BitMap_FillCircle(xo->bm, XoGUI_XC(xo, x), XoGUI_YC(xo, y), r, p);
}

/*--------------------------------------------------------------------------*/

#ifdef CMOD_PNG
BitMap  *XoGUI_LoadPNG(XoGUI *gui, const char *filename) {
        BitMap *bm = BitMap_ReadPNGFile(filename);
        
        if (bm == NULL && filename != NULL) {
                String *fn = String_Create(String_Length(filename)+64);
                String_Printf(fn, "Could not load PNG file %s\n", filename);
                XoGUI_DialogN(gui, fn, 1, "OK");
                String_Destroy(fn);
        }
        return bm;
}
#endif

/*--------------------------------------------------------------------------*/

void XoGUI_FillRectangle(XoGUI *gui, int x1, int y1, int x2, int y2, PixelColor p) {
   BitMap_FillRect(gui->bm, XoGUI_XC(gui, x1), XoGUI_YC(gui, y1), XoGUI_XC(gui, x2), XoGUI_YC(gui, y2), p);
}

/*--------------------------------------------------------------------------*/

void XoGUI_PutPixel(XoGUI *gui, int x, int y, PixelColor p) {
        BitMap_PutPixel(gui->bm, XoGUI_XC(gui, x), XoGUI_YC(gui, y),p);
}

/*--------------------------------------------------------------------------*/

void XoGUI_PutImage(XoGUI *gui, int id, BitMap *bm, int x, int y) {
   int x1, x2, y1, y2;

   if (x<0) x-= BitMap_SizeX(bm);
   if (y<0) y-= BitMap_SizeY(bm);
   
   x1 = XoGUI_XC(gui, x);
   y1 = XoGUI_YC(gui, y);
   x2 = XoGUI_XC(gui, x + BitMap_SizeX(bm));
   y2 = XoGUI_YC(gui, y + BitMap_SizeY(bm));

   BitMap_PutBitMapAlpha(gui->bm, bm, x1, y1);
   DArray_Add(gui->xodget, Xodget_Create2(id, x1, y1, x2, y2));
}

/*--------------------------------------------------------------------------*/

void XoGUI_Restore(XoGUI *xogui) {
	XoWin_Destroy(xogui->win);
	FSFont_Destroy(xogui->font);
	DArray_Destroy(xogui->xodget, (DestFunc *)Xodget_Destroy);
        DArray_Destroy(xogui->last_xodget, (DestFunc *)Xodget_Destroy);
        IDArray_Destroy(xogui->viewstack);
}

/*--------------------------------------------------------------------------*/

BitMap *XoGUI_BorrowCanvas(XoGUI *gui) { return gui->bm; }

/*--------------------------------------------------------------------------*/

int XoGUI_Socket(XoGUI *gui) { return XoWin_Socket(gui->win); }

/*--------------------------------------------------------------------------*/

void XoGUI_Destroy(XoGUI *xogui) {
	XoGUI_Restore(xogui);
	Memory_Free(xogui, XoGUI);
}

/*--------------------------------------------------------------------------*/

