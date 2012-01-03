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
    FcitxInstance* FcitxInstanceCreate(sem_t *sem, int argc, char* argv[]);

    /**
     * @brief replace existing fcitx instance
     *
     * @param instance fcitx instance
     * @return boolean
     **/
    boolean FcitxInstanceIsTryReplace(FcitxInstance* instance);

    /**
     * @brief replace existing fcitx instance
     *
     * @param instance fcitx instance
     * @return bool
     **/
    void FcitxInstanceResetTryReplace(FcitxInstance* instance);

    /**
     * @brief lock the instance
     *
     * @param instance fcitx instance
     * @return int
     **/
    int FcitxInstanceLock(FcitxInstance* instance);

    /**
     * @brief lock the instance
     *
     * @param instance fcitx instance
     * @return int
     **/
    int FcitxInstanceUnlock(FcitxInstance* instance);

    /**
     * @brief notify the instance is end
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxInstanceEnd(FcitxInstance* instance);

    /**
     * @brief Get Current Input Context
     *
     * @param instance
     * @return FcitxInputContext*
     **/
    FcitxInputContext* FcitxInstanceGetCurrentIC(struct _FcitxInstance* instance);

    /**
     * @brief Set Current Input Context
     *
     * @param instance
     * @param ic new input context
     * @return current ic changed
     **/
    boolean FcitxInstanceSetCurrentIC(struct _FcitxInstance* instance, FcitxInputContext* ic);

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

    FcitxGlobalConfig* FcitxInstanceGetGlobalConfig(FcitxInstance* instance);

    FcitxProfile* FcitxInstanceGetProfile(FcitxInstance* instance);

    FcitxInputState* FcitxInstanceGetInputState(FcitxInstance* instance);

    void FcitxInstanceIncreateInputCharacterCount(FcitxInstance* instance, int count);

#ifdef __cplusplus
}
#endif

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
