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
#include "fcitx-utils/utf8.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <endian.h>

#include "spell-internal.h"
#include "spell-custom.h"
#include "spell-custom-dict.h"

#define EN_DICT_FORMAT "%s/data/%s_dict.fscd"

#define case_a_z case 'a': case 'b': case 'c': case 'd': case 'e':      \
case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':   \
case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's':   \
case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z'

#define case_A_Z case 'A': case 'B': case 'C': case 'D': case 'E':      \
case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':   \
case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':   \
case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z'

#define DICT_BIN_MAGIC "FSCD0000"

static inline uint32_t
load_le32(const void* p)
{
    return le32toh(*(uint32_t*)p);
}

#if 0
static inline uint16_t
load_le16(const void* p)
{
    return le16toh(*(uint16_t*)p);
}
#endif

static boolean
SpellCustomEnglishCompare(unsigned int c1, unsigned int c2)
{
    switch (c1) {
    case_A_Z:
        c1 += 'a' - 'A';
        break;
    case_a_z:
        break;
    default:
        return c1 == c2;
    }
    switch (c2) {
    case_A_Z:
        c2 += 'a' - 'A';
        break;
    case_a_z:
        break;
    default:
        break;
    }
    return c1 == c2;
}

static boolean
SpellCustomEnglishIsFirstCapital(const char *str)
{
    if (!str || !*str)
        return false;
    switch (*str) {
    case_A_Z:
        break;
    default:
        return false;
    }
    while (*(++str)) {
        switch (*str) {
        case_A_Z:
            return false;
        default:
            continue;
        }
    }
    return true;
}

static boolean
SpellCustomEnglishIsAllCapital(const char *str)
{
    if (!str || !*str)
        return false;
    do {
        switch (*str) {
        case_a_z:
            return false;
        default:
            continue;
        }
    } while (*(++str));
    return true;
}

enum {
    CUSTOM_DEFAULT,
    CUSTOM_FIRST_CAPITAL,
    CUSTOM_ALL_CAPITAL,
};

static int
SpellCustomEnglishCheck(const char *str)
{
    if (SpellCustomEnglishIsFirstCapital(str))
        return CUSTOM_FIRST_CAPITAL;
    if (SpellCustomEnglishIsAllCapital(str))
        return CUSTOM_ALL_CAPITAL;
    return CUSTOM_DEFAULT;
}

static void
SpellCustomEnglishUpperString(char *str)
{
    if (!str || !*str)
        return;
    do {
        switch (*str) {
        case_a_z:
            *str += 'A' - 'a';
            break;
        default:
            break;
        }
    } while (*(++str));
}

static void
SpellCustomEnglishComplete(SpellHint *hint, int type)
{
    switch (type) {
    case CUSTOM_ALL_CAPITAL:
        for (;hint->commit;hint++)
            SpellCustomEnglishUpperString(hint->commit);
        break;
    case CUSTOM_FIRST_CAPITAL:
        for (;hint->commit;hint++) {
            switch (*hint->commit) {
            case_a_z:
                *hint->commit += 'A' - 'a';
                break;
            default:
                break;
            }
        }
    default:
        break;
    }
}

/**
 * Open the dict file, return -1 if failed.
 **/
static int
SpellCustomGetSysDictFile(FcitxSpell *spell, const char *lang)
{
    int fd;
    char *path;
    char *fname = NULL;
    path = fcitx_utils_get_fcitx_path("pkgdatadir");
    asprintf(&fname, EN_DICT_FORMAT, path, lang);
    free(path);
    fd = open(fname, O_RDONLY);
    free(fname);
    return fd;
}

static off_t
SpellCustomMapDict(FcitxSpell *spell, SpellCustomDict *dict, const char *lang)
{
    int fd;
    struct stat stat_buf;
    off_t flen = 0;
    fd = SpellCustomGetSysDictFile(spell, lang);

    /* try to save whatever loaded. */
    if (fd == -1)
        return 0;
    if (fstat(fd, &stat_buf) == -1 || stat_buf.st_size <= sizeof(uint32_t)) {
        close(fd);
        return 0;
    }
    dict->map = malloc(stat_buf.st_size + 1);
    if (!dict->map) {
        close(fd);
        return 0;
    }
    do {
        int c;
        c = read(fd, dict->map, stat_buf.st_size - flen);
        if (c <= 0)
            break;
        flen += c;
    } while (flen < stat_buf.st_size);
    close(fd);
    dict->map[flen] = '\0';
    return flen;
}

static boolean
SpellCustomInitDict(FcitxSpell *spell, SpellCustomDict *dict, const char *lang)
{
    int i;
    int j;
    off_t map_len;
    int lcount;
    if (!lang || !lang[0])
        return false;
    if (SpellLangIsLang(lang, "en")) {
        dict->word_comp_func = SpellCustomEnglishCompare;
        dict->word_check_func = SpellCustomEnglishCheck;
        dict->hint_cmplt_func = SpellCustomEnglishComplete;
    } else {
        dict->word_comp_func = NULL;
        dict->word_check_func = NULL;
        dict->hint_cmplt_func = NULL;
    }
    map_len = SpellCustomMapDict(spell, dict, lang);
    /* fail */
    if (map_len <= sizeof(uint32_t) + strlen(DICT_BIN_MAGIC))
        return false;

    lcount = load_le32(dict->map + strlen(DICT_BIN_MAGIC));
    dict->words = malloc(lcount * sizeof(char*));
    /* well, not likely though. */
    if (!dict->words)
        return false;

    /* save words pointers. */
    for (i = sizeof(uint16_t) + sizeof(uint32_t) + strlen(DICT_BIN_MAGIC), j = 0;
         i < map_len && j < lcount;i += sizeof(uint16_t) + 1) {
        int l = strlen(dict->map + i);
        if (!l)
            continue;
        dict->words[j++] = dict->map + i;
        i += l;
    }
    dict->words_count = j;
    return true;
}

SpellCustomDict*
SpellCustomNewDict(FcitxSpell *spell, const char *lang)
{
    SpellCustomDict *dict = fcitx_utils_new(SpellCustomDict);
    if (!SpellCustomInitDict(spell, dict, lang)) {
        SpellCustomFreeDict(spell, dict);
        return NULL;
    }
    return dict;
}

void
SpellCustomFreeDict(FcitxSpell *spell, SpellCustomDict *dict)
{
    if (dict->map)
        free(dict->map);
    if (dict->words)
        free(dict->words);
    free(dict);
}
