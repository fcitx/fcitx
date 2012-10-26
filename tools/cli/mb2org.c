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
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include "im/pinyin/pyParser.h"
#include "im/pinyin/pyMapTable.h"
#include "im/pinyin/PYFA.h"
#include "im/pinyin/sp.h"
#include "pyTools.h"
#include "fcitx-config/xdg.h"
#include "im/pinyin/pyconfig.h"
#include "im/pinyin/py.h"

FcitxPinyinConfig pyconfig;

/* Bad programming practice :( */
boolean bSingleHZMode;

void usage();
char *HZToPY(struct _HZMap *, char []);

int main(int argc, char **argv)
{
    FILE *fi, *fi2;
    int i, j, k;
    char *pyusrphrase_mb = NULL, *pybase_mb = NULL, *HZPY, tMap[3], tPY[10];

    struct _HZMap *HZMap;

    struct _PYMB *PYMB;
    char c;
    boolean isUser = true;

    while ((c = getopt(argc, argv, "f:b:sh")) != -1) {
        switch (c) {

        case 'f':
            pyusrphrase_mb = strdup(optarg);
            break;

        case 'b':
            pybase_mb = strdup(optarg);
            break;

        case 's':
            isUser = false;
            break;

        case 'h':

        default:
            usage();
        }
    }

    if (pyusrphrase_mb)
        fi = fopen(pyusrphrase_mb , "r");
    else
        fi = FcitxXDGGetFileUserWithPrefix("pinyin", PY_USERPHRASE_FILE, "r" , &pyusrphrase_mb);

    if (!fi) {
        perror("fopen");
        fprintf(stderr, "Can't open file `%s' for reading\n", pyusrphrase_mb);
        exit(1);
    }

    free(pyusrphrase_mb);

    if (pybase_mb)
        fi2 = fopen(pybase_mb , "r");
    else
        fi2 = FcitxXDGGetFileWithPrefix("pinyin", PY_BASE_FILE, "r", &pybase_mb);

    if (!fi2) {
        perror("fopen");
        fprintf(stderr, "Can't open file `%s' for reading\n", pybase_mb);
        exit(1);
    }

    free(pybase_mb);


    LoadPYMB(fi, &PYMB, isUser);
    LoadPYBase(fi2, &HZMap);

    for (i = 0; PYMB[i].HZ[0]; ++i) {
        for (j = 0; j < PYMB[i].UserPhraseCount; ++j) {
            HZPY = HZToPY(&(HZMap[PYMB[i].PYFAIndex]), PYMB[i].HZ);
            printf("%s", HZPY);

            for (k = 0; k < PYMB[i].UserPhrase[j].Length / 2; ++k) {
                memcpy(tMap, PYMB[i].UserPhrase[j].Map + 2 * k, 2);
                tMap[2] = '\0';
                tPY[0] = '\0';

                if (!MapToPY(tMap, tPY))
                    strcpy(tPY, "'*");

                printf("'%s", tPY);
            }

            printf(" %s%s\n", PYMB[i].HZ, PYMB[i].UserPhrase[j].Phrase);

            free(HZPY);
        }
    }

    return 0;
}

/*
  This function takes a HanZi (HZ) and returns a PinYin (PY) string.
  If no match is found, "*" is returned.
*/

char *HZToPY(struct _HZMap *pHZMap1, char* HZ)
{
    int i;
    char Map[3], tPY[10];

    Map[0] = '\0';

    for (i = 0; i < pHZMap1->BaseCount; ++i)
        if (strcmp(HZ, pHZMap1->HZ[i]) == 0) {
            strcpy(Map, pHZMap1->Map);
            break;
        }

    if (!Map[0] || !MapToPY(Map, tPY))
        strcpy(tPY, "*");

    return strdup(tPY);
}

void usage()
{
    char* pkgdatadir = fcitx_utils_get_fcitx_path("pkgdatadir");
    printf(
        "mb2org - Convert .mb file to .org file (SEE NOTES BELOW)\n"
        "\n"
        "  usage: mb2org [OPTION]\n"
        "\n"
        "  -f <pyusrphrase.mb> this is the .mb file to be decoded, usually this is\n"
        "                      ~/.fcitx/" PY_USERPHRASE_FILE "\n"
        "                      if not specified, defaults to\n"
        "                      ~/.fcitx/" PY_USERPHRASE_FILE "\n"
        "  -b <pybase.mb>      this is the pybase.mb file used to determine the\n"
        "                      of the first character in HZ. Usually, this is\n"
        "                      %s/pinyin/" PY_BASE_FILE "\n"
        "                      if not specified, defaults to\n"
        "                      %s/pinyin/" PY_BASE_FILE "\n"
        "  -s                  Is MB from user or from system (they have different format).\n"
        "  -h                  display this help\n"
        "\n"
        "NOTES:\n"
        "1. If no match is found for a particular HZ, then the pinyin for that HZ\n"
        "   will be `*'.\n"
        "2. Always check the produced output for errors.\n",
        pkgdatadir, pkgdatadir
    );
    free(pkgdatadir);
    exit(1);
    return;
}


// kate: indent-mode cstyle; space-indent on; indent-width 4;
