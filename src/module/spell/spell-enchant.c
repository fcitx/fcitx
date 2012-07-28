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

#include <unicode/unorm.h>

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

boolean
SpellEnchantInit(FcitxSpell *spell)
{
    spell->broker = enchant_broker_init();
    spell->cur_enchant_provider = EP_Default;
    return !!(spell->broker);
}

SpellHint*
SpellEnchantHintWords(FcitxSpell *spell, unsigned int len_limit)
{
    SpellHint *res = NULL;
    if (!spell->enchant_dict || spell->enchant_saved_lang)
        return NULL;
    char **suggestions = NULL;
    size_t number = 0;
    suggestions = enchant_dict_suggest(spell->enchant_dict, spell->current_str,
                                       strlen(spell->current_str), &number);
    if (!suggestions)
        return NULL;
    number = number > len_limit ? len_limit : number;
    res = SpellHintList(number, suggestions, NULL);
    enchant_dict_free_string_list(spell->enchant_dict, suggestions);
    return res;
}

boolean
SpellEnchantCheck(FcitxSpell *spell)
{
    if (spell->enchant_dict && !spell->enchant_saved_lang)
        return true;
    return false;
}

void
SpellEnchantDestroy(FcitxSpell *spell)
{
    if (spell->broker) {
        if (spell->enchant_dict)
            enchant_broker_free_dict(spell->broker, spell->enchant_dict);
        enchant_broker_free(spell->broker);
    }
    if (spell->enchant_saved_lang)
        free(spell->enchant_saved_lang);
    /* if (spell->enchantLanguages) */
    /*     fcitx_utils_free_string_list(spell->enchantLanguages); */
}

boolean
SpellEnchantLoadDict(FcitxSpell *spell, const char *lang)
{
    EnchantDict *enchant_dict;
    if (!spell->broker)
        return false;
    if (spell->enchant_saved_lang &&
        !strcmp(spell->enchant_saved_lang, lang)) {
        free(spell->enchant_saved_lang);
        spell->enchant_saved_lang = NULL;
        return false;
    }
    enchant_dict = enchant_broker_request_dict(spell->broker, lang);
    if (enchant_dict) {
        if (spell->enchant_saved_lang) {
            free(spell->enchant_saved_lang);
            spell->enchant_saved_lang = NULL;
        }
        if (spell->enchant_dict)
            enchant_broker_free_dict(spell->broker, spell->enchant_dict);
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
    if (!spell->broker) {
        spell->broker = enchant_broker_init();
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
            enchant_broker_free_dict(spell->broker, spell->enchant_dict);
            spell->enchant_dict = NULL;
        }
        enchant_broker_free(spell->broker);
        spell->broker = enchant_broker_init();
        spell->cur_enchant_provider = EP_Default;
        if (!spell->broker)
            return;
    }
    switch (spell->config.enchant_provider) {
    case EP_Aspell:
        enchant_broker_set_ordering(spell->broker, "*",
                                    "aspell,myspell,ispell");
        break;
    case EP_Myspell:
        enchant_broker_set_ordering(spell->broker, "*",
                                    "myspell,aspell,ispell");
        break;
    default:
        break;
    }
    spell->cur_enchant_provider = spell->config.enchant_provider;
    if (!spell->enchant_dict && spell->dictLang && spell->dictLang[0]) {
        spell->enchant_dict = enchant_broker_request_dict(spell->broker,
                                                          spell->dictLang);
    }
}

void
SpellEnchantAddPersonal(FcitxSpell *spell, const char *new_word)
{
    if (spell->enchant_dict && !spell->enchant_saved_lang) {
        enchant_dict_add_to_personal(spell->enchant_dict, new_word,
                                     strlen(new_word));
        return;
    }
}
