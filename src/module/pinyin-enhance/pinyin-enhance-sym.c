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

#include "fcitx-utils/uthash.h"
#include "pinyin-enhance-sym.h"

#include "config.h"

#define PY_SYMBOL_FILE  "pySym.mb"

typedef struct _PySymWord PySymWord;

struct _PySymWord {
    PySymWord *next;
};

static inline void*
sym_word(PySymWord *word)
{
    return ((void*)word) + sizeof(PySymWord);
}

struct _PySymTable {
    PySymWord *words;
    UT_hash_handle hh;
};

static inline void*
table_sym(PySymTable *table)
{
    return (void*)table + sizeof(PySymTable);
}

#define SYM_BLANK " \t\b\r\n"

static void
PinyinEnhanceAddSym(PinyinEnhance *pyenhance, const char *sym, int sym_l,
                    const char *word, int word_l)
{
    PySymTable *table;
    PySymWord *py_word;
    word_l++;
    py_word = fcitx_memory_pool_alloc_align(pyenhance->sym_pool,
                                            sizeof(PySymWord) + word_l, 1);
    memcpy(sym_word(py_word), word, word_l);
    HASH_FIND_STR(pyenhance->sym_table, sym, table);
    if (table) {
        py_word->next = table->words;
        table->words = py_word;
    } else {
        /**
         * the + 1 here is actually not necessary,
         * just to make printing easier
         **/
        table = fcitx_memory_pool_alloc_align(pyenhance->sym_pool,
                                              sizeof(PySymTable) + sym_l + 1,
                                              1);
        table->words = py_word;
        py_word->next = NULL;
        memcpy(table_sym(table), sym, sym_l + 1);
        HASH_ADD_KEYPTR(hh, pyenhance->sym_table,
                        table_sym(table), sym_l, table);
    }
}

static PySymWord*
PinyinEnhanceGetSym(PinyinEnhance *pyenhance, const char *sym)
{
    PySymTable *table;
    HASH_FIND_STR(pyenhance->sym_table, sym, table);
    if (table) {
        return table->words;
    }
    return NULL;
}

static void
PySymClearDict(PinyinEnhance *pyenhance)
{
    if (pyenhance->sym_table) {
        HASH_CLEAR(hh, pyenhance->sym_table);
        pyenhance->sym_table = NULL;
    }
    if (pyenhance->sym_pool)
        fcitx_memory_pool_clear(pyenhance->sym_pool);
}

static boolean
PySymLoadDict(PinyinEnhance *pyenhance)
{
    FILE *fp = FcitxXDGGetFileWithPrefix("pinyin", PY_SYMBOL_FILE, "r", NULL);
    if (!fp)
        return false;
    char *buff = NULL;
    char *sym;
    char *word;
    int sym_l;
    int word_l;
    size_t len;
    while (getline(&buff, &len, fp) != -1) {
        /* remove leading spaces */
        sym = buff + strspn(buff, SYM_BLANK);
        /* empty line or comment */
        if (*sym == '\0' || *sym == '#')
            continue;
        /* find delimiter */
        sym_l = strcspn(sym, SYM_BLANK);
        word = sym + sym_l;
        *word = '\0';
        word++;
        /* find start of word */
        word = word + strspn(word, SYM_BLANK);
        word_l = strcspn(word, SYM_BLANK);
        if (!(word_l && sym_l))
            continue;
        word[word_l] = '\0';
        PinyinEnhanceAddSym(pyenhance, sym, sym_l, word, word_l);
    }

    if (buff)
        free(buff);
    fclose(fp);
    return true;
}

boolean
PinyinEnhanceSymInit(PinyinEnhance *pyenhance)
{
    pyenhance->sym_table = NULL;
    if (pyenhance->config.disable_sym) {
        pyenhance->sym_pool = NULL;
        return false;
    }
    pyenhance->sym_pool = fcitx_memory_pool_create();
    if (!pyenhance->sym_pool)
        return false;
    return PySymLoadDict(pyenhance);
}

void
PinyinEnhanceReloadDict(PinyinEnhance *pyenhance)
{
    PySymClearDict(pyenhance);
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
    PySymTable *table = pyenhance->sym_table;
    if ((!table) || pyenhance->config.disable_sym)
        return false;
    FcitxInputState *input = FcitxInstanceGetInputState(pyenhance->owner);
    char *sym = FcitxInputStateGetRawInputBuffer(input);
    PySymWord *words = PinyinEnhanceGetSym(pyenhance, sym);
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
        cand_word.strWord = strdup(sym_word(words));
        FcitxCandidateWordInsert(cand_list, &cand_word, 0);
    }
    FcitxMessagesSetMessageCount(client_preedit, 0);
    FcitxMessagesAddMessageAtLast(client_preedit, MSG_INPUT, "%s",
                                  cand_word.strWord);
    return true;
}

void
PinyinEnhanceSymDestroy(PinyinEnhance *pyenhance)
{
    PySymClearDict(pyenhance);
    if (pyenhance->sym_pool)
        fcitx_memory_pool_destroy(pyenhance->sym_pool);
}
