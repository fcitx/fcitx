/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "InputWindow.h"

#include <string.h>
#include "version.h"
#include <time.h>

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
int             iTempInputWindowX, iTempInputWindowY;	//记录输入条的临时位置，用于光标跟随模式

uint            iInputWindowHeight = INPUTWND_HEIGHT;
uint            iFixedInputWindowWidth = 400;
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
uint            uMessageUp = 0;

// *************************************************************
MESSAGE         messageDown[32];	//输入条下部分显示的内容
uint            uMessageDown = 0;

XImage         *pNext = NULL, *pPrev = NULL;

Bool            bIsResizingInputWindow = False;	//窗口在改变尺寸时不要重绘
Bool            bShowPrev = False;
Bool            bShowNext = False;
Bool            bTrackCursor = True;
Bool            bCenterInputWindow = True;
Bool		bShowInputWindowTriggering=True;

int             iCursorPos = 0;
Bool            bShowCursor = False;

_3D_EFFECT      _3DEffectInputWindow = _3D_LOWER;

extern Display *dpy;
extern int      iScreen;

#ifdef _USE_XFT
extern iconv_t  convUTF8;
extern XftFont *xftFont;
extern XftFont *xftFontEn;
#else
extern XFontSet fontSet;
#endif

extern GC       dimGC;
extern GC       lightGC;

extern Bool     bStartRecordType;
extern Bool     bShowUserSpeed;
extern time_t   timeStart;
extern uint     iHZInputed;

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
	Draw3DEffect (inputWindow, 1, 1, iInputWindowWidth - 2, iInputWindowHeight - 2, _3D_UPPER);
    else if (_3DEffectInputWindow == _3D_LOWER)
	Draw3DEffect (inputWindow, 0, 0, iInputWindowWidth, iInputWindowHeight, _3D_LOWER);

    XDrawRectangle (dpy, inputWindow, inputWindowLineColor.gc, 0, 0, iInputWindowWidth - 1, iInputWindowHeight - 1);
    //XDrawRectangle (dpy, inputWindow, inputWindowLineColor.gc, 1, 1, iInputWindowWidth - 3, iInputWindowHeight - 3);
    if (_3DEffectInputWindow == _3D_LOWER)
	XDrawLine (dpy, inputWindow, lightGC, 2 + 5, iInputWindowHeight / 2 - 1, iInputWindowWidth - 2 - 5, iInputWindowHeight / 2 - 1);
    else if (_3DEffectInputWindow == _3D_UPPER)
	XDrawLine (dpy, inputWindow, dimGC, 2 + 5, iInputWindowHeight / 2 - 1, iInputWindowWidth - 2 - 5, iInputWindowHeight / 2 - 1);
    XDrawLine (dpy, inputWindow, inputWindowLineColor.gc, 2 + 5, iInputWindowHeight / 2, iInputWindowWidth - 2 - 5, iInputWindowHeight / 2);
    if (_3DEffectInputWindow == _3D_LOWER)
	XDrawLine (dpy, inputWindow, dimGC, 2 + 5, iInputWindowHeight / 2 + 1, iInputWindowWidth - 2 - 5, iInputWindowHeight / 2 + 1);
    else if (_3DEffectInputWindow == _3D_UPPER)
	XDrawLine (dpy, inputWindow, lightGC, 2 + 5, iInputWindowHeight / 2 + 1, iInputWindowWidth - 2 - 5, iInputWindowHeight / 2 + 1);

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

#ifdef _USE_XFT
    char            strTemp[MESSAGE_MAX_LENGTH];
    char           *p1, *p2;
    Bool            bEn;
#endif
    XWindowAttributes wa;

    bIsResizingInputWindow = True;	//由于改变窗口的属性可能会引起窗口重画，利用此变量来防止窗口抖动

    XClearArea (dpy, inputWindow, 2, 2, iInputWindowWidth - 2, iInputWindowHeight / 2 - 2, False);
    XClearArea (dpy, inputWindow, 2, iInputWindowHeight / 2 + 1, iInputWindowWidth - 2, iInputWindowHeight / 2 - 2, False);

    if (!uMessageUp && !uMessageDown) {
	bShowCursor = False;
	uMessageUp = 1;
	strcpy (messageUp[0].strMsg, "FCITX V");
	strcat (messageUp[0].strMsg, FCITX_VERSION);
	messageUp[0].type = MSG_TIPS;

	if (bStartRecordType && bShowUserSpeed) {
	    double          timePassed;

	    timePassed = difftime (time (NULL), timeStart);
	    if (((int) timePassed) == 0)
		timePassed = 1.0;

	    uMessageDown = 6;
	    strcpy (messageDown[0].strMsg, "打字速度：");
	    messageDown[0].type = MSG_OTHER;
	    sprintf (messageDown[1].strMsg, "%d", (int) (iHZInputed * 60 / timePassed));
	    messageDown[1].type = MSG_CODE;
	    strcpy (messageDown[2].strMsg, "/分  用时：");
	    messageDown[2].type = MSG_OTHER;
	    sprintf (messageDown[3].strMsg, "%d", (int) timePassed);
	    messageDown[3].type = MSG_CODE;
	    strcpy (messageDown[4].strMsg, "秒  字数：");
	    messageDown[4].type = MSG_OTHER;
	    sprintf (messageDown[5].strMsg, "%u", iHZInputed);
	    messageDown[5].type = MSG_CODE;
	}
	else {
	    uMessageDown = 1;
	    strcpy (messageDown[0].strMsg, "http://www.fcitx.org");
	    messageDown[0].type = MSG_CODE;
	}
    }

    iInputWindowUpWidth = 2 * INPUTWND_START_POS_UP + 1;
    for (i = 0; i < uMessageUp; i++) {
#ifdef _USE_XFT
	p1 = messageUp[i].strMsg;
	while (*p1) {
	    p2 = strTemp;
	    if (isprint (*p1))	//使用中文字体
		bEn = True;
	    else {
		*p2++ = *p1++;
		*p2++ = *p1++;
		bEn = False;
	    }
	    while (*p1) {
		if (isprint (*p1)) {
		    if (!bEn)
			break;
		    *p2++ = *p1++;
		}
		else {
		    if (bEn)
			break;
		    *p2++ = *p1++;
		    *p2++ = *p1++;
		}
	    }
	    *p2 = '\0';

	    iInputWindowUpWidth += StringWidth (strTemp, (bEn) ? xftFontEn : xftFont);
	}
#else
	iInputWindowUpWidth += StringWidth (messageUp[i].strMsg, fontSet);
#endif
    }

    if (bShowPrev)
	iInputWindowUpWidth += 16;
    else if (bShowNext)
	iInputWindowUpWidth += 8;

    iInputWindowDownWidth = 2 * INPUTWND_START_POS_DOWN + 1;
    for (i = 0; i < uMessageDown; i++) {
#ifdef _USE_XFT
	p1 = messageDown[i].strMsg;
	while (*p1) {
	    p2 = strTemp;
	    if (isprint (*p1))	//使用中文字体
		bEn = True;
	    else {
		*p2++ = *p1++;
		*p2++ = *p1++;
		bEn = False;
	    }
	    while (*p1) {
		if (isprint (*p1)) {
		    if (!bEn)
			break;
		    *p2++ = *p1++;
		}
		else {
		    if (bEn)
			break;
		    *p2++ = *p1++;
		    *p2++ = *p1++;
		}
	    }
	    *p2 = '\0';

	    iInputWindowDownWidth += StringWidth (strTemp, (bEn) ? xftFontEn : xftFont);
	}
#else
	iInputWindowDownWidth += StringWidth (messageDown[i].strMsg, fontSet);
#endif
    }

    if (iInputWindowUpWidth < iInputWindowDownWidth)
	iInputWindowWidth = iInputWindowDownWidth;
    else
	iInputWindowWidth = iInputWindowUpWidth;

    if (iInputWindowWidth < INPUTWND_WIDTH)
	iInputWindowWidth = INPUTWND_WIDTH;
    if (iFixedInputWindowWidth) {
	if (iInputWindowWidth < iFixedInputWindowWidth)
	    iInputWindowWidth = iFixedInputWindowWidth;
    }

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
    if (bCenterInputWindow && !bTrackCursor) {
	iInputWindowX = (DisplayWidth (dpy, iScreen) - iInputWindowWidth) / 2;
	if (iInputWindowX < 0)
	    iInputWindowX = 0;
	XMoveWindow (dpy, inputWindow, iInputWindowX, iInputWindowY);
    }

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

#ifdef _USE_XFT
    char            strTemp[MESSAGE_MAX_LENGTH];
    char           *p1, *p2;
    Bool            bEn;
#endif

    iPos = INPUTWND_START_POS_UP;
    iChar = iCursorPos;

    for (i = 0; i < uMessageUp; i++) {
#ifdef _USE_XFT
	p1 = messageUp[i].strMsg;
	while (*p1) {
	    p2 = strTemp;
	    if (isprint (*p1))	//使用中文字体
		bEn = True;
	    else {
		*p2++ = *p1++;
		*p2++ = *p1++;
		bEn = False;
	    }
	    while (*p1) {
		if (isprint (*p1)) {
		    if (!bEn)
			break;
		    *p2++ = *p1++;
		}
		else {
		    if (bEn)
			break;
		    *p2++ = *p1++;
		    *p2++ = *p1++;
		}
	    }
	    *p2 = '\0';

	    iInputWindowUpWidth = StringWidth (strTemp, (bEn) ? xftFontEn : xftFont);
	    OutputString (inputWindow, (bEn) ? xftFontEn : xftFont, strTemp, iPos, (2 * iInputWindowHeight - 1) / 5, messageColor[messageUp[i].type].color);
	    iPos += iInputWindowUpWidth;
	}
#else
	iInputWindowUpWidth = StringWidth (messageUp[i].strMsg, fontSet);
	OutputString (inputWindow, fontSet, messageUp[i].strMsg, iPos, (2 * iInputWindowHeight - 1) / 5, messageColor[messageUp[i].type].gc);
	iPos += iInputWindowUpWidth;
#endif

	if (bShowCursor && iChar) {
	    if (strlen (messageUp[i].strMsg) > iChar) {
		strncpy (strText, messageUp[i].strMsg, iChar);
		strText[iChar] = '\0';
#ifdef _USE_XFT
		p1 = strText;
		while (*p1) {
		    p2 = strTemp;
		    if (isprint (*p1))	//使用中文字体
			bEn = True;
		    else {
			*p2++ = *p1++;
			*p2++ = *p1++;
			bEn = False;
		    }
		    while (*p1) {
			if (isprint (*p1)) {
			    if (!bEn)
				break;
			    *p2++ = *p1++;
			}
			else {
			    if (bEn)
				break;
			    *p2++ = *p1++;
			    *p2++ = *p1++;
			}
		    }
		    *p2 = '\0';

		    iCursorPixPos += StringWidth (strTemp, (bEn) ? xftFontEn : xftFont);
		}
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

#ifdef _USE_XFT
    char            strTemp[MESSAGE_MAX_LENGTH];
    char           *p1, *p2;
    Bool            bEn;
#endif

    iPos = INPUTWND_START_POS_DOWN;
    for (i = 0; i < uMessageDown; i++) {
	//借用iInputWindowDownWidth作为一个临时变量

#ifdef _USE_XFT
	p1 = messageDown[i].strMsg;
	
	while (*p1) {
	    p2 = strTemp;
	    if (isprint (*p1))	//使用中文字体
		bEn = True;
	    else {
		*p2++ = *p1++;
		*p2++ = *p1++;
		bEn = False;
	    }
	    while (*p1) {
		if (isprint (*p1)) {
		    if (!bEn)
			break;
		    *p2++ = *p1++;
		}
		else {
		    if (bEn)
			break;
		    *p2++ = *p1++;
		    *p2++ = *p1++;
		}
	    }
	    *p2 = '\0';

	    iInputWindowDownWidth = StringWidth (strTemp, (bEn) ? xftFontEn : xftFont);
	    OutputString (inputWindow, (bEn) ? xftFontEn : xftFont, strTemp, iPos, (9 * iInputWindowHeight - 12) / 10, messageColor[messageDown[i].type].color);
	    iPos += iInputWindowDownWidth;
	}

	/*iInputWindowDownWidth = StringWidth (messageDown[i].strMsg, xftFont);
	   OutputString (inputWindow, xftFont, messageDown[i].strMsg, iPos, (9 * iInputWindowHeight - 12) / 10, messageColor[messageDown[i].type].color);
	   iPos += iInputWindowDownWidth; */
#else
	iInputWindowDownWidth = StringWidth (messageDown[i].strMsg, fontSet);
	OutputString (inputWindow, fontSet, messageDown[i].strMsg, iPos, (9 * iInputWindowHeight - 12) / 10, messageColor[messageDown[i].type].gc);
	iPos += iInputWindowDownWidth;
#endif
    }
}

void DrawCursor (int iPos)
{
    XDrawLine (dpy, inputWindow, cursorColor.gc, iPos, 8, iPos, iInputWindowHeight / 2 - 4);
    XDrawLine (dpy, inputWindow, cursorColor.gc, iPos + 1, 8, iPos + 1, iInputWindowHeight / 2 - 4);
}
