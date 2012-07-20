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

#include "spell.h"

CONFIG_DESC_DEFINE(GetSpellConfigDesc, "fcitx-spell.desc");
static void *SpellCreate(FcitxInstance *instance);
// static void LoadSpell(FcitxSpell *spell);
static void SpellDestroy(void *arg);
static void SpellReloadConfig(void *arg);

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
FcitxSpellGetPastStream(void* arg)
{
    return "";
}

static const char*
FcitxSpellGetFutureStream(void* arg)
{
    return "";
}
#endif

static void*
SpellCreate(FcitxInstance *instance)
{
    FcitxSpell* spell = fcitx_utils_malloc0(sizeof(FcitxSpell));
    spell->owner = instance;
    if (!LoadSpellConfig(&spell->config)) {
        free(spell);
        return NULL;
    }

#ifdef PRESAGE_FOUND
    presage_new(FcitxSpellGetPastStream, spell,
                FcitxSpellGetFutureStream, spell, &spell->presage);
#endif
#ifdef ENCHANT_FOUND
    spell->broker = enchant_broker_init();
    switch (spell->config.provider) {
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
    spell->enchantLanguages = fcitx_utils_new_string_list();
#endif
    return spell;
}

static void
SpellDestroy(void *arg)
{
}

static void
SpellReloadConfig(void* arg)
{
    FcitxSpell *spell = (FcitxSpell*)arg;
    LoadSpellConfig(&spell->config);
}
