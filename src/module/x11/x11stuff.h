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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef X11STUFF_H
#define X11STUFF_H
#include <X11/Xlib.h>
#include "fcitx-utils/utarray.h"

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

struct FcitxInstance;

typedef struct FcitxXEventHandler {
    boolean (*eventHandler)(void* instance, XEvent* event);
    void* instance;
} FcitxXEventHandler;


typedef struct FcitxX11 {
    Display *dpy;
    UT_array handlers;
    struct FcitxInstance* owner;
    Window compManager;
    Atom compManagerAtom;
    int iScreen;
    Atom typeMenuAtom;
    Atom windowTypeAtom;
    Atom typeDialogAtom;
    Atom typeDockAtom;
    Atom pidAtom;
} FcitxX11;

typedef enum FcitxXWindowType {
    FCITX_WINDOW_UNKNOWN,
    FCITX_WINDOW_DOCK,
    FCITX_WINDOW_MENU,
    FCITX_WINDOW_DIALOG
} FcitxXWindowType;

#endif