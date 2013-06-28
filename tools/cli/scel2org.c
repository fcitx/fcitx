/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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
#include <iconv.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "config.h"

#include "fcitx-utils/utils.h"
#include "fcitx-utils/utarray.h"

#define HEADER_SIZE 12
#define BUFLEN 0x1000

#define DESC_START 0x130
#define DESC_LENGTH (0x338 - 0x130)

#define LDESC_LENGTH (0x540 - 0x338)
#define NEXT_LENGTH (0x1540 - 0x540)

#define PINYIN_SIZE 4

char header_str[HEADER_SIZE] = { '\x40', '\x15', '\0', '\0', '\x44', '\x43', '\x53' , '\x01' , '\x01', '\0', '\0', '\0'};
char pinyin_str[PINYIN_SIZE] = { '\x9d', '\x01', '\0', '\0' };
iconv_t conv;

typedef struct _ScelPinyin {
    char pinyin[10];
} ScelPinyin;

UT_icd py_icd = { sizeof(ScelPinyin), NULL, NULL, NULL};
static void usage();

void usage()
{
    puts(
        "scel2org - Convert .scel file to .org file (SEE NOTES BELOW)\n"
        "\n"
        "  usage: scel2org [OPTION] [scel file]\n"
        "\n"
        "  -a            use alternative order, output hanzi first, then pinyin\n"
        "  -o <file.org> specify the output file, if not specified, the output will\n"
        "                be stdout.\n"
        "  -h            display this help.\n"
        "\n"
        "NOTES:\n"
        "   Always check the produced output for errors.\n"
    );
    exit(1);
}

int main(int argc, char **argv)
{
    FILE *fout = stdout;
    char c;

    boolean alternativeOrder = false;

    while ((c = getopt(argc, argv, "ao:h")) != -1) {
        switch (c) {
        case 'a':
            alternativeOrder = true;
            break;
        case 'o':
            fout = fopen(optarg, "w");

            if (!fout) {
                fprintf(stderr, "Cannot open file: %s\n", optarg);
                return 1;
            }

            break;

        case 'h':

        default:
            usage();

            break;
        }
    }

    if (optind >= argc) {
        usage();
        return 1;
    }

    FILE *fp = fopen(argv[optind], "r");
    if (!fp) {
        fprintf(stderr, "Cannot open file: %s\n", argv[optind]);
        return 1;
    }

    char buf[BUFLEN], bufout[BUFLEN];

    size_t count = fread(buf, 1, HEADER_SIZE, fp);

    if (count < HEADER_SIZE || memcmp(buf, header_str, HEADER_SIZE) != 0) {
        fprintf(stderr, "format error.\n");
        fclose(fp);
        return 1;
    }

    conv = iconv_open("utf-8", "unicode");
    if (conv == (iconv_t) -1) {
        fprintf(stderr, "iconv error.\n");
        return 1;
    }

    fseek(fp, DESC_START, SEEK_SET);
    fread(buf, 1, DESC_LENGTH, fp);
    IconvStr in = buf;
    char *out = bufout;
    size_t inlen = DESC_LENGTH, outlen = BUFLEN;
    iconv(conv, &in, &inlen, &out, &outlen);
    fprintf(stderr, "%s\n", bufout);

    fread(buf, 1, LDESC_LENGTH, fp);
    in = buf;
    out = bufout;
    inlen = LDESC_LENGTH;
    outlen = BUFLEN;
    iconv(conv, &in, &inlen, &out, &outlen);
    fprintf(stderr, "%s\n", bufout);

    fread(buf, 1, NEXT_LENGTH, fp);
    in = buf;
    out = bufout;
    inlen = NEXT_LENGTH;
    outlen = BUFLEN;
    iconv(conv, &in, &inlen, &out, &outlen);
    fprintf(stderr, "%s\n", bufout);

    count = fread(buf, 1, PINYIN_SIZE, fp);

    if (count < PINYIN_SIZE || memcmp(buf, pinyin_str, PINYIN_SIZE) != 0) {
        fprintf(stderr, "format error.\n");
        fclose(fp);
        return 1;
    }

    UT_array pys;
    utarray_init(&pys, &py_icd);

    for (; ;) {
        int16_t index;
        int16_t count;
        fread(&index, 1, sizeof(int16_t), fp);
        fread(&count, 1, sizeof(int16_t), fp);
        ScelPinyin py;
        memset(buf, 0, sizeof(buf));
        memset(&py, 0, sizeof(ScelPinyin));
        fread(buf, count, sizeof(char), fp);

        in = buf;
        out = py.pinyin;
        inlen = count * sizeof(char);
        outlen = 10;
        iconv(conv, &in, &inlen, &out, &outlen);

        utarray_push_back(&pys, &py);

        if (strcmp(py.pinyin, "zuo") == 0)
            break;
    }

    while (!feof(fp)) {
        int16_t symcount;
        int16_t count;
        int16_t wordcount;
        fread(&symcount, 1, sizeof(int16_t), fp);

        if (feof(fp))
            break;

        fread(&count, 1, sizeof(int16_t), fp);

        wordcount = count / 2;
        int16_t* pyindex = malloc(sizeof(int16_t) * wordcount);

        fread(pyindex, wordcount, sizeof(int16_t), fp);

        int s;

        for (s = 0; s < symcount ; s++) {

            memset(buf, 0, sizeof(buf));

            memset(bufout, 0, sizeof(bufout));
            fread(&count, 1, sizeof(int16_t), fp);
            fread(buf, count, sizeof(char), fp);
            in = buf;
            out = bufout;
            inlen = count * sizeof(char);
            outlen = BUFLEN;
            iconv(conv, &in, &inlen, &out, &outlen);

            if (alternativeOrder) {
                fprintf(fout, "%s ", bufout);
            }

            ScelPinyin *py = (ScelPinyin*)utarray_eltptr(
                &pys, (unsigned int)pyindex[0]);
            fprintf(fout, "%s",  py->pinyin);
            int i;

            for (i = 1 ; i < wordcount ; i ++) {
                py = (ScelPinyin*)utarray_eltptr(&pys,
                                                 (unsigned int)pyindex[i]);
                fprintf(fout, "\'%s", py->pinyin);
            }

            if (!alternativeOrder) {
                fprintf(fout, " %s", bufout);
            }
            fprintf(fout, "\n");

            fread(&count, 1, sizeof(int16_t), fp);
            fread(buf, count, sizeof(char), fp);
        }

        free(pyindex);
    }

    iconv_close(conv);

    utarray_done(&pys);
    fclose(fout);
    fclose(fp);
    return 0;
}

// kate: indent-mode cstyle; space-indent on; indent-width 4;
