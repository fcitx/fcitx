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

#include <dlfcn.h>
#include <libintl.h>
#include <pthread.h>

#include "fcitx/fcitx.h"
#include "module.h"
#include "addon.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "instance.h"

static UT_icd  module_icd = {sizeof(FcitxModule*), NULL, NULL, NULL};
typedef void*(*FcitxModuleFunction)(void *arg, FcitxModuleFunctionArg);

FCITX_EXPORT_API
void InitFcitxModules(UT_array* modules)
{
    utarray_init(modules, &module_icd);
}

FCITX_EXPORT_API
void LoadModule(FcitxInstance* instance)
{
    UT_array* addons = &instance->addons;
    FcitxAddon *addon;
    for ( addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon))
    {
        if (addon->bEnabled && addon->category == AC_MODULE)
        {
            char *modulePath;
            switch (addon->type)
            {
            case AT_SHAREDLIBRARY:
            {
                FILE *fp = GetLibFile(addon->library, "r", &modulePath);
                void *handle;
                FcitxModule* module;
                void* moduleinstance = NULL;
                if (!fp)
                    break;
                fclose(fp);
                handle = dlopen(modulePath,RTLD_LAZY | RTLD_GLOBAL);
                if (!handle)
                {
                    FcitxLog(ERROR, _("Module: open %s fail %s") ,modulePath ,dlerror());
                    break;
                }
                module=dlsym(handle,"module");
                if (!module || !module->Create)
                {
                    FcitxLog(ERROR, _("Module: bad module"));
                    dlclose(handle);
                    break;
                }
                if ((moduleinstance = module->Create(instance)) == NULL)
                {
                    dlclose(handle);
                    break;
                }
                addon->module = module;
                addon->addonInstance = moduleinstance;
                if (module->ProcessEvent && module->SetFD)
                    utarray_push_back(&instance->eventmodules, &addon);
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
void* InvokeModuleFunction(FcitxAddon* addon, int functionId, FcitxModuleFunctionArg args)
{
    if (addon == NULL)
    {
        FcitxLog(ERROR, "addon is not valid");
        return NULL;
    }
    FcitxModuleFunction* func =(FcitxModuleFunction*) utarray_eltptr(&addon->functionList, functionId);
    if (func == NULL)
    {
        FcitxLog(ERROR, "addon %s doesn't have function with id %d", addon->name, functionId);
        return NULL;
    }
    void* result = (*func)(addon->addonInstance, args);
    return result;
}

FCITX_EXPORT_API
void* InvokeModuleFunctionWithName(FcitxInstance* instance, const char* name, int functionId, FcitxModuleFunctionArg args)
{
    FcitxAddon* module = GetAddonByName(&instance->addons, name);
    if (module == NULL)
        return NULL;
    else
        return InvokeModuleFunction(module, functionId, args);
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
