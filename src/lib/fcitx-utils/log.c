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

#include <iconv.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#include "config.h"
#include "fcitx/fcitx.h"
#include "utf8.h"
#include "log.h"
#include "utils.h"

static iconv_t iconvW = NULL;
static int init = 0;
static int is_utf8 = 0;

FCITX_EXPORT_API
void FcitxLogFunc(ErrorLevel e, const char* filename, const int line, const char* fmt, ...)
{
    if (!init) {
        init = 1;
        is_utf8 = fcitx_utils_current_locale_is_utf8();
    }

#ifndef _DEBUG
    if (e == DEBUG)
        return;
#endif
    switch (e) {
    case INFO:
        fprintf(stderr, "[INFO]");
        break;
    case ERROR:
        fprintf(stderr, "[ERROR]");
        break;
    case DEBUG:
        fprintf(stderr, "[DEBUG]");
        break;
    case WARNING:
        fprintf(stderr, "[WARN]");
        break;
    case FATAL:
        fprintf(stderr, "[FATAL]");
        break;
    }

    char *buffer = NULL;
    va_list ap;
    fprintf(stderr, " %s:%u-", filename, line);
    va_start(ap, fmt);
    vasprintf(&buffer, fmt, ap);
    va_end(ap);

    if (is_utf8) {
        fprintf(stderr, "%s\n", buffer);
        return;
    }

    if (iconvW == NULL)
        iconvW = iconv_open("WCHAR_T", "utf-8");

    if (iconvW == (iconv_t) - 1) {
        fprintf(stderr, "%s\n", buffer);
    } else {
        size_t len = strlen(buffer);
        wchar_t *wmessage = NULL;
        size_t wlen = (len) * sizeof(wchar_t);
        wmessage = (wchar_t *) fcitx_utils_malloc0((len + 10) * sizeof(wchar_t));

        IconvStr inp = buffer;
        char *outp = (char*) wmessage;

        iconv(iconvW, &inp, &len, &outp, &wlen);

        fprintf(stderr, "%zu %zu", fcitx_utf8_strlen(buffer), wcslen(wmessage));

        fprintf(stderr, "%ls\n", wmessage);

        free(wmessage);
    }

    free(buffer);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
