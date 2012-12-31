/***************************************************************************
 *   Copyright (C) 2011~2012 by CSSlayer                                   *
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
/**
 * @file   instance-internal.h
 *
 */

#ifndef _FCITX_INSTANCE_INTERNAL_H_
#define _FCITX_INSTANCE_INTERNAL_H_

#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>

#include "fcitx-utils/utarray.h"
#include "fcitx-utils/utils.h"
#include "ui-internal.h"
#include "configfile.h"
#include "profile.h"
#include "addon.h"
#include "context.h"

typedef struct _UnusedIMItem {
    char* name;
    boolean status;
    UT_hash_handle hh;
} UnusedIMItem;

typedef struct _TimeoutItem {
    FcitxTimeoutCallback callback;
    void* arg;
    unsigned long int milli;
    uint64_t idx;
    uint64_t time;
} TimeoutItem;

typedef struct _FcitxICDataInfo {
    FcitxICDataAllocCallback allocCallback;
    FcitxICDataCopyCallback copyCallback;
    FcitxICDataFreeCallback freeCallback;
    void* arg;
} FcitxICDataInfo;

struct _FcitxInstance {
    pthread_mutex_t fcitxMutex;
    UT_array uistats;
    UT_array uimenus;
    UT_array uicompstats;
    FcitxAddon* ui;
    FcitxInputState* input;
    boolean bMutexInited;
    FcitxUIMenu imMenu;

    /* Fcitx is not good at multi process, so put a readonlyMode in it */
    boolean readonlyMode;

    /* config file */
    FcitxGlobalConfig* config;
    FcitxProfile* profile;
    UT_array addons;
    UT_array imeclasses;
    UT_array imes;
    UT_array frontends;
    UT_array modules;
    UT_array eventmodules;

    struct _FcitxInputContext *CurrentIC;
    struct _FcitxInputContext *ic_list;
    struct _FcitxInputContext *free_list;
    sem_t* sem;
    pthread_t pid;
    fd_set rfds, wfds, efds;
    int maxfd;
    char* uiname;

    struct _HookStack* hookPreInputFilter;
    struct _HookStack* hookPostInputFilter;
    struct _HookStack* hookOutputFilter;
    struct _HookStack* hookHotkeyFilter;
    struct _HookStack* hookResetInputHook;
    struct _HookStack* hookTriggerOnHook;
    struct _HookStack* hookTriggerOffHook;
    struct _HookStack* hookInputFocusHook;
    struct _HookStack* hookInputUnFocusHook;
    struct _HookStack* hookUpdateCandidateWordHook;
    struct _HookStack* hookUpdateIMListHook;
    struct _HookStack* hookCommitFilter;
    struct _HookStack* hookICStateChangedHook;
    struct _HookStack* hookIMChangedHook;

    FcitxUIFlag uiflag;

    FcitxContextState globalState;

    time_t totaltime;
    time_t timeStart;
    int iHZInputed;

    int iIMIndex;

    UT_array availimes;

    boolean imLoaded;

    FcitxAddon* uifallback;

    FcitxAddon* uinormal;

    FcitxContext* context;

    boolean tryReplace;

    char* globalIMName;

    char* fallbackuiName;

    FcitxAddon* currentIMAddon;

    UnusedIMItem* unusedItem;
    UT_array* disableList;
    UT_array* enableList;

    UT_array timeout;
    boolean initialized;
    uint64_t timeoutIdx;

    UT_array icdata;
    boolean loadingFatalError;
    boolean quietQuit;
    boolean destroy;
    int fd;
};

static inline FcitxAddon**
FcitxInstanceGetPFrontend(FcitxInstance *instance, int id)
{
    return fcitx_array_eltptr(&instance->frontends, id);
}

#endif
