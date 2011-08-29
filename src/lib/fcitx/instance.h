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
#ifndef __FCITX_INSTANCE_H__
#define __FCITX_INSTANCE_H__

#include <pthread.h>
#include <semaphore.h>
#include <sys/select.h>
#include <fcitx/ui.h>
#include <fcitx-utils/utarray.h>
#include <fcitx/configfile.h>
#include <fcitx/profile.h>
#include <fcitx/addon.h>
#include <fcitx/ime.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct _HookStack;
    struct _FcitxInputContext;

    /**
     * @brief Fcitx Instance, including all global settings
     **/
    typedef struct _FcitxInstance {
        pthread_mutex_t fcitxMutex;
        UT_array uistats;
        UT_array uimenus;
        FcitxAddon* ui;
        FcitxInputState input;
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

        FcitxUIFlag uiflag;
    } FcitxInstance;

    /**
     * @brief create new fcitx instance
     *
     * @param sem semaphore to notify the instance is end
     * @param argc argc
     * @param argv argv
     * @return FcitxInstance*
     **/
    FcitxInstance* CreateFcitxInstance(sem_t *sem, int argc, char* argv[]);

    /**
     * @brief lock the instance
     *
     * @param instance fcitx instance
     * @return int
     **/
    int FcitxLock(FcitxInstance* instance);

    /**
     * @brief lock the instance
     *
     * @param instance fcitx instance
     * @return int
     **/
    int FcitxUnlock(FcitxInstance* instance);

    /**
     * @brief notify the instance is end
     *
     * @param instance fcitx instance
     * @return void
     **/
    void EndInstance(FcitxInstance* instance);

#ifdef __cplusplus
}
#endif

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
