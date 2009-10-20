/*
 * =====================================================================================
 *
 *       Filename:  
 *
 *    Description:  
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


static const char socketfile[]="/tmp/fcitx.socket";

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



int main ( int argc, char *argv[] )
{
	int socket_fd = create_socket(socketfile);
	if (socket_fd < 0) {
		fprintf(stderr, "Can't open socket %s: %s\n", socketfile, strerror(errno));
		return ;
	}

	if (argc != 2) {
		int o = 0;
		write(socket_fd, &o, sizeof(o));
		int buf;
		int len = read(socket_fd, &buf, sizeof(buf));

		printf("%d\n", buf);

		close(socket_fd);
	} else {
		int o = 1;
		if (strcasecmp("-o", argv[1]) == 0)
			o |= (1 << 16);
		else if (strcasecmp("-c", argv[1]) != 0) {
			close(socket_fd);
			return 0;
		}
			
		write(socket_fd, &o, sizeof(o));
		close(socket_fd);
	}

	return 0;
}				/* ----------  end of function main  ---------- */

