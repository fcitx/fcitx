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
#include "fcitx/keys.h"

static void CharFromPhraseFreeStrList(char **list)
{
    char **p;
    for (p = list;*p;p++)
        free(*p);
    free(list);
}

static void
CharFromPhraseModeReset(PinyinEnhance *pyenhance)
{
    if (pyenhance->cfp_mode_lists) {
        int i;
        for (i = 0;i < pyenhance->cfp_mode_count;i++)
            CharFromPhraseFreeStrList(pyenhance->cfp_mode_lists[i]);
        free(pyenhance->cfp_mode_lists);
        pyenhance->cfp_mode_lists = NULL;
    }
    if (pyenhance->cfp_mode_selected) {
        free(pyenhance->cfp_mode_selected);
        pyenhance->cfp_mode_selected = NULL;
    }
    pyenhance->cfp_mode_cur = 0;
    pyenhance->cfp_mode_count = 0;
}

static INPUT_RETURN_VALUE
CharFromPhraseModeGetCandCb(void *arg, FcitxCandidateWord *cand_word)
{
    PinyinEnhance *pyenhance = (PinyinEnhance*)arg;
    int len = strlen(pyenhance->cfp_mode_selected);
    int len2 = strlen(cand_word->strWord);
    pyenhance->cfp_mode_selected = realloc(pyenhance->cfp_mode_selected,
                                           len + len2 + 1);
    memcpy(pyenhance->cfp_mode_selected + len, cand_word->strWord, len2 + 1);
    FcitxInstanceCommitString(pyenhance->owner,
                              FcitxInstanceGetCurrentIC(pyenhance->owner),
                              pyenhance->cfp_mode_selected);
    return IRV_FLAG_RESET_INPUT | IRV_FLAG_UPDATE_INPUT_WINDOW;
}

static void
CharFromPhraseModeInitCandword(PinyinEnhance *pyenhance,
                               FcitxCandidateWord *cand_word)
{
    cand_word->strWord = malloc(UTF8_MAX_LENGTH + 1);
    cand_word->strWord[UTF8_MAX_LENGTH] = '\0';
    cand_word->strExtra = NULL;
    cand_word->callback = CharFromPhraseModeGetCandCb;
    cand_word->wordType = MSG_OTHER;
    cand_word->owner = pyenhance;
    cand_word->priv = NULL;
}

static boolean
CandwordIsCharFromPhrase(PinyinEnhance *pyenhance,
                         FcitxCandidateWord *cand_word)
{
    return (cand_word->callback == CharFromPhraseModeGetCandCb &&
            cand_word->owner == pyenhance);
}

static void
CharFromPhraseSetClientPreedit(PinyinEnhance *pyenhance, const char *str)
{
    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxMessages *client_preedit = FcitxInputStateGetClientPreedit(input);
    FcitxMessagesSetMessageCount(client_preedit, 0);
    FcitxMessagesAddMessageStringsAtLast(client_preedit, MSG_INPUT,
                                         pyenhance->cfp_mode_selected, str);
}

static void
CharFromPhraseModeUpdateUI(PinyinEnhance *pyenhance)
{
    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    FcitxMessages *preedit = FcitxInputStateGetPreedit(input);
    char **cur_list = pyenhance->cfp_mode_lists[pyenhance->cfp_mode_cur];
    FcitxCandidateWordSetPage(cand_list, 0);
    FcitxMessagesSetMessageCount(preedit, 0);
    FcitxMessagesAddMessageStringsAtLast(preedit, MSG_INPUT,
                                         pyenhance->cfp_mode_selected, " (",
                                         cur_list[0], ")");
    CharFromPhraseSetClientPreedit(pyenhance, *(++cur_list));
    FcitxInputStateSetShowCursor(input, false);
    int i;
    FcitxCandidateWord *cand_word;
    /* use existing cand_word added before if they exist */
    for (i = 0;(cand_word = FcitxCandidateWordGetByTotalIndex(cand_list, i));
         i++) {
        if (CandwordIsCharFromPhrase(pyenhance, cand_word)) {
            strncpy(cand_word->strWord, *cur_list, UTF8_MAX_LENGTH);
            cur_list++;
            if (!*cur_list) {
                break;
            }
        }
    }
    if (*cur_list) {
        FcitxCandidateWord new_word;
        for (;*cur_list;cur_list++) {
            CharFromPhraseModeInitCandword(pyenhance, &new_word);
            strncpy(new_word.strWord, *cur_list, UTF8_MAX_LENGTH);
            FcitxCandidateWordAppend(cand_list, &new_word);
        }
    } else {
        for (i++;
             (cand_word = FcitxCandidateWordGetByTotalIndex(cand_list, i));) {
            if (!CandwordIsCharFromPhrase(pyenhance, cand_word)) {
                i++;
                continue;
            }
            FcitxCandidateWordRemoveByIndex(cand_list, i);
        }
    }
}

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
    if (!(cand_word && cand_word->strWord))
        return IRV_TO_PROCESS;

    if (!(*fcitx_utils_get_ascii_end(cand_word->strWord) &&
          *(p = fcitx_utf8_get_nth_char(cand_word->strWord, index))))
        return IRV_DO_NOTHING;

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
    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    if (FcitxInputStateGetIsInRemind(input))
        return IRV_TO_PROCESS;
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
CharFromPhraseString(PinyinEnhance *pyenhance, FcitxKeySym sym,
                     unsigned int state)
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

static void
CharFromPhraseSyncPreedit(PinyinEnhance *pyenhance,
                          FcitxCandidateWordList *cand_list)
{
    FcitxCandidateWord *cand_word;
    cand_word = FcitxCandidateWordGetCurrentWindow(cand_list);
    if (cand_word && cand_word->strWord) {
        CharFromPhraseSetClientPreedit(pyenhance, cand_word->strWord);
    } else {
        CharFromPhraseSetClientPreedit(pyenhance, "");
    }
}

static INPUT_RETURN_VALUE
CharFromPhraseModePre(PinyinEnhance *pyenhance, FcitxKeySym sym,
                      unsigned int state)
{
    if (!pyenhance->cfp_active)
        return IRV_TO_PROCESS;
    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    FcitxGlobalConfig *config = FcitxInstanceGetGlobalConfig(instance);

    int index = FcitxCandidateWordCheckChooseKey(cand_list, sym, state);
    if (index >= 0)
        return FcitxCandidateWordChooseByIndex(cand_list, index);
    if (FcitxHotkeyIsHotKey(sym, state, config->hkPrevPage)) {
        if (FcitxCandidateWordGoPrevPage(cand_list)) {
            CharFromPhraseSyncPreedit(pyenhance, cand_list);
            return IRV_FLAG_UPDATE_INPUT_WINDOW;
        }
        if (pyenhance->cfp_mode_cur > 0) {
            pyenhance->cfp_mode_cur--;
            CharFromPhraseModeUpdateUI(pyenhance);
            return IRV_FLAG_UPDATE_INPUT_WINDOW;
        }
        return IRV_DO_NOTHING;
    } else if (FcitxHotkeyIsHotKey(sym, state, config->hkNextPage)) {
        if (FcitxCandidateWordGoNextPage(cand_list)) {
            CharFromPhraseSyncPreedit(pyenhance, cand_list);
            return IRV_FLAG_UPDATE_INPUT_WINDOW;
        }
        if (pyenhance->cfp_mode_cur < (pyenhance->cfp_mode_count - 1)) {
            pyenhance->cfp_mode_cur++;
            CharFromPhraseModeUpdateUI(pyenhance);
            return IRV_FLAG_UPDATE_INPUT_WINDOW;
        }
        return IRV_DO_NOTHING;
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_HOME)) {
        pyenhance->cfp_mode_cur = 0;
        CharFromPhraseModeUpdateUI(pyenhance);
        return IRV_FLAG_UPDATE_INPUT_WINDOW;
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE)) {
        return FcitxCandidateWordChooseByIndex(cand_list, 0);
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_END)) {
        pyenhance->cfp_mode_cur = pyenhance->cfp_mode_count - 1;
        CharFromPhraseModeUpdateUI(pyenhance);
        return IRV_FLAG_UPDATE_INPUT_WINDOW;
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER)) {
        int len = strlen(pyenhance->cfp_mode_selected);
        char *cur_full = *pyenhance->cfp_mode_lists[pyenhance->cfp_mode_cur];
        int len2 = strlen(cur_full);
        pyenhance->cfp_mode_selected = realloc(pyenhance->cfp_mode_selected,
                                               len + len2 + 1);
        memcpy(pyenhance->cfp_mode_selected + len, cur_full, len2 + 1);
        FcitxInstanceCommitString(pyenhance->owner,
                                  FcitxInstanceGetCurrentIC(pyenhance->owner),
                                  pyenhance->cfp_mode_selected);
        return IRV_FLAG_RESET_INPUT | IRV_FLAG_UPDATE_INPUT_WINDOW;
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
        FcitxInstanceCommitString(pyenhance->owner,
                                  FcitxInstanceGetCurrentIC(pyenhance->owner),
                                  pyenhance->cfp_mode_selected);
        return IRV_FLAG_RESET_INPUT | IRV_FLAG_UPDATE_INPUT_WINDOW;
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
        return IRV_FLAG_RESET_INPUT | IRV_FLAG_UPDATE_INPUT_WINDOW;
    }

    return IRV_DO_NOTHING;
}

/**
 * First element is the original string with ascii characters removed (if any)
 **/
static char**
CharFromPhraseModeListFromWord(const char *word)
{
    if (!(word && *(word = fcitx_utils_get_ascii_end(word))))
        return NULL;
    int n = 0;
    uint32_t chr;
    char *p;
    int len = strlen(word);
    char *buff[len / 2];
    char full[len + 1];
    full[0] = '\0';
    p = fcitx_utf8_get_char(word, &chr);
    if (!*p)
        return NULL;
    do {
        if ((len = p - word) > 1) {
            buff[n] = fcitx_utils_set_str_with_len(NULL, word, len);
            strncat(full, word, len);
            n++;
        }
        if (!*p)
            break;
        word = p;
        p = fcitx_utf8_get_char(word, &chr);
    } while(true);

    /* impossible though */
    if (n <= 0)
        return NULL;
    if (n == 1) {
        free(buff[0]);
        return NULL;
    }

    char **res = malloc(sizeof(char*) * (n + 2));
    res[0] = strdup(full);
    res[n + 1] = NULL;
    for (;n > 0;n--) {
        res[n] = buff[n - 1];
    }
    return res;
}

static boolean
CharFromPhraseModeGetCandLists(PinyinEnhance *pyenhance)
{
    FcitxInstance *instance = pyenhance->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    int size = FcitxCandidateWordGetCurrentWindowSize(cand_list);
    char **lists[size];
    int n = 0;
    int i;
    FcitxCandidateWord *cand_word;
    int cur = 0;
    for (i = 0;i < size;i++) {
        cand_word = FcitxCandidateWordGetByIndex(cand_list, i);
        /* try to turn a candidate word into string list */
        if (cand_word &&
            (lists[n] = CharFromPhraseModeListFromWord(cand_word->strWord))) {
            /* use the same current word index as cfp_string */
            if (i == pyenhance->cfp_cur_word)
                cur = n;
            n++;
        }
    }
    if (!n)
        return false;

    pyenhance->cfp_mode_cur = cur;
    pyenhance->cfp_mode_count = n;
    pyenhance->cfp_mode_lists = malloc(sizeof(char**) * n);
    memcpy(pyenhance->cfp_mode_lists, lists, sizeof(char**) * n);
    return true;
}

static INPUT_RETURN_VALUE
CharFromPhraseModePost(PinyinEnhance *pyenhance, FcitxKeySym sym,
                       unsigned int state)
{
    if (!FcitxHotkeyIsHotKey(sym, state,
                             pyenhance->config.char_from_phrase_key))
        return IRV_TO_PROCESS;
    if (!CharFromPhraseModeGetCandLists(pyenhance))
        return IRV_TO_PROCESS;
    pyenhance->cfp_mode_selected = PinyinEnhanceGetSelected(pyenhance);
    pyenhance->cfp_active = true;
    FcitxInstanceCleanInputWindow(pyenhance->owner);
    CharFromPhraseModeUpdateUI(pyenhance);
    return IRV_FLAG_UPDATE_INPUT_WINDOW;
}

boolean
PinyinEnhanceCharFromPhrasePost(PinyinEnhance *pyenhance, FcitxKeySym sym,
                                unsigned int state, INPUT_RETURN_VALUE *retval)
{
    CharFromPhraseCheckPage(pyenhance);
    CharFromPhraseModeReset(pyenhance);
    if (*retval)
        return false;
    if ((*retval = CharFromPhraseModePost(pyenhance, sym, state)))
        return true;
    return false;
}

boolean
PinyinEnhanceCharFromPhrasePre(PinyinEnhance *pyenhance, FcitxKeySym sym,
                               unsigned int state, INPUT_RETURN_VALUE *retval)
{
    CharFromPhraseCheckPage(pyenhance);
    if ((*retval = CharFromPhraseString(pyenhance, sym, state)))
        return true;
    if ((*retval = CharFromPhraseModePre(pyenhance, sym, state)))
        return true;
    return false;
}

void
PinyinEnhanceCharFromPhraseCandidate(PinyinEnhance *pyenhance)
{
    pyenhance->cfp_cur_word = 0;
    pyenhance->cfp_cur_page = 0;
    CharFromPhraseModeReset(pyenhance);
}

void
PinyinEnhanceCharFromPhraseReset(PinyinEnhance *pyenhance)
{
    pyenhance->cfp_cur_word = 0;
    pyenhance->cfp_cur_page = 0;
    pyenhance->cfp_active = false;
    CharFromPhraseModeReset(pyenhance);
}
