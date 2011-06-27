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
typedef enum _IME_STATE {
    IS_CLOSED = 0,
    IS_ENG,
    IS_ACTIVE
} IME_STATE;

typedef struct FcitxInputContext
{
    IME_STATE state; /* im state */
    int offset_x, offset_y;
    int backendid;
    void *privateic;
    struct FcitxInputContext* next;
} FcitxInputContext;

typedef struct FcitxBackend
{
    void* (*Create)(struct FcitxInstance*, int backendindex);
    boolean (*Destroy)(void *arg);
    void (*CreateIC)(void* arg, FcitxInputContext*, void* priv);
    boolean (*CheckIC)(void* arg, FcitxInputContext* arg1, void* arg2);
    void (*DestroyIC) (void* arg, FcitxInputContext *context);
    void (*CloseIM)(void* arg, FcitxInputContext* arg1);
    void (*CommitString)(void* arg, FcitxInputContext* arg1, char* arg2);
    void (*ForwardKey)(void* arg, FcitxInputContext* arg1, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state);
    void (*SetWindowOffset)(void* arg, FcitxInputContext* ic, int x, int y);
    void (*GetWindowPosition)(void* arg, FcitxInputContext* ic, int* x, int* y);
} FcitxBackend;

FcitxInputContext* GetCurrentIC(struct FcitxInstance* instance);
void SetCurrentIC(struct FcitxInstance* instance, FcitxInputContext* ic);
void InitFcitxBackends(UT_array* );
FcitxInputContext* FindIC(struct FcitxInstance* instance, int backendid, void* filter);
FcitxInputContext* CreateIC(struct FcitxInstance* instance, int backendid, void* priv);
void DestroyIC(struct FcitxInstance* instance, int backendid, void* filter);
void LoadBackend(struct FcitxInstance* instance );
void CloseIM(struct FcitxInstance* instance, FcitxInputContext* ic);
void CommitString(struct FcitxInstance* instance, FcitxInputContext* ic, char* str);
void ChangeIMState (struct FcitxInstance*, FcitxInputContext* ic);
void SetWindowOffset(struct FcitxInstance*, FcitxInputContext* ic, int x, int y);
void GetWindowPosition(struct FcitxInstance*, FcitxInputContext *ic, int* x, int* y);
IME_STATE GetCurrentState(struct FcitxInstance* instance);

#endif