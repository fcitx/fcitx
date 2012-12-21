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

#ifndef _PINYIN_ENHANCE_STROKE_H
#define _PINYIN_ENHANCE_STROKE_H

#include <stdint.h>
#include "pinyin-enhance-internal.h"
#include "pinyin-enhance-map.h"
#include "fcitx-utils/utf8.h"

typedef struct {
    char word[UTF8_MAX_LENGTH + 1];
    /**
     * next % 4:
     *     0: word_id,
     *     2: key_id + 2,
     *     1, 3: table_offset * 2 + 1,
     **/
    uint32_t next;
} PyEnhanceStrokeWord;

static inline PyEnhanceStrokeWord*
_py_enhance_stroke_id_to_word(const PyEnhanceStrokeTree *tree, uint32_t id)
{
    return (PyEnhanceStrokeWord*)(tree->words.data + id);
}

static inline PyEnhanceStrokeWord*
py_enhance_stroke_id_to_word(const PyEnhanceStrokeTree *tree, uint32_t id)
{
    if (id % 4 != 0)
        return NULL;
    return _py_enhance_stroke_id_to_word(tree, id);
}

static inline PyEnhanceStrokeWord*
py_enhance_stroke_word_next(const PyEnhanceStrokeTree *tree,
                            const PyEnhanceStrokeWord *w)
{
    return py_enhance_stroke_id_to_word(tree, w->next);
}

static inline void
py_enhance_stroke_word_tonext(const PyEnhanceStrokeTree *tree,
                              const PyEnhanceStrokeWord **w)
{
    *w = py_enhance_stroke_word_next(tree, *w);
}

void py_enhance_stroke_load_tree(PyEnhanceStrokeTree *tree, FILE *fp);
int py_enhance_stroke_get_match_keys(PinyinEnhance *pyenhance,
                                     const char *key_s, int key_l,
                                     PyEnhanceStrokeWord **word_buff,
                                     int buff_len);
uint8_t *py_enhance_stroke_find_stroke(
    PinyinEnhance *pyenhance, const char *str,
    uint8_t *stroke, unsigned int *len);
char *py_enhance_stroke_get_str(const uint8_t *stroke, unsigned int s_l,
                                char *str, unsigned int *len);

#endif
