/***************************************************************************
 *   Copyright (C) 2013~2013 by CSSlayer                                   *
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

#ifndef FCITX_DBUSSTUFF_PROPERTY_H
#define FCITX_DBUSSTUFF_PROPERTY_H

#include <dbus/dbus.h>

typedef struct _FcitxDBusPropertyTable {
    char* interface;
    char* name;
    char* type;
    void (*getfunc)(void* arg, DBusMessageIter* iter);
    void (*setfunc)(void* arg, DBusMessageIter* iter);
} FcitxDBusPropertyTable;

DBusMessage* FcitxDBusPropertyGet(void* arg, const FcitxDBusPropertyTable* propertTable, DBusMessage* message);
DBusMessage* FcitxDBusPropertySet(void* arg, const FcitxDBusPropertyTable* propertTable, DBusMessage* message);
DBusMessage* FcitxDBusPropertyGetAll(void* arg, const FcitxDBusPropertyTable* propertTable, DBusMessage* message);

static inline DBusMessage* FcitxDBusPropertyUnknownMethod(DBusMessage *msg)
{
    DBusMessage* reply = dbus_message_new_error_printf(msg,
                                                       DBUS_ERROR_UNKNOWN_METHOD,
                                                       "No such method with signature (%s)",
                                                       dbus_message_get_signature(msg));
    return reply;
}

#endif // FCITX_DBUSSTUFF_PROPERTY_H
