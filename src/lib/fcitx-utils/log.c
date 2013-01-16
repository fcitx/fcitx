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
#include <wchar.h>

#include "config.h"
#include "fcitx/fcitx.h"
#include "utf8.h"
#include "log.h"
#include "utils.h"

static iconv_t iconvW = NULL;
static int init = 0;
static int is_utf8 = 0;

#ifndef _DEBUG
static FcitxLogLevel errorLevel = FCITX_WARNING;
#else
static FcitxLogLevel errorLevel = FCITX_DEBUG;
#endif

static const int RealLevelIndex[] = {0, 2, 3, 4, 1, 6};

FCITX_EXPORT_API
void FcitxLogSetLevel(FcitxLogLevel e) {
    if ((int) e < 0)
        e = 0;
    else if (e > FCITX_NONE)
        e = FCITX_NONE;
    errorLevel = e;
}

FcitxLogLevel FcitxLogGetLevel()
{
    return errorLevel;
}


FCITX_EXPORT_API void
FcitxLogFuncV(FcitxLogLevel e, const char* filename, const int line,
              const char* fmt, va_list ap)
{
    if (!init) {
        init = 1;
        is_utf8 = fcitx_utils_current_locale_is_utf8();
    }

    if ((int) e < 0) {
        e = 0;
    } else if (e >= FCITX_NONE) {
        e = FCITX_INFO;
    }

    int realLevel = RealLevelIndex[e];
    int globalRealLevel = RealLevelIndex[errorLevel];

    if (realLevel < globalRealLevel)
        return;

    switch (e) {
    case FCITX_INFO:
        fprintf(stderr, "(INFO-");
        break;
    case FCITX_ERROR:
        fprintf(stderr, "(ERROR-");
        break;
    case FCITX_DEBUG:
        fprintf(stderr, "(DEBUG-");
        break;
    case FCITX_WARNING:
        fprintf(stderr, "(WARN-");
        break;
    case FCITX_FATAL:
        fprintf(stderr, "(FATAL-");
        break;
    default:
        break;
    }

    char *buffer = NULL;
    fprintf(stderr, "%d %s:%u) ", getpid(), filename, line);
    vasprintf(&buffer, fmt, ap);

    if (is_utf8) {
        fprintf(stderr, "%s\n", buffer);
        free(buffer);
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
        wmessage = (wchar_t *)fcitx_utils_malloc0((len + 10) * sizeof(wchar_t));

        IconvStr inp = buffer;
        char *outp = (char*) wmessage;

        iconv(iconvW, &inp, &len, &outp, &wlen);

        fprintf(stderr, "%ls\n", wmessage);
        free(wmessage);
    }
    free(buffer);
}

FCITX_EXPORT_API void
FcitxLogFunc(FcitxLogLevel e, const char* filename, const int line,
             const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    FcitxLogFuncV(e, filename, line, fmt, ap);
    va_end(ap);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
