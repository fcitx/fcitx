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

#include "fcitx/fcitx.h"
#include "config.h"

#include "fcitx/ime.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "spell-internal.h"
#include "spell-enchant.h"
#include <dlfcn.h>

static void *_enchant_handle = NULL;
static void *(*_enchant_broker_init)() = NULL;
static char **(*_enchant_dict_suggest)(void *dict, const char *str,
                                       ssize_t len, size_t *out_n) = NULL;
static void (*_enchant_dict_free_string_list)(void *dict,
                                              char **str_list) = NULL;
static void (*_enchant_broker_free_dict)(void *broker, void *dict) = NULL;
static void (*_enchant_broker_free)(void *broker) = NULL;
static void *(*_enchant_broker_request_dict)(void *broker,
                                             const char *const tag) = NULL;
static void (*_enchant_broker_set_ordering)(void *broker,
                                            const char *const tag,
                                            const char *const ordering) = NULL;
static void (*_enchant_dict_add_to_personal)(void *dict, const char *const word,
                                             ssize_t len) = NULL;

static boolean
SpellEnchantLoadLib()
{
    if (_enchant_handle)
        return true;
    _enchant_handle = dlopen(ENCHANT_LIBRARY_FILENAME, RTLD_NOW | RTLD_GLOBAL);
    if (!_enchant_handle)
        goto fail;
#define ENCHANT_LOAD_SYMBOL(sym) do {            \
        _##sym = dlsym(_enchant_handle, #sym);\
            if (!_##sym)                         \
                goto fail;                       \
    } while(0)
    ENCHANT_LOAD_SYMBOL(enchant_broker_init);
    ENCHANT_LOAD_SYMBOL(enchant_dict_suggest);
    ENCHANT_LOAD_SYMBOL(enchant_dict_free_string_list);
    ENCHANT_LOAD_SYMBOL(enchant_broker_free_dict);
    ENCHANT_LOAD_SYMBOL(enchant_broker_free);
    ENCHANT_LOAD_SYMBOL(enchant_broker_request_dict);
    ENCHANT_LOAD_SYMBOL(enchant_broker_set_ordering);
    ENCHANT_LOAD_SYMBOL(enchant_dict_add_to_personal);
    return true;
fail:
    if (_enchant_handle) {
        dlclose(_enchant_handle);
        _enchant_handle = NULL;
    }
    return false;
}

boolean
SpellEnchantInit(FcitxSpell *spell)
{
    if (spell->broker)
        return true;
    if (!SpellEnchantLoadLib())
        return false;
    spell->broker = _enchant_broker_init();
    spell->cur_enchant_provider = EP_Default;
    if (!spell->broker)
        return false;
    if (spell->dictLang)
        SpellEnchantLoadDict(spell, spell->dictLang);
    return true;
}

SpellHint*
SpellEnchantHintWords(FcitxSpell *spell, unsigned int len_limit)
{
    SpellHint *res = NULL;
    if (!SpellEnchantInit(spell))
        return NULL;
    if (!spell->enchant_dict || spell->enchant_saved_lang)
        return NULL;
    char **suggestions = NULL;
    size_t number = 0;
    if (!*spell->current_str)
        return NULL;
    suggestions = _enchant_dict_suggest(spell->enchant_dict, spell->current_str,
                                        strlen(spell->current_str), &number);
    if (!suggestions)
        return NULL;
    number = number > len_limit ? len_limit : number;
    res = SpellHintList(number, suggestions, NULL);
    _enchant_dict_free_string_list(spell->enchant_dict, suggestions);
    return res;
}

boolean
SpellEnchantCheck(FcitxSpell *spell)
{
    if (!SpellEnchantInit(spell))
        return false;
    if (spell->enchant_dict && !spell->enchant_saved_lang)
        return true;
    return false;
}

void
SpellEnchantDestroy(FcitxSpell *spell)
{
    if (spell->broker) {
        if (spell->enchant_dict)
            _enchant_broker_free_dict(spell->broker, spell->enchant_dict);
        _enchant_broker_free(spell->broker);
    }
    if (spell->enchant_saved_lang)
        free(spell->enchant_saved_lang);
    /* if (spell->enchantLanguages) */
    /*     fcitx_utils_free_string_list(spell->enchantLanguages); */
}

boolean
SpellEnchantLoadDict(FcitxSpell *spell, const char *lang)
{
    void *enchant_dict;
    if (!SpellEnchantInit(spell))
        return false;
    if (!spell->broker)
        return false;
    if (spell->enchant_saved_lang &&
        !strcmp(spell->enchant_saved_lang, lang)) {
        free(spell->enchant_saved_lang);
        spell->enchant_saved_lang = NULL;
        return true;
    }
    enchant_dict = _enchant_broker_request_dict(spell->broker, lang);
    if (enchant_dict) {
        if (spell->enchant_saved_lang) {
            free(spell->enchant_saved_lang);
            spell->enchant_saved_lang = NULL;
        }
        if (spell->enchant_dict)
            _enchant_broker_free_dict(spell->broker, spell->enchant_dict);
        spell->enchant_dict = enchant_dict;
        return true;
    }
    if (!spell->enchant_dict || !spell->dictLang)
        return false;
    if (spell->enchant_saved_lang)
        return false;
    spell->enchant_saved_lang = strdup(spell->dictLang);
    return false;
}

void
SpellEnchantApplyConfig(FcitxSpell *spell)
{
    if (!SpellEnchantInit(spell))
        return;
    if (!spell->broker) {
        spell->broker = _enchant_broker_init();
        spell->cur_enchant_provider = EP_Default;
        if (!spell->broker)
            return;
    }
    if (spell->cur_enchant_provider == spell->config.enchant_provider)
        return;
    if (spell->config.enchant_provider == EP_Default) {
        if (spell->enchant_saved_lang) {
            free(spell->enchant_saved_lang);
            spell->enchant_saved_lang = NULL;
        }
        if (spell->enchant_dict) {
            _enchant_broker_free_dict(spell->broker, spell->enchant_dict);
            spell->enchant_dict = NULL;
        }
        _enchant_broker_free(spell->broker);
        spell->broker = _enchant_broker_init();
        spell->cur_enchant_provider = EP_Default;
        if (!spell->broker)
            return;
    }
    switch (spell->config.enchant_provider) {
    case EP_Aspell:
        _enchant_broker_set_ordering(spell->broker, "*",
                                    "aspell,myspell,ispell");
        break;
    case EP_Myspell:
        _enchant_broker_set_ordering(spell->broker, "*",
                                    "myspell,aspell,ispell");
        break;
    default:
        break;
    }
    spell->cur_enchant_provider = spell->config.enchant_provider;
    if (!spell->enchant_dict && spell->dictLang && spell->dictLang[0]) {
        spell->enchant_dict = _enchant_broker_request_dict(spell->broker,
                                                           spell->dictLang);
    }
}

void
SpellEnchantAddPersonal(FcitxSpell *spell, const char *new_word)
{
    if (!SpellEnchantInit(spell))
        return;
    if (spell->enchant_dict && !spell->enchant_saved_lang) {
        _enchant_dict_add_to_personal(spell->enchant_dict, new_word,
                                      strlen(new_word));
        return;
    }
}
