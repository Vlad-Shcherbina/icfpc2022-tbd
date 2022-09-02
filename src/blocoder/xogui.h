#ifndef _xogui_h
#define _xogui_h

#include "bitmap.h"
#include "cmod.h"
#include "string.h"

typedef struct XoGUI XoGUI;

/*       Constructor */
XoGUI   *XoGUI_Create(int initial_sizex, int initial_sizey);
XoGUI   *XoGUI_CreatePos(int x, int y, int width, int height);
XoGUI   *XoGUI_CreateFullScreen(void);
/*       Destructor */
void     XoGUI_Destroy(XoGUI *xogui);

/*       Current size of window */
int      XoGUI_Width(XoGUI *xo);
int      XoGUI_Height(XoGUI *xo);
#define  XoGUI_SizeX XoGUI_Width 
#define  XoGUI_SizeY XoGUI_Height
/*       Retrieve array of RGB values, see bitmap.h */
BitMap  *XoGUI_Canvas(XoGUI *xo);

/*       Update view; wait for user input at most timeout secs, 
         return s
         tatus has bit 0 on input and bit 1 on resize */
int      XoGUI_Update(XoGUI *xo, double timeout_secs);

/*       Clear screen to default color */
void     XoGUI_Clear(XoGUI *xogui); /* Black window */

/*       Current position of mouse pointer */
void     XoGUI_MousePos(XoGUI *xo, int *x, int *y);

/*       ASCII code for key pressed; ESC=27, DEL=127, RET=13, TAB=9, ...  */
char     XoGUI_Key(XoGUI *xo);
int      XoGUI_IsKeyPressed(XoGUI *xo, int scancode);

/*       Which control was toggled (button clicked etc) */
int      XoGUI_ClickedId(XoGUI *xo);


/*       Restricts output to this view (in coordinates of current view) */
void     XoGUI_SetView(XoGUI *xo, int x1, int y1, int x2, int y2);
/*       Restores the previous view */
void     XoGUI_RestoreView(XoGUI *xo);
/*       Restricts graphics output to specified rectangle */
void     XoGUI_SetClip(XoGUI *xo, int x1, int y1, int x2, int y2);
/*       Resets view and clip to the whole window */
void     XoGUI_ResetView(XoGUI *xo);

/*       Panels -- just looks, no functionality */
void     XoGUI_Panel(XoGUI *xo, int id, int x1, int y1, int x2, int y2);
/*       Inverse (sunken) panel -- no functionality */
void     XoGUI_SunkenPanel(XoGUI *xo, int id, int x1, int y1, int x2, int y2);
/*       Inverse panel with clip on and coordinate system centered in middle of panel */
void     XoGUI_PlotArea(XoGUI *xo, int id, int x1, int y1, int x2, int y2);

/*       Add a button with text */
void     XoGUI_Button(XoGUI *xo, int id, int x, int y, const String *text);
void     XoGUI_Button2(XoGUI *xo, int id, int x1, int y1, int x2, int y2, const String *text);
/*       Button in pushed in state */
void     XoGUI_Button2Pushed(XoGUI *xo, int id, int x1, int y1, int x2, int y2, const String *text);

void     XoGUI_Printf(XoGUI *xo, int id, int x, int y, const String *format, ... );
void     XoGUI_PrintfCentered(XoGUI *xo, int id, int x, int y, const String *format, ...);

/*       Add a text input field */
void     XoGUI_TextInput(XoGUI *xo, int id, int x1, int y1, int width, String *initial);
void     XoGUI_TextInput2(XoGUI *xo, int id, int x1, int y1, int x2, int y2, String *initial);

/*       True/false checkbox with a text description beside (use "" for no desc) */
void     XoGUI_CheckBox(XoGUI *xo, int id, int x1, int y1, const String *message, int default_value);

/*       Get/set integer value from control */
int      XoGUI_GetValue(XoGUI *xo, int id); 
void     XoGUI_SetValue(XoGUI *xo, int id, int val); 
/*       Get/set data pointers. Memory is owned by XoGUI! */
void    *XoGUI_GetData(XoGUI *xo, int id);
void     XoGUI_SetData(XoGUI *xo, int id, const void *data);
/*       Alternative names for the data set/get methods */
#define  XoGUI_Value     XoGUI_GetValue
#define  XoGUI_Data      XoGUI_GetData
#define  XoGUI_String    (String *)XoGUI_Data
#define  XoGUI_GetString XoGUI_String
#define  XoGUI_Text      XoGUI_String
#define  XoGUI_GetText   XoGUI_String
#define  XoGUI_SetString XoGUI_SetData
#define  XoGUI_SetText   XoGUI_SetData

/*       Dialog functions, the second is a macro-hack to avoid passing the number of buttons */
int      XoGUI_DialogN(XoGUI *xogui, const String *message, int nof_buttons, ... );
#define  XoGUI_Dialog(g,m,...) XoGUI_DialogN((g),(m),sizeof(((String *)[]){NULL,__VA_ARGS__})/sizeof(String*)-1, ##__VA_ARGS__)

/*       Customizing looks */
/*       Sets the window title */
void     XoGUI_SetWindowTitle(XoGUI *xogui, const String *title);

/*       Overall looks of buttons etc: 0=default, 1=rounded */
void     XoGUI_SetStyle(XoGUI *xo, int style);
int      XoGUI_Style(XoGUI *xo);
void     XoGUI_SetTextColor(XoGUI *xo, int r, int g, int b);
void     XoGUI_SetSelectionColor(XoGUI *xo, int r, int g, int b);

/*       Set the surface color, and then adjust light and dark suitably */
void     XoGUI_SetColorScheme(XoGUI *xo, int r, int g, int b);
/*       Set the light and dark colors */
void     XoGUI_SetShadowColor(XoGUI *xo, int r, int g, int b);
void     XoGUI_SetLightColor(XoGUI *xo, int r, int g, int b);
void     XoGUI_SetSurfaceColor(XoGUI *xo, int r, int g, int b);

/*       Graphics primitives */
void     XoGUI_PutPixel(XoGUI *gui, int x, int y, PixelColor p);
void     XoGUI_Line(XoGUI *co, int x1, int y1, int x2, int y2, PixelColor p);
void     XoGUI_Circle(XoGUI *co, int x1, int y1, int r, PixelColor p);
void     XoGUI_FillCircle(XoGUI *co, int x1, int y1, int r, PixelColor p);
void     XoGUI_FillRectangle(XoGUI *gui, int x1, int y1, int x2, int y2, PixelColor p);
BitMap  *XoGUI_LoadPNG(XoGUI *gui, const char *filename);
BitMap  *XoGUI_BorrowCanvas(XoGUI *gui); /* Gets the master canvas */
void     XoGUI_PutImage(XoGUI *gui, int id, BitMap *bm, int x, int y);

/* Socket for waiting for events */
int      XoGUI_Socket(XoGUI *gui);

#ifdef CMOD_NOMAKEFILE
#include "xogui.c"
#endif
#endif

