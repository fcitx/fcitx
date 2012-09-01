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

static INPUT_RETURN_VALUE
CharFromParseString(PinyinEnhance *pyenhance,
                    FcitxKeySym sym, unsigned int state)
{
    FcitxKeySym keymain = FcitxHotkeyPadToMain(sym);
    char *p;
    int index;

    if (!(pyenhance->config.char_from_phrase_str &&
          FcitxHotkeyIsHotKeySimple(keymain, state)))
        return IRV_TO_PROCESS;
    p = strchr(pyenhance->config.char_from_phrase_str, keymain);
    if (!p)
        return IRV_TO_PROCESS;
    index = p - pyenhance->config.char_from_phrase_str;

    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input;
    FcitxCandidateWordList *cand_list;
    FcitxCandidateWord *cand_word;
    input = FcitxInstanceGetInputState(instance);
    cand_list = FcitxInputStateGetCandidateList(input);
    cand_word = FcitxCandidateWordGetCurrentWindow(cand_list);
    if (!(cand_word && cand_word->strWord &&
          *(p = fcitx_utf8_get_nth_char(cand_word->strWord, index))))
        return IRV_TO_PROCESS;

    uint32_t chr;
    char buff[UTF8_MAX_LENGTH + 1];
    FcitxInputContext *cur_ic = FcitxInstanceGetCurrentIC(instance);
    char *selected;
    int len;
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

boolean
PinyinEnhanceCharFromPhrasePost(PinyinEnhance *pyenhance, FcitxKeySym sym,
                                unsigned int state, INPUT_RETURN_VALUE *retval)
{
    if (*retval)
        return false;
    return false;
}

boolean
PinyinEnhanceCharFromPhrasePre(PinyinEnhance *pyenhance, FcitxKeySym sym,
                               unsigned int state, INPUT_RETURN_VALUE *retval)
{
    if ((*retval = CharFromParseString(pyenhance, sym, state)))
        return true;
    return false;
}
