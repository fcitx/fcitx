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

#include <dlfcn.h>
#include <libintl.h>
#include <pthread.h>

#include "fcitx/fcitx.h"
#include "module.h"
#include "addon.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "instance.h"
#include "instance-internal.h"
#include "addon-internal.h"
#include "ime-internal.h"

void InitFcitxModules(UT_array* modules)
{
    utarray_init(modules, fcitx_ptr_icd);
}

FCITX_EXPORT_API
void FcitxModuleLoad(FcitxInstance* instance)
{
    UT_array* addons = &instance->addons;
    FcitxAddon *addon;
    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if (addon->bEnabled && addon->category == AC_MODULE) {
            char *modulePath;
            switch (addon->type) {
            case AT_SHAREDLIBRARY: {
                FILE *fp = FcitxXDGGetLibFile(addon->library, "r", &modulePath);
                void *handle;
                FcitxModule* module;
                void* moduleinstance = NULL;
                if (!fp)
                    break;
                fclose(fp);
                handle = dlopen(modulePath, RTLD_NOW | (addon->loadLocal ? RTLD_LOCAL : RTLD_GLOBAL));
                if (!handle) {
                    FcitxLog(ERROR, _("Module: open %s fail %s") , modulePath , dlerror());
                    break;
                }

                if (!FcitxCheckABIVersion(handle, addon->name)) {
                    FcitxLog(ERROR, "%s ABI Version Error", addon->name);
                    dlclose(handle);
                    break;
                }

                module = FcitxGetSymbol(handle, addon->name, "module");
                if (!module || !module->Create) {
                    FcitxLog(ERROR, _("Module: bad module"));
                    dlclose(handle);
                    break;
                }
                if ((moduleinstance = module->Create(instance)) == NULL) {
                    dlclose(handle);
                    break;
                }
                if (instance->loadingFatalError)
                    return;
                addon->module = module;
                addon->addonInstance = moduleinstance;
                if (module->ProcessEvent && module->SetFD)
                    utarray_push_back(&instance->eventmodules, &addon);
                utarray_push_back(&instance->modules, &addon);
            }
            break;
            default:
                break;
            }
            free(modulePath);
        }
    }
}

FCITX_EXPORT_API
FcitxModuleFunction
FcitxModuleFindFunction(FcitxAddon *addon, int func_id)
{
    if (!addon) {
        FcitxLog(DEBUG, "addon is not valid");
        return NULL;
    }

    /*
     * Input Methods support lazy load
     */
    if (addon->category == AC_INPUTMETHOD) {
        boolean flag = false;
        FcitxAddon **pimclass = NULL;
        for (pimclass = (FcitxAddon**)utarray_front(&addon->owner->imeclasses);
             pimclass;pimclass = (FcitxAddon**)utarray_next(
                 &addon->owner->imeclasses, pimclass)) {
            if (*pimclass == addon) {
                flag = true;
                break;
            }
        }
        if (!flag && !addon->addonInstance) {
            FcitxInstanceLoadIM(addon->owner, addon);
            FcitxInstanceUpdateIMList(addon->owner);
        }
    }
    FcitxModuleFunction *func_p = fcitx_array_eltptr(&addon->functionList,
                                                     func_id);
    if (func_p)
        return *func_p;
    return NULL;
}

FCITX_EXPORT_API void*
FcitxModuleInvokeOnAddon(FcitxAddon *addon, FcitxModuleFunction func,
                         FcitxModuleFunctionArg *args)
{
    if (!func)
        return NULL;
    return func(addon->addonInstance, *args);
}

static void*
_FcitxModuleInvokeFunction(FcitxAddon* addon, int functionId,
                           FcitxModuleFunctionArg args)
{
    FcitxModuleFunction func = FcitxModuleFindFunction(addon, functionId);
    if (!func) {
        FcitxLog(DEBUG, "addon %s doesn't have function with id %d",
                 addon->name, functionId);
        return NULL;
    }
    return func(addon->addonInstance, args);
}

FCITX_EXPORT_API void*
FcitxModuleInvokeFunction(FcitxAddon* addon, int functionId,
                          FcitxModuleFunctionArg args)
{
    return _FcitxModuleInvokeFunction(addon, functionId, args);
}

FCITX_EXPORT_API
void* FcitxModuleInvokeFunctionByName(FcitxInstance* instance, const char* name, int functionId, FcitxModuleFunctionArg args)
{
    FcitxAddon* module = FcitxAddonsGetAddonByName(&instance->addons, name);

    if (module == NULL) {
        return NULL;
    } else {
        return _FcitxModuleInvokeFunction(module, functionId, args);
    }
}

FCITX_EXPORT_API void
FcitxModuleAddFunction(FcitxAddon *addon, FcitxModuleFunction func)
{
    void *temp = (void*)func;
    utarray_push_back(&addon->functionList, &temp);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
