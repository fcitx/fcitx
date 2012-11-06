/***************************************************************************
 *   Copyright (C) 2010~2012 by CSSlayer                                   *
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

#include "fcitx/instance.h"
#include "fcitx/instance-internal.h"
#include "fcitx-config/hotkey.h"
#include "context.h"

typedef struct _FcitxContextCallbackInfo {
    void *arg;
    FcitxContextCallback callback;
} FcitxContextCallbackInfo;

struct _FcitxContext {
    char* name;
    FcitxContextType type;
    unsigned int flag;
    union {
        FcitxHotkey hotkey[2];
        char* str;
        boolean b;
    };

    UT_array* callback;

    UT_hash_handle hh;
};

static const UT_icd ci_icd = {
    sizeof(FcitxContextCallbackInfo), NULL, NULL, NULL
};
static void FcitxInstanceSetContextInternal(FcitxInstance* instance, FcitxContext* context, const void* value);

FCITX_EXPORT_API
void FcitxInstanceRegisterWatchableContext(FcitxInstance* instance, const char* key, FcitxContextType type, unsigned int flag)
{
    FcitxContext* context = fcitx_utils_malloc0(sizeof(FcitxContext));
    context->name = strdup(key);
    context->flag = flag;
    context->type = type;
    utarray_new(context->callback, &ci_icd);
    HASH_ADD_KEYPTR(hh, instance->context, context->name, strlen(context->name), context);
}

FCITX_EXPORT_API
void FcitxInstanceSetContext(FcitxInstance* instance, const char* key, const void* value)
{
    FcitxContext* context;
    HASH_FIND_STR(instance->context, key, context);
    if (context == NULL)
        return;

    FcitxInstanceSetContextInternal(instance, context, value);
}

void FcitxInstanceSetContextInternal(FcitxInstance* instance, FcitxContext* context, const void* value)
{
    FCITX_UNUSED(instance);
    boolean changed = false;
    void* newvalue = NULL;

    switch(context->type) {
        case FCT_String: {
            const char* s = (const char*) value;
            char* old = context->str;
            if (s)
                context->str = strdup(s);
            else
                context->str = NULL;

            if ((old == NULL && context->str != NULL)
                || (old != NULL && context->str == NULL)
                || (old && context->str && strcmp(old, context->str) != 0))
                changed = true;

            if (old)
                free(old);

            newvalue = context->str;

            break;
        }
        case FCT_Hotkey: {
            if (value) {
                FcitxHotkey* hotkey = (FcitxHotkey*) value;
                context->hotkey[0].sym = hotkey[0].sym;
                context->hotkey[0].state = hotkey[0].state;
                context->hotkey[1].sym = hotkey[1].sym;
                context->hotkey[1].state = hotkey[1].state;
            }
            else {
                context->hotkey[0].sym = FcitxKey_None;
                context->hotkey[0].state = FcitxKeyState_None;
                context->hotkey[1].sym = FcitxKey_None;
                context->hotkey[1].state = FcitxKeyState_None;
            }

            newvalue = context->hotkey;
            changed = true;
            break;
        }
        case FCT_Boolean: {
            boolean* pb = (boolean*) value;
            boolean b = false;
            if (pb == NULL)
                b = false;
            else
                b = *pb;
            if (b != context->b) {
                context->b = b;
                changed = true;
            }
            newvalue = &context->b;
            break;
        }
        case FCT_Void: {
            newvalue = NULL;
            changed = true;
            break;
        }
    }

    if (changed) {
        FcitxContextCallbackInfo* info;
        for (info = (FcitxContextCallbackInfo*) utarray_front(context->callback);
             info != NULL;
             info = (FcitxContextCallbackInfo*) utarray_next(context->callback, info))
        {
            info->callback(info->arg, newvalue);
        }
    }
}

FCITX_EXPORT_API
void FcitxInstanceWatchContext(FcitxInstance* instance, const char* key, FcitxContextCallback callback, void* arg)
{
    FcitxContext* context;
    HASH_FIND_STR(instance->context, key, context);
    if (context == NULL)
        return;

    FcitxContextCallbackInfo info;
    info.callback = callback;
    info.arg = arg;

    utarray_push_back(context->callback, &info);
}

FCITX_EXPORT_API
const FcitxHotkey* FcitxInstanceGetContextHotkey(FcitxInstance* instance, const char* key)
{
    FcitxContext* context;
    HASH_FIND_STR(instance->context, key, context);
    if (context == NULL)
        return NULL;

    if (context->hotkey[0].sym == FcitxKey_None
        && context->hotkey[0].state == FcitxKeyState_None
        && context->hotkey[1].sym == FcitxKey_None
        && context->hotkey[1].state == FcitxKeyState_None)
        return NULL;

    return context->hotkey;
}

FCITX_EXPORT_API
const char* FcitxInstanceGetContextString(FcitxInstance* instance, const char* key)
{
    FcitxContext* context;
    HASH_FIND_STR(instance->context, key, context);
    if (context == NULL)
        return NULL;

    return context->str;
}

FCITX_EXPORT_API
boolean FcitxInstanceGetContextBoolean(FcitxInstance* instance, const char* key)
{
    FcitxContext* context;
    HASH_FIND_STR(instance->context, key, context);
    if (context == NULL)
        return false;

    return context->b;
}

void FcitxInstanceResetContext(FcitxInstance* instance, FcitxContextFlag flag)
{
    FcitxContext *context = instance->context;
    while (context) {
        if (context->flag & flag) {
            FcitxInstanceSetContextInternal(instance, context, NULL);
        }
        context = context->hh.next;
    }
}
