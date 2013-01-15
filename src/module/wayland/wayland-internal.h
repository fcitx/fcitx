/***************************************************************************
 *   Copyright (C) 2013~2013 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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

#ifndef WAYLAND_DEFS_INTERNAL_H
#define WAYLAND_DEFS_INTERNAL_H

#include "config.h"
#include "fcitx/fcitx.h"
#include "wayland-defs.h"
#include <fcitx-utils/handler-table.h>

typedef struct {
    FcitxInstance *owner;
    struct wl_display *dpy;
    int epoll_fd;
    FcitxWaylandTask dpy_task;
    FcitxHandlerTable *global_handlers;
} FcitxWayland;

#endif
