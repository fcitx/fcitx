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
#include "spell-custom.h"
#ifdef ENABLE_PRESAGE
#  include "spell-presage.h"
#endif
#ifdef ENABLE_ENCHANT
#  include "spell-enchant.h"
#endif

static CONFIG_DESC_DEFINE(GetSpellConfigDesc, "fcitx-spell.desc");
static void *SpellCreate(FcitxInstance *instance);
static void SpellDestroy(void *arg);
static void SpellReloadConfig(void *arg);
static void SpellSetLang(FcitxSpell *spell, const char *lang);
static void ApplySpellConfig(FcitxSpell *spell);
static boolean SpellOrderHasValidProvider(const char *providers);

DECLARE_ADDFUNCTIONS(Spell)

FCITX_DEFINE_PLUGIN(fcitx_spell, module, FcitxModule) = {
    .Create = SpellCreate,
    .Destroy = SpellDestroy,
    .SetFD = NULL,
    .ProcessEvent = NULL,
    .ReloadConfig = SpellReloadConfig
};

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

static void*
SpellCreate(FcitxInstance *instance)
{
    FcitxSpell *spell = fcitx_utils_new(FcitxSpell);
    spell->owner = instance;

    /* SpellCustomInit(spell); */
#ifdef ENABLE_PRESAGE
    SpellPresageInit(spell);
#endif
#ifdef ENABLE_ENCHANT
    SpellEnchantInit(spell);
#endif

    if (!LoadSpellConfig(&spell->config)) {
        SpellDestroy(spell);
        return NULL;
    }
    ApplySpellConfig(spell);

    SpellSetLang(spell, "en");
    FcitxSpellAddFunctions(instance);
    return spell;
}

static void
SpellDestroy(void *arg)
{
    FcitxSpell *spell = (FcitxSpell*)arg;
    if (spell->dictLang)
        free(spell->dictLang);
#ifdef ENABLE_ENCHANT
    SpellEnchantDestroy(spell);
#endif
#ifdef ENABLE_PRESAGE
    SpellPresageDestroy(spell);
#endif
    SpellCustomDestroy(spell);
    free(arg);
}

static void
ApplySpellConfig(FcitxSpell *spell)
{
    if (SpellOrderHasValidProvider(spell->config.provider_order)) {
        spell->provider_order = spell->config.provider_order;
    } else {
        spell->provider_order = "presage,custom,enchant";
    }
#ifdef ENABLE_ENCHANT
    SpellEnchantApplyConfig(spell);
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
    /* struct timespec start, end; */
    /* int t; */
    /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); */
    SpellCustomLoadDict(spell, lang);
    /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end); */
    /* t = ((end.tv_sec - start.tv_sec) * 1000000000) */
    /*     + end.tv_nsec - start.tv_nsec; */
    /* printf("%s, %d\n", __func__, t); */
#ifdef ENABLE_ENCHANT
    SpellEnchantLoadDict(spell, lang);
#endif
#ifdef ENABLE_PRESAGE
    SpellPresageLoadDict(spell, lang);
#endif
    if (spell->dictLang)
        free(spell->dictLang);
    spell->dictLang = strdup(lang);
}

#define GET_NTH(p, size, i) (*((typeof(p))(((void*)p) + size * i)))

int
SpellCalListSizeWithSize(char **list, int count, int size)
{
    int i;
    if (!list)
        return 0;
    if (count >= 0)
        return count;
    for (i = 0;GET_NTH(list, size, i);i++) {
    }
    return i;
}

static int
SpellHintStrLengths(int count, char **strs, int sizes, int *lens)
{
    int i;
    int total = 0;
    if (!strs) {
        memset(lens, 0, sizeof(int) * count);
        return 0;
    }
    for (i = 0;i < count;i++)
        total += lens[i] = ((GET_NTH(strs, i, sizes)) ?
                            (strlen(GET_NTH(strs, i, sizes)) + 1) : 0);
    return total;
}

SpellHint*
SpellHintListWithSize(int count, char **displays, int sized,
                      char **commits, int sizec)
{
    SpellHint *res;
    void *p;
    int i;
    if (!displays && commits) {
        displays = commits;
        sized = sizec;
        commits = NULL;
        sizec = 0;
    }
    count = SpellCalListSizeWithSize(displays, count, sized);
    if (!count)
        return NULL;
    int lens[2][count];
    int total_l = 0;
    total_l += SpellHintStrLengths(count, displays, sized, lens[0]);
    total_l += SpellHintStrLengths(count, commits, sizec, lens[1]);
    res = fcitx_utils_malloc0(total_l + sizeof(SpellHint) * (count + 1));
    p = res + count + 1;
    for (i = 0;i < count;i++) {
        memcpy(p, GET_NTH(displays, i, sized), lens[0][i]);
        res[i].display = p;
        p += lens[0][i];
        if (!lens[1][i]) {
            res[i].commit = res[i].display;
            continue;
        }
        memcpy(p, GET_NTH(commits, i, sizec), lens[1][i]);
        res[i].commit = p;
        p += lens[1][i];
    }
    return res;
}

SpellHint*
SpellHintListWithPrefix(int count, const char *prefix, int prefix_len,
                        char **commits, int sizec)
{
    SpellHint *res;
    void *p;
    int i;
    count = SpellCalListSizeWithSize(commits, count, sizec);
    if (!count)
        return NULL;
    if (!prefix) {
        prefix_len = 0;
    } else if (prefix_len < 0) {
        prefix_len = strlen(prefix);
    }
    int lens[count];
    int total_l = 0;
    total_l += SpellHintStrLengths(count, commits, sizec, lens);
    total_l += prefix_len * count;
    res = fcitx_utils_malloc0(total_l + sizeof(SpellHint) * (count + 1));
    p = res + count + 1;
    for (i = 0;i < count;i++) {
        res[i].commit = p;
        if (prefix_len) {
            memcpy(p, prefix, prefix_len);
            p += prefix_len;
        }
        res[i].display = p;
        memcpy(p, GET_NTH(commits, i, sizec), lens[i]);
        p += lens[i];
    }
    return res;
}

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
        *len = strlen(str);
        return NULL;
    }
    *len = p - str;
    return p + 1;
}

static const SpellHintProvider hint_provider[] = {
#ifdef ENABLE_ENCHANT
    {"enchant", "en", SpellEnchantHintWords, SpellEnchantCheck},
#endif
#ifdef ENABLE_PRESAGE
    {"presage", "pre", SpellPresageHintWords, SpellPresageCheck},
#endif
    {"custom", "cus", SpellCustomHintWords, SpellCustomCheck},
    {NULL, NULL, NULL, NULL}
};

static const SpellHintProvider*
SpellFindHintProvider(const char *str, unsigned int len)
{
    int i;
    if (!str || !len)
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
    const SpellHintProvider *hint_provider;
    const char *iter = providers ? providers : spell->provider_order;
    const char *name = NULL;
    int len = 0;
    SpellSetLang(spell, lang);
    spell->before_str = before_str ? before_str : "";
    spell->current_str = current_str ? current_str : "";
    spell->after_str = after_str ? after_str : "";
    if (!(*spell->before_str || *spell->current_str || *spell->after_str))
        return NULL;
    while (true) {
        iter = SpellParseNextProvider(iter, &name, &len);
        if (!name)
            break;
        hint_provider = SpellFindHintProvider(name, len);
        if (hint_provider) {
            /* struct timespec start, end; */
            /* int t; */
            /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); */
            res = hint_provider->hint_func(spell, len_limit);
            /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end); */
            /* t = ((end.tv_sec - start.tv_sec) * 1000000000) */
            /*     + end.tv_nsec - start.tv_nsec; */
            /* printf("%s, %s, %d\n", __func__, hint_provider->name, t); */
        }
        if (res)
            break;
    }
    spell->before_str = NULL;
    spell->current_str = NULL;
    spell->after_str = NULL;
    return res;
}

#define HINT_WORDS_ARGC 6

static boolean
SpellAddPersonal(FcitxSpell *spell, const char *new_word, const char *lang)
{
    if (!new_word || !new_word[0])
        return false;
    SpellSetLang(spell, lang);
#ifdef ENABLE_ENCHANT
    SpellEnchantAddPersonal(spell, new_word);
#endif
    return false;
}

static boolean
FcitxSpellDictAvailable(FcitxSpell *spell, const char *lang,
                        const char *providers)
{
    const char *iter = providers ? providers : spell->provider_order;
    const SpellHintProvider *hint_provider;
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
                return true;
        }
    }
    return false;
}

boolean
SpellLangIsLang(const char *full_lang, const char *lang)
{
    int len;
    if (!full_lang || !lang || !*full_lang || !*lang)
        return false;
    len = strlen(lang);
    if (strncmp(full_lang, lang, len))
        return false;
    switch (full_lang[len]) {
    case '\0':
    case '_':
        return true;
    default:
        break;
    }
    return false;
}

typedef struct {
    FcitxSpellGetCandWordCb cb;
    void *arg;
} GetCandWordsArgs;

static const char*
FcitxSpellCandWordGetCommit(FcitxSpell *spell, FcitxCandidateWord *candWord)
{
    GetCandWordsArgs *cand_word_args = candWord->priv;
    if (cand_word_args->arg != spell)
        return NULL;
    return (const char*)(cand_word_args + 1);
}

static INPUT_RETURN_VALUE
FcitxSpellGetCandWord(void *arg, FcitxCandidateWord *candWord)
{
    GetCandWordsArgs *args = candWord->priv;
    FcitxSpell *spell = (FcitxSpell*)args->arg;
    FcitxInstance *instance = spell->owner;
    char *commit = (void*)(args + 1);
    INPUT_RETURN_VALUE res = IRV_TO_PROCESS;
    if (!args->cb || !(res = args->cb(arg, commit))) {
        FcitxInstanceCommitString(instance,
                                  FcitxInstanceGetCurrentIC(instance), commit);
        res = IRV_FLAG_RESET_INPUT;
    } else {
        res &= ~IRV_DO_NOTHING;
    }
    return IRV_FLAG_UPDATE_INPUT_WINDOW | res;
}

static void*
SpellNewGetCandWordArgs(FcitxSpellGetCandWordCb cb, void *arg, const char *commit)
{
    int len;
    void *res;
    GetCandWordsArgs *args;
    len = strlen(commit);
    args = res = fcitx_utils_malloc0(len + sizeof(GetCandWordsArgs) + 1);
    args->cb = cb;
    args->arg = arg;
    memcpy(args + 1, commit, len);
    return res;
}

static FcitxCandidateWordList*
SpellGetCandWords(FcitxSpell *spell, const char *before_str,
                  const char *current_str, const char *after_str,
                  unsigned int len_limit, const char *lang,
                  const char *providers, FcitxSpellGetCandWordCb cb, void *arg)
{
    SpellHint *hints;
    int i;
    FcitxCandidateWordList* cand_list;
    hints = SpellGetSpellHintWords(spell, before_str, current_str, after_str,
                                   len_limit, lang, providers);
    if (!hints)
        return NULL;
    cand_list =  FcitxCandidateWordNewList();
    FcitxCandidateWord candWord = {
        .callback = FcitxSpellGetCandWord,
        .owner = arg,
        .strExtra = NULL,
        .wordType = MSG_OTHER
    };
    for (i = 0;hints[i].display;i++) {
        candWord.strWord = strdup(hints[i].display);
        candWord.priv = SpellNewGetCandWordArgs(cb, spell, hints[i].commit);
        FcitxCandidateWordAppend(cand_list, &candWord);
    }
    free(hints);
    return cand_list;
}

#include "fcitx-spell-addfunctions.h"
