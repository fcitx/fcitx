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

#ifndef _FCITX_MODULE_H
#define _FCITX_MODULE_H
#include <fcitx-config/fcitx-config.h>
#include <fcitx-utils/utarray.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct _FcitxInstance;
    struct _FcitxAddon;

    /**
     * @brief A misc module in Fcitx, it can register hook, or add it's own event
     *        to Fcitx main loop.
     **/
    typedef struct _FcitxModule
    {
        /**
         * @brief construction function
         */
        void* (*Create)(struct _FcitxInstance* instance);
        /**
         * @brief set main loop watch fd, no need to implement
         */
        void (*SetFD)(void*);
        /**
         * @brief main loop event handle, no need to implement
         */
        void (*ProcessEvent)(void*);
        /**
         * @brief destruct function
         */
        void (*Destroy)(void*);
        /**
         * @brief reload config, no need to implement
         */
        void (*ReloadConfig)(void*);
    } FcitxModule;

    /**
     * @brief the argument to invoke module function
     **/
    typedef struct _FcitxModuleFunctionArg
    {
        /**
         * @brief arguments
         **/
        void* args[10];
    } FcitxModuleFunctionArg;

    /**
     * @brief init module array
     *
     * @param modules module array
     * @return void
     **/
    void InitFcitxModules(UT_array* modules);

    /**
     * @brief load all modules
     *
     * @param instance fcitx instance
     * @return void
     **/
    void LoadModule(struct _FcitxInstance* instance);

    /**
     * @brief invode inter module function wiht addon pointer, returns NULL when fails (the function itself can also return NULL)
     *
     * @param addon addon
     * @param functionId function index
     * @param args arguments
     * @return void*
     **/
    void* InvokeModuleFunction(struct _FcitxAddon* addon, int functionId, FcitxModuleFunctionArg args);

    /**
     * @brief invoke inter module function with addon name, returns NULL when fails (the function itself can also return NULL)
     *
     * @param instance fcitx instance
     * @param name addon name
     * @param functionId function index
     * @param args arguments
     * @return void*
     **/
    void* InvokeModuleFunctionWithName(struct _FcitxInstance* instance, const char* name, int functionId, FcitxModuleFunctionArg args);

#define InvokeFunction(INST, MODULE, FUNC, ARG)  \
    ((MODULE##_##FUNC##_RETURNTYPE) InvokeModuleFunctionWithName(INST, MODULE##_NAME, MODULE##_##FUNC, ARG))

#define AddFunction(ADDON, Realname) \
    do { \
        void *temp = Realname; \
        utarray_push_back(&ADDON->functionList, &temp); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
