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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifndef _TRAY_WINDOW_H
#define _TRAY_WINDOW_H

#ifdef _ENABLE_TRAY

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <string.h>
#include <cairo.h>
#include <cairo-xlib.h>

#define INACTIVE_ICON 0
#define ACTIVE_ICON   1

typedef struct TrayWindow {
    Window window;

    XImage* icon[2];
    Pixmap picon[2];
    GC gc;
    Bool bTrayMapped;
    XVisualInfo visual;
    Atom atoms[6];

    cairo_surface_t *cs;
    int size;
} TrayWindow;

Bool CreateTrayWindow();
void DrawTrayWindow(int f_state, int x, int y, int w, int h);
void DeInitTrayWindow(TrayWindow *f_tray);
void RedrawTrayWindow(void);
void TrayEventHandler(XEvent* event);

extern TrayWindow tray;

#endif

#endif
