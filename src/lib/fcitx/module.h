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

#ifndef _MODULE_H
#define _MODULE_H
#include "fcitx-config/fcitx-config.h"
#include "fcitx-utils/utarray.h"

struct FcitxInstance;
struct FcitxAddon;

typedef struct FcitxModule
{
    void* (*Create)(struct FcitxInstance* instance);
    void* (*Run)(void*);
    void (*Destroy)(void*);
    UT_array functionList;
} FcitxModule;

typedef struct FcitxModuleFunctionArg
{
    void* args[10];
} FcitxModuleFunctionArg;

void LoadModule(struct FcitxInstance* instance);
void* InvokeModuleFunction(struct FcitxAddon* addon, int functionId, FcitxModuleFunctionArg args);
void* InvokeModuleFunctionWithName(struct FcitxInstance* instance, const char* name, int functionId, FcitxModuleFunctionArg args);

#define InvokeFunction(INST, MODULE, FUNC, ARG)  \
    ((MODULE##_##FUNC##_RETURNTYPE) InvokeModuleFunctionWithName(INST, MODULE##_NAME, MODULE##_##FUNC, ARG))
    
#define AddFunction(Realname) \
    do { \
        void *temp = Realname; \
        utarray_push_back(&module.functionList, &temp); \
    } while(0)        

#endif