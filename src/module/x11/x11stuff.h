/***************************************************************************
 *   Copyright (C) 2010~2012 by CSSlayer                                   *
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
#include <stdint.h>
#include <fcitx-utils/utarray.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx/instance.h>
#include <fcitx/addon.h>

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
#define FCITX_X11_PROCESSREMAINEVENT 10
#define FCITX_X11_PROCESSREMAINEVENT_RETURNTYPE void
#define FCITX_X11_GETDPI 11
#define FCITX_X11_GETDPI_RETURNTYPE void
#define FCITX_X11_REG_SELECT_NOTIFY 12
#define FCITX_X11_REG_SELECT_NOTIFY_RETURNTYPE intptr_t
#define FCITX_X11_REMOVE_SELECT_NOTIFY 13
#define FCITX_X11_REMOVE_SELECT_NOTIFY_RETURNTYPE void

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

typedef enum _FcitxXWindowType {
    FCITX_WINDOW_UNKNOWN,
    FCITX_WINDOW_DOCK,
    FCITX_WINDOW_MENU,
    FCITX_WINDOW_DIALOG
} FcitxXWindowType;

typedef void (*FcitxDestroyNotify)(void*);
// typedef void (*X11ConvertSelectionCallback)(void *owner, const char *selection,
//                                             int subtype, void *data);
typedef void (*X11SelectionNotifyCallback)(void *owner, const char *sel_str,
                                           int subtype, void *data);

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
