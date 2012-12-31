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

#ifndef _PINYIN_ENHANCE_MAP_H
#define _PINYIN_ENHANCE_MAP_H

#define PYENHANCE_MAP_BLANK " \t\b\r\n"

#include "stdint.h"

typedef struct _PyEnhanceMapWord PyEnhanceMapWord;
struct _PyEnhanceMapWord {
    PyEnhanceMapWord *next;
};
static inline void*
py_enhance_map_word(const PyEnhanceMapWord *word)
{
    return ((void*)(intptr_t)word) + sizeof(PyEnhanceMapWord);
}

typedef struct _PyEnhanceMap PyEnhanceMap;
struct _PyEnhanceMap {
    PyEnhanceMapWord *words;
    UT_hash_handle hh;
};

static inline PyEnhanceMap*
py_enhance_map_next(PyEnhanceMap *map)
{
    return (PyEnhanceMap*)map->hh.next;
}

static inline void*
py_enhance_map_key(PyEnhanceMap *map)
{
    return (void*)map + sizeof(PyEnhanceMap);
}
void PinyinEnhanceMapAdd(PyEnhanceMap **map, FcitxMemoryPool *pool,
                         const char *key, unsigned int key_l,
                         const char *word, unsigned int word_l);
PyEnhanceMapWord *PinyinEnhanceMapGet(PyEnhanceMap *map,
                                      const char *key, unsigned int key_l);
static inline void
PinyinEnhanceMapClear(PyEnhanceMap **map, FcitxMemoryPool *pool)
{
    *map = NULL;
    if (fcitx_likely(pool)) {
        fcitx_memory_pool_clear(pool);
    }
}
void PinyinEnhanceMapLoad(PyEnhanceMap **map, FcitxMemoryPool *pool, FILE *fp);

#endif /* _PINYIN_ENHANCE_MAP_H */
