/***************************************************************************
 *   Copyright (C) 2012~2013 by Yichao Yu                                  *
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
#include <fcitx/fcitx.h>
#include <fcitx-utils/utils.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/utarray.h>
#include "config.h"

#ifndef _FCITX_DISABLE_GETTEXT

#include <gettext-po.h>

static void
_xerror_handler(int severity, po_message_t message, const char *filename,
                size_t lineno, size_t column, int multiline_p,
                const char *message_text)
{
    FCITX_UNUSED(message);
    FCITX_UNUSED(filename);
    FCITX_UNUSED(lineno);
    FCITX_UNUSED(column);
    FCITX_UNUSED(multiline_p);
    FCITX_UNUSED(message_text);
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
    FCITX_UNUSED(message1);
    FCITX_UNUSED(filename1);
    FCITX_UNUSED(lineno1);
    FCITX_UNUSED(column1);
    FCITX_UNUSED(multiline_p1);
    FCITX_UNUSED(message_text1);
    FCITX_UNUSED(message2);
    FCITX_UNUSED(filename2);
    FCITX_UNUSED(lineno2);
    FCITX_UNUSED(column2);
    FCITX_UNUSED(multiline_p2);
    FCITX_UNUSED(message_text2);
    if (severity == PO_SEVERITY_FATAL_ERROR) {
        exit(1);
    }
}
static const struct po_xerror_handler handler = {
    .xerror = _xerror_handler,
    .xerror2 = _xerror2_handler
};

#endif

static inline void
encode_char(char *out, char c)
{
    unsigned char *uout = (unsigned char*)out;
    unsigned char uc = (unsigned char)c;
    uout[0] = 'a' + (uc & 0x0f);
    uout[1] = 'a' + (uc >> 4);
}

static void
encode_string(const char *in, char *out, size_t l)
{
    size_t i;
    for (i = 0;i < l;i++) {
        encode_char(out + i * 2, in[i]);
    }
    out[l * 2] = '\0';
}

#ifndef _FCITX_DISABLE_GETTEXT

static int
lang_to_prefix(const char *lang, char **out)
{
    int lang_len = strlen(lang);
    char *res = malloc(lang_len * 2 + 4);
    *out = res;
    int len = lang_len * 2;
    encode_string(lang, res, lang_len);
    strcpy(res + len, "___");
    return len + 3;
}

static void
parse_po(const char *lang, const char *fname)
{
    po_file_t po_file = po_file_read(fname, &handler);
    po_message_iterator_t msg_iter = po_message_iterator(po_file, NULL);
    po_message_t msg;
    char *buff = NULL;
    size_t buff_len = 0;
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
        unsigned int len = (id_len + str_len) * 2 + prefix_len + 2;
        if (len > buff_len) {
            buff_len = len;
            buff = realloc(buff, buff_len);
        }
        char *p = buff + prefix_len;
        encode_string(msg_id, p, id_len);
        p += id_len * 2;
        p[0] = '=';
        encode_string(msg_str, p + 1, str_len);
        fwrite(buff, 1, len - 1, stdout);
        fwrite("\n", 1, 1, stdout);
    }

    fcitx_utils_free(buff);
    po_message_iterator_free(msg_iter);
    po_file_free(po_file);
}

static void
fix_header(const char *header, const char *fname)
{
    po_file_t po_file = po_file_read(fname, &handler);
    po_message_iterator_t msg_iter = po_message_iterator(po_file, NULL);
    po_message_t msg;

    while ((msg = po_next_message(msg_iter))) {
        const char *msg_id = po_message_msgid(msg);
        int fuzzy = po_message_is_fuzzy(msg);
        if (*msg_id || !fuzzy)
            continue;
        po_message_set_msgstr(msg, header);
    }

    po_message_iterator_free(msg_iter);
    po_file_write(po_file, "-", &handler);
    po_file_free(po_file);
}

#else

static void
parse_po(const char *lang, const char *fname)
{
    FCITX_UNUSED(lang);
    FCITX_UNUSED(fname);
}

static void
fix_header(const char *header, const char *fname)
{
    FCITX_UNUSED(header);
    FCITX_UNUSED(fname);
}

#endif

static void
encode_stdin()
{
#define ENCODE_BUF_SIZE (1024)
    char in_buff[ENCODE_BUF_SIZE];
    char out_buff[ENCODE_BUF_SIZE * 2];
    size_t len;
    while ((len = fread(in_buff, 1, ENCODE_BUF_SIZE, stdin))) {
        encode_string(in_buff, out_buff, len);
        fwrite(out_buff, 2, len, stdout);
    }
}

static inline int
check_char(char c)
{
    return c >= 'a' && c < 'a' + 0x10;
}

static inline size_t
check_buff(char *buff, size_t len)
{
    size_t i;
    for (i = 0;i < len;i++) {
        if (check_char(buff[i]))
            continue;
        len--;
        if (len <= i)
            continue;
        memmove(buff + i, buff + i + 1, len - i);
        i--;
    }
    return len;
}

static inline char
hex_to_char(const char c[2])
{
    return (c[0] - 'a') + ((c[1] - 'a') << 4);
}

static inline void
decode_stdin()
{
#define DECODE_BUF_SIZE (1024)
    char out_buff[DECODE_BUF_SIZE];
    char in_buff[DECODE_BUF_SIZE * 2];
    size_t len;
    size_t left = 0;
    size_t count = 0;
    size_t i;
    while ((len = fread(in_buff + left, 1, DECODE_BUF_SIZE * 2, stdin))) {
        len = check_buff(in_buff, len + left);
        count = len / 2;
        for (i = 0;i < count;i++) {
            out_buff[i] = hex_to_char(in_buff + i * 2);
        }
        fwrite(out_buff, count, 1, stdout);
        if ((left = len % 2)) {
            in_buff[0] = in_buff[len - 1];
        }
    }
}

static inline const char*
std_filename(const char *fname)
{
    return (fname && *fname) ? fname : "-";
}

int
main(int argc, char **argv)
{
    if (argc <= 1)
        return 1;
    const char *action = argv[1];
    if (strcmp(action, "--parse-po") == 0) {
        const char *lang = argv[2];
        if (!lang)
            return 1;
        parse_po(lang, std_filename(argv[3]));
    } else if (strcmp(action, "--encode") == 0) {
        encode_stdin();
    } else if (strcmp(action, "--decode") == 0) {
        decode_stdin();
    } else if (strcmp(action, "--fix-header") == 0) {
        const char *header = argv[2];
        if (!header)
            return 1;
        fix_header(header, std_filename(argv[3]));
    } else if (strcmp(action, "--gettext-support") == 0) {
#ifndef _FCITX_DISABLE_GETTEXT
        return 0;
#else
        return 1;
#endif
    } else {
        return 1;
    }
    return 0;
}
