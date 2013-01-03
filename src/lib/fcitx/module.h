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

#ifndef _FCITX_MODULE_H
#define _FCITX_MODULE_H
#include <fcitx-config/fcitx-config.h>
#include <fcitx-utils/utarray.h>
#include <fcitx-utils/utils.h>
#include <fcitx/addon.h>
#include <fcitx/fcitx.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct _FcitxInstance;

    /**
     * A misc module in Fcitx, it can register hook, or add it's own event
     *        to Fcitx main loop.
     **/
    typedef struct _FcitxModule {
        /**
         * construction function
         */
        void* (*Create)(struct _FcitxInstance* instance);
        /**
         * set main loop watch fd, no need to implement
         */
        void (*SetFD)(void*);
        /**
         * main loop event handle, no need to implement
         */
        void (*ProcessEvent)(void*);
        /**
         * destruct function
         */
        void (*Destroy)(void*);
        /**
         * reload config, no need to implement
         */
        void (*ReloadConfig)(void*);
    } FcitxModule;

    /**
     * the argument to invoke module function
     **/
    typedef struct _FcitxModuleFunctionArg {
        /**
         * arguments
         **/
        void* args[10];
    } FcitxModuleFunctionArg;
    typedef void *(*FcitxModuleFunction)(void *self, FcitxModuleFunctionArg);

    /**
     * load all modules
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxModuleLoad(struct _FcitxInstance* instance);

    /**
     * Find a exported function of a addon.
     *
     * @param addon addon
     * @param functionId function index
     * @return FcitxModuleFunction
     **/
    FcitxModuleFunction FcitxModuleFindFunction(FcitxAddon *addon,
                                                int func_id);

    void *FcitxModuleInvokeOnAddon(FcitxAddon *addon, FcitxModuleFunction func,
                                   FcitxModuleFunctionArg *args);
    /**
     * invode inter module function wiht addon pointer, returns NULL when fails (the function itself can also return NULL)
     *
     * @param addon addon
     * @param functionId function index
     * @param args arguments
     * @return void*
     **/
    FCITX_DEPRECATED
    void* FcitxModuleInvokeFunction(FcitxAddon* addon, int functionId, FcitxModuleFunctionArg args);
#define FcitxModuleInvokeVaArgs(addon, functionId, ARGV...)             \
    (FcitxModuleInvokeFunction(addon, functionId,                       \
                               (FcitxModuleFunctionArg){ .args = {ARGV} }))

    /**
     * invoke inter module function with addon name, returns NULL when fails (the function itself can also return NULL)
     *
     * @param instance fcitx instance
     * @param name addon name
     * @param functionId function index
     * @param args arguments
     * @return void*
     **/
    FCITX_DEPRECATED
    void* FcitxModuleInvokeFunctionByName(struct _FcitxInstance* instance, const char* name, int functionId, FcitxModuleFunctionArg args);
#define FcitxModuleInvokeVaArgsByName(instance, name, functionId, ARGV...) \
    (FcitxModuleInvokeFunctionByName(instance, name, functionId,        \
                               (FcitxModuleFunctionArg){ {ARGV} }))

/** call a function provides by other addon */
#define InvokeFunction(INST, MODULE, FUNC, ARG)                         \
    ((MODULE##_##FUNC##_RETURNTYPE)FcitxModuleInvokeFunctionByName(INST, MODULE##_NAME, MODULE##_##FUNC, ARG))

#define InvokeVaArgs(INST, MODULE, FUNC, ARGV...)                       \
    ((MODULE##_##FUNC##_RETURNTYPE)FcitxModuleInvokeFunctionByName(     \
        INST, MODULE##_NAME, MODULE##_##FUNC,                           \
        (FcitxModuleFunctionArg){ .args = {ARGV} }))

/** add a function to a addon */
#define AddFunction(ADDON, Realname)                                    \
    do {                                                                \
        FCITX_DEPRECATED void *(AddonFunction) = (void*)Realname;       \
        utarray_push_back(&ADDON->functionList, &(AddonFunction));      \
    } while(0)

    /**
     * add a function to a addon
     *
     * @param addon
     * @param func
     **/
    void FcitxModuleAddFunction(FcitxAddon *addon, FcitxModuleFunction func);

// Well won't work if there are multiple instances, but that will also break
// lots of other things anyway.
#define DEFINE_GET_ADDON(name, prefix)                           \
    static inline FcitxAddon*                                    \
    Fcitx##prefix##GetAddon(FcitxInstance *instance)             \
    {                                                            \
        static int _init = false;                                \
        static FcitxAddon *addon = NULL;                         \
        if (fcitx_unlikely(!_init)) {                            \
            _init = true;                                        \
            addon = FcitxAddonsGetAddonByName(                   \
                FcitxInstanceGetAddons(instance), name);         \
        }                                                        \
        return addon;                                            \
    }

#define DEFINE_GET_AND_INVOKE_FUNC(prefix, suffix, id)                  \
    DEFINE_GET_AND_INVOKE_FUNC_WITH_ERROR(prefix, suffix, id, NULL)

#define DEFINE_GET_AND_INVOKE_FUNC_WITH_ERROR(prefix, suffix, id, err_ret) \
    static inline FcitxModuleFunction                                  \
    Fcitx##prefix##Find##suffix(FcitxAddon *addon)                     \
    {                                                                  \
        static int _init = false;                                      \
        static FcitxModuleFunction func = NULL;                        \
        if (fcitx_unlikely(!_init)) {                                  \
            _init = true;                                              \
            func = FcitxModuleFindFunction(addon, id);                 \
        }                                                              \
        return func;                                                   \
    }                                                                  \
    static inline void*                                                \
    Fcitx##prefix##Invoke##suffix(FcitxInstance *instance,             \
                                  FcitxModuleFunctionArg args)         \
    {                                                                  \
        static void *const on_err = (void*)(intptr_t)(err_ret);        \
        FcitxAddon *addon = Fcitx##prefix##GetAddon(instance);         \
        if (fcitx_unlikely(!addon))                                    \
            return on_err;                                             \
        FcitxModuleFunction func = Fcitx##prefix##Find##suffix(addon); \
        if (fcitx_unlikely(!func))                                     \
            return on_err;                                             \
        return FcitxModuleInvokeOnAddon(addon, func, &args);           \
    }

#define FCITX_DEF_MODULE_ARGS(var, ARGV...)             \
    FcitxModuleFunctionArg var = { .args = {ARGV} }
    /* void *__##var##_array[] = {ARGV};                                   \ */
    /* size_t __##var##_length = sizeof(__##var##_array) / sizeof(void*);  \ */
    /* FcitxModuleFunctionArg var[] = { { .n = __##var##_length,           \ */
    /*                                    .args = __##var##_array } } */

#define FCITX_MODULE_FUNCTION_ARGS void* arg, FcitxModuleFunctionArg args
#define FCITX_MODULE_SELF(NAME, TYPE) TYPE* NAME = (TYPE*) arg;
#define FCITX_MODULE_ARG(NAME, TYPE, INDEX) TYPE NAME = (TYPE) (intptr_t) args.args[(INDEX)]

#ifdef __cplusplus
}
#endif

#endif
/**
 * @}
 */
// kate: indent-mode cstyle; space-indent on; indent-width 0;
