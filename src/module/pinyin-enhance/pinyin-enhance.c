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
#include <ctype.h>

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

enum {
    PY_IM_INVALID = 0,
    PY_IM_PINYIN,
    PY_IM_SHUANGPIN,
};

static inline int
check_im_type(FcitxIM *im)
{
    if (!im)
        return PY_IM_INVALID;
    if (strcmp(im->uniqueName, "pinyin") == 0 ||
        strcmp(im->uniqueName, "pinyin-libpinyin") == 0 ||
        strcmp(im->uniqueName, "googlepinyin") == 0 ||
        strcmp(im->uniqueName, "sunpinyin") == 0)
        return PY_IM_PINYIN;
    if (strcmp(im->uniqueName, "shuangpin-libpinyin") == 0 ||
        strcmp(im->uniqueName, "shuangpin") == 0)
        return PY_IM_SHUANGPIN;
    return PY_IM_INVALID;
}

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
    boolean short_as_english;
    boolean allow_replace_first;
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
CONFIG_BINDING_REGISTER("Pinyin Enhance", "ShortAsEnglish", short_as_english);
CONFIG_BINDING_REGISTER("Pinyin Enhance", "AllowReplaceFirst",
                        allow_replace_first);
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

static void
PinyinEnhanceMergeCandList(FcitxCandidateWordList *candList,
                           FcitxCandidateWordList *newList, int position)
{
    int i1;
    int n1;
    int i2;
    int n2;
    FcitxCandidateWord *word1;
    FcitxCandidateWord *word2;
    n1 = FcitxCandidateWordGetPageSize(candList);
    for (i1 = 0;i1 < n1 &&
             (word1 = FcitxCandidateWordGetByTotalIndex(candList, i1));i1++) {
        if (!word1->strWord)
            continue;
        n2 = FcitxCandidateWordGetListSize(newList);
        for (i2 = n2 - 1;i2 >= 0;i2--) {
            word2 = FcitxCandidateWordGetByTotalIndex(newList, i2);
            if (!word2->strWord) {
                FcitxCandidateWordRemoveByIndex(newList, i2);
                continue;
            }
            if (strcasecmp(word1->strWord, word2->strWord))
                continue;
            FcitxCandidateWordRemoveByIndex(newList, i2);
            if (i1 == position)
                position++;
        }
    }
    FcitxCandidateWordMerge(candList, newList, position);
    FcitxCandidateWordFreeList(newList);
}

static boolean
PinyinEnhanceGetCandWords(PinyinEnhance *pyenhance, const char *string,
                          int position, int len_limit)
{
    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input;
    FcitxCandidateWordList *candList;
    FcitxCandidateWordList *newList;
    input = FcitxInstanceGetInputState(instance);
    if (!FcitxAddonsIsAddonAvailable(FcitxInstanceGetAddons(instance),
                                     FCITX_SPELL_NAME))
        return false;
    candList = FcitxInputStateGetCandidateList(input);
    if (len_limit <= 0) {
        len_limit = FcitxCandidateWordGetPageSize(candList) / 2;
        len_limit = len_limit > 0 ? len_limit : 1;
    }
    if (position < 0 ||
        (position < 1 && pyenhance->config.allow_replace_first)) {
        position = 1;
    }
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
    PinyinEnhanceMergeCandList(candList, newList, position);
    return true;
}

enum {
    PY_TYPE_FULL,
    PY_TYPE_SHORT,
    PY_TYPE_INVALID,
};

static int
PinyinGetWordType(const char *str, int len)
{
    int i;
    if (len <= 0)
        len = strlen(str);
    switch (*str) {
    case 'a':
    case 'e':
    case 'o':
        return PY_TYPE_FULL;
    case 'i':
    case 'u':
    case 'v':
    case '\0':
        return PY_TYPE_INVALID;
    default:
        break;
    }
    for (i = 1;i < len;i++) {
        switch (str[i]) {
        case '\0':
            return PY_TYPE_SHORT;
        case 'a':
        case 'e':
        case 'o':
        case 'i':
        case 'u':
        case 'v':
            return PY_TYPE_FULL;
        default:
            continue;
        }
    }
    return PY_TYPE_SHORT;
}

static boolean
PinyinEnhanceSpellHint(PinyinEnhance *pyenhance, int im_type)
{
    FcitxInputState *input;
    char *string;
    char *pinyin;
    char *p;
    int spaces = 0;
    int vowels = 0;
    int letters = 0;
    int len_limit = -1;
    boolean res = false;
    FcitxCandidateWordList *cand_list;
    FcitxCandidateWord *cand_word;
    input = FcitxInstanceGetInputState(pyenhance->owner);
    string = FcitxUIMessagesToCString(FcitxInputStateGetPreedit(input));
    pinyin = fcitx_utils_get_ascii_part(string);
    cand_list = FcitxInputStateGetCandidateList(input);
    if (pinyin != string)
        pyenhance->selected = strndup(string, pinyin - string);
    p = pinyin;
    char *last_start = p;
    int words_count = 0;
    int words_type[strlen(p) / 2 + 1];
    do {
        switch (*p) {
        case ' ': {
            int word_len = p - spaces - last_start;
            if (word_len > 0) {
                if (im_type == PY_IM_PINYIN) {
                    words_type[words_count++] = PinyinGetWordType(last_start,
                                                                  word_len);
                } else if (im_type == PY_IM_SHUANGPIN) {
                    words_type[words_count++] = (word_len == 2 ?
                                                 PY_TYPE_FULL :
                                                 PY_TYPE_INVALID);
                }
            }
            last_start = last_start + word_len;
        }
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
    /* not at the end of the string */
    if (*last_start) {
        if (im_type == PY_IM_PINYIN) {
            words_type[words_count++] = PinyinGetWordType(last_start, -1);
        } else if (im_type == PY_IM_SHUANGPIN) {
            words_type[words_count++] = (strlen(last_start) == 2 ?
                                         PY_TYPE_FULL :
                                         PY_TYPE_INVALID);
        }
    }
    cand_word = FcitxCandidateWordGetFirst(cand_list);
    if (!cand_word || !cand_word->strWord || !*cand_word->strWord
        || isascii(cand_word->strWord[0])) {
        len_limit = FcitxCandidateWordGetPageSize(cand_list) - 1;
        res = PinyinEnhanceGetCandWords(pyenhance, pinyin, 0, len_limit);
        goto out;
    } else {
        int page_size = FcitxCandidateWordGetPageSize(cand_list);
        int cand_wordsc = FcitxCandidateWordGetListSize(cand_list);
        if (page_size > cand_wordsc) {
            len_limit = page_size - cand_wordsc;
            len_limit = len_limit > page_size / 2 ? len_limit : page_size / 2;
        }
    }
    if (im_type == PY_IM_PINYIN || im_type == PY_IM_SHUANGPIN) {
        int eng_ness = 5;
        int py_invalid = 0;
        int py_full = 0;
        int py_short = 0;
        int i;
        for (i = 0;i < words_count;i++) {
            switch (words_type[i]) {
            case PY_TYPE_FULL:
                py_full++;
                eng_ness -= 2;
                break;
            case PY_TYPE_SHORT:
                py_short++;
                eng_ness += 3;
                break;
            case PY_TYPE_INVALID:
            default:
                py_invalid++;
                eng_ness += 6;
                break;
            }
            if (eng_ness > 10 || eng_ness < 0)
                break;
        }
        if (py_invalid || (py_short && pyenhance->config.short_as_english)) {
            res = PinyinEnhanceGetCandWords(pyenhance, pinyin,
                                            eng_ness > 10 ? 0 : 1, len_limit);
            goto out;
        }
        if (eng_ness > 10) {
            res = PinyinEnhanceGetCandWords(pyenhance, pinyin, 1, len_limit);
            goto out;
        }
    }
    /* pretty random numbers here. */
    if ((letters >= 4) &&
        (spaces * 2 > letters ||
         (spaces * 3 >= letters && vowels * 3 >= letters))) {
        res = PinyinEnhanceGetCandWords(pyenhance, pinyin, 1, len_limit);
        goto out;
    }
    if (len_limit > 0) {
        res = PinyinEnhanceGetCandWords(pyenhance, pinyin, 2, len_limit);
        goto out;
    }
out:
    free(string);
    return res;
}

static void
PinyinEnhanceAddCandidateWord(void *arg)
{
    PinyinEnhance *pyenhance = (PinyinEnhance*)arg;
    FcitxIM *im = FcitxInstanceGetCurrentIM(pyenhance->owner);
    int im_type;

    if (pyenhance->selected) {
        free(pyenhance->selected);
        pyenhance->selected = NULL;
    }
    /* check whether the current im is pinyin */
    if (!(im_type = check_im_type(im)))
        return;
    PinyinEnhanceSpellHint(pyenhance, im_type);
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
