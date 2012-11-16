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

#include "pinyin-enhance-sym.h"

#include "config.h"

#define PY_SYMBOL_FILE  "pySym.mb"
#define PY_STROKE_FILE  "py_stroke.mb"

static boolean
PySymLoadDict(PinyinEnhance *pyenhance)
{
    FILE *fp;
    boolean res = false;
    if (!pyenhance->config.disable_sym) {
        fp = FcitxXDGGetFileWithPrefix("pinyin", PY_SYMBOL_FILE, "r", NULL);
        if (fp) {
            res = true;
            PinyinEnhanceMapLoad(&pyenhance->sym_table,
                                 pyenhance->sym_pool, fp);
            fclose(fp);
        }
    }
    if (!pyenhance->stroke_table && pyenhance->config.stroke_thresh >= 0) {
        fp = FcitxXDGGetFileWithPrefix("py-enhance",
                                       PY_STROKE_FILE, "r", NULL);
        if (fp) {
            res = true;
            PinyinEnhanceMapLoad(&pyenhance->stroke_table,
                                 pyenhance->stroke_pool, fp);
            fclose(fp);
        }
    }
    return res;
}

boolean
PinyinEnhanceSymInit(PinyinEnhance *pyenhance)
{
    pyenhance->sym_table = NULL;
    pyenhance->stroke_table = NULL;
    pyenhance->sym_pool = fcitx_memory_pool_create();
    pyenhance->stroke_pool = fcitx_memory_pool_create();
    return PySymLoadDict(pyenhance);
}

void
PinyinEnhanceSymReloadDict(PinyinEnhance *pyenhance)
{
    PinyinEnhanceMapClear(&pyenhance->sym_table, pyenhance->sym_pool);
    if (pyenhance->config.disable_sym)
        return;
    PySymLoadDict(pyenhance);
}

static INPUT_RETURN_VALUE
PySymGetCandCb(void *arg, FcitxCandidateWord *cand_word)
{
    PinyinEnhance *pyenhance = arg;
    FcitxInstanceCommitString(pyenhance->owner,
                              FcitxInstanceGetCurrentIC(pyenhance->owner),
                              cand_word->strWord);
    return IRV_FLAG_RESET_INPUT | IRV_FLAG_UPDATE_INPUT_WINDOW;
}

static void
PySymInsertCandidateWords(FcitxCandidateWordList *cand_list,
                          FcitxCandidateWord *cand_temp,
                          PyEnhanceMapWord *words)
{
    for (;words;words = words->next) {
        cand_temp->strWord = strdup(py_enhance_map_word(words));
        FcitxCandidateWordInsert(cand_list, cand_temp, 0);
    }
}

boolean
PinyinEnhanceSymCandWords(PinyinEnhance *pyenhance, int im_type)
{
    FcitxInputState *input = FcitxInstanceGetInputState(pyenhance->owner);
    char *sym = FcitxInputStateGetRawInputBuffer(input);
    int sym_l = strlen(sym);
    PyEnhanceMapWord *words = NULL;
    boolean res = false;
    FcitxCandidateWord cand_word = {
        .strWord = NULL,
        .strExtra = NULL,
        .callback = PySymGetCandCb,
        .wordType = MSG_OTHER,
        .owner = pyenhance,
        .priv = NULL,
    };
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    FcitxMessages *client_preedit = FcitxInputStateGetClientPreedit(input);
    if (!pyenhance->config.disable_sym) {
        words = PinyinEnhanceMapGet(pyenhance->sym_table, sym, sym_l);
        if (words) {
            res = true;
            PySymInsertCandidateWords(cand_list, &cand_word, words);
        }
    }
    if (im_type == PY_IM_PINYIN &&
        pyenhance->config.stroke_thresh >= 0 &&
        pyenhance->config.stroke_thresh <= sym_l &&
        !sym[strspn(sym, "hnpsz")]) {
        words = PinyinEnhanceMapGet(pyenhance->stroke_table, sym, sym_l);
        if (!words) {
            PyEnhanceMap *map = pyenhance->stroke_table;
            int len = 0;
            for (;map;map = py_enhance_map_next(map)) {
                const char *key = py_enhance_map_key(map);
                if (strncmp(key, sym, sym_l) == 0) {
                    int new_len = strlen(key);
                    if (!words || new_len < len) {
                        words = map->words;
                        if (new_len - len <= 1)
                            break;
                        len = new_len;
                    }
                }
            }
        }
        if (words) {
            res = true;
            PySymInsertCandidateWords(cand_list, &cand_word, words);
        }
    }
    if (!res)
        return false;
    FcitxMessagesSetMessageCount(client_preedit, 0);
    FcitxMessagesAddMessageStringsAtLast(client_preedit, MSG_INPUT,
                                         cand_word.strWord);
    return true;
}

void
PinyinEnhanceSymDestroy(PinyinEnhance *pyenhance)
{
    PinyinEnhanceMapClear(&pyenhance->sym_table, pyenhance->sym_pool);
    if (fcitx_likely(pyenhance->sym_pool)) {
        fcitx_memory_pool_destroy(pyenhance->sym_pool);
    }
}
