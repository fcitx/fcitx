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

#include "core/fcitx.h"
#include "module.h"
#include "addon.h"
#include "fcitx-config/xdg.h"
#include "fcitx-config/cutils.h"

void LoadModule()
{
    UT_array* addons = GetFcitxAddons();
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
                        if (!fp)
                            break;
                        fclose(fp);
                        handle = dlopen(modulePath,RTLD_LAZY);
                        if(!handle)
                        {
                            FcitxLog(ERROR, _("Module: open %s fail %s") ,modulePath ,dlerror());
                            break;
                        }
                        module=dlsym(handle,"module");
                        if(!module || !module->Init)
                        {
                            FcitxLog(ERROR, _("Module: bad module"));
                            dlclose(handle);
                            break;
                        }
                        if(!module->Init())
                        {
                            dlclose(handle);
                            return;
                        }
                        addon->module = module;
                    }
                default:
                    break;
            }
            free(modulePath);
        }
    }
}