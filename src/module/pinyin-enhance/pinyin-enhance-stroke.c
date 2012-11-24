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

#include "fcitx-utils/utils.h"
#include "fcitx-utils/memory.h"
#include "pinyin-enhance-stroke.h"

static inline char
py_enhance_stroke_sym_to_num(char c)
{
    switch (c) {
    case 'h':
        return '1';
    case 's':
        return '2';
    case 'p':
        return '3';
    case 'n':
        return '4';
    case 'z':
        return '5';
    }
    return '\0';
}

typedef struct {
    const PyEnhanceStrokeKey *key;
    const char *key_s;
    int diff;
    int key_l;
} PyEnhanceStrokeKeyLookup;

typedef struct {
    const PyEnhanceMapWord *word;
    int distance;
} PyEnhanceStrokeResult;

#define REPLACE_WEIGHT 5
#define INSERT_WEIGHT 5
#define REMOVE_WEIGHT 5
#define END_WEIGHT 1
static inline int
py_enhance_stroke_get_distance(const char *word, int word_len,
                               const char *dict, int dict_len)
{
    int replace = 0;
    int insert = 0;
    int remove = 0;
    int diff = 0;
    int maxdiff;
    int maxremove;
    int word_i = 0;
    int dict_i = 0;
    maxdiff = word_len / 3;
    maxremove = (word_len - 2) / 3;
    while ((diff = replace + insert + remove) <= maxdiff &&
           remove <= maxremove) {
        if (!word[word_i]) {
            return ((replace * REPLACE_WEIGHT + insert * INSERT_WEIGHT
                     + remove * REMOVE_WEIGHT)
                    + (dict_len - dict_i) * END_WEIGHT);
        }
        /* check remove error */
        if (!dict[dict_i]) {
            if (dict[dict_i + 1]) {
                return -1;
            } else {
                remove++;
                if (diff <= maxdiff && remove <= maxremove) {
                    return (replace * REPLACE_WEIGHT + insert * INSERT_WEIGHT
                            + remove * REMOVE_WEIGHT);
                }
                return -1;
            }
        }
        if (word[word_i] == dict[dict_i]) {
            word_i++;
            dict_i++;
            continue;
        }
        if (word[word_i + 1] == dict[dict_i]) {
            word_i += 2;
            dict_i++;
            remove++;
            continue;
        }

        /* check insert error */
        if (word[word_i] == dict[dict_i + 1]) {
            word_i++;
            dict_i += 2;
            insert++;
            continue;
        }

        /* check replace error */
        if (word[word_i + 1] == dict[dict_i + 1]) {
            if (word[word_i + 1]) {
                dict_i += 2;
                word_i += 2;
            } else {
                word_i++;
                dict_i++;
            }
            replace++;
            continue;
        }
        break;
    }
    return -1;
}

int
py_enhance_stroke_get_match_keys(PinyinEnhance *pyenhance, const char *key_s,
                                 int key_l, const PyEnhanceMapWord **word_buff,
                                 int buff_len)
{
    const PyEnhanceStrokeTree *tree;
    int i;
    int count = 0;
    char *key_buff = malloc(key_l + 1);
    for (i = 0;i < key_l;i++) {
        key_buff[i] = py_enhance_stroke_sym_to_num(key_s[i]);
    }
    key_buff[key_l] = '\0';
    key_buff[0] -= '1';
    tree = &pyenhance->stroke_tree;
    if (buff_len > 16)
        buff_len = 16;
    switch (key_l) {
    case 1: {
        const PyEnhanceMapWord *tmp_word;
        tmp_word = tree->singles[(int)key_buff[0]];
        if (tmp_word) {
            word_buff[0] = tmp_word;
            count++;
            if (count >= buff_len) {
                goto out;
            }
        }
        PyEnhanceMapWord *const *tmp_word_p;
        tmp_word_p = tree->doubles[(int)key_buff[0]];
        int left = buff_len - count;
        if (left > 5)
            left = 5;
        memcpy(word_buff + count, tmp_word_p, left * sizeof(PyEnhanceMapWord*));
        count += left;
        goto out;
    }
    case 2: {
        const PyEnhanceMapWord *tmp_word;
        key_buff[1] -= '1';
        tmp_word = tree->doubles[(int)key_buff[0]][(int)key_buff[1]];
        if (tmp_word) {
            word_buff[0] = tmp_word;
            count++;
            if (count >= buff_len) {
                goto out;
            }
        }
        PyEnhanceStrokeKey *const *tmp_key_p;
        const PyEnhanceStrokeKey *tmp_key;
        tmp_key_p = tree->multiples[(int)key_buff[0]][(int)key_buff[1]];
        for (i = 0;count < buff_len && i < 5;i++) {
            tmp_key = tmp_key_p[i];
            if (!tmp_key || *py_enhance_stroke_get_key(tmp_key))
                continue;
            word_buff[count] = tmp_key->words;
            count++;
        }
        goto out;
    }
    default:
        break;
    }
    // maximum size from (key_buff[0] == i || key_buff[1] == j)
    PyEnhanceStrokeKeyLookup lookup[(5 + 5 - 1) * 5];
    const PyEnhanceStrokeKey *tmp_key;
    int lookup_c = 0;
    int j;
    int k;
    key_buff[1] -= '1';
    key_buff[2] -= '1';
    key_s = key_buff + 3;
    key_l -= 3;
    for (i = 0;i < 5;i++) {
        for (j = 0;j < 5;j++) {
            boolean diff0 = key_buff[0] != i;
            boolean diff1 = key_buff[1] != j;
            if (diff0 && diff1)
                continue;
            for (k = 0;k < 5;k++) {
                tmp_key = (tree->multiples[i][j][k]);
                if (!tmp_key)
                    continue;
                PyEnhanceStrokeKeyLookup *lookup_p = lookup + lookup_c;
                if (key_buff[2] == k) {
                    lookup_p->key = tmp_key;
                    lookup_p->key_s = key_s;
                    lookup_p->key_l = key_l;
                    lookup_p->diff = diff0 + diff1;
                    lookup_c++;
                    continue;
                }
                if (diff0 || diff1)
                    continue;
                lookup_p->key = tmp_key;
                lookup_p->key_s = key_s - 1;
                lookup_p->key_l = key_l + 1;
                lookup_p->diff = 1;
                lookup_c++;
            }
        }
    }
    key_buff[2] += '1';
    int cur_len = key_l * 2 / 3;
    PyEnhanceStrokeResult res_buff[16];
    while (lookup_c > 0 && (count < buff_len || cur_len <= key_l + 4)) {
        for (i = 0;i < lookup_c;i++) {
            PyEnhanceStrokeKeyLookup *lookup_p = lookup + i;
            if (!lookup_p->key) {
                lookup_c--;
                memmove(lookup_p, lookup_p + 1,
                        sizeof(PyEnhanceStrokeKeyLookup) * (lookup_c - i));
                i--;
                continue;
            }
            while (lookup_p->key && lookup_p->key->key_l < cur_len) {
                lookup_p->key = lookup_p->key->next;
            }
            for (;lookup_p->key && lookup_p->key->key_l == cur_len;
                 lookup_p->key = lookup_p->key->next) {
                int distance = py_enhance_stroke_get_distance(
                    lookup_p->key_s, lookup_p->key_l,
                    py_enhance_stroke_get_key(lookup_p->key),
                    lookup_p->key->key_l);
                if (distance < 0)
                    continue;
                distance += lookup_p->diff * REPLACE_WEIGHT;
                for (j = 0;j < count;j++) {
                    if (distance < res_buff[j].distance) {
                        break;
                    }
                }
                if (count < buff_len) {
                    count++;
                } else if (j >= count) {
                    continue;
                }
                PyEnhanceStrokeResult *pos = res_buff + j;
                int move_size = count - j - 1;
                if (move_size > 0) {
                    memmove(pos + 1, pos,
                            move_size * sizeof(PyEnhanceStrokeResult));
                }
                pos->word = lookup_p->key->words;
                pos->distance = distance;
            }
        }
        cur_len++;
    }
    for (j = 0;j < count;j++) {
        word_buff[j] = res_buff[j].word;
    }
out:
    free(key_buff);
    return count;
}

static void
py_enhance_stroke_add_word(PyEnhanceStrokeTree *tree, FcitxMemoryPool *pool,
                           const char *key_s, int key_l,
                           const char *word_s, int word_l)
{
    PyEnhanceMapWord **word_p;
    switch (key_l) {
    case 1:
        word_p = &tree->singles[key_s[0] - '1'];
        break;
    case 2:
        word_p = &tree->doubles[key_s[0] - '1'][key_s[1] - '1'];
        break;
    default: {
        PyEnhanceStrokeKey *key;
        PyEnhanceStrokeKey **key_p;
        key_p = (&tree->multiples[key_s[0] - '1'][key_s[1] - '1']
                 [key_s[2] - '1']);
        key = *key_p;
        int res;
        key_s += 3;
        /**
         * since all the words are ordered, which means res <= 0,
         * the loop is actually not doing anything.
         **/
        for (;;key_p = &key->next, key = *key_p) {
            if ((!key) || key_l - 3 < key->key_l ||
                (res = strcmp(key_s,
                              py_enhance_stroke_get_key(key))) < 0) {
                PyEnhanceStrokeKey *new_key;
                new_key = fcitx_memory_pool_alloc_align(
                    pool, sizeof(PyEnhanceStrokeKey) + key_l - 2, 1);
                new_key->words = NULL;
                new_key->next = key;
                new_key->key_l = key_l - 3;
                *key_p = new_key;
                memcpy(py_enhance_stroke_get_key(new_key), key_s, key_l - 2);
                word_p = &new_key->words;
                break;
            } else if (fcitx_likely(res == 0)) {
                word_p = &key->words;
                break;
            }
        }
    }
    }
    PyEnhanceMapWord *new_word;
    new_word = fcitx_memory_pool_alloc_align(
        pool, sizeof(PyEnhanceMapWord) + word_l + 1, 1);
    new_word->next = *word_p;
    *word_p = new_word;
    memcpy(py_enhance_map_word(new_word), word_s, word_l + 1);
}

void
py_enhance_stroke_load_tree(PyEnhanceStrokeTree *tree, FILE *fp,
                            FcitxMemoryPool *pool)
{
    char *buff = NULL;
    char *key;
    char *word;
    int key_l;
    int word_l;
    size_t len;
    memset(tree, 0, sizeof(PyEnhanceStrokeTree));
    while (getline(&buff, &len, fp) != -1) {
        /* remove leading spaces */
        key = buff + strspn(buff, PYENHANCE_MAP_BLANK);
        /* empty line or comment */
        if (*key == '\0' || *key == '#')
            continue;
        /* find delimiter */
        key_l = strspn(key, "12345");
        if (!key_l)
            continue;
        word = key + key_l;
        word_l = strspn(word, PYENHANCE_MAP_BLANK);
        if (!word_l)
            continue;
        *word = '\0';
        word += word_l;
        word_l = strcspn(word, PYENHANCE_MAP_BLANK);
        if (!word_l)
            continue;
        word[word_l] = '\0';
        word_l++;
        py_enhance_stroke_add_word(tree, pool, key, key_l, word, word_l);
    }
    fcitx_utils_free(buff);
}
