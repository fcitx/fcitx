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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

/*
 * Define const keys that will be used in code
 */

#include "fcitx/fcitx.h"
#include "fcitx/keys.h"

FCITX_EXPORT_API
FcitxHotkey FCITX_NONE_KEY[2] = {
    {NULL, FcitxKey_None, 0},
    {NULL, FcitxKey_None, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_DELETE[2] = {
    {NULL, FcitxKey_Delete, 0},
    {NULL, FcitxKey_KP_Delete, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_CTRL_DELETE[2] = {
    {NULL, FcitxKey_Delete, FcitxKeyState_Ctrl},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_BACKSPACE[2] = {
    {NULL, FcitxKey_BackSpace, 0},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_HOME[2] = {
    {NULL, FcitxKey_Home, 0},
    {NULL, FcitxKey_KP_Home, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_END[2] = {
    {NULL, FcitxKey_End, 0},
    {NULL, FcitxKey_KP_End, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RIGHT[2] = {
    {NULL, FcitxKey_Right, 0},
    {NULL, FcitxKey_KP_Right, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LEFT[2] = {
    {NULL, FcitxKey_Left, 0},
    {NULL, FcitxKey_KP_Left, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_ESCAPE[2] = {
    {NULL, FcitxKey_Escape, 0},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_ENTER[2] = {
    {NULL, FcitxKey_Return, 0},
    {NULL, FcitxKey_KP_Enter, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LCTRL_LSHIFT[2] = {
    {NULL, FcitxKey_Shift_L, FcitxKeyState_Ctrl_Shift},
    {NULL, FcitxKey_Control_L, FcitxKeyState_Ctrl_Shift},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LCTRL_LSHIFT2[2] = {
    {NULL, FcitxKey_Shift_L, FcitxKeyState_Ctrl},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RCTRL_RSHIFT[2] = {
    {NULL, FcitxKey_Shift_R, FcitxKeyState_Ctrl_Shift},
    {NULL, FcitxKey_Control_R, FcitxKeyState_Ctrl_Shift},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RCTRL_RSHIFT2[2] = {
    {NULL, FcitxKey_Shift_R, FcitxKeyState_Ctrl},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LALT_LSHIFT[2] = {
    {NULL, FcitxKey_Shift_L, FcitxKeyState_Alt_Shift},
    {NULL, FcitxKey_Alt_L, FcitxKeyState_Alt_Shift},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LALT_LSHIFT2[2] = {
    {NULL, FcitxKey_Shift_L, FcitxKeyState_Alt},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RALT_RSHIFT[2] = {
    {NULL, FcitxKey_Shift_R, FcitxKeyState_Alt_Shift},
    {NULL, FcitxKey_Alt_R, FcitxKeyState_Alt_Shift},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RALT_RSHIFT2[2] = {
    {NULL, FcitxKey_Shift_R, FcitxKeyState_Alt},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LCTRL_LSUPER[2] = {
    {NULL, FcitxKey_Super_L, FcitxKeyState_Ctrl | FcitxKeyState_Super},
    {NULL, FcitxKey_Control_L, FcitxKeyState_Ctrl | FcitxKeyState_Super},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LCTRL_LSUPER2[2] = {
    {NULL, FcitxKey_Super_L, FcitxKeyState_Ctrl},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RCTRL_RSUPER[2] = {
    {NULL, FcitxKey_Super_R, FcitxKeyState_Ctrl | FcitxKeyState_Super},
    {NULL, FcitxKey_Control_R, FcitxKeyState_Ctrl | FcitxKeyState_Super},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RCTRL_RSUPER2[2] = {
    {NULL, FcitxKey_Super_R, FcitxKeyState_Ctrl},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LALT_LSUPER[2] = {
    {NULL, FcitxKey_Super_L, FcitxKeyState_Alt | FcitxKeyState_Super},
    {NULL, FcitxKey_Alt_L, FcitxKeyState_Alt | FcitxKeyState_Super},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LALT_LSUPER2[2] = {
    {NULL, FcitxKey_Super_L, FcitxKeyState_Alt},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RALT_RSUPER[2] = {
    {NULL, FcitxKey_Super_R, FcitxKeyState_Alt | FcitxKeyState_Super},
    {NULL, FcitxKey_Alt_R, FcitxKeyState_Alt | FcitxKeyState_Super},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RALT_RSUPER2[2] = {
    {NULL, FcitxKey_Super_R, FcitxKeyState_Alt},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_SEMICOLON[2] = {
    {NULL, FcitxKey_semicolon, FcitxKeyState_None},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_SPACE[2] = {
    {NULL, FcitxKey_space, FcitxKeyState_None},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_SHIFT_SPACE[2] = {
    {NULL, FcitxKey_space, FcitxKeyState_Shift},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_SHIFT_ENTER[2] = {
    {NULL, FcitxKey_Return, FcitxKeyState_Shift},
    {NULL, FcitxKey_KP_Enter, FcitxKeyState_Shift},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_TAB[2] = {
    {NULL, FcitxKey_Tab, FcitxKeyState_None},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_COMMA[2] = {
    {NULL, FcitxKey_comma, FcitxKeyState_None},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_PERIOD[2] = {
    {NULL, FcitxKey_period, FcitxKeyState_None},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_CTRL_5[2] = {
    {NULL, FcitxKey_5, FcitxKeyState_Ctrl},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_SEPARATOR[2] = {
    {NULL, FcitxKey_apostrophe, FcitxKeyState_None},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_CTRL_ALT_E[2] = {
    {NULL, FcitxKey_E, FcitxKeyState_Ctrl_Alt},
    {NULL, 0, 0},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LCTRL[2] = {
    {NULL, FcitxKey_Control_L, FcitxKeyState_None},
    {NULL, FcitxKey_Control_L, FcitxKeyState_Ctrl},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LSHIFT[2] = {
    {NULL, FcitxKey_Shift_L, FcitxKeyState_None},
    {NULL, FcitxKey_Shift_L, FcitxKeyState_Shift},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RCTRL[2] = {
    {NULL, FcitxKey_Control_R, FcitxKeyState_None},
    {NULL, FcitxKey_Control_R, FcitxKeyState_Ctrl},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RSHIFT[2] = {
    {NULL, FcitxKey_Shift_R, FcitxKeyState_None},
    {NULL, FcitxKey_Shift_R, FcitxKeyState_Shift},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_ALT_LSHIFT[2] = {
    {NULL, FcitxKey_Shift_L, FcitxKeyState_Alt_Shift},
    {NULL, FcitxKey_Shift_L, FcitxKeyState_Alt},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_ALT_RSHIFT[2] = {
    {NULL, FcitxKey_Shift_R, FcitxKeyState_Alt_Shift},
    {NULL, FcitxKey_Shift_R, FcitxKeyState_Alt},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LALT[2] = {
    {NULL, FcitxKey_Alt_L, FcitxKeyState_None},
    {NULL, FcitxKey_Alt_L, FcitxKeyState_Alt},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RALT[2] = {
    {NULL, FcitxKey_Alt_R, FcitxKeyState_None},
    {NULL, FcitxKey_Alt_R, FcitxKeyState_Alt},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_LSUPER[2] = {
    {NULL, FcitxKey_Super_L, FcitxKeyState_None},
    {NULL, FcitxKey_Super_L, FcitxKeyState_Super},
};

FCITX_EXPORT_API
FcitxHotkey FCITX_RSUPER[2] = {
    {NULL, FcitxKey_Super_R, FcitxKeyState_None},
    {NULL, FcitxKey_Super_R, FcitxKeyState_Super},
};

// kate: indent-mode cstyle; space-indent on; indent-width 0;
