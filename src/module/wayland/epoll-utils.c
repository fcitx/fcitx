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

#include "config.h"
#include "fcitx/fcitx.h"
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include "fcitx/module.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"
#include "fcitx/instance.h"
#include "wayland-internal.h"
#include "epoll-utils.h"

static int
set_cloexec_or_close(int fd)
{
    long flags;

    if (fcitx_unlikely(fd == -1))
        return -1;

    flags = fcntl(fd, F_GETFD);
    if (fcitx_unlikely(flags == -1))
        goto err;

    if (fcitx_unlikely(fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1))
        goto err;

    return fd;
err:
    close(fd);
    return -1;
}

int
fx_epoll_create_cloexec()
{
    int fd;
#ifdef EPOLL_CLOEXEC
    fd = epoll_create1(EPOLL_CLOEXEC);
    if (fcitx_likely(fd >= 0))
        return fd;
    if (fcitx_likely(errno != EINVAL))
        return -1;
#endif
    fd = epoll_create(1);
    return set_cloexec_or_close(fd);
}

int
fx_epoll_add_task(int epoll_fd, FcitxWaylandTask *task, uint32_t events)
{
    if (fcitx_unlikely(!task->handler))
        return -1;
    struct epoll_event ep = {
        .events = events,
        .data = {
            .ptr = task
        }
    };
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, task->fd, &ep);
}

int
fx_epoll_del_task(int epoll_fd, const FcitxWaylandTask *task)
{
    /**
     * Kernel < 2.6.9 doesn't accept event=NULL, but wayland may not be able
     * to run on such an old kernel either...
     **/
    return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, task->fd, NULL);
}

int
fx_epoll_mod_task(int epoll_fd, FcitxWaylandTask *task, uint32_t events)
{
    if (fcitx_unlikely(!task->handler))
        return -1;
    struct epoll_event ep = {
        .events = events,
        .data = {
            .ptr = task
        }
    };
    return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, task->fd, &ep);
}

int
fx_epoll_dispatch(int epoll_fd)
{
    struct epoll_event ep[16];
    int i;
    int count = epoll_wait(epoll_fd, ep, sizeof(ep) / sizeof(ep[0]), 0);
    FcitxWaylandTask *task;
    for (i = 0;i < count;i++) {
        task = ep[i].data.ptr;
        task->handler(task, ep[i].events);
    }
    return count;
}
