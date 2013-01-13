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
#include <fcitx-utils/utils.h>
#include <fcitx-utils/memory.h>
#include <fcitx/candidate.h>
#include <fcitx-config/xdg.h>

#include "pinyin-enhance.h"
#include "pinyin-enhance-map.h"

#include "config.h"

typedef struct {
    uint32_t len;
    uint32_t alloc;
    void *data;
} PyEnhanceBuff;

#define PY_ENHANCE_BUFF_PAGE (8 * 1024)
#define PY_ENHANCE_BUFF_ALIGH (sizeof(int) >= 4 ? sizeof(int) : 4)

static inline void
_py_enhance_buff_resize(PyEnhanceBuff *buff, uint32_t len)
{
    len = fcitx_utils_align_to(len, PY_ENHANCE_BUFF_PAGE);
    buff->data = realloc(buff->data, len);
    buff->alloc = len;
}

static inline void
py_enhance_buff_reserve(PyEnhanceBuff *buff, uint32_t len)
{
    len += buff->len;
    if (len <= buff->alloc)
        return;
    _py_enhance_buff_resize(buff, len);
}

static inline uint32_t
py_enhance_buff_alloc(PyEnhanceBuff *buff, uint32_t len)
{
    uint32_t res = fcitx_utils_align_to(buff->len, PY_ENHANCE_BUFF_ALIGH);
    buff->len = res + len;
    if (fcitx_unlikely(buff->len > buff->alloc)) {
        _py_enhance_buff_resize(buff, buff->len);
    }
    return res;
}

static inline uint32_t
py_enhance_buff_alloc_noalign(PyEnhanceBuff *buff, uint32_t len)
{
    uint32_t res = buff->len;
    buff->len = res + len;
    if (fcitx_unlikely(buff->len > buff->alloc)) {
        _py_enhance_buff_resize(buff, buff->len);
    }
    return res;
}

static inline void
py_enhance_buff_shrink(PyEnhanceBuff *buff)
{
    _py_enhance_buff_resize(buff, buff->len);
}

static inline void
py_enhance_buff_free(PyEnhanceBuff *buff)
{
    fcitx_utils_free(buff->data);
}

typedef struct {
    const char *const str;
    const int len;
} PyEnhanceStrLen;

/**
 * s must be a string literal.
 **/
#define PY_STR_LEN(s) {.str = s, .len = sizeof(s) - 1}

typedef struct {
    uint32_t table[5 + 5 * 5 + 5 * 5 * 5];
    PyEnhanceBuff keys;
    PyEnhanceBuff words;
} PyEnhanceStrokeTree;

typedef struct {
    FcitxGenericConfig gconfig;
    boolean short_as_english;
    boolean allow_replace_first;
    boolean disable_spell;
    boolean disable_sym;
    int stroke_thresh;
    int stroke_limit;
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

    PyEnhanceBuff py_list;
    PyEnhanceBuff py_table;
} PinyinEnhance;

enum {
    PY_IM_INVALID = 0,
    PY_IM_PINYIN,
    PY_IM_SHUANGPIN,
};

DEFINE_GET_ADDON("fcitx-sunpinyin", SunPinyin)
char *PinyinEnhanceGetSelected(PinyinEnhance *pyenhance);

#endif /* _PINYIN_ENHANCE_H */
