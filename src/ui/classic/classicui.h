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
#ifndef CLASSICUI_H
#define CLASSICUI_H

#include "fcitx/fcitx.h"
#include "fcitx/ui.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx-utils/utarray.h"

#include <X11/Xlib.h>
#include <cairo.h>
#include "ui/cairostuff/cairostuff.h"

#ifdef _ENABLE_PANGO
#include <pango/pangocairo.h>
#endif

#include "skin.h"
#include <module/x11/fcitx-x11.h>

struct _MainWindow;
struct _AboutWindow;
struct _FcitxClassicUIStatus;

typedef enum _HIDE_MAINWINDOW {
    HM_SHOW = 0,
    HM_AUTO = 1,
    HM_HIDE = 2
} HIDE_MAINWINDOW;

/**
 * Config and Global State for Classic UI
 **/
typedef struct _FcitxClassicUI {
    FcitxGenericConfig gconfig;
    Display* dpy;
    int iScreen;
    Atom protocolAtom;
    Atom killAtom;
    struct _InputWindow* inputWindow;
    struct _MainWindow* mainWindow;
    struct _MessageWindow* messageWindow;
    struct _TrayWindow* trayWindow;
    FcitxUIMenu skinMenu;

    FcitxSkin skin;
    UT_array skinBuf;
    struct _FcitxInstance *owner;

    int fontSize;
    char* font;
    char* menuFont;
    char* strUserLocale;
    boolean bUseTrayIcon;
    boolean bUseTrayIcon_;
    HIDE_MAINWINDOW hideMainWindow;
    boolean bVerticalList;
    char* skinType;
    int iMainWindowOffsetX;
    int iMainWindowOffsetY;

    UT_array status;
    struct _XlibMenu* mainMenuWindow;
    FcitxUIMenu mainMenu;
    boolean isSuspend;
    boolean isfallback;

    int dpi;
    uint64_t trayTimeout;
    boolean notificationItemAvailable;
} FcitxClassicUI;

void GetScreenSize(FcitxClassicUI* classicui, int* width, int* height);
FcitxRect GetScreenGeometry(FcitxClassicUI* classicui, int x, int y);
void
ClassicUIInitWindowAttribute(FcitxClassicUI* classicui, Visual ** vs, Colormap * cmap,
                             XSetWindowAttributes * attrib,
                             unsigned long *attribmask, int *depth);
Visual * ClassicUIFindARGBVisual(FcitxClassicUI* classicui);
boolean ClassicUIMouseClick(FcitxClassicUI* classicui, Window window, int *x, int *y);
boolean IsInRspArea(int x0, int y0, struct _FcitxClassicUIStatus* status);
void ClassicUISetWindowProperty(FcitxClassicUI* classicui, Window window, FcitxXWindowType type, char *windowTitle);
void ActivateWindow(Display *dpy, int iScreen, Window window);
boolean LoadClassicUIConfig(FcitxClassicUI* classicui);
void SaveClassicUIConfig(FcitxClassicUI* classicui);
boolean WindowIsVisable(Display* dpy, Window window);
boolean EnlargeCairoSurface(cairo_surface_t** sur, int w, int h);
void ResizeSurface(cairo_surface_t** surface, int w, int h);

#define GetPrivateStatus(status) ((FcitxClassicUIStatus*)(status)->uipriv[classicui->isfallback])

CONFIG_BINDING_DECLARE(FcitxClassicUI);
#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
