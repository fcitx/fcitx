#include "MainWindow.h"

#include <stdio.h>

#include "IC.h"
#include "ui.h"

//下面的顺序不能改变
#include "logo.xpm"
// #include "eng.xpm"
#include "wb-0.xpm"
#include "wb-1.xpm"
#include "wb-2.xpm"
#include "eb-0.xpm"
#include "eb-1.xpm"
#include "eb-2.xpm"
#include "py-0.xpm"
#include "py-1.xpm"
#include "py-2.xpm"
#include "fullcorner.xpm"
#include "halfcorner.xpm"
#include "chnPunc.xpm"
#include "engPunc.xpm"
#include "gbk-0.xpm"
#include "gbk-1.xpm"
#include "sp-0.xpm"
#include "sp-1.xpm"
#include "lx-0.xpm"
#include "lx-1.xpm"

Window          mainWindow;

int             MAINWND_WIDTH = _MAINWND_WIDTH;

int             iMainWindowX = MAINWND_STARTX;
int             iMainWindowY = MAINWND_STARTY;
WINDOW_COLOR    mainWindowColor;
MESSAGE_COLOR   mainWindowLineColor;	//线条色

XImage         *pLogo = NULL;
XImage         *pIME[3][3] = { {NULL, NULL, NULL}, {NULL, NULL, NULL}, {NULL, NULL, NULL} };
char           *IMELogo[3][3] = {
    {(char *) wb_0_xpm, (char *) wb_2_xpm, (char *) wb_1_xpm},
    {(char *) py_0_xpm, (char *) py_2_xpm, (char *) py_1_xpm},
    {(char *) eb_0_xpm, (char *) eb_2_xpm, (char *) eb_1_xpm}
};

XImage         *pCorner[2] = { NULL, NULL };
char          **CornerLogo[2] = { halfcorner_xpm, fullcorner_xpm };
XImage         *pPunc[2] = { NULL, NULL };
char          **PuncLogo[2] = { engPunc_xpm, chnPunc_xpm };
XImage         *pGBK[2] = { NULL, NULL };
char          **GBKLogo[2] = { gbk_0_xpm, gbk_1_xpm };
XImage         *pSP[2] = { NULL, NULL };
char          **SPLogo[2] = { sp_0_xpm, sp_1_xpm };
XImage         *pLX[2] = { NULL, NULL };
char          **LXLogo[2] = { lx_0_xpm, lx_1_xpm };

HIDE_MAINWINDOW hideMainWindow = HM_SHOW;

extern Display *dpy;
extern GC       dimGC;
extern int      i3DEffect;
extern IC      *CurrentIC;
extern Bool     bCorner;
extern Bool     bChnPunc;
extern IME      imeIndex;
extern Bool     bUseGBK;
extern Bool     bSP;
extern Bool     bUseLegend;

Bool CreateMainWindow (void)
{
    XSetWindowAttributes attrib;
    unsigned long   attribmask;
    int             iBackPixel;

    attrib.override_redirect = True;
    attribmask = CWOverrideRedirect;

    sprintf (strMainWindowXPMBackColor, ". 	c #%2x%2x%2x", mainWindowColor.backColor.red >> 8, mainWindowColor.backColor.green >> 8, mainWindowColor.backColor.blue >> 8);

    if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &(mainWindowColor.backColor)))
	iBackPixel = mainWindowColor.backColor.pixel;
    else
	iBackPixel = WhitePixel (dpy, DefaultScreen (dpy));

    mainWindow = XCreateSimpleWindow (dpy, DefaultRootWindow (dpy), iMainWindowX, iMainWindowY, MAINWND_WIDTH, MAINWND_HEIGHT, 0, WhitePixel (dpy, DefaultScreen (dpy)), iBackPixel);
    if (mainWindow == (Window) NULL)
	return False;

    XChangeWindowAttributes (dpy, mainWindow, attribmask, &attrib);
    XSelectInput (dpy, mainWindow, ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | EnterWindowMask | PointerMotionMask | LeaveWindowMask | VisibilityChangeMask);

    InitMainWindowColor ();

    return True;
}

void DisplayMainWindow (void)
{
    unsigned char   iIndex;

    iIndex = IS_CLOSED;
    if (hideMainWindow == HM_SHOW || (hideMainWindow == HM_AUTO && (CurrentIC && CurrentIC->imeState != IS_CLOSED))) {
	XMapRaised (dpy, mainWindow);

	XDrawRectangle (dpy, mainWindow, mainWindowLineColor.gc, 0, 0, MAINWND_WIDTH - 1, MAINWND_HEIGHT - 1);

	if (!pLogo) {
	    pLogo = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
	    FillImageByXPMData (pLogo, logo_xpm);
	}
	XPutImage (dpy, mainWindow, mainWindowColor.backGC, pLogo, 0, 0, 2, 2, 15, 16);

	if (!pPunc[bChnPunc]) {
	    pPunc[bChnPunc] = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
	    FillImageByXPMData (pPunc[bChnPunc], PuncLogo[bChnPunc]);
	}
	XPutImage (dpy, mainWindow, mainWindowColor.backGC, pPunc[bChnPunc], 0, 0, 20, 2, 15, 16);

	if (!pCorner[bCorner]) {
	    pCorner[bCorner] = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
	    FillImageByXPMData (pCorner[bCorner], CornerLogo[bCorner]);
	}
	XPutImage (dpy, mainWindow, mainWindowColor.backGC, pCorner[bCorner], 0, 0, 38, 2, 15, 16);

	if (!pGBK[bUseGBK]) {
	    pGBK[bUseGBK] = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
	    FillImageByXPMData (pGBK[bUseGBK], GBKLogo[bUseGBK]);
	}
	XPutImage (dpy, mainWindow, mainWindowColor.backGC, pGBK[bUseGBK], 0, 0, 56, 2, 15, 16);

	if (!pLX[bUseLegend]) {
	    pLX[bUseLegend] = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
	    FillImageByXPMData (pLX[bUseLegend], LXLogo[bUseLegend]);
	}
	XPutImage (dpy, mainWindow, mainWindowColor.backGC, pLX[bUseLegend], 0, 0, 74, 2, 15, 16);

	if (CurrentIC)
	    iIndex = CurrentIC->imeState;
	if (!pIME[imeIndex][iIndex]) {
	    pIME[imeIndex][iIndex] = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
	    FillImageByXPMData (pIME[imeIndex][iIndex], (char **) IMELogo[imeIndex][iIndex]);
	}
	XPutImage (dpy, mainWindow, mainWindowColor.backGC, pIME[imeIndex][iIndex], 0, 0, 92, 2, 15, 16);

	if (imeIndex == IME_PINYIN) {
	    if (!pSP[bSP]) {
		pSP[bSP] = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
		FillImageByXPMData (pSP[bSP], SPLogo[bSP]);
	    }
	    XPutImage (dpy, mainWindow, mainWindowColor.backGC, pSP[bSP], 0, 0, 110, 2, 15, 16);
	}

	if (i3DEffect) {
	    Draw3DEffect (mainWindow, 1, 1, MAINWND_WIDTH - 2, MAINWND_HEIGHT - 2, _3D_UPPER);

	    Draw3DEffect (mainWindow, 1, 1, 18, 18, _3D_UPPER);
	    Draw3DEffect (mainWindow, 19, 1, 18, 18, _3D_UPPER);
	    Draw3DEffect (mainWindow, 37, 1, 18, 18, _3D_UPPER);
	    Draw3DEffect (mainWindow, 55, 1, 18, 18, _3D_UPPER);
	    Draw3DEffect (mainWindow, 73, 1, 18, 18, _3D_UPPER);
	    Draw3DEffect (mainWindow, 91, 1, 18, 18, _3D_UPPER);
	    if (imeIndex == IME_PINYIN)
		Draw3DEffect (mainWindow, 109, 1, 18, 18, _3D_UPPER);
	}
	else {
	    XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, 19, 4, 19, MAINWND_HEIGHT - 4);
	    XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, 37, 4, 37, MAINWND_HEIGHT - 4);
	    XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, 55, 4, 55, MAINWND_HEIGHT - 4);
	    XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, 73, 4, 73, MAINWND_HEIGHT - 4);
	    XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, 91, 4, 91, MAINWND_HEIGHT - 4);
	    if (imeIndex == IME_PINYIN)
		XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, 109, 4, 109, MAINWND_HEIGHT - 4);
	}
    }
    else
	XUnmapWindow (dpy, mainWindow);
}

void InitMainWindowColor (void)
{
    XGCValues       values;
    int             iPixel;

    mainWindowLineColor.gc = XCreateGC (dpy, mainWindow, 0, &values);
    if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &(mainWindowLineColor.color)))
	iPixel = mainWindowLineColor.color.pixel;
    else
	iPixel = WhitePixel (dpy, DefaultScreen (dpy));
    XSetForeground (dpy, mainWindowLineColor.gc, iPixel);
}
