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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#ifndef _FCITX_DBUS_H
#define _FCITX_DBUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <fcitx-config/fcitx-config.h>

#define FCITX_DBUS_NAME "fcitx-dbus"
#define FCITX_DBUS_GETCONNECTION 0
#define FCITX_DBUS_GETCONNECTION_RETURNTYPE DBusConnection*

#define FCITX_DBUS_SERVICE "org.fcitx.Fcitx"

#ifdef __cplusplus
}
#endif

#endif // _FCITX_DBUS_H
// kate: indent-mode cstyle; space-indent on; indent-width 0;
