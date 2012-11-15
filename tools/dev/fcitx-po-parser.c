/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <fcitx-utils/utils.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/utarray.h>
#include <gettext-po.h>

static void
_xerror_handler(int severity, po_message_t message, const char *filename,
                size_t lineno, size_t column, int multiline_p,
                const char *message_text)
{
    if (severity == PO_SEVERITY_FATAL_ERROR) {
        exit(1);
    }
}

static void
_xerror2_handler(int severity, po_message_t message1, const char *filename1,
                 size_t lineno1, size_t column1, int multiline_p1,
                 const char *message_text1, po_message_t message2,
                 const char *filename2, size_t lineno2, size_t column2,
                 int multiline_p2, const char *message_text2)
{
    if (severity == PO_SEVERITY_FATAL_ERROR) {
        exit(1);
    }
}

static int
str_base64_write(unsigned char *out, unsigned char c,
                 boolean escape, boolean escape_cap)
{
    c &= 63;
    if (c < 26) {
        if (escape_cap) {
            out[0] = '_';
            out[1] = 'a' + c;
            return 2;
        } else {
            out[0] = 'A' + c;
            return 1;
        }
    } else if (c < 52) {
        out[0] = 'a' + c - 26;
        return 1;
    } else if (c < 62) {
        out[0] = '0' + c - 52;
        return 1;
    } else if (c < 63) {
        if (escape) {
            out[0] = '_';
            out[1] = '2';
            return 2;
        } else {
            out[0] = '+';
            return 1;
        }
    } else {
        if (escape) {
            out[0] = '_';
            out[1] = '3';
            return 2;
        } else {
            out[0] = '/';
            return 1;
        }
    }
}

static int
str_base64_write_end(unsigned char *out, boolean escape)
{
    if (escape) {
        out[0] = '_';
        out[1] = '1';
        return 2;
    } else {
        out[0] = '=';
        return 1;
    }
}

static int
str_base64_escape(const char *in, char *out, boolean escape,
                  boolean escape_cap)
{
    unsigned char left = 0;
    int left_bits = 0;
    const unsigned char *uin = (const unsigned char*)in;
    unsigned char *uout = (unsigned char*)out;
    unsigned char res;
    for (;*uin;uin++) {
        res = left << (6 - left_bits) | *uin >> (2 + left_bits);
        uout += str_base64_write(uout, res, escape, escape_cap);
        left = *uin;
        left_bits += 2;
        if (left_bits == 6) {
            left_bits = 0;
            uout += str_base64_write(uout, left, escape, escape_cap);
        }
    }
    if (left_bits) {
        res = left << (6 - left_bits);
        uout += str_base64_write(uout, res, escape, escape_cap);
        uout += str_base64_write_end(uout, escape);
        if (left_bits <= 2) {
            uout += str_base64_write_end(uout, escape);
        }
    }
    uout[0] = '\0';
    return (char*)uout - out;
}

static int
lang_to_prefix(const char *lang, char **out)
{
    int lang_len = strlen(lang);
    char *res = malloc(lang_len * 3 + 4);
    *out = res;
    int len = str_base64_escape(lang, res, true, true);
    strcpy(res + len, "___");
    return len + 3;
}

int
main(int argc, char **argv)
{
    struct po_xerror_handler handler = {
        .xerror = _xerror_handler,
        .xerror2 = _xerror2_handler
    };
    po_file_t po_file = po_file_read("-", &handler);
    const char *lang = argv[1];
    if (!lang)
        return 1;

    /* const char *header = po_file_domain_header(po_file, NULL); */
    po_message_iterator_t msg_iter = po_message_iterator(po_file, NULL);
    po_message_t msg;
    char *buff = NULL;
    int prefix_len = lang_to_prefix(lang, &buff);

    while ((msg = po_next_message(msg_iter))) {
        const char *msg_id = po_message_msgid(msg);
        const char *msg_str = po_message_msgstr(msg);
        int fuzzy = po_message_is_fuzzy(msg);
        int obsolete = po_message_is_obsolete(msg);
        if (fuzzy || obsolete || !msg_id || !*msg_id || !msg_str || !*msg_str)
            continue;
        int id_len = strlen(msg_id);
        int str_len = strlen(msg_str);
        buff = realloc(buff, (id_len + str_len) * 3 + prefix_len + 2);
        char *p = buff + prefix_len;
        p += str_base64_escape(msg_id, p, true, false);
        p[0] = '=';
        str_base64_escape(msg_str, p + 1, false, false);
        printf("%s\n", buff);
    }

    fcitx_utils_free(buff);
    po_message_iterator_free(msg_iter);
    po_file_free(po_file);
    return 0;
}
