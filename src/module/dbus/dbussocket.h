/***************************************************************************
 *   Copyright (C) 2012~2013 by CSSlayer                                   *
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

#ifndef _FCITX_DBUSSOCKET_H
#define _FCITX_DBUSSOCKET_H

#include <sys/select.h>
#include <dbus/dbus.h>

typedef struct _FcitxDBusWatch {
    DBusWatch *watch;
    struct _FcitxDBusWatch *next;
} FcitxDBusWatch;

dbus_bool_t DBusAddWatch(DBusWatch *watch, void *data);
void DBusRemoveWatch(DBusWatch *watch, void *data);
int DBusUpdateFDSet(FcitxDBusWatch* watches, fd_set* rfds, fd_set* wfds, fd_set* efds);
void DBusProcessEventForWatches(FcitxDBusWatch* watches, fd_set* rfds, fd_set* wfds, fd_set* efds);
void DBusProcessEventForConnection(DBusConnection* connection);

#endif // _FCITX_DBUSSOCKET_H

