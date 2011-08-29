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
     * @brief Input Method State
     **/
    typedef enum _IME_STATE {
        IS_CLOSED = 0,
        IS_ENG,
        IS_ACTIVE
    } IME_STATE;

    typedef enum _CapacityFlags {
        CAPACITY_NONE = 0,
        CAPACITY_CLIENT_SIDE_UI = (1 << 0),
        CAPACITY_PREEDIT = (1 << 1)
    } CapacityFlags;

    /**
     * @brief Input Context, normally one for one program
     **/
    typedef struct _FcitxInputContext
    {
        IME_STATE state; /* im state */
        int offset_x, offset_y;
        int frontendid;
        void *privateic;
        CapacityFlags contextCaps;
        struct _FcitxInputContext* next;
    } FcitxInputContext;

    /**
     * @brief Program IM Module Frontend
     **/
    typedef struct _FcitxFrontend
    {
        void* (*Create)(struct _FcitxInstance*, int frontendindex);
        boolean (*Destroy)(void *arg);
        void (*CreateIC)(void* arg, FcitxInputContext*, void* priv);
        boolean (*CheckIC)(void* arg, FcitxInputContext* arg1, void* arg2);
        void (*DestroyIC) (void* arg, FcitxInputContext *context);
        void (*EnableIM)(void* arg, FcitxInputContext* arg1);
        void (*CloseIM)(void* arg, FcitxInputContext* arg1);
        void (*CommitString)(void* arg, FcitxInputContext* arg1, char* arg2);
        void (*ForwardKey)(void* arg, FcitxInputContext* arg1, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state);
        void (*SetWindowOffset)(void* arg, FcitxInputContext* ic, int x, int y);
        void (*GetWindowPosition)(void* arg, FcitxInputContext* ic, int* x, int* y);
        void (*UpdatePreedit)(void* arg, FcitxInputContext* ic);
        void (*UpdateClientSideUI)(void* arg, FcitxInputContext* ic);
        void (*padding2)();
        void (*padding3)();
        void (*padding4)();
    } FcitxFrontend;

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
     * @brief Initial frontends array
     *
     * @param  frontends array
     * @return void
     **/
    void InitFcitxFrontends(UT_array* );

    /**
     * @brief Find Input Context By Frontend Specific filter
     *
     * @param instance
     * @param frontendid frontend id
     * @param filter frontend specfic filter
     * @return FcitxInputContext*
     **/
    FcitxInputContext* FindIC(struct _FcitxInstance* instance, int frontendid, void* filter);

    /**
     * @brief Creat New Input Context
     *
     * @param instance
     * @param frontendid frontend id
     * @param priv frontend specfic data
     * @return FcitxInputContext*
     **/
    FcitxInputContext* CreateIC(struct _FcitxInstance* instance, int frontendid, void* priv);

    /**
     * @brief Destroy Input context
     *
     * @param instance
     * @param frontendid frontend id
     * @param filter frontend specfic filter
     * @return void
     **/
    void DestroyIC(struct _FcitxInstance* instance, int frontendid, void* filter);

    /**
     * @brief Load All frontend
     *
     * @param instance
     * @return void
     **/
    boolean LoadFrontend(struct _FcitxInstance* instance );

    /**
     * @brief End Input
     *
     * @param instance
     * @param ic input context
     * @return void
     **/
    void CloseIM(struct _FcitxInstance* instance, FcitxInputContext* ic);

    /**
     * @brief Commit String to Client
     *
     * @param instance
     * @param ic input context
     * @param str String to commit
     * @return void
     **/
    void CommitString(struct _FcitxInstance* instance, FcitxInputContext* ic, char* str);

    /**
     * @brief ...
     *
     * @param  ...
     * @param ic ...
     * @return void
     **/
    void ChangeIMState (struct _FcitxInstance*, FcitxInputContext* ic);

    /**
     * @brief Set Cursor Position
     *
     * @param  ...
     * @param ic input context
     * @param x xpos
     * @param y ypos
     * @return void
     **/
    void SetWindowOffset(struct _FcitxInstance*, FcitxInputContext* ic, int x, int y);

    /**
     * @brief Get Cursor Position
     *
     * @param  ...
     * @param ic input context
     * @param x xpos
     * @param y ypos
     * @return void
     **/
    void GetWindowPosition(struct _FcitxInstance*, FcitxInputContext *ic, int* x, int* y);

    /**
     * @brief Update preedit text to client window
     *
     * @param instance fcitx instance
     * @param ic input context
     * @return void
     **/
    void UpdatePreedit(struct _FcitxInstance* instance, FcitxInputContext* ic);

    /**
     * @brief Update all user interface element to client (Aux Text, Preedit, Candidate Word)
     *
     * @param instance fcitx instance
     * @param ic input context
     * @return void
     **/
    void UpdateClientSideUI(struct _FcitxInstance* instance, FcitxInputContext* ic);

    /**
     * @brief Get Current State, if only want to get state, this function is better, because it will handle the case that Input Context is NULL.
     *
     * @param instance fcitx instance
     * @return IME_STATE
     **/
    IME_STATE GetCurrentState(struct _FcitxInstance* instance);

    /**
     * @brief get current ic capacity flag, if only want to get capacity, this function is better, because it will handle the case that Input Context is NULL.
     *
     * @param instance fcitx instance
     * @return CapacityFlags
     **/
    CapacityFlags GetCurrentCapacity(struct _FcitxInstance* instance);

#ifdef __cplusplus
}
#endif

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
