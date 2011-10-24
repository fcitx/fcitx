/***************************************************************************
 *   Copyright (C) 2010~2011 by CSSlayer                                   *
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

#ifndef X11STUFF_H
#define X11STUFF_H
#include <X11/Xlib.h>
#include <fcitx-utils/utarray.h>
#include <fcitx-config/fcitx-config.h>

#define FCITX_X11_NAME "fcitx-x11"
#define FCITX_X11_GETDISPLAY 0
#define FCITX_X11_GETDISPLAY_RETURNTYPE Display*
#define FCITX_X11_ADDXEVENTHANDLER 1
#define FCITX_X11_ADDXEVENTHANDLER_RETURNTYPE void
#define FCITX_X11_REMOVEXEVENTHANDLER 2
#define FCITX_X11_REMOVEXEVENTHANDLER_RETURNTYPE void
#define FCITX_X11_FINDARGBVISUAL 3
#define FCITX_X11_FINDARGBVISUAL_RETURNTYPE Visual*
#define FCITX_X11_INITWINDOWATTR 4
#define FCITX_X11_INITWINDOWATTR_RETURNTYPE void
#define FCITX_X11_SETWINDOWPROP 5
#define FCITX_X11_SETWINDOWPROP_RETURNTYPE void
#define FCITX_X11_GETSCREENSIZE 6
#define FCITX_X11_GETSCREENSIZE_RETURNTYPE void
#define FCITX_X11_MOUSECLICK 7
#define FCITX_X11_MOUSECLICK_RETURNTYPE void
#define FCITX_X11_ADDCOMPOSITEHANDLER 8
#define FCITX_X11_ADDCOMPOSITEHANDLER_RETURNTYPE void
#define FCITX_X11_GETSCREENGEOMETRY 9
#define FCITX_X11_GETSCREENGEOMETRY_RETURNTYPE void


struct _FcitxInstance;

typedef struct _FcitxXEventHandler {
    boolean(*eventHandler)(void* instance, XEvent* event);
    void* instance;
} FcitxXEventHandler;

typedef struct _FcitxCompositeChangedHandler {
    void (*eventHandler)(void* instance, boolean enable);
    void *instance;
} FcitxCompositeChangedHandler;

typedef struct _FcitxRect {
    int x1, y1, x2, y2;
} FcitxRect;

typedef struct _FcitxX11 {
    Display *dpy;
    UT_array handlers;
    UT_array comphandlers;
    struct _FcitxInstance* owner;
    Window compManager;
    Atom compManagerAtom;
    int iScreen;
    Atom typeMenuAtom;
    Atom windowTypeAtom;
    Atom typeDialogAtom;
    Atom typeDockAtom;
    Atom pidAtom;
    boolean bUseXinerama;
    FcitxRect* rects;
    int screenCount;
    int defaultScreen;
} FcitxX11;

typedef enum _FcitxXWindowType {
    FCITX_WINDOW_UNKNOWN,
    FCITX_WINDOW_DOCK,
    FCITX_WINDOW_MENU,
    FCITX_WINDOW_DIALOG
} FcitxXWindowType;

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
