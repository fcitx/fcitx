/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#include <semaphore.h>

#include "fcitx/addon.h"
#include "fcitx/module.h"
#include "fcitx/fcitx.h"
#include "fcitx/instance.h"
#include <fcntl.h>

#include "simple.h"
#include "fcitx-simple-frontend.h"

/*
 * Self pipe socket package format
 * | 4-byte type | key | state | keycode
 *
 */

typedef struct _FcitxSimpleModule {
    int selfPipe[2];
    sem_t sem;
    FcitxInstance* owner;
} FcitxSimpleModule;

void* SimpleModuleCreate(FcitxInstance* instance);
void SimpleModuleSetFD(void* arg);
void SimpleModuleProcessEvent(void* arg);
void SimpleModuleDestroy(void* arg);
void SimpleModuleReloadConfig(void* arg);
void* SimpleModuleGetFD(void* arg, FcitxModuleFunctionArg args);
void* SimpleModuleGetSemaphore(void* arg, FcitxModuleFunctionArg args);

FCITX_DEFINE_PLUGIN(fcitx_simple_module, module, FcitxModule) = {
    SimpleModuleCreate,
    SimpleModuleSetFD,
    SimpleModuleProcessEvent,
    SimpleModuleDestroy,
    SimpleModuleReloadConfig
};

void* SimpleModuleCreate(FcitxInstance* instance)
{
    FcitxSimpleModule* simple = fcitx_utils_new(FcitxSimpleModule);
    UT_array *addons = FcitxInstanceGetAddons(instance);
    FcitxAddon *addon = FcitxAddonsGetAddonByName(addons, "fcitx-simple-module");
    simple->owner = instance;

    sem_init(&simple->sem,0 ,0);

    if (0 != pipe(simple->selfPipe)) {
        free(simple);
        return NULL;
    }

    fcntl(simple->selfPipe[0], F_SETFL, O_NONBLOCK);
    fcntl(simple->selfPipe[1], F_SETFL, O_NONBLOCK);

    FcitxModuleAddFunction(addon, SimpleModuleGetFD);
    FcitxModuleAddFunction(addon, SimpleModuleGetSemaphore);

    return simple;
}

void* SimpleModuleGetFD(void* arg, FcitxModuleFunctionArg args)
{
    FcitxSimpleModule* simple = (FcitxSimpleModule*) arg;
    return (void*) (intptr_t) simple->selfPipe[1];
}

void* SimpleModuleGetSemaphore(void* arg, FcitxModuleFunctionArg args)
{
    FcitxSimpleModule* simple = (FcitxSimpleModule*) arg;
    return (void*) &simple->sem;
}


void SimpleModuleDestroy(void* arg)
{
    FcitxSimpleModule* simple = (FcitxSimpleModule*) arg;
    sem_destroy(&simple->sem);
    close(simple->selfPipe[0]);
    close(simple->selfPipe[1]);
    free(simple);
}


void SimpleModuleSetFD(void* arg)
{
    FcitxSimpleModule* simple = (FcitxSimpleModule*) arg;
    FcitxInstance* instance = simple->owner;
    int maxfd = simple->selfPipe[0];
    FD_SET(maxfd, FcitxInstanceGetReadFDSet(instance));
    if (maxfd > FcitxInstanceGetMaxFD(instance))
        FcitxInstanceSetMaxFD(instance, maxfd);
}

void SimpleModuleProcessEvent(void* arg)
{
    FcitxSimpleModule* simple = (FcitxSimpleModule*) arg;
    FcitxInstance* instance = simple->owner;
    if (!FD_ISSET(simple->selfPipe[0], FcitxInstanceGetReadFDSet(instance)))
        return;

    FcitxSimplePackage package;

    while(read(simple->selfPipe[0], &package, sizeof(package)) > 0) {
        switch(package.type) {
            case SE_KeyEventPress:
            case SE_KeyEventRelease:
                FcitxSimpleFrontendProcessKey(instance, &package);
                break;
        }
    }

    sem_post(&simple->sem);
}

void SimpleModuleReloadConfig(void* arg)
{

}


