/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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

#include "pinyin-enhance-spell.h"
#include "module/spell/fcitx-spell.h"

#include "config.h"

enum {
    PY_TYPE_FULL,
    PY_TYPE_SHORT,
    PY_TYPE_INVALID,
};

#define case_vowel case 'a': case 'e': case 'i': case 'o':      \
case 'u': case 'A': case 'E': case 'I': case 'O': case 'U'

#define case_consonant case 'B': case 'C': case 'D':            \
case 'F': case 'G': case 'H': case 'J': case 'K': case 'L':     \
case 'M': case 'N': case 'P': case 'Q': case 'R': case 'S':     \
case 'T': case 'V': case 'W': case 'X': case 'Y': case 'Z':     \
case 'b': case 'c': case 'd': case 'f': case 'g': case 'h':     \
case 'j': case 'k': case 'l': case 'm': case 'n': case 'p':     \
case 'q': case 'r': case 's': case 't': case 'v': case 'w':     \
case 'x': case 'y': case 'z'

static inline void
py_fix_input_string(PinyinEnhance *pyenhance, char *str, int len)
{
    FcitxIM *im = FcitxInstanceGetCurrentIM(pyenhance->owner);
    if ((!im) || strcmp(im->uniqueName, "shuangpin-libpinyin"))
        return;
    FcitxInputState *input = FcitxInstanceGetInputState(pyenhance->owner);
    /**
     * hopefully the input string (shuang-pin) is shorter than
     * the buffer (which is longer than the full pinyin)
     **/
    strncpy(str, FcitxInputStateGetRawInputBuffer(input), len);
}

#if 0
static INPUT_RETURN_VALUE
FcitxPYEnhanceGetSpellCandWordCb(void *arg, const char *commit)
{
    PinyinEnhance *pyenhance = (PinyinEnhance*)arg;
    FcitxInstance *instance = pyenhance->owner;
    return IRV_TO_PROCESS;
}
#endif

static void
PinyinEnhanceMergeSpellCandList(PinyinEnhance *pyenhance,
                                FcitxCandidateWordList *candList,
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
    /**
     * Check if we have the right number of candidate words,
     * there might be one more because we have raised the limit by one just now.
     * TODO: also do the trick for limit set by PinyinEnhanceSpellHint ?
     **/
    if ((n2 = FcitxCandidateWordGetListSize(newList)) >
        pyenhance->config.max_hint_length) {
        FcitxCandidateWordRemoveByIndex(newList, n2 - 1);
    }
    FcitxCandidateWordMerge(candList, newList, position);
    FcitxCandidateWordFreeList(newList);
}

static boolean
PinyinEnhanceGetSpellCandWords(PinyinEnhance *pyenhance, const char *string,
                               int position, int len_limit)
{
    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input;
    FcitxCandidateWordList *candList;
    FcitxCandidateWordList *newList;
    input = FcitxInstanceGetInputState(instance);
    candList = FcitxInputStateGetCandidateList(input);
    if (len_limit <= 0) {
        len_limit = FcitxCandidateWordGetPageSize(candList) / 2;
        len_limit = len_limit > 0 ? len_limit : 1;
    }
    /**
     * Set the limit to one more than the maximum length in case one of the
     * result is removed because of duplicate
     **/
    if (len_limit > pyenhance->config.max_hint_length)
        len_limit = pyenhance->config.max_hint_length + 1;
    if (position < 0 ||
        (position < 1 && !pyenhance->config.allow_replace_first)) {
        position = 1;
    }
    newList = FcitxSpellGetCandWords(instance, NULL, (void*)string, NULL,
                                     len_limit, "en", "cus", NULL, pyenhance);
    if (!newList)
        return false;
    if (position == 0) {
        const char *preedit_str;
        FcitxMessages *message = FcitxInputStateGetClientPreedit(input);
        preedit_str = FcitxInputStateGetRawInputBuffer(input);
        FcitxMessagesSetMessageCount(message, 0);
        FcitxMessagesAddMessageStringsAtLast(message, MSG_INPUT, preedit_str);
    }
    PinyinEnhanceMergeSpellCandList(pyenhance, candList, newList, position);
    return true;
}

static int
PinyinSpellGetWordType(const char *str, int len)
{
    int i;
    if (len <= 0)
        len = strlen(str);
    if (!strncmp(str, "ng", strlen("ng")))
        return PY_TYPE_FULL;
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

boolean
PinyinEnhanceSpellHint(PinyinEnhance *pyenhance, int im_type)
{
    FcitxInputState *input;
    char *pinyin;
    char *p;
    int spaces = 0;
    int letters = 0;
    int len_limit = -1;
    boolean res = false;
    int pinyin_len;
    FcitxCandidateWordList *cand_list;
    FcitxCandidateWord *cand_word;
    if (!FcitxAddonsIsAddonAvailable(FcitxInstanceGetAddons(pyenhance->owner),
                                     FCITX_SPELL_NAME))
        return false;
    input = FcitxInstanceGetInputState(pyenhance->owner);
    pinyin = FcitxUIMessagesToCString(FcitxInputStateGetPreedit(input));
    if (!pinyin)
        return false;
    if (*fcitx_utils_get_ascii_end(pinyin)) {
        free(pinyin);
        return false;
    }
    pinyin_len = strlen(pinyin);
    cand_list = FcitxInputStateGetCandidateList(input);
    p = pinyin;
    char *last_start = p;
    int words_count = 0;
    int words_type[pinyin_len / 2 + 1];
    do {
        switch (*p) {
        case ' ': {
            int word_len = p - spaces - last_start;
            if (word_len > 0) {
                if (im_type == PY_IM_PINYIN) {
                    words_type[words_count++] =
                        PinyinSpellGetWordType(last_start, word_len);
                } else if (im_type == PY_IM_SHUANGPIN) {
                    words_type[words_count++] = (word_len == 2 ?
                                                 PY_TYPE_FULL :
                                                 PY_TYPE_INVALID);
                }
            }
            last_start = last_start + word_len;
            spaces++;
            continue;
        }
        case_vowel:
        case_consonant:
            letters++;
        default:
            *(p - spaces) = *p;
            break;
        }
    } while (*(p++));
    /* for linpinyin-shuangpin only */
    py_fix_input_string(pyenhance, pinyin, pinyin_len);
    /* not at the end of the string */
    if (*last_start) {
        if (im_type == PY_IM_PINYIN) {
            words_type[words_count++] = PinyinSpellGetWordType(last_start, -1);
        } else if (im_type == PY_IM_SHUANGPIN) {
            words_type[words_count++] = (strlen(last_start) == 2 ?
                                         PY_TYPE_FULL :
                                         PY_TYPE_SHORT);
        }
    } else if (im_type == PY_IM_SHUANGPIN) {
        if (words_type[words_count - 1] != PY_TYPE_FULL)
            words_type[words_count - 1] = PY_TYPE_SHORT;
    }
    cand_word = FcitxCandidateWordGetFirst(cand_list);
    if (!(cand_word && cand_word->strWord && (*cand_word->strWord & 0x80))) {
        len_limit = FcitxCandidateWordGetPageSize(cand_list) - 1;
        res = PinyinEnhanceGetSpellCandWords(pyenhance, pinyin, 0, len_limit);
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
            res = PinyinEnhanceGetSpellCandWords(pyenhance, pinyin,
                                                 eng_ness > 10 ? 0 : 1, len_limit);
            goto out;
        }
        if (eng_ness > 10) {
            res = PinyinEnhanceGetSpellCandWords(pyenhance, pinyin,
                                                 1, len_limit);
            goto out;
        }
    }
    /* pretty random numbers here. */
    if ((letters >= 4) && (spaces * 2 > letters)) {
        res = PinyinEnhanceGetSpellCandWords(pyenhance, pinyin, 1, len_limit);
        goto out;
    }
    if (len_limit > 0) {
        res = PinyinEnhanceGetSpellCandWords(pyenhance, pinyin, 2, len_limit);
        goto out;
    }
out:
    free(pinyin);
    return res;
}
