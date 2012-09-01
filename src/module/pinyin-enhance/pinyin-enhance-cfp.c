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

#include "pinyin-enhance-cfp.h"

#include "config.h"

static void
CharFromPhraseCheckPage(PinyinEnhance *pyenhance)
{
    if (pyenhance->cfp_cur_word == 0)
        return;
    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    if (FcitxCandidateWordGetCurrentPage(cand_list) == pyenhance->cfp_cur_page)
        return;
    pyenhance->cfp_cur_word = 0;
}

static INPUT_RETURN_VALUE
CharFromPhraseStringCommit(PinyinEnhance *pyenhance, FcitxKeySym sym)
{
    char *p;
    int index;

    p = strchr(pyenhance->config.char_from_phrase_str, sym);
    if (!p)
        return IRV_TO_PROCESS;
    index = p - pyenhance->config.char_from_phrase_str;

    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    if (FcitxCandidateWordGetCurrentWindowSize(cand_list)
        <= pyenhance->cfp_cur_word) {
        pyenhance->cfp_cur_word = 0;
    }
    FcitxCandidateWord *cand_word =
        FcitxCandidateWordGetByIndex(cand_list, pyenhance->cfp_cur_word);
    if (!(cand_word && cand_word->strWord &&
          *(p = fcitx_utf8_get_nth_char(cand_word->strWord, index))))
        return IRV_TO_PROCESS;

    int len;
    uint32_t chr;
    char *selected;
    char buff[UTF8_MAX_LENGTH + 1];
    FcitxInputContext *cur_ic = FcitxInstanceGetCurrentIC(instance);
    strncpy(buff, p, UTF8_MAX_LENGTH);
    *fcitx_utf8_get_char(buff, &chr) = '\0';
    selected = PinyinEnhanceGetSelected(pyenhance);
    /* only commit once */
    len = strlen(selected);
    selected = realloc(selected, len + UTF8_MAX_LENGTH + 1);
    strcpy(selected + len, buff);
    FcitxInstanceCommitString(instance, cur_ic, selected);
    free(selected);
    return IRV_FLAG_RESET_INPUT | IRV_FLAG_UPDATE_INPUT_WINDOW;
}

static INPUT_RETURN_VALUE
CharFromPhraseStringSelect(PinyinEnhance *pyenhance, FcitxKeySym sym)
{
#define SET_CUR_WORD(key, index) case key:      \
    pyenhance->cfp_cur_word = index; break

    switch (sym) {
        SET_CUR_WORD(FcitxKey_parenright, 9);
        SET_CUR_WORD(FcitxKey_parenleft, 8);
        SET_CUR_WORD(FcitxKey_asterisk, 7);
        SET_CUR_WORD(FcitxKey_ampersand, 6);
        SET_CUR_WORD(FcitxKey_asciicircum, 5);
        SET_CUR_WORD(FcitxKey_percent, 4);
        SET_CUR_WORD(FcitxKey_dollar, 3);
        SET_CUR_WORD(FcitxKey_numbersign, 2);
        SET_CUR_WORD(FcitxKey_at, 1);
        SET_CUR_WORD(FcitxKey_exclam, 0);
    default:
        return IRV_TO_PROCESS;
    }
#undef SET_CUR_WORD

    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    if (FcitxCandidateWordGetCurrentWindowSize(cand_list)
        <= pyenhance->cfp_cur_word) {
        pyenhance->cfp_cur_word = 0;
        return IRV_TO_PROCESS;
    }
    pyenhance->cfp_cur_page = FcitxCandidateWordGetCurrentPage(cand_list);
    return IRV_DO_NOTHING;
}

static INPUT_RETURN_VALUE
CharFromPhraseString(PinyinEnhance *pyenhance,
                    FcitxKeySym sym, unsigned int state)
{
    FcitxKeySym keymain = FcitxHotkeyPadToMain(sym);
    INPUT_RETURN_VALUE retval;

    if (!(pyenhance->config.char_from_phrase_str &&
          *pyenhance->config.char_from_phrase_str))
        return IRV_TO_PROCESS;
    if (!FcitxHotkeyIsHotKeySimple(keymain, state))
        return IRV_TO_PROCESS;
    if ((retval = CharFromPhraseStringCommit(pyenhance, keymain)))
        return retval;
    if ((retval = CharFromPhraseStringSelect(pyenhance, keymain)))
        return retval;
    return IRV_TO_PROCESS;
}

boolean
PinyinEnhanceCharFromPhrasePost(PinyinEnhance *pyenhance, FcitxKeySym sym,
                                unsigned int state, INPUT_RETURN_VALUE *retval)
{
    CharFromPhraseCheckPage(pyenhance);
    if (*retval)
        return false;
    return false;
}

boolean
PinyinEnhanceCharFromPhrasePre(PinyinEnhance *pyenhance, FcitxKeySym sym,
                               unsigned int state, INPUT_RETURN_VALUE *retval)
{
    CharFromPhraseCheckPage(pyenhance);
    if ((*retval = CharFromPhraseString(pyenhance, sym, state)))
        return true;
    return false;
}

void
PinyinEnhanceCharFromPhraseCandidate(PinyinEnhance *pyenhance)
{
    pyenhance->cfp_cur_word = 0;
    pyenhance->cfp_cur_page = 0;
}
