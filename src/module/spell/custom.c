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

#include "fcitx/fcitx.h"
#include "config.h"

#include <unicode/unorm.h>

#include "fcitx/ime.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "spell-internal.h"
#define EN_DIC_FILE "/data/en_dic.txt"

static void
SpellCustomMapDict(FcitxSpell *spell)
{
    /* Ignore dictLang for now. */
    int fd;
    struct stat stat_buf;
    char *path;
    char *fname;
    off_t flen = 0;
    if (spell->custom_map) {
        free(spell->custom_map);
        spell->custom_map = NULL;
        spell->custom_map_len = 0;
    }
    path = fcitx_utils_get_fcitx_path("pkgdatadir");
    asprintf(&fname, "%s"EN_DIC_FILE, path);
    free(path);
    fd = open(fname, O_RDONLY);
    free(fname);
    if (fd == -1)
        return;
    if (fstat(fd, &stat_buf) == -1) {
        close(fd);
        return;
    }
    spell->custom_map = fcitx_utils_malloc0(stat_buf.st_size + 1);
    if (!spell->custom_map) {
        close(fd);
        return;
    }
    while (flen < stat_buf.st_size) {
        int c;
        c = read(fd, spell->custom_map, stat_buf.st_size - flen);
        if (c <= 0)
            break;
        flen += c;
        if (flen >= stat_buf.st_size)
            break;
    }
    if (!flen) {
        close(fd);
        free(spell->custom_map);
        spell->custom_map = NULL;
        return;
    } else if (flen < stat_buf.st_size) {
        spell->custom_map = realloc(spell->custom_map, flen + 1);
        spell->custom_map_len = flen;
    } else {
        spell->custom_map_len = stat_buf.st_size;
    }
}

static int
spell_strchomp(char *str)
{
    int len;
    int i;
    i = len = strlen(str);
    /* ok since the result string will never be empty in our case */
    while (--i) {
        switch (str[i]) {
        case ' ':
        case '\r':
        case '\t':
            str[i] = '\0';
            continue;
        default:
            break;
        }
        break;
    }
    return len;
}

/* static for now */
static void
SpellCustomLoadDict(FcitxSpell *spell)
{
    int i;
    int j;
    boolean empty_line = true;
    int lcount = 0;
    SpellCustomMapDict(spell);
    if (!spell->custom_map) {
        if (spell->custom_words) {
            free(spell->custom_words);
            spell->custom_words = NULL;
        }
        return;
    }
    for (i = 0;i < spell->custom_map_len;i++) {
        switch (spell->custom_map[i]) {
        case '\n':
            spell->custom_map[i] = '\0';
        case '\0':
            empty_line = true;
            break;
        case ' ':
        case '\t':
        case '\r':
            if (empty_line) {
                spell->custom_map[i] = '\0';
            }
        default:
            if (empty_line) {
                empty_line = false;
                lcount++;
            }
        }
    }
    if (!spell->custom_words) {
        spell->custom_words = malloc(lcount * sizeof(char*));
    } else {
        spell->custom_words = realloc(spell->custom_words,
                                      lcount * sizeof(char*));
    }
    if (!spell->custom_words)
        return;
    empty_line = true;
    for (i = 0, j = 0;i < spell->custom_map_len && j < lcount;i++) {
        if (!spell->custom_map[i])
            continue;
        spell->custom_words[j] = spell->custom_map + i;
        j++;
        i += spell_strchomp(spell->custom_map + i);
    }
    spell->custom_words_count = j;
}

boolean
SpellCustomInit(FcitxSpell *spell)
{
    /* struct timespec start, end; */
    /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); */
    SpellCustomLoadDict(spell);
    /* clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end); */
    /* int t = ((end.tv_sec - start.tv_sec) * 1000000000) */
    /*     + end.tv_nsec - start.tv_nsec; */
    /* printf("%s, %d\n", __func__, t); */
    return true;
}

SpellHint*
SpellCustomHintWords(FcitxSpell *spell, unsigned int len_limit)
{
    return NULL;
}

boolean
SpellCustomCheck(FcitxSpell *spell)
{
    return false;
}

void
SpellCustomDestroy(FcitxSpell *spell)
{
    if (spell->custom_map)
        free(spell->custom_map);
    if (spell->custom_words)
        free(spell->custom_words);
}
