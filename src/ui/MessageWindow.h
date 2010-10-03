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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* A very simple MessageBox for FCITX */

#ifndef _MESSAGE_WINDOW_H
#define _MESSAGE_WINDOW_H

#include <X11/Xlib.h>
#include "ui/skin.h"

typedef struct MessageWindow
{
    Window window;
    cairo_surface_t* surface;
    ConfigColor color;
    ConfigColor fontColor;
    int height, width;
    int fontSize;
    char *title;
    char **msg;
    int length;
} MessageWindow;

extern MessageWindow messageWindow;

Bool            CreateMessageWindow (void);
void            DisplayMessageWindow (void);
void            DrawMessageWindow (char *title, char **msg, int length);

#endif
