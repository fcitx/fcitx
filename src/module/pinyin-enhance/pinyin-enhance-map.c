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

#include "fcitx-utils/utils.h"
#include "fcitx-utils/memory.h"
#include "pinyin-enhance-map.h"
#include "config.h"

#undef uthash_malloc
#undef uthash_free

void
PinyinEnhanceMapAdd(PyEnhanceMap **map, FcitxMemoryPool *pool,
                    const char *key, unsigned int key_l,
                    const char *word, unsigned int word_l)
{
    PyEnhanceMapWord *py_word;
    PyEnhanceMap *py_map;
#define uthash_malloc(sz) fcitx_memory_pool_alloc_align(pool, sz)
#define uthash_free(ptr)
    word_l++;
    py_word = uthash_malloc(sizeof(PyEnhanceMapWord) + word_l);
    memcpy(py_enhance_map_word(py_word), word, word_l);
    HASH_FIND(hh, *map, key, key_l, py_map);
    if (py_map) {
        py_word->next = py_map->words;
        py_map->words = py_word;
    } else {
        py_map = uthash_malloc(sizeof(PyEnhanceMap) + key_l + 1);
        py_map->words = py_word;
        py_word->next = NULL;
        memcpy(py_enhance_map_key(py_map), key, key_l + 1);
        HASH_ADD_KEYPTR(hh, *map, py_enhance_map_key(py_map), key_l, py_map);
    }
#undef uthash_malloc
#undef uthash_free
}

PyEnhanceMapWord*
PinyinEnhanceMapGet(PyEnhanceMap *map, const char *key, unsigned int key_l)
{
    PyEnhanceMap *py_map;
    HASH_FIND(hh, map, key, key_l, py_map);
    if (py_map) {
        return py_map->words;
    }
    return NULL;
}

void
PinyinEnhanceMapLoad(PyEnhanceMap **map, FcitxMemoryPool *pool, FILE *fp)
{
    char *buff = NULL;
    char *key;
    char *word;
    int key_l;
    int word_l;
    size_t len;
    while (getline(&buff, &len, fp) != -1) {
        /* remove leading spaces */
        key = buff + strspn(buff, PYENHANCE_MAP_BLANK);
        /* empty line or comment */
        if (*key == '\0' || *key == '#')
            continue;
        /* find delimiter */
        key_l = strcspn(key, PYENHANCE_MAP_BLANK);
        if (!key_l)
            continue;
        word = key + key_l;
        *word = '\0';
        word++;
        /* find start of word */
        word = word + strspn(word, PYENHANCE_MAP_BLANK);
        word_l = strcspn(word, PYENHANCE_MAP_BLANK);
        if (!word_l)
            continue;
        word[word_l] = '\0';
        PinyinEnhanceMapAdd(map, pool, key, key_l, word, word_l);
    }

    fcitx_utils_free(buff);
}
