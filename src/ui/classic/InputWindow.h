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
 * @file   InputWindow.h
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 *  Preedit Window for Input
 *
 */

#ifndef _INPUT_WINDOW_H
#define _INPUT_WINDOW_H

#include <X11/Xlib.h>
#include <cairo.h>

#include "fcitx/fcitx.h"
#include "classicui.h"
#include "XlibWindow.h"

#define ROUND_SIZE 80
#define INPUTWND_WIDTH  50
#define INPUTWND_HEIGHT 40
#define INPUT_BAR_HMIN_WIDTH ROUND_SIZE
#define INPUT_BAR_VMIN_WIDTH 160
#define INPUT_BAR_MAX_WIDTH 1000
#define INPUT_BAR_MAX_HEIGHT 300

struct _FcitxSkin;
struct _FcitxClassicUI;

typedef struct _InputWindow {
    FcitxXlibWindow parent;

    //这两个变量是GTK+ OverTheSpot光标跟随的临时解决方案
    int     iOffsetX;
    int     iOffsetY;

    FcitxMessages* msgUp;
    FcitxMessages* msgDown;
    int cursorPos;
    boolean vertical;

    /* cached data */
    char *strUp[MAX_MESSAGE_COUNT];
    char *strDown[MAX_MESSAGE_COUNT];
    int posUpX[MAX_MESSAGE_COUNT], posUpY[MAX_MESSAGE_COUNT];
    FcitxRect candRect[10];
    int posDownX[MAX_MESSAGE_COUNT], posDownY[MAX_MESSAGE_COUNT];
    int fontHeight;
    int pixelCursorPos;
    FcitxRect prevRect, nextRect;
    uint32_t highlight;
} InputWindow;

InputWindow* InputWindowCreate(struct _FcitxClassicUI* classicui);
void InputWindowClose(InputWindow* inputWindow);
void InputWindowShow(InputWindow* inputWindow);

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
