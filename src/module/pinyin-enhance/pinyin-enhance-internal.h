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

#ifndef _PINYIN_ENHANCE_INTERNAL_H
#define _PINYIN_ENHANCE_INTERNAL_H

#include <fcitx/fcitx.h>
#include <fcitx/module.h>
#include <fcitx/instance.h>
#include <fcitx/hook.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/memory.h>
#include <fcitx/candidate.h>
#include <fcitx-config/xdg.h>

#include "pinyin-enhance.h"
#include "pinyin-enhance-map.h"

#include "config.h"

typedef struct _PyEnhanceStrokeTree PyEnhanceStrokeTree;
typedef struct _PyEnhanceStrokeKey PyEnhanceStrokeKey;

struct _PyEnhanceStrokeKey {
    PyEnhanceMapWord *words;
    PyEnhanceStrokeKey *next;
    int key_l;
};

struct _PyEnhanceStrokeTree {
    PyEnhanceMapWord *singles[5];
    PyEnhanceMapWord *doubles[5][5];
    PyEnhanceStrokeKey *multiples[5][5][5];
};

typedef struct {
    FcitxGenericConfig gconfig;
    boolean short_as_english;
    boolean allow_replace_first;
    boolean disable_spell;
    boolean disable_sym;
    int stroke_thresh;
    int max_hint_length;
    char *char_from_phrase_str;
    FcitxHotkey char_from_phrase_key[2];
} PinyinEnhanceConfig;

typedef struct {
    PinyinEnhanceConfig config;
    FcitxInstance *owner;

    boolean cfp_active; /* for "char from phrase" */
    int cfp_cur_word;
    int cfp_cur_page;

    char *cfp_mode_selected;
    int cfp_mode_cur;
    int cfp_mode_count;
    char ***cfp_mode_lists;

    PyEnhanceMap *sym_table;
    FcitxMemoryPool *sym_pool;

    boolean stroke_loaded;
    PyEnhanceStrokeTree stroke_tree;

    UT_array py_list;

    FcitxMemoryPool *static_pool;
} PinyinEnhance;

enum {
    PY_IM_INVALID = 0,
    PY_IM_PINYIN,
    PY_IM_SHUANGPIN,
};

DEFINE_GET_ADDON("fcitx-sunpinyin", SunPinyin)

#ifdef __cplusplus
extern "C" {
#endif

    char *PinyinEnhanceGetSelected(PinyinEnhance *pyenhance);

#ifdef __cplusplus
}
#endif

#endif /* _PINYIN_ENHANCE_H */
