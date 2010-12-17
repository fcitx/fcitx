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

#include "core/fcitx.h"

#include "fcitx-config/cutils.h"
#include "tools/tools.h"
#include "fcitx-config/hotkey.h"
#include <string.h>
#include <X11/keysym.h>

/* fcitx key name translist */
KEY_LIST        keyList[] = {
    {"TAB", XK_Tab},
    {"ENTER", XK_Return},
    {"LCTRL", XK_Control_L},
    {"LSHIFT", XK_Shift_L},
    {"LALT", XK_Alt_L},
    {"RCTRL", XK_Control_R},
    {"RSHIFT", XK_Shift_R},
    {"RALT", XK_Alt_R},
    {"INSERT", XK_Insert},
    {"HOME", XK_Home},
    {"PGUP", XK_Page_Up},
    {"END", XK_End},
    {"PGDN", XK_Page_Down},
    {"ESCAPE", XK_Escape},
    {"SPACE", XK_space},
    {"DELETE", XK_Delete},
    {"\0", 0}
};

static char *GetKeyListString(int key);

FCITX_EXPORT_API
Bool IsHotKeyDigit(KeySym sym, int state)
{
    if (state)
        return False;

    if (sym >= XK_0 && sym <= XK_9)
        return True;

    return False;
}

FCITX_EXPORT_API
Bool IsHotKeyUAZ(KeySym sym, int state)
{
    if (state)
        return False;

    if (sym >= XK_A && sym <= XK_Z)
        return True;

    return False;
}

FCITX_EXPORT_API
Bool IsHotKeySimple(KeySym sym, int state)
{
    if (state)
        return False;

    if (sym >= XK_space && sym <= XK_asciitilde)
        return True;

    return False;
}

FCITX_EXPORT_API
Bool IsHotKeyLAZ(KeySym sym, int state)
{
    if (state)
        return False;

    if (sym >= XK_a && sym <= XK_z)
        return True;

    return False;
}

/*
 * Do some custom process
 */
FCITX_EXPORT_API
void GetKey (KeySym keysym, int iKeyState, int iCount, KeySym* outk, unsigned int* outs)
{
    if (iKeyState)
    {
        if (IsHotKeyLAZ(keysym, 0))
            keysym = keysym + XK_A - XK_a;

        if (iKeyState == KEY_SHIFT_COMP)
            if (IsHotKeySimple(keysym, 0) && keysym != XK_space)
                iKeyState = KEY_NONE;
    }
    *outk = keysym;
    *outs = iKeyState;
}


FCITX_EXPORT_API
char* GetKeyString(KeySym sym, unsigned int state)
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

    str = malloc0(sizeof(char) * (len + 1));
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
Bool ParseKey (char *strKey, KeySym* sym, int* state)
{
    char           *p;
    int             iKey;
    int             iKeyState = 0;

    p = strKey;
    if (strstr(p, "CTRL_"))
    {
        iKeyState |= KEY_CTRL_COMP;
        p += strlen("CTRL_");
    }

    if (strstr(p, "ALT_"))
    {
        iKeyState |= KEY_ALT_COMP;
        p += strlen("ALT_");
    }

    if (strstr(strKey, "SHIFT_"))
    {
        iKeyState |= KEY_SHIFT_COMP;
        p += strlen("SHIFT_");
    }

    iKey = GetKeyList (p);
    if (iKey == -1)
        return False;
    *sym = iKey;
    *state = iKeyState;
    return True;
}

FCITX_EXPORT_API
int GetKeyList (char *strKey)
{
    int             i;

    i = 0;
    for (;;) {
        if (!keyList[i].code)
            break;
        if (!strcmp (strKey, keyList[i].strKey))
            return keyList[i].code;
        i++;
    }

    if (strlen(strKey) == 1)
        return strKey[0];

    return -1;
}

char *GetKeyListString(int key)
{
    if (key > XK_space && key <= XK_asciitilde)
    {
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
void SetHotKey (char *strKeys, HOTKEYS * hotkey)
{
    char           *p;
    char           *strKey;
    int             i = 0, j = 0, k;

    strKeys = trim(strKeys);
    p = strKeys;

    for (k = 0; k < 2; k++)
    {
        KeySym sym;
        int state;
        i = 0;
        while (p[i] != ' ' && p[i] != '\0')
            i++;
        strKey = strndup (p, i);
        strKey[i] = '\0';
        if (ParseKey (strKey, &sym, &state))
        {
            hotkey[j].sym = sym;
            hotkey[j].state = state;
            hotkey[j].desc = trim(strKey);
            j ++;
        }
        free(strKey);
        if (p[i] == '\0')
            break;
        p = &p[i + 1];
    }
    for (; j < 2; j++)
    {
        hotkey[j].sym = 0;
        hotkey[j].state = 0;
        hotkey[j].desc= NULL;
    }
    free(strKeys);
}

