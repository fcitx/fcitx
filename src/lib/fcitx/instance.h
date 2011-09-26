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
    typedef struct _FcitxInstance FcitxInstance;

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

    /**
     * @brief Get Current Input Context
     *
     * @param instance
     * @return FcitxInputContext*
     **/
    FcitxInputContext* GetCurrentIC(struct _FcitxInstance* instance);

    /**
     * @brief Set Current Input Context
     *
     * @param instance
     * @param ic new input context
     * @return current ic changed
     **/
    boolean SetCurrentIC(struct _FcitxInstance* instance, FcitxInputContext* ic);


    /**
     * @brief Get Addons From Instance
     *
     * @param instance fcitx instance
     * @return UT_array*
     **/
    UT_array* FcitxInstanceGetAddons(FcitxInstance* instance);

    UT_array* FcitxInstanceGetUIMenus(FcitxInstance* instance);

    UT_array* FcitxInstanceGetUIStats(FcitxInstance* instance);

    UT_array* FcitxInstanceGetIMEs(FcitxInstance* instance);

    UT_array* FcitxInstanceGetAvailIMEs(FcitxInstance* instance);

    fd_set* FcitxInstanceGetReadFDSet(FcitxInstance* instance);

    fd_set* FcitxInstanceGetWriteFDSet(FcitxInstance* instance);

    fd_set* FcitxInstanceGetExceptFDSet(FcitxInstance* instance);

    int FcitxInstanceGetMaxFD(FcitxInstance* instance);

    void FcitxInstanceSetMaxFD(FcitxInstance* instance, int maxfd);

    FcitxConfig* FcitxInstanceGetConfig(FcitxInstance* instance);

    FcitxProfile* FcitxInstanceGetProfile(FcitxInstance* instance);

    FcitxInputState* FcitxInstanceGetInputState(FcitxInstance* instance);

    void FcitxInstanceIncreateInputCharacterCount(FcitxInstance* instance, int count);

#ifdef __cplusplus
}
#endif

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
