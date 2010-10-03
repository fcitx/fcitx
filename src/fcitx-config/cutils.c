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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "tools/tools.h"
#include "fcitx-config/cutils.h"
#include "fcitx-config/sprintf.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <iconv.h>

static iconv_t iconvW = NULL;

/** 
 * @brief 返回申请后的内存，并清零
 * 
 * @param 申请的内存长度
 * 
 * @return 申请的内存指针
 */
void *malloc0(size_t bytes)
{
    void *p = malloc(bytes);
    if (!p)
        return NULL;

    memset(p, 0, bytes);
    return p;
}

/** 
 * @brief 去除字符串首末尾空白字符
 * 
 * @param s
 * 
 * @return malloc的字符串，需要free
 */
char *trim(char *s)
{
    register char *end;
    register char csave;
    
    while (isspace(*s))                 /* skip leading space */
        ++s;
    end = strchr(s,'\0') - 1;
    while (end >= s && isspace(*end))               /* skip trailing space */
        --end;
    
    csave = end[1];
    end[1] = '\0';
    s = strdup(s);
    end[1] = csave;
    return (s);
}

/** 
 * @brief Fcitx记录Log的函数
 * 
 * @param ErrorLevel
 * @param fmt
 * @param ...
 */
void FcitxLogFunc(ErrorLevel e, const char* filename, const int line, const char* fmt, ...)
{
#ifndef _DEBUG
    if (e == DEBUG)
        return;
#endif
    switch (e)
    {
        case INFO:
            fprintf(stderr, "Info:");
            break;
        case ERROR:
            fprintf(stderr, "Error:");
            break;
        case DEBUG:
            fprintf(stderr, "Debug:");
            break;
        case WARNING:
            fprintf(stderr, "Warning:");
            break;
        case FATAL:
            fprintf(stderr, "Fatal:");
            break;
    }

    char *buffer;
    va_list ap;
    fprintf(stderr, "%s:%u-", filename, line);
    va_start(ap, fmt);
    vasprintf(&buffer, fmt, ap);
    va_end(ap);

    if (iconvW == NULL)
        iconvW = iconv_open("WCHAR_T", "utf-8");

    if (iconvW == (iconv_t) -1)
    {
        fprintf(stderr, "%s\n", buffer);
    }
    else
    {
        size_t len = strlen(buffer);
        wchar_t *wmessage;
        size_t wlen = (len + 1) * sizeof (wchar_t);
        wmessage = (wchar_t *) malloc0 ((len + 1) * sizeof (wchar_t));

        char *inp = buffer;
        char *outp = (char*) wmessage;
        
        iconv(iconvW, &inp, &len, &outp, &wlen);
        
        fprintf(stderr, "%ls\n", wmessage);

        free(wmessage);
    }

    free(buffer);
}
