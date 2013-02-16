/***************************************************************************
 *   Copyright (C) 2009~2010 by t3swing                                    *
 *   t3swing@sina.com                                                      *
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
/* 鼠标点击logo图标的时候产生的窗口 */

#ifndef _MENUWINDOW_H_
#define _MENUWINDOW_H_

#include <X11/Xlib.h>
#include <cairo.h>
#include "fcitx-utils/utarray.h"
#include "fcitx-config/fcitx-config.h"
#include "classicui.h"
#include "XlibWindow.h"

struct _FcitxClassicUI;
struct _FcitxUIMenu;

typedef enum _MenuAnchor {
    MA_None,
    MA_MainWindow,
    MA_Menu,
    MA_Tray
} MenuAnchor;

typedef struct _XlibMenu {
    FcitxXlibWindow parent;
    int iPosX;
    int iPosY;
    struct _FcitxUIMenu *menushell;
    int fontheight;

    MenuAnchor anchor;
    struct _XlibMenu* anchorMenu;
    int offseth;
    boolean visible;
    int trayY;
    int trayX;
} XlibMenu;

XlibMenu* MainMenuWindowCreate(struct _FcitxClassicUI *classicui);
XlibMenu* XlibMenuCreate(struct _FcitxClassicUI* classicui);
void XlibMenuShow(XlibMenu * menu);
void CalMenuWindowPosition(XlibMenu *menu, int x, int y, int dodgeHeight);

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
