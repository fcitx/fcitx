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
#include "pinyin-enhance-py.h"

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
            py_enhance_stroke_load_tree(&pyenhance->stroke_tree, fp);
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

static inline boolean
_str_is_single_char(const char *str)
{
    const char prob[2] = {str[0]};
    return !str[strspn(str, prob)];
}

static int
PinyinEnhanceStrokeInsertIndex(FcitxCandidateWordList *cand_list,
                               int im_type, const char *sym, int sym_l)
{
    FcitxCandidateWord *orig_cand;
    orig_cand = FcitxCandidateWordGetFirst(cand_list);
    const char *orig_word = orig_cand ? orig_cand->strWord : NULL;
    int orig_len = orig_word ? fcitx_utf8_strlen(orig_word) : 0;
    if (!orig_len || !(*orig_word & 0x80))
        return 0;
    if (im_type == PY_IM_PINYIN) {
        if (orig_len <= 2)
            return 1;
        if (_str_is_single_char(sym)) {
            if (orig_len > 4)
                return 1;
            goto return_2;
        }
        return 0;
    } else if (im_type == PY_IM_SHUANGPIN) {
        if (sym_l > 4)
            return 1;
        goto return_2;
    }
    return -1;
return_2:
    if (FcitxCandidateWordGetByTotalIndex(cand_list, 1))
        return 2;
    return 1;
}

static char*
PinyinEnhanceGetAllPinyin(PinyinEnhance *pyenhance, const char *str)
{
    const int8_t *py_list;
    py_list = py_enhance_py_find_py(pyenhance, str);
    if (!(py_list && *py_list))
        return NULL;
    int i;
    const int8_t *py;
    char buff[64];
    int alloc_len = 16 * *py_list + 4;
    char *res = malloc(alloc_len);
    int len = 2;
    memcpy(res, " (", 2);
    for (i = 0;i < *py_list;i++) {
        py = pinyin_enhance_pylist_get(py_list, i);
        int py_len = 0;
        py_enhance_py_to_str(buff, py, &py_len);
        if (len + py_len + 4 >= alloc_len) {
            alloc_len = len + py_len + 5;
            res = realloc(res, alloc_len);
        }
        memcpy(res + len, buff, py_len);
        len += py_len;
        memcpy(res + len, ", ", 2);
        len += 2;
    }
    memcpy(res + len - 2, ")", 2);
    return res;
}

boolean
PinyinEnhanceSymCandWords(PinyinEnhance *pyenhance, int im_type)
{
    FcitxInputState *input = FcitxInstanceGetInputState(pyenhance->owner);
    const char *sym = FcitxInputStateGetRawInputBuffer(input);
    int sym_l = strlen(sym);
    if (!sym_l)
        return false;
    const PyEnhanceMapWord *words = NULL;
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
        pyenhance->config.stroke_limit > 0 &&
        !sym[strspn(sym, "hnpsz")]) {
        if (fcitx_unlikely(pyenhance->config.stroke_limit > 10))
            pyenhance->config.stroke_limit = 10;
        const int limit = pyenhance->config.stroke_limit;
        /* struct timespec start, end; */
        /* int t; */
        /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); */
        PyEnhanceStrokeWord *word_buff[10];
        int count = py_enhance_stroke_get_match_keys(
            pyenhance, sym, sym_l, word_buff, limit);
        /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end); */
        /* t = ((end.tv_sec - start.tv_sec) * 1000000000) */
        /*     + end.tv_nsec - start.tv_nsec; */
        /* printf("%s, %d\n", __func__, t); */
        /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); */
        if (count) {
            int index;
            /* py_sym inserted */
            if (res) {
                index = 1;
            } else {
                index = PinyinEnhanceStrokeInsertIndex(cand_list, im_type,
                                                       sym, sym_l);
            }
            if (index >= 0) {
                res = true;
                FcitxCandidateWordList *new_list;
                new_list =  FcitxCandidateWordNewList();
                int i;
                int size = 0;
                const PyEnhanceStrokeWord *words;
                for (i = 0;i < count;i++) {
                    for (words = word_buff[i];words;
                         py_enhance_stroke_word_tonext(&pyenhance->stroke_tree,
                                                       &words)) {
                        const char *str_word = words->word;
                        cand_word.strWord = strdup(str_word);
                        cand_word.strExtra = PinyinEnhanceGetAllPinyin(
                            pyenhance, str_word);
                        FcitxCandidateWordAppend(new_list, &cand_word);
                    }
                    size = FcitxCandidateWordGetListSize(new_list);
                    if (size >= limit) {
                        break;
                    }
                }
                if (index == 0 && size > 0) {
                    preedit_str = FcitxCandidateWordGetFirst(new_list)->strWord;
                }
                FcitxCandidateWordMerge(cand_list, new_list, index);
                FcitxCandidateWordFreeList(new_list);
            }
        }
        /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end); */
        /* t = ((end.tv_sec - start.tv_sec) * 1000000000) */
        /*     + end.tv_nsec - start.tv_nsec; */
        /* printf("%s, %d\n", __func__, t); */
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
    if (fcitx_likely(pyenhance->sym_pool)) {
        fcitx_memory_pool_destroy(pyenhance->sym_pool);
    }
}
