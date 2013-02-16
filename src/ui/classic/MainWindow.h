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
/**
 * @file   MainWindow.h
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 *  主窗口
 *
 *
 */

#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H
#include <X11/Xlib.h>
#include <cairo.h>
#include "fcitx-config/fcitx-config.h"
#include "classicui.h"
#include "XlibWindow.h"

struct _FcitxSkin;
struct _FcitxClassicUI;

typedef struct _FcitxClassicUIStatus {
    MouseE mouse;
    int x, y;
    int w, h;
    boolean avail;
} FcitxClassicUIStatus;

typedef struct _MainWindow {
    FcitxXlibWindow parent;
    FcitxClassicUIStatus logostat;
    FcitxClassicUIStatus imiconstat;
} MainWindow;

MainWindow* MainWindowCreate(FcitxClassicUI* classicui);
void MainWindowClose(MainWindow *mainWindow);
void MainWindowShow(MainWindow* mainWindow);
boolean MainWindowSetMouseStatus(MainWindow *mainWindow, MouseE* mouseE, MouseE value, MouseE other);

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
