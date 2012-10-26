/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
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

#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "im/pinyin/py.h"
#include "pyTools.h"
#include "fcitx-config/xdg.h"

void usage();

int main(int argc, char **argv)
{
    FILE *fi;
    int i, PYFACount;
    char *pybase_mb = NULL;

    struct _HZMap *HZMap;
    char c;

    while ((c = getopt(argc, argv, "b:h")) != -1) {
        switch (c) {

        case 'b':
            pybase_mb = strdup(optarg);
            break;

        case 'h':

        default:
            usage();
        }
    }

    if (pybase_mb)
        fi = fopen(pybase_mb , "r");
    else
        fi = FcitxXDGGetFileWithPrefix("pinyin", PY_BASE_FILE, "r", &pybase_mb);

    if (!fi) {
        perror("fopen");
        fprintf(stderr, "Can't open file `%s' for reading\n", pybase_mb);
        exit(1);
    }

    free(pybase_mb);

    PYFACount = LoadPYBase(fi, &HZMap);

    if (PYFACount > 0) {
#if 0

        for (i = 0; i < PYFACount; ++i) {
            printf("%s: ", HZMap[i].Map);
            fwrite(HZMap[i].HZ, 2, HZMap[i].BaseCount, stdout);
            printf("\n\n");
        }

#else
        for (i = 0; i < PYFACount; ++i) {
            int j;
            printf("%s: HZ Index\n", HZMap[i].Map);

            for (j = 0; j < HZMap[i].BaseCount; ++j) {
                printf("\t%s %5d", HZMap[i].HZ[j], HZMap[i].Index[j]);
            }

            printf("\n");
        }

#endif
    }

    return 0;
}

void usage()
{
    char* pkgdatadir = fcitx_utils_get_fcitx_path("pkgdatadir");
    printf(
        "readPYBase - read pybase.mb file and display its contents\n"
        "\n"
        "  usage: readPYBase [OPTION]\n"
        "\n"
        "  -b <pybase.mb> full path to the file, usually\n"
        "                 %s/pinyin/" PY_BASE_FILE "\n"
        "                 if not specified, defaults to\n"
        "                 %s/pinyin/" PY_BASE_FILE "\n"
        "  -h             display this help\n"
        "\n",
        pkgdatadir, pkgdatadir
    );
    free(pkgdatadir);
    exit(1);
    return;
}


// kate: indent-mode cstyle; space-indent on; indent-width 4;
