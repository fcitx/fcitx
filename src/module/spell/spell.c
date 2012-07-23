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

#include <libintl.h>
#include <errno.h>

#include "fcitx/ime.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"

#include "spell-internal.h"
#include "custom.h"

static CONFIG_DESC_DEFINE(GetSpellConfigDesc, "fcitx-spell.desc");
static void *SpellCreate(FcitxInstance *instance);
static void SpellDestroy(void *arg);
static void SpellReloadConfig(void *arg);
static void SpellSetLang(FcitxSpell *spell, const char *lang);
static void ApplySpellConfig(FcitxSpell *spell);

static void *FcitxSpellHintWords(void *arg, FcitxModuleFunctionArg args);
static void *FcitxSpellAddPersonal(void *arg, FcitxModuleFunctionArg args);
static void *FcitxSpellDictAvailable(void *arg, FcitxModuleFunctionArg args);

static boolean SpellOrderHasValidProvider(const char *providers);

FCITX_EXPORT_API
const FcitxModule module = {
    .Create = SpellCreate,
    .Destroy = SpellDestroy,
    .SetFD = NULL,
    .ProcessEvent = NULL,
    .ReloadConfig = SpellReloadConfig
};

FCITX_EXPORT_API
const int ABI_VERSION = FCITX_ABI_VERSION;

static void
SaveSpellConfig(FcitxSpellConfig* fs)
{
    FcitxConfigFileDesc *configDesc = GetSpellConfigDesc();
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-spell.config",
                                             "w", NULL);
    FcitxConfigSaveConfigFileFp(fp, &fs->gconfig, configDesc);
    if (fp)
        fclose(fp);
}

static boolean
LoadSpellConfig(FcitxSpellConfig *config)
{
    FcitxConfigFileDesc *configDesc = GetSpellConfigDesc();
    if (configDesc == NULL)
        return false;

    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-spell.config",
                                             "r", NULL);

    if (!fp) {
        if (errno == ENOENT)
            SaveSpellConfig(config);
    }
    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);
    FcitxSpellConfigConfigBind(config, cfile, configDesc);
    FcitxConfigBindSync(&config->gconfig);

    if (fp)
        fclose(fp);

    return true;
}

#ifdef PRESAGE_FOUND
static const char*
FcitxSpellGetPastStream(void *arg)
{
    FcitxSpell *spell = (FcitxSpell*)arg;
    if (!spell->past_stm)
        asprintf(&spell->past_stm, "%s%s",
                 spell->before_str, spell->current_str);
    return spell->past_stm;
}

static const char*
FcitxSpellGetFutureStream(void *arg)
{
    FcitxSpell *spell = (FcitxSpell*)arg;
    return spell->after_str;
}
#endif

static void*
SpellCreate(FcitxInstance *instance)
{
    FcitxSpell *spell = fcitx_utils_new(FcitxSpell);
    FcitxAddon* addon;
    spell->owner = instance;

    SpellCustomInit(spell);
#ifdef PRESAGE_FOUND
    presage_new(FcitxSpellGetPastStream, spell,
                FcitxSpellGetFutureStream, spell, &spell->presage);
#endif
#ifdef ENCHANT_FOUND
    spell->broker = enchant_broker_init();
    spell->cur_enchant_provider = EP_Default;
#endif

    if (!LoadSpellConfig(&spell->config)) {
        SpellDestroy(spell);
        return NULL;
    }
    ApplySpellConfig(spell);

#ifdef ENCHANT_FOUND
    /* if (spell->broker) { */
    /*     spell->enchantLanguages = fcitx_utils_new_string_list(); */
    /* } */
#endif
    SpellSetLang(spell, "en");
    addon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance),
                                      FCITX_SPELL_NAME);
    AddFunction(addon, FcitxSpellHintWords);
    AddFunction(addon, FcitxSpellAddPersonal);
    AddFunction(addon, FcitxSpellDictAvailable);
    return spell;
}

static void
SpellDestroy(void *arg)
{
    FcitxSpell *spell = (FcitxSpell*)arg;
    if (spell->dictLang)
        free(spell->dictLang);
#ifdef ENCHANT_FOUND
    if (spell->broker) {
        if (spell->enchant_dict)
            enchant_broker_free_dict(spell->broker, spell->enchant_dict);
        enchant_broker_free(spell->broker);
    }
    /* if (spell->enchantLanguages) */
    /*     fcitx_utils_free_string_list(spell->enchantLanguages); */
#endif
#ifdef PRESAGE_FOUND
    if (spell->presage) {
        presage_free(spell->presage);
        spell->presage = NULL;
    }
#endif
    SpellCustomDestroy(spell);
    free(arg);
}

#ifdef ENCHANT_FOUND
static void
ApplyEnchantConfig(FcitxSpell *spell)
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
#endif

static void
ApplySpellConfig(FcitxSpell *spell)
{
    if (SpellOrderHasValidProvider(spell->config.provider_order)) {
        spell->provider_order = spell->config.provider_order;
    } else {
        spell->provider_order = "presage,custom,enchant";
    }
#ifdef ENCHANT_FOUND
    ApplyEnchantConfig(spell);
#endif
}

static void
SpellReloadConfig(void* arg)
{
    FcitxSpell *spell = (FcitxSpell*)arg;
    LoadSpellConfig(&spell->config);
    ApplySpellConfig(spell);
}

static void
SpellSetLang(FcitxSpell *spell, const char *lang)
{
    if (!lang || !lang[0])
        return;
    if (spell->dictLang) {
        if (!strcmp(spell->dictLang, lang))
            return;
    }
    SpellCustomLoadDict(spell, lang);
#ifdef ENCHANT_FOUND
    if (spell->broker) {
        if (spell->enchant_dict) {
            enchant_broker_free_dict(spell->broker, spell->enchant_dict);
            spell->enchant_dict = NULL;
        }
        spell->enchant_dict = enchant_broker_request_dict(spell->broker, lang);
    }
#endif
#ifdef PRESAGE_FOUND
    if (!strcmp(lang, "en")) {
        spell->presage_support = true;
    } else {
        spell->presage_support = false;
    }
#endif
    if (spell->dictLang)
        free(spell->dictLang);
    spell->dictLang = strdup(lang);
}

static int
SpellCalListSize(char **list, int count)
{
    int i;
    if (!list)
        return 0;
    if (count >= 0)
        return count;
    for (i = 0;list[i];i++) {
    }
    return i;
}

SpellHint*
SpellHintList(int count, char **displays, char **commits)
{
    SpellHint *res;
    void *p;
    int i;
    count = SpellCalListSize(displays, count);
    if (!count)
        return NULL;
    int lens[count][2];
    int total_l = 0;
    for (i = 0;i < count;i++) {
        total_l += lens[i][0] = strlen(displays[i]) + 1;
        total_l += lens[i][1] = ((commits && commits[i]) ?
                                 (strlen(commits[i]) + 1) : 0);
    }
    res = fcitx_utils_malloc0(total_l + sizeof(SpellHint) * (count + 1));
    p = res + count + 1;
    for (i = 0;i < count;i++) {
        memcpy(p, displays[i], lens[i][0]);
        res[i].display = p;
        p += lens[i][0];
        if (!lens[i][1]) {
            res[i].commit = res[i].display;
            continue;
        }
        memcpy(p, commits[i], lens[i][1]);
        res[i].commit = p;
        p += lens[i][1];
    }
    return res;
}

#ifdef PRESAGE_FOUND
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
        presage_completion(spell->presage, suggestions[i], &result);
        if (!result)
            continue;
        tmp_str = fcitx_utils_trim(result);
        presage_free_string(result);
        asprintf(&result, "%s%s", spell->current_str, tmp_str);
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

static SpellHint*
SpellPresageHintWords(FcitxSpell *spell, unsigned int len_limit)
{
    SpellHint *res = NULL;
    if (!(spell->presage && spell->presage_support))
        return NULL;
    do {
        char **suggestions = NULL;
        char buf[(int)(sizeof(unsigned int) * 5.545177444479562) + 1];
        sprintf(buf, "%u", len_limit);
        presage_config_set(spell->presage,
                           "Presage.Selector.SUGGESTIONS", buf);
        presage_predict(spell->presage, &suggestions);
        if (!suggestions)
            break;
        res = SpellPresageResult(spell, suggestions);
        presage_free_string_array(suggestions);
    } while(0);
    if (spell->past_stm) {
        free(spell->past_stm);
        spell->past_stm = NULL;
    }
    return res;
}

static boolean
SpellPresageCheck(FcitxSpell *spell)
{
    if (spell->presage && spell->presage_support)
        return true;
    return false;
}
#endif

#ifdef ENCHANT_FOUND
static SpellHint*
SpellEnchantHintWords(FcitxSpell *spell, unsigned int len_limit)
{
    SpellHint *res = NULL;
    if (!spell->enchant_dict)
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

static boolean
SpellEnchantCheck(FcitxSpell *spell)
{
    if (spell->enchant_dict)
        return true;
    return false;
}
#endif

typedef SpellHint *(*SpellProviderHintFunc)(FcitxSpell *spell,
                                            unsigned int len_limit);
typedef boolean (*SpellProviderCheckFunc)(FcitxSpell *spell);

typedef struct {
    const char *name;
    const char *short_name;
    SpellProviderHintFunc hint_func;
    SpellProviderCheckFunc check_func;
} SpellHintProvider;

static const char*
SpellParseNextProvider(const char *str, const char **name, int *len)
{
    char *p;
    if (!name || !len)
        return str;
    if (!str || !str[0]) {
        *name = NULL;
        *len = 0;
        return NULL;
    }
    *name = str;
    p = index(str, ',');
    if (!p) {
        *len = -1;
        return NULL;
    }
    *len = p - str;
    return p + 1;
}

static SpellHintProvider hint_provider[] = {
#ifdef ENCHANT_FOUND
    {"enchant", "en", SpellEnchantHintWords, SpellEnchantCheck},
#endif
#ifdef PRESAGE_FOUND
    {"presage", "pre", SpellPresageHintWords, SpellPresageCheck},
#endif
    {"custom", "cus", SpellCustomHintWords, SpellCustomCheck},
    {NULL, NULL, NULL, NULL}
};

static SpellHintProvider*
SpellFindHintProvider(const char *str, int len)
{
    int i;
    if (!str)
        return NULL;
    if (len < 0)
        len = strlen(str);
    if (!len)
        return NULL;
    for (i = 0;hint_provider[i].hint_func;i++) {
        if ((strlen(hint_provider[i].name) == len &&
             !strncasecmp(str, hint_provider[i].name, len)) ||
            (strlen(hint_provider[i].short_name) == len &&
             !strncasecmp(str, hint_provider[i].short_name, len)))
            return hint_provider + i;
    }
    return NULL;
}

static boolean
SpellOrderHasValidProvider(const char *providers)
{
    const char *name = NULL;
    int len = 0;
    while (true) {
        providers = SpellParseNextProvider(providers, &name, &len);
        if (!name)
            break;
        if (SpellFindHintProvider(name, len))
            return true;
    }
    return false;
}

static SpellHint*
SpellGetSpellHintWords(FcitxSpell *spell, const char *before_str,
                       const char *current_str, const char *after_str,
                       unsigned int len_limit, const char *lang,
                       const char *providers)
{
    SpellHint *res = NULL;
    SpellHintProvider *hint_provider;
    const char *iter = providers ? providers : spell->provider_order;
    const char *name = NULL;
    int len = 0;
    SpellSetLang(spell, lang);
    spell->before_str = before_str ? before_str : "";
    spell->current_str = current_str ? current_str : "";
    spell->after_str = after_str ? after_str : "";
    while (true) {
        iter = SpellParseNextProvider(iter, &name, &len);
        if (!name)
            break;
        hint_provider = SpellFindHintProvider(name, len);
        if (hint_provider)
            res = hint_provider->hint_func(spell, len_limit);
        if (res)
            break;
    }
    spell->before_str = NULL;
    spell->current_str = NULL;
    spell->after_str = NULL;
    return res;
}

static void*
FcitxSpellHintWords(void *arg, FcitxModuleFunctionArg args)
{
    FcitxSpell *spell = (FcitxSpell*)arg;
    const char *before_str = args.args[0];
    const char *current_str = args.args[1];
    const char *after_str = args.args[2];
    /* from GPOINTER_TO_UINT */
    unsigned int len_limit = (unsigned int)(unsigned long)args.args[3];
    const char *lang = args.args[4];
    const char *providers = args.args[5];
    return SpellGetSpellHintWords(spell, before_str, current_str, after_str,
                                  len_limit, lang, providers);
}

static boolean
SpellAddPersonal(FcitxSpell *spell, const char *new_word, const char *lang)
{
    size_t len;
    if (!new_word)
        return false;
    len = strlen(new_word);
    if (!len)
        return false;
    SpellSetLang(spell, lang);
    /* enchant is the only one that support personal dict now (AFAIK) */
#ifdef ENCHANT_FOUND
    if (spell->enchant_dict) {
        enchant_dict_add_to_personal(spell->enchant_dict, new_word, len);
        return true;
    }
#endif
    return false;
}

static void*
FcitxSpellAddPersonal(void *arg, FcitxModuleFunctionArg args)
{
    FcitxSpell *spell = (FcitxSpell*)arg;
    const char *new_word = args.args[0];
    const char *lang = args.args[1];
    return (void*)(unsigned long)SpellAddPersonal(spell, new_word, lang);
}

static void*
FcitxSpellDictAvailable(void *arg, FcitxModuleFunctionArg args)
{
    FcitxSpell *spell = (FcitxSpell*)arg;
    const char *lang = args.args[0];
    const char *providers = args.args[1];
    const char *iter = providers ? providers : spell->provider_order;
    SpellHintProvider *hint_provider;
    const char *name = NULL;
    int len = 0;
    SpellSetLang(spell, lang);
    while (true) {
        iter = SpellParseNextProvider(iter, &name, &len);
        if (!name)
            break;
        hint_provider = SpellFindHintProvider(name, len);
        if (hint_provider && hint_provider->check_func) {
            if (hint_provider->check_func(spell))
                return (void*)true;
        }
    }
    return (void*)false;
}
