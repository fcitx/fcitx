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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/**
 * @file   InputWindow.h
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 * 
 * @brief  Preedit Window for Input
 * 
 */

#ifndef _INPUT_WINDOW_H
#define _INPUT_WINDOW_H

#include <X11/Xlib.h>
#include <cairo.h>

#define INPUTWND_WIDTH	50
#define INPUTWND_HEIGHT	40
#define INPUT_BAR_MAX_LEN 1500

struct FcitxSkin;
struct FcitxClassicUI;

typedef struct InputWindow {
    Window window;
    
    uint            iInputWindowHeight;
    uint            iInputWindowWidth;
    Bool            bShowPrev;
    Bool            bShowNext;
    Bool            bShowCursor;
    
    //这两个变量是GTK+ OverTheSpot光标跟随的临时解决方案
    ///* Issue 11: piaoairy: 为适应generic_config_integer(), 改int8_t 为int */
    int		iOffsetX;
    int		iOffsetY;
    
    Pixmap pm_input_bar;
    
    cairo_surface_t *cs_input_bar;
    cairo_surface_t *cs_input_back;
    cairo_surface_t *input, *prev, *next;
    cairo_t *c_back, *c_cursor;
    cairo_t *c_font[8];
    Display* dpy;
    int iScreen;
    struct FcitxSkin* skin;
    const char* font;
    struct FcitxClassicUI *owner;
} InputWindow;

InputWindow* CreateInputWindow(struct FcitxClassicUI* classicui);
void MoveInputWindowInternal(InputWindow* inputWindow);
void CloseInputWindowInternal(InputWindow* inputWindow);
void DestroyInputWindow(InputWindow* inputWindow);
void DrawInputWindow(InputWindow* inputWindow);
void ShowInputWindowInternal(InputWindow* inputWindow);

#endif
