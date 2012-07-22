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
#include <iconv.h>
#include <ctype.h>

#include <unicode/unorm.h>

#include "fcitx/ime.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"

#include "spell-internal.h"

static CONFIG_DESC_DEFINE(GetSpellConfigDesc, "fcitx-spell.desc");
static void *SpellCreate(FcitxInstance *instance);
static void SpellDestroy(void *arg);
static void SpellReloadConfig(void *arg);
static void SpellSetLang(FcitxSpell *spell, const char *lang);

static void *FcitxSpellHintWords(void *arg, FcitxModuleFunctionArg args);
static void *FcitxSpellAddPersonal(void *arg, FcitxModuleFunctionArg args);
static void *FcitxSpellDictAvailable(void *arg, FcitxModuleFunctionArg args);

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
    if (!LoadSpellConfig(&spell->config)) {
        free(spell);
        return NULL;
    }

#ifdef PRESAGE_FOUND
    presage_new(FcitxSpellGetPastStream, spell,
                FcitxSpellGetFutureStream, spell, &spell->presage);
    if (!spell->presage) {
        SpellDestroy(spell);
        return NULL;
    }
#endif
#ifdef ENCHANT_FOUND
    spell->broker = enchant_broker_init();
    if (!spell->broker) {
        SpellDestroy(spell);
        return NULL;
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
        case EP_Default:
        default:
            break;
    }
    /* spell->enchantLanguages = fcitx_utils_new_string_list(); */
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
        if (spell->dict)
            enchant_broker_free_dict(spell->broker, spell->dict);
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
    free(arg);
}

static void
ApplySpellConfig(FcitxSpell *spell)
{

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
        free(spell->dictLang);
    }
    spell->dictLang = strdup(lang);
#ifdef ENCHANT_FOUND
    if (spell->dict) {
        enchant_broker_free_dict(spell->broker, spell->dict);
        spell->dict = NULL;
    }
    spell->dict = enchant_broker_request_dict(spell->broker, lang);
#endif
#ifdef PRESAGE_FOUND
    if (!strcmp(lang, "en")) {
        spell->presage_support = true;
    } else {
        spell->presage_support = false;
    }
#endif
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

static SpellHint*
SpellHintList(int count, char **displays, char **commits)
{
    SpellHint *res;
    void *p;
    int i;
    count = SpellCalListSize(displays, count);
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
    if (!(spell->config.usePresage && spell->presage && spell->presage_support))
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
#endif

#ifdef ENCHANT_FOUND
static SpellHint*
SpellEnchantHintWords(FcitxSpell *spell, unsigned int len_limit)
{
    SpellHint *res = NULL;
    if (!spell->dict)
        return NULL;
    char **suggestions = NULL;
    size_t number = 0;
    suggestions = enchant_dict_suggest(spell->dict, spell->current_str,
                                       strlen(spell->current_str), &number);
    if (!suggestions)
        return NULL;
    number = number > len_limit ? len_limit : number;
    res = SpellHintList(number, suggestions, NULL);
    enchant_dict_free_string_list(spell->dict, suggestions);
    return res;
}
#endif

typedef SpellHint *(*SpellHintProviderFunc)(FcitxSpell *spell,
                                            unsigned int len_limit);

typedef struct {
    const char *name;
    const char *short_name;
    SpellHintProviderFunc func;
} SpellHintProvider;

static SpellHintProvider hint_provider[] = {
#ifdef ENCHANT_FOUND
    {"enchant", "en", SpellEnchantHintWords},
#endif
#ifdef PRESAGE_FOUND
    {"presage", "pre", SpellPresageHintWords},
#endif
    {NULL, NULL, NULL}
};

static SpellHintProviderFunc
SpellFindHintProvider(const char *str, int len)
{
    int i;
    if (!str)
        return NULL;
    if (len < 0)
        len = strlen(str);
    if (!len)
        return NULL;
    for (i = 0;hint_provider[i].func;i++) {
        if ((strlen(hint_provider[i].name) == len &&
             !strncasecmp(str, hint_provider[i].name, len)) ||
            (strlen(hint_provider[i].short_name) == len &&
             !strncasecmp(str, hint_provider[i].short_name, len)))
            return hint_provider[i].func;
    }
    return NULL;
}

static SpellHint*
SpellGetSpellHintWords(FcitxSpell *spell, const char *before_str,
                       const char *current_str, const char *after_str,
                       unsigned int len_limit, const char *lang,
                       const char *providers)
{
    SpellHint *res = NULL;
    SpellHintProviderFunc provider_func;
    SpellSetLang(spell, lang);
    spell->before_str = before_str ? before_str : "";
    spell->current_str = current_str ? current_str : "";
    spell->after_str = after_str ? after_str : "";
#ifdef PRESAGE_FOUND
    provider_func = SpellFindHintProvider("pre", -1);
    if (provider_func)
        res = provider_func(spell, len_limit);
    if (res)
        goto out;
#endif
#ifdef ENCHANT_FOUND
    provider_func = SpellFindHintProvider("en", -1);
    if (provider_func)
        res = provider_func(spell, len_limit);
    if (res)
        goto out;
#endif
out:
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
#ifdef ENCHANT_FOUND
    if (spell->dict) {
        enchant_dict_add_to_personal(spell->dict, new_word, len);
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
    SpellSetLang(spell, lang);
#ifdef ENCHANT_FOUND
    if (spell->dict)
        return (void*)true;
#endif
#ifdef PRESAGE_FOUND
    if (spell->config.usePresage && spell->presage && spell->presage_support)
        return (void*)true;
#endif
    return (void*)false;
}
