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
#include <fcitx/context.h>

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
 * Trans full width punc for Fcitx
 */

static void* FullWidthCharCreate(FcitxInstance* instance);
char* ProcessFullWidthChar(void* arg, const char* str);
static void ToggleFullWidthState(void *arg);
static boolean GetFullWidthState(void *arg);
static INPUT_RETURN_VALUE ToggleFullWidthStateWithHotkey(void *arg);
static boolean FullWidthPostFilter(void* arg, FcitxKeySym sym,
                              unsigned int state,
                              INPUT_RETURN_VALUE *retval
                             );
static void DisableFullWidthCharChanged(void* arg, const void* value);

typedef struct _FcitxFullWidthChar {
    FcitxInstance* owner;
} FcitxFullWidthChar;

FCITX_DEFINE_PLUGIN(fcitx_fullwidth_char, module, FcitxModule) = {
    FullWidthCharCreate,
    NULL,
    NULL,
    NULL,
    NULL
};

void* FullWidthCharCreate(FcitxInstance* instance)
{
    FcitxFullWidthChar* fwchar = fcitx_utils_malloc0(sizeof(FcitxFullWidthChar));
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(instance);
    fwchar->owner = instance;
    FcitxStringFilterHook hk;
    hk.arg = fwchar;
    hk.func = ProcessFullWidthChar;
    FcitxInstanceRegisterCommitFilter(instance, hk);

    FcitxKeyFilterHook phk;
    phk.arg = fwchar;
    phk.func = FullWidthPostFilter;
    FcitxInstanceRegisterPostInputFilter(instance, phk);

    FcitxHotkeyHook hotkey;
    hotkey.hotkey = config->hkFullWidthChar;
    hotkey.hotkeyhandle = ToggleFullWidthStateWithHotkey;
    hotkey.arg = fwchar;

    FcitxInstanceRegisterHotkeyFilter(instance, hotkey);

    FcitxProfile* profile = FcitxInstanceGetProfile(instance);
    FcitxUIRegisterStatus(instance, fwchar, "fullwidth",
                          profile->bUseFullWidthChar ? _("Full width Character") :  _("Half width Character"),
                          _("Toggle Half/Full width Character"),
                          ToggleFullWidthState, GetFullWidthState);

    FcitxInstanceRegisterWatchableContext(instance, CONTEXT_DISABLE_FULLWIDTH, FCT_Boolean, FCF_ResetOnInputMethodChange);
    FcitxInstanceWatchContext(instance, CONTEXT_DISABLE_FULLWIDTH, DisableFullWidthCharChanged, fwchar);

    return fwchar;
}

void DisableFullWidthCharChanged(void* arg, const void* value)
{
    const boolean* b = value;
    FcitxFullWidthChar* fwchar = arg;
    FcitxUISetStatusVisable(fwchar->owner, "fullwidth",  !(*b));
}

boolean FullWidthPostFilter(void* arg, FcitxKeySym sym,
                              unsigned int state,
                              INPUT_RETURN_VALUE *retval
                             )
{
    FcitxFullWidthChar* fwchar = (FcitxFullWidthChar*)arg;
    FcitxProfile* profile = FcitxInstanceGetProfile(fwchar->owner);
    if (*retval != IRV_TO_PROCESS)
        return false;

    FcitxUIStatus *status = FcitxUIGetStatusByName(fwchar->owner, "fullwidth");
    if (profile->bUseFullWidthChar && status->visible && FcitxHotkeyIsHotKeySimple(sym, state)) {
        strcpy(FcitxInputStateGetOutputString(FcitxInstanceGetInputState(fwchar->owner)), sCornerTrans[sym - 32]);
        *retval = IRV_COMMIT_STRING;
        return true;
    }
    return false;
}


char* ProcessFullWidthChar(void* arg, const char* str)
{
    FcitxFullWidthChar* fwchar = (FcitxFullWidthChar*)arg;
    FcitxProfile* profile = FcitxInstanceGetProfile(fwchar->owner);
    FcitxUIStatus *status = FcitxUIGetStatusByName(fwchar->owner, "fullwidth");
    if (profile->bUseFullWidthChar && status->visible) {
        size_t i = 0, ret_len = 0, len = fcitx_utf8_strlen(str);
        char* ret = (char *) fcitx_utils_malloc0(sizeof(char) * (UTF8_MAX_LENGTH * len + 1));
        const char* ps = str;
        ret[0] = '\0';
        for (; i < len; ++i) {
            uint32_t wc;
            int chr_len = fcitx_utf8_char_len(ps);
            char *nps;
            nps = fcitx_utf8_get_char(ps , &wc);

            if (chr_len == 1 && ps[0] >= '\x20' && ps[0] <= '\x7e') {
                strcat(ret, sCornerTrans[ps[0] - 32]);
                ret_len += strlen(sCornerTrans[ps[0] - 32]);
            } else {
                strncat(ret, ps, chr_len);
                ret_len += chr_len;
            }

            ps = nps;

        }
        ret[ret_len] = '\0';
        return ret;
    }
    else
        return NULL;
}

void ToggleFullWidthState(void* arg)
{
    FcitxFullWidthChar* fwchar = (FcitxFullWidthChar*)arg;
    FcitxProfile* profile = FcitxInstanceGetProfile(fwchar->owner);
    profile->bUseFullWidthChar = !profile->bUseFullWidthChar;
    FcitxUISetStatusString(fwchar->owner, "fullwidth",
                            profile->bUseFullWidthChar ? _("Full width Character") :  _("Half width Character"),
                          _("Toggle Half/Full width Character"));
    FcitxProfileSave(profile);
}

boolean GetFullWidthState(void* arg)
{
    FcitxFullWidthChar* fwchar = (FcitxFullWidthChar*)arg;
    FcitxProfile* profile = FcitxInstanceGetProfile(fwchar->owner);
    return profile->bUseFullWidthChar;
}

INPUT_RETURN_VALUE ToggleFullWidthStateWithHotkey(void* arg)
{
    FcitxFullWidthChar* fwchar = (FcitxFullWidthChar*)arg;
    FcitxUIStatus *status = FcitxUIGetStatusByName(fwchar->owner, "fullwidth");
    if (status->visible) {
        FcitxUIUpdateStatus(fwchar->owner, "fullwidth");
        return IRV_DO_NOTHING;
    }
    else
        return IRV_TO_PROCESS;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
