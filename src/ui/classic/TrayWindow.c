/***************************************************************************
 *   Copyright (C) 2002~2010 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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

#include "fcitx/fcitx.h"

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/extensions/Xrender.h>

#include "TrayWindow.h"
#include "tray.h"
#include "skin.h"
#include "classicui.h"
#include "fcitx-utils/log.h"
#include "fcitx/frontend.h"
#include "fcitx/module.h"
#include "MenuWindow.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"

static boolean TrayEventHandler(void *arg, XEvent* event);

void InitTrayWindow(TrayWindow *trayWindow)
{
    FcitxClassicUI *classicui = trayWindow->owner;
    Display *dpy = classicui->dpy;
    int iScreen = classicui->iScreen;
    char   strWindowName[] = "Fcitx Tray Window";
    if (!classicui->bUseTrayIcon || classicui->isSuspend)
        return;

    InitTray(dpy, trayWindow);

    XVisualInfo* vi = TrayGetVisual(dpy, trayWindow);
    if (vi && vi->visual) {
        Window p = DefaultRootWindow(dpy);
        Colormap colormap = XCreateColormap(dpy, p, vi->visual, AllocNone);
        XSetWindowAttributes wsa;
        wsa.background_pixmap = 0;
        wsa.colormap = colormap;
        wsa.background_pixel = 0;
        wsa.border_pixel = 0;
        trayWindow->window = XCreateWindow(dpy, p, -1, -1, 1, 1,
                                           0, vi->depth, InputOutput, vi->visual,
                                           CWBackPixmap | CWBackPixel | CWBorderPixel | CWColormap, &wsa);
    } else {
        trayWindow->window = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
                             -1, -1, 1, 1, 0,
                             BlackPixel(dpy, DefaultScreen(dpy)),
                             WhitePixel(dpy, DefaultScreen(dpy)));
        XSetWindowBackgroundPixmap(dpy, trayWindow->window, ParentRelative);
    }
    if (trayWindow->window == (Window) NULL)
        return;

    XSizeHints size_hints;
    size_hints.flags = PWinGravity | PBaseSize;
    size_hints.base_width = trayWindow->size;
    size_hints.base_height = trayWindow->size;
    XSetWMNormalHints(dpy, trayWindow->window, &size_hints);

    if (vi && vi->visual)
        trayWindow->cs_x = cairo_xlib_surface_create(dpy, trayWindow->window, trayWindow->visual.visual, 200, 200);
    else {
        Visual *target_visual = DefaultVisual(dpy, iScreen);
        trayWindow->cs_x = cairo_xlib_surface_create(dpy, trayWindow->window, target_visual, 200, 200);
    }
    trayWindow->cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 200, 200);

    XSelectInput(dpy, trayWindow->window, ExposureMask | KeyPressMask |
                 ButtonPressMask | ButtonReleaseMask | StructureNotifyMask
                 | EnterWindowMask | PointerMotionMask | LeaveWindowMask | VisibilityChangeMask);

    ClassicUISetWindowProperty(classicui, trayWindow->window, FCITX_WINDOW_DOCK, strWindowName);

    TrayFindDock(dpy, trayWindow);
}

TrayWindow* CreateTrayWindow(FcitxClassicUI *classicui)
{
    TrayWindow *trayWindow = fcitx_utils_malloc0(sizeof(TrayWindow));
    trayWindow->owner = classicui;
    FcitxX11AddXEventHandler(classicui->owner, TrayEventHandler, trayWindow);
    /* We used to init trayWindow here, but now we should delay it */
    return trayWindow;
}

void ReleaseTrayWindow(TrayWindow *trayWindow)
{
    FcitxClassicUI *classicui = trayWindow->owner;
    Display *dpy = classicui->dpy;
    if (trayWindow->window == None)
        return;
    cairo_surface_destroy(trayWindow->cs);
    cairo_surface_destroy(trayWindow->cs_x);
    XDestroyWindow(dpy, trayWindow->window);
    trayWindow->window = None;
    trayWindow->cs = NULL;
    trayWindow->cs_x = NULL;
    trayWindow->bTrayMapped = false;
}

void DrawTrayWindow(TrayWindow* trayWindow)
{
    FcitxClassicUI *classicui = trayWindow->owner;
    FcitxSkin *sc = &classicui->skin;
    SkinImage *image;
    int f_state;
    if (!classicui->bUseTrayIcon)
        return;

    if (!trayWindow->bTrayMapped)
        return;

    if (FcitxInstanceGetCurrentState(classicui->owner) == IS_ACTIVE)
        f_state = ACTIVE_ICON;
    else
        f_state = INACTIVE_ICON;
    cairo_t *c;
    cairo_surface_t *png_surface;

    /* 画png */
    if (f_state) {
        image = GetIMIcon(classicui, sc, sc->skinTrayIcon.active, 2, true);
    } else {
        image = LoadImage(sc, sc->skinTrayIcon.inactive, true);
    }
    if (image == NULL)
        return;
    /* active, try draw im icon on tray */
    if (f_state && image) {

    }
    png_surface = image->image;

    c = cairo_create(trayWindow->cs);
    cairo_set_source_rgba(c, 0, 0, 0, 0);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_paint(c);

    do {
        if (png_surface) {
            int w = cairo_image_surface_get_width(png_surface);
            int h = cairo_image_surface_get_height(png_surface);
            if (w == 0 || h == 0)
                break;
            double scaleW = 1.0, scaleH = 1.0;
            if (w > trayWindow->size || h > trayWindow->size)
            {
                scaleW = ((double) trayWindow->size) / w;
                scaleH = ((double) trayWindow->size) / h;
                if (scaleW > scaleH)
                    scaleH = scaleW;
                else
                    scaleW = scaleH;
            }
            int aw = scaleW * w;
            int ah = scaleH * h;

            cairo_scale(c, scaleW, scaleH);
            cairo_set_source_surface(c, png_surface, (trayWindow->size - aw) / 2 , (trayWindow->size - ah) / 2);
            cairo_set_operator(c, CAIRO_OPERATOR_OVER);
            cairo_paint_with_alpha(c, 1);
        }
    } while(0);

    cairo_destroy(c);

    XVisualInfo* vi = trayWindow->visual.visual ? &trayWindow->visual : NULL;
    if (!(vi && vi->visual)) {
        XClearArea(trayWindow->owner->dpy, trayWindow->window, 0, 0, trayWindow->size, trayWindow->size, False);
    }

    c = cairo_create(trayWindow->cs_x);
    if (vi && vi->visual) {
        cairo_set_source_rgba(c, 0, 0, 0, 0);
        cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
        cairo_paint(c);
    }
    cairo_set_operator(c, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(c, trayWindow->cs, 0, 0);
    cairo_rectangle(c, 0, 0, trayWindow->size, trayWindow->size);
    cairo_clip(c);
    cairo_paint(c);
    cairo_destroy(c);
    cairo_surface_flush(trayWindow->cs_x);
}

boolean TrayEventHandler(void *arg, XEvent* event)
{
    TrayWindow *trayWindow = arg;
    FcitxClassicUI *classicui = trayWindow->owner;
    FcitxInstance* instance = classicui->owner;
    Display *dpy = classicui->dpy;
    if (!classicui->bUseTrayIcon)
        return false;
    switch (event->type) {
    case ClientMessage:
        if (event->xclient.message_type == trayWindow->atoms[ATOM_MANAGER]
                && event->xclient.data.l[1] == trayWindow->atoms[ATOM_SELECTION]) {
            if (classicui->notificationItemAvailable)
                return true;
            if (trayWindow->window == None)
                InitTrayWindow(trayWindow);
            TrayFindDock(dpy, trayWindow);
            return true;
        }
        break;

    case Expose:
        if (event->xexpose.window == trayWindow->window) {
            DrawTrayWindow(trayWindow);
        }
        break;
    case ConfigureNotify:
        if (trayWindow->window == event->xconfigure.window) {
            int size = event->xconfigure.height;
            if (size != trayWindow->size) {
                trayWindow->size = size;
                XSizeHints size_hints;
                size_hints.flags = PWinGravity | PBaseSize;
                size_hints.base_width = trayWindow->size;
                size_hints.base_height = trayWindow->size;
                XSetWMNormalHints(dpy, trayWindow->window, &size_hints);
            }

            DrawTrayWindow(trayWindow);
            return true;
        }
        break;
    case ButtonPress: {
        if (event->xbutton.window == trayWindow->window) {
            switch (event->xbutton.button) {
            case Button1:
                FcitxInstanceChangeIMState(instance, FcitxInstanceGetCurrentIC(instance));
                break;
            case Button3: {
                XlibMenu *mainMenuWindow = classicui->mainMenuWindow;
                FcitxMenuUpdate(mainMenuWindow->menushell);
                GetMenuSize(mainMenuWindow);
                CalMenuWindowPosition(mainMenuWindow, event->xbutton.x_root - event->xbutton.x, event->xbutton.y_root - event->xbutton.y, trayWindow->size);
                DrawXlibMenu(mainMenuWindow);
                DisplayXlibMenu(mainMenuWindow);
            }
            break;
            }
            return true;
        }
    }
    break;
    case DestroyNotify:
        if (event->xdestroywindow.window == trayWindow->dockWindow) {
            trayWindow->dockWindow = None;
            trayWindow->bTrayMapped = False;
            ReleaseTrayWindow(trayWindow);
            return true;
        }
        break;

    case ReparentNotify:
        if (event->xreparent.parent == DefaultRootWindow(dpy) && event->xreparent.window == trayWindow->window) {
            trayWindow->bTrayMapped = False;
            ReleaseTrayWindow(trayWindow);
            return true;
        }
        break;
    }
    return false;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
