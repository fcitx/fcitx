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

#include "utils/utarray.h"
#include "backend.h"
#include "addon.h"
#include "ime-internal.h"
#include "fcitx-config/xdg.h"
#include "fcitx-config/cutils.h"

static FcitxInputContext *ic_list = NULL;
static FcitxInputContext *free_list = NULL;
static FcitxInputContext *CurrentIC = NULL;
static const UT_icd backend_icd = {sizeof(FcitxBackend*), NULL, NULL, NULL };

FcitxInputContext* GetCurrentIC()
{
    return CurrentIC;
}

void SetCurrentIC(FcitxInputContext* ic)
{
    CurrentIC = ic;
}

UT_array* GetFcitxBackends()
{
    static UT_array* backends = NULL;
    if (backends == NULL)
        utarray_new(backends, &backend_icd);
    return backends;
}

FcitxInputContext* CreateIC(int backendid, void * priv)
{
    UT_array* backends = GetFcitxBackends();
    FcitxBackend** pbackend =(FcitxBackend**) utarray_eltptr(backends, backendid);
    if (pbackend == NULL)
        return NULL;
    FcitxBackend* backend = *pbackend;
    
    FcitxInputContext *rec;
    if (free_list != NULL)
    {
        rec = free_list;
        free_list = free_list->next;
    }
    else
        rec = malloc(sizeof(FcitxInputContext));
    
    memset (rec, 0, sizeof(FcitxInputContext));
    rec->backendid = backendid;
    rec->offset_x = -1;
    rec->offset_y = -1;
    rec->state = IS_CHN;
    
    backend->CreateIC(rec, priv);
    
    rec->next = ic_list;
    ic_list = rec;
    
    return rec;
}

FcitxInputContext* FindIC(int backendid, void *filter)
{
    UT_array* backends = GetFcitxBackends();
    FcitxBackend** pbackend = (FcitxBackend**) utarray_eltptr(backends, backendid);
    if (pbackend == NULL)
        return NULL;
    FcitxBackend* backend = *pbackend;
    FcitxInputContext *rec = ic_list;
    while (rec != NULL)
    {
        if (rec->backendid == backendid && backend->CheckIC(rec, filter))
            return rec;
        rec = rec->next;
    }
    return NULL;
}

void DestroyIC(int backendid, void* filter)
{
    FcitxInputContext             *rec, *last;
    UT_array* backends = GetFcitxBackends();
    FcitxBackend** pbackend = (FcitxBackend**) utarray_eltptr(backends, backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = *pbackend;
 
    last = NULL;

    for (rec = ic_list; rec != NULL; last = rec, rec = rec->next) {
        if (rec->backendid == backendid && backend->CheckIC(rec, filter)) {
            if (last != NULL)
                last->next = rec->next;
            else
                ic_list = rec->next;
            
            rec->next = free_list;
            free_list = rec;
            
            backend->DestroyIC(rec);
            return;
        }
    }

    return;
}

void CloseIM(FcitxInputContext* ic)
{
    UT_array* backends = GetFcitxBackends();
    FcitxBackend** pbackend = (FcitxBackend**) utarray_eltptr(backends, ic->backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = *pbackend;
    backend->CloseIM(ic);
}

/** 
 * @brief 更改输入法状态
 * 
 * @param _connect_id
 */
void ChangeIMState(FcitxInputContext* ic)
{
    if (!ic)
        return;
    if (ic->state == IS_ENG) {
        ic->state = IS_CHN;
    } else {
        ic->state = IS_ENG;
        ResetInput();
    }
}

IME_STATE GetCurrentState()
{
    return CurrentIC->state;
}

void CommitString(FcitxInputContext* ic, char* str)
{
    UT_array* backends = GetFcitxBackends();
    FcitxBackend** pbackend = (FcitxBackend**) utarray_eltptr(backends, ic->backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = *pbackend;
    backend->CommitString(ic, str);
}

void SetWindowOffset(FcitxInputContext *ic, int x, int y)
{
    UT_array* backends = GetFcitxBackends();
    FcitxBackend** pbackend = (FcitxBackend**) utarray_eltptr(backends, ic->backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = *pbackend;
    if (backend->SetWindowOffset)
        backend->SetWindowOffset(ic, x, y);
}

void GetWindowPosition(FcitxInputContext* ic, int* x, int* y)
{
    UT_array* backends = GetFcitxBackends();
    FcitxBackend** pbackend = (FcitxBackend**) utarray_eltptr(backends, ic->backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = *pbackend;
    if (backend->GetWindowPosition)
        backend->GetWindowPosition(ic, x, y);
}


void StartBackend()
{
    UT_array* addons = GetFcitxAddons();
    UT_array* backends = GetFcitxBackends();
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
                        if(!backend || !backend->Init)
                        {
                            FcitxLog(ERROR, _("Backend: bad backend"));
                            dlclose(handle);
                            break;
                        }
                        if(!backend->Init())
                        {
                            dlclose(handle);
                            return;
                        }
                        addon->backend = backend;
                        backend->backendid = backendindex;
                        backendindex ++;
                        utarray_push_back(backends, &backend);
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