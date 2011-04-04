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

#include "core/hook.h"
#include "fcitx-config/cutils.h"
#include "ime.h"
#include "fcitx-config/hotkey.h"

typedef struct HookStack {
    union {
        void *func;
        boolean (*keyfilter)(long unsigned int sym,
                             unsigned int state,
                             INPUT_RETURN_VALUE *retval
                            );
        char* (*stringfilter)(const char* in);
        HotkeyHook hotkey;
    };
    struct HookStack* next;
} HookStack;


#define DEFINE_HOOK(name, type, field) \
static HookStack* Get##name(); \
HookStack* Get##name() \
{ \
    static HookStack* name = NULL; \
    if (name == NULL) \
    { \
        name = malloc0(sizeof(HookStack)); \
    } \
    return name; \
} \
void Register##name(type value) \
{ \
    HookStack* head = Get##name(); \
    while(head->next != NULL) \
        head = head->next; \
    head->next = malloc0(sizeof(HookStack)); \
    head = head->next; \
    head->field = value; \
}

DEFINE_HOOK(PreInputFilter, void*, func)
DEFINE_HOOK(PostInputFilter, void*, func)
DEFINE_HOOK(OutputFilter, void*, func)
DEFINE_HOOK(HotkeyFilter, HotkeyHook, hotkey)

void ProcessPreInputFilter(FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retval)
{
    HookStack* stack = GetPreInputFilter();
    stack = stack->next;
    *retval = IRV_TO_PROCESS;
    while(stack)
    {
        if (stack->keyfilter(sym, state, retval))
            break;
        stack = stack->next;
    }
}

void ProcessPostInputFilter(FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retval)
{
    HookStack* stack = GetPostInputFilter();
    stack = stack->next;
    *retval = IRV_TO_PROCESS;
    while(stack)
    {
        if (stack->keyfilter(sym, state, retval))
            break;
        stack = stack->next;
    }
}

char* ProcessOutputFilter(char *in)
{
    HookStack* stack = GetOutputFilter();
    stack = stack->next;
    char *out = NULL;
    while(stack)
    {
        if ((out = stack->stringfilter(in)) != NULL)
            break;
        stack = stack->next;
    }
    return out;
}

INPUT_RETURN_VALUE CheckHotkey(FcitxKeySym keysym, unsigned int state)
{
    HookStack* stack = GetHotkeyFilter();
    stack = stack->next;
    INPUT_RETURN_VALUE out = IRV_TO_PROCESS;
    while(stack)
    {
        if (IsHotKey(keysym, state, stack->hotkey.hotkey))
        {
            out = stack->hotkey.hotkeyhandle();
            break;
        }
        stack = stack->next;
    }
    return out;
}