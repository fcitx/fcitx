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

static boolean
PySymLoadDict(PinyinEnhance *pyenhance)
{
    FILE *fp = FcitxXDGGetFileWithPrefix("pinyin", PY_SYMBOL_FILE, "r", NULL);
    if (!fp)
        return false;
    PinyinEnhanceMapLoad(&pyenhance->sym_table, pyenhance->sym_pool, fp);
    fclose(fp);
    return true;
}

boolean
PinyinEnhanceSymInit(PinyinEnhance *pyenhance)
{
    pyenhance->sym_table = NULL;
    pyenhance->sym_pool = fcitx_memory_pool_create();
    if (pyenhance->config.disable_sym || !pyenhance->sym_pool) {
        return false;
    }
    return PySymLoadDict(pyenhance);
}

void
PinyinEnhanceReloadDict(PinyinEnhance *pyenhance)
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

boolean
PinyinEnhanceSymCandWords(PinyinEnhance *pyenhance)
{
    PyEnhanceMap *table = pyenhance->sym_table;
    if ((!table) || pyenhance->config.disable_sym)
        return false;
    FcitxInputState *input = FcitxInstanceGetInputState(pyenhance->owner);
    char *sym = FcitxInputStateGetRawInputBuffer(input);
    int sym_l = strlen(sym);
    PyEnhanceMapWord *words = PinyinEnhanceMapGet(table, sym, sym_l);
    if (!words)
        return false;
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
    for (;words;words = words->next) {
        cand_word.strWord = strdup(py_enhance_map_word(words));
        FcitxCandidateWordInsert(cand_list, &cand_word, 0);
    }
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
