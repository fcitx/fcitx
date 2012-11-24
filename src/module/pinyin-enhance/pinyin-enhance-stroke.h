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


static inline char *py_enhance_stroke_get_key(const PyEnhanceStrokeKey *key)
{
    return ((void*)(intptr_t)key) + sizeof(PyEnhanceStrokeKey);
}

void py_enhance_stroke_load_tree(PyEnhanceStrokeTree *tree, FILE *fp,
                                 FcitxMemoryPool *pool);
int py_enhance_stroke_get_match_keys(PinyinEnhance *pyenhance,
                                     const char *key_s, int key_l,
                                     const PyEnhanceMapWord **word_buff,
                                     int buff_len);

#endif
