/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <errno.h>
#include <libintl.h>

#include "fcitx.h"
#include "fcitx-utils/log.h"
#include "profile.h"
#include "fcitx-config/xdg.h"
#include "instance-internal.h"
#include "instance.h"
#include "ime-internal.h"

static ConfigFileDesc* GetProfileDesc();
static void FilterIMName(GenericConfig* config, ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);
static void FilterIMList(GenericConfig* config, ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);

CONFIG_BINDING_BEGIN_WITH_ARG(FcitxProfile, FcitxInstance* instance)
CONFIG_BINDING_REGISTER("Profile", "FullWidth", bUseFullWidthChar)
CONFIG_BINDING_REGISTER("Profile", "UseRemind", bUseRemind)
CONFIG_BINDING_REGISTER_WITH_FILTER_ARG("Profile", "IMName", imName, FilterIMName, instance)
CONFIG_BINDING_REGISTER("Profile", "WidePunc", bUseWidePunc)
CONFIG_BINDING_REGISTER("Profile", "PreeditStringInClientWindow", bUsePreedit)
CONFIG_BINDING_REGISTER_WITH_FILTER_ARG("Profile", "EnabledIMList", imList, FilterIMList, instance)
CONFIG_BINDING_END()

/**
 * @brief 加载配置文件
 */
FCITX_EXPORT_API
boolean LoadProfile(FcitxProfile* profile, FcitxInstance* instance)
{
    ConfigFileDesc* profileDesc = GetProfileDesc();
    if (!profileDesc)
        return false;

    FILE *fp;
    fp = GetXDGFileUserWithPrefix("", "profile", "rt", NULL);
    if (!fp) {
        if (errno == ENOENT)
            SaveProfile(profile);
    }

    ConfigFile *cfile = ParseConfigFileFp(fp, profileDesc);

    FcitxProfileConfigBind(profile, cfile, profileDesc, instance);
    ConfigBindSync(&profile->gconfig);

    if (fp)
        fclose(fp);

    UpdateIMList(instance);

    return true;
}

CONFIG_DESC_DEFINE(GetProfileDesc, "profile.desc")

FCITX_EXPORT_API
void SaveProfile(FcitxProfile* profile)
{
    ConfigFileDesc* profileDesc = GetProfileDesc();
    FILE* fp = GetXDGFileUserWithPrefix("", "profile", "wt", NULL);
    SaveConfigFileFp(fp, &profile->gconfig, profileDesc);
    if (fp)
        fclose(fp);
}

FCITX_EXPORT_API
boolean UseRemind(FcitxProfile* profile)
{
    return profile->bUseRemind;
}

void FilterIMList(GenericConfig* config, ConfigGroup* group, ConfigOption* option, void* value, ConfigSync sync, void* arg)
{
    FcitxInstance* instance = arg;
    if (sync == Value2Raw) {
        char* result = NULL;
        FcitxIM* ime;
        for (ime = (FcitxIM*) utarray_front(&instance->imes);
                ime != NULL;
                ime = (FcitxIM*) utarray_next(&instance->imes, ime)) {
            char* newresult;
            if (result == NULL)
                asprintf(&newresult, "%s:True", ime->uniqueName);
            else
                asprintf(&newresult, "%s,%s:True", result, ime->uniqueName);
            if (result)
                free(result);
            result = newresult;
        }

        for (ime = (FcitxIM*) utarray_front(&instance->availimes);
                ime != NULL;
                ime = (FcitxIM*) utarray_next(&instance->availimes, ime)) {
            if (!GetIMFromIMList(&instance->imes, ime->uniqueName)) {
                char* newresult;
                if (result == NULL)
                    asprintf(&newresult, "%s:False", ime->uniqueName);
                else
                    asprintf(&newresult, "%s,%s:False", result, ime->uniqueName);
                if (result)
                    free(result);
                result = newresult;
            }
        }

        char** imList = (char**) value;

        if (*imList)
            free(*imList);
        if (result)
            *imList = result;
        else
            *imList = strdup("");
    }
}

void FilterIMName(GenericConfig* config, ConfigGroup* group, ConfigOption* option, void* value, ConfigSync sync, void* arg)
{
    FcitxInstance* instance = arg;
    if (sync == Value2Raw) {
        FcitxIM* im = GetCurrentIM(instance);
        char** imName = (char**) value;
        if (*imName)
            free(*imName);

        if (im)
            *imName = strdup(im->uniqueName);
        else
            *imName = strdup("");
    }
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
