/***************************************************************************
 *   Copyright (C) 2010 by CSSlayer
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

/* A very simple MessageBox for FCITX */

#ifndef _MESSAGE_WINDOW_H
#define _MESSAGE_WINDOW_H
#include <X11/Xlib.h>
#include <cairo.h>
#include "fcitx-config/fcitx-config.h"

struct _FcitxClassicUI;

typedef struct _MessageWindow {
    Window window;
    cairo_surface_t* surface;
    FcitxConfigColor color;
    FcitxConfigColor fontColor;
    int height, width;
    int fontSize;
    char *title;
    char **msg;
    int length;
    struct _FcitxClassicUI* owner;
} MessageWindow;

MessageWindow* CreateMessageWindow(struct _FcitxClassicUI * classicui);
void DrawMessageWindow(MessageWindow* messageWindow, char *title, char **msg, int length);
#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
