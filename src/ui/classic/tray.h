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

#include "fcitx/fcitx.h"

#ifndef _TRAY_H_
#define _TRAY_H_

#include "TrayWindow.h"

int InitTray(Display* dpy, TrayWindow* win);
void TrayHandleClientMessage(Display *dpy, Window win, XEvent *an_event);
int TrayFindDock(Display *dpy, TrayWindow* tray);
XVisualInfo* TrayGetVisual(Display* dpy, TrayWindow* tray);
Window TrayGetDock(Display* dpy, TrayWindow* tray);
void TraySendOpcode( Display* dpy, Window dock,  TrayWindow* tray, long message,
                     long data1, long data2, long data3 );

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define TRAY_ICON_WIDTH 22
#define TRAY_ICON_HEIGHT 22

#define ATOM_SELECTION 0
#define ATOM_MANAGER 1
#define ATOM_SYSTEM_TRAY_OPCODE 2
#define ATOM_ORIENTATION 3
#define ATOM_VISUAL 4

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0; 
