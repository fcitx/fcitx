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

#ifdef HAVE_CONFIG_H
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
#include <limits.h>
#include "tools/tools.h"
#include "core/xim.h"
#include "ui/MainWindow.h"
#include "ui/TrayWindow.h"
#include "ui/font.h"
#include "fcitx-config/configfile.h"
#include "fcitx-config/cutils.h"
#include "im/pinyin/sp.h"
#include "im/special/QuickPhrase.h"
#include "im/special/AutoEng.h"
#include "im/special/punc.h"
#include "ui/AboutWindow.h"
#ifdef _ENABLE_DBUS
#include "interface/DBus.h"
extern Property state_prop;
#endif
extern Display* dpy;
char socketfile[PATH_MAX]="";
CARD16 g_last_connect_id;

int create_socket(const char *name)
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


int ud_accept(int listenfd)
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

static void send_ime_state(int fd)
{
	IME_STATE r = ConnectIDGetState(g_last_connect_id);
	write(fd, &r, sizeof(r));
}


extern void DisplayMainWindow (void);
static void main_loop (int socket_fd)
{
	unsigned int O;  // 低16位, 0 = get, 1 = set;
			// 高16位, 只用于 set, 0 关闭输入法, 1 打开输入法.
	for (;;) {
		int client_fd = ud_accept(socket_fd);
		read(client_fd, &O, sizeof(int));
        unsigned int cmd = O & 0xFFFF;
        unsigned int arg = (O >> 16) & 0xFFFF;
        FcitxLock();
        switch (cmd)
        {
            /// {{{
            case 0:
                send_ime_state(client_fd);
                break;
            case 1:
                SetIMState(arg);
                if (!fc.bUseDBus) {
                    DisplayMainWindow();
                    DrawMainWindow();
                }
#ifdef _ENABLE_DBUS
                if (fc.bUseDBus)
                    updateProperty(&state_prop);
#endif
#ifdef _ENABLE_TRAY
                if (!fc.bUseDBus) {
                    if (ConnectIDGetState (g_last_connect_id) == IS_CHN)
                        DrawTrayWindow (ACTIVE_ICON, 0, 0, tray.size, tray.size );
                    else
                        DrawTrayWindow (INACTIVE_ICON, 0, 0, tray.size, tray.size );
                }
#endif
                break;
            case 2:
                ReloadConfig();
                break;
            default:
                break;
            /// }}}
		}
        FcitxUnlock();
		close(client_fd);
	}
}

void* remoteThread (void* val)
{
	sprintf(socketfile, "/tmp/fcitx-socket-%s", DisplayString(dpy));
	int socket_fd = create_socket(socketfile);
	if (socket_fd < 0) {
		FcitxLog(ERROR, _("Can't open socket %s: %s"), socketfile, strerror(errno));
		return 0;
	}

	fcntl(socket_fd, F_SETFD, FD_CLOEXEC);
	chmod(socketfile, 0666);
	main_loop(socket_fd);
	close(socket_fd);
	return 0;
}				/* ----------  end of function main  ---------- */

