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
 * @file   hotkey.h
 * @author Yuking yuking_net@sohu.com CS Slayer wengxt@gmail.com
 * @date   2008-1-16
 *
 *  hotkey related config and functions
 *
 */

#ifndef _FCITX_HOTKEY_H_
#define _FCITX_HOTKEY_H_

#include <stdint.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-utils/keysym.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * A fcitx hotkey, define the keysym (keyval) and (state) modifiers key state.
     **/

    typedef struct _FcitxHotkey {
        /**
         * A hotkey string
         **/
        char *desc;
        /**
         * keyval of hotkey
         **/
        FcitxKeySym sym;
        /**
         * state of hotkey
         **/
        unsigned int state;
    } FcitxHotkey;

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

    /**
     * Set the hotkey with a string
     *
     * @param strKey key string
     * @param hotkey hotkey array, it should have length 2
     * @return void
     **/
    void FcitxHotkeySetKey(const char *strKey, FcitxHotkey * hotkey);

    /**
     * translate the fcitx key to it's own value,
     * like uniform the keypad and the numbers, remove Shift from A-Z
     *
     * @param keysym keyval
     * @param iKeyState state
     * @param outk return of keyval
     * @param outs return of state
     * @return void
     **/
    void FcitxHotkeyGetKey(FcitxKeySym keysym, unsigned int iKeyState, FcitxKeySym* outk, unsigned int* outs);

    /**
     * parse the fcitx key string, like CTRL_SHIFT_A
     *
     * @param strKey key string
     * @param sym return of keyval
     * @param state return of key state
     * @return boolean can be parsed or not
     **/
    boolean FcitxHotkeyParseKey(const char *strKey, FcitxKeySym* sym, unsigned int* state);

    /**
     * Get the Fcitx Key String for given keyval and state
     *
     * @param sym keyval
     * @param state state
     * @return char* string like CTRL_SPACE
     **/
    char* FcitxHotkeyGetKeyString(FcitxKeySym sym, unsigned int state);

    /**
     * is hotkey 0-9
     *
     * @param sym keyval
     * @param state state
     * @return boolean
     **/
    boolean FcitxHotkeyIsHotKeyDigit(FcitxKeySym sym, unsigned int state);

    /**
     * is hotkey A-Z
     *
     * @param sym keyval
     * @param state state
     * @return boolean
     **/
    boolean FcitxHotkeyIsHotKeyUAZ(FcitxKeySym sym, unsigned int state);
    /**
     * is hotkey a-z
     *
     * @param sym keyval
     * @param state keystate
     * @return boolean
     **/
    boolean FcitxHotkeyIsHotKeyLAZ(FcitxKeySym sym, unsigned int state);
    /**
     * is hotkey printable
     *
     * @param sym keyval
     * @param state state
     * @return boolean
     **/
    boolean FcitxHotkeyIsHotKeySimple(FcitxKeySym sym, unsigned int state);

    /**
     * is hotkey upper case
     *
     * @param sym keyval
     * @param state keystate
     * @return boolean
     **/
    boolean FcitxHotkeyIsHotKeyCapital(FcitxKeySym sym, unsigned int state);

    /**
     * hotkey have combine modifier
     *
     * @param sym keyval
     * @param state state
     * @return state is combine modifier or not
     **/
    boolean FcitxHotkeyIsHotKeyModifierCombine(FcitxKeySym sym, unsigned int state);

    /**
     * check the key is this hotkey or not
     *
     * @param sym keysym
     * @param state key state
     * @param hotkey hotkey
     * @return boolean
     **/
    boolean FcitxHotkeyIsHotKey(FcitxKeySym sym, unsigned int state, const FcitxHotkey * hotkey);

    /**
     * is key will make cursor move, include left, right, home, end, and so on.
     *
     * @param sym keyval
     * @param state state
     * @return boolean
     **/
    boolean FcitxHotkeyIsHotkeyCursorMove(FcitxKeySym sym, unsigned int state);

    /**
     * convert key pad key to simple FcitxKeyState_STATE
     *
     * @param sym keyval
     * @return FcitxKeySym
     * @since 4.1.1
     */
    FcitxKeySym FcitxHotkeyPadToMain(FcitxKeySym sym);

    /**
     * @brief free hotkey description
     *
     * @param hotkey hotkey array
     * @return void
     *
     * @since 4.2.3
     **/
    void FcitxHotkeyFree(FcitxHotkey* hotkey);

    /**
     * convert unicode character to keyval
     *
     * If No matching keysym value found, return Unicode value plus 0x01000000
     * (a convention introduced in the UTF-8 work on xterm).
     *
     * @param wc unicode
     * @return FcitxKeySym
     **/
    FcitxKeySym FcitxUnicodeToKeySym (uint32_t wc);

    /**
     * convert keyval to unicode character
     *
     * @param keyval keyval
     * @return unicode
     **/
    uint32_t FcitxKeySymToUnicode (FcitxKeySym keyval);

#ifdef __cplusplus
}

#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
