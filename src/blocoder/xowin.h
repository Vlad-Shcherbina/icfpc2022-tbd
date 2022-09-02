
#ifndef _xowin_h
#define _xowin_h

#include "bitmap.h"
#include "cmod.h"
#include "string.h"
#include <X11/Xlib.h>

#define XOWIN_XSHM

#ifdef XOWIN_XSHM
#include <X11/extensions/XShm.h>  
#endif

typedef struct {
   Display *dpy; // connection to xserver
	unsigned int blackcolor, whitecolor;
	GC gc; // graphical context
	Window mainwin;
   Visual *visual;
	int mainwinsizex, mainwinsizey; // size of window
   BitMap *bitmap;
   XImage *shared_image;
   /* 256k for... this!? 32 bytes should do (256 bits) */
   Bool pressed[32768];
#ifdef XOWIN_XSHM   
   XShmSegmentInfo shminfo;
#endif
} XoWin;

XoWin  *XoWin_Create(int sizex, int sizey);
XoWin  *XoWin_CreatePos(int x, int y, int sizex, int sizey);
XoWin  *XoWin_CreateFullScreen(void);
void    XoWin_Destroy(XoWin *xowin);
void    XoWin_RefreshBitMap(XoWin *xowin); /* Activates XSHM and updates SHM image when resized */
void    XoWin_PutBitMap(XoWin *xowin, BitMap *bm, int x, int y);
int     XoWin_PointerPos(XoWin *xowin); /* High 16 bits is x (signed), low 16 y (IntCoord format) */
int     XoWin_Pos(XoWin *xowin); /* Window pos: High 16 = x (signed), low 16 = y */
void    XoWin_Move(XoWin *xowin, int x, int y);
void    XoWin_SetTitle(XoWin *xowin, const String *title); /* Takes ownership of title String */
#define XoWin_GetMousePos XoWin_PointerPos
//#define XoWin_SetBitMap(X,B) XoWin_PutBitMap(X,B,0,0);
#define XoWin_SizeX(x) ((x)->mainwinsizex) 
#define XoWin_SizeY(x) ((x)->mainwinsizey) 
BitMap *XoWin_BitMap(XoWin *xowin);
void    XoWin_Update(XoWin *xowin);
void    XoWin_SetCursor(XoWin *xowin, int type);
void    XoWin_ResetCursor(XoWin *xowin);
void    XoWin_SetSize(XoWin *xowin, int width, int height);
int     XoWin_InputSocket(XoWin *xowin); /* Returns socket for select() */
int     XoWin_Socket(XoWin *xowin);
void    XoWin_HideCursor(XoWin *xowin);

/* on return: lowest bit -> interaction, bit 0x2 -> resized (needs refresh) */
/* mousex, mousey, key can be NULL */
int     XoWin_IsUserInput(XoWin *xowin, Bool block, int *mousex, int *mousey, char *key);
int     XoWin_WaitUserInput(XoWin *xowin, int *mousex, int *mousey, char *key, double timeout);
Bool    XoWin_IsUserEvent(XoWin *xowin);
int     XoWin_NofPending(XoWin *xowin);
int     XoWin_HandleEvent(XoWin *xowin, int *mousex, int *mousey, char *key);
unsigned char XoWin_KeyCodeToASCII(XoWin *xowin, char keycode);
//Bool    XoWin_IsPressed(XoWin *xowin, int key);
#define XoWin_IsPressed(x,k) ((x)->pressed[(k)])
#define XoWin_IsKeyPressed XoWin_IsPressed

#define XOWIN_LMB 1
#define XOWIN_MMB 2
#define XOWIN_RMB 3
#define XOWIN_ESC 9
#define XOWIN_1 10
#define XOWIN_2 11
#define XOWIN_3 12
#define XOWIN_4 13
#define XOWIN_5 14
#define XOWIN_6 15
#define XOWIN_7 16
#define XOWIN_8 17
#define XOWIN_9 18
#define XOWIN_0 19
#define XOWIN_A 38
#define XOWIN_B 56
#define XOWIN_C 54
#define XOWIN_D 40
#define XOWIN_E 26
#define XOWIN_F 41
#define XOWIN_G 42
#define XOWIN_H 43
#define XOWIN_I 31
#define XOWIN_J 44
#define XOWIN_K 45
#define XOWIN_L 46
#define XOWIN_M 58
#define XOWIN_N 57
#define XOWIN_O 32
#define XOWIN_P 33
#define XOWIN_Q 24
#define XOWIN_R 27
#define XOWIN_S 39
#define XOWIN_T 28
#define XOWIN_U 30
#define XOWIN_V 55
#define XOWIN_W 25
#define XOWIN_X 53
#define XOWIN_Y 29
#define XOWIN_Z 52

#define XOWIN_RETURN 36
#define XOWIN_ENTER 108
#define XOWIN_CTRLL 37
#define XOWIN_ALTL 64
#define XOWIN_ALTR 113
#define XOWIN_CTRLR 109
#define XOWIN_SHIFTR 62
#define XOWIN_SHIFTL 50

#define XOWIN_BACKSPACE 22
#define XOWIN_TAB 23
#define XOWIN_SPACE 65
#define XOWIN_UP 98
#define XOWIN_LEFT 100
#define XOWIN_RIGHT 102
#define XOWIN_DOWN 104
#define XOWIN_PGUP 99
#define XOWIN_PGDN 105
#define XOWIN_HOME 97 
#define XOWIN_END 103
#define XOWIN_DEL 107
#define XOWIN_INS 106
#define XOWIN_PLUS 86
#define XOWIN_MINUS 82
#define XOWIN_TIMES 63
#define XOWIN_DIVIDE 112

#define XOWIN_F1 67
#define XOWIN_F2 68
#define XOWIN_F3 69
#define XOWIN_F4 70
#define XOWIN_F5 71
#define XOWIN_F6 72
#define XOWIN_F7 73
#define XOWIN_F8 74
#define XOWIN_F9 75
#define XOWIN_F10 76
#define XOWIN_F11 95
#define XOWIN_F12 96


#ifdef CMOD_NOMAKEFILE
#include "xowin.c"
#endif
#endif

