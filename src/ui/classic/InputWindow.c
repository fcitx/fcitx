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
#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cairo-xlib.h>
#include <libintl.h>

#include "fcitx/ui.h"
#include "fcitx/module.h"
#include "fcitx/profile.h"
#include "fcitx/backend.h"
#include "fcitx/configfile.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"

#include "InputWindow.h"
#include "classicui.h"
#include "skin.h"
#include "module/x11/x11stuff.h"
#include "MainWindow.h"

static boolean InputWindowEventHandler(void *arg, XEvent* event);

InputWindow* CreateInputWindow(FcitxClassicUI *classicui)
{
    XSetWindowAttributes    attrib;
    unsigned long   attribmask;
    char        strWindowName[]="Fcitx Input Window";
    int depth;
    Colormap cmap;
    Visual * vs;
    InputWindow* inputWindow;
    int iScreen = classicui->iScreen;
    Display* dpy = classicui->dpy;
    FcitxSkin *sc = &classicui->skin;
    
    inputWindow = fcitx_malloc0(sizeof(InputWindow));    
    inputWindow->window = None;
    inputWindow->iInputWindowHeight = INPUTWND_HEIGHT;
    inputWindow->iInputWindowWidth = INPUTWND_WIDTH;
    inputWindow->iOffsetX = 0;
    inputWindow->iOffsetY = 8;
    inputWindow->dpy = dpy;
    inputWindow->iScreen = iScreen;
    inputWindow->skin = sc;
    inputWindow->owner = classicui;

    SkinImage *back = LoadImage(sc, sc->skinInputBar.backImg, false);

    inputWindow->iInputWindowHeight= cairo_image_surface_get_height(back->image);
    vs=ClassicUIFindARGBVisual (classicui);
    ClassicUIInitWindowAttribute(classicui, &vs, &cmap, &attrib, &attribmask, &depth);

    inputWindow->window=XCreateWindow (dpy,
                                      RootWindow(dpy, iScreen),
                                      classicui->iMainWindowOffsetX,
                                      classicui->iMainWindowOffsetY,
                                      INPUT_BAR_MAX_WIDTH,
                                      inputWindow->iInputWindowHeight,
                                      0,
                                      depth,InputOutput,
                                      vs,attribmask,
                                      &attrib);

    inputWindow->pm_input_bar=XCreatePixmap(
                                 dpy,
                                 inputWindow->window,
                                 INPUT_BAR_MAX_WIDTH,
                                 INPUT_BAR_MAX_HEIGHT,
                                 depth);
    inputWindow->cs_input_bar=cairo_xlib_surface_create(
                                 dpy,
                                 inputWindow->pm_input_bar,
                                 vs,
                                 INPUT_BAR_MAX_WIDTH,
                                 INPUT_BAR_MAX_HEIGHT);

    inputWindow->cs_input_back = cairo_surface_create_similar(inputWindow->cs_input_bar,
            CAIRO_CONTENT_COLOR_ALPHA,
            INPUT_BAR_MAX_WIDTH,
            INPUT_BAR_MAX_HEIGHT);

    LoadInputMessage(sc, inputWindow, classicui->font);
    XSelectInput (dpy, inputWindow->window, ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | ExposureMask);

    ClassicUISetWindowProperty(classicui, inputWindow->window, FCITX_WINDOW_DOCK, strWindowName);

    FcitxModuleFunctionArg arg;
    arg.args[0] = InputWindowEventHandler;
    arg.args[1] = inputWindow;
    InvokeFunction(classicui->owner, FCITX_X11, ADDXEVENTHANDLER, arg);
    return inputWindow;
}

boolean InputWindowEventHandler(void *arg, XEvent* event)
{
    InputWindow* inputWindow = arg;
    if (event->xany.window == inputWindow->window)
    {
        switch (event->type)
        {
            case Expose:
                DrawInputWindow(inputWindow);
                break;
            case ButtonPress:
                switch (event->xbutton.button) {
                    case Button1:
                        {
                            SetMouseStatus(inputWindow->owner->mainWindow, NULL, RELEASE, RELEASE);
                            int             x,
                                            y;
                            x = event->xbutton.x;
                            y = event->xbutton.y;
                            ClassicUIMouseClick(inputWindow->owner, inputWindow->window, &x, &y);
                                                    
                            FcitxInputContext* ic = GetCurrentIC(inputWindow->owner->owner);

                            if (ic)
                                SetWindowOffset(inputWindow->owner->owner, ic, x, y);

                            DrawInputWindow(inputWindow);
                        }
                        break;
                }
                break;
        }
        return true;
    }
    return false;
}

void DisplayInputWindow (InputWindow* inputWindow)
{
    FcitxLog(DEBUG, _("DISPLAY InputWindow"));
    MoveInputWindowInternal(inputWindow);
    XMapRaised (inputWindow->dpy, inputWindow->window);
}

void DrawInputWindow(InputWindow* inputWindow)
{
    int lastW = inputWindow->iInputWindowWidth, lastH = inputWindow->iInputWindowHeight;
    DrawInputBar(inputWindow->skin, inputWindow, GetMessageUp(inputWindow->owner->owner), GetMessageDown(inputWindow->owner->owner), &inputWindow->iInputWindowHeight ,&inputWindow->iInputWindowWidth);

    /* Resize Window will produce Expose Event, so there is no need to draw right now */
    if (lastW != inputWindow->iInputWindowWidth || lastH != inputWindow->iInputWindowHeight)
    {
        XResizeWindow(
                inputWindow->dpy,
                inputWindow->window,
                inputWindow->iInputWindowWidth,
                inputWindow->iInputWindowHeight);
        MoveInputWindowInternal(inputWindow);
    }
    GC gc = XCreateGC( inputWindow->dpy, inputWindow->window, 0, NULL );
    XCopyArea (inputWindow->dpy,
            inputWindow->pm_input_bar,
            inputWindow->window,
            gc,
            0,
            0,
            inputWindow->iInputWindowWidth,
            inputWindow->iInputWindowHeight, 0, 0);
    XFreeGC(inputWindow->dpy, gc);
}

void MoveInputWindowInternal(InputWindow* inputWindow)
{
    int dwidth, dheight;
    int x, y;
    GetScreenSize(inputWindow->owner, &dwidth, &dheight);
    
    FcitxInputContext* ic = GetCurrentIC(inputWindow->owner->owner);
    GetWindowPosition(inputWindow->owner->owner, ic, &x, &y);

    int iTempInputWindowX, iTempInputWindowY;

    if (x < 0)
        iTempInputWindowX = 0;
    else
        iTempInputWindowX = x + inputWindow->iOffsetX;

    if (y < 0)
        iTempInputWindowY = 0;
    else
        iTempInputWindowY = y + inputWindow->iOffsetY;

    if ((iTempInputWindowX + inputWindow->iInputWindowWidth) > dwidth)
        iTempInputWindowX = dwidth - inputWindow->iInputWindowWidth;

    if ((iTempInputWindowY + inputWindow->iInputWindowHeight) > dheight) {
        if ( iTempInputWindowY > dheight )
            iTempInputWindowY = dheight - 2 * inputWindow->iInputWindowHeight;
        else
            iTempInputWindowY = iTempInputWindowY - 2 * inputWindow->iInputWindowHeight;
    }
    XMoveWindow (inputWindow->dpy, inputWindow->window, iTempInputWindowX, iTempInputWindowY);
}

void CloseInputWindowInternal(InputWindow* inputWindow)
{
    XUnmapWindow (inputWindow->dpy, inputWindow->window);
}

void DestroyInputWindow(InputWindow* inputWindow)
{
    int i = 0;
    cairo_destroy(inputWindow->c_back);

    for ( i = 0 ; i < 7; i ++)
        cairo_destroy(inputWindow->c_font[i]);
    cairo_destroy(inputWindow->c_cursor);

    cairo_surface_destroy(inputWindow->cs_input_bar);
    cairo_surface_destroy(inputWindow->cs_input_back);
    XFreePixmap(inputWindow->dpy, inputWindow->pm_input_bar);
    XDestroyWindow(inputWindow->dpy, inputWindow->window);
    
    FcitxModuleFunctionArg arg;
    arg.args[0] = inputWindow;
    InvokeFunction(inputWindow->owner->owner, FCITX_X11, REMOVEXEVENTHANDLER, arg);
    
    free(inputWindow);
}

void ShowInputWindowInternal(InputWindow* inputWindow)
{
    XMapRaised(inputWindow->dpy, inputWindow->window);
    DrawInputWindow(inputWindow);
}
