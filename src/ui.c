#include "ui.h"

#include <X11/Xlocale.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <iconv.h>

#ifdef _USE_XFT
#include <ft2build.h>
#include <X11/Xft/Xft.h>
#endif

#include "MyErrorsHandlers.h"
#include "MainWindow.h"
#include "InputWindow.h"
#include "IC.h"
#include "tools.h"
#include "sp.h"

Display        *dpy;
int             iScreen;

#ifdef _USE_XFT
XftFont        *xftFont;
XftDraw        *xftDraw;
Bool            bUseAA = True;
#else
XFontSet        fontSet;
#endif

iconv_t         convUTF8;	//我只会用XFT输出UTF8中文字串，另外该变量还用于UTF8支持

int             iFontSize = 16;

GC              dimGC;
GC              lightGC;

Bool            bIsUtf8 = False;
int             i3DEffect = _3D_LOWER;

char            strFontName[100] = "*";

extern Window   mainWindow;
extern int      iMainWindowX;
extern int      iMainWindowY;
extern Window   inputWindow;
extern int      iInputWindowX;
extern int      iInputWindowY;
extern WINDOW_COLOR mainWindowColor;
extern WINDOW_COLOR inputWindowColor;
extern Bool     bIsResizingInputWindow;
extern HIDE_MAINWINDOW hideMainWindow;
extern IC      *CurrentIC;
extern char    *strDefaultLocale;

Bool InitX (void)
{
    if ((dpy = XOpenDisplay ((char *) NULL)) == NULL) {
	fprintf (stderr, "Error: FCITX can only run under X\n");
	return False;
    }

    SetMyXErrorHandler ();
    iScreen = DefaultScreen (dpy);

    convUTF8 = iconv_open ("UTF-8", "GBK");

    return True;
}

void InitGC (Window window)
{
    XGCValues       values;
    XColor          color;
    int             iPixel;

    lightGC = XCreateGC (dpy, window, 0, &values);
    color.red = LIGHT_COLOR;
    color.green = LIGHT_COLOR;
    color.blue = LIGHT_COLOR;
    if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &color))
	iPixel = color.pixel;
    else
	iPixel = WhitePixel (dpy, DefaultScreen (dpy));
    XSetForeground (dpy, lightGC, iPixel);

    dimGC = XCreateGC (dpy, window, 0, &values);
    color.red = DIM_COLOR;
    color.green = DIM_COLOR;
    color.blue = DIM_COLOR;
    if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &color))
	iPixel = color.pixel;
    else
	iPixel = BlackPixel (dpy, DefaultScreen (dpy));
    XSetForeground (dpy, dimGC, iPixel);

    mainWindowColor.backGC = XCreateGC (dpy, window, 0, &values);
    if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &(mainWindowColor.backColor)))
	iPixel = mainWindowColor.backColor.pixel;
    else
	iPixel = WhitePixel (dpy, DefaultScreen (dpy));
    XSetForeground (dpy, mainWindowColor.backGC, iPixel);

    inputWindowColor.foreGC = XCreateGC (dpy, window, 0, &values);
    if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &(inputWindowColor.foreColor)))
	iPixel = inputWindowColor.foreColor.pixel;
    else
	iPixel = BlackPixel (dpy, DefaultScreen (dpy));
    XSetForeground (dpy, inputWindowColor.foreGC, iPixel);

    inputWindowColor.backGC = XCreateGC (dpy, window, 0, &values);
    if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &(inputWindowColor.backColor)))
	iPixel = inputWindowColor.backColor.pixel;
    else
	iPixel = BlackPixel (dpy, DefaultScreen (dpy));
    XSetForeground (dpy, inputWindowColor.backGC, iPixel);
}

#ifdef _USE_XFT
void CreateFont (void)
{
    XftPattern     *pattern;
    XftResult       result;

    pattern = XftPatternCreate ();
    XftPatternAddString (pattern, "family", strFontName);
    XftPatternAddInteger (pattern, "pixelsize", iFontSize);
    if (bUseAA)
	XftPatternAddBool (pattern, "antialias", 1);
    else
	XftPatternAddBool (pattern, "antialias", 0);
    xftFont = XftFontOpenPattern (dpy, XftFontMatch (dpy, iScreen, pattern, &result));
    xftDraw = XftDrawCreate (dpy, inputWindow, DefaultVisual (dpy, DefaultScreen (dpy)), DefaultColormap (dpy, DefaultScreen (dpy)));

    XftPatternDestroy (pattern);
}
#else
void CreateFont (void)
{
    char          **missing_charsets;
    int             num_missing_charsets = 0;
    char           *default_string;
    char            strFont[256];

    setlocale (LC_CTYPE, "");
    sprintf (strFont, "-*-%s-medium-r-normal--%d-*-*-*-*-*-*-*,-*-", strFontName, iFontSize);
    fontSet = XCreateFontSet (dpy, strFont, &missing_charsets, &num_missing_charsets, &default_string);

    if (num_missing_charsets > 0) {
	iFontSize = 16;
	setlocale (LC_CTYPE, "zh_CN.GB2312");
	fontSet = XCreateFontSet (dpy, "-*-fixed-medium-r-normal--16-*-*-*-*-*-*-*,-*-*-medium-r-normal--16-*-*-*-*-*-*-*", &missing_charsets, &num_missing_charsets, &default_string);
	if (num_missing_charsets > 0)
	    fprintf (stderr, "\7Error: Cannot Create Chinese Fonts!\n");
    }
}
#endif

/*
 * 让指定的区域显示三维效果
 * effect:
 *	_3D_UPPER:显示为凸出效果
 *	_3D_LOWER:显示为凹下效果
 */
void Draw3DEffect (Window window, int x, int y, int width, int height, _3D_EFFECT effect)
{
    if (effect == _3D_UPPER) {
	XDrawLine (dpy, window, lightGC, x, y, x + width - 1, y);
	XDrawLine (dpy, window, lightGC, x, y, x, y + height - 1);
	XDrawLine (dpy, window, dimGC, x + width - 1, y, x + width - 1, y + height - 1);
	XDrawLine (dpy, window, dimGC, x, y + height - 1, x + width - 1, y + height - 1);
    }
    else if (effect == _3D_LOWER) {
	XDrawLine (dpy, window, lightGC, x, y, x + width - 1, y);
	XDrawLine (dpy, window, lightGC, x, y, x, y + height - 1);
	XDrawLine (dpy, window, dimGC, x + width - 1, y, x + width - 1, y + height - 1);
	XDrawLine (dpy, window, dimGC, x, y + height - 1, x + width - 1, y + height - 1);

	XDrawLine (dpy, window, dimGC, x + 1, y + 1, x + width - 2, y + 1);
	XDrawLine (dpy, window, dimGC, x + 1, y + 1, x + 1, y + height - 2);
	XDrawLine (dpy, window, lightGC, x + 1, y + height - 2, x + width - 2, y + height - 2);
	XDrawLine (dpy, window, lightGC, x + width - 2, y + 1, x + width - 2, y + height - 2);
    }
    else {			//   _3D_FLAT
	XDrawRectangle (dpy, window, dimGC, x, y, width - 1, height - 1);
	return;
    }
}

/*
 * 有关界面的消息都在这里处理
 */
void MyXEventHandler (Window im_window, XEvent * event)
{
    switch (event->type) {
    case Expose:
	if (event->xexpose.count > 0)
	    break;
	if (event->xexpose.window == mainWindow) {
	    DisplayMainWindow ();
	    break;
	}
	if (event->xexpose.window == inputWindow) {
	    if (!bIsResizingInputWindow) {
		DisplayInputWindow ();
		bIsResizingInputWindow = False;
	    }
	}
	break;
    case DestroyNotify:
	break;
    case ButtonPress:
	switch (event->xbutton.button) {
	case Button1:
	    if (event->xbutton.window == inputWindow) {
		iInputWindowX = event->xbutton.x;
		iInputWindowY = event->xbutton.y;
		MouseClick (&iInputWindowX, &iInputWindowY, 1);
		SaveProfile ();
	    }
	    else if (event->xbutton.window == mainWindow) {
		if (IsInBox (event->xbutton.x, event->xbutton.y, 1, 1, 16, 17)) {
		    iMainWindowX = event->xbutton.x;
		    iMainWindowY = event->xbutton.y;
		    MouseClick (&iMainWindowX, &iMainWindowY, 0);
		}
	    }
	    break;
	}
	break;
    case ButtonRelease:
	switch (event->xbutton.button) {
	case Button1:
	    if (event->xbutton.window == mainWindow) {
		if (IsInBox (event->xbutton.x, event->xbutton.y, 20, 1, 38, 19))
		    ChangePunc ();
		else if (IsInBox (event->xbutton.x, event->xbutton.y, 38, 1, 55, 19))
		    ChangeCorner ();
		else if (IsInBox (event->xbutton.x, event->xbutton.y, 56, 1, 73, 19))
		    ChangeGBK ();
		else if (IsInBox (event->xbutton.x, event->xbutton.y, 74, 1, 91, 19))
		    ChangeLegend ();
		else if (IsInBox (event->xbutton.x, event->xbutton.y, 92, 1, 109, 19))
		    SwitchIME (-1);
		else if (IsInBox (event->xbutton.x, event->xbutton.y, 110, 1, 127, 19))
		    SwitchSP ();

		SaveProfile ();
	    }
	}
	break;
    case FocusIn:
	if (CurrentIC) {
	    if (CurrentIC->imeState == IS_CHN)
		DisplayInputWindow ();
	}
	if (hideMainWindow != HM_HIDE)
	    XMapRaised (dpy, mainWindow);
	break;
    default:
	break;
    }
}

/*
 * 判断鼠标点击处是否处于指定的区域内
 */
Bool IsInBox (int x0, int y0, int x1, int y1, int x2, int y2)
{
    if (x0 >= x1 && x0 <= x2 && y0 >= y1 && y0 <= y2)
	return True;
    else
	return False;
}

#ifdef _USE_XFT
int StringWidth (char *str)
{
    XGlyphInfo      extents;
    char            str1[100];
    char           *ps, *ps1;
    int             l1, l2;

    if (!xftFont)
	return 0;

    l1 = strlen (str);
    l2 = 99;
    ps = str1;
    ps1 = str;

    l1 = iconv (convUTF8, (char **) &str, &l1, &ps, &l2);
    *ps = '\0';
    XftTextExtentsUtf8 (dpy, xftFont, str1, strlen (str1), &extents);
    return extents.width;
}
#else
int StringWidth (char *str)
{
    XRectangle      InkBox, LogicalBox;

    if (!fontSet)
	return 0;

    if (!str)
	return 0;

    XmbTextExtents (fontSet, str, strlen (str), &InkBox, &LogicalBox);

    return LogicalBox.width;
}
#endif

/*
 * 以指定的颜色在窗口的指定位置输出字串
 */
#ifdef _USE_XFT
void OutputString (Window window, char *str, int x, int y, XColor color)
{
    char            strOutput[100] = "";	//该长度应该够用了
    int             l1, l2;
    char           *ps;
    XftColor        xftColor;
    XRenderColor    renderColor;

    if (!xftFont)
	return;

    if (!str)
	return;

    //使用UTF8串
    l1 = strlen (str);
    l2 = 99;
    ps = strOutput;

    l1 = iconv (convUTF8, (char **) (&str), &l1, &ps, &l2);

    renderColor.red = color.red;
    renderColor.green = color.green;
    renderColor.blue = color.blue;
    renderColor.alpha = 0xFFFF;

    XftColorAllocValue (dpy, DefaultVisual (dpy, DefaultScreen (dpy)), DefaultColormap (dpy, DefaultScreen (dpy)), &renderColor, &xftColor);
    XftDrawChange (xftDraw, window);
    XftDrawStringUtf8 (xftDraw, &xftColor, xftFont, x, y, strOutput, strlen (strOutput));

    XftColorFree (dpy, DefaultVisual (dpy, DefaultScreen (dpy)), DefaultColormap (dpy, DefaultScreen (dpy)), &xftColor);
}
#else
void OutputString (Window window, char *str, int x, int y, GC gc)
{
    if (!fontSet)
	return;

    if (!str)
	return;

    XmbDrawString (dpy, window, fontSet, gc, x, y, str, strlen (str));
}
#endif

/* *************下列函数取自于 rfinput-2.0 ************************ */
/*
 * 从xpm图形文件中图形数据填入到XImage变量中
 */
int FillImageByXPMData (XImage * pImage, char **apcData)
{
    int             iWidth, iHeight;
    int             iColorCount;
    int             iCharsPerPixel;
    char           *pcNext;
    int             i, j, k;
    StrPixel       *pPixels = NULL;
    char            acColor[3];
    XColor          AColor;
    int             iRV;

    iWidth = strtol (apcData[0], &pcNext, 10);
    iHeight = strtol (pcNext, &pcNext, 10);
    iColorCount = strtol (pcNext, &pcNext, 10);
    iCharsPerPixel = strtol (pcNext, &pcNext, 10);

    // Make the color array
    if (NULL == (pPixels = (StrPixel *) malloc (sizeof (StrPixel) * iColorCount)))
	goto ErrorExit;

    for (i = 0; i < iColorCount; i++)
	pPixels[i].pcStr = NULL;

    //File the Array
    for (i = 0; i < iColorCount; i++) {
	if (NULL == (pPixels[i].pcStr = (char *) malloc (iCharsPerPixel + 1)))
	    goto ErrorExit;

	strncpy (pPixels[i].pcStr, apcData[i + 1], iCharsPerPixel);
	pPixels[i].pcStr[iCharsPerPixel] = 0;

	acColor[0] = apcData[i + 1][strlen (apcData[i + 1]) - 6];
	acColor[1] = apcData[i + 1][strlen (apcData[i + 1]) - 5];
	acColor[2] = 0;
	AColor.red = strtol (acColor, NULL, 16) * 0xffff / 0xff;
	acColor[0] = apcData[i + 1][strlen (apcData[i + 1]) - 4];
	acColor[1] = apcData[i + 1][strlen (apcData[i + 1]) - 3];
	acColor[2] = 0;
	AColor.green = strtol (acColor, NULL, 16) * 0xffff / 0xff;
	acColor[0] = apcData[i + 1][strlen (apcData[i + 1]) - 2];
	acColor[1] = apcData[i + 1][strlen (apcData[i + 1]) - 1];
	acColor[2] = 0;
	AColor.blue = strtol (acColor, NULL, 16) * 0xffff / 0xff;

	iRV = XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &AColor);
	pPixels[i].iPixel = AColor.pixel;
    }

    // Fill the image
    for (i = 0; i < iHeight; i++) {
	for (j = 0; j < iWidth; j++) {
	    for (k = 0; k < iColorCount; k++) {
		if (!strncmp (pPixels[k].pcStr, apcData[i + 1 + iColorCount] + j * iCharsPerPixel, iCharsPerPixel)) {
		    XPutPixel (pImage, j, i, pPixels[k].iPixel);
		    break;
		}
	    }
	}
    }
    // Free the Array
    if (NULL != pPixels) {
	for (i = 0; i < iColorCount; i++) {
	    if (NULL != pPixels[i].pcStr)
		free (pPixels[i].pcStr);
	}
	free (pPixels);
    }
    return 1;
  ErrorExit:
    if (NULL != pPixels) {
	for (i = 0; i < iColorCount; i++) {
	    if (NULL != pPixels[i].pcStr)
		free (pPixels[i].pcStr);
	}
	free (pPixels);
    }
    return 0;
}

XPoint MouseClick (int *x, int *y, int iWnd)
{
    XEvent          evtGrabbed;
    XPoint          point;

    // To motion the  window
    while (1) {
	XMaskEvent (dpy, PointerMotionMask | ButtonReleaseMask | ButtonPressMask, &evtGrabbed);
	if (ButtonRelease == evtGrabbed.xany.type) {
	    if (Button1 == evtGrabbed.xbutton.button) {
		break;
	    }
	}
	else if (MotionNotify == evtGrabbed.xany.type) {
	    static Time     LastTime;

	    if (evtGrabbed.xmotion.time - LastTime < 20)
		continue;

	    switch (iWnd) {
	    case 0:
		XMoveWindow (dpy, mainWindow, evtGrabbed.xmotion.x_root - *x, evtGrabbed.xmotion.y_root - *y);
		XRaiseWindow (dpy, mainWindow);
		break;
	    case 1:
		XMoveWindow (dpy, inputWindow, evtGrabbed.xmotion.x_root - *x, evtGrabbed.xmotion.y_root - *y);
		XRaiseWindow (dpy, inputWindow);
		break;
	    }

	    LastTime = evtGrabbed.xmotion.time;
	}

	switch (iWnd) {
	case 0:
	    DisplayMainWindow ();
	    break;
	case 1:
	    DisplayInputWindow ();
	    break;
	}
    }

    *x = evtGrabbed.xmotion.x_root - *x;
    *y = evtGrabbed.xmotion.y_root - *y;

    return point;
}

void WaitButtonRelease (XPoint * point)
{
    XEvent          ev;

    while (1) {
	XNextEvent (dpy, &ev);
	switch (ev.type) {
	case ButtonRelease:
	    point->x = ev.xbutton.x;
	    point->y = ev.xbutton.y;
	    return;
	default:
	    break;
	}
    }
}

/* ****************************************************************** */
