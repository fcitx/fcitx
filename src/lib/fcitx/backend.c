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

#include "fcitx-utils/utarray.h"
#include "backend.h"
#include "addon.h"
#include "ime-internal.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/cutils.h"
#include "ui.h"
#include "hook.h"
#include "hook-internal.h"
#include "instance.h"

static const UT_icd backend_icd = {sizeof(FcitxAddon*), NULL, NULL, NULL };

FcitxInputContext* GetCurrentIC(FcitxInstance* instance)
{
    return instance->CurrentIC;
}

void SetCurrentIC(FcitxInstance* instance, FcitxInputContext* ic)
{
    instance->CurrentIC = ic;
}

void InitFcitxBackends(UT_array* backends)
{
    utarray_init(backends, &backend_icd);
}

FcitxInputContext* CreateIC(FcitxInstance* instance, int backendid, void * priv)
{
    UT_array* backends = &instance->backends;
    FcitxAddon** pbackend =(FcitxAddon**) utarray_eltptr(backends, backendid);
    if (pbackend == NULL)
        return NULL;
    FcitxBackend* backend = (*pbackend)->backend;
    
    FcitxInputContext *rec;
    if (instance->free_list != NULL)
    {
        rec = instance->free_list;
        instance->free_list = instance->free_list->next;
    }
    else
        rec = malloc(sizeof(FcitxInputContext));
    
    memset (rec, 0, sizeof(FcitxInputContext));
    rec->backendid = backendid;
    rec->offset_x = -1;
    rec->offset_y = -1;
    rec->state = IS_CLOSED;
    
    backend->CreateIC((*pbackend)->addonInstance, rec, priv);
    
    rec->next = instance->ic_list;
    instance->ic_list = rec;
    
    return rec;
}

FcitxInputContext* FindIC(FcitxInstance* instance, int backendid, void *filter)
{
    UT_array* backends = &instance->backends;
    FcitxAddon** pbackend = (FcitxAddon**) utarray_eltptr(backends, backendid);
    if (pbackend == NULL)
        return NULL;
    FcitxBackend* backend = (*pbackend)->backend;
    FcitxInputContext *rec = instance->ic_list;
    while (rec != NULL)
    {
        if (rec->backendid == backendid && backend->CheckIC((*pbackend)->addonInstance, rec, filter))
            return rec;
        rec = rec->next;
    }
    return NULL;
}

void DestroyIC(FcitxInstance* instance, int backendid, void* filter)
{
    FcitxInputContext             *rec, *last;
    UT_array* backends = &instance->backends;
    FcitxAddon** pbackend = (FcitxAddon**) utarray_eltptr(backends, backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = (*pbackend)->backend;
 
    last = NULL;

    for (rec = instance->ic_list; rec != NULL; last = rec, rec = rec->next) {
        if (rec->backendid == backendid && backend->CheckIC((*pbackend)->addonInstance, rec, filter)) {
            if (last != NULL)
                last->next = rec->next;
            else
                instance->ic_list = rec->next;
            
            rec->next = instance->free_list;
            instance->free_list = rec;
            
            backend->DestroyIC((*pbackend)->addonInstance, rec);
            return;
        }
    }

    return;
}

void CloseIM(FcitxInstance* instance, FcitxInputContext* ic)
{
    UT_array* backends = &instance->backends;
    FcitxAddon** pbackend = (FcitxAddon**) utarray_eltptr(backends, ic->backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = (*pbackend)->backend;
    ic->state = IS_CLOSED;
    backend->CloseIM((*pbackend)->addonInstance, ic);
    CloseInputWindow(instance);
}

/** 
 * @brief 更改输入法状态
 * 
 * @param _connect_id
 */
void ChangeIMState(FcitxInstance* instance, FcitxInputContext* ic)
{
    if (!ic)
        return;
    if (ic->state == IS_ENG) {
        ic->state = IS_ACTIVE;
    } else {
        ic->state = IS_ENG;
        ResetInput(instance);
    }
}

IME_STATE GetCurrentState(FcitxInstance* instance)
{
    if (instance->CurrentIC)
        return instance->CurrentIC->state;
    else
        return IS_CLOSED;
}

void CommitString(FcitxInstance* instance, FcitxInputContext* ic, char* str)
{
    if (str == NULL)
        return ;
    
    UT_array* backends = &instance->backends;
    
    char *pstr = ProcessOutputFilter(str);
    if (pstr != NULL)
        str = pstr;
    
    FcitxAddon** pbackend = (FcitxAddon**) utarray_eltptr(backends, ic->backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = (*pbackend)->backend;
    backend->CommitString((*pbackend)->addonInstance, ic, str);
    
    if (pstr)
        free(pstr);
}

void SetWindowOffset(FcitxInstance* instance, FcitxInputContext *ic, int x, int y)
{
    UT_array* backends = &instance->backends;
    FcitxAddon** pbackend = (FcitxAddon**) utarray_eltptr(backends, ic->backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = (*pbackend)->backend;
    if (backend->SetWindowOffset)
        backend->SetWindowOffset((*pbackend)->addonInstance, ic, x, y);
}

void GetWindowPosition(FcitxInstance* instance, FcitxInputContext* ic, int* x, int* y)
{
    UT_array* backends = &instance->backends;
    FcitxAddon** pbackend = (FcitxAddon**) utarray_eltptr(backends, ic->backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = (*pbackend)->backend;
    if (backend->GetWindowPosition)
        backend->GetWindowPosition((*pbackend)->addonInstance, ic, x, y);
}


void StartBackend(FcitxInstance* instance)
{
    UT_array* addons = &instance->addons;
    UT_array* backends = &instance->backends;
    FcitxAddon *addon;
    int backendindex = 0;
    utarray_clear(backends);
    for ( addon = (FcitxAddon *) utarray_front(addons);
          addon != NULL;
          addon = (FcitxAddon *) utarray_next(addons, addon))
    {
        if (addon->bEnabled && addon->category == AC_BACKEND)
        {
            char *modulePath;
            switch (addon->type)
            {
                case AT_SHAREDLIBRARY:
                    {
                        FILE *fp = GetLibFile(addon->library, "r", &modulePath);
                        void *handle;
                        FcitxBackend* backend;
                        if (!fp)
                            break;
                        fclose(fp);
                        handle = dlopen(modulePath,RTLD_LAZY);
                        if(!handle)
                        {
                            FcitxLog(ERROR, _("Backend: open %s fail %s") ,modulePath ,dlerror());
                            break;
                        }
                        backend=dlsym(handle,"backend");
                        if(!backend || !backend->Create)
                        {
                            FcitxLog(ERROR, _("Backend: bad backend"));
                            dlclose(handle);
                            break;
                        }
                        if((addon->addonInstance = backend->Create(instance, backendindex)) == NULL)
                        {
                            dlclose(handle);
                            return;
                        }                        
                        addon->backend = backend;
                        backendindex ++;
                        utarray_push_back(backends, &addon);
                        if (backend->Run)
                        {
                            pthread_create(&addon->pid, NULL, backend->Run, NULL);
                        }
                    }
                    break;
                default:
                    break;
            }
            free(modulePath);
        }
    }
}