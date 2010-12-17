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

#include <string.h>
#include <ctype.h>
#include <time.h>
#include <X11/Xatom.h>

#include "core/fcitx.h"

#include "ui/InputWindow.h"
#include "ui/ui.h"
#include "core/ime.h"

#include "interface/DBus.h"
#include "skin.h"
#include "tools/profile.h"
#include "tools/configfile.h"
#include "fcitx-config/cutils.h"

InputWindow     inputWindow;
Messages        messageUp;
Messages        messageDown;
int iCursorPos = 0;

extern Display *dpy;

extern int iScreen;
//计算速度
extern Bool     bStartRecordType;
extern time_t   timeStart;
extern uint     iHZInputed;

extern int  iClientCursorX;
extern int  iClientCursorY;

#ifdef _DEBUG
extern char     strXModifiers[];
#endif

#ifdef _ENABLE_RECORDING
extern FILE *fpRecord;
#endif
static void InitInputWindow();

void InitInputWindow()
{
    memset(&inputWindow, 0, sizeof(InputWindow));
    inputWindow.window = None;
    inputWindow.iInputWindowHeight = INPUTWND_HEIGHT;
    inputWindow.iInputWindowWidth = INPUTWND_WIDTH;
    inputWindow.bShowPrev = False;
    inputWindow.bShowNext = False;
    inputWindow.bShowCursor = False;
    inputWindow.iOffsetX = 0;
    inputWindow.iOffsetY = 8;
}

Bool CreateInputWindow (void)
{
    XSetWindowAttributes    attrib;
    unsigned long   attribmask;
    XTextProperty   tp;
    char        strWindowName[]="Fcitx Input Window";
    int depth;
    Colormap cmap;
    Visual * vs;
    int scr;

    InitInputWindow();

    LoadInputBarImage();
    CalculateInputWindowHeight ();
    scr=DefaultScreen(dpy);
    vs=FindARGBVisual (dpy, scr);
    InitWindowAttribute(&vs, &cmap, &attrib, &attribmask, &depth);

    inputWindow.window=XCreateWindow (dpy,
                                      RootWindow(dpy, scr),
                                      fcitxProfile.iInputWindowOffsetX,
                                      fcitxProfile.iInputWindowOffsetY,
                                      INPUT_BAR_MAX_LEN,
                                      inputWindow.iInputWindowHeight,
                                      0,
                                      depth,InputOutput,
                                      vs,attribmask,
                                      &attrib);

    if (mainWindow.window == None)
        return False;

    inputWindow.pm_input_bar=XCreatePixmap(
                                 dpy,
                                 inputWindow.window,
                                 INPUT_BAR_MAX_LEN,
                                 inputWindow.iInputWindowHeight,
                                 depth);
    inputWindow.cs_input_bar=cairo_xlib_surface_create(
                                 dpy,
                                 inputWindow.pm_input_bar,
                                 vs,
                                 INPUT_BAR_MAX_LEN,
                                 inputWindow.iInputWindowHeight);

    inputWindow.cs_input_back = cairo_surface_create_similar(inputWindow.cs_input_bar,
            CAIRO_CONTENT_COLOR_ALPHA,
            INPUT_BAR_MAX_LEN,
            inputWindow.iInputWindowHeight);

    LoadInputMessage();
    XSelectInput (dpy, inputWindow.window, ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | ExposureMask);

    /* Set the name of the window */
    tp.value = (void *)strWindowName;
    tp.encoding = XA_STRING;
    tp.format = 16;
    tp.nitems = strlen(strWindowName);
    XSetWMName (dpy, inputWindow.window, &tp);

    return True;
}

/*
 * 根据字体的大小来调整窗口的高度
 */
void CalculateInputWindowHeight (void)
{
    inputWindow.iInputWindowHeight=sc.skinInputBar.backImg.height;
}

void DisplayInputWindow (void)
{
#ifdef _DEBUG
    FcitxLog(DEBUG, _("DISPLAY InputWindow"));
#endif
    CalInputWindow();
    MoveInputWindow();
    if (MESSAGE_IS_NOT_EMPTY)
    {
        if (!fc.bUseDBus)
            XMapRaised (dpy, inputWindow.window);
#ifdef _ENABLE_DBUS
        else
            updateMessages();
#endif
    }
}

void ResetInputWindow (void)
{
    SetMessageCount(&messageUp, 0);
    SetMessageCount(&messageDown, 0);
}

void CalInputWindow (void)
{

#ifdef _DEBUG
    FcitxLog(DEBUG, _("CAL InputWindow"));
#endif

    if (MESSAGE_IS_EMPTY) {
        inputWindow.bShowCursor = False;
        if (fc.bShowVersion) {
            AddMessageAtLast(&messageUp, MSG_TIPS, "FCITX " VERSION);
        }

        //显示打字速度
        if (bStartRecordType && fc.bShowUserSpeed) {
            double          timePassed;

            timePassed = difftime (time (NULL), timeStart);
            if (((int) timePassed) == 0)
                timePassed = 1.0;

            SetMessageCount(&messageDown, 0);
            AddMessageAtLast(&messageDown, MSG_OTHER, "打字速度：");
            AddMessageAtLast(&messageDown, MSG_CODE, "%d", (int) (iHZInputed * 60 / timePassed));
            AddMessageAtLast(&messageDown, MSG_OTHER, "/分  用时：");
            AddMessageAtLast(&messageDown, MSG_CODE, "%d", (int) timePassed);
            AddMessageAtLast(&messageDown, MSG_OTHER, "秒  字数：");
            AddMessageAtLast(&messageDown, MSG_CODE, "%u", iHZInputed);
        }
    }

#ifdef _ENABLE_RECORDING
    if ( fcitxProfile.bRecording && fpRecord ) {
        if ( messageUp.msgCount > 0 ) {
            if ( MESSAGE_TYPE_IS(LAST_MESSAGE(messageUp), MSG_RECORDING) )
                DecMessageCount(&messageUp);
        }
        AddMessageAtLast(&messageUp, MSG_RECORDING, "[记录模式]");
    }
#endif
}

void DrawInputWindow(void)
{
    int lastW = inputWindow.iInputWindowWidth, lastH = inputWindow.iInputWindowHeight;
    DrawInputBar(&messageUp, &messageDown ,&inputWindow.iInputWindowWidth);

    /* Resize Window will produce Expose Event, so there is no need to draw right now */
    if (lastW != inputWindow.iInputWindowWidth || lastH != inputWindow.iInputWindowHeight)
    {
        XResizeWindow(
                dpy,
                inputWindow.window,
                inputWindow.iInputWindowWidth,
                inputWindow.iInputWindowHeight);
        MoveInputWindow();
    }
    GC gc = XCreateGC( dpy, inputWindow.window, 0, NULL );
    XCopyArea (dpy,
            inputWindow.pm_input_bar,
            inputWindow.window,
            gc,
            0,
            0,
            inputWindow.iInputWindowWidth,
            inputWindow.iInputWindowHeight, 0, 0);
    XFreeGC(dpy, gc);
}

void MoveInputWindow()
{
    int dwidth, dheight;
    GetScreenSize(&dwidth, &dheight);
    if (fcitxProfile.bTrackCursor)
    {
        Window window = None, dst;
        int offset_x, offset_y;
        if (CurrentIC)
        {
            if (CurrentIC->focus_win)
                window = CurrentIC->focus_win;
            else if(CurrentIC->client_win)
                window = CurrentIC->client_win;
            else
                return;
            
            offset_x = CurrentIC->offset_x;
            offset_y = CurrentIC->offset_y;
            
            if(offset_x < 0 || offset_y < 0)
            {
                    
                XWindowAttributes attr;
                XGetWindowAttributes(dpy, window, &attr);

                offset_x = 0;
                offset_y = attr.height;
            }
        
            XTranslateCoordinates(dpy, window, RootWindow(dpy, iScreen),
                                  offset_x, offset_y,
                                  &iClientCursorX, &iClientCursorY, &dst);
        }

        int iTempInputWindowX, iTempInputWindowY;

        if (iClientCursorX < 0)
            iTempInputWindowX = 0;
        else
            iTempInputWindowX = iClientCursorX + inputWindow.iOffsetX;

        if (iClientCursorY < 0)
            iTempInputWindowY = 0;
        else
            iTempInputWindowY = iClientCursorY + inputWindow.iOffsetY;

        if ((iTempInputWindowX + inputWindow.iInputWindowWidth) > dwidth)
            iTempInputWindowX = dwidth - inputWindow.iInputWindowWidth;

        if ((iTempInputWindowY + inputWindow.iInputWindowHeight) > dheight) {
            if ( iTempInputWindowY > dheight )
                iTempInputWindowY = dheight - 2 * inputWindow.iInputWindowHeight;
            else
                iTempInputWindowY = iTempInputWindowY - 2 * inputWindow.iInputWindowHeight;
        }

        if (!fc.bUseDBus)
        {
            XMoveWindow (dpy, inputWindow.window, iTempInputWindowX, iTempInputWindowY);
        }
#ifdef _ENABLE_DBUS
        else
        {
            if (iClientCursorX < 0)
                iTempInputWindowX = 0;
            else
                iTempInputWindowX = iClientCursorX + inputWindow.iOffsetX;

            if (iClientCursorY < 0)
                iTempInputWindowY = 0;
            else
                iTempInputWindowY = iClientCursorY + inputWindow.iOffsetY;

            KIMUpdateSpotLocation(iTempInputWindowX, iTempInputWindowY);
        }
#endif
    }
    else
    {
        if (fc.bCenterInputWindow) {
            fcitxProfile.iInputWindowOffsetX = (dwidth - inputWindow.iInputWindowWidth) / 2;
            if (fcitxProfile.iInputWindowOffsetX < 0)
                fcitxProfile.iInputWindowOffsetX = 0;
        }

        if (!fc.bUseDBus)
        {
            XMoveWindow (dpy, inputWindow.window, fcitxProfile.iInputWindowOffsetX, fcitxProfile.iInputWindowOffsetY);
        }
#ifdef _ENABLE_DBUS
        else
            KIMUpdateSpotLocation(fcitxProfile.iInputWindowOffsetX, fcitxProfile.iInputWindowOffsetY);
#endif
    }

}

void CloseInputWindow()
{
    XUnmapWindow (dpy, inputWindow.window);
#ifdef _ENABLE_DBUS
    if (fc.bUseDBus)
    {
        KIMShowAux(False);
        KIMShowPreedit(False);
        KIMShowLookupTable(False);
    }
#endif
}

void AddMessageAtLast(Messages* message, MSG_TYPE type, char *fmt, ...)
{

    if (message->msgCount < MAX_MESSAGE_COUNT)
    {
        va_list ap;
        va_start(ap, fmt);
        SetMessageV(message, message->msgCount, type, fmt, ap);
        va_end(ap);
        message->msgCount ++;
        message->changed = True;
    }
}

void SetMessage(Messages* message, int position, MSG_TYPE type, char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SetMessageV(message, position, type, fmt, ap);
    va_end(ap);
}

void SetMessageV(Messages* message, int position, MSG_TYPE type,char* fmt, va_list ap)
{
    if (position < MAX_MESSAGE_COUNT)
    {
        vsnprintf(message->msg[position].strMsg, MESSAGE_MAX_LENGTH, fmt, ap);
        message->msg[position].type = type;
        message->changed = True;
    }
}

void MessageConcatLast(Messages* message, char* text)
{
    strncat(message->msg[message->msgCount - 1].strMsg, text, MESSAGE_MAX_LENGTH);
    message->changed = True;
}

void MessageConcat(Messages* message, int position, char* text)
{
    strncat(message->msg[position].strMsg, text, MESSAGE_MAX_LENGTH);
    message->changed = True;
}

void DestroyInputWindow()
{
    int i = 0;
    cairo_destroy(inputWindow.c_back);

    for ( i = 0 ; i < 7; i ++)
        cairo_destroy(inputWindow.c_font[i]);
    cairo_destroy(inputWindow.c_cursor);

    cairo_surface_destroy(inputWindow.cs_input_bar);
    cairo_surface_destroy(inputWindow.cs_input_back);
    XFreePixmap(dpy, inputWindow.pm_input_bar);
    XDestroyWindow(dpy, inputWindow.window);
}
