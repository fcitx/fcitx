/***************************************************************************
 *   Copyright (C) 2011~2012 by CSSlayer                                   *
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

#include <errno.h>
#include <iconv.h>
#include <unistd.h>

#include <libintl.h>

#include <fcitx/fcitx.h>
#include <fcitx/module.h>
#include <fcitx/instance.h>
#include <fcitx/hook.h>
#include <fcitx-utils/log.h>
#include <fcitx/candidate.h>
#include <fcitx-config/xdg.h>
#include "module/spell/spell.h"

#include "config.h"

#define CHECK_VALID_IM(im)                                      \
    (im && strcmp(im->langCode, "zh_CN") == 0 &&                \
     (strcmp(im->uniqueName, "pinyin") == 0 ||                  \
      strcmp(im->uniqueName, "pinyin-libpinyin") == 0 ||        \
      strcmp(im->uniqueName, "shuangpin-libpinyin") == 0 ||     \
      strcmp(im->uniqueName, "googlepinyin") == 0 ||            \
      strcmp(im->uniqueName, "sunpinyin") == 0 ||               \
      strcmp(im->uniqueName, "shuangpin") == 0))

#define case_vowel case 'a': case 'e': case 'i': case 'o':      \
case 'u': case 'A': case 'E': case 'I': case 'O': case 'U'
#define case_consonant case 'B': case 'C': case 'D':          \
case 'F': case 'G': case 'H': case 'J': case 'K': case 'L':   \
case 'M': case 'N': case 'P': case 'Q': case 'R': case 'S':   \
case 'T': case 'V': case 'W': case 'X': case 'Y': case 'Z':   \
case 'b': case 'c': case 'd': case 'f': case 'g': case 'h':   \
case 'j': case 'k': case 'l': case 'm': case 'n': case 'p':   \
case 'q': case 'r': case 's': case 't': case 'v': case 'w':   \
case 'x': case 'y': case 'z'

#define LOGLEVEL DEBUG

typedef struct {
    FcitxGenericConfig gconfig;
} PinyinEnhanceConfig;

typedef struct {
    PinyinEnhanceConfig config;
    FcitxInstance *owner;
    char *selected;
} PinyinEnhance;

static void *PinyinEnhanceCreate(FcitxInstance *instance);
static void PinyinEnhanceDestroy(void *arg);
static void PinyinEnhanceReloadConfig(void *arg);
static void PinyinEnhanceAddCandidateWord(void *arg);

CONFIG_BINDING_BEGIN(PinyinEnhanceConfig)
CONFIG_BINDING_END()

CONFIG_DEFINE_LOAD_AND_SAVE(PinyinEnhance, PinyinEnhanceConfig,
                            "fcitx-pinyin-enhance")

FCITX_DEFINE_PLUGIN(fcitx_pinyin_enhance, module, FcitxModule) = {
    .Create = PinyinEnhanceCreate,
    .Destroy = PinyinEnhanceDestroy,
    .SetFD = NULL,
    .ProcessEvent = NULL,
    .ReloadConfig = PinyinEnhanceReloadConfig
};

static void*
PinyinEnhanceCreate(FcitxInstance *instance)
{
    PinyinEnhance *pyenhance = fcitx_utils_new(PinyinEnhance);
    pyenhance->owner = instance;

    if (!PinyinEnhanceLoadConfig(&pyenhance->config)) {
        free(pyenhance);
        return NULL;
    }

    FcitxIMEventHook hook;
    hook.arg = pyenhance;
    hook.func = PinyinEnhanceAddCandidateWord;

    FcitxInstanceRegisterUpdateCandidateWordHook(instance, hook);
    return pyenhance;
}

static boolean
FcitxPYEnhanceGetCandWordCb(void *arg, const char *commit)
{
    PinyinEnhance *pyenhance = (PinyinEnhance*)arg;
    FcitxInstance *instance = pyenhance->owner;
    if (pyenhance->selected) {
        FcitxInstanceCommitString(instance,
                                  FcitxInstanceGetCurrentIC(instance),
                                  pyenhance->selected);
        free(pyenhance->selected);
        pyenhance->selected = NULL;
    }
    return false;
}

static boolean
PinyinEnhanceGetCandWords(PinyinEnhance *pyenhance, const char *string)
{
    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input;
    FcitxCandidateWordList *candList;
    FcitxCandidateWordList *newList;
    int len_limit;
    input = FcitxInstanceGetInputState(instance);
    if (!FcitxAddonsIsAddonAvailable(FcitxInstanceGetAddons(instance),
                                     FCITX_SPELL_NAME))
        return false;
    candList = FcitxInputStateGetCandidateList(input);
    len_limit = FcitxCandidateWordGetPageSize(candList) / 2;
    len_limit = len_limit > 0 ? len_limit : 1;
    FcitxModuleFunctionArg func_arg;
    func_arg.args[0] = NULL;
    func_arg.args[1] = (void*)string;
    func_arg.args[2] = NULL;
    func_arg.args[3] = (void*)(long)len_limit;
    func_arg.args[4] = "en";
    func_arg.args[5] = "cus";
    func_arg.args[6] = FcitxPYEnhanceGetCandWordCb;
    func_arg.args[7] = pyenhance;
    newList = InvokeFunction(instance, FCITX_SPELL, GET_CANDWORDS, func_arg);
    if (!newList)
        return false;
    FcitxCandidateWordMerge(candList, newList, 1);
    FcitxCandidateWordFreeList(newList);
    return true;
}

static boolean
PinyinEnhanceGetStrings(PinyinEnhance *pyenhance,
                        char **ret_selected, char **ret_pinyin)
{
    FcitxIM *im;
    FcitxInputState *input;
    char *string;
    char *pinyin;
    char *p;
    int spaces = 0;
    int vowels = 0;
    int letters = 0;
    im = FcitxInstanceGetCurrentIM(pyenhance->owner);
    *ret_selected = NULL;
    *ret_pinyin = NULL;
    if (!im)
        return false;
    input = FcitxInstanceGetInputState(pyenhance->owner);
    string = FcitxUIMessagesToCString(FcitxInputStateGetPreedit(input));
    pinyin = fcitx_utils_get_ascii_part(string);
    if (pinyin != string)
        *ret_selected = strndup(string, pinyin - string);
    p = pinyin;
    do {
        switch (*p) {
        case ' ':
            spaces++;
            continue;
        case_vowel:
            vowels++;
        case_consonant:
            letters++;
        default:
            *(p - spaces) = *p;
            break;
        }
    } while (*(p++));
    *ret_pinyin = strdup(pinyin);
    free(string);
    // pretty random numbers here,
    // just want to add all possible parameters (that I can think of) which can
    // show the difference between Chinese and English.
    return (letters >= 4) &&
        (spaces * 2 > letters ||
         (spaces * 3 >= letters && vowels * 3 >= letters));
}

static void
PinyinEnhanceAddCandidateWord(void *arg)
{
    char *pinyin = NULL;
    PinyinEnhance *pyenhance = (PinyinEnhance*)arg;
    FcitxIM *im = FcitxInstanceGetCurrentIM(pyenhance->owner);

    if (pyenhance->selected) {
        free(pyenhance->selected);
        pyenhance->selected = NULL;
    }
    /* check whether the current im is pinyin */
    if (!CHECK_VALID_IM(im))
        return;
    if (PinyinEnhanceGetStrings(pyenhance, &pyenhance->selected, &pinyin)) {
        PinyinEnhanceGetCandWords(pyenhance, pinyin);
    }
    if (pinyin)
        free(pinyin);
    return;
}

static void
PinyinEnhanceDestroy(void *arg)
{
    PinyinEnhance *pyenhance = (PinyinEnhance*)arg;
    if (pyenhance->selected)
        free(pyenhance->selected);
}

static void
PinyinEnhanceReloadConfig(void *arg)
{
    PinyinEnhance *pyenhance = (PinyinEnhance*)arg;
    PinyinEnhanceLoadConfig(&pyenhance->config);
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
