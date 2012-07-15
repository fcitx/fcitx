/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#ifndef FCITX_UTILS_KEYSYM_H
#define FCITX_UTILS_KEYSYM_H

#include <fcitx-utils/keysymgen.h>

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * fcitx key state (modifier keys)
     **/
    typedef enum _FcitxKeyState {
        FcitxKeyState_None = 0,
        FcitxKeyState_Shift = 1 << 0,
        FcitxKeyState_CapsLock = 1 << 1,
        FcitxKeyState_Ctrl = 1 << 2,
        FcitxKeyState_Alt = 1 << 3,
        FcitxKeyState_Alt_Shift = FcitxKeyState_Alt | FcitxKeyState_Shift,
        FcitxKeyState_Ctrl_Shift = FcitxKeyState_Ctrl | FcitxKeyState_Shift,
        FcitxKeyState_Ctrl_Alt = FcitxKeyState_Ctrl | FcitxKeyState_Alt,
        FcitxKeyState_Ctrl_Alt_Shift = FcitxKeyState_Ctrl | FcitxKeyState_Alt | FcitxKeyState_Shift,
        FcitxKeyState_NumLock = 1 << 4,
        FcitxKeyState_Super = 1 << 6,
        FcitxKeyState_ScrollLock = 1 << 7,
        FcitxKeyState_MousePressed = 1 << 8,
        FcitxKeyState_HandledMask = 1 << 24,
        FcitxKeyState_IgnoredMask = 1 << 25,
        FcitxKeyState_Super2    = 1 << 26,
        FcitxKeyState_Hyper    = 1 << 27,
        FcitxKeyState_Meta     = 1 << 28,
        FcitxKeyState_UsedMask = 0x5c001fff,
        FcitxKeyState_SimpleMask = FcitxKeyState_Ctrl_Alt_Shift | FcitxKeyState_Super | FcitxKeyState_Super2 | FcitxKeyState_Hyper | FcitxKeyState_Meta,
    } FcitxKeyState;


#ifdef __cplusplus
}
#endif

#endif
