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

/**
 * @file addon.h
 * @brief Header Addon Support for fcitx
 * @author CSSlayer wengxt@gmail.com
 */

#ifndef _FCITX_ADDON_H_
#define _FCITX_ADDON_H_
#include "fcitx-utils/utarray.h"
#include "fcitx-config/fcitx-config.h"

/**
 * @brief Addon Category Definition
 **/
typedef enum AddonCategory
{
    /**
     * @brief Input method
     **/
    AC_INPUTMETHOD = 0,
    /**
     * @brief Input backend, like xim
     **/
   AC_BACKEND,
    /**
     * @brief General module, can be implemented in a quite extensive way
     **/
    AC_MODULE,
    /**
     * @brief User Interface, only one of it can be enabled currently.
     **/
    AC_UI
} AddonCategory;

/**
 * @brief Supported Addon Type, Currently only sharedlibrary
 **/
typedef enum AddonType
{
    AT_SHAREDLIBRARY = 0
} AddonType;

/**
 * @brief Addon Instance in Fcitx
 **/
typedef struct FcitxAddon
{
    GenericConfig config;
    char *name;
    boolean bEnabled;
    AddonCategory category;
    AddonType type;
    char *library;
    char *depend;
    int priority;
    union {
        struct FcitxBackend *backend;
        struct FcitxModule *module;
        struct FcitxIMClass* imclass;
        struct FcitxUI* ui;
    };
    void *addonInstance;
    pthread_t pid;
    UT_array functionList;
} FcitxAddon;

/**
 * @brief Init utarray for addon
 *
 * @return void
 **/
void InitFcitxAddons(UT_array* addons);

/** 
 * @brief Free one addon info
 * 
 * @param v addon info
 */
void FreeAddon(void *v);

/**
 * @brief Load all addon of fcitx during initialize
 *
 * @return void
 **/
void LoadAddonInfo(UT_array* addons);

/**
 * @brief Resolve addon dependency, in order to make every addon works
 *
 * @return void
 **/
void AddonResolveDependency(UT_array* addons);

/**
 * @brief Check whether an addon is enabled or not by addon name
 *
 * @param name ...
 * @return boolean
 **/
boolean AddonIsAvailable(UT_array* addons, const char* name);

/**
 * @brief Get addon instance by addon name
 *
 * @param name addon name
 * @return FcitxAddon*
 **/
FcitxAddon* GetAddonByName(UT_array* addons, const char* name);

#endif
