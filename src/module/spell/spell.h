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
#ifndef _FCITX_MODULE_SPELL_H
#define _FCITX_MODULE_SPELL_H

#include "config.h"
#include <fcitx-config/fcitx-config.h>
#ifdef ENCHANT_FOUND
#include <enchant/enchant.h>
#endif

#ifdef PRESAGE_FOUND
#include <presage.h>
#endif

typedef enum {
    EP_Default = 0,
    EP_Aspell = 1,
    EP_Myspell = 2
} EnchantProvider;

typedef struct {
    char *display;
    char *commit;
} SpellHint;

#define FCITX_SPELL_NAME "fcitx-spell"

enum {
    FCITX_SPELL_HINT_WORDS,
    FCITX_SPELL_ADD_PERSONAL,
    FCITX_SPELL_DICT_AVAILABLE,
};

typedef struct {
    FcitxGenericConfig gconfig;
#ifdef PRESAGE_FOUND
    boolean usePresage;
#endif
#ifdef ENCHANT_FOUND
    EnchantProvider provider;
#endif
} FcitxSpellConfig;

typedef struct {
    struct _FcitxInstance *owner;
    FcitxSpellConfig config;
    char *dictLang;
    const char *before_str;
    const char *current_str;
    const char *after_str;
#ifdef ENCHANT_FOUND
    EnchantBroker* broker;
    // UT_array* enchantLanguages;
    EnchantDict* dict;
#endif
#ifdef PRESAGE_FOUND
    presage_t presage;
    boolean presage_support;
    char *past_stm;
#endif
} FcitxSpell;

#ifdef __cplusplus
extern "C" {
#endif
    CONFIG_BINDING_DECLARE(FcitxSpellConfig);
#ifdef __cplusplus
}
#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
