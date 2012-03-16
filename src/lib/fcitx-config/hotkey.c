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

#include <stdlib.h>
#include <string.h>

#include "fcitx/fcitx.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"
#include "hotkey.h"
#include "keydata.h"

/**
 * String to key list.
 **/

typedef struct _KEY_LIST {
    /**
     * string name for the key in fcitx
     **/
    char         *strKey;
    /**
     * the keyval for the key.
     **/
    FcitxKeySym  code;
} KEY_LIST;

/* fcitx key name translist */
KEY_LIST        keyList[] = {
    {"F1", FcitxKey_F1},
    {"F2", FcitxKey_F2},
    {"F3", FcitxKey_F3},
    {"F4", FcitxKey_F4},
    {"F5", FcitxKey_F5},
    {"F6", FcitxKey_F6},
    {"F7", FcitxKey_F7},
    {"F8", FcitxKey_F8},
    {"F9", FcitxKey_F9},
    {"F10", FcitxKey_F10},
    {"F11", FcitxKey_F11},
    {"F12", FcitxKey_F12},
    {"F13", FcitxKey_F13},
    {"F14", FcitxKey_F14},
    {"F15", FcitxKey_F15},
    {"F16", FcitxKey_F16},
    {"F17", FcitxKey_F17},
    {"F18", FcitxKey_F18},
    {"F19", FcitxKey_F19},
    {"F20", FcitxKey_F20},
    {"F21", FcitxKey_F21},
    {"F22", FcitxKey_F22},
    {"F23", FcitxKey_F23},
    {"F24", FcitxKey_F24},
    {"F25", FcitxKey_F25},
    {"F26", FcitxKey_F26},
    {"F27", FcitxKey_F27},
    {"F28", FcitxKey_F28},
    {"F29", FcitxKey_F29},
    {"F30", FcitxKey_F30},
    {"F31", FcitxKey_F31},
    {"F32", FcitxKey_F32},
    {"F33", FcitxKey_F33},
    {"F34", FcitxKey_F34},
    {"F35", FcitxKey_F35},
    {"TAB", FcitxKey_Tab},
    {"ENTER", FcitxKey_Return},
    {"LCTRL", FcitxKey_Control_L},
    {"LSHIFT", FcitxKey_Shift_L},
    {"LALT", FcitxKey_Alt_L},
    {"RCTRL", FcitxKey_Control_R},
    {"RSHIFT", FcitxKey_Shift_R},
    {"RALT", FcitxKey_Alt_R},
    {"INSERT", FcitxKey_Insert},
    {"HOME", FcitxKey_Home},
    {"PGUP", FcitxKey_Page_Up},
    {"END", FcitxKey_End},
    {"PGDN", FcitxKey_Page_Down},
    {"ESCAPE", FcitxKey_Escape},
    {"SPACE", FcitxKey_space},
    {"DELETE", FcitxKey_Delete},
    {"UP", FcitxKey_Up},
    {"DOWN", FcitxKey_Down},
    {"LEFT", FcitxKey_Left},
    {"RIGHT", FcitxKey_Right},
    {"HANGUL", FcitxKey_Hangul},
    {"HANGULHANJA", FcitxKey_Hangul_Hanja},
    {"ZENKAKUHANKAKU", FcitxKey_Zenkaku_Hankaku},
    {"\0", 0}
};


FCITX_EXPORT_API
uint32_t
FcitxKeySymToUnicode (FcitxKeySym keyval)
{
    int min = 0;
    int max = sizeof (gdk_keysym_to_unicode_tab) / sizeof(gdk_keysym_to_unicode_tab[0]) - 1;
    int mid;

    /* First check for Latin-1 characters (1:1 mapping) */
    if ((keyval >= 0x0020 && keyval <= 0x007e) ||
            (keyval >= 0x00a0 && keyval <= 0x00ff))
        return keyval;

    /* Also check for directly encoded 24-bit UCS characters:
    */
    if ((keyval & 0xff000000) == 0x01000000)
        return keyval & 0x00ffffff;

    /* binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (gdk_keysym_to_unicode_tab[mid].keysym < keyval)
            min = mid + 1;
        else if (gdk_keysym_to_unicode_tab[mid].keysym > keyval)
            max = mid - 1;
        else {
            /* found it */
            return gdk_keysym_to_unicode_tab[mid].ucs;
        }
    }

    /* No matching Unicode value found */
    return 0;
}

FCITX_EXPORT_API
FcitxKeySym
FcitxUnicodeToKeySym (uint32_t wc)
{
    int min = 0;
    int max = sizeof(gdk_unicode_to_keysym_tab) / sizeof(gdk_unicode_to_keysym_tab[0]) - 1;
    int mid;

    /* First check for Latin-1 characters (1:1 mapping) */
    if ((wc >= 0x0020 && wc <= 0x007e) ||
            (wc >= 0x00a0 && wc <= 0x00ff))
        return wc;

    /* Binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (gdk_unicode_to_keysym_tab[mid].ucs < wc)
            min = mid + 1;
        else if (gdk_unicode_to_keysym_tab[mid].ucs > wc)
            max = mid - 1;
        else {
            /* found it */
            return gdk_unicode_to_keysym_tab[mid].keysym;
        }
    }

    /*
    * No matching keysym value found, return Unicode value plus 0x01000000
    * (a convention introduced in the UTF-8 work on xterm).
    */
    return wc | 0x01000000;
}

static int FcitxHotkeyGetKeyList(const char *strKey);
static char *FcitxHotkeyGetKeyListString(int key);

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKeyDigit(FcitxKeySym sym, int state)
{
    if (state)
        return false;

    if (sym >= FcitxKey_0 && sym <= FcitxKey_9)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKeyUAZ(FcitxKeySym sym, int state)
{
    if (state)
        return false;

    if (sym >= FcitxKey_A && sym <= FcitxKey_Z)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKeySimple(FcitxKeySym sym, int state)
{
    if (state)
        return false;

    if (sym >= FcitxKey_space && sym <= FcitxKey_asciitilde)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKeyLAZ(FcitxKeySym sym, int state)
{
    if (state)
        return false;

    if (sym >= FcitxKey_a && sym <= FcitxKey_z)
        return true;

    return false;
}


FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKey(FcitxKeySym sym, int state, const FcitxHotkey * hotkey)
{
    state &= FcitxKeyState_Ctrl_Alt_Shift;
    if (hotkey[0].sym && sym == hotkey[0].sym && (hotkey[0].state == state))
        return true;
    if (hotkey[1].sym && sym == hotkey[1].sym && (hotkey[1].state == state))
        return true;
    return false;
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotkeyCursorMove(FcitxKeySym sym, int state)
{
    if ((
                sym == FcitxKey_Left
                || sym == FcitxKey_Right
                || sym == FcitxKey_Up
                || sym == FcitxKey_Down
                || sym == FcitxKey_Page_Up
                || sym == FcitxKey_Page_Down
                || sym == FcitxKey_Home
                || sym == FcitxKey_End
            ) && (
                state == FcitxKeyState_Ctrl
                || state == FcitxKeyState_Ctrl_Shift
                || state == FcitxKeyState_Shift
                || state == FcitxKeyState_None
            )

       ) {
        return true;
    }
    return false;
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKeyModifierCombine(FcitxKeySym sym, int state)
{
    if (sym == FcitxKey_Control_L
            || sym == FcitxKey_Control_R
            || sym == FcitxKey_Shift_L
            || sym == FcitxKey_Shift_R)
        return true;

    return false;
}

/*
 * Do some custom process
 */
FCITX_EXPORT_API
void FcitxHotkeyGetKey(FcitxKeySym keysym, unsigned int iKeyState, FcitxKeySym* outk, unsigned int* outs)
{
    if (iKeyState) {
        if (iKeyState != FcitxKeyState_Shift && FcitxHotkeyIsHotKeyLAZ(keysym, 0))
            keysym = keysym + FcitxKey_A - FcitxKey_a;

        if (iKeyState == FcitxKeyState_Shift)
            if (((FcitxHotkeyIsHotKeySimple(keysym, 0) || FcitxKeySymToUnicode(keysym) != 0)
                && keysym != FcitxKey_space && keysym != FcitxKey_Return)
                || (keysym >= FcitxKey_KP_0 && keysym <= FcitxKey_KP_9))
                iKeyState = FcitxKeyState_None;
    }

    *outk = keysym;

    *outs = iKeyState;
}


FCITX_EXPORT_API
char* FcitxHotkeyGetKeyString(FcitxKeySym sym, unsigned int state)
{
    char *str;
    size_t len = 0;

    if (state & FcitxKeyState_Ctrl)
        len += strlen("CTRL_");

    if (state & FcitxKeyState_Alt)
        len += strlen("ALT_");

    if (state & FcitxKeyState_Shift)
        len += strlen("SHIFT_");

    char *key = FcitxHotkeyGetKeyListString(sym);

    if (!key)
        return NULL;

    len += strlen(key);

    str = fcitx_utils_malloc0(sizeof(char) * (len + 1));

    if (state & FcitxKeyState_Ctrl)
        strcat(str, "CTRL_");

    if (state & FcitxKeyState_Alt)
        strcat(str, "ALT_");

    if (state & FcitxKeyState_Shift)
        strcat(str, "SHIFT_");

    strcat(str, key);

    free(key);

    return str;
}

/*
 * 根据字串来判断键
 * 主要用于从设置文件中读取热键设定
 * 返回-1表示用户设置的热键不支持，一般是因为拼写错误或该热键不在列表中
 */
FCITX_EXPORT_API
boolean FcitxHotkeyParseKey(const char *strKey, FcitxKeySym* sym, int* state)
{
    const char      *p;
    int             iKey;
    int             iKeyState = 0;

    p = strKey;

    if (strstr(p, "CTRL_")) {
        iKeyState |= FcitxKeyState_Ctrl;
        p += strlen("CTRL_");
    }

    if (strstr(p, "ALT_")) {
        iKeyState |= FcitxKeyState_Alt;
        p += strlen("ALT_");
    }

    if (strstr(strKey, "SHIFT_")) {
        iKeyState |= FcitxKeyState_Shift;
        p += strlen("SHIFT_");
    }

    iKey = FcitxHotkeyGetKeyList(p);

    if (iKey == -1)
        return false;

    *sym = iKey;

    *state = iKeyState;

    return true;
}

FCITX_EXPORT_API
int FcitxHotkeyGetKeyList(const char *strKey)
{
    int             i;

    i = 0;

    for (;;) {
        if (!keyList[i].code)
            break;

        if (!strcmp(strKey, keyList[i].strKey))
            return keyList[i].code;

        i++;
    }

    if (strlen(strKey) == 1)
        return strKey[0];

    return -1;
}

char *FcitxHotkeyGetKeyListString(int key)
{
    if (key > FcitxKey_space && key <= FcitxKey_asciitilde) {
        char *p;
        p = malloc(sizeof(char) * 2);
        p[0] = key;
        p[1] = '\0';
        return p;
    }

    int             i;

    i = 0;

    for (;;) {
        if (!keyList[i].code)
            break;

        if (keyList[i].code == key)
            return strdup(keyList[i].strKey);

        i++;
    }

    return NULL;

}

FCITX_EXPORT_API
void FcitxHotkeySetKey(char *strKeys, FcitxHotkey * hotkey)
{
    char           *p;
    char           *strKey;
    int             i = 0, j = 0, k;

    strKeys = fcitx_utils_trim(strKeys);
    p = strKeys;

    for (k = 0; k < 2; k++) {
        FcitxKeySym sym;
        int state;
        i = 0;

        while (p[i] != ' ' && p[i] != '\0')
            i++;

        strKey = strndup(p, i);

        strKey[i] = '\0';

        if (FcitxHotkeyParseKey(strKey, &sym, &state)) {
            hotkey[j].sym = sym;
            hotkey[j].state = state;
            hotkey[j].desc = fcitx_utils_trim(strKey);
            j ++;
        }

        free(strKey);

        if (p[i] == '\0')
            break;

        p = &p[i + 1];
    }

    for (; j < 2; j++) {
        hotkey[j].sym = 0;
        hotkey[j].state = 0;
        hotkey[j].desc = NULL;
    }

    free(strKeys);
}

struct KeyPadTable {
    FcitxKeySym keypad;
    FcitxKeySym keymain;
};

static int key_table_cmp(const void* a, const void* b)
{
    const struct KeyPadTable* ka = a;
    const struct KeyPadTable* kb = b;
    return ka->keypad - kb->keypad;
}

FCITX_EXPORT_API
FcitxKeySym FcitxHotkeyPadToMain(FcitxKeySym sym)
{
    static struct KeyPadTable keytable[] = {
        {FcitxKey_KP_Space, FcitxKey_space},
        {FcitxKey_KP_Tab, FcitxKey_Tab},
        {FcitxKey_KP_Enter, FcitxKey_Return},
        {FcitxKey_KP_F1, FcitxKey_F1},
        {FcitxKey_KP_F2, FcitxKey_F2},
        {FcitxKey_KP_F3, FcitxKey_F3},
        {FcitxKey_KP_F4, FcitxKey_F4},
        {FcitxKey_KP_Home, FcitxKey_Home},
        {FcitxKey_KP_Left, FcitxKey_Left},
        {FcitxKey_KP_Up, FcitxKey_Up},
        {FcitxKey_KP_Right, FcitxKey_Right},
        {FcitxKey_KP_Down, FcitxKey_Down},
        {FcitxKey_KP_Prior, FcitxKey_Prior},
        {FcitxKey_KP_Page_Up, FcitxKey_Page_Up},
        {FcitxKey_KP_Next, FcitxKey_Next},
        {FcitxKey_KP_Page_Down, FcitxKey_Page_Down},
        {FcitxKey_KP_End, FcitxKey_End},
        {FcitxKey_KP_Begin, FcitxKey_Begin},
        {FcitxKey_KP_Insert, FcitxKey_Insert},
        {FcitxKey_KP_Delete, FcitxKey_Delete},
        {FcitxKey_KP_Equal, FcitxKey_equal},
        {FcitxKey_KP_Multiply, FcitxKey_asterisk},
        {FcitxKey_KP_Add, FcitxKey_plus},
        {FcitxKey_KP_Separator, FcitxKey_comma},
        {FcitxKey_KP_Subtract, FcitxKey_minus},
        {FcitxKey_KP_Decimal, FcitxKey_period},
        {FcitxKey_KP_Divide, FcitxKey_slash},

        {FcitxKey_KP_0, FcitxKey_0},
        {FcitxKey_KP_1, FcitxKey_1},
        {FcitxKey_KP_2, FcitxKey_2},
        {FcitxKey_KP_3, FcitxKey_3},
        {FcitxKey_KP_4, FcitxKey_4},
        {FcitxKey_KP_5, FcitxKey_5},
        {FcitxKey_KP_6, FcitxKey_6},
        {FcitxKey_KP_7, FcitxKey_7},
        {FcitxKey_KP_8, FcitxKey_8},
        {FcitxKey_KP_9, FcitxKey_9},
    };

    struct KeyPadTable key = { sym, FcitxKey_None };

    struct KeyPadTable *result = bsearch(&key, keytable, sizeof(keytable) / sizeof(struct KeyPadTable) , sizeof(struct KeyPadTable), key_table_cmp);
    if (result == NULL)
        return sym;
    else
        return result->keymain;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
