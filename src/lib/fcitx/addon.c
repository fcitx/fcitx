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

/**
 * @file addon.c
 * Addon Support for fcitx
 * @author CSSlayer wengxt@gmail.com
 */

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
CONFIG_BINDING_REGISTER("Addon", "IMRegisterMethod", registerMethod)
CONFIG_BINDING_REGISTER("Addon", "IMRegisterArgument", registerArgument)
CONFIG_BINDING_REGISTER("Addon", "UIFallback", uifallback)
CONFIG_BINDING_REGISTER("Addon", "Advance", advance)
CONFIG_BINDING_REGISTER("Addon", "LoadLocal", loadLocal)
CONFIG_BINDING_END()

static const UT_icd addon_icd = {
    sizeof(FcitxAddon), NULL , NULL, FcitxAddonFree
};
static int AddonPriorityCmp(const void* a, const void* b)
{
    FcitxAddon *aa = (FcitxAddon*)a, *ab = (FcitxAddon*)b;
    return aa->priority - ab->priority;
}

FCITX_EXPORT_API
void FcitxAddonsInit(UT_array* addons)
{
    utarray_init(addons, &addon_icd);
}

void* FcitxGetSymbol(void* handle, const char* addonName, const char* symbolName)
{
    char *p;
    char *escapedAddonName;
    fcitx_utils_alloc_cat_str(escapedAddonName, addonName, "_", symbolName);
    for (p = escapedAddonName;*p;p++) {
        if (*p == '-') {
            *p = '_';
        }
    }
    void *result = dlsym(handle, escapedAddonName);
    free(escapedAddonName);
    if (!result)
        return dlsym(handle, symbolName);
    return result;
}

/**
 * Load Addon Info
 */
FCITX_EXPORT_API
void FcitxAddonsLoad(UT_array* addons)
{
    FcitxAddonsLoadInternal(addons, false);
}

FcitxAddon* FcitxAddonsLoadInternal(UT_array* addons, boolean reloadIM)
{
    char **addonPath;
    size_t len;
    size_t start;
    if (!reloadIM)
        utarray_clear(addons);

    start = utarray_len(addons);

    FcitxStringHashSet* sset = FcitxXDGGetFiles("addon", NULL, ".conf");
    addonPath = FcitxXDGGetPathWithPrefix(&len, "addon");
    char *paths[len];
    HASH_FOREACH(string, sset, FcitxStringHashSet) {
        int i;
        for (i = len - 1; i >= 0; i--) {
            fcitx_utils_alloc_cat_str(paths[i], addonPath[len - i - 1],
                                      "/", string->name);
            FcitxLog(DEBUG, "Load Addon Config File:%s", paths[i]);
        }
        FcitxConfigFile* cfile = FcitxConfigParseMultiConfigFile(paths, len, FcitxAddonGetConfigDesc());
        if (cfile) {
            utarray_extend_back(addons);
            FcitxAddon *a = (FcitxAddon*) utarray_back(addons);
            utarray_init(&a->functionList, fcitx_ptr_icd);
            FcitxAddonConfigBind(a, cfile, FcitxAddonGetConfigDesc());
            FcitxConfigBindSync((FcitxGenericConfig*)a);
            FcitxLog(DEBUG, _("Addon Config %s is %s"), string->name, (a->bEnabled) ? "Enabled" : "Disabled");
            boolean error = false;
            if (reloadIM) {
                if (a->category !=  AC_INPUTMETHOD)
                    error = true;
            }
            /* if loaded, don't touch the old one */
            if (FcitxAddonsGetAddonByNameInternal(addons, a->name, true) != a)
                error = true;

            if (error)
                utarray_pop_back(addons);
            else
                FcitxLog(INFO, _("Load Addon Config File:%s"), string->name);
        }

        for (i = len - 1;i >= 0;i--) {
            free(paths[i]);
        }
    }
    FcitxXDGFreePath(addonPath);

    fcitx_utils_free_string_hash_set(sset);

    size_t to = utarray_len(addons);
    utarray_sort_range(addons, AddonPriorityCmp, start, to);

    return (FcitxAddon*)utarray_eltptr(addons, start);
}

void FcitxInstanceFillAddonOwner(FcitxInstance* instance, FcitxAddon* addonHead)
{
    /* FIXME: a walkaround for not have instance in function FcitxModuleInvokeFunction */
    FcitxAddon* addon;
    if (addonHead)
        addon = addonHead;
    else
        addon = (FcitxAddon *) utarray_front(&instance->addons);
    for (; addon != NULL; addon = (FcitxAddon *) utarray_next(&instance->addons, addon)) {
        addon->owner = instance;
    }
}

FCITX_EXPORT_API
void FcitxInstanceResolveAddonDependency(FcitxInstance* instance)
{
    FcitxInstanceResolveAddonDependencyInternal(instance, NULL);
}

void FcitxInstanceResolveAddonDependencyInternal(FcitxInstance* instance, FcitxAddon* startAddon)
{
    UT_array* addons = &instance->addons;
    boolean remove = true;
    FcitxAddon *addon;
    FcitxAddon *uiaddon = NULL, *uifallbackaddon = NULL;
    boolean reloadIM = true;

    if (!startAddon) {
        startAddon = (FcitxAddon*) utarray_front(addons);
        reloadIM = false;
    }

    /* check "all" */
    if (instance->disableList
        && utarray_len(instance->disableList) == 1
        && fcitx_utils_string_list_contains(instance->disableList, "all"))
    {
        for (addon = startAddon;
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
            addon->bEnabled = false;
        }
    }

    /* override the enable and disable option */
    for (addon = startAddon;
         addon != NULL;
         addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if (instance->enableList && fcitx_utils_string_list_contains(instance->enableList, addon->name))
            addon->bEnabled = true;
        else if (instance->disableList && fcitx_utils_string_list_contains(instance->disableList, addon->name))
            addon->bEnabled = false;
    }

    if (!reloadIM) {
        /* choose ui */
        for (addon = startAddon;
             addon != NULL;
             addon = (FcitxAddon *) utarray_next(addons, addon)) {
            if (addon->category == AC_UI) {
                if (instance->uiname == NULL) {
                    if (addon->bEnabled) {
                        uiaddon = addon;
                        break;
                    }
                } else {
                    if (strcmp(instance->uiname, addon->name) == 0) {
                        addon->bEnabled = true;
                        uiaddon = addon;
                        break;
                    }
                }
            }
        }

        if (uiaddon && uiaddon->uifallback) {
            for (addon = startAddon;
                 addon != NULL;
                 addon = (FcitxAddon *) utarray_next(addons, addon)) {
                if (addon->category == AC_UI && addon->bEnabled && strcmp(uiaddon->uifallback, addon->name) == 0) {
                    FcitxAddon temp;
                    int uiidx = utarray_eltidx(addons, uiaddon);
                    int fallbackidx = utarray_eltidx(addons, addon);
                    if (fallbackidx < uiidx) {
                        temp = *uiaddon;
                        *uiaddon = *addon;
                        *addon = temp;

                        /* they swapped, addon is normal ui, and ui addon is fallback */
                        uifallbackaddon = uiaddon;
                        uiaddon = addon;
                    }
                    else {
                        uifallbackaddon = addon;
                    }
                    break;
                }
            }
        }

        for (addon = startAddon;
                addon != NULL;
                addon = (FcitxAddon *) utarray_next(addons, addon)) {
            if (addon->category == AC_UI && addon != uiaddon && addon != uifallbackaddon) {
                addon->bEnabled = false;
            }
        }
    }

    while (remove) {
        remove = false;
        for (addon = startAddon;
                addon != NULL;
                addon = (FcitxAddon *) utarray_next(addons, addon)) {
            if (!addon->bEnabled)
                continue;
            UT_array* dependlist = fcitx_utils_split_string(addon->depend, ',');
            boolean valid = true;
            char **depend = NULL;
            for (depend = (char **) utarray_front(dependlist);
                    depend != NULL;
                    depend = (char **) utarray_next(dependlist, depend)) {
                if (!FcitxAddonsIsAddonAvailable(addons, *depend)) {
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
boolean FcitxAddonsIsAddonAvailable(UT_array* addons, const char* name)
{
    return FcitxAddonsGetAddonByNameInternal(addons, name, false) != NULL;
}

FcitxAddon* FcitxAddonsGetAddonByNameInternal(UT_array* addons, const char* name, boolean checkDisabled)
{
    FcitxAddon *addon;
    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if ((checkDisabled || addon->bEnabled) && strcmp(name, addon->name) == 0)
            return addon;
    }
    return NULL;
}


FCITX_EXPORT_API
FcitxAddon* FcitxAddonsGetAddonByName(UT_array* addons, const char* name)
{
    return FcitxAddonsGetAddonByNameInternal(addons, name, false);
}

/**
 * Load addon.desc file
 *
 * @return the description of addon configure.
 */
FCITX_EXPORT_API
CONFIG_DESC_DEFINE(FcitxAddonGetConfigDesc, "addon.desc")

FCITX_EXPORT_API
void FcitxAddonFree(void* v)
{
    FcitxAddon *addon = (FcitxAddon*) v;
    if (!addon)
        return ;
    FcitxConfigFreeConfigFile(addon->config.configFile);
    free(addon->name);
    free(addon->library);
    free(addon->comment);
    free(addon->generalname);
    free(addon->depend);
    free(addon->subconfig);
}

boolean FcitxCheckABIVersion(void* handle, const char* addonName)
{
    int* version = (int*) FcitxGetSymbol(handle, addonName, "ABI_VERSION");
    if (!version)
        return false;
    if (*version < FCITX_ABI_VERSION)
        return false;
    return true;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
