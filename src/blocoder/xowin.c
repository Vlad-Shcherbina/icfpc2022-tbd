
#include "bitmap.h"
#include "cmod.h"
#include "memory.h"
#include "pixelcolor.h"
#include "stdfunc.h"
#include "xowin.h"
#include <sys/select.h>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#ifdef XOWIN_XSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

/* ----------------------------------------------------------------------- */

XoWin  *XoWin_CreatePos(int x, int y, int sizex, int sizey) {
   XoWin *xowin;
   int i;
   XEvent e;
   unsigned long valuemask = 0;
   XSizeHints *sizehint;
   
   XSetWindowAttributes attribs;
   
   xowin = Memory_Reserve(1, XoWin);
   
   xowin->mainwinsizex = sizex;
   xowin->mainwinsizey = sizey;

   xowin->dpy = XOpenDisplay(NULL);
   if (xowin->dpy == NULL) {
      Memory_Free(xowin, XoWin);
      return NULL;
   }
   xowin->visual = DefaultVisual(xowin->dpy, DefaultScreen(xowin->dpy));
   xowin->blackcolor = BlackPixel(xowin->dpy, DefaultScreen(xowin->dpy));
   xowin->whitecolor = WhitePixel(xowin->dpy, DefaultScreen(xowin->dpy));

   valuemask |= CWBackingStore;
   attribs.backing_store = NotUseful;
   valuemask |= CWOverrideRedirect;
   attribs.override_redirect = TRUE;
   /* To hint initial position */
   if (x != -1 && y != -1) {
      sizehint = XAllocSizeHints();
      XGetNormalHints(xowin->dpy, DefaultRootWindow(xowin->dpy), sizehint);
      sizehint->x = x;
      sizehint->y = y;
      sizehint->flags |= PPosition | USPosition;
      XSetNormalHints(xowin->dpy, DefaultRootWindow(xowin->dpy), sizehint);
      XFree(sizehint);
   }
   
   xowin->mainwin = XCreateWindow(xowin->dpy, DefaultRootWindow(xowin->dpy), 
      		x, y, xowin->mainwinsizex, xowin->mainwinsizey, 1, CopyFromParent,
            InputOutput, xowin->visual, 0, &attribs);

	xowin->gc = XCreateGC(xowin->dpy, xowin->mainwin, 0, NULL);
   xowin->shared_image = NULL;
   xowin->bitmap = BitMap_Create(sizex, sizey);
   for (i=0; i<32768; i++) xowin->pressed[i] = FALSE;
   
	XSelectInput(xowin->dpy, xowin->mainwin, 
		StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | ExposureMask | 
      KeyPressMask | KeyReleaseMask | ButtonMotionMask | PointerMotionMask );

   XSetWindowBorderWidth(xowin->dpy, xowin->mainwin, 0);
	XMapWindow(xowin->dpy, xowin->mainwin);

	XSetForeground(xowin->dpy, xowin->gc, xowin->blackcolor);	
   XFlush(xowin->dpy);

   do {
      XNextEvent(xowin->dpy, &e);
   } while (e.type != Expose);

   
	return(xowin);
}

/* ----------------------------------------------------------------------- */

XoWin *XoWin_Create(int sizex, int sizey) {return XoWin_CreatePos(0,0,sizex,sizey);}

/* ----------------------------------------------------------------------- */

XoWin  *XoWin_CreateFullScreen(void) {
        Display *display = XOpenDisplay(NULL);
        XoWin *ret = NULL;
        if (display != NULL) {
                int screen = DefaultScreen(display);
                ret = XoWin_Create(DisplayWidth(display,screen), DisplayHeight(display, screen));
                XCloseDisplay(display);
        }

        return ret;
}

/* ----------------------------------------------------------------------- */

void   XoWin_Destroy(XoWin *xowin) {
#ifdef XOWIN_XSHM
   if (xowin->shared_image != NULL) {
      XShmDetach(xowin->dpy, &xowin->shminfo);
      XDestroyImage(xowin->shared_image);
      shmdt(xowin->shminfo.shmaddr);
      shmctl(xowin->shminfo.shmid, IPC_RMID, 0);
   }
#endif
  if (xowin->bitmap != NULL)
      BitMap_Destroy(xowin->bitmap);
   XDestroyWindow(xowin->dpy, xowin->mainwin);
   XFreeGC(xowin->dpy, xowin->gc);
   XCloseDisplay(xowin->dpy);
   Memory_Free(xowin, XoWin);
}

/* ----------------------------------------------------------------------- */

void XoWin_PutBitMap(XoWin *xowin, BitMap *bm, int x, int y) {
   XImage *image;

//   printf("%x\n", xowin->dpy);
   #ifdef XOWIN_XSHM
   if (xowin->shared_image != NULL) {
      XShmPutImage(xowin->dpy, xowin->mainwin, xowin->gc, xowin->shared_image, 0,0,x,y, BitMap_SizeX(bm)-1, BitMap_SizeY(bm)-1, FALSE);
   } else 
   #endif
   {
   /* XCreateImage does not allocate data space; pointer is copied! */
      image = XCreateImage(xowin->dpy, xowin->visual, 24, ZPixmap, 0, 
      (char *)BitMap_Buf(bm), BitMap_SizeX(bm), BitMap_SizeY(bm), BitmapPad(xowin->dpy), 0);

      XPutImage(xowin->dpy, xowin->mainwin, xowin->gc, image, 0, 0, x, y, BitMap_SizeX(bm)-1, BitMap_SizeY(bm)-1);
      free(image);
   }
   XFlush(xowin->dpy);
}

/* ----------------------------------------------------------------------- */

/* Takes ownership of title */
void XoWin_SetTitle(XoWin *xowin, const String *title) {
/*
   XTextProperty windowname;

   windowname.nitems = String_Length(title);
   windowname.value = title;
   windowname.encoding = XA_STRING;
   windowname.format = 8;
   XSetWMName(xowin->dpy, xowin->mainwin, &windowname);
*/
   XStoreName(xowin->dpy, xowin->mainwin, title);
   XFlush(xowin->dpy);
}

/* ----------------------------------------------------------------------- */

void XoWin_Move(XoWin *xowin, int x, int y) { XMoveWindow(xowin->dpy, xowin->mainwin, x, y); }

/* ----------------------------------------------------------------------- */

int XoWin_Pos(XoWin *xowin) {
   XWindowAttributes xa;
   XGetWindowAttributes(xowin->dpy, xowin->mainwin, &xa);
   return (xa.x << 16) + xa.y;
}

/* ----------------------------------------------------------------------- */

void XoWin_Update(XoWin *xowin) { XFlush(xowin->dpy); }

/* ----------------------------------------------------------------------- */

BitMap *XoWin_BitMap(XoWin *xowin) { return xowin->bitmap; }

/* ----------------------------------------------------------------------- */

void XoWin_RefreshBitMap(XoWin *xowin) {
   XImage *im;
   unsigned int xw,yw,depth, border_width;
   int x,y;
   Window root;
   
   XGetGeometry(xowin->dpy, xowin->mainwin, &root, &x, &y, &xw, &yw, &border_width, &depth);
  
#ifdef XOWIN_XSHM
   if (XShmQueryExtension(xowin->dpy)) {
      im = XShmCreateImage(xowin->dpy, xowin->visual, depth, ZPixmap, NULL, &xowin->shminfo, xw, yw);

      xowin->shminfo.shmid = shmget(IPC_PRIVATE, im->bytes_per_line * im->height, IPC_CREAT|0777);

      xowin->shminfo.shmaddr = im->data = shmat(xowin->shminfo.shmid, 0, 0);
      xowin->shminfo.readOnly = FALSE;
      xowin->shared_image = im;
      XShmAttach(xowin->dpy, &xowin->shminfo);
   } else
#endif
   im = XGetImage(xowin->dpy, xowin->mainwin, 0, 0, xw, yw, AllPlanes, ZPixmap);

   BitMap_SetSize(xowin->bitmap, xw, yw);

#ifndef XOWIN_XSHM
   Memory_Free(xowin->bitmap->buf, PixelColor);
#endif
//   free(xowin->bitmap->buf);
   xowin->bitmap->buf = (PixelColor *)im->data;
}

/* ----------------------------------------------------------------------- */

int XoWin_HandleEvent(XoWin *xowin, int *mousex, int *mousey, char *key) {
   XEvent e;
   Bool interaction;
   Bool resize;
   int sizex, sizey;
   int ret;
   
   interaction = FALSE;
   resize = FALSE;
   ret = 0;
   sizex = sizey = 0; // To avoid compiler warning
   
   while (XPending(xowin->dpy) > 0) {
		XNextEvent(xowin->dpy, &e);
      switch (e.type) {
      case KeyPress:
         if (mousex != NULL) *mousex = e.xkey.x;
			if (mousey != NULL) *mousey = e.xkey.y;
         if (key != NULL) *key = e.xkey.keycode;
         xowin->pressed[e.xkey.keycode] = TRUE;
         interaction = TRUE;
         break;

      case KeyRelease:
			if (mousex != NULL) *mousex = e.xkey.x;
			if (mousey != NULL) *mousey = e.xkey.y;
         if (key != NULL) *key = -e.xkey.keycode;         
         xowin->pressed[e.xkey.keycode] = FALSE;
         interaction = TRUE;
         break;
         
      case MotionNotify:
			if (mousex != NULL) *mousex = e.xmotion.x;
			if (mousey != NULL) *mousey = e.xmotion.y;
         interaction = TRUE;
         break;

		case ButtonPress:
			if (mousex != NULL) *mousex = e.xbutton.x;
			if (mousey != NULL) *mousey = e.xbutton.y;
         if (key != NULL) *key = e.xbutton.button;
         xowin->pressed[e.xbutton.button] = TRUE;
         interaction = TRUE;
         break;

		case ButtonRelease:
			if (mousex != NULL) *mousex = e.xbutton.x;
			if (mousey != NULL) *mousey = e.xbutton.y;
         if (key != NULL) *key = -e.xbutton.button;
         xowin->pressed[e.xbutton.button] = FALSE;
         interaction = TRUE;
         break;

		case Expose:
//       XoWin_PutBitMap(xowin, xowin->bitmap, 0, 0);         
   		break;
      
		case ResizeRequest:
		case ConfigureNotify:
         sizex = e.xconfigure.width;
         sizey = e.xconfigure.height;
         resize = TRUE;
/*
   		BitMap_SetSize(xowin->bitmap, e.xconfigure.width, e.xconfigure.height);
         free(bitmap->buf);
         bitmap->buf = Memory_Reserve(
*/
//   		XoWin_RefreshBitMap(xowin);
         break;

      }
	}

   /* To compress several sequential window resizes into one change of bitmap */
   if (resize) {
   #ifdef XOWIN_XSHM
      if (xowin->shared_image != NULL) {
         XoWin_RefreshBitMap(xowin);
      } else
   #endif
      BitMap_Resize(xowin->bitmap, sizex, sizey, FALSE);
      xowin->mainwinsizex = sizex;
      xowin->mainwinsizey = sizey;
      ret |= 2;
   }
   if (interaction) ret |= 1;
   return ret;
}

/* ----------------------------------------------------------------------- */

int XoWin_WaitUserInput(XoWin *xowin, int *mousex, int *mousey, char *key, double timeout) {
   fd_set fds;
   struct timeval tv;
   int fd = ConnectionNumber(xowin->dpy);
   
   FD_ZERO(&fds);
   FD_SET(fd, &fds);
   tv.tv_sec = (int)timeout;
   tv.tv_usec = (int)1000000 * (timeout - (double)tv.tv_sec);
   if (select(fd+1, &fds, NULL, NULL, &tv)) return XoWin_HandleEvent(xowin, mousex, mousey, key);
   return 0;
}

/* ----------------------------------------------------------------------- */

int XoWin_Socket(XoWin *xowin) { return ConnectionNumber(xowin->dpy); }

/* ----------------------------------------------------------------------- */

int XoWin_IsUserInput(XoWin *xowin, Bool block, int *mousex, int *mousey, char *key) {
   if (block) return XoWin_WaitUserInput(xowin, mousex, mousey, key, 1e7);
   else return XoWin_HandleEvent(xowin, mousex, mousey, key);
}

/* ----------------------------------------------------------------------- */

int XoWin_NofPending(XoWin *xowin) { return XPending(xowin->dpy); }

/* ----------------------------------------------------------------------- */

Bool XoWin_IsUserEvent(XoWin *xowin) {
   return (XoWin_IsUserInput(xowin, FALSE, NULL, NULL, NULL) & 0x1) > 0;
}
/* ----------------------------------------------------------------------- */

unsigned char XoWin_KeyCodeToASCII(XoWin *xowin, char keycode) {
   XKeyEvent ev;
   String str[16];
   if(keycode<0) return 0;
   Memory_Set(&ev, sizeof(ev), 0);
   ev.type = KeyPress;
   ev.display = xowin->dpy;
   ev.window = xowin->mainwin;
   ev.root = DefaultRootWindow(xowin->dpy);
   ev.keycode = keycode;
   if (XoWin_IsKeyPressed(xowin, XOWIN_SHIFTL) 
   || XoWin_IsKeyPressed(xowin, XOWIN_SHIFTR)) 
      ev.state |= ShiftMask;
   XLookupString(&ev, str, 16, NULL, NULL);
   return str[0];
}

/* ----------------------------------------------------------------------- */

int XoWin_PointerPos(XoWin *xowin) {
   Window w, root;
   int root_x, root_y;
   int x, y;
   unsigned int mask;
   XQueryPointer(xowin->dpy, xowin->mainwin, &root, &w, &root_x, &root_y, &x, &y, &mask);
   x&=32767;
   y&=32767;
   return (x<<16) | y;
}

/* ----------------------------------------------------------------------- */
void XoWin_SetSize(XoWin *xowin, int sizex, int sizey) {
   XResizeWindow(xowin->dpy, xowin->mainwin, sizex, sizey);
   BitMap_Resize(xowin->bitmap, sizex, sizey, FALSE);
   xowin->mainwinsizex = sizex;
   xowin->mainwinsizey = sizey;
}

/* ----------------------------------------------------------------------- */

/*
XC_X_cursor_0
XC_watch
XC_xterm
*/

void XoWin_SetCursor(XoWin *xowin, int type) {
   Cursor cursor = XCreateFontCursor(xowin->dpy, type);
   XDefineCursor(xowin->dpy, xowin->mainwin, cursor);
//   XFreeCursor(xowin->dpy, cursor);
}

void XoWin_HideCursor(XoWin *xowin) {
        Cursor no_ptr;
        Pixmap bm_no;
        XColor black, dummy;
        Colormap colormap;
        static char no_data[] = { 0,0,0,0,0,0,0,0 };

        colormap = DefaultColormap(xowin->dpy, DefaultScreen(xowin->dpy));
        XAllocNamedColor(xowin->dpy, colormap, "black", &black, &dummy);
        bm_no = XCreateBitmapFromData(xowin->dpy, xowin->mainwin, no_data, 8, 8);
        no_ptr = XCreatePixmapCursor(xowin->dpy, bm_no, bm_no, &black, &black, 0, 0);

        XDefineCursor(xowin->dpy, xowin->mainwin, no_ptr);
        XFreeCursor(xowin->dpy, no_ptr);
}
/* ----------------------------------------------------------------------- */

void XoWin_ResetCursor(XoWin *xowin) {
//   XoWin_SetCursor(xowin, 68);
   XoWin_SetCursor(xowin, 5);
}
