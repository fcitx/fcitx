/***************************************************************************
 *   Copyright (C) 2010~2012 by CSSlayer                                   *
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

#ifndef FCITX_CLIENT_H
#define FCITX_CLIENT_H

#include <dbus/dbus-glib.h>
#include "fcitx-config/fcitx-config.h"
#include "fcitx/ime.h"
#include "fcitx/frontend.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct _FcitxIMClient FcitxIMClient;
    typedef void (*FcitxIMClientDestroyCallback)(FcitxIMClient* client, void* data);
    typedef void (*FcitxIMClientConnectCallback)(FcitxIMClient* client, void* data);


    FcitxIMClient* FcitxIMClientOpen(FcitxIMClientConnectCallback connectcb, FcitxIMClientDestroyCallback destroycb, GObject* data);
    boolean IsFcitxIMClientValid(FcitxIMClient* client);
    boolean IsFcitxIMClientEnabled(FcitxIMClient* client);
    void FcitxIMClientSetEnabled(FcitxIMClient* client, boolean enable);
    void FcitxIMClientClose(FcitxIMClient* client);
    void FcitxIMClientFocusIn(FcitxIMClient* client);
    void FcitxIMClientFocusOut(FcitxIMClient* client);
    void FcitxIMClientSetCursorLocation(FcitxIMClient* client, int x, int y);
    void FcitxIMClientSetCursorRect(FcitxIMClient* client, int x, int y, int w, int h);
    void FcitxIMClientSetCapacity(FcitxIMClient* client, FcitxCapacityFlags flags);
    void FcitxIMClientReset(FcitxIMClient* client);
    void FcitxIMClientProcessKey(FcitxIMClient* client, DBusGProxyCallNotify callback, void* user_data, GDestroyNotify notify, uint32_t keyval, uint32_t keycode, uint32_t state, FcitxKeyEventType type, uint32_t t);
    int FcitxIMClientProcessKeySync(FcitxIMClient* client,
                                    uint32_t keyval, uint32_t keycode, uint32_t state, FcitxKeyEventType type, uint32_t t);
    void FcitxIMClientConnectSignal(FcitxIMClient* imclient,
                                    GCallback enableIM,
                                    GCallback closeIM,
                                    GCallback commitString,
                                    GCallback forwardKey,
                                    GCallback updatePreedit,
                                    GCallback updateFormattedPreedit,
                                    GCallback deleteSurroundingText,
                                    void* user_data,
                                    GClosureNotify freefunc
                                   );
    FcitxHotkey* FcitxIMClientGetTriggerKey(FcitxIMClient* client);
    void FcitxIMClientSetSurroundingText(FcitxIMClient* imclient,
                                         const gchar* text,
                                         guint cursorPos,
                                         guint anchorPos
                                        );

#ifdef __cplusplus
}
#endif

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
