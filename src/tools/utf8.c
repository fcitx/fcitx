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

#include "core/fcitx.h"
#include "tools/utf8.h"

#define CONT(i)   ISUTF8_CB(in[i])
#define VAL(i, s) ((in[i]&0x3f) << s)

#define UTF8_LENGTH(Char)              \
  ((Char) < 0x80 ? 1 :                 \
   ((Char) < 0x800 ? 2 :               \
    ((Char) < 0x10000 ? 3 :            \
     ((Char) < 0x200000 ? 4 :          \
      ((Char) < 0x4000000 ? 5 : 6)))))

#define UNICODE_VALID(Char)                   \
    ((Char) < 0x110000 &&                     \
     (((Char) & 0xFFFFF800) != 0xD800) &&     \
     ((Char) < 0xFDD0 || (Char) > 0xFDEF) &&  \
     ((Char) & 0xFFFE) != 0xFFFE)

size_t
utf8_strlen(const char *s)
{
    unsigned int l = 0;
    
    while(*s)
    {
        int chr;
        
        s = utf8_get_char(s, &chr);
        l++;
    }
    
    return l;
}

int utf8_char_len(const char *in)
{
    if (!(in[0] & 0x80))
        return 1;
    /* 2-byte, 0x80-0x7ff */
    if ( (in[0]&0xe0) == 0xc0 && CONT(1) )
        return 2;
    /* 3-byte, 0x800-0xffff */
    if ( (in[0]&0xf0) == 0xe0 && CONT(1) && CONT(2) )
        return 3;
    /* 4-byte, 0x10000-0x1FFFFF */
    if ( (in[0]&0xf8) == 0xf0 && CONT(1) && CONT(2) && CONT(3) )
        return 4;
    /* 5-byte, 0x200000-0x3FFFFFF */
    if ( (in[0]&0xfc) == 0xf8 && CONT(1) && CONT(2) && CONT(3) && CONT(4) )
        return 5;
    /* 6-byte, 0x400000-0x7FFFFFF */
    if ( (in[0]&0xfe) == 0xfc && CONT(1) && CONT(2) && CONT(3) && CONT(4) && CONT(5) )
        return 6;
    
    return 1;
}

char *
utf8_get_char(const char *in, int *chr)
{
    if (!(in[0] & 0x80))
    {
        *(chr) = *(in);
        return (char *)in+1;
    }
    /* 2-byte, 0x80-0x7ff */
    if ( (in[0]&0xe0) == 0xc0 && CONT(1) )
    {
        *chr = ((in[0]&0x1f) << 6)|VAL(1,0);
        return (char *)in+2;
    }
    /* 3-byte, 0x800-0xffff */
    if ( (in[0]&0xf0) == 0xe0 && CONT(1) && CONT(2) )
    {
        *chr = ((in[0]&0xf) << 12)|VAL(1,6)|VAL(2,0);
        return (char *)in+3;
    }
    /* 4-byte, 0x10000-0x1FFFFF */
    if ( (in[0]&0xf8) == 0xf0 && CONT(1) && CONT(2) && CONT(3) )
    {
        *chr = ((in[0]&0x7) << 18)|VAL(1,12)|VAL(2,6)|VAL(3,0);
        return (char *)in+4;
    }
    /* 5-byte, 0x200000-0x3FFFFFF */
    if ( (in[0]&0xfc) == 0xf8 && CONT(1) && CONT(2) && CONT(3) && CONT(4) )
    {
        *chr = ((in[0]&0x3) << 24)|VAL(1,18)|VAL(2,12)|VAL(3,6)|VAL(4,0);
        return (char *)in+5;
    }
    /* 6-byte, 0x400000-0x7FFFFFF */
    if ( (in[0]&0xfe) == 0xfc && CONT(1) && CONT(2) && CONT(3) && CONT(4) && CONT(5) )
    {
        *chr = ((in[0]&0x1) << 30)|VAL(1,24)|VAL(2,18)|VAL(3,12)|VAL(4,6)|VAL(5,0);
        return (char *)in+6;
    }
    
    *chr = *in;
    
    return (char *)in+1;
}

int utf8_strncmp(const char *s1, const char *s2, int n) {
    // Seems to work.
    int c1,c2;
    int i;
    for (i=0;i < n;i++) {
        if (!(*s1 && 0x80))
        {
            if (*s1 != *s2)
                return 1;
            if (*s1 == 0)
                return 0;
            s1 ++;
            s2 ++;
        }
        else {
            s1 = utf8_get_char(s1, &c1);
            s2 = utf8_get_char(s2, &c2);
            if (c1 != c2)
                return 1;
        }
    }
    return 0;
}

char* utf8_get_nth_char(char* s, unsigned int n)
{
    unsigned int l = 0;
    
    while(*s && l < n)
    {
        int chr;
        
        s = utf8_get_char(s, &chr);
        l++;
    }

    return s;
}

int
utf8_get_char_extended (const char *s,
        int max_len)
{
    const unsigned char*p = (const unsigned char*)s;
    int i, len;
    unsigned int wc = (unsigned char) *p;
    
    if (wc < 0x80) {
        return wc;
    } else if (wc < 0xc0) {
        return (unsigned int)-1;
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
        return (unsigned int)-1;
    }
    
    if (max_len >= 0 && len > max_len) {
        for (i = 1; i < max_len; i++) {
            if ((((unsigned char *)p)[i] & 0xc0) != 0x80)
                return (unsigned int)-1;
        }
        return (unsigned int)-2;
    }
    
    for (i = 1; i < len; ++i) {
        unsigned int ch = ((unsigned char *)p)[i];
        
        if ((ch & 0xc0) != 0x80) {
            if (ch)
                return (unsigned int)-1;
            else
                return (unsigned int)-2;
        }
        wc <<= 6;
        wc |= (ch & 0x3f);
    }
    
    if (UTF8_LENGTH(wc) != len)
        return (unsigned int)-1;
    
    return wc;
}

int utf8_get_char_validated (const char *p,
        int max_len)
{
    int result;
    
    if (max_len == 0)
        return -2;
    
    result = utf8_get_char_extended (p, max_len);
    if (result & 0x80000000)
        return result;
    else if (!UNICODE_VALID (result))
        return -1;
    else
        return result;
}
int utf8_check_string(const char *s)
{
    while(*s)
    {
        int chr;
    
        if (utf8_get_char_validated(s, 6) < 0)
            return 0;
        s = utf8_get_char(s, &chr);
    }
    return 1;    
}

/* Modeline for ViM {{{
 * vim: ts=4 et sw=4
 * }}} */


