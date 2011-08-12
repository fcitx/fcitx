/*
    Copyright (C) 2001 Ellis Whitehead <ellis@kde.org>

    Win32 port:
    Copyright (C) 2004 Jaroslaw Staniek <js@iidea.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 */

#ifndef KEYSERVER_X11_H
#define KEYSERVER_X11_H

#include "fcitx-config/hotkey.h"

struct TransKey {
        int keySymQt;
        FcitxKeySym keySymX;
};

static const TransKey g_rgQtToSymX[] =
{
        { Qt::Key_Escape,     Key_Escape },
        { Qt::Key_Tab,        Key_Tab },
        { Qt::Key_Backtab,    Key_ISO_Left_Tab },
        { Qt::Key_Backspace,  Key_BackSpace },
        { Qt::Key_Return,     Key_Return },
        { Qt::Key_Enter,      Key_KP_Enter },
        { Qt::Key_Insert,     Key_Insert },
        { Qt::Key_Delete,     Key_Delete },
        { Qt::Key_Pause,      Key_Pause },
#ifdef sun
        { Qt::Key_Print,      Key_F22 },
#else
        { Qt::Key_Print,      Key_Print },
#endif
        { Qt::Key_SysReq,     Key_Sys_Req },
        { Qt::Key_Home,       Key_Home },
        { Qt::Key_End,        Key_End },
        { Qt::Key_Left,       Key_Left },
        { Qt::Key_Up,         Key_Up },
        { Qt::Key_Right,      Key_Right },
        { Qt::Key_Down,       Key_Down },
        //{ Qt::Key_Shift,      0 },
        //{ Qt::Key_Control,    0 },
        //{ Qt::Key_Meta,       0 },
        //{ Qt::Key_Alt,        0 },
        { Qt::Key_CapsLock,   Key_Caps_Lock },
        { Qt::Key_NumLock,    Key_Num_Lock },
        { Qt::Key_ScrollLock, Key_Scroll_Lock },
        { Qt::Key_F1,         Key_F1 },
        { Qt::Key_F2,         Key_F2 },
        { Qt::Key_F3,         Key_F3 },
        { Qt::Key_F4,         Key_F4 },
        { Qt::Key_F5,         Key_F5 },
        { Qt::Key_F6,         Key_F6 },
        { Qt::Key_F7,         Key_F7 },
        { Qt::Key_F8,         Key_F8 },
        { Qt::Key_F9,         Key_F9 },
        { Qt::Key_F10,        Key_F10 },
        { Qt::Key_F11,        Key_F11 },
        { Qt::Key_F12,        Key_F12 },
        { Qt::Key_F13,        Key_F13 },
        { Qt::Key_F14,        Key_F14 },
        { Qt::Key_F15,        Key_F15 },
        { Qt::Key_F16,        Key_F16 },
        { Qt::Key_F17,        Key_F17 },
        { Qt::Key_F18,        Key_F18 },
        { Qt::Key_F19,        Key_F19 },
        { Qt::Key_F20,        Key_F20 },
        { Qt::Key_F21,        Key_F21 },
        { Qt::Key_F22,        Key_F22 },
        { Qt::Key_F23,        Key_F23 },
        { Qt::Key_F24,        Key_F24 },
        { Qt::Key_F25,        Key_F25 },
        { Qt::Key_F26,        Key_F26 },
        { Qt::Key_F27,        Key_F27 },
        { Qt::Key_F28,        Key_F28 },
        { Qt::Key_F29,        Key_F29 },
        { Qt::Key_F30,        Key_F30 },
        { Qt::Key_F31,        Key_F31 },
        { Qt::Key_F32,        Key_F32 },
        { Qt::Key_F33,        Key_F33 },
        { Qt::Key_F34,        Key_F34 },
        { Qt::Key_F35,        Key_F35 },
        { Qt::Key_Super_L,    Key_Super_L },
        { Qt::Key_Super_R,    Key_Super_R },
        { Qt::Key_Menu,       Key_Menu },
        { Qt::Key_Hyper_L,    Key_Hyper_L },
        { Qt::Key_Hyper_R,    Key_Hyper_R },
        { Qt::Key_Help,       Key_Help },

        { '/',                Key_KP_Divide },
        { '*',                Key_KP_Multiply },
        { '-',                Key_KP_Subtract },
        { '+',                Key_KP_Add },
        { Qt::Key_Return,     Key_KP_Enter }
};

#include <qstring.h> 

inline int map_sym_to_qt(uint keySym)
{
    if( keySym < 0x1000 ) {
        if( keySym >= 'a' && keySym <= 'z' )
            return QChar(keySym).toUpper().unicode();
        return keySym;
    }
#ifdef Q_WS_WIN
        if( keySym < 0x3000 )
                return keySym;
#else
        if( keySym < 0x3000 )
                return keySym | Qt::UNICODE_ACCEL;

        for( uint i = 0; i < sizeof(g_rgQtToSymX)/sizeof(TransKey); i++ )
            if( g_rgQtToSymX[i].keySymX == keySym )
                return g_rgQtToSymX[i].keySymQt;
#endif
        return Qt::Key_unknown;
}

static bool symToKeyQt( uint keySym, int& keyQt )
{
    keyQt = map_sym_to_qt(keySym);
    return (keyQt != Qt::Key_unknown);
}

#endif
