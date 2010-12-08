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
#include <X11/keysym.h>

#include "fcitx-config/hotkey.h"

Bool IsHotKeyModifierCombine(KeySym sym, int state);

extern HOTKEYS FCITX_DELETE[2];
extern HOTKEYS FCITX_CTRL_DELETE[2];
extern HOTKEYS FCITX_BACKSPACE[2];
extern HOTKEYS FCITX_CTRL_H[2];
extern HOTKEYS FCITX_HOME[2];
extern HOTKEYS FCITX_END[2];
extern HOTKEYS FCITX_RIGHT[2];
extern HOTKEYS FCITX_LEFT[2];
extern HOTKEYS FCITX_ESCAPE[2];
extern HOTKEYS FCITX_ENTER[2];
extern HOTKEYS FCITX_LCTRL_LSHIFT[2];
extern HOTKEYS FCITX_SEMICOLON[2];
extern HOTKEYS FCITX_SPACE[2];
extern HOTKEYS FCITX_COMMA[2];
extern HOTKEYS FCITX_PERIOD[2];
extern HOTKEYS FCITX_CTRL_5[2];
extern HOTKEYS FCITX_SEPARATOR[2];
