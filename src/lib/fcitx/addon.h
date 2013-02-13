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
 * @addtogroup Fcitx
 * @{
 */

/**
 * @file addon.h
 * Header Addon Support for fcitx
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
    struct _FcitxModule;

    /**
     * Addon Category Definition
     **/
    typedef enum _FcitxAddonCategory {
        /**
         * Input method
         **/
        AC_INPUTMETHOD = 0,
        /**
         * Input frontend, like xim
         **/
        AC_FRONTEND,
        /**
         * General module, can be implemented in a quite extensive way
         **/
        AC_MODULE,
        /**
         * User Interface, only one of it can be enabled currently.
         **/
        AC_UI
    } FcitxAddonCategory;

    /**
     * Supported Addon Type, Currently only sharedlibrary
     **/
    typedef enum _FcitxAddonType {
        AT_SHAREDLIBRARY = 0
    } FcitxAddonType;

    /**
     * How addon get input method list
     **/
    typedef enum _IMRegisterMethod {
        IMRM_SELF,
        IMRM_EXEC,
        IMRM_CONFIGFILE
    } IMRegisterMethod;

    /**
     * Addon Instance in Fcitx
     **/
    typedef struct _FcitxAddon {
        FcitxGenericConfig config; /**< config file */
        char *name; /**< addon name, used as a identifier */
        char *generalname; /**< addon name, translatable user visible string */
        char *comment; /**< longer desc translatable user visible string */
        boolean bEnabled; /**< enabled or not*/
        FcitxAddonCategory category; /**< addon category */
        FcitxAddonType type; /**< addon type */
        char *library; /**< library string */
        char *depend; /**< dependency string */
        int priority; /**< priority */
        char *subconfig; /**< used by ui for subconfig */
        union {
            struct _FcitxFrontend *frontend;
            struct _FcitxModule *module;
            struct _FcitxIMClass* imclass;
            struct _FcitxIMClass2* imclass2;
            struct _FcitxUI* ui;
        };
        void *addonInstance; /**< addon private pointer */
        UT_array functionList; /**< addon exposed function */

        IMRegisterMethod registerMethod; /**< the input method register method */
        char* registerArgument; /**< extra argument for register, unused for now */
        char* uifallback; /**< if's a user interface addon, the fallback UI addon name */
        struct _FcitxInstance* owner; /**< upper pointer to instance */
        union {
            boolean advance; /**< a hint for GUI */
            void* dummy;
        };

        union {
            boolean isIMClass2;
            void* dummy2;
        };

        union {
            boolean loadLocal;
            void* dummy3;
        };

        void* padding[7]; /**< padding */
    } FcitxAddon;

    /**
     * Init utarray for addon
     *
     * @return void
     **/
    void FcitxAddonsInit(UT_array* addons);

    /**
     * Free one addon info
     *
     * @param v addon info
     */
    void FcitxAddonFree(void *v);

    /**
     * Load all addon of fcitx during initialize
     *
     * @return void
     **/
    void FcitxAddonsLoad(UT_array* addons);

    /**
     * Resolve addon dependency, in order to make every addon works
     *
     * @return void
     **/
    void FcitxInstanceResolveAddonDependency(struct _FcitxInstance* instance);

    /**
     * Check whether an addon is enabled or not by addon name
     *
     * @param addons addon array
     * @param name addon name
     * @return boolean
     **/
    boolean FcitxAddonsIsAddonAvailable(UT_array* addons, const char* name);

    /**
     * Get addon instance by addon name
     *
     * @param addons addon array
     * @param name addon name
     * @return FcitxAddon*
     **/
    FcitxAddon* FcitxAddonsGetAddonByName(UT_array* addons, const char* name);

    /**
     * Load addon.desc file
     *
     * @return FcitxConfigFileDesc*
     **/
    FcitxConfigFileDesc* FcitxAddonGetConfigDesc();

#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */
// kate: indent-mode cstyle; space-indent on; indent-width 0;
