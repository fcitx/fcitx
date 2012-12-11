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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <libintl.h>

#include "fcitx/ui.h"
#include "fcitx/module.h"
#include "fcitx/profile.h"
#include "fcitx/frontend.h"
#include "fcitx/configfile.h"
#include "fcitx/instance.h"
#include "fcitx/candidate.h"
#include "fcitx-utils/utils.h"

#include "InputWindow.h"
#include "classicui.h"
#include "skin.h"
#include "MainWindow.h"
#include "fcitx-utils/log.h"
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

static boolean InputWindowEventHandler(void *arg, XEvent* event);
static void InitInputWindow(InputWindow* inputWindow);
static void ReloadInputWindow(void* arg, boolean enabled);

void InitInputWindow(InputWindow* inputWindow)
{
    XSetWindowAttributes    attrib;
    unsigned long   attribmask;
    char        strWindowName[] = "Fcitx Input Window";
    int depth;
    Colormap cmap;
    Visual * vs;
    FcitxClassicUI* classicui = inputWindow->owner;
    int iScreen = classicui->iScreen;
    Display* dpy = classicui->dpy;
    FcitxSkin *sc = &classicui->skin;
    inputWindow->window = None;
    inputWindow->iInputWindowHeight = INPUTWND_HEIGHT;
    inputWindow->iInputWindowWidth = INPUTWND_WIDTH;
    inputWindow->iOffsetX = 0;
    inputWindow->iOffsetY = 8;
    inputWindow->dpy = dpy;
    inputWindow->iScreen = iScreen;
    inputWindow->skin = sc;

    SkinImage *back = LoadImage(sc, sc->skinInputBar.backImg, false);

    inputWindow->iInputWindowHeight = cairo_image_surface_get_height(back->image);
    vs = ClassicUIFindARGBVisual(classicui);
    ClassicUIInitWindowAttribute(classicui, &vs, &cmap, &attrib, &attribmask, &depth);

    inputWindow->window = XCreateWindow(dpy,
                                        RootWindow(dpy, iScreen),
                                        classicui->iMainWindowOffsetX,
                                        classicui->iMainWindowOffsetY,
                                        cairo_image_surface_get_width(back->image),
                                        inputWindow->iInputWindowHeight,
                                        0,
                                        depth, InputOutput,
                                        vs, attribmask,
                                        &attrib);

    inputWindow->cs_x_input_bar = cairo_xlib_surface_create(
                                      dpy,
                                      inputWindow->window,
                                      vs,
                                      cairo_image_surface_get_width(back->image),
                                      cairo_image_surface_get_height(back->image));
    inputWindow->cs_input_bar = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                INPUT_BAR_MAX_WIDTH,
                                INPUT_BAR_MAX_HEIGHT);

    inputWindow->cs_input_back = cairo_surface_create_similar(inputWindow->cs_input_bar,
                                 CAIRO_CONTENT_COLOR_ALPHA,
                                 INPUT_BAR_MAX_WIDTH,
                                 INPUT_BAR_MAX_HEIGHT);

    LoadInputMessage(sc, inputWindow, classicui->font);
    XSelectInput(dpy, inputWindow->window, ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | ExposureMask);

    ClassicUISetWindowProperty(classicui, inputWindow->window, FCITX_WINDOW_DOCK, strWindowName);
}

InputWindow* CreateInputWindow(FcitxClassicUI *classicui)
{
    InputWindow* inputWindow;


    inputWindow = fcitx_utils_malloc0(sizeof(InputWindow));
    inputWindow->owner = classicui;
    InitInputWindow(inputWindow);

    FcitxX11AddXEventHandler(classicui->owner,
                             InputWindowEventHandler, inputWindow);
    FcitxX11AddCompositeHandler(classicui->owner,
                                ReloadInputWindow, inputWindow);

    inputWindow->msgUp = FcitxMessagesNew();
    inputWindow->msgDown = FcitxMessagesNew();
    return inputWindow;
}

boolean InputWindowEventHandler(void *arg, XEvent* event)
{
    InputWindow* inputWindow = arg;
    if (event->xany.window == inputWindow->window) {
        switch (event->type) {
        case Expose:
            DrawInputWindow(inputWindow);
            break;
        case ButtonPress:
            switch (event->xbutton.button) {
            case Button1: {
                SetMouseStatus(inputWindow->owner->mainWindow, NULL, RELEASE, RELEASE);
                int             x,
                                y;
                x = event->xbutton.x;
                y = event->xbutton.y;
                ClassicUIMouseClick(inputWindow->owner, inputWindow->window, &x, &y);

                FcitxInputContext* ic = FcitxInstanceGetCurrentIC(inputWindow->owner->owner);

                if (ic)
                    FcitxInstanceSetWindowOffset(inputWindow->owner->owner, ic, x, y);

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

void DisplayInputWindow(InputWindow* inputWindow)
{
    FcitxLog(DEBUG, _("DISPLAY InputWindow"));
    XMapRaised(inputWindow->dpy, inputWindow->window);
}

void DrawInputWindow(InputWindow* inputWindow)
{
    int lastW = inputWindow->iInputWindowWidth, lastH = inputWindow->iInputWindowHeight;
    int cursorPos = FcitxUINewMessageToOldStyleMessage(inputWindow->owner->owner, inputWindow->msgUp, inputWindow->msgDown);
    FcitxInputState* input = FcitxInstanceGetInputState(inputWindow->owner->owner);
    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
    FcitxCandidateLayoutHint layout = FcitxCandidateWordGetLayoutHint(candList);

    boolean vertical = inputWindow->owner->bVerticalList;
    if (layout == CLH_Vertical)
        vertical = true;
    else if (layout == CLH_Horizontal)
        vertical = false;

    DrawInputBar(inputWindow->skin, inputWindow, vertical, cursorPos, inputWindow->msgUp, inputWindow->msgDown, &inputWindow->iInputWindowHeight , &inputWindow->iInputWindowWidth);

    /* Resize Window will produce Expose Event, so there is no need to draw right now */
    if (lastW != inputWindow->iInputWindowWidth || lastH != inputWindow->iInputWindowHeight) {
        _CAIRO_SETSIZE(inputWindow->cs_x_input_bar,
                       inputWindow->iInputWindowWidth,
                       inputWindow->iInputWindowHeight);
        MoveInputWindowInternal(inputWindow);
        XResizeWindow(
            inputWindow->dpy,
            inputWindow->window,
            inputWindow->iInputWindowWidth,
            inputWindow->iInputWindowHeight);
    }

    cairo_t* c = cairo_create(inputWindow->cs_x_input_bar);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(c, inputWindow->cs_input_bar, 0, 0);
    cairo_rectangle(c, 0, 0, inputWindow->iInputWindowWidth, inputWindow->iInputWindowHeight);
    cairo_clip(c);
    cairo_paint(c);
    cairo_destroy(c);
    cairo_surface_flush(inputWindow->cs_x_input_bar);

    XFlush(inputWindow->dpy);
}

void MoveInputWindowInternal(InputWindow* inputWindow)
{
    int x = 0, y = 0, w = 0, h = 0;

    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(inputWindow->owner->owner);
    FcitxInstanceGetWindowRect(inputWindow->owner->owner, ic, &x, &y, &w, &h);
    FcitxRect rect = GetScreenGeometry(inputWindow->owner, x, y);

    int iTempInputWindowX, iTempInputWindowY;

    if (x < rect.x1)
        iTempInputWindowX = rect.x1;
    else
        iTempInputWindowX = x + inputWindow->iOffsetX;

    if (y < rect.y1)
        iTempInputWindowY = rect.y1;
    else
        iTempInputWindowY = y + h + inputWindow->iOffsetY;

    if ((iTempInputWindowX + inputWindow->iInputWindowWidth) > rect.x2)
        iTempInputWindowX =  rect.x2 - inputWindow->iInputWindowWidth;

    if ((iTempInputWindowY + inputWindow->iInputWindowHeight) >  rect.y2) {
        if (iTempInputWindowY >  rect.y2)
            iTempInputWindowY =  rect.y2 - inputWindow->iInputWindowHeight - 40;
        else /* better position the window */
            iTempInputWindowY = iTempInputWindowY - inputWindow->iInputWindowHeight - ((h == 0)?40:h) - 2 * inputWindow->iOffsetY;
    }
    XMoveWindow(inputWindow->dpy, inputWindow->window, iTempInputWindowX, iTempInputWindowY);
}

void CloseInputWindowInternal(InputWindow* inputWindow)
{
    XUnmapWindow(inputWindow->dpy, inputWindow->window);
}

void ReloadInputWindow(void* arg, boolean enabled)
{
    InputWindow* inputWindow = (InputWindow*) arg;
    boolean visable = WindowIsVisable(inputWindow->dpy, inputWindow->window);
    int i = 0;
    cairo_destroy(inputWindow->c_back);
    inputWindow->c_back = NULL;

    for (i = 0 ; i < 7; i ++) {
        cairo_destroy(inputWindow->c_font[i]);
        inputWindow->c_font[i] = NULL;
    }
    cairo_destroy(inputWindow->c_cursor);
    inputWindow->c_cursor = NULL;

    cairo_surface_destroy(inputWindow->cs_input_bar);
    cairo_surface_destroy(inputWindow->cs_input_back);
    cairo_surface_destroy(inputWindow->cs_x_input_bar);
    XDestroyWindow(inputWindow->dpy, inputWindow->window);

    inputWindow->cs_input_back = NULL;
    inputWindow->cs_input_bar = NULL;
    inputWindow->cs_x_input_bar = NULL;
    inputWindow->window = None;

    InitInputWindow(inputWindow);

    if (visable)
        ShowInputWindowInternal(inputWindow);
}

void ShowInputWindowInternal(InputWindow* inputWindow)
{
    if (!WindowIsVisable(inputWindow->dpy, inputWindow->window))
        MoveInputWindowInternal(inputWindow);
    XMapRaised(inputWindow->dpy, inputWindow->window);
    DrawInputWindow(inputWindow);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
