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

/**
 * @file addon.h
 * @brief Header Addon Support for fcitx
 * @author CSSlayer wengxt@gmail.com
 */

#ifndef _FCITX_ADDON_H_
#define _FCITX_ADDON_H_

#include <fcitx-utils/utarray.h>
#include <fcitx-config/fcitx-config.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct _FcitxInstance;
    /**
     * @brief Addon Category Definition
     **/
    typedef enum _FcitxAddonCategory {
        /**
         * @brief Input method
         **/
        AC_INPUTMETHOD = 0,
        /**
         * @brief Input frontend, like xim
         **/
        AC_FRONTEND,
        /**
         * @brief General module, can be implemented in a quite extensive way
         **/
        AC_MODULE,
        /**
         * @brief User Interface, only one of it can be enabled currently.
         **/
        AC_UI
    } FcitxAddonCategory;

    /**
     * @brief Supported Addon Type, Currently only sharedlibrary
     **/
    typedef enum _FcitxAddonType {
        AT_SHAREDLIBRARY = 0
    } FcitxAddonType;

    /**
     * @brief How addon get input method list
     **/
    typedef enum _IMRegisterMethod {
        IMRM_SELF,
        IMRM_EXEC,
        IMRM_CONFIGFILE
    } IMRegisterMethod;

    /**
     * @brief Addon Instance in Fcitx
     **/
    typedef struct _FcitxAddon {
        FcitxGenericConfig config;
        char *name;
        char *generalname;
        char *comment;
        boolean bEnabled;
        FcitxAddonCategory category;
        FcitxAddonType type;
        char *library;
        char *depend;
        int priority;
        char *subconfig;
        union {
            struct _FcitxFrontend *frontend;
            struct _FcitxModule *module;
            struct _FcitxIMClass* imclass;
            struct _FcitxUI* ui;
        };
        void *addonInstance;
        UT_array functionList;

        IMRegisterMethod registerMethod;
        char* registerArgument;
        char* uifallback;
        struct _FcitxInstance* owner;

        void* padding[9];
    } FcitxAddon;

    /**
     * @brief Init utarray for addon
     *
     * @return void
     **/
    void FcitxAddonsInit(UT_array* addons);

    /**
     * @brief Free one addon info
     *
     * @param v addon info
     */
    void FcitxAddonFree(void *v);

    /**
     * @brief Load all addon of fcitx during initialize
     *
     * @return void
     **/
    void FcitxAddonsLoad(UT_array* addons);

    /**
     * @brief Resolve addon dependency, in order to make every addon works
     *
     * @return void
     **/
    void FcitxInstanceResolveAddonDependency(struct _FcitxInstance* instance);

    /**
     * @brief Check whether an addon is enabled or not by addon name
     *
     * @param addons addon array
     * @param name addon name
     * @return boolean
     **/
    boolean FcitxAddonsIsAddonAvailable(UT_array* addons, const char* name);

    /**
     * @brief Get addon instance by addon name
     *
     * @param addons addon array
     * @param name addon name
     * @return FcitxAddon*
     **/
    FcitxAddon* FcitxAddonsGetAddonByName(UT_array* addons, const char* name);

    /**
     * @brief Load addon.desc file
     *
     * @return FcitxConfigFileDesc*
     **/
    FcitxConfigFileDesc* FcitxAddonGetConfigDesc();

#ifdef __cplusplus
}
#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
