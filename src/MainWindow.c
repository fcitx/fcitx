#include "MainWindow.h"

#include <stdio.h>

#include "IC.h"
#include "ui.h"
#include "ime.h"
#include "tools.h"

//下面的顺序不能改变
#include "logo.xpm"
#include "fullcorner.xpm"
#include "halfcorner.xpm"
#include "chnPunc.xpm"
#include "engPunc.xpm"
#include "gbk-0.xpm"
#include "gbk-1.xpm"
#include "lx-0.xpm"
#include "lx-1.xpm"
#include "lock-0.xpm"
#include "lock-1.xpm"
#include "ui.h"

Window          mainWindow;
int             MAINWND_WIDTH = _MAINWND_WIDTH;

int             iMainWindowX = MAINWND_STARTX;
int             iMainWindowY = MAINWND_STARTY;

WINDOW_COLOR    mainWindowColor = { NULL, NULL, {0, 220 << 8, 220 << 8, 220 << 8} };
MESSAGE_COLOR   mainWindowLineColor = { NULL, {0, 100 << 8, 180 << 8, 255 << 8} };	//线条色
MESSAGE_COLOR   IMNameColor[3] = {	//输入法名称的颜色
    {NULL, {0, 170 << 8, 170 << 8, 170 << 8}},
    {NULL, {0, 150 << 8, 200 << 8, 150 << 8}},
    {NULL, {0, 0, 0, 255 << 8}}
};

Bool            _3DEffectMainWindow = False;
XImage         *pLogo = NULL;

XImage         *pCorner[2] = { NULL, NULL };
char          **CornerLogo[2] = { halfcorner_xpm, fullcorner_xpm };
XImage         *pPunc[2] = { NULL, NULL };
char          **PuncLogo[2] = { engPunc_xpm, chnPunc_xpm };
XImage         *pGBK[2] = { NULL, NULL };
char          **GBKLogo[2] = { gbk_0_xpm, gbk_1_xpm };
XImage         *pLX[2] = { NULL, NULL };
char          **LXLogo[2] = { lx_0_xpm, lx_1_xpm };
XImage         *pLock[2] = { NULL, NULL };
char          **LockLogo[2] = { lock_0_xpm, lock_1_xpm };

HIDE_MAINWINDOW hideMainWindow = HM_SHOW;

Bool            bLocked = True;
Bool            bCompactMainWindow = False;

extern Display *dpy;
extern GC       dimGC;
extern int      i3DEffect;
extern IC      *CurrentIC;
extern Bool     bCorner;
extern Bool     bChnPunc;
extern INT8     iIMIndex;
extern Bool     bUseGBK;
extern Bool     bSP;
extern Bool     bUseLegend;
extern IM      *im;
extern CARD16   connect_id;

#ifdef _USE_XFT
extern XftFont *xftMainWindowFont;
extern XftFont *xftMainWindowFontEn;
#else
extern XFontSet fontSetMainWindow;
#endif

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
    INT8            iIndex = 0;
    INT16           iPos;

#ifdef _USE_XFT
    char            strTemp[MAX_IM_NAME + 1];
    char           *p1, *p2;
    Bool            bEn;
#endif
    iIndex = IS_CLOSED;
    if (hideMainWindow == HM_SHOW || (hideMainWindow == HM_AUTO && (ConnectIDGetState (connect_id) != IS_CLOSED))) {
	XMapRaised (dpy, mainWindow);

	XDrawRectangle (dpy, mainWindow, mainWindowLineColor.gc, 0, 0, MAINWND_WIDTH - 1, MAINWND_HEIGHT - 1);

	if (!pLogo) {
	    pLogo = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
	    FillImageByXPMData (pLogo, logo_xpm);
	}
	XPutImage (dpy, mainWindow, mainWindowColor.backGC, pLogo, 0, 0, 2, 2, 15, 16);

	iPos = 20;
	if (!bCompactMainWindow) {
	    if (!pPunc[bChnPunc]) {
		pPunc[bChnPunc] = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
		FillImageByXPMData (pPunc[bChnPunc], PuncLogo[bChnPunc]);
	    }
	    XPutImage (dpy, mainWindow, mainWindowColor.backGC, pPunc[bChnPunc], 0, 0, iPos, 2, 15, 16);
	    iPos += 18;

	    if (!pCorner[bCorner]) {
		pCorner[bCorner] = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
		FillImageByXPMData (pCorner[bCorner], CornerLogo[bCorner]);
	    }
	    XPutImage (dpy, mainWindow, mainWindowColor.backGC, pCorner[bCorner], 0, 0, iPos, 2, 15, 16);
	    iPos += 18;

	    if (!pGBK[bUseGBK]) {
		pGBK[bUseGBK] = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
		FillImageByXPMData (pGBK[bUseGBK], GBKLogo[bUseGBK]);
	    }
	    XPutImage (dpy, mainWindow, mainWindowColor.backGC, pGBK[bUseGBK], 0, 0, iPos, 2, 15, 16);
	    iPos += 18;

	    if (!pLX[bUseLegend]) {
		pLX[bUseLegend] = XGetImage (dpy, mainWindow, 0, 0, 15, 16, AllPlanes, XYPixmap);
		FillImageByXPMData (pLX[bUseLegend], LXLogo[bUseLegend]);
	    }
	    XPutImage (dpy, mainWindow, mainWindowColor.backGC, pLX[bUseLegend], 0, 0, iPos, 2, 15, 16);
	    iPos += 18;
	}

	if (!pLock[bLocked]) {
	    pLock[bLocked] = XGetImage (dpy, mainWindow, 0, 0, 8, 16, AllPlanes, XYPixmap);
	    FillImageByXPMData (pLock[bLocked], LockLogo[bLocked]);
	}
	XPutImage (dpy, mainWindow, mainWindowColor.backGC, pLock[bLocked], 0, 0, iPos, 2, 15, 16);
	iPos += 13;

	iIndex = ConnectIDGetState (connect_id);
	XClearArea (dpy, mainWindow, iPos, 2, MAINWND_WIDTH - iPos - 2, MAINWND_HEIGHT - 4, False);
#ifdef _USE_XFT
	p1 = im[iIMIndex].strName;
	while (*p1) {
	    if (isprint (*p1))
		bEn = True;
	    else
		bEn = False;
	    p2 = strTemp;
	    while (*p1) {
		*p2++ = *p1++;
		if (isprint (*p1)) {
		    if (!bEn)
			break;
		}
		else {
		    if (bEn)
			break;
		}
	    }
	    *p2 = '\0';

	    OutputString (mainWindow, (bEn) ? xftMainWindowFontEn : xftMainWindowFont, strTemp, iPos, FontHeight (xftMainWindowFont) + (MAINWND_HEIGHT - FontHeight (xftMainWindowFont)) / 2 - 1, IMNameColor[iIndex].color);
	    iPos += StringWidth (strTemp, (bEn) ? xftMainWindowFontEn : xftMainWindowFont);
	}
#else
	OutputString (mainWindow, fontSetMainWindow, im[iIMIndex].strName, iPos, FontHeight (fontSetMainWindow) + (MAINWND_HEIGHT - FontHeight (fontSetMainWindow)) / 2 - 1, IMNameColor[iIndex].gc);
#endif

	if (_3DEffectMainWindow) {
	    Draw3DEffect (mainWindow, 1, 1, MAINWND_WIDTH - 2, MAINWND_HEIGHT - 2, _3D_UPPER);
	    if (!bCompactMainWindow) {
		Draw3DEffect (mainWindow, 1, 1, 18, 18, _3D_UPPER);
		Draw3DEffect (mainWindow, 19, 1, 18, 18, _3D_UPPER);
		Draw3DEffect (mainWindow, 37, 1, 18, 18, _3D_UPPER);
		Draw3DEffect (mainWindow, 55, 1, 18, 18, _3D_UPPER);
		Draw3DEffect (mainWindow, 73, 1, 18, 18, _3D_UPPER);
		Draw3DEffect (mainWindow, 91, 1, 11, 18, _3D_UPPER);
	    }
	}
	else {
	    iPos = 19;
	    XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, 18, 4, 18, MAINWND_HEIGHT - 4);
	    if (!bCompactMainWindow) {
		XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, 36, 4, 36, MAINWND_HEIGHT - 4);
		XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, 54, 4, 54, MAINWND_HEIGHT - 4);
		XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, 72, 4, 72, MAINWND_HEIGHT - 4);
		XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, 89, 4, 89, MAINWND_HEIGHT - 4);
		iPos = 90;
	    }
	    iPos += 11;
	    XDrawLine (dpy, mainWindow, mainWindowLineColor.gc, iPos, 4, iPos, MAINWND_HEIGHT - 4);
	}
    }
    else
	XUnmapWindow (dpy, mainWindow);
}

void InitMainWindowColor (void)
{
    XGCValues       values;
    int             iPixel;
    int             i;

    mainWindowLineColor.gc = XCreateGC (dpy, mainWindow, 0, &values);
    if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &(mainWindowLineColor.color)))
	iPixel = mainWindowLineColor.color.pixel;
    else
	iPixel = WhitePixel (dpy, DefaultScreen (dpy));
    XSetForeground (dpy, mainWindowLineColor.gc, iPixel);

    for (i = 0; i < 3; i++) {
	IMNameColor[i].gc = XCreateGC (dpy, mainWindow, 0, &values);
	if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &(IMNameColor[i].color)))
	    iPixel = IMNameColor[i].color.pixel;
	else
	    iPixel = WhitePixel (dpy, DefaultScreen (dpy));
	XSetForeground (dpy, IMNameColor[i].gc, iPixel);
    }
}

void ChangeLock (void)
{
    bLocked = !bLocked;
    DisplayMainWindow ();

    SaveProfile ();
}
