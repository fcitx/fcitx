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

#include <string.h>
#include "fcitx/fcitx.h"
#include "fcitx-utils/utf8.h"

#define CONT(i)   ISUTF8_CB(in[i])
#define VAL(i, s) ((in[i]&0x3f) << s)

#define UTF8_LENGTH(Char)                       \
    ((Char) < 0x80 ? 1 :                        \
     ((Char) < 0x800 ? 2 :                      \
      ((Char) < 0x10000 ? 3 :                   \
       ((Char) < 0x200000 ? 4 :                 \
        ((Char) < 0x4000000 ? 5 : 6)))))

#define UNICODE_VALID(Char)                     \
    ((Char) < 0x110000 &&                       \
     (((Char) & 0xFFFFF800) != 0xD800) &&       \
     ((Char) < 0xFDD0 || (Char) > 0xFDEF) &&    \
     ((Char) & 0xFFFE) != 0xFFFE)

FCITX_EXPORT_API
size_t
fcitx_utf8_strlen(const char *s)
{
    unsigned int l = 0;

    while (*s) {
        uint32_t chr;

        s = fcitx_utf8_get_char(s, &chr);
        l++;
    }

    return l;
}

FCITX_EXPORT_API
int fcitx_utf8_char_len(const char *in)
{
    if (!(in[0] & 0x80))
        return 1;

    /* 2-byte, 0x80-0x7ff */
    if ((in[0] & 0xe0) == 0xc0 && CONT(1))
        return 2;

    /* 3-byte, 0x800-0xffff */
    if ((in[0] & 0xf0) == 0xe0 && CONT(1) && CONT(2))
        return 3;

    /* 4-byte, 0x10000-0x1FFFFF */
    if ((in[0] & 0xf8) == 0xf0 && CONT(1) && CONT(2) && CONT(3))
        return 4;

    /* 5-byte, 0x200000-0x3FFFFFF */
    if ((in[0] & 0xfc) == 0xf8 && CONT(1) && CONT(2) && CONT(3) && CONT(4))
        return 5;

    /* 6-byte, 0x400000-0x7FFFFFF */
    if ((in[0] & 0xfe) == 0xfc && CONT(1) && CONT(2) && CONT(3) && CONT(4) && CONT(5))
        return 6;

    return 1;
}

FCITX_EXPORT_API
int fcitx_ucs4_char_len(uint32_t c)
{
    if (c < 0x00080) {
        return 1;
    } else if (c < 0x00800) {
        return 2;
    } else if (c < 0x10000) {
        return 3;
    } else if (c < 0x200000) {
        return 4;
        // below is not in UCS4 but in 32bit int.
    } else if (c < 0x8000000) {
        return 5;
    } else {
        return 6;
    }
}

FCITX_EXPORT_API
int fcitx_ucs4_to_utf8(uint32_t c, char* output)
{
    if (c < 0x00080) {
        output[0] = (char) (c & 0xFF);
        output[1] = '\0';
        return 1;
    } else if (c < 0x00800) {
        output[0] = (char) (0xC0 + ((c >> 6) & 0x1F));
        output[1] = (char) (0x80 + (c & 0x3F));
        output[2] = '\0';
        return 2;
    } else if (c < 0x10000) {
        output[0] = (char) (0xE0 + ((c >> 12) & 0x0F));
        output[1] = (char) (0x80 + ((c >> 6) & 0x3F));
        output[2] = (char) (0x80 + (c & 0x3F));
        output[3] = '\0';
        return 3;
    } else if (c < 0x200000) {
        output[0] = (char) (0xF0 + ((c >> 18) & 0x07));
        output[1] = (char) (0x80 + ((c >> 12) & 0x3F));
        output[2] = (char) (0x80 + ((c >> 6)  & 0x3F));
        output[3] = (char) (0x80 + (c & 0x3F));
        output[4] = '\0';
        return 4;
        // below is not in UCS4 but in 32bit int.
    } else if (c < 0x8000000) {
        output[0] = (char) (0xF8 + ((c >> 24) & 0x03));
        output[1] = (char) (0x80 + ((c >> 18) & 0x3F));
        output[2] = (char) (0x80 + ((c >> 12) & 0x3F));
        output[3] = (char) (0x80 + ((c >> 6)  & 0x3F));
        output[4] = (char) (0x80 + (c & 0x3F));
        output[5] = '\0';
        return 5;
    } else {
        output[0] = (char) (0xFC + ((c >> 30) & 0x01));
        output[1] = (char) (0x80 + ((c >> 24) & 0x3F));
        output[2] = (char) (0x80 + ((c >> 18) & 0x3F));
        output[3] = (char) (0x80 + ((c >> 12) & 0x3F));
        output[4] = (char) (0x80 + ((c >> 6)  & 0x3F));
        output[5] = (char) (0x80 + (c & 0x3F));
        output[6] = '\0';
        return 6;
    }
    return 6;
}

FCITX_EXPORT_API
char *
fcitx_utf8_get_char(const char *i, uint32_t *chr)
{
    const unsigned char* in = (const unsigned char *)i;
    if (!(in[0] & 0x80)) {
        *(chr) = *(in);
        return (char *)in + 1;
    }

    /* 2-byte, 0x80-0x7ff */
    if ((in[0] & 0xe0) == 0xc0 && CONT(1)) {
        *chr = ((in[0] & 0x1f) << 6) | VAL(1, 0);
        return (char *)in + 2;
    }

    /* 3-byte, 0x800-0xffff */
    if ((in[0] & 0xf0) == 0xe0 && CONT(1) && CONT(2)) {
        *chr = ((in[0] & 0xf) << 12) | VAL(1, 6) | VAL(2, 0);
        return (char *)in + 3;
    }

    /* 4-byte, 0x10000-0x1FFFFF */
    if ((in[0] & 0xf8) == 0xf0 && CONT(1) && CONT(2) && CONT(3)) {
        *chr = ((in[0] & 0x7) << 18) | VAL(1, 12) | VAL(2, 6) | VAL(3, 0);
        return (char *)in + 4;
    }

    /* 5-byte, 0x200000-0x3FFFFFF */
    if ((in[0] & 0xfc) == 0xf8 && CONT(1) && CONT(2) && CONT(3) && CONT(4)) {
        *chr = ((in[0] & 0x3) << 24) | VAL(1, 18) | VAL(2, 12) | VAL(3, 6) | VAL(4, 0);
        return (char *)in + 5;
    }

    /* 6-byte, 0x400000-0x7FFFFFF */
    if ((in[0] & 0xfe) == 0xfc && CONT(1) && CONT(2) && CONT(3) && CONT(4) && CONT(5)) {
        *chr = ((in[0] & 0x1) << 30) | VAL(1, 24) | VAL(2, 18) | VAL(3, 12) | VAL(4, 6) | VAL(5, 0);
        return (char *)in + 6;
    }

    *chr = *in;

    return (char *)in + 1;
}

FCITX_EXPORT_API
int fcitx_utf8_strncmp(const char *s1, const char *s2, int n)
{
    // Seems to work.
    uint32_t c1, c2;
    int i;

    for (i = 0; i < n; i++) {
        if (!(*s1 & 0x80)) {
            if (*s1 != *s2)
                return 1;

            if (*s1 == 0)
                return 0;

            s1 ++;

            s2 ++;
        } else {
            s1 = fcitx_utf8_get_char(s1, &c1);
            s2 = fcitx_utf8_get_char(s2, &c2);

            if (c1 != c2)
                return 1;
        }
    }

    return 0;
}

FCITX_EXPORT_API
char* fcitx_utf8_get_nth_char(char* s, size_t n)
{
    unsigned int l = 0;

    while (*s && l < n) {
        uint32_t chr;

        s = fcitx_utf8_get_char(s, &chr);
        l++;
    }

    return s;
}

FCITX_EXPORT_API
int
fcitx_utf8_get_char_extended(const char *s,
                             int max_len)
{
    const unsigned char*p = (const unsigned char*)s;
    int i, len;
    unsigned int wc = (unsigned char) * p;

    if (wc < 0x80) {
        return wc;
    } else if (wc < 0xc0) {
        return (unsigned int) - 1;
    } else if (wc < 0xe0) {
        len = 2;
        wc &= 0x1f;
    } else if (wc < 0xf0) {
        len = 3;
        wc &= 0x0f;
    } else if (wc < 0xf8) {
        len = 4;
        wc &= 0x07;
    } else if (wc < 0xfc) {
        len = 5;
        wc &= 0x03;
    } else if (wc < 0xfe) {
        len = 6;
        wc &= 0x01;
    } else {
        return (unsigned int) - 1;
    }

    if (max_len >= 0 && len > max_len) {
        for (i = 1; i < max_len; i++) {
            if ((((unsigned char *)p)[i] & 0xc0) != 0x80)
                return (unsigned int) - 1;
        }

        return (unsigned int) - 2;
    }

    for (i = 1; i < len; ++i) {
        unsigned int ch = ((unsigned char *)p)[i];

        if ((ch & 0xc0) != 0x80) {
            if (ch)
                return (unsigned int) - 1;
            else
                return (unsigned int) - 2;
        }

        wc <<= 6;

        wc |= (ch & 0x3f);
    }

    if (UTF8_LENGTH(wc) != len)
        return (unsigned int) - 1;

    return wc;
}

FCITX_EXPORT_API
int fcitx_utf8_get_char_validated(const char *p,
                                  int max_len)
{
    int result;

    if (max_len == 0)
        return -2;

    result = fcitx_utf8_get_char_extended(p, max_len);

    if (result & 0x80000000)
        return result;
    else if (!UNICODE_VALID(result))
        return -1;
    else
        return result;
}

FCITX_EXPORT_API
int fcitx_utf8_check_string(const char *s)
{
    while (*s) {
        uint32_t chr;

        if (fcitx_utf8_get_char_validated(s, 6) < 0)
            return 0;

        s = fcitx_utf8_get_char(s, &chr);
    }

    return 1;
}

FCITX_EXPORT_API
void fcitx_utf8_strncpy(char* str, const char* s, size_t byte)
{
    while (*s) {
        uint32_t chr;

        const char* next = fcitx_utf8_get_char(s, &chr);
        size_t diff = next - s;
        if (byte < diff)
            break;

        memcpy(str, s, diff);
        str += diff;
        byte -= diff;
        s = next;
    }

    while(byte --) {
        *str = '\0';
        str++;
    }
}

FCITX_EXPORT_API
size_t fcitx_utf8_strnlen(const char* str, size_t byte)
{
    size_t len = 0;
    while (*str && byte > 0) {
        uint32_t chr;

        const char* next = fcitx_utf8_get_char(str, &chr);
        size_t diff = next - str;
        if (byte < diff)
            break;

        str += diff;
        byte -= diff;
        str = next;
        len ++ ;
    }
    return len;
}

FCITX_EXPORT_API
char*
fcitx_utils_get_ascii_partn(char *string, ssize_t len)
{
    if (!string)
        return NULL;

    if (len < 0)
        len = strlen(string);
    char *s = string + len;
    while ((--s) >= string && !(*s & 0x80)) {
    }
    return s + 1;
}

FCITX_EXPORT_API
char*
fcitx_utils_get_ascii_part(char *string)
{
    return fcitx_utils_get_ascii_partn(string, -1);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
