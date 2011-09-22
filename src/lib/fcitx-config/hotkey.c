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

#include <stdlib.h>
#include <string.h>

#include "fcitx/fcitx.h"
#include "hotkey.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"

/**
 * @brief String to key list.
 **/

typedef struct _KEY_LIST {
    /**
     * @brief string name for the key in fcitx
     **/
    char         *strKey;
    /**
     * @brief the keyval for the key.
     **/
    FcitxKeySym  code;
} KEY_LIST;

/* fcitx key name translist */
KEY_LIST        keyList[] = {
    {"TAB", Key_Tab},
    {"ENTER", Key_Return},
    {"LCTRL", Key_Control_L},
    {"LSHIFT", Key_Shift_L},
    {"LALT", Key_Alt_L},
    {"RCTRL", Key_Control_R},
    {"RSHIFT", Key_Shift_R},
    {"RALT", Key_Alt_R},
    {"INSERT", Key_Insert},
    {"HOME", Key_Home},
    {"PGUP", Key_Page_Up},
    {"END", Key_End},
    {"PGDN", Key_Page_Down},
    {"ESCAPE", Key_Escape},
    {"SPACE", Key_space},
    {"DELETE", Key_Delete},
    {"\0", 0}
};

static int GetKeyList(char *strKey);
static char *GetKeyListString(int key);

FCITX_EXPORT_API
boolean IsHotKeyDigit(FcitxKeySym sym, int state)
{
    if (state)
        return false;

    if (sym >= Key_0 && sym <= Key_9)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean IsHotKeyUAZ(FcitxKeySym sym, int state)
{
    if (state)
        return false;

    if (sym >= Key_A && sym <= Key_Z)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean IsHotKeySimple(FcitxKeySym sym, int state)
{
    if (state)
        return false;

    if (sym >= Key_space && sym <= Key_asciitilde)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean IsHotKeyLAZ(FcitxKeySym sym, int state)
{
    if (state)
        return false;

    if (sym >= Key_a && sym <= Key_z)
        return true;

    return false;
}

/*
 * Do some custom process
 */
FCITX_EXPORT_API
void GetKey(FcitxKeySym keysym, unsigned int iKeyState, FcitxKeySym* outk, unsigned int* outs)
{
    if (iKeyState) {
        if (IsHotKeyLAZ(keysym, 0))
            keysym = keysym + Key_A - Key_a;

        if (iKeyState == KEY_SHIFT_COMP)
            if ((IsHotKeySimple(keysym, 0) && keysym != Key_space)
                    || (keysym >= Key_KP_0 && keysym <= Key_KP_9))
                iKeyState = KEY_NONE;
    }

    *outk = keysym;

    *outs = iKeyState;
}


FCITX_EXPORT_API
char* GetKeyString(FcitxKeySym sym, unsigned int state)
{
    char *str;
    size_t len = 0;

    if (state & KEY_CTRL_COMP)
        len += strlen("CTRL_");

    if (state & KEY_ALT_COMP)
        len += strlen("ALT_");

    if (state & KEY_SHIFT_COMP)
        len += strlen("SHIFT_");

    char *key = GetKeyListString(sym);

    if (!key)
        return NULL;

    len += strlen(key);

    str = fcitx_malloc0(sizeof(char) * (len + 1));

    if (state & KEY_CTRL_COMP)
        strcat(str, "CTRL_");

    if (state & KEY_ALT_COMP)
        strcat(str, "ALT_");

    if (state & KEY_SHIFT_COMP)
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
boolean ParseKey(char *strKey, FcitxKeySym* sym, int* state)
{
    char           *p;
    int             iKey;
    int             iKeyState = 0;

    p = strKey;

    if (strstr(p, "CTRL_")) {
        iKeyState |= KEY_CTRL_COMP;
        p += strlen("CTRL_");
    }

    if (strstr(p, "ALT_")) {
        iKeyState |= KEY_ALT_COMP;
        p += strlen("ALT_");
    }

    if (strstr(strKey, "SHIFT_")) {
        iKeyState |= KEY_SHIFT_COMP;
        p += strlen("SHIFT_");
    }

    iKey = GetKeyList(p);

    if (iKey == -1)
        return false;

    *sym = iKey;

    *state = iKeyState;

    return true;
}

FCITX_EXPORT_API
int GetKeyList(char *strKey)
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

char *GetKeyListString(int key)
{
    if (key > Key_space && key <= Key_asciitilde) {
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
void SetHotKey(char *strKeys, HOTKEYS * hotkey)
{
    char           *p;
    char           *strKey;
    int             i = 0, j = 0, k;

    strKeys = fcitx_trim(strKeys);
    p = strKeys;

    for (k = 0; k < 2; k++) {
        FcitxKeySym sym;
        int state;
        i = 0;

        while (p[i] != ' ' && p[i] != '\0')
            i++;

        strKey = strndup(p, i);

        strKey[i] = '\0';

        if (ParseKey(strKey, &sym, &state)) {
            hotkey[j].sym = sym;
            hotkey[j].state = state;
            hotkey[j].desc = fcitx_trim(strKey);
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
FcitxKeySym KeyPadToMain(FcitxKeySym sym)
{
    static struct KeyPadTable keytable[] = {
        {Key_KP_Space, Key_space},
        {Key_KP_Tab, Key_Tab},
        {Key_KP_Enter, Key_Return},
        {Key_KP_F1, Key_F1},
        {Key_KP_F2, Key_F2},
        {Key_KP_F3, Key_F3},
        {Key_KP_F4, Key_F4},
        {Key_KP_Home, Key_Home},
        {Key_KP_Left, Key_Left},
        {Key_KP_Up, Key_Up},
        {Key_KP_Right, Key_Right},
        {Key_KP_Down, Key_Down},
        {Key_KP_Prior, Key_Prior},
        {Key_KP_Page_Up, Key_Page_Up},
        {Key_KP_Next, Key_Next},
        {Key_KP_Page_Down, Key_Page_Down},
        {Key_KP_End, Key_End},
        {Key_KP_Begin, Key_Begin},
        {Key_KP_Insert, Key_Insert},
        {Key_KP_Delete, Key_Delete},
        {Key_KP_Equal, Key_equal},
        {Key_KP_Multiply, Key_asterisk},
        {Key_KP_Add, Key_plus},
        {Key_KP_Separator, Key_comma},
        {Key_KP_Subtract, Key_minus},
        {Key_KP_Decimal, Key_period},
        {Key_KP_Divide, Key_slash},

        {Key_KP_0, Key_0},
        {Key_KP_1, Key_1},
        {Key_KP_2, Key_2},
        {Key_KP_3, Key_3},
        {Key_KP_4, Key_4},
        {Key_KP_5, Key_5},
        {Key_KP_6, Key_6},
        {Key_KP_7, Key_7},
        {Key_KP_8, Key_8},
        {Key_KP_9, Key_9},
    };

    struct KeyPadTable key = { sym, Key_None };

    struct KeyPadTable *result = bsearch(&key, keytable, sizeof(keytable) / sizeof(struct KeyPadTable) , sizeof(struct KeyPadTable), key_table_cmp);
    if (result == NULL)
        return sym;
    else
        return result->keymain;
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
