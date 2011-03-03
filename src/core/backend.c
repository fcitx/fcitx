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

#include "core/fcitx.h"
#include "utils/utils.h"
#include "core/addon.h"
#include "core/backend.h"
#include "fcitx-config/xdg.h"
#include "fcitx-config/cutils.h"
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>

UT_array* backends;
UT_icd backend_icd = {sizeof(FcitxBackend*), NULL, NULL, NULL };
FcitxInputContext *ic_list = NULL;
FcitxInputContext *free_list = NULL;
FcitxInputContext *CurrentIC = NULL;

FcitxInputContext* CreateIC(int backendid, void * priv)
{
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

FcitxInputContext* DestroyIC(int backendid, void* filter)
{
    FcitxInputContext             *rec, *last;
    FcitxBackend** pbackend = (FcitxBackend**) utarray_eltptr(backends, backendid);
    if (pbackend == NULL)
        return NULL;
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

void StartBackend()
{
    FcitxAddon *addon;
    int backendindex = 0;
    utarray_new(backends, &backend_icd);
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
                            FcitxLog(ERROR, _("Backend: bad im"));
                            dlclose(handle);
                            break;
                        }
                        if(!backend->Init())
                        {
                            dlclose(handle);
                            return;
                        }
                        backend->backendid = backendindex;
                        backendindex ++;
                        utarray_push_back(backends, &backend);
                        //backend->Run();
                        pthread_create(&backend->pid, NULL, backend->Run, NULL);
                        pthread_join(backend->pid, NULL);
                        //backend->Destroy();
                        //pthread_create(&backend->pid, NULL, backend->Run, NULL);
                    }
                    break;
                default:
                    break;
            }
            free(modulePath);
        }
    }
}