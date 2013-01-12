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

#ifndef __FX_EPOLL_UTILS_H
#define __FX_EPOLL_UTILS_H

#include "config.h"
#include "fcitx/fcitx.h"
#include "wayland-defs.h"

int fx_epoll_create_cloexec();
int fx_epoll_add_task(int epoll_fd, FcitxWaylandTask *task, uint32_t events);
int fx_epoll_del_task(int epoll_fd, const FcitxWaylandTask *task);
int fx_epoll_mod_task(int epoll_fd, FcitxWaylandTask *task, uint32_t events);
int fx_epoll_dispatch(int epoll_fd);

#endif
