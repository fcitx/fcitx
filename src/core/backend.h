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

#ifndef _BACKEND_H
#define _BACKEND_H
#include "core/fcitx.h"

typedef enum _IME_STATE {
    IS_CLOSED = 0,
    IS_ENG,
    IS_CHN
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
    pthread_t pid;
    int backendid;
} FcitxBackend;

extern FcitxInputContext* CurrentIC;

FcitxInputContext* FindIC(int backendid, void* filter);
FcitxInputContext* CreateIC(int backendid, void * priv);
FcitxInputContext* DestroyIC(int backendid, void *filter);
void StartBackend();

#endif