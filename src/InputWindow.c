#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "InputWindow.h"

#include <string.h>

#ifdef _USE_XFT
#include <ft2build.h>
#include <X11/Xft/Xft.h>
#include <iconv.h>
#endif

#include "ui.h"
#include "ime.h"

//下面的顺序不能颠倒
#include "next.xpm"
#include "prev.xpm"

Window          inputWindow;
int             iInputWindowX = INPUTWND_STARTX;
int             iInputWindowY = INPUTWND_STARTY;

uint            iInputWindowHeight = INPUTWND_HEIGHT;
uint            iInputWindowWidth = INPUTWND_WIDTH;
uint            iInputWindowUpWidth = INPUTWND_WIDTH;
uint            iInputWindowDownWidth = INPUTWND_WIDTH;

MESSAGE_COLOR   messageColor[MESSAGE_TYPE_COUNT] = {
    {NULL, {0, 255 << 8, 0, 0}},
    {NULL, {0, 0, 0, 255 << 8}},
    {NULL, {0, 200 << 8, 0, 0}},
    {NULL, {0, 0, 150 << 8, 100 << 8}},
    {NULL, {0, 0, 0, 255 << 8}},
    {NULL, {0, 100 << 8, 100 << 8, 255 << 8}},
    {NULL, {0, 0, 0, 0}}
};

MESSAGE_COLOR   inputWindowLineColor = { NULL, {0, 100 << 8, 200 << 8, 255 << 8} };	//输入框中的线条色
XColor          colorArrow = { 0, 255 << 8, 150 << 8, 255 << 8 };	//箭头的颜色

WINDOW_COLOR    inputWindowColor = { NULL, NULL, {0, 240 << 8, 240 << 8, 240 << 8} };
MESSAGE_COLOR   cursorColor = { NULL, {0, 92 << 8, 210 << 8, 131 << 8} };

// *************************************************************
MESSAGE         messageUp[32];	//输入条上部分显示的内容

// *************************************************************
uint            uMessageUp = 0;

// *************************************************************
MESSAGE         messageDown[32];	//输入条下部分显示的内容

// *************************************************************
uint            uMessageDown = 0;

XImage         *pNext = NULL, *pPrev = NULL;

Bool            bIsResizingInputWindow = False;	//窗口在改变尺寸时不要重绘
Bool            bShowPrev = False;
Bool            bShowNext = False;
Bool            bTrackCursor = True;

int             iCursorPos = 0;
Bool            bShowCursor = True;

_3D_EFFECT      _3DEffectInputWindow = _3D_LOWER;

extern Display *dpy;
extern int      iScreen;

#ifdef _USE_XFT
extern iconv_t  convUTF8;
extern XftFont *xftFont;
#else
extern XFontSet fontSet;
#endif

extern GC       dimGC;
extern GC       lightGC;

Bool CreateInputWindow (void)
{
    XSetWindowAttributes attrib;
    unsigned long   attribmask;
    int             iBackPixel;

    //根据窗口的背景色来设置XPM的色彩
    sprintf (strXPMBackColor, ". c #%2x%2x%2x", inputWindowColor.backColor.red >> 8, inputWindowColor.backColor.green >> 8, inputWindowColor.backColor.blue >> 8);
    //设置箭头的颜色
    sprintf (strXPMColor, "# c #%2x%2x%2x", colorArrow.red >> 8, colorArrow.green >> 8, colorArrow.blue >> 8);

    CalculateInputWindowHeight ();

    attrib.override_redirect = True;
    attribmask = CWOverrideRedirect;

    if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &(inputWindowColor.backColor)))
	iBackPixel = inputWindowColor.backColor.pixel;
    else
	iBackPixel = WhitePixel (dpy, DefaultScreen (dpy));

    inputWindow = XCreateSimpleWindow (dpy, DefaultRootWindow (dpy), iInputWindowX, iInputWindowY, INPUTWND_WIDTH, iInputWindowHeight, 0, WhitePixel (dpy, DefaultScreen (dpy)), iBackPixel);
    if (inputWindow == (Window) NULL)
	return False;

    XChangeWindowAttributes (dpy, inputWindow, attribmask, &attrib);
    XSelectInput (dpy, inputWindow, ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | EnterWindowMask | PointerMotionMask | LeaveWindowMask | VisibilityChangeMask);

    InitInputWindowColor ();

    return True;
}

/*
 * 根据字体的大小来调整窗口的高度
 */
void CalculateInputWindowHeight (void)
{
    int             iHeight;

#ifdef _USE_XFT
    iHeight = FontHeight (xftFont);
#else
    iHeight = FontHeight (fontSet);
#endif

    iInputWindowHeight = iHeight * 2 + iHeight / 2 + 8;
}

void DisplayInputWindow (void)
{
    XMapRaised (dpy, inputWindow);
    DisplayMessage ();
    DrawInputWindow ();
}

void DrawInputWindow (void)
{
    if (_3DEffectInputWindow == _3D_UPPER)
	Draw3DEffect (inputWindow, 2, 2, iInputWindowWidth - 4, iInputWindowHeight - 4, _3D_UPPER);
    else if (_3DEffectInputWindow == _3D_LOWER)
	Draw3DEffect (inputWindow, 0, 0, iInputWindowWidth, iInputWindowHeight, _3D_LOWER);

    XDrawRectangle (dpy, inputWindow, inputWindowLineColor.gc, 0, 0, iInputWindowWidth - 1, iInputWindowHeight - 1);
    //XDrawRectangle (dpy, inputWindow, inputWindowLineColor.gc, 1, 1, iInputWindowWidth - 3, iInputWindowHeight - 3);
    XDrawLine (dpy, inputWindow, lightGC, 2 + 5, iInputWindowHeight / 2 - 1, iInputWindowWidth - 2 - 5, iInputWindowHeight / 2 - 1);
    XDrawLine (dpy, inputWindow, inputWindowLineColor.gc, 2 + 5, iInputWindowHeight / 2, iInputWindowWidth - 2 - 5, iInputWindowHeight / 2);
    XDrawLine (dpy, inputWindow, dimGC, 2 + 5, iInputWindowHeight / 2 + 1, iInputWindowWidth - 2 - 5, iInputWindowHeight / 2 + 1);

    if (bShowPrev) {
	if (!pPrev) {
	    pPrev = XGetImage (dpy, inputWindow, 0, 0, 7, 13, AllPlanes, XYPixmap);
	    FillImageByXPMData (pPrev, xpm_prev);
	}
	XPutImage (dpy, inputWindow, inputWindowColor.foreGC, pPrev, 0, 0, iInputWindowWidth - 20, (iInputWindowHeight / 2 - 12) / 2, 6, 12);
    }
    if (bShowNext) {
	if (!pNext) {
	    pNext = XGetImage (dpy, inputWindow, 0, 0, 7, 13, AllPlanes, XYPixmap);
	    FillImageByXPMData (pNext, xpm_next);
	}
	XPutImage (dpy, inputWindow, inputWindowColor.foreGC, pNext, 0, 0, iInputWindowWidth - 10, (iInputWindowHeight / 2 - 12) / 2, 6, 12);
    }
}

void InitInputWindowColor (void)
{
    XGCValues       values;
    int             iPixel;
    int             i;

    for (i = 0; i < MESSAGE_TYPE_COUNT; i++) {
	messageColor[i].gc = XCreateGC (dpy, inputWindow, 0, &values);
	if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &(messageColor[i].color)))
	    iPixel = messageColor[i].color.pixel;
	else
	    iPixel = WhitePixel (dpy, DefaultScreen (dpy));
	XSetForeground (dpy, messageColor[i].gc, iPixel);
    }

    inputWindowLineColor.gc = XCreateGC (dpy, inputWindow, 0, &values);
    if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &(inputWindowLineColor.color)))
	iPixel = inputWindowLineColor.color.pixel;
    else
	iPixel = WhitePixel (dpy, DefaultScreen (dpy));
    XSetForeground (dpy, inputWindowLineColor.gc, iPixel);

    cursorColor.color.red = cursorColor.color.red ^ inputWindowColor.backColor.red;
    cursorColor.color.green = cursorColor.color.green ^ inputWindowColor.backColor.green;
    cursorColor.color.blue = cursorColor.color.blue ^ inputWindowColor.backColor.blue;
    cursorColor.gc = XCreateGC (dpy, inputWindow, 0, &values);
    //为了画绿色光标
    if (XAllocColor (dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &cursorColor.color))
	iPixel = cursorColor.color.pixel;
    else
	iPixel = BlackPixel (dpy, DefaultScreen (dpy));

    XSetForeground (dpy, cursorColor.gc, iPixel);
    XSetFunction (dpy, cursorColor.gc, GXxor);
}

void ResetInputWindow (void)
{
    uMessageDown = 0;
    uMessageUp = 0;
}

void DisplayMessage (void)
{
    int             i;
    XWindowAttributes wa;

    bIsResizingInputWindow = True;	//由于改变窗口的属性可能会引起窗口重画，利用此变量来防止窗口抖动

    XClearArea (dpy, inputWindow, 2, 2, iInputWindowWidth - 2, iInputWindowHeight / 2 - 2, False);
    XClearArea (dpy, inputWindow, 2, iInputWindowHeight / 2 + 1, iInputWindowWidth - 2, iInputWindowHeight / 2 - 2, False);

    iInputWindowUpWidth = 0;
    for (i = 0; i < uMessageUp; i++)
#ifdef _USE_XFT
	iInputWindowUpWidth += StringWidth (messageUp[i].strMsg, xftFont);
#else
	iInputWindowUpWidth += StringWidth (messageUp[i].strMsg, fontSet);
#endif

    iInputWindowUpWidth += 2 * INPUTWND_START_POS_UP + 1;
    if (bShowPrev)
	iInputWindowUpWidth += 16;
    else if (bShowNext)
	iInputWindowUpWidth += 8;

    iInputWindowDownWidth = 0;
    for (i = 0; i < uMessageDown; i++)
#ifdef _USE_XFT
	iInputWindowDownWidth += StringWidth (messageDown[i].strMsg, xftFont);
#else
	iInputWindowDownWidth += StringWidth (messageDown[i].strMsg, fontSet);
#endif

    iInputWindowDownWidth += 2 * INPUTWND_START_POS_DOWN + 1;

    if (iInputWindowUpWidth < iInputWindowDownWidth)
	iInputWindowWidth = iInputWindowDownWidth;
    else
	iInputWindowWidth = iInputWindowUpWidth;

    if (iInputWindowWidth < INPUTWND_WIDTH)
	iInputWindowWidth = INPUTWND_WIDTH;

    XGetWindowAttributes (dpy, inputWindow, &wa);
    if ((wa.x + iInputWindowWidth) > DisplayWidth (dpy, iScreen))
	i = DisplayWidth (dpy, iScreen) - iInputWindowWidth - 2;
    else if (wa.x < 0) {
	if (iInputWindowWidth <= DisplayWidth (dpy, iScreen))
	    i = 0;
	else
	    i = DisplayWidth (dpy, iScreen) - iInputWindowWidth;
    }
    else
	i = wa.x;

    XMoveWindow (dpy, inputWindow, i, wa.y);
    XResizeWindow (dpy, inputWindow, iInputWindowWidth, iInputWindowHeight);

    DisplayMessageUp ();
    DisplayMessageDown ();
}

/*
 * 在输入窗的上部分显示信息
 */
void DisplayMessageUp (void)
{
    int             i = 0;
    int             iPos;
    int             iCursorPixPos = 0;
    int             iChar;
    char            strText[MESSAGE_MAX_LENGTH];

    iPos = INPUTWND_START_POS_UP;
    iChar = iCursorPos;

    for (i = 0; i < uMessageUp; i++) {
#ifdef _USE_XFT
	iInputWindowUpWidth = StringWidth (messageUp[i].strMsg, xftFont);
	OutputString (inputWindow, xftFont, messageUp[i].strMsg, iPos, (2 * iInputWindowHeight - 1) / 5, messageColor[messageUp[i].type].color);
#else
	iInputWindowUpWidth = StringWidth (messageUp[i].strMsg, fontSet);
	OutputString (inputWindow, fontSet, messageUp[i].strMsg, iPos, (2 * iInputWindowHeight - 1) / 5, messageColor[messageUp[i].type].gc);
#endif

	if (bShowCursor && iChar) {
	    if (strlen (messageUp[i].strMsg) > iChar) {
		strncpy (strText, messageUp[i].strMsg, iChar);
		strText[iChar] = '\0';
#ifdef _USE_XFT
		iCursorPixPos += StringWidth (strText, xftFont);
#else
		iCursorPixPos += StringWidth (strText, fontSet);
#endif
		iChar = 0;
	    }
	    else {
		iCursorPixPos += iInputWindowUpWidth;
		iChar -= strlen (messageUp[i].strMsg);
	    }
	}

	iPos += iInputWindowUpWidth;
    }

    if (bShowCursor)
	DrawCursor (INPUTWND_START_POS_UP + iCursorPixPos);
}

/*
 * 在输入窗的下部分显示信息
 */
void DisplayMessageDown (void)
{
    uint            i;
    uint            iPos;

    iPos = INPUTWND_START_POS_DOWN;
    for (i = 0; i < uMessageDown; i++) {
	//借用iInputWindowDownWidth作为一个临时变量

#ifdef _USE_XFT
	iInputWindowDownWidth = StringWidth (messageDown[i].strMsg, xftFont);
	OutputString (inputWindow, xftFont, messageDown[i].strMsg, iPos, (9 * iInputWindowHeight - 12) / 10, messageColor[messageDown[i].type].color);
#else
	iInputWindowDownWidth = StringWidth (messageDown[i].strMsg, fontSet);
	OutputString (inputWindow, fontSet, messageDown[i].strMsg, iPos, (9 * iInputWindowHeight - 12) / 10, messageColor[messageDown[i].type].gc);
#endif
	iPos += iInputWindowDownWidth;
    }
}

void DrawCursor (int iPos)
{
    XDrawLine (dpy, inputWindow, cursorColor.gc, iPos, 8, iPos, iInputWindowHeight / 2 - 4);
    XDrawLine (dpy, inputWindow, cursorColor.gc, iPos + 1, 8, iPos + 1, iInputWindowHeight / 2 - 4);
}
