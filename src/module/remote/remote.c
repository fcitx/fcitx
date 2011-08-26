/***************************************************************************
 *   Copyright (C) 2008~2010 by wind (xihe)                                *
 *   xihels@gmail.com                                                      *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <libintl.h>
#include <fcntl.h>
#include <limits.h>

#include "fcitx/fcitx.h"
#include "fcitx-utils/log.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"

static void* RemoteCreate(FcitxInstance* instance);
static void RemoteProcessEvent(void* arg);
static void RemoteSetFD(void* arg);
static int CreateSocket(const char *name);

FCITX_EXPORT_API
FcitxModule module = {
    RemoteCreate,
    RemoteSetFD,
    RemoteProcessEvent,
    NULL,
    NULL
};

typedef struct _FcitxRemote {
    char socketfile[PATH_MAX];
    FcitxInstance* owner;
    int socket_fd;
} FcitxRemote;

void* RemoteCreate(FcitxInstance* instance)
{
    FcitxRemote* remote = fcitx_malloc0(sizeof(FcitxRemote));
    remote->owner = instance;

    char *socketfile = remote->socketfile;
    sprintf(socketfile, "/tmp/fcitx-socket-:%d", FcitxGetDisplayNumber());
    remote->socket_fd = CreateSocket(socketfile);
    if ( remote->socket_fd < 0) {
        FcitxLog(ERROR, _("Can't open socket %s: %s"), socketfile, strerror(errno));
        free(remote);
        return NULL;
    }

    fcntl(remote->socket_fd, F_SETFD, FD_CLOEXEC);
    fcntl(remote->socket_fd, F_SETFL, O_NONBLOCK);
    chmod(socketfile, 0600);
    return remote;
}

int CreateSocket(const char *name)
{
    int fd;
    int r;
    struct sockaddr_un uds_addr;

    /* JIC */
    unlink(name);

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return fd;
    }

    /* setup address struct */
    memset(&uds_addr, 0, sizeof(uds_addr));
    uds_addr.sun_family = AF_UNIX;
    strcpy(uds_addr.sun_path, name);

    /* bind it to the socket */
    r = bind(fd, (struct sockaddr *)&uds_addr, sizeof(uds_addr));
    if (r < 0) {
        return r;
    }


    /* listen - allow 10 to queue */
    r = listen(fd, 10);
    if (r < 0) {
        return r;
    }

    return fd;
}


int UdAccept(int listenfd)
{
    for (;;) {
        int newsock = 0;
        struct sockaddr_un cliaddr;
        socklen_t len = sizeof(struct sockaddr_un);

        newsock = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
        if (newsock < 0) {
            if (errno == EINTR) {
                continue; /* signal */
            }
        }

        return newsock;
    }
}

static void SendIMState(FcitxRemote* remote, int fd)
{
    IME_STATE r = GetCurrentState(remote->owner);
    write(fd, &r, sizeof(r));
}

static void RemoteProcessEvent (void* p)
{
    FcitxRemote* remote = (FcitxRemote*) p;
    unsigned int O;  // 低16位, 0 = get, 1 = set;
    // 高16位, 只用于 set, 0 关闭输入法, 1 打开输入法.
    int client_fd = UdAccept(remote->socket_fd);
    if (client_fd < 0)
        return;
    read(client_fd, &O, sizeof(int));
    unsigned int cmd = O & 0xFFFF;
    unsigned int arg = (O >> 16) & 0xFFFF;
    FcitxLock(remote->owner);
    switch (cmd)
    {
        /// {{{
    case 0:
        SendIMState(remote, client_fd);
        break;
    case 1:
        if (arg == IS_CLOSED)
            CloseIM(remote->owner, GetCurrentIC(remote->owner));
        else
            EnableIM(remote->owner, GetCurrentIC(remote->owner), false);
        break;
    case 2:
        ReloadConfig(remote->owner);
        break;
    default:
        break;
        /// }}}
    }
    FcitxUnlock(remote->owner);
    close(client_fd);
}

void RemoteSetFD (void* arg)
{
    FcitxRemote* remote = (FcitxRemote*) arg;
    FD_SET(remote->socket_fd, &remote->owner->rfds);
    if (remote->owner->maxfd < remote->socket_fd)
        remote->owner->maxfd = remote->socket_fd;
}


// kate: indent-mode cstyle; space-indent on; indent-width 0; 
