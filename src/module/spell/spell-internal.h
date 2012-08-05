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
#ifndef _FCITX_MODULE_SPELL_INTERNAL_H
#define _FCITX_MODULE_SPELL_INTERNAL_H

#include "config.h"
#include <fcitx-config/fcitx-config.h>

#include "spell.h"

typedef struct {
    FcitxGenericConfig gconfig;
#ifdef ENABLE_ENCHANT
    EnchantProvider enchant_provider;
#endif
    char *provider_order;
} FcitxSpellConfig;

typedef struct {
    struct _FcitxInstance *owner;
    FcitxSpellConfig config;
    char *dictLang;
    const char *before_str;
    const char *current_str;
    const char *after_str;
    const char *provider_order;
#ifdef ENABLE_ENCHANT
    void *broker;
    EnchantProvider cur_enchant_provider;
    char *enchant_saved_lang;
    // UT_array* enchantLanguages;
    void *enchant_dict;
#endif
#ifdef ENABLE_PRESAGE
    void *presage;
    boolean presage_support;
    char *past_stm;
#endif
    void *custom_dict;
    char *custom_saved_lang;
} FcitxSpell;

#ifdef __cplusplus
extern "C" {
#endif
    CONFIG_BINDING_DECLARE(FcitxSpellConfig);
    SpellHint *SpellHintListWithSize(int count, char **displays, int sized,
                                     char **commits, int sizec);
    SpellHint *SpellHintListWithPrefix(int count, const char *prefix,
                                       int prefix_len,
                                       char **commits, int sizec);
    static inline SpellHint*
    SpellHintList(int count, char **displays, char **commits)
    {
        return SpellHintListWithSize(count, displays, sizeof(char*),
                                     commits, sizeof(char*));
    }
    int SpellCalListSizeWithSize(char **list, int count, int size);
    static inline int
    SpellCalListSize(char **list, int count)
    {
        return SpellCalListSizeWithSize(list, count, sizeof(char*));
    }
    boolean SpellLangIsLang(const char *full_lang, const char *lang);
#ifdef __cplusplus
}
#endif
#endif
