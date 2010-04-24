/*
 * =====================================================================================
 *
 *       Filename:  client-test.c
 *
 *    Description:  use socket to check and change the fcitx state
 *
 *        Version:  1.0
 *        Created:  2009年10月14日 17时37分57秒
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
#include <poll.h>
#include <limits.h>
#include <X11/Xlib.h>
#include "xim.h"

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

	r = connect(fd, (struct sockaddr *)&uds_addr, sizeof(uds_addr));
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
           "\t[no option]\tdisplay fcitx state, %d for close, %d for english, %d for chinese\n"
		   "\t-h\t\tdisplay this help and exit\n",
		   IS_CLOSED, IS_ENG, IS_CHN);
}

int main ( int argc, char *argv[] )
{
    char socketfile[PATH_MAX] = "";
	int socket_fd;

    int o = 0;
	char c;
	while((c = getopt(argc, argv, "cho")) != -1) {
		switch (c) {
		case 'o':
            o = 1;
            o |= (1 << 16);
            break;
        case 'c':
            o = 1;
            break;
        case 'h':
        default:
            usage();
            return 0;
            break;
		}
	}

	sprintf(socketfile, "/tmp/fcitx-soeckt-%s", XDisplayName(NULL));
	socket_fd = create_socket(socketfile);
	if (socket_fd < 0) {
		fprintf(stderr, "Can't open socket %s: %s\n", socketfile, strerror(errno));
		return 0;
	}

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
}				/* ----------  end of function main  ---------- */

