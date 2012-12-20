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

static inline uint32_t
py_enhance_single_offset(uint8_t i)
{
    return i;
}
#define _py_enhance_single_offset(dummy, i, ...)        \
    (py_enhance_single_offset)(i)
#define py_enhance_single_offset(args...)       \
    _py_enhance_single_offset(NULL, ##args, 0)
#define py_enhance_get_single(t, args...)                               \
    (((PyEnhanceStrokeTree*)(t))->table + py_enhance_single_offset(args))

static inline uint32_t
py_enhance_double_offset(uint8_t i1, uint8_t i2)
{
    return 5 + i1 * 5 + i2;
}
#define _py_enhance_double_offset(dummy, i1, i2, ...)   \
    (py_enhance_double_offset)(i1, i2)
#define py_enhance_double_offset(args...)       \
    _py_enhance_double_offset(NULL, ##args, 0)
#define py_enhance_get_double(t, args...)                               \
    (((PyEnhanceStrokeTree*)(t))->table + py_enhance_double_offset(args))

static inline uint32_t
py_enhance_multiple_offset(uint8_t i1, uint8_t i2, uint8_t i3)
{
    return 5 + 5 * 5 + i1 * 5 * 5 + i2 * 5 + i3;
}
#define _py_enhance_multiple_offset(dummy, i1, i2, i3, ...)     \
    (py_enhance_multiple_offset)(i1, i2, i3)
#define py_enhance_multiple_offset(args...)       \
    _py_enhance_multiple_offset(NULL, ##args, 0)
#define py_enhance_get_multiple(t, args...)                               \
    (((PyEnhanceStrokeTree*)(t))->table + py_enhance_multiple_offset(args))

typedef struct {
    /**
     * same as word->next
     **/
    uint32_t words;
    /**
     * next % 2 != 0: end
     **/
    uint32_t next;
    uint8_t key_l;
    uint8_t prefix;
    char key[1];
} PyEnhanceStrokeKey;

static inline PyEnhanceStrokeKey*
_py_enhance_stroke_id_to_key(const PyEnhanceStrokeTree *tree, uint32_t id)
{
    return (PyEnhanceStrokeKey*)(tree->keys.data + id);
}

static inline PyEnhanceStrokeKey*
py_enhance_stroke_id_to_key(const PyEnhanceStrokeTree *tree, uint32_t id)
{
    if (id % 4 != 0)
        return NULL;
    return _py_enhance_stroke_id_to_key(tree, id);
}

static inline PyEnhanceStrokeKey*
py_enhance_stroke_key_next(const PyEnhanceStrokeTree *tree,
                           const PyEnhanceStrokeKey *k)
{
    return py_enhance_stroke_id_to_key(tree, k->next);
}

static inline void
py_enhance_stroke_key_tonext(const PyEnhanceStrokeTree *tree,
                             const PyEnhanceStrokeKey **k)
{
    *k = py_enhance_stroke_key_next(tree, *k);
}

#define PY_ENHANCE_STROKE_KEY_REAL_SIZE                 \
    (((void*)((PyEnhanceStrokeKey*)NULL)->key) - NULL)

static inline uint32_t
py_enhance_stroke_alloc_key(PyEnhanceStrokeTree *tree, const char *key_s,
                            uint8_t key_l, PyEnhanceStrokeKey **key_p)
{
    uint32_t size = PY_ENHANCE_STROKE_KEY_REAL_SIZE + key_l + 1;
    uint32_t id = py_enhance_buff_alloc(&tree->keys, size);
    PyEnhanceStrokeKey *key = _py_enhance_stroke_id_to_key(tree, id);
    key->key_l = key_l;
    if (key_l)
        memcpy(key->key, key_s, key_l);
    key->key[key_l] = '\0';
    *key_p = key;
    return id;
}

static inline uint32_t
py_enhance_stroke_alloc_word(PyEnhanceStrokeTree *tree, const char *word_s,
                             uint8_t word_l, PyEnhanceStrokeWord **word_p)
{
    uint32_t id;
    id = py_enhance_buff_alloc(&tree->words, sizeof(PyEnhanceStrokeWord));
    PyEnhanceStrokeWord *word = _py_enhance_stroke_id_to_word(tree, id);
    memcpy(word->word, word_s, word_l);
    word->word[word_l] = '\0';
    return id;
}

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
    uint32_t word;
    int distance;
} PyEnhanceStrokeResult;

#define REPLACE_WEIGHT 5
#define INSERT_WEIGHT 5
#define REMOVE_WEIGHT 5
#define EXCHANGE_WEIGHT 5
#define END_WEIGHT 1
static inline int
py_enhance_stroke_get_distance(const char *word, int word_len,
                               const char *dict, int dict_len)
{
    int replace = 0;
    int insert = 0;
    int remove = 0;
    int exchange = 0;
    int diff = 0;
    int maxdiff;
    int maxremove;
    int word_i = 0;
    int dict_i = 0;
    maxdiff = word_len / 3;
    maxremove = (word_len - 2) / 3;
    while ((diff = replace + insert + remove + exchange) <= maxdiff &&
           remove <= maxremove) {
        if (word_i >= word_len) {
            return ((replace * REPLACE_WEIGHT + insert * INSERT_WEIGHT
                     + remove * REMOVE_WEIGHT + exchange * EXCHANGE_WEIGHT)
                    + (dict_len - dict_i) * END_WEIGHT);
        }
        if (dict_i >= dict_len) {
            if (word_i + 1 < word_len)
                return -1;
            remove++;
            if (diff + 1 <= maxdiff && remove <= maxremove) {
                return (replace * REPLACE_WEIGHT + insert * INSERT_WEIGHT
                        + remove * REMOVE_WEIGHT + exchange * EXCHANGE_WEIGHT);
            }
            return -1;
        }
        if (word[word_i] == dict[dict_i]) {
            word_i++;
            dict_i++;
            continue;
        }
        if (word_i + 1 >= word_len && dict_i + 1 >= dict_len) {
            replace++;
            word_i++;
            dict_i++;
            continue;
        }
        if (word[word_i + 1] == dict[dict_i + 1]) {
            replace++;
            dict_i += 2;
            word_i += 2;
            continue;
        }
        if (word[word_i + 1] == dict[dict_i]) {
            word_i += 2;
            if (word[word_i] == dict[dict_i + 1]) {
                dict_i += 2;
                exchange++;
                continue;
            }
            dict_i++;
            remove++;
            continue;
        }
        if (word[word_i] == dict[dict_i + 1]) {
            word_i++;
            dict_i += 2;
            insert++;
            continue;
        }
        break;
    }
    return -1;
}

int
py_enhance_stroke_get_match_keys(
    PinyinEnhance *pyenhance, const char *key_s, int key_l,
    PyEnhanceStrokeWord **word_buff, int buff_len)
{
    int i;
    int count = 0;
    char *key_buff = malloc(key_l + 1);
    for (i = 0;i < key_l;i++) {
        key_buff[i] = py_enhance_stroke_sym_to_num(key_s[i]);
    }
    key_buff[key_l] = '\0';
    key_buff[0] -= '1';
    const PyEnhanceStrokeTree *tree = &pyenhance->stroke_tree;
    if (buff_len > 16)
        buff_len = 16;
    switch (key_l) {
    case 1: {
        uint32_t tmp_word = *py_enhance_get_single(tree, key_buff[0]);
        if (tmp_word % 4 == 0) {
            word_buff[0] = _py_enhance_stroke_id_to_word(tree, tmp_word);
            count++;
            if (count >= buff_len) {
                goto out;
            }
        }
        const uint32_t *tmp_word_p = py_enhance_get_double(tree, key_buff[0]);
        int left = buff_len - count;
        if (left > 5)
            left = 5;
        for (i = 0;i < left;i++) {
            word_buff[count + i] = _py_enhance_stroke_id_to_word(
                tree, tmp_word_p[i]);
        }
        count += left;
        goto out;
    }
    case 2: {
        key_buff[1] -= '1';
        uint32_t tmp_word;
        tmp_word = *py_enhance_get_double(tree, key_buff[0], key_buff[1]);
        if (tmp_word % 4 == 0) {
            word_buff[0] = _py_enhance_stroke_id_to_word(tree, tmp_word);
            count++;
            if (count >= buff_len) {
                goto out;
            }
        }
        const uint32_t *tmp_key_p;
        const PyEnhanceStrokeKey *tmp_key;
        tmp_key_p = py_enhance_get_multiple(tree, key_buff[0], key_buff[1]);
        int left = buff_len - count;
        if (left > 5)
            left = 5;
        for (i = 0;i < left;i++) {
            tmp_key = py_enhance_stroke_id_to_key(tree, tmp_key_p[i]);
            /**
             * skip if there the prefix has no keys or the shortest key in the
             * series is longer than 3 (i.e. > 0 after the prefix is removed.)
             **/
            if (!tmp_key || tmp_key->key[0])
                continue;
            word_buff[count] = py_enhance_stroke_id_to_word(tree,
                                                            tmp_key->words);
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
    uint32_t tmp_key_id;
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
                tmp_key_id = *py_enhance_get_multiple(tree, i, j, k);
                if (tmp_key_id % 4 != 0)
                    continue;
                tmp_key = _py_enhance_stroke_id_to_key(tree, tmp_key_id);
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
        /**
         * check keys of certain length for all prefix.
         **/
        for (i = 0;i < lookup_c;i++) {
            PyEnhanceStrokeKeyLookup *lookup_p = lookup + i;
            /**
             * remove a prefix if it is already reached the end.
             **/
            if (!lookup_p->key) {
                lookup_c--;
                memmove(lookup_p, lookup_p + 1,
                        sizeof(PyEnhanceStrokeKeyLookup) * (lookup_c - i));
                i--;
                continue;
            }
            /**
             * skip keys shorter than the current length.
             **/
            while (lookup_p->key && lookup_p->key->key_l < cur_len) {
                py_enhance_stroke_key_tonext(tree, &lookup_p->key);
            }
            for (;lookup_p->key && lookup_p->key->key_l == cur_len;
                 py_enhance_stroke_key_tonext(tree, &lookup_p->key)) {
                int distance = py_enhance_stroke_get_distance(
                    lookup_p->key_s, lookup_p->key_l,
                    lookup_p->key->key, lookup_p->key->key_l);
                if (distance < 0)
                    continue;
                distance += lookup_p->diff * REPLACE_WEIGHT;
                /**
                 * insert in the ordered list.
                 **/
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
        word_buff[j] = py_enhance_stroke_id_to_word(tree, res_buff[j].word);
    }
out:
    free(key_buff);
    return count;
}

static void
py_enhance_stroke_add_word(PyEnhanceStrokeTree *tree,
                           const char *key_s, int key_l,
                           const char *word_s, int word_l)
{
    uint32_t key_id;
    switch (key_l) {
    case 1:
        key_id = py_enhance_single_offset(key_s[0] - '1') * 2 + 1;
        break;
    case 2:
        key_id = py_enhance_double_offset(key_s[0] - '1',
                                          key_s[1] - '1') * 2 + 1;
        break;
    default: {
        uint32_t *key_p;
        PyEnhanceStrokeKey *key;
        uint8_t prefix = (((key_s[0] - '1') * 5 + key_s[1] - '1') * 5
                          + key_s[2] - '1');
        key_p = tree->table + prefix + 5 * 5 + 5;
        key_id = *key_p;
        int res;
        key_s += 3;
        key_l -= 3;
        /**
         * since all the words are ordered, which means res <= 0,
         * the loop is actually not doing anything.
         **/
        for (;;key_p = &key->next, key_id = *key_p) {
            key = py_enhance_stroke_id_to_key(tree, key_id);
            if (!key || (key_l < key->key_l) ||
                (res = strcmp(key_s, key->key)) < 0) {
                PyEnhanceStrokeKey *new_key;
                uint32_t new_id;
                new_id = py_enhance_stroke_alloc_key(tree, key_s, key_l,
                                                     &new_key);
                *key_p = new_id;
                new_key->words = new_id + 2;
                new_key->next = key_id;
                new_key->prefix = prefix;
                key_id = new_id;
                break;
            } else if (fcitx_likely(res == 0)) {
                break;
            }
        }
        key_id += 2;
    }
    }
    PyEnhanceStrokeWord *new_word;
    py_enhance_stroke_alloc_word(tree, word_s, word_l, &new_word);
    new_word->next = key_id;
}

static inline uint32_t*
py_enhance_stroke_key_get_words(PyEnhanceStrokeTree *tree, uint32_t key_id)
{
    if (key_id % 4 == 0) {
        return NULL;
    } else if (key_id % 2 == 0) {
        return &_py_enhance_stroke_id_to_key(tree, key_id - 2)->words;
    }
    return &tree->table[key_id / 2];
}

static void
py_enhance_stroke_load_finish(PyEnhanceStrokeTree *tree)
{
    unsigned int words_l = tree->words.len / sizeof(PyEnhanceStrokeWord);
    qsort(tree->words.data, words_l, sizeof(PyEnhanceStrokeWord),
          (int (*)(const void*, const void*))strcmp);
    unsigned int i;
    uint32_t *words;
    uint32_t size = py_enhance_align(sizeof(PyEnhanceStrokeWord),
                                     PY_ENHANCE_BUFF_ALIGH);
    for (i = 0;i < words_l;i++) {
        PyEnhanceStrokeWord *word;
        uint32_t offset = size * i;
        word = tree->words.data + offset;
        words = py_enhance_stroke_key_get_words(tree, word->next);
        word->next = *words;
        *words = offset;
    }
}

void
py_enhance_stroke_load_tree(PyEnhanceStrokeTree *tree, FILE *fp)
{
    char *buff = NULL;
    char *key;
    char *word;
    int key_l;
    int word_l;
    size_t len;
    memset(tree, 0, sizeof(PyEnhanceStrokeTree));
    unsigned int i;
    for (i = 0;i < sizeof(tree->table) / sizeof(uint32_t);i++)
        tree->table[i] = i * 2 + 1;
    // TODO
    py_enhance_buff_reserve(&tree->keys, 1024 * 1024);
    py_enhance_buff_reserve(&tree->words, 1024 * 1024);
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
        py_enhance_stroke_add_word(tree, key, key_l, word, word_l);
    }
    py_enhance_stroke_load_finish(tree);
    py_enhance_buff_shrink(&tree->keys);
    py_enhance_buff_shrink(&tree->words);
    fcitx_utils_free(buff);
}
