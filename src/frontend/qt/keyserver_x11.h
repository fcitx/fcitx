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

static const TransKey g_rgQtToSymX[] = {
    { Qt::Key_Escape,     FcitxKey_Escape },
    { Qt::Key_Tab,        FcitxKey_Tab },
    { Qt::Key_Backtab,    FcitxKey_ISO_Left_Tab },
    { Qt::Key_Backspace,  FcitxKey_BackSpace },
    { Qt::Key_Return,     FcitxKey_Return },
    { Qt::Key_Enter,      FcitxKey_KP_Enter },
    { Qt::Key_Insert,     FcitxKey_Insert },
    { Qt::Key_Delete,     FcitxKey_Delete },
    { Qt::Key_Pause,      FcitxKey_Pause },
#ifdef sun
    { Qt::Key_Print,      FcitxKey_F22 },
#else
    { Qt::Key_Print,      FcitxKey_Print },
#endif
    { Qt::Key_SysReq,     FcitxKey_Sys_Req },
    { Qt::Key_Home,       FcitxKey_Home },
    { Qt::Key_End,        FcitxKey_End },
    { Qt::Key_Left,       FcitxKey_Left },
    { Qt::Key_Up,         FcitxKey_Up },
    { Qt::Key_Right,      FcitxKey_Right },
    { Qt::Key_Down,       FcitxKey_Down },
    //{ Qt::Key_Shift,      0 },
    //{ Qt::Key_Control,    0 },
    //{ Qt::Key_Meta,       0 },
    //{ Qt::Key_Alt,        0 },
    { Qt::Key_CapsLock,   FcitxKey_Caps_Lock },
    { Qt::Key_NumLock,    FcitxKey_Num_Lock },
    { Qt::Key_ScrollLock, FcitxKey_Scroll_Lock },
    { Qt::Key_F1,         FcitxKey_F1 },
    { Qt::Key_F2,         FcitxKey_F2 },
    { Qt::Key_F3,         FcitxKey_F3 },
    { Qt::Key_F4,         FcitxKey_F4 },
    { Qt::Key_F5,         FcitxKey_F5 },
    { Qt::Key_F6,         FcitxKey_F6 },
    { Qt::Key_F7,         FcitxKey_F7 },
    { Qt::Key_F8,         FcitxKey_F8 },
    { Qt::Key_F9,         FcitxKey_F9 },
    { Qt::Key_F10,        FcitxKey_F10 },
    { Qt::Key_F11,        FcitxKey_F11 },
    { Qt::Key_F12,        FcitxKey_F12 },
    { Qt::Key_F13,        FcitxKey_F13 },
    { Qt::Key_F14,        FcitxKey_F14 },
    { Qt::Key_F15,        FcitxKey_F15 },
    { Qt::Key_F16,        FcitxKey_F16 },
    { Qt::Key_F17,        FcitxKey_F17 },
    { Qt::Key_F18,        FcitxKey_F18 },
    { Qt::Key_F19,        FcitxKey_F19 },
    { Qt::Key_F20,        FcitxKey_F20 },
    { Qt::Key_F21,        FcitxKey_F21 },
    { Qt::Key_F22,        FcitxKey_F22 },
    { Qt::Key_F23,        FcitxKey_F23 },
    { Qt::Key_F24,        FcitxKey_F24 },
    { Qt::Key_F25,        FcitxKey_F25 },
    { Qt::Key_F26,        FcitxKey_F26 },
    { Qt::Key_F27,        FcitxKey_F27 },
    { Qt::Key_F28,        FcitxKey_F28 },
    { Qt::Key_F29,        FcitxKey_F29 },
    { Qt::Key_F30,        FcitxKey_F30 },
    { Qt::Key_F31,        FcitxKey_F31 },
    { Qt::Key_F32,        FcitxKey_F32 },
    { Qt::Key_F33,        FcitxKey_F33 },
    { Qt::Key_F34,        FcitxKey_F34 },
    { Qt::Key_F35,        FcitxKey_F35 },
    { Qt::Key_Super_L,    FcitxKey_Super_L },
    { Qt::Key_Super_R,    FcitxKey_Super_R },
    { Qt::Key_Menu,       FcitxKey_Menu },
    { Qt::Key_Hyper_L,    FcitxKey_Hyper_L },
    { Qt::Key_Hyper_R,    FcitxKey_Hyper_R },
    { Qt::Key_Help,       FcitxKey_Help },

    { '/',                FcitxKey_KP_Divide },
    { '*',                FcitxKey_KP_Multiply },
    { '-',                FcitxKey_KP_Subtract },
    { '+',                FcitxKey_KP_Add },
    { Qt::Key_Return,     FcitxKey_KP_Enter },
    {Qt::Key_Multi_key, FcitxKey_Multi_key},
    {Qt::Key_Codeinput, FcitxKey_Codeinput},
    {Qt::Key_SingleCandidate,   FcitxKey_SingleCandidate},
    {Qt::Key_MultipleCandidate, FcitxKey_MultipleCandidate},
    {Qt::Key_PreviousCandidate, FcitxKey_PreviousCandidate},
    {Qt::Key_Mode_switch,   FcitxKey_Mode_switch},
    {Qt::Key_Kanji, FcitxKey_Kanji},
    {Qt::Key_Muhenkan,  FcitxKey_Muhenkan},
    {Qt::Key_Henkan,    FcitxKey_Henkan},
    {Qt::Key_Romaji,    FcitxKey_Romaji},
    {Qt::Key_Hiragana,  FcitxKey_Hiragana},
    {Qt::Key_Katakana,  FcitxKey_Katakana},
    {Qt::Key_Hiragana_Katakana, FcitxKey_Hiragana_Katakana},
    {Qt::Key_Zenkaku,   FcitxKey_Zenkaku},
    {Qt::Key_Hankaku,   FcitxKey_Hankaku},
    {Qt::Key_Zenkaku_Hankaku,   FcitxKey_Zenkaku_Hankaku},
    {Qt::Key_Touroku,   FcitxKey_Touroku},
    {Qt::Key_Massyo,    FcitxKey_Massyo},
    {Qt::Key_Kana_Lock, FcitxKey_Kana_Lock},
    {Qt::Key_Kana_Shift,    FcitxKey_Kana_Shift},
    {Qt::Key_Eisu_Shift,    FcitxKey_Eisu_Shift},
    {Qt::Key_Eisu_toggle,   FcitxKey_Eisu_toggle},
    {Qt::Key_Hangul,    FcitxKey_Hangul},
    {Qt::Key_Hangul_Start,  FcitxKey_Hangul_Start},
    {Qt::Key_Hangul_End,    FcitxKey_Hangul_End},
    {Qt::Key_Hangul_Hanja,  FcitxKey_Hangul_Hanja},
    {Qt::Key_Hangul_Jamo,   FcitxKey_Hangul_Jamo},
    {Qt::Key_Hangul_Romaja, FcitxKey_Hangul_Romaja},
    {Qt::Key_Hangul_Jeonja, FcitxKey_Hangul_Jeonja},
    {Qt::Key_Hangul_Banja,  FcitxKey_Hangul_Banja},
    {Qt::Key_Hangul_PreHanja,   FcitxKey_Hangul_PreHanja},
    {Qt::Key_Hangul_PostHanja,  FcitxKey_Hangul_PostHanja},
    {Qt::Key_Hangul_Special,    FcitxKey_Hangul_Special},
};

#include <qstring.h>

static inline int map_sym_to_qt(uint keySym)
{
    if (keySym < 0x1000) {
        if (keySym >= 'a' && keySym <= 'z')
            return QChar(keySym).toUpper().unicode();
        return keySym;
    }
#ifdef Q_WS_WIN
    if (keySym < 0x3000)
        return keySym;
#else
    if (keySym < 0x3000)
        return keySym | Qt::UNICODE_ACCEL;

    for (uint i = 0; i < sizeof(g_rgQtToSymX) / sizeof(TransKey); i++)
        if (g_rgQtToSymX[i].keySymX == keySym)
            return g_rgQtToSymX[i].keySymQt;
#endif
    return Qt::Key_unknown;
}

static bool symToKeyQt(uint keySym, int& keyQt)
{
    keyQt = map_sym_to_qt(keySym);
    return (keyQt != Qt::Key_unknown);
}

#endif
