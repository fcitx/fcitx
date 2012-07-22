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
#include <sys/mman.h>
#include <fcntl.h>

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
    char *content;
    off_t flen;
    if (spell->custom_map) {
        munmap(spell->custom_map, spell->custom_map_len);
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
    flen = stat_buf.st_size;
    content = mmap(NULL, flen, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (content == MAP_FAILED)
        return;
    spell->custom_map = content;
    spell->custom_map_len = flen;
}

/* static for now */
static void
SpellCustomLoadDict(FcitxSpell *spell)
{
    int i;
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
            empty_line = true;
            break;
        case ' ':
        case '\t':
        case '\r':
            break;
        default:
            if (empty_line) {
                empty_line = false;
                lcount++;
            }
        }
    }
    if (!spell->custom_map) {
        spell->custom_map = malloc(lcount * sizeof(SpellCustomWord));
    } else {
        spell->custom_map = realloc(spell->custom_map,
                                    lcount * sizeof(SpellCustomWord));
    }
    if (!spell->custom_map)
        return;
}

boolean
SpellCustomInit(FcitxSpell *spell)
{
    SpellCustomLoadDict(spell);
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
