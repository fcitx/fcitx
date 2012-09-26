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
#ifdef FCITX_HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include "im/table/tabledict.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#define MAX_CODE_LENGTH 30

enum {
    TEMPL_VERNEW = 0,
    TEMPL_VEROLD,
    TEMPL_KEYCODE,
    TEMPL_LEN,
    TEMPL_PY,
    TEMPL_PYLEN,
    TEMPL_PROMPT,
    TEMPL_CONSTRUCTPHRASE,
    TEMPL_INVALIDCHAR,
    TEMPL_RULE,
    TEMPL_DATA,
    TEMPL_PROMPT2,
    TEMPL_CONSTRUCTPHRASE2,
};

const char* templateOld[] = {
    ";fcitx 版本 0x%02x 码表文件\n",
    ";fcitx 版本 0x02 码表文件\n",
    "键码=%s\n",
    "码长=%d\n",
    "拼音=%c\n",
    "拼音长度=%d\n",
    "提示=%c\n",
    "构词=%c\n",
    "规避字符=%s\n",
    "[组词规则]\n",
    "[数据]\n",
    "提示=\n",
    "构词=\n",
};
const char* templateNew[] = {
    ";fcitx Version 0x%02x Table file\n",
    ";fcitx Version 0x02 Table file\n",
    "KeyCode=%s\n",
    "Length=%d\n",
    "Pinyin=%c\n",
    "PinyinLength=%d\n",
    "Prompt=%c\n",
    "ConstructPhrase=%c\n",
    "InvalidChar=%s\n",
    "[Rule]\n",
    "[Data]\n",
    "Prompt=\n",
    "ConstructPhrase=\n",
};

char guessValidChar(char prefer, const char* invalid) {
    if (!strchr(invalid, prefer))
        return prefer;
    unsigned char c = 0;
    for (c = 0; c <= 127; c ++) {
        if (ispunct(c) || isalnum(c)) {
            if (!strchr(invalid, c))
                break;
        }
    }
    if (c == 128)
        return 0;
    else
        return c;
}

void usage()
{
    printf("Usage: mb2txt [-o] <Source File>\n");
    printf("\t-o\t\tOld format table file\n\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    char            strCode[100];
    char            strHZ[UTF8_MAX_LENGTH * 31];
    FILE           *fpDict;
    unsigned int    i = 0;
    uint32_t        iTemp;
    uint32_t        j;
    unsigned char   iLen;
    unsigned char   iRule;
    unsigned char   iPYLen;
    char            iVersion = 0;
    boolean         old = false;
    const char**          templ = NULL;

    int c;
    while ((c = getopt(argc, argv, "oh")) != -1) {
        switch (c) {
        case 'o':
            old = true;
            break;
        case 'h':

        default:
            usage();
        }
    }

    templ = old ? templateOld : templateNew;

    if (optind + 1 != argc) {
        usage();
    }

    fpDict = fopen(argv[optind], "r");

    if (!fpDict) {
        printf("\nCan not read source file!\n\n");
        exit(2);
    }

    //先读取码表的信息
    fcitx_utils_read_uint32(fpDict, &iTemp);

    if (iTemp == 0) {
        fread(&iVersion, sizeof(char), 1, fpDict);
        printf(templ[TEMPL_VERNEW], iVersion);
        fcitx_utils_read_uint32(fpDict, &iTemp);
    } else
        printf("%s", templ[TEMPL_VEROLD]);

    fread(strCode, sizeof(char), iTemp + 1, fpDict);

    printf(templ[TEMPL_KEYCODE], strCode);
    char cPinyin = '\0';

    fread(&iLen, sizeof(unsigned char), 1, fpDict);

    printf(templ[TEMPL_LEN], iLen);

    if (iVersion) {
        fread(&iPYLen, sizeof(unsigned char), 1, fpDict);

        if (iPYLen) {
            cPinyin = guessValidChar('@', strCode);
            printf(templ[TEMPL_PY], cPinyin);
            printf(templ[TEMPL_PYLEN], iPYLen);
        }
    }
    char* temp = malloc(strlen(strCode) * sizeof(char) + 3);
    strcpy(temp, strCode);
    char pyStr[] = {cPinyin, '\0'};
    strcat(temp, pyStr);
    char cPrompt = guessValidChar('&', temp);
    char prStr[] = {cPrompt, '\0'};
    strcat(temp, prStr);
    char cPhrase = guessValidChar('^', temp);
    free(temp);
    if (cPrompt == 0) {
        printf("%s", templ[TEMPL_PROMPT2]);
    }
    else {
        printf(templ[TEMPL_PROMPT], cPrompt);
    }
    if (cPhrase == 0) {
        printf("%s", templ[TEMPL_CONSTRUCTPHRASE2]);
    }
    else {
        printf(templ[TEMPL_CONSTRUCTPHRASE], cPhrase);
    }

    fcitx_utils_read_uint32(fpDict, &iTemp);

    fread(strCode, sizeof(char), iTemp + 1, fpDict);

    if (iTemp)
        printf(templ[TEMPL_INVALIDCHAR], strCode);

    fread(&iRule, sizeof(unsigned char), 1, fpDict);

    if (iRule) {
        //表示有组词规则
        printf("%s", templ[TEMPL_RULE]);

        for (i = 0; i < iLen - 1; i++) {
            fread(&iRule, sizeof(unsigned char), 1, fpDict);
            printf("%c", (iRule) ? 'a' : 'e');
            fread(&iRule, sizeof(unsigned char), 1, fpDict);
            printf("%d=", iRule);

            for (iTemp = 0; iTemp < iLen; iTemp++) {
                fread(&iRule, sizeof(unsigned char), 1, fpDict);
                printf("%c", (iRule) ? 'p' : 'n');
                fread(&iRule, sizeof(unsigned char), 1, fpDict);
                printf("%d", iRule);
                fread(&iRule, sizeof(unsigned char), 1, fpDict);
                printf("%d", iRule);

                if (iTemp != (iLen - 1))
                    printf("+");
            }

            printf("\n");
        }
    }

    printf("%s", templ[TEMPL_DATA]);

    fcitx_utils_read_uint32(fpDict, &j);

    if (iVersion)
        iLen = iPYLen;

    for (i = 0; i < j; i++) {
        fread(strCode, sizeof(char), iLen + 1, fpDict);
        fcitx_utils_read_uint32(fpDict, &iTemp);
        if (iTemp > UTF8_MAX_LENGTH * 30)
            break;
        fread(strHZ, sizeof(unsigned char), iTemp, fpDict);

        if (iVersion) {
            fread(&iRule, sizeof(unsigned char), 1, fpDict);

            if (iRule == RECORDTYPE_PINYIN)
                printf("%c%s %s\n", cPinyin, strCode, strHZ);
            else if (iRule == RECORDTYPE_CONSTRUCT) {
                if (cPhrase == 0) {
                    fprintf(stderr, "Could not find a valid char for construct phrase\n");
                    exit(1);
                }
                else
                    printf("%c%s %s\n", cPhrase, strCode, strHZ);
            }
            else if (iRule == RECORDTYPE_PROMPT)
                if (cPrompt == 0) {
                    fprintf(stderr, "Could not find a valid char for prompt\n");
                    exit(1);
                }
                else
                    printf("%c%s %s\n", cPrompt, strCode, strHZ);
            else
                printf("%s %s\n", strCode, strHZ);
        }

        fcitx_utils_read_uint32(fpDict, &iTemp);
        fcitx_utils_read_uint32(fpDict, &iTemp);
    }

    fclose(fpDict);

    return 0;
}

// kate: indent-mode cstyle; space-indent on; indent-width 4;
