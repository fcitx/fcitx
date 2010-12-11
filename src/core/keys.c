/***************************************************************************
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

/*
 * Define const keys that will be used in code
 */

#include "core/keys.h"

HOTKEYS FCITX_DELETE[2] =
{
    {NULL, XK_Delete, 0},
    {NULL, 0, 0},
};

HOTKEYS FCITX_CTRL_DELETE[2] =
{
    {NULL, XK_Delete, KEY_CTRL_COMP},
    {NULL, 0, 0},
};

HOTKEYS FCITX_BACKSPACE[2] =
{
    {NULL, XK_BackSpace, 0},
    {NULL, 0, 0},
};

HOTKEYS FCITX_CTRL_H[2] =
{
    {NULL, XK_H, KEY_CTRL_COMP},
    {NULL, 0, 0},
};

HOTKEYS FCITX_HOME[2] =
{
    {NULL, XK_Home, 0},
    {NULL, 0, 0},
};

HOTKEYS FCITX_END[2] =
{
    {NULL, XK_End, 0},
    {NULL, 0, 0},
};

HOTKEYS FCITX_RIGHT[2] =
{
    {NULL, XK_Right, 0},
    {NULL, 0, 0},
};

HOTKEYS FCITX_LEFT[2] =
{
    {NULL, XK_Left, 0},
    {NULL, 0, 0},
};

HOTKEYS FCITX_ESCAPE[2] =
{
    {NULL, XK_Escape, 0},
    {NULL, 0, 0},
};

HOTKEYS FCITX_ENTER[2] =
{
    {NULL, XK_Return, 0},
    {NULL, 0, 0},
};

HOTKEYS FCITX_LCTRL_LSHIFT[2] =
{
    {NULL, XK_Shift_L, KEY_CTRL_SHIFT_COMP},
    {NULL, XK_Control_L, KEY_CTRL_SHIFT_COMP},
};

HOTKEYS FCITX_SEMICOLON[2] = 
{
    {NULL, XK_semicolon, KEY_NONE},
    {NULL, 0, 0},
};

HOTKEYS FCITX_SEPARATOR[2] = 
{
    {NULL, XK_apostrophe, KEY_NONE},
    {NULL, 0, 0},
};

HOTKEYS FCITX_COMMA[2] = 
{
    {NULL, XK_comma, KEY_NONE},
    {NULL, 0, 0},
};

HOTKEYS FCITX_PERIOD[2] = 
{
    {NULL, XK_period, KEY_NONE},
    {NULL, 0, 0},
};

HOTKEYS FCITX_SPACE[2] = 
{
    {NULL, XK_space, KEY_NONE},
    {NULL, 0, 0},
};

HOTKEYS FCITX_CTRL_5[2] = 
{
    {NULL, XK_5, KEY_CTRL_COMP},
    {NULL, 0, 0},
};

Bool IsHotKeyModifierCombine(KeySym sym, int state)
{
   if (sym == XK_Control_L
    || sym == XK_Control_R
    || sym == XK_Shift_L
    || sym == XK_Shift_R )
       return True;

   return False;
}
