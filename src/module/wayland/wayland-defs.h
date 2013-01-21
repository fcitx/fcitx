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

#ifndef WAYLAND_DEFS_H
#define WAYLAND_DEFS_H

#include <stdint.h>
#include <fcitx-utils/utils.h>

#ifndef FCITX_DISABLE_WAYLAND

#include <wayland-client.h>
#include <sys/epoll.h>

#else

/* copied from glibc header */
enum EPOLL_EVENTS {
    EPOLLIN = 0x001,
#define EPOLLIN EPOLLIN
    EPOLLPRI = 0x002,
#define EPOLLPRI EPOLLPRI
    EPOLLOUT = 0x004,
#define EPOLLOUT EPOLLOUT
    EPOLLRDNORM = 0x040,
#define EPOLLRDNORM EPOLLRDNORM
    EPOLLRDBAND = 0x080,
#define EPOLLRDBAND EPOLLRDBAND
    EPOLLWRNORM = 0x100,
#define EPOLLWRNORM EPOLLWRNORM
    EPOLLWRBAND = 0x200,
#define EPOLLWRBAND EPOLLWRBAND
    EPOLLMSG = 0x400,
#define EPOLLMSG EPOLLMSG
    EPOLLERR = 0x008,
#define EPOLLERR EPOLLERR
    EPOLLHUP = 0x010,
#define EPOLLHUP EPOLLHUP
    EPOLLRDHUP = 0x2000,
#define EPOLLRDHUP EPOLLRDHUP
    EPOLLWAKEUP = 1u << 29,
#define EPOLLWAKEUP EPOLLWAKEUP
    EPOLLONESHOT = 1u << 30,
#define EPOLLONESHOT EPOLLONESHOT
    EPOLLET = 1u << 31
#define EPOLLET EPOLLET
};

#endif

typedef struct _FcitxWaylandTask FcitxWaylandTask;
typedef void (*FcitxWaylandTaskHandler)(FcitxWaylandTask *task,
                                        uint32_t events);
struct _FcitxWaylandTask {
    FcitxWaylandTaskHandler handler;
    int fd;
    void *padding[2];
};

typedef void (*FcitxWaylandHandleGlobalAdded)(void *data, uint32_t name,
                                              const char *iface, uint32_t ver);
typedef void (*FcitxWaylandHandleGlobalRemoved)(void *data, uint32_t name,
                                                const char *iface,
                                                uint32_t ver);
typedef void (*FcitxWaylandSyncCallback)(void *data, uint32_t serial);

/* need to free buff */
typedef void (*FcitxWaylandReadAllCallback)(void *data, char *buff,
                                            size_t len);

#endif
