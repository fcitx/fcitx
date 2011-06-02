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

#include "instance.h"
#include <fcitx-utils/cutils.h>
#include "ime-internal.h"
#include "ui.h"
#include <libintl.h>
#include <pthread.h>
#include "addon.h"
#include "module.h"
#include "backend.h"

const UT_icd stat_icd = {sizeof(FcitxUIStatus), 0, 0, 0};
static void FcitxInitThread(FcitxInstance* inst);

FcitxInstance* CreateFcitxInstance()
{
    FcitxInstance* instance = fcitx_malloc0(sizeof(FcitxInstance));
    InitFcitxAddons(&instance->addons);
    InitFcitxIM(instance);
    InitFcitxBackends(&instance->backends);
    utarray_init(&instance->uistats, &stat_icd);
    instance->messageDown = InitMessages();
    instance->messageUp = InitMessages();
    
    FcitxInitThread(instance);
    LoadConfig(&instance->config);
    LoadProfile(&instance->profile);
    LoadAddonInfo(&instance->addons);
    AddonResolveDependency(&instance->addons);
    InitBuiltInHotkey(instance);
    LoadModule(instance);
    LoadAllIM(instance);
    LoadUserInterface(instance);
    
    SwitchIM(instance, instance->iIMIndex);
    
    StartBackend(instance);

    return instance;
}

Messages* GetMessageUp(FcitxInstance *instance)
{
    return instance->messageUp;
}

Messages* GetMessageDown(FcitxInstance *instance)
{
    return instance->messageDown;
}

void FcitxInitThread(FcitxInstance* inst)
{
    int rc;
    rc = pthread_mutex_init(&inst->fcitxMutex, NULL);
    if (rc != 0)
        FcitxLog(ERROR, _("pthread mutex init failed"));
}


int FcitxLock(FcitxInstance* inst)
{
    if (inst->bMutexInited)
        return pthread_mutex_lock(&inst->fcitxMutex);
    return 0;
}

int FcitxUnlock(FcitxInstance* inst)
{
    if (inst->bMutexInited)
        return pthread_mutex_unlock(&inst->fcitxMutex);
    return 0;
}
