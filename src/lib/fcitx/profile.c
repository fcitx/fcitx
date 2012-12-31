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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
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

static FcitxConfigFileDesc* GetProfileDesc();
static void FilterIMName(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void* value, FcitxConfigSync sync, void* arg);
static void FilterIMList(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void* value, FcitxConfigSync sync, void* arg);

CONFIG_BINDING_BEGIN_WITH_ARG(FcitxProfile, FcitxInstance* instance)
CONFIG_BINDING_REGISTER("Profile", "FullWidth", bUseFullWidthChar)
CONFIG_BINDING_REGISTER("Profile", "UseRemind", bUseRemind)
CONFIG_BINDING_REGISTER_WITH_FILTER_ARG("Profile", "IMName", imName, FilterIMName, instance)
CONFIG_BINDING_REGISTER("Profile", "WidePunc", bUseWidePunc)
CONFIG_BINDING_REGISTER("Profile", "PreeditStringInClientWindow", bUsePreedit)
CONFIG_BINDING_REGISTER_WITH_FILTER_ARG("Profile", "EnabledIMList", imList, FilterIMList, instance)
CONFIG_BINDING_END()

/**
 * 加载配置文件
 */
FCITX_EXPORT_API
boolean FcitxProfileLoad(FcitxProfile* profile, FcitxInstance* instance)
{
    FcitxConfigFileDesc* profileDesc = GetProfileDesc();
    if (!profileDesc)
        return false;

    FILE *fp;
    fp = FcitxXDGGetFileUserWithPrefix("", "profile", "r", NULL);
    if (!fp) {
        if (errno == ENOENT)
            FcitxProfileSave(profile);
    }

    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, profileDesc);

    FcitxProfileConfigBind(profile, cfile, profileDesc, instance);
    FcitxConfigBindSync(&profile->gconfig);

    if (fp)
        fclose(fp);

    /*
     * Do we need to load im list here? No!
     * There is only two place call this function.
     *
     * 1. in initialize phase, this function is useless, due to
     * im is not loaded. (See the FcitxInstanceUpdateIMList impl
     * and check imLoaded boolean)
     * 2. in reload phase, this function is useless too, due to
     * LoadAllIM will be called anyway.
     */
    // FcitxInstanceUpdateIMList(instance);

    return true;
}

CONFIG_DESC_DEFINE(GetProfileDesc, "profile.desc")

#define PROFILE_TEMP_FILE "profile_XXXXXX"


FCITX_EXPORT_API
void FcitxProfileSave(FcitxProfile* profile)
{
    /* profile save need to be safe, so we put an rename trick here */
    FcitxConfigFileDesc* profileDesc = GetProfileDesc();
    do {
        if (!profileDesc)
            break;
        // make ~/.config/fcitx
        char* tempfile = NULL;
        FcitxXDGGetFileUserWithPrefix("", "", "w", NULL);
        FcitxXDGGetFileUserWithPrefix("", PROFILE_TEMP_FILE, NULL, &tempfile);
        int fd = mkstemp(tempfile);

        FILE* fp;
        if (fd <= 0) {
            fcitx_utils_free(tempfile);
            break;
        }

        fp = fdopen(fd, "w");
        FcitxConfigSaveConfigFileFp(fp, &profile->gconfig, profileDesc);
        if (fp)
            fclose(fp);
        char* profileFileName = 0;
        FcitxXDGGetFileUserWithPrefix("", "profile", NULL, &profileFileName);
        if (access(profileFileName, 0))
            unlink(profileFileName);
        rename(tempfile, profileFileName);

        free(tempfile);
        free(profileFileName);
    } while(0);
}

void
FilterIMList(FcitxGenericConfig* config, FcitxConfigGroup* group,
             FcitxConfigOption* option, void* value,
             FcitxConfigSync sync, void* arg)
{
    FCITX_UNUSED(config);
    FCITX_UNUSED(group);
    FCITX_UNUSED(option);
    FcitxInstance* instance = arg;
    if (sync == Value2Raw) {
        size_t imes_len = utarray_len(&instance->imes);
        size_t avail_len = utarray_len(&instance->availimes);
        size_t count = (imes_len + avail_len +
                        HASH_COUNT(instance->unusedItem)) * 2;
        const char *names[count];
        size_t name_lens[count];
        count = 0;
        size_t res_len = 0;
        unsigned int i;
        FcitxIM *ime;
#define ADD_IM_NAME(imname, state, state_len)  do {             \
            names[count] = imname;                              \
            res_len += (name_lens[count] = strlen(imname));     \
            count++;                                            \
            names[count] = state;                               \
            res_len += (name_lens[count] = state_len);          \
            count++;                                            \
        } while (0)

        for (i = 0;i < imes_len;i++) {
            ime = (FcitxIM*)_utarray_eltptr(&instance->imes, i);
            ADD_IM_NAME(ime->uniqueName, ":True,", strlen(":True,"));
        }
        for (i = 0;i < avail_len;i++) {
            ime = (FcitxIM*)_utarray_eltptr(&instance->availimes, i);
            if (FcitxInstanceGetIMFromIMList(instance, IMAS_Enable,
                                             ime->uniqueName))
                continue;
            ADD_IM_NAME(ime->uniqueName, ":False,", strlen(":False,"));
        }
        UnusedIMItem* item = instance->unusedItem;
        while(item) {
            char *status;
            size_t s_len;
            if (item->status) {
                status = ":True,";
                s_len = strlen(":True,");
            } else {
                status = ":False,";
                s_len = strlen(":False,");
            }
            ADD_IM_NAME(item->name, status, s_len);
            item = item->hh.next;
        }
        char** imList = (char**)value;
        if (*imList)
            free(*imList);
        if (!count) {
            *imList = strdup("");
            return;
        }
        name_lens[count - 1]--;
        *imList = malloc(res_len);
        fcitx_utils_cat_str(*imList, count, names, name_lens);
#undef ADD_IM_NAME
    }
}

void FilterIMName(FcitxGenericConfig* config, FcitxConfigGroup* group, FcitxConfigOption* option, void* value, FcitxConfigSync sync, void* arg)
{
    FCITX_UNUSED(config);
    FCITX_UNUSED(group);
    FCITX_UNUSED(option);
    FcitxInstance* instance = arg;
    char** imName = (char**) value;
    if (sync == Value2Raw) {
        if (instance->globalIMName) {
            fcitx_utils_string_swap(imName, instance->globalIMName);
        }
    } else if (sync == Raw2Value) {
        if (*imName) {
            fcitx_utils_string_swap(&instance->globalIMName, *imName);
        }
    }
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
