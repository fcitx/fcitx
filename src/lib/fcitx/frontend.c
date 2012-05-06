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

#include "fcitx-utils/utarray.h"
#include "frontend.h"
#include "addon.h"
#include "ime-internal.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "ui.h"
#include "hook.h"
#include "hook-internal.h"
#include "instance.h"
#include "instance-internal.h"
#include "addon-internal.h"

static const UT_icd frontend_icd = {sizeof(FcitxAddon*), NULL, NULL, NULL };
static void FcitxInstanceCleanUpIC(FcitxInstance* instance);

FCITX_EXPORT_API
void FcitxFrontendsInit(UT_array* frontends)
{
    utarray_init(frontends, &frontend_icd);
}

FCITX_EXPORT_API
FcitxInputContext* FcitxInstanceCreateIC(FcitxInstance* instance, int frontendid, void * priv)
{
    /* clean up invalid ic here */
    FcitxInstanceCleanUpIC(instance);
    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, frontendid);
    if (pfrontend == NULL)
        return NULL;
    FcitxFrontend* frontend = (*pfrontend)->frontend;

    FcitxInputContext *rec;
    if (instance->free_list != NULL) {
        rec = instance->free_list;
        instance->free_list = instance->free_list->next;
    } else
        rec = malloc(sizeof(FcitxInputContext));

    memset(rec, 0, sizeof(FcitxInputContext));
    rec->frontendid = frontendid;
    rec->offset_x = -1;
    rec->offset_y = -1;
    switch (instance->config->shareState) {
    case ShareState_All:
        rec->state = instance->globalState;
        break;
    case ShareState_None:
    case ShareState_PerProgram:
        rec->state = instance->config->defaultIMState;
        break;
    default:
        break;
    }

    frontend->CreateIC((*pfrontend)->addonInstance, rec, priv);

    rec->next = instance->ic_list;
    instance->ic_list = rec;
    return rec;
}

void FcitxInstanceCleanUpIC(FcitxInstance* instance)
{
    UT_array* frontends = &instance->frontends;
    FcitxInputContext *rec = instance->ic_list, *last = NULL, *todel;

    while (rec) {
        FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, rec->frontendid);
        FcitxFrontend* frontend = (*pfrontend)->frontend;
        pid_t pid = frontend->GetPid((*pfrontend)->addonInstance, rec);
        if (pid && !fcitx_utils_pid_exists(pid)) {
            if (last != NULL)
                last->next = rec->next;
            else
                instance->ic_list = rec->next;
            todel = rec;
            rec = rec->next;
            todel->next = instance->free_list;
            instance->free_list = todel;
            frontend->DestroyIC((*pfrontend)->addonInstance, todel);
            if (todel == instance->CurrentIC) {
                instance->CurrentIC = NULL;
                FcitxUICloseInputWindow(instance);
                FcitxUIOnInputUnFocus(instance);
                FcitxInstanceSetCurrentIC(instance, NULL);
            }
        }
        else {
            last = rec;
            rec = rec->next;
        }
    }
}

FCITX_EXPORT_API
FcitxInputContext* FcitxInstanceFindIC(FcitxInstance* instance, int frontendid, void *filter)
{
    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, frontendid);
    if (pfrontend == NULL)
        return NULL;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    FcitxInputContext *rec = instance->ic_list;
    while (rec != NULL) {
        if (rec->frontendid == frontendid && frontend->CheckIC((*pfrontend)->addonInstance, rec, filter))
            return rec;
        rec = rec->next;
    }
    return NULL;
}

FCITX_EXPORT_API
void FcitxInstanceSetICStateFromSameApplication(FcitxInstance* instance, int frontendid, FcitxInputContext *ic)
{
    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    if (!frontend->CheckICFromSameApplication)
        return;
    FcitxInputContext *rec = instance->ic_list;
    while (rec != NULL) {
        if (rec->frontendid == frontendid && frontend->CheckICFromSameApplication((*pfrontend)->addonInstance, rec, ic)) {
            ic->state = rec->state;
            break;
        }
        rec = rec->next;
    }
}

FCITX_EXPORT_API
void FcitxInstanceDestroyIC(FcitxInstance* instance, int frontendid, void* filter)
{
    FcitxInputContext             *rec, *last;
    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;

    last = NULL;

    for (rec = instance->ic_list; rec != NULL; last = rec, rec = rec->next) {
        if (rec->frontendid == frontendid && frontend->CheckIC((*pfrontend)->addonInstance, rec, filter)) {
            if (last != NULL)
                last->next = rec->next;
            else
                instance->ic_list = rec->next;

            rec->next = instance->free_list;
            instance->free_list = rec;

            if (rec == FcitxInstanceGetCurrentIC(instance)) {
                FcitxUICloseInputWindow(instance);
                FcitxUIOnInputUnFocus(instance);
                FcitxInstanceSetCurrentIC(instance, NULL);
            }

            frontend->DestroyIC((*pfrontend)->addonInstance, rec);
            return;
        }
    }

    return;
}

FCITX_EXPORT_API
FcitxContextState FcitxInstanceGetCurrentState(FcitxInstance* instance)
{
    if (instance->CurrentIC)
        return instance->CurrentIC->state;
    else
        return IS_CLOSED;
}

FCITX_EXPORT_API
FcitxContextState FcitxInstanceGetCurrentStatev2(FcitxInstance* instance)
{
    if (instance->CurrentIC) {
        if (instance->config->firstAsInactive && instance->CurrentIC->state == IS_INACTIVE)
            return IS_ACTIVE;
        return instance->CurrentIC->state;
    }
    else
        return IS_CLOSED;
}

FCITX_EXPORT_API
FcitxCapacityFlags FcitxInstanceGetCurrentCapacity(FcitxInstance* instance)
{
    if (instance->CurrentIC)
        return instance->CurrentIC->contextCaps;
    else
        return CAPACITY_NONE;
}

FCITX_EXPORT_API
void FcitxInstanceCommitString(FcitxInstance* instance, FcitxInputContext* ic, const char* str)
{
    if (str == NULL)
        return ;

    if (ic == NULL)
        return;

    UT_array* frontends = &instance->frontends;

    char *pstr = FcitxInstanceProcessCommitFilter(instance, str);
    if (pstr != NULL)
        str = pstr;

    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    frontend->CommitString((*pfrontend)->addonInstance, ic, str);

    FcitxInputState* input = instance->input;
    fcitx_utf8_strncpy(input->strLastCommit, str, MAX_USER_INPUT);
    input->strLastCommit[MAX_USER_INPUT] = '\0';
    instance->iHZInputed += (int)(fcitx_utf8_strlen(str));

    if (pstr)
        free(pstr);
}

FCITX_EXPORT_API
boolean FcitxInstanceGetSurroundingText(FcitxInstance* instance, FcitxInputContext* ic, char** str, unsigned int* cursor, unsigned int* anchor)
{
    if (ic == NULL)
        return false;

    if (!(ic->contextCaps & CAPACITY_SURROUNDING_TEXT))
        return false;

    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return false;

    FcitxFrontend* frontend = (*pfrontend)->frontend;
    if (frontend->GetSurroundingPreedit) {
        return frontend->GetSurroundingPreedit((*pfrontend)->addonInstance, ic, str, cursor, anchor);
    }
    return false;
}

FCITX_EXPORT_API
void FcitxInstanceDeleteSurroundingText(FcitxInstance* instance, FcitxInputContext* ic, int offset, unsigned int size)
{
    if (ic == NULL)
        return;

    UT_array* frontends = &instance->frontends;

    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    if (frontend->DeleteSurroundingText) {
        frontend->DeleteSurroundingText((*pfrontend)->addonInstance, ic, offset, size);
    }
}

FCITX_EXPORT_API
void FcitxInstanceUpdatePreedit(FcitxInstance* instance, FcitxInputContext* ic)
{
    if (!instance->profile->bUsePreedit)
        return;

    if (ic == NULL)
        return;

    if (!(ic->contextCaps & CAPACITY_PREEDIT))
        return;

    UT_array* frontends = &instance->frontends;

    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    frontend->UpdatePreedit((*pfrontend)->addonInstance, ic);
}

FCITX_EXPORT_API
void FcitxInstanceUpdateClientSideUI(FcitxInstance* instance, FcitxInputContext* ic)
{
    if (ic == NULL)
        return;

    if (!(ic->contextCaps & CAPACITY_CLIENT_SIDE_UI))
        return;

    UT_array* frontends = &instance->frontends;

    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    if (frontend->UpdateClientSideUI)
        frontend->UpdateClientSideUI((*pfrontend)->addonInstance, ic);
}

FCITX_EXPORT_API
void FcitxInstanceSetWindowOffset(FcitxInstance* instance, FcitxInputContext *ic, int x, int y)
{
    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    if (frontend->SetWindowOffset)
        frontend->SetWindowOffset((*pfrontend)->addonInstance, ic, x, y);
}

FCITX_EXPORT_API
void FcitxInstanceGetWindowPosition(FcitxInstance* instance, FcitxInputContext* ic, int* x, int* y)
{
    if (ic == NULL)
        return;

    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    int rx, ry, rw, rh;
    if (frontend->GetWindowRect) {
        frontend->GetWindowRect((*pfrontend)->addonInstance, ic, &rx, &ry, &rw, &rh);
        *x = rx;
        *y = ry + rh;
    }
}

FCITX_EXPORT_API
void FcitxInstanceGetWindowRect(FcitxInstance* instance, FcitxInputContext* ic, int* x, int* y, int* w, int* h)
{
    if (ic == NULL)
        return;

    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    if (frontend->GetWindowRect) {
        frontend->GetWindowRect((*pfrontend)->addonInstance, ic, x, y, w, h);
    }
}

FCITX_EXPORT_API
boolean FcitxInstanceLoadFrontend(FcitxInstance* instance)
{
    UT_array* addons = &instance->addons;
    UT_array* frontends = &instance->frontends;
    FcitxAddon *addon;
    int frontendindex = 0;
    utarray_clear(frontends);
    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if (addon->bEnabled && addon->category == AC_FRONTEND) {
            char *modulePath;
            switch (addon->type) {
            case AT_SHAREDLIBRARY: {
                FILE *fp = FcitxXDGGetLibFile(addon->library, "r", &modulePath);
                void *handle;
                FcitxFrontend* frontend;
                if (!fp)
                    break;
                fclose(fp);
                handle = dlopen(modulePath, RTLD_LAZY | RTLD_GLOBAL);
                if (!handle) {
                    FcitxLog(ERROR, _("Frontend: open %s fail %s") , modulePath , dlerror());
                    break;
                }

                if (!CheckABIVersion(handle)) {
                    FcitxLog(ERROR, "%s ABI Version Error", addon->name);
                    dlclose(handle);
                    break;
                }

                frontend = dlsym(handle, "frontend");
                if (!frontend || !frontend->Create) {
                    FcitxLog(ERROR, _("Frontend: bad frontend"));
                    dlclose(handle);
                    break;
                }
                if ((addon->addonInstance = frontend->Create(instance, frontendindex)) == NULL) {
                    dlclose(handle);
                    break;
                }
                addon->frontend = frontend;
                frontendindex ++;
                utarray_push_back(frontends, &addon);
            }
            break;
            default:
                break;
            }
            free(modulePath);
        }
    }

    if (utarray_len(&instance->frontends) <= 0) {
        FcitxLog(ERROR, _("No available frontend"));
        return false;
    }
    return true;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
