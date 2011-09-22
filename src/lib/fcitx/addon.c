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

/**
 * @file addon.c
 * @brief Addon Support for fcitx
 * @author CSSlayer wengxt@gmail.com
 */

#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libintl.h>
#include <dlfcn.h>

#include "fcitx/fcitx.h"
#include "addon.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"
#include "instance.h"
#include "instance-internal.h"
#include "addon-internal.h"

CONFIG_BINDING_BEGIN(FcitxAddon)
CONFIG_BINDING_REGISTER("Addon", "Name", name)
CONFIG_BINDING_REGISTER("Addon", "GeneralName", generalname)
CONFIG_BINDING_REGISTER("Addon", "Comment", comment)
CONFIG_BINDING_REGISTER("Addon", "Category", category)
CONFIG_BINDING_REGISTER("Addon", "Enabled", bEnabled)
CONFIG_BINDING_REGISTER("Addon", "Library", library)
CONFIG_BINDING_REGISTER("Addon", "Type", type)
CONFIG_BINDING_REGISTER("Addon", "Dependency", depend)
CONFIG_BINDING_REGISTER("Addon", "Priority", priority)
CONFIG_BINDING_REGISTER("Addon", "SubConfig", subconfig)
CONFIG_BINDING_END()

static const UT_icd function_icd = {sizeof(void*), 0, 0 , 0};
static const UT_icd addon_icd = {sizeof(FcitxAddon), NULL , NULL, FreeAddon};
static int AddonPriorityCmp(const void* a, const void* b)
{
    FcitxAddon *aa = (FcitxAddon*)a, *ab = (FcitxAddon*)b;
    return aa->priority - ab->priority;
}

FCITX_EXPORT_API
void InitFcitxAddons(UT_array* addons)
{
    utarray_init(addons, &addon_icd);
}

/**
 * @brief Load Addon Info
 */
FCITX_EXPORT_API
void LoadAddonInfo(UT_array* addons)
{
    char **addonPath;
    size_t len;
    char pathBuf[PATH_MAX];
    size_t i = 0;
    DIR *dir;
    struct dirent *drt;
    struct stat fileStat;
    utarray_clear(addons);

    StringHashSet* sset = NULL;

    addonPath = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", PACKAGE "/addon" , DATADIR, PACKAGE "/addon");

    for (i = 0; i < len; i++) {
        snprintf(pathBuf, sizeof(pathBuf), "%s", addonPath[i]);
        pathBuf[sizeof(pathBuf) - 1] = '\0';

        dir = opendir(pathBuf);
        if (dir == NULL)
            continue;

        /* collect all *.conf files */
        while ((drt = readdir(dir)) != NULL) {
            size_t nameLen = strlen(drt->d_name);
            if (nameLen <= strlen(".conf"))
                continue;
            memset(pathBuf, 0, sizeof(pathBuf));

            if (strcmp(drt->d_name + nameLen - strlen(".conf"), ".conf") != 0)
                continue;
            snprintf(pathBuf, sizeof(pathBuf), "%s/%s", addonPath[i], drt->d_name);

            if (stat(pathBuf, &fileStat) == -1)
                continue;

            if (fileStat.st_mode & S_IFREG) {
                StringHashSet *string;
                HASH_FIND_STR(sset, drt->d_name, string);
                if (!string) {
                    char *bStr = strdup(drt->d_name);
                    string = malloc(sizeof(StringHashSet));
                    memset(string, 0, sizeof(StringHashSet));
                    string->name = bStr;
                    HASH_ADD_KEYPTR(hh, sset, string->name, strlen(string->name), string);
                }
            }
        }

        closedir(dir);
    }

    char **paths = malloc(sizeof(char*) * len);
    for (i = 0; i < len ; i ++)
        paths[i] = malloc(sizeof(char) * PATH_MAX);
    StringHashSet* string;
    for (string = sset;
            string != NULL;
            string = (StringHashSet*)string->hh.next) {
        int i = 0;
        for (i = len - 1; i >= 0; i--) {
            snprintf(paths[i], PATH_MAX, "%s/%s", addonPath[len - i - 1], string->name);
            FcitxLog(DEBUG, "Load Addon Config File:%s", paths[i]);
        }
        FcitxLog(INFO, _("Load Addon Config File:%s"), string->name);
        ConfigFile* cfile = ParseMultiConfigFile(paths, len, GetAddonConfigDesc());
        if (cfile) {
            FcitxAddon addon;
            memset(&addon, 0, sizeof(FcitxAddon));
            utarray_push_back(addons, &addon);
            FcitxAddon *a = (FcitxAddon*) utarray_back(addons);
            utarray_init(&a->functionList, &function_icd);
            FcitxAddonConfigBind(a, cfile, GetAddonConfigDesc());
            ConfigBindSync((GenericConfig*)a);
            FcitxLog(DEBUG, _("Addon Config %s is %s"), string->name, (a->bEnabled) ? "Enabled" : "Disabled");
        }
    }

    for (i = 0; i < len ; i ++)
        free(paths[i]);
    free(paths);

    FreeXDGPath(addonPath);

    StringHashSet *curStr;
    while (sset) {
        curStr = sset;
        HASH_DEL(sset, curStr);
        free(curStr->name);
        free(curStr);
    }

    utarray_sort(addons, AddonPriorityCmp);
}

FCITX_EXPORT_API
void AddonResolveDependency(FcitxInstance* instance)
{
    UT_array* addons = &instance->addons;
    boolean remove = true;
    FcitxAddon *addon;

    /* choose ui */
    boolean founduiflag = false;
    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if (addon->category == AC_UI) {
            if (instance->uiname == NULL) {
                if (addon->bEnabled) {
                    if (!founduiflag)
                        founduiflag = true;
                    else
                        addon->bEnabled = false;
                }
            } else {
                if (strcmp(instance->uiname, addon->name) != 0)
                    addon->bEnabled = false;
                else
                    addon->bEnabled = true;
            }
        }
    }

    while (remove) {
        remove = false;
        for (addon = (FcitxAddon *) utarray_front(addons);
                addon != NULL;
                addon = (FcitxAddon *) utarray_next(addons, addon)) {
            if (!addon->bEnabled)
                continue;
            UT_array* dependlist = SplitString(addon->depend, ',');
            boolean valid = true;
            char **depend = NULL;
            for (depend = (char **) utarray_front(dependlist);
                    depend != NULL;
                    depend = (char **) utarray_next(dependlist, depend)) {
                if (!AddonIsAvailable(addons, *depend)) {
                    valid = false;
                    break;
                }
            }

            utarray_free(dependlist);
            if (!valid) {
                FcitxLog(WARNING, _("Disable addon %s, dependency %s can not be satisfied."), addon->name, addon->depend);
                addon->bEnabled = false;
            }
        }
    }
}

FCITX_EXPORT_API
boolean AddonIsAvailable(UT_array* addons, const char* name)
{
    FcitxAddon *addon;
    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if (addon->bEnabled && strcmp(name, addon->name) == 0)
            return true;
    }
    return false;
}

FCITX_EXPORT_API
FcitxAddon* GetAddonByName(UT_array* addons, const char* name)
{
    FcitxAddon *addon;
    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if (addon->bEnabled && strcmp(name, addon->name) == 0)
            return addon;
    }
    return NULL;
}

/**
 * @brief Load addon.desc file
 *
 * @return the description of addon configure.
 */
FCITX_EXPORT_API
CONFIG_DESC_DEFINE(GetAddonConfigDesc, "addon.desc")

FCITX_EXPORT_API
void FreeAddon(void *v)
{
    FcitxAddon *addon = (FcitxAddon*) v;
    if (!addon)
        return ;
    FreeConfigFile(addon->config.configFile);
    free(addon->name);
    free(addon->library);
    free(addon->comment);
    free(addon->generalname);
    free(addon->depend);
    free(addon->subconfig);
}

boolean CheckABIVersion(void* handle)
{
    int* version = (int*) dlsym(handle, "ABI_VERSION");
    if (!version)
        return false;
    if (*version < FCITX_ABI_VERSION)
        return false;
    return true;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
