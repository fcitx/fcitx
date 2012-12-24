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

#include "fcitx/hook.h"
#include "fcitx-utils/log.h"
#include "ime.h"
#include "fcitx-config/hotkey.h"
#include "instance.h"
#include "fcitx/hook-internal.h"
#include "fcitx-utils/utils.h"
#include "instance-internal.h"

/**
 * @file hook.c
 * A list for a stack of processing
 **/


/**
 * hook stack
 **/
typedef struct _HookStack {
    union {
        FcitxKeyFilterHook keyfilter;
        FcitxStringFilterHook stringfilter;
        FcitxIMEventHook eventhook;
        FcitxICEventHook ichook;
        FcitxHotkeyHook hotkey;
    };
    /**
     * stack next
     **/
    struct _HookStack* next;
} HookStack;

/**
 * internal macro to define a hook
 */
#define DEFINE_HOOK(name, type, field) \
    static HookStack* Get##name(FcitxInstance* instance); \
    HookStack* Get##name(FcitxInstance* instance) \
    { \
        if (instance->hook##name == NULL) \
        { \
            instance->hook##name = fcitx_utils_malloc0(sizeof(HookStack)); \
        } \
        return instance->hook##name; \
    } \
    FCITX_EXPORT_API \
    void FcitxInstanceRegister##name(FcitxInstance* instance, type value) \
    { \
        HookStack* head = Get##name(instance); \
        while(head->next != NULL) \
            head = head->next; \
        head->next = fcitx_utils_malloc0(sizeof(HookStack)); \
        head = head->next; \
        head->field = value; \
    }

DEFINE_HOOK(PreInputFilter, FcitxKeyFilterHook, keyfilter)
DEFINE_HOOK(PostInputFilter, FcitxKeyFilterHook, keyfilter)
DEFINE_HOOK(OutputFilter, FcitxStringFilterHook, stringfilter)
DEFINE_HOOK(CommitFilter, FcitxStringFilterHook, stringfilter)
DEFINE_HOOK(HotkeyFilter, FcitxHotkeyHook, hotkey)
DEFINE_HOOK(ResetInputHook, FcitxIMEventHook, eventhook);
DEFINE_HOOK(TriggerOnHook, FcitxIMEventHook, eventhook);
DEFINE_HOOK(TriggerOffHook, FcitxIMEventHook, eventhook);
DEFINE_HOOK(InputFocusHook, FcitxIMEventHook, eventhook);
DEFINE_HOOK(InputUnFocusHook, FcitxIMEventHook, eventhook);
DEFINE_HOOK(IMChangedHook, FcitxIMEventHook, eventhook);
DEFINE_HOOK(UpdateCandidateWordHook, FcitxIMEventHook, eventhook);
DEFINE_HOOK(UpdateIMListHook, FcitxIMEventHook, eventhook);
DEFINE_HOOK(ICStateChangedHook, FcitxICEventHook, ichook);

void FcitxInstanceProcessPreInputFilter(FcitxInstance* instance, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retval)
{
    HookStack* stack = GetPreInputFilter(instance);
    stack = stack->next;
    *retval = IRV_TO_PROCESS;
    while (stack) {
        if (stack->keyfilter.func(stack->keyfilter.arg, sym, state, retval))
            break;
        stack = stack->next;
    }
}

void FcitxInstanceProcessPostInputFilter(FcitxInstance* instance, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retval)
{
    HookStack* stack = GetPostInputFilter(instance);
    stack = stack->next;
    while (stack) {
        if (stack->keyfilter.func(stack->keyfilter.arg, sym, state, retval))
            break;
        stack = stack->next;
    }
}

void FcitxInstanceProcessUpdateCandidates(FcitxInstance* instance)
{
    HookStack* stack = GetUpdateCandidateWordHook(instance);
    stack = stack->next;
    while (stack) {
        stack->eventhook.func(stack->eventhook.arg);
        stack = stack->next;
    }
}

FCITX_EXPORT_API
char* FcitxInstanceProcessOutputFilter(FcitxInstance* instance, const char *in)
{
    HookStack* stack = GetOutputFilter(instance);
    stack = stack->next;
    char *out = NULL;
    char* newout = NULL;
    while (stack) {
        newout = stack->stringfilter.func(stack->stringfilter.arg, in);
        if (newout) {
            if (out) {
                free(out);
                out = NULL;
            }
            out = newout;
        }
        stack = stack->next;
    }
    return out;
}

FCITX_EXPORT_API
char* FcitxInstanceProcessCommitFilter(FcitxInstance* instance, const char *in)
{
    HookStack* stack = GetCommitFilter(instance);
    stack = stack->next;
    char *out = NULL;
    char* newout = NULL;
    while (stack) {
        newout = stack->stringfilter.func(stack->stringfilter.arg, in);
        if (newout) {
            if (out) {
                free(out);
                out = NULL;
            }
            out = newout;
            in = out;
        }
        stack = stack->next;
    }
    return out;
}

void FcitxInstanceProcessResetInputHook(FcitxInstance* instance)
{
    HookStack* stack = GetResetInputHook(instance);
    stack = stack->next;
    while (stack) {
        stack->eventhook.func(stack->eventhook.arg);
        stack = stack->next;
    }
}

void FcitxInstanceProcessTriggerOffHook(FcitxInstance* instance)
{
    HookStack* stack = GetTriggerOffHook(instance);
    stack = stack->next;
    while (stack) {
        stack->eventhook.func(stack->eventhook.arg);
        stack = stack->next;
    }
}
void FcitxInstanceProcessTriggerOnHook(FcitxInstance* instance)
{
    HookStack* stack = GetTriggerOnHook(instance);
    stack = stack->next;
    while (stack) {
        stack->eventhook.func(stack->eventhook.arg);
        stack = stack->next;
    }
}
void FcitxInstanceProcessInputFocusHook(FcitxInstance* instance)
{
    HookStack* stack = GetInputFocusHook(instance);
    stack = stack->next;
    while (stack) {
        stack->eventhook.func(stack->eventhook.arg);
        stack = stack->next;
    }
}
void FcitxInstanceProcessInputUnFocusHook(FcitxInstance* instance)
{
    HookStack* stack = GetInputUnFocusHook(instance);
    stack = stack->next;
    while (stack) {
        stack->eventhook.func(stack->eventhook.arg);
        stack = stack->next;
    }
}

void FcitxInstanceProcessIMChangedHook(FcitxInstance* instance)
{
    HookStack* stack = GetIMChangedHook(instance);
    stack = stack->next;
    while (stack) {
        stack->eventhook.func(stack->eventhook.arg);
        stack = stack->next;
    }
}

void FcitxInstanceProcessUpdateIMListHook(FcitxInstance* instance)
{
    HookStack* stack = GetUpdateIMListHook(instance);
    stack = stack->next;
    while (stack) {
        stack->eventhook.func(stack->eventhook.arg);
        stack = stack->next;
    }
}

void FcitxInstanceProcessICStateChangedHook(FcitxInstance* instance, FcitxInputContext* ic)
{
    HookStack* stack = GetICStateChangedHook(instance);
    stack = stack->next;
    while (stack) {
        stack->ichook.func(stack->ichook.arg, ic);
        stack = stack->next;
    }
}

INPUT_RETURN_VALUE FcitxInstanceProcessHotkey(FcitxInstance* instance, FcitxKeySym keysym, unsigned int state)
{
    HookStack* stack = GetHotkeyFilter(instance);
    stack = stack->next;
    INPUT_RETURN_VALUE out = IRV_TO_PROCESS;
    while (stack) {
        /* a little bit hack here, but safer */
        FcitxHotkey tempKey[2];
        FcitxHotkeyGetKey(
            stack->hotkey.hotkey[0].sym,
            stack->hotkey.hotkey[0].state,
            &tempKey[0].sym,
            &tempKey[0].state
        );
        FcitxHotkeyGetKey(
            stack->hotkey.hotkey[1].sym,
            stack->hotkey.hotkey[1].state,
            &tempKey[1].sym,
            &tempKey[1].state
        );
        if (FcitxHotkeyIsHotKey(keysym, state, tempKey)) {
            out = stack->hotkey.hotkeyhandle(stack->hotkey.arg);
            break;
        }
        stack = stack->next;
    }
    return out;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
