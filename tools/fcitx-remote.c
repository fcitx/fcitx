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

#ifdef FCITX_HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <limits.h>
#include "fcitx/frontend.h"
#include "fcitx-utils/utils.h"

int create_socket(const char *name)
{
    int fd;
    int r;

    struct sockaddr_un uds_addr;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd < 0) {
        return fd;
    }

    /* setup address struct */
    memset(&uds_addr, 0, sizeof(uds_addr));

    uds_addr.sun_family = AF_UNIX;

    strcpy(uds_addr.sun_path, name);

    r = connect(fd, (struct sockaddr *) & uds_addr, sizeof(uds_addr));

    if (r < 0) {
        return r;
    }

    return fd;
}

void usage()
{
    printf("Usage: fcitx-remote [OPTION]\n"
           "\t-c\t\tclose input method\n"
           "\t-o\t\topen input method\n"
           "\t-r\t\treload fcitx config\n"
           "\t[no option]\tdisplay fcitx state, %d for close, %d for english, %d for chinese\n"
           "\t-h\t\tdisplay this help and exit\n",
           IS_CLOSED, IS_ENG, IS_ACTIVE);
}

int main(int argc, char *argv[])
{
    char *socketfile = NULL;
    int socket_fd;

    int o = 0;
    char c;

    while ((c = getopt(argc, argv, "chor")) != -1) {
        switch (c) {

        case 'o':
            o = 1;
            o |= (1 << 16);
            break;

        case 'c':
            o = 1;
            break;

        case 'r':
            o = 2;
            break;

        case 'h':

        default:
            usage();
            return 0;
            break;
        }
    }

    asprintf(&socketfile, "/tmp/fcitx-socket-:%d", fcitx_utils_get_display_number());

    socket_fd = create_socket(socketfile);

    if (socket_fd < 0) {
        fprintf(stderr, "Can't open socket %s: %s\n", socketfile, strerror(errno));
        free(socketfile);
        return 1;
    }
    free(socketfile);

    if (o == 0) {
        write(socket_fd, &o, sizeof(o));
        int buf;
        read(socket_fd, &buf, sizeof(buf));
        printf("%d\n", buf);
        close(socket_fd);
    } else {
        write(socket_fd, &o, sizeof(o));
        close(socket_fd);
    }

    return 0;
}               /* ----------  end of function main  ---------- */


// kate: indent-mode cstyle; space-indent on; indent-width 4;
