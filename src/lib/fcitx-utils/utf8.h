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

#ifndef _FCITX_UTF8_H_
#define _FCITX_UTF8_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UTF8_MAX_LENGTH 6

#define ISUTF8_CB(c)  (((c)&0xc0) == 0x80)

    size_t fcitx_utf8_strlen(const char *s);
    char*  fcitx_utf8_get_char(const char *in, int *chr);
    int    fcitx_utf8_strncmp(const char *s1, const char *s2, int n);
    int    fcitx_utf8_char_len(const char *in);
    char*  fcitx_utf8_get_nth_char(char* s, unsigned int n);
    int    fcitx_utf8_check_string(const char *s);
    int    fcitx_utf8_get_char_extended(const char *p, int max_len);
    int    fcitx_utf8_get_char_validated(const char *p, int max_len);

#ifdef __cplusplus
}
#endif

#endif /* ifndef UTF8_H */


// kate: indent-mode cstyle; space-indent on; indent-width 0;
