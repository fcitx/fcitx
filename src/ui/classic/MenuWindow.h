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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/* 鼠标点击logo图标的时候产生的窗口 */

#ifndef _MENUWINDOW_H_
#define _MENUWINDOW_H_

#define MENU_WINDOW_WIDTH   200
#define MENU_WINDOW_HEIGHT  400

#define IM_MENU_WINDOW_WIDTH    110
#define IM_MENU_WINDOW_HEIGHT   300

#define SKIN_MENU_WINDOW_WIDTH  110
#define SKIN_MENU_WINDOW_HEIGHT 300

#define VK_MENU_WINDOW_WIDTH    110
#define VK_MENU_WINDOW_HEIGHT   300
#include <X11/Xlib.h>
#include "fcitx-utils/utarray.h"
#include <cairo.h>
#include <fcitx-config/fcitx-config.h>

struct FcitxClassicUI;

typedef struct XlibMenu
{
    int iPosX;
    int iPosY;
    int width;
    int height;
    Window menuWindow;
    Pixmap pixmap;
    cairo_surface_t *menu_cs;
    int mark;
    int font_size;
    XColor bgcolor;
    XColor bgselectcolor;
    char font[32];
    XColor charcolor;
    XColor charselectcolor;
    struct FcitxClassicUI* owner;
} XlibMenu;

boolean CreateMenuWindow (void);
void InitMenuWindowColor (void);
void DisplayMenuWindow (void);
void DrawMenuWindow (void);
void GetMenuSize(XlibMenu* menu);

boolean CreateMenuWindow (void);

XlibMenu* CreateXlibMenu(struct FcitxClassicUI* classicui);
void DrawXlibMenu(XlibMenu* menu);
void DrawDivLine(XlibMenu* menu, int line_y);
void DisplayText(XlibMenu* menu, int shellindex, int line_y);
int SelectShellIndex(XlibMenu * Menu, int x, int y, int* offseth);
void DisplayXlibMenu(XlibMenu* menu);
void ClearSelectFlag(XlibMenu * Menu);
void MainMenuEvent(int x,int y);
void IMMenuEvent(int x,int y);
void VKMenuEvent(int x,int y);
void SkinMenuEvent(int x,int y);
void DestroyMenuWindow();

#endif
