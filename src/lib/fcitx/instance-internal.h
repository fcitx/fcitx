/***************************************************************************
 *   Copyright (C) 2011~2011 by CSSlayer                                   *
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
/**
 * @file   instance-internal.h
 *
 */

#ifndef _FCITX_INSTANCE_INTERNAL_H_
#define _FCITX_INSTANCE_INTERNAL_H_

#include <semaphore.h>

#include "fcitx-utils/utarray.h"
#include "configfile.h"
#include "profile.h"
#include "addon.h"

struct _FcitxInstance {
    pthread_mutex_t fcitxMutex;
    UT_array uistats;
    UT_array uimenus;
    FcitxAddon* ui;
    FcitxInputState* input;
    boolean bMutexInited;
    FcitxUIMenu imMenu;

    /* Fcitx is not good at multi process, so put a readonlyMode in it */
    boolean readonlyMode;

    /* config file */
    FcitxConfig* config;
    FcitxProfile* profile;
    UT_array addons;
    UT_array imeclasses;
    UT_array imes;
    UT_array frontends;
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

    FcitxUIFlag uiflag;

    IME_STATE globalState;

    time_t totaltime;
    time_t timeStart;
    int iHZInputed;

    int iIMIndex;

    UT_array availimes;

    boolean imLoaded;

    /* gives more padding, since we want to break abi */
    int padding[62];
};

#endif

