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

#include "ui.h"
#include "core/xim.h"
#include "core/ime.h"
#include "im/pinyin/PYFA.h"
#include "MainWindow.h"
#include "TrayWindow.h"
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>

#define MENU_WINDOW_WIDTH	200
#define MENU_WINDOW_HEIGHT	400

#define IM_MENU_WINDOW_WIDTH	110
#define IM_MENU_WINDOW_HEIGHT	300

#define SKIN_MENU_WINDOW_WIDTH	110
#define SKIN_MENU_WINDOW_HEIGHT	300

#define VK_MENU_WINDOW_WIDTH	110
#define VK_MENU_WINDOW_HEIGHT	300

#define CHAR_COLOR 0x111111
#define CHAR_SELECT_COLOR (0xFFFFFF-0x111111)

//背景色 默认银灰色
#define BG_COLOR 0xDCDCDC
//选中后的背景色 深蓝色
#define BG_SELECT_COLOR 0x0A2465

typedef enum MenuState
{
    MENU_ACTIVE = 0,
    MENU_INACTIVE = 1
} MenuState;

typedef enum
{
	menushell, //暂时只有菜单项和分割线两种类型
	divline
}shellType;

//菜单项属性
typedef	struct 
{
	char tipstr[24];
	int  next;//下一级菜单
	int  isselect;
	shellType type;
}menuShell;

typedef struct
{
	int pos_x;
	int pos_y;
	int width;
	int height;
	Window menuWindow;
	cairo_surface_t *menu_cs;
	int	mark;
	int font_size;
	XColor bgcolor;
	XColor bgselectcolor;
	char font[32];
	XColor charcolor;
	XColor charselectcolor;	
	menuShell shell[16];//暂时用静态的 ,每级支持16给菜单项
	int useItemCount;
}xlibMenu;

extern xlibMenu mainMenu,imMenu,vkMenu,skinMenu;
Bool            CreateMenuWindow (void);
void            InitMenu(void );
void            InitMenuWindowColor (void);
void            DisplayMenuWindow (void);
void            DrawMenuWindow (void);
void GetMenuHeight(Display * dpy,xlibMenu * Menu);

Bool CreateMenuWindow (void);

int CreateXlibMenu(Display * dpy,xlibMenu * Menu);
void DrawXlibMenu(Display * dpy,xlibMenu * Menu);
void DrawDivLine(Display * dpy,xlibMenu * Menu,int line_y);
void DisplayText(Display * dpy,xlibMenu * Menu,int shellindex,int line_y);
int selectShellIndex(xlibMenu * Menu, int x, int y, int* offseth);
void DisplayXlibMenu(Display * dpy,xlibMenu * Menu);
void menuMark(Display * dpy,xlibMenu * Menu,int y,int i);
void clearSelectFlag(xlibMenu * Menu);
void MainMenuEvent(int x,int y);
void IMMenuEvent(int x,int y);
void VKMenuEvent(int x,int y);
void SkinMenuEvent(int x,int y);
void DestroyMenuWindow();

#endif
