#ifndef _UI_H
#define _UI_H

#include <X11/Xlib.h>

#define LIGHT_COLOR	0xffff
#define BACK_COLOR	0x0000
#define DIM_COLOR	0x6666

typedef enum {
    _3D_FLAT = 0,
    _3D_UPPER = 1,
    _3D_LOWER = 2
} _3D_EFFECT;

typedef struct {
    GC              foreGC;
    GC              backGC;
    XColor          foreColor;
    XColor          backColor;
} WINDOW_COLOR;

typedef struct {
    GC              gc;
    XColor          color;
} MESSAGE_COLOR;

typedef struct {
    char           *pcStr;
    int             iPixel;
} StrPixel;

Bool            InitX (void);
void            Draw3DEffect (Window window, int x, int y, int width, int height, _3D_EFFECT effect);
void            InitGC (Window window);
void            CreateFont (void);
void            MyXEventHandler (Window im_window, XEvent * event);
Bool            IsInBox (int x0, int y0, int x1, int y1, int x2, int y2);

#ifdef _USE_XFT
void            OutputString (Window window, char *str, int x, int y, XColor);
#else
void            OutputString (Window window, char *str, int x, int y, GC gc);
#endif

int		StringWidth(char *str);

int             FillImageByXPMData (XImage * pImage, char **apcData);
XPoint          MouseClick (int *x, int *y, int iWnd);
void            WaitButtonRelease (XPoint * point);

#endif
