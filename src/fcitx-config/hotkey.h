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
 * @file   hotkey.h
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 * 
 * @brief  键盘扫描码列表
 * 
 * 
 */

#ifndef _HOTKEY_H
#define _HOTKEY_H

#include <stdio.h>
#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HOTKEYS
{
    char *desc;
    int sym;
    int state;
} HOTKEYS;

typedef struct _KEY_LIST {
    char           *strKey;
    int             code;
} KEY_LIST;

typedef enum _KEY_STATE {
    KEY_NONE = 0,
    KEY_SHIFT_COMP = 1 << 0,
    KEY_CAPSLOCK = 1 << 1,
    KEY_CTRL_COMP = 1 << 2,
    KEY_ALT_COMP = 1 << 3,
    KEY_ALT_SHIFT_COMP = KEY_ALT_COMP | KEY_SHIFT_COMP,
    KEY_CTRL_SHIFT_COMP = KEY_CTRL_COMP | KEY_SHIFT_COMP,
    KEY_CTRL_ALT_COMP = KEY_CTRL_COMP | KEY_ALT_COMP,
    KEY_CTRL_ALT_SHIFT_COMP = KEY_CTRL_COMP | KEY_ALT_COMP | KEY_SHIFT_COMP,
    KEY_NUMLOCK = 1 << 4,
    KEY_SUPER_COMP = 1 << 6,
    KEY_SCROLLLOCK = 1 << 7,
    KEY_MOUSE_PRESSED = 1 << 8
} KEY_STATE;

void SetHotKey (char *strKey, HOTKEYS * hotkey);
void GetKey (KeySym keysym, int iKeyState, int iCount, KeySym* outk, unsigned int* outs);
Bool ParseKey (char *strKey, KeySym* sym, int* state);
int GetKeyList (char *strKey);
char* GetKeyString(KeySym sym, unsigned int state);

Bool IsHotKeyDigit(KeySym sym, int state);
Bool IsHotKeyUAZ(KeySym sym, int state);
Bool IsHotKeyLAZ(KeySym sym, int state);
Bool IsHotKeySimple(KeySym sym, int state);

#ifdef __cplusplus
}
#endif

#endif
