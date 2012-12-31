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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
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

#define MAX_IMNAME_LEN 30

static void* RemoteCreate(FcitxInstance* instance);
static void RemoteProcessEvent(void* arg);
static void RemoteSetFD(void* arg);
static void RemoteDestroy(void* arg);
static int CreateSocket(const char *name);

FCITX_DEFINE_PLUGIN(fcitx_remote, module, FcitxModule) = {
    RemoteCreate,
    RemoteSetFD,
    RemoteProcessEvent,
    RemoteDestroy,
    NULL
};

typedef struct _FcitxRemote {
    FcitxInstance* owner;
    int socket_fd;
} FcitxRemote;

void* RemoteCreate(FcitxInstance* instance)
{
    FcitxRemote* remote = fcitx_utils_malloc0(sizeof(FcitxRemote));
    remote->owner = instance;

    char *socketfile;
    asprintf(&socketfile, "/tmp/fcitx-socket-:%d", fcitx_utils_get_display_number());
    remote->socket_fd = CreateSocket(socketfile);
    if (remote->socket_fd < 0) {
        FcitxLog(ERROR, _("Can't open socket %s: %s"), socketfile, strerror(errno));
        free(remote);
        free(socketfile);
        return NULL;
    }

    fcntl(remote->socket_fd, F_SETFD, FD_CLOEXEC);
    fcntl(remote->socket_fd, F_SETFL, O_NONBLOCK);
    chmod(socketfile, 0600);
    free(socketfile);
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

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*) &opt, sizeof(opt));

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
    FcitxContextState r = FcitxInstanceGetCurrentState(remote->owner);
    write(fd, &r, sizeof(r));
}

static void RemoteProcessEvent(void* p)
{
    FcitxRemote* remote = (FcitxRemote*) p;
    unsigned int O;
    // lower 16bit 0->get 1->set, 2->reload, 3->toggle, 4->setIM
    int client_fd = UdAccept(remote->socket_fd);
    if (client_fd < 0)
        return;
    read(client_fd, &O, sizeof(int));
    unsigned int cmd = O & 0xFFFF;
    unsigned int arg = (O >> 16) & 0xFFFF;
    switch (cmd) {
        /// {{{
    case 0:
        SendIMState(remote, client_fd);
        break;
    case 1:
        /* arg is not state, due to backward compatible */
        if (arg == 0)
            FcitxInstanceCloseIM(remote->owner, FcitxInstanceGetCurrentIC(remote->owner));
        else {
            FcitxInstanceEnableIM(remote->owner, FcitxInstanceGetCurrentIC(remote->owner), false);
        }
        break;
    case 2:
        FcitxInstanceReloadConfig(remote->owner);
        break;
    case 3:
        FcitxInstanceChangeIMState(remote->owner, FcitxInstanceGetCurrentIC(remote->owner));
        break;
    case 4: {
        char imname[MAX_IMNAME_LEN];
        int n = read(client_fd, imname, MAX_IMNAME_LEN-1);
        imname[n] = '\0';
        FcitxInstanceSwitchIMByName(remote->owner, imname);
        break;
    }
    default:
        break;
        /// }}}
    }
    close(client_fd);
}

void RemoteSetFD(void* arg)
{
    FcitxRemote* remote = (FcitxRemote*) arg;
    FD_SET(remote->socket_fd, FcitxInstanceGetReadFDSet(remote->owner));
    if (FcitxInstanceGetMaxFD(remote->owner) < remote->socket_fd)
        FcitxInstanceSetMaxFD(remote->owner, remote->socket_fd);
}

void RemoteDestroy(void* arg)
{
    FcitxRemote* remote = (FcitxRemote*) arg;
    close(remote->socket_fd);
    free(remote);
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
