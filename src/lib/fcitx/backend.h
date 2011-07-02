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

#ifndef _FCITX_BACKEND_H_
#define _FCITX_BACKEND_H_

#include "fcitx-utils/utarray.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/hotkey.h"
#include "ime.h"

struct FcitxInstance;

/**
 * @brief Input Method State
 **/
typedef enum IME_STATE {
    IS_CLOSED = 0,
    IS_ENG,
    IS_ACTIVE
} IME_STATE;

/**
 * @brief Input Context, normally one for one program
 **/
typedef struct FcitxInputContext
{
    IME_STATE state; /* im state */
    int offset_x, offset_y;
    int backendid;
    void *privateic;
    struct FcitxInputContext* next;
} FcitxInputContext;

/**
 * @brief Program IM Module Backend
 **/
typedef struct FcitxBackend
{
    void* (*Create)(struct FcitxInstance*, int backendindex);
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
} FcitxBackend;

/**
 * @brief Get Current Input Context
 *
 * @param instance 
 * @return FcitxInputContext*
 **/
FcitxInputContext* GetCurrentIC(struct FcitxInstance* instance);

/**
 * @brief Set Current Input Context
 *
 * @param instance 
 * @param ic new input context
 * @return void
 **/
void SetCurrentIC(struct FcitxInstance* instance, FcitxInputContext* ic);

/**
 * @brief Initial backends array
 *
 * @param  backends array
 * @return void
 **/
void InitFcitxBackends(UT_array* );

/**
 * @brief Find Input Context By Backend Specific filter
 *
 * @param instance 
 * @param backendid backend id
 * @param filter backend specfic filter
 * @return FcitxInputContext*
 **/
FcitxInputContext* FindIC(struct FcitxInstance* instance, int backendid, void* filter);

/**
 * @brief Creat New Input Context
 *
 * @param instance 
 * @param backendid backend id
 * @param priv backend specfic data
 * @return FcitxInputContext*
 **/
FcitxInputContext* CreateIC(struct FcitxInstance* instance, int backendid, void* priv);

/**
 * @brief Destroy Input context
 *
 * @param instance 
 * @param backendid backend id
 * @param filter backend specfic filter
 * @return void
 **/
void DestroyIC(struct FcitxInstance* instance, int backendid, void* filter);

/**
 * @brief Load All backend
 *
 * @param instance 
 * @return void
 **/
void LoadBackend(struct FcitxInstance* instance );

/**
 * @brief End Input
 *
 * @param instance 
 * @param ic input context
 * @return void
 **/
void CloseIM(struct FcitxInstance* instance, FcitxInputContext* ic);

/**
 * @brief Commit String to Client
 *
 * @param instance 
 * @param ic input context
 * @param str String to commit
 * @return void
 **/
void CommitString(struct FcitxInstance* instance, FcitxInputContext* ic, char* str);

/**
 * @brief ...
 *
 * @param  ...
 * @param ic ...
 * @return void
 **/
void ChangeIMState (struct FcitxInstance*, FcitxInputContext* ic);

/**
 * @brief Set Cursor Position
 *
 * @param  ...
 * @param ic input context
 * @param x xpos
 * @param y ypos
 * @return void
 **/
void SetWindowOffset(struct FcitxInstance*, FcitxInputContext* ic, int x, int y);

/**
 * @brief Get Cursor Position
 *
 * @param  ...
 * @param ic input context
 * @param x xpos
 * @param y ypos
 * @return void
 **/
void GetWindowPosition(struct FcitxInstance*, FcitxInputContext *ic, int* x, int* y);

/**
 * @brief Get Current State, if only want to get state, this function is better, because it will handle the case that Input Context is NULL.
 *
 * @param instance 
 * @return IME_STATE
 **/
IME_STATE GetCurrentState(struct FcitxInstance* instance);

#endif