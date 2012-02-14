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

#ifndef _FCITX_FRONTEND_H_
#define _FCITX_FRONTEND_H_

#include <fcitx-utils/utarray.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-config/hotkey.h>
#include <fcitx/ime.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct _FcitxInstance;

    /**
     * Input Method State
     **/
    typedef enum _FcitxContextState {
        IS_CLOSED = 0,
        IS_INACTIVE,
        IS_ACTIVE,
        IS_ENG = IS_INACTIVE /* backward compatible */
    } FcitxContextState;

    typedef enum _FcitxCapacityFlags {
        CAPACITY_NONE = 0,
        CAPACITY_CLIENT_SIDE_UI = (1 << 0),
        CAPACITY_PREEDIT = (1 << 1),
        CAPACITY_CLIENT_SIDE_CONTROL_STATE =  (1 << 2)
    } FcitxCapacityFlags;

    /**
     * Input Context, normally one for one program
     **/
    typedef struct _FcitxInputContext {
        FcitxContextState state; /**< input method state */
        int offset_x; /**< x offset to the window */
        int offset_y; /**< y offset to the window */
        int frontendid; /**< frontend id */
        void *privateic; /**< private input context data */
        FcitxCapacityFlags contextCaps; /**< input context capacity */
        struct _FcitxInputContext* next; /**< next input context */
    } FcitxInputContext;

    /**
     * Program IM Module Frontend
     **/
    typedef struct _FcitxFrontend {
        void* (*Create)(struct _FcitxInstance*, int frontendindex); /**< frontend create callback */
        boolean(*Destroy)(void *arg); /**< frontend destroy callback */
        void (*CreateIC)(void* arg, FcitxInputContext*, void* priv); /**< frontend create input context callback */
        boolean(*CheckIC)(void* arg, FcitxInputContext* arg1, void* arg2); /**< frontend check context with private value callback */
        void (*DestroyIC)(void* arg, FcitxInputContext *context); /**< frontend destroy input context callback */
        void (*EnableIM)(void* arg, FcitxInputContext* arg1); /**< frontend enable input method to client callback */
        void (*CloseIM)(void* arg, FcitxInputContext* arg1); /**< frontend close input method to client callback */
        void (*CommitString)(void* arg, FcitxInputContext* arg1, char* arg2); /**< frontend commit string callback */
        void (*ForwardKey)(void* arg, FcitxInputContext* arg1, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state); /**< frontend forward key callback */
        void (*SetWindowOffset)(void* arg, FcitxInputContext* ic, int x, int y); /**< frontend set window offset callback */
        void (*GetWindowPosition)(void* arg, FcitxInputContext* ic, int* x, int* y); /**< frontend get window position callback */
        void (*UpdatePreedit)(void* arg, FcitxInputContext* ic); /**< frontend update preedit callback */
        void (*UpdateClientSideUI)(void* arg, FcitxInputContext* ic); /**< frontend update client side user interface callback */
        void (*ReloadConfig)(void* arg); /**< frontend reload config callback */
        boolean(*CheckICFromSameApplication)(void* arg, FcitxInputContext* icToCheck, FcitxInputContext* ic); /**< frontend check input context from same application callback */
        void (*padding4)(); /**< padding */
    } FcitxFrontend;

    /**
     * Initial frontends array
     *
     * @param  frontends array
     * @return void
     **/
    void FcitxFrontendsInit(UT_array* frontends);

    /**
     * Find Input Context By Frontend Specific filter
     *
     * @param instance
     * @param frontendid frontend id
     * @param filter frontend specfic filter
     * @return FcitxInputContext*
     **/
    FcitxInputContext* FcitxInstanceFindIC(struct _FcitxInstance* instance, int frontendid, void* filter);

    /**
     * Creat New Input Context
     *
     * @param instance
     * @param frontendid frontend id
     * @param priv frontend specfic data
     * @return FcitxInputContext*
     **/
    FcitxInputContext* FcitxInstanceCreateIC(struct _FcitxInstance* instance, int frontendid, void* priv);

    /**
     * Destroy Input context
     *
     * @param instance
     * @param frontendid frontend id
     * @param filter frontend specfic filter
     * @return void
     **/
    void FcitxInstanceDestroyIC(struct _FcitxInstance* instance, int frontendid, void* filter);

    /**
     * Load All frontend
     *
     * @param instance
     * @return void
     **/
    boolean FcitxInstanceLoadFrontend(struct _FcitxInstance* instance);

    /**
     * Commit String to Client
     *
     * @param instance
     * @param ic input context
     * @param str String to commit
     * @return void
     **/
    void FcitxInstanceCommitString(struct _FcitxInstance* instance, FcitxInputContext* ic, char* str);

    /**
     * Set Cursor Position
     *
     * @param instance fcitx instance
     * @param ic input context
     * @param x xpos
     * @param y ypos
     * @return void
     **/
    void FcitxInstanceSetWindowOffset(struct _FcitxInstance* instance, FcitxInputContext* ic, int x, int y);

    /**
     * Get Cursor Position
     *
     * @param  ...
     * @param ic input context
     * @param x xpos
     * @param y ypos
     * @return void
     **/
    void FcitxInstanceGetWindowPosition(struct _FcitxInstance*, FcitxInputContext *ic, int* x, int* y);

    /**
     * Update preedit text to client window
     *
     * @param instance fcitx instance
     * @param ic input context
     * @return void
     **/
    void FcitxInstanceUpdatePreedit(struct _FcitxInstance* instance, FcitxInputContext* ic);

    /**
     * Update all user interface element to client (Aux Text, Preedit, Candidate Word)
     *
     * @param instance fcitx instance
     * @param ic input context
     * @return void
     **/
    void FcitxInstanceUpdateClientSideUI(struct _FcitxInstance* instance, FcitxInputContext* ic);

    /**
     * Get Current State, if only want to get state, this function is better, because it will handle the case that Input Context is NULL.
     *
     * @param instance fcitx instance
     * @return IME_STATE
     **/
    FcitxContextState FcitxInstanceGetCurrentState(struct _FcitxInstance* instance);
    
    /**
     * Get Current State, consider the option firstAsInactive
     *
     * @param instance fcitx instance
     * @return IME_STATE
     * 
     * @see FcitxInstanceGetCurrentState
     **/
    FcitxContextState FcitxInstanceGetCurrentStatev2(struct _FcitxInstance* instance);

    /**
     * get current ic capacity flag, if only want to get capacity, this function is better, because it will handle the case that Input Context is NULL.
     *
     * @param instance fcitx instance
     * @return CapacityFlags
     **/
    FcitxCapacityFlags FcitxInstanceGetCurrentCapacity(struct _FcitxInstance* instance);

    /**
     * set all ic from same application to the given ic
     *
     * @param instance fcitx instance
     * @param frontendid frontend id
     * @param ic object ic
     * @return void
     **/
    void FcitxInstanceSetICStateFromSameApplication(struct _FcitxInstance* instance, int frontendid, FcitxInputContext *ic);

#ifdef __cplusplus
}
#endif

#endif
/**
 * @}
 */
// kate: indent-mode cstyle; space-indent on; indent-width 0;
