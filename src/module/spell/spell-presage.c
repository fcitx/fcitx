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
#include <time.h>
#include <dlfcn.h>

#include "spell-internal.h"
#include "spell-presage.h"

static void *_presage_handle = NULL;
static int (*_presage_completion)(void *prsg, const char *token,
                                  char **result) = NULL;
static void (*_presage_free_string)(char *str) = NULL;
static int (*_presage_new)(const char *(*past_stream_cb)(void*),
                           void *past_stream_cb_arg,
                           const char *(*future_stream_cb)(void*),
                           void *future_stream_cb_arg,
                           void **result) = NULL;
static int (*_presage_config_set)(void *prsg,
                                  const char *variable,
                                  const char *value) = NULL;
static int (*_presage_predict)(void *prsg, char ***result) = NULL;
static void (*_presage_free_string_array)(char **str) = NULL;
static void (*_presage_free)(void *prsg) = NULL;

static boolean
SpellPresageLoadLib()
{
    if (_presage_handle)
        return true;
    _presage_handle = dlopen(PRESAGE_LIBRARY_FILENAME, RTLD_NOW | RTLD_GLOBAL);
    if (!_presage_handle)
        goto fail;
#define PRESAGE_LOAD_SYMBOL(sym) do {            \
        _##sym = dlsym(_presage_handle, #sym);   \
            if (!_##sym)                         \
                goto fail;                       \
    } while(0)
    PRESAGE_LOAD_SYMBOL(presage_completion);
    PRESAGE_LOAD_SYMBOL(presage_free_string);
    PRESAGE_LOAD_SYMBOL(presage_new);
    PRESAGE_LOAD_SYMBOL(presage_config_set);
    PRESAGE_LOAD_SYMBOL(presage_predict);
    PRESAGE_LOAD_SYMBOL(presage_free_string_array);
    PRESAGE_LOAD_SYMBOL(presage_free);
    return true;
fail:
    if (_presage_handle) {
        dlclose(_presage_handle);
        _presage_handle = NULL;
    }
    return false;
}

static const char*
FcitxSpellGetPastStream(void *arg)
{
    FcitxSpell *spell = (FcitxSpell*)arg;
    if (!spell->past_stm) {
        fcitx_utils_alloc_cat_str(spell->past_stm, spell->before_str,
                                  spell->current_str);
    }
    return spell->past_stm;
}

static const char*
FcitxSpellGetFutureStream(void *arg)
{
    FcitxSpell *spell = (FcitxSpell*)arg;
    return spell->after_str;
}

static SpellHint*
SpellPresageResult(FcitxSpell *spell, char **suggestions)
{
    int i;
    int count = 0;
    int len = SpellCalListSize(suggestions, -1);
    char *commits[len];
    char *displays[len];
    SpellHint *res;
    if (!len)
        return NULL;
    for (i = 0;i < len;i++) {
        char *result = NULL;
        char *tmp_str = NULL;
        _presage_completion(spell->presage, suggestions[i], &result);
        if (!result)
            continue;
        tmp_str = fcitx_utils_trim(result);
        _presage_free_string(result);
        fcitx_utils_alloc_cat_str(result, spell->current_str, tmp_str);
        free(tmp_str);
        commits[count] = result;
        displays[count] = suggestions[i];
        count++;
    }
    res = SpellHintList(count, displays, commits);
    for (i = 0;i < count;i++) {
        free(commits[i]);
    }
    return res;
}

boolean
SpellPresageInit(FcitxSpell *spell)
{
    if (spell->presage)
        return true;
    if (!SpellPresageLoadLib())
        return false;
    _presage_new(FcitxSpellGetPastStream, spell,
                 FcitxSpellGetFutureStream, spell, &spell->presage);
    spell->presage_support = false;
    if (!spell->presage)
        return false;
    if (spell->dictLang)
        SpellPresageLoadDict(spell, spell->dictLang);
    return true;
}

SpellHint*
SpellPresageHintWords(FcitxSpell *spell, unsigned int len_limit)
{
    if (!SpellPresageInit(spell))
        return NULL;
    SpellHint *res = NULL;
    if (!(spell->presage && spell->presage_support))
        return NULL;
    do {
        char **suggestions = NULL;
        char buf[(int)(sizeof(unsigned int) * 5.545177444479562) + 1];
        sprintf(buf, "%u", len_limit);
        _presage_config_set(spell->presage,
                            "Presage.Selector.SUGGESTIONS", buf);
        _presage_predict(spell->presage, &suggestions);
        if (!suggestions)
            break;
        res = SpellPresageResult(spell, suggestions);
        _presage_free_string_array(suggestions);
    } while(0);
    if (spell->past_stm) {
        free(spell->past_stm);
        spell->past_stm = NULL;
    }
    return res;
}

boolean
SpellPresageCheck(FcitxSpell *spell)
{
    if (!SpellPresageInit(spell))
        return false;
    if (spell->presage && spell->presage_support)
        return true;
    return false;
}

void
SpellPresageDestroy(FcitxSpell *spell)
{
    if (spell->presage) {
        _presage_free(spell->presage);
        spell->presage = NULL;
    }
}

boolean
SpellPresageLoadDict(FcitxSpell *spell, const char *lang)
{
    if (!SpellPresageInit(spell))
        return false;
    if (SpellLangIsLang(lang, "en")) {
        spell->presage_support = true;
    } else {
        spell->presage_support = false;
    }
    return spell->presage_support;
}
