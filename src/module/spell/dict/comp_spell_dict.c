/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <endian.h>
#include <string.h>

#define DICT_BIN_MAGIC "FSCD0000"
const char null_byte = '\0';

static int
compile_dict(int ifd, int ofd)
{
    struct stat istat_buf;
    off_t iflen;
    off_t off = 0;
    uint32_t wcount = 0;
    char *icontent;
    if (fstat(ifd, &istat_buf) == -1 ||
        (iflen = istat_buf.st_size) <= 3) {
        return 1;
    }
    icontent = mmap(NULL, iflen + 1, PROT_READ, MAP_PRIVATE, ifd, 0);
    write(ofd, DICT_BIN_MAGIC, strlen(DICT_BIN_MAGIC));
    lseek(ofd, sizeof(uint32_t), SEEK_CUR);
    while (off < iflen) {
        char *p;
        char *end;
        long int ceff;
        uint16_t ceff_buff;
        ceff = strtol(icontent + off, &p, 10);
        if (icontent + off == p || *p != ' ')
            return 1;
        if (ceff > UINT16_MAX)
            ceff = UINT16_MAX;
        ceff_buff = htole16(ceff);
        write(ofd, &ceff_buff, sizeof(uint16_t));
        p++;
        end = strchrnul(p, '\n');
        write(ofd, p, end - p);
        write(ofd, &null_byte, 1);
        off = p + 1 - icontent;
    }
    lseek(ofd, strlen(DICT_BIN_MAGIC), SEEK_SET);
    wcount = htole32(wcount);
    write(ofd, &wcount, sizeof(uint32_t));
    return 0;
}


int
main(int argc, char *argv[])
{
    int ifd;
    int ofd;
    const char *ifname;
    const char *ofname;
    if (argc != 3)
        exit(1);
    ifname = argv[0];
    ofname = argv[1];
    ifd = open(ifname, O_RDONLY);
    ofd = open(ofname, O_WRONLY | O_CREAT, 0644);
    if (ifd < 0 || ofd < 0)
        return 1;
    return compile_dict(ifd, ofd);
}
