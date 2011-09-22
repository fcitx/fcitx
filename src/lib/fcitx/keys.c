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

#include "fcitx/fcitx.h"
#include "fcitx/keys.h"

FCITX_EXPORT_API
HOTKEYS FCITX_NONE_KEY[2] = {
    {NULL, Key_None, 0},
    {NULL, Key_None, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_DELETE[2] = {
    {NULL, Key_Delete, 0},
    {NULL, Key_KP_Delete, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_CTRL_DELETE[2] = {
    {NULL, Key_Delete, KEY_CTRL_COMP},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_BACKSPACE[2] = {
    {NULL, Key_BackSpace, 0},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_HOME[2] = {
    {NULL, Key_Home, 0},
    {NULL, Key_KP_Home, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_END[2] = {
    {NULL, Key_End, 0},
    {NULL, Key_KP_End, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_RIGHT[2] = {
    {NULL, Key_Right, 0},
    {NULL, Key_KP_Right, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_LEFT[2] = {
    {NULL, Key_Left, 0},
    {NULL, Key_KP_Left, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_ESCAPE[2] = {
    {NULL, Key_Escape, 0},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_ENTER[2] = {
    {NULL, Key_Return, 0},
    {NULL, Key_KP_Enter, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_LCTRL_LSHIFT[2] = {
    {NULL, Key_Shift_L, KEY_CTRL_SHIFT_COMP},
    {NULL, Key_Control_L, KEY_CTRL_SHIFT_COMP},
};

FCITX_EXPORT_API
HOTKEYS FCITX_LCTRL_LSHIFT2[2] = {
    {NULL, Key_Shift_L, KEY_CTRL_COMP},
    {NULL, 0, 0},
};


FCITX_EXPORT_API
HOTKEYS FCITX_SEMICOLON[2] = {
    {NULL, Key_semicolon, KEY_NONE},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_SEPARATOR[2] = {
    {NULL, Key_apostrophe, KEY_NONE},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_COMMA[2] = {
    {NULL, Key_comma, KEY_NONE},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_PERIOD[2] = {
    {NULL, Key_period, KEY_NONE},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_SPACE[2] = {
    {NULL, Key_space, KEY_NONE},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_CTRL_5[2] = {
    {NULL, Key_5, KEY_CTRL_COMP},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_CTRL_ALT_E[2] = {
    {NULL, Key_E, KEY_CTRL_ALT_COMP},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
HOTKEYS FCITX_LCTRL[2] = {
    {NULL, Key_Control_L, KEY_NONE},
    {NULL, Key_Control_L, KEY_CTRL_COMP},
};

FCITX_EXPORT_API
HOTKEYS FCITX_LSHIFT[2] = {
    {NULL, Key_Shift_L, KEY_NONE},
    {NULL, Key_Shift_L, KEY_SHIFT_COMP},
};

FCITX_EXPORT_API
HOTKEYS FCITX_RCTRL[2] = {
    {NULL, Key_Control_R, KEY_NONE},
    {NULL, Key_Control_R, KEY_CTRL_COMP},
};

FCITX_EXPORT_API
HOTKEYS FCITX_RSHIFT[2] = {
    {NULL, Key_Shift_R, KEY_NONE},
    {NULL, Key_Shift_R, KEY_SHIFT_COMP},
};

FCITX_EXPORT_API
boolean IsHotKeyModifierCombine(FcitxKeySym sym, int state)
{
    if (sym == Key_Control_L
            || sym == Key_Control_R
            || sym == Key_Shift_L
            || sym == Key_Shift_R)
        return true;

    return false;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
