/***************************************************************************
 *   Copyright (C) 2010~2011 by CSSlayer                                   *
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

#ifndef FCITX_CLIENT_H
#define FCITX_CLIENT_H

#include "fcitx-config/fcitx-config.h"
#include "fcitx/ime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FcitxIMClient FcitxIMClient;
typedef void (*FcitxIMClientDestroyCallback)(FcitxIMClient* client, void* data);
typedef void (*FcitxIMClientConnectCallback)(FcitxIMClient* client, void* data);


FcitxIMClient* FcitxIMClientOpen(FcitxIMClientConnectCallback connectcb, FcitxIMClientDestroyCallback destroycb, GObject* data);
boolean IsFcitxIMClientValid(FcitxIMClient* client);
void FcitxIMClientClose(FcitxIMClient* client);
void FcitxIMClientFocusIn(FcitxIMClient* client);
void FcitxIMClientFocusOut(FcitxIMClient* client);
void FcitxIMClientSetCursorLocation(FcitxIMClient* client, int x, int y);
void FcitxIMClientReset(FcitxIMClient* client);
int FcitxIMClientProcessKey(FcitxIMClient* client, uint32_t keyval, uint32_t keycode, uint32_t state, FcitxKeyEventType type, uint32_t time);
void FcitxIMClientConnectSignal(FcitxIMClient* imclient,
    GCallback enableIM,
    GCallback closeIM,
    GCallback commitString,
    GCallback forwardKey,
    void* user_data,
    GClosureNotify freefunc
);
HOTKEYS* FcitxIMClientGetTriggerKey(FcitxIMClient* client);

#ifdef __cplusplus
}
#endif

#endif