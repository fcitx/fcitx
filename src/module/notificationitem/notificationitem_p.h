/***************************************************************************
 *   Copyright (C) 2013~2013 by CSSlayer                                   *
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

#ifndef NOTIFICATIONITEM_P_H
#define NOTIFICATIONITEM_P_H

#include <dbus/dbus.h>
#include "fcitx/instance.h"
#include "notificationitem.h"

typedef struct _MenuIdSet {
    int id;
    UT_hash_handle hh;
} MenuIdSet;

typedef struct _FcitxNotificationItem {
    FcitxInstance* owner;
    DBusConnection* conn;
    FcitxNotificationItemAvailableCallback callback;
    void* data;
    boolean available;
    int index;
    char* serviceName;
    uint32_t revision;
    int pendingActionId;
    boolean isUnity;
    char layoutNameBuffer[3];
    MenuIdSet *ids;
} FcitxNotificationItem;

boolean FcitxDBusMenuCreate(FcitxNotificationItem* notificationitem);

MenuIdSet* MenuIdSetAdd(MenuIdSet *ids, int id);
boolean MenuIdSetContains(MenuIdSet *ids, int id);
MenuIdSet* MenuIdSetClear(MenuIdSet* ids);

static inline boolean CheckAddPrefix( const char** name) {
    boolean result = !((*name)[0] == '\0' || (*name)[0] == '/' || (*name)[0] == '@');
    if ((*name)[0] == '@') {
        (*name) += 1;
    }

    return result;
}

#endif // NOTIFICATIONITEM_P_H
