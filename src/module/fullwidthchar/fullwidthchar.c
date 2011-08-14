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

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <libintl.h>

#include "fcitx/module.h"
#include "fcitx/fcitx.h"
#include "fcitx/hook.h"
#include "fcitx/ime.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"
#include "fcitx/keys.h"
#include "fcitx/frontend.h"
#include "fcitx/instance.h"

char *sCornerTrans[] = {
    "　", "！", "＂", "＃", "￥", "％", "＆", "＇", "（", "）",
    "＊",
    "＋", "，", "－", "．", "／",
    "０", "１", "２", "３", "４", "５", "６", "７", "８", "９",
    "：",
    "；", "＜", "＝", "＞", "？",
    "＠", "Ａ", "Ｂ", "Ｃ", "Ｄ", "Ｅ", "Ｆ", "Ｇ", "Ｈ", "Ｉ",
    "Ｊ",
    "Ｋ", "Ｌ", "Ｍ", "Ｎ", "Ｏ",
    "Ｐ", "Ｑ", "Ｒ", "Ｓ", "Ｔ", "Ｕ", "Ｖ", "Ｗ", "Ｘ", "Ｙ",
    "Ｚ",
    "［", "＼", "］", "＾", "＿",
    "｀", "ａ", "ｂ", "ｃ", "ｄ", "ｅ", "ｆ", "ｇ", "ｈ", "ｉ",
    "ｊ",
    "ｋ", "ｌ", "ｍ", "ｎ", "ｏ",
    "ｐ", "ｑ", "ｒ", "ｓ", "ｔ", "ｕ", "ｖ", "ｗ", "ｘ", "ｙ",
    "ｚ",
    "｛", "｜", "｝", "～",
};

#define FCITX_FULLWIDTH_CHAR "fcitx-fullwidth-char"

/**
 * @file fullwidthchar.c
 * @brief Trans full width punc for Fcitx
 */

static void* FullWidthCharCreate(FcitxInstance* instance);
static boolean ProcessFullWidthChar(void* arg, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retVal);
static void ToggleFullWidthState(void *arg);
static boolean GetFullWidthState(void *arg);
static INPUT_RETURN_VALUE ToggleFullWidthStateWithHotkey(void *arg);

typedef struct _FcitxFullWidthChar {
    FcitxInstance* owner;
} FcitxFullWidthChar;

FCITX_EXPORT_API
FcitxModule module = {
    FullWidthCharCreate,
    NULL,
    NULL,
    NULL,
    NULL
};

void* FullWidthCharCreate(FcitxInstance* instance)
{
    FcitxFullWidthChar* fwchar = fcitx_malloc0(sizeof(FcitxFullWidthChar));
    fwchar->owner = instance;
    KeyFilterHook hk;
    hk.arg = fwchar;
    hk.func = ProcessFullWidthChar;

    RegisterPreInputFilter(instance, hk);

    HotkeyHook hotkey;
    hotkey.hotkey = instance->config->hkFullWidthChar;
    hotkey.hotkeyhandle = ToggleFullWidthStateWithHotkey;
    hotkey.arg = fwchar;

    RegisterHotkeyFilter(instance, hotkey);

    RegisterStatus(instance, fwchar, "fullwidth", "Full Width Character", "Full Width Character",  ToggleFullWidthState, GetFullWidthState);

    return fwchar;
}

boolean ProcessFullWidthChar(void* arg, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retVal)
{
    FcitxFullWidthChar* fwchar = (FcitxFullWidthChar* )arg;
    if (fwchar->owner->profile->bUseFullWidthChar && IsHotKeySimple(sym, state))
    {
        sprintf(GetOutputString(&fwchar->owner->input), "%s", sCornerTrans[sym - 32]);
        *retVal = IRV_COMMIT_STRING;
        return true;
    }
    return false;
}

void ToggleFullWidthState(void* arg)
{
    FcitxFullWidthChar* fwchar = (FcitxFullWidthChar* )arg;
    fwchar->owner->profile->bUseFullWidthChar = !fwchar->owner->profile->bUseFullWidthChar;
    SaveProfile(fwchar->owner->profile);
    ResetInput(fwchar->owner);
}

boolean GetFullWidthState(void* arg)
{
    FcitxFullWidthChar* fwchar = (FcitxFullWidthChar* )arg;
    return fwchar->owner->profile->bUseFullWidthChar;
}

INPUT_RETURN_VALUE ToggleFullWidthStateWithHotkey(void* arg)
{
    FcitxFullWidthChar* fwchar = (FcitxFullWidthChar* )arg;
    UpdateStatus(fwchar->owner, "fullwidth");
    return IRV_DO_NOTHING;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
