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

#ifndef FCITX_IPC_PORTAL_H
#define FCITX_IPC_PORTAL_H

#ifdef __cplusplus
extern "C" {
#endif

#define FCITX_IM_DBUS_PORTAL_PATH "/org/freedesktop/portal/inputmethod"
#define FCITX_IC_DBUS_PORTAL_PATH "/org/freedesktop/portal/inputcontext/%d"

#define FCITX_PORTAL_SERVICE "org.freedesktop.portal.Fcitx"
#define FCITX_IM_DBUS_INTERFACE "org.fcitx.Fcitx.InputMethod1"
#define FCITX_IC_DBUS_INTERFACE "org.fcitx.Fcitx.InputContext1"

#ifdef __cplusplus
}
#endif

#endif // FCITX_IPC_PORTAL_H
// kate: indent-mode cstyle; space-indent on; indent-width 0;
