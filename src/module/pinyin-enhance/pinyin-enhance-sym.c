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
#include "pinyin-enhance-stroke.h"

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
    if (!pyenhance->stroke_loaded && pyenhance->config.stroke_thresh >= 0) {
        pyenhance->stroke_loaded = true;
        /* struct timespec start, end; */
        /* int t; */
        char *fname;
        fname = fcitx_utils_get_fcitx_path_with_filename(
            "pkgdatadir", "py-enhance/"PY_STROKE_FILE);
        fp = fopen(fname, "r");
        free(fname);
        if (fp) {
            /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); */
            res = true;
            py_enhance_stroke_load_tree(&pyenhance->stroke_tree,
                                        fp, pyenhance->stroke_pool);
            /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end); */
            /* t = ((end.tv_sec - start.tv_sec) * 1000000000) */
            /*     + end.tv_nsec - start.tv_nsec; */
            /* printf("%s, %d\n", __func__, t); */
            fclose(fp);
        }
    }
    return res;
}

boolean
PinyinEnhanceSymInit(PinyinEnhance *pyenhance)
{
    pyenhance->sym_table = NULL;
    pyenhance->stroke_loaded = false;
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
                          const PyEnhanceMapWord *words, int index)
{
    for (;words;words = words->next) {
        cand_temp->strWord = strdup(py_enhance_map_word(words));
        FcitxCandidateWordInsert(cand_list, cand_temp, index);
    }
}

static int
PinyinEnhanceStrokeInsertIndex(FcitxCandidateWordList *cand_list,
                               int im_type, int sym_l)
{
    FcitxCandidateWord *orig_cand;
    orig_cand = FcitxCandidateWordGetFirst(cand_list);
    const char *orig_word = orig_cand ? orig_cand->strWord : NULL;
    int orig_len = orig_word ? fcitx_utf8_strlen(orig_word) : 0;
    if (im_type == PY_IM_PINYIN) {
        return (orig_len == 1 && (*orig_word & 0x80)) ? 1 : 0;
    } else if (im_type == PY_IM_SHUANGPIN) {
        if (!orig_len || !(*orig_word & 0x80)) {
            return 0;
        } else if (sym_l > 4 ||
                   !FcitxCandidateWordGetByTotalIndex(cand_list, 1)) {
            return 1;
        } else {
            return 2;
        }
    }
    return -1;
}

boolean
PinyinEnhanceSymCandWords(PinyinEnhance *pyenhance, int im_type)
{
    FcitxInputState *input = FcitxInstanceGetInputState(pyenhance->owner);
    char *sym = FcitxInputStateGetRawInputBuffer(input);
    int sym_l = strlen(sym);
    if (!sym_l)
        return false;
    PyEnhanceMapWord *words = NULL;
    boolean res = false;
    char *preedit_str = NULL;
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
            PySymInsertCandidateWords(cand_list, &cand_word, words, 0);
            preedit_str = cand_word.strWord;
        }
    }
    if (pyenhance->config.stroke_thresh >= 0 &&
        pyenhance->config.stroke_thresh <= sym_l &&
        !sym[strspn(sym, "hnpsz")]) {
        /* struct timespec start, end; */
        /* int t; */
        /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); */
#define PY_STROKE_BUFF_SIZE 5
        const PyEnhanceMapWord *word_buff[PY_STROKE_BUFF_SIZE];
        int count = py_enhance_stroke_get_match_keys(
            pyenhance, sym, sym_l, word_buff, PY_STROKE_BUFF_SIZE);
        /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end); */
        /* t = ((end.tv_sec - start.tv_sec) * 1000000000) */
        /*     + end.tv_nsec - start.tv_nsec; */
        /* printf("%s, %d\n", __func__, t); */
        if (count) {
            int index;
            /* py_sym inserted */
            if (res) {
                index = 1;
            } else {
                index = PinyinEnhanceStrokeInsertIndex(cand_list, im_type,
                                                       sym_l);
            }
            if (index >= 0) {
                res = true;
                FcitxCandidateWordList *new_list;
                new_list =  FcitxCandidateWordNewList();
                int i;
                for (i = 0;i < count;i++) {
                    PySymInsertCandidateWords(new_list, &cand_word,
                                              word_buff[i], 0);
                    int size = FcitxCandidateWordGetListSize(new_list);
                    if (size > 5)
                        break;
                }
                FcitxCandidateWordMerge(cand_list, new_list, index);
                FcitxCandidateWordFreeList(new_list);
                if (index == 0) {
                    preedit_str = cand_word.strWord;
                }
            }
        }
    }
    if (!res)
        return false;
    if (preedit_str) {
        FcitxMessagesSetMessageCount(client_preedit, 0);
        FcitxMessagesAddMessageStringsAtLast(client_preedit, MSG_INPUT,
                                             preedit_str);
    }
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
