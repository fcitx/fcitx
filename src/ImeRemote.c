/*
 * =====================================================================================
 *
 *       Filename:  ImRemote.c
 *
 *    Description:
 *
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wind (xihe), xihels@gmail.com
 *        Company:  cyclone
 *
 * =====================================================================================
 */

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
#include "xim.h"
#include "MainWindow.h"
#include "TrayWindow.h"
#ifdef _ENABLE_DBUS
#include "DBus.h"
extern Property state_prop;
#endif
extern Bool bUseDBus; 
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
	int O;  // 低16位, 0 = get, 1 = set;
			// 高16位, 只用于 set, 0 关闭输入法, 1 打开输入法.
	for (;;) {
		int client_fd = ud_accept(socket_fd);
		read(client_fd, &O, sizeof(int));
		if (!O) {
			send_ime_state(client_fd);
		} else {
			O >>= 16;
			SetIMState(O);
			if (O) {
				if (!bUseDBus) {
					DisplayMainWindow();
					DrawMainWindow();
				}
			}
#ifdef _ENABLE_DBUS
			if (bUseDBus)
				updateProperty(&state_prop);
#endif
#ifdef _ENABLE_TRAY
			if (!bUseDBus) {
				if (ConnectIDGetState (g_last_connect_id) == IS_CHN)
					DrawTrayWindow (ACTIVE_ICON, 0, 0, TRAY_ICON_HEIGHT, TRAY_ICON_WIDTH );
				else
					DrawTrayWindow (INACTIVE_ICON, 0, 0, TRAY_ICON_HEIGHT, TRAY_ICON_WIDTH );
			}
#endif
		}
		close(client_fd);
	}
}

void* remoteThread (void* val)
{
	sprintf(socketfile, "/tmp/fcitx-soeckt-%s", XDisplayName(NULL));
	int socket_fd = create_socket(socketfile);
	if (socket_fd < 0) {
		fprintf(stderr, "Can't open socket %s: %s\n", socketfile, strerror(errno));
		return 0;
	}

	fcntl(socket_fd, F_SETFD, FD_CLOEXEC);
	chmod(socketfile, 0666);
	main_loop(socket_fd);
	close(socket_fd);
	return 0;
}				/* ----------  end of function main  ---------- */

