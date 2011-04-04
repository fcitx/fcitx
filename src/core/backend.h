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

#include "utils/utarray.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/hotkey.h"
#include "ime.h"

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
    boolean (*Init)();
    void* (*Run)();
    boolean (*Destroy)();
    void (*CreateIC)(FcitxInputContext*, void* priv);
    boolean (*CheckIC)(FcitxInputContext* arg1, void* arg2);
    void (*DestroyIC) (FcitxInputContext *context);
    void (*CloseIM)(FcitxInputContext* arg1);
    void (*CommitString)(FcitxInputContext* arg1, char* arg2);
    void (*ForwardKey)(FcitxInputContext* arg1, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state);
    void (*SetWindowOffset)(FcitxInputContext* ic, int x, int y);
    void (*GetWindowPosition)(FcitxInputContext* ic, int* x, int* y);
    int backendid;
} FcitxBackend;

FcitxInputContext* GetCurrentIC();
void SetCurrentIC(FcitxInputContext*);
UT_array* GetFcitxBackends();
FcitxInputContext* FindIC(int backendid, void* filter);
FcitxInputContext* CreateIC(int backendid, void * priv);
void DestroyIC(int backendid, void *filter);
void StartBackend();
void CloseIM(FcitxInputContext* ic);
void CommitString(FcitxInputContext* ic, char* str);
void ChangeIMState (FcitxInputContext* ic);
void SetWindowOffset(FcitxInputContext *ic, int x, int y);
void GetWindowPosition(FcitxInputContext *ic, int* x, int* y);
IME_STATE GetCurrentState();

#endif