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

#ifndef _FCITX_MODULE_H
#define _FCITX_MODULE_H
#include <fcitx-config/fcitx-config.h>
#include <fcitx-utils/utarray.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _FcitxInstance;
struct _FcitxAddon;

typedef struct _FcitxModule
{
    void* (*Create)(struct _FcitxInstance* instance);
    void (*SetFD)(void*);
    void (*ProcessEvent)(void*);
    void (*Destroy)(void*);
    void (*ReloadConfig)(void*);
} FcitxModule;

typedef struct _FcitxModuleFunctionArg
{
    void* args[10];
} FcitxModuleFunctionArg;

void InitFcitxModules(UT_array* modules);
void LoadModule(struct _FcitxInstance* instance);
void* InvokeModuleFunction(struct _FcitxAddon* addon, int functionId, FcitxModuleFunctionArg args);
void* InvokeModuleFunctionWithName(struct _FcitxInstance* instance, const char* name, int functionId, FcitxModuleFunctionArg args);

#define InvokeFunction(INST, MODULE, FUNC, ARG)  \
    ((MODULE##_##FUNC##_RETURNTYPE) InvokeModuleFunctionWithName(INST, MODULE##_NAME, MODULE##_##FUNC, ARG))
    
#define AddFunction(ADDON, Realname) \
    do { \
        void *temp = Realname; \
        utarray_push_back(&ADDON->functionList, &temp); \
    } while(0)
    
#ifdef __cplusplus
}
#endif

#endif